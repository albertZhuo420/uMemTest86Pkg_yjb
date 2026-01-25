//PassMark MemTest86
//
//Copyright (c) 2013-2016
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
//Program:
//	MemTest86
//
//Module:
//	BlockMove.c
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 5 [Block move, 64 moves] - This test stresses memory by using 
//  block move (movsl) instructions and is based on Robert Redelmeier's 
//  burnBX test.
//
//  Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
//
//History
//	27 Feb 2013: Initial version

/* test.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */

#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>
#include <Library/MemTestUiLib.h>

#define SPINSZ		0x2000000

extern UINT32*
EFIAPI
AsmBlockMove (
    IN UINT32   *p,
    IN UINTN  Count
    );

#if defined(__GNUC__)
#if defined(MDE_CPU_X64)
static inline void __movsd(unsigned long* dst, unsigned long* src, UINTN wlen)
{
    asm __volatile__ (
        "mov %0,%%rsi\n\t" \
        "mov %1,%%rdi\n\t" \
        "mov %2,%%rcx\n\t" \
        "cld\n\t" \
        "rep\n\t" \
        "movsl\n\t" \
        :: "g" (src), "g" (dst), "g" (wlen)
        : "rsi", "rdi", "rcx"
    );
}
#elif defined(MDE_CPU_AARCH64)
static __inline VOID _arm_block_move(
    IN VOID* Destination,
    IN VOID* Source,
    IN UINTN  Count
)
{
    // Reference: https://github.com/ssvb/tinymembench/blob/master/aarch64-asm.S
    __asm__ __volatile__(
        "mov x0, %0" "\n\t" // r0 = pointer to destination block
        "mov x1, %1" "\n\t" // r1 = pointer to source block
        "mov x2, %2" "\n\t" // r2 = number of words to copy
        "loop:" "\n\t"
        "prfm        pldl1keep, [x1, #320]" "\n\t"
        "ldp         x3, x4, [x1, #0]" "\n\t"
        "ldp         x5, x6, [x1, #16]" "\n\t"
        "add         x1, x1, #32" "\n\t"
        "stp         x3, x4, [x0, #0]" "\n\t"
        "stp         x5, x6, [x0, #16]" "\n\t"
        "add         x0, x0, #32" "\n\t"
        "subs        x2, x2, #8" "\n\t"
        "bgt         loop" "\n\t"
        :
        : "r"(Destination), "r"(Source), "r" (Count)
        : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "memory"
    );

}
#endif
#elif defined(_MSC_VER)
extern  
VOID 
__movsd (
    IN VOID   *Destination,
    IN VOID   *Source,
    IN UINTN  Count
);
#endif

EFI_STATUS
EFIAPI
RunBlockMoveTest (
    IN UINT32                   TestNum,
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context
    )
{
    EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;
    UINT64                  IterNumber;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint(L"Testing Memory Range: 0x%016lx - 0x%016lx\n", Start, End);

    if (Start == End)
        return EFI_SUCCESS;
#if 0
    //
    // Enable cache
    //
    AsmEnableCache();
#endif
    
    {
        UINTN len;
        volatile UINT32 *p, *pe, *pp;
        volatile UINT32 *start, *end;

        MtUiDebugPrint(L"Writing intial pattern to memory\n");

        /* Initialize memory with the initial pattern.  */
        start = (UINT32 *)(UINTN)Start;
#ifdef USB_WAR
        /* We can't do the block move test on low memory beacuase
            * BIOS USB support clobbers location 0x410 and 0x4e0
            */
        if (start < 0x4f0) {
            start = 0x4f0;
        }
#endif
        end = (UINT32 *)(UINTN)End;
        pe = end;
        p = start;

        len  = ((UINTN)pe - (UINTN)p) / 64;

#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
        asm __volatile__ (
            "jmp L100\n\t"

            ".p2align 4,,7\n\t"
            "L100:\n\t"

            // First loop eax is 0x00000001, edx is 0xfffffffe
            "movl %%eax, %%edx\n\t"
            "notl %%edx\n\t"

            // Set a block of 64-bytes	// First loop DWORDS are 
            "movl %%eax,0(%%edi)\n\t"	// 0x00000001
            "movl %%eax,4(%%edi)\n\t"	// 0x00000001
            "movl %%eax,8(%%edi)\n\t"	// 0x00000001
            "movl %%eax,12(%%edi)\n\t"	// 0x00000001
            "movl %%edx,16(%%edi)\n\t"	// 0xfffffffe
            "movl %%edx,20(%%edi)\n\t"	// 0xfffffffe
            "movl %%eax,24(%%edi)\n\t"	// 0x00000001
            "movl %%eax,28(%%edi)\n\t"	// 0x00000001
            "movl %%eax,32(%%edi)\n\t"	// 0x00000001
            "movl %%eax,36(%%edi)\n\t"	// 0x00000001
            "movl %%edx,40(%%edi)\n\t"	// 0xfffffffe
            "movl %%edx,44(%%edi)\n\t"	// 0xfffffffe
            "movl %%eax,48(%%edi)\n\t"	// 0x00000001
            "movl %%eax,52(%%edi)\n\t"	// 0x00000001
            "movl %%edx,56(%%edi)\n\t"	// 0xfffffffe
            "movl %%edx,60(%%edi)\n\t"	// 0xfffffffe

            // rotate left with carry, 
            // second loop eax is		 0x00000002
            // second loop edx is (~eax) 0xfffffffd
            "rcll $1, %%eax\n\t"		
            
            // Move current position forward 64-bytes (to start of next block)
            "leal 64(%%edi), %%edi\n\t"

            // Loop until end
            "decl %%ecx\n\t"
            "jnz  L100\n\t"

            : "=D" (p)
            : "D" (p), "c" (len), "a" (1)
            : "edx"
        );
#elif defined(MDE_CPU_X64)
        asm __volatile__ (
            "jmp L100\n\t"

            ".p2align 4,,7\n\t"
            "L100:\n\t"

            // First loop eax is 0x00000001, edx is 0xfffffffe
            "movl %%eax, %%edx\n\t"
            "notl %%edx\n\t"

            // Set a block of 64-bytes	// First loop DWORDS are 
            "movl %%eax,0(%%rdi)\n\t"	// 0x00000001
            "movl %%eax,4(%%rdi)\n\t"	// 0x00000001
            "movl %%eax,8(%%rdi)\n\t"	// 0x00000001
            "movl %%eax,12(%%rdi)\n\t"	// 0x00000001
            "movl %%edx,16(%%rdi)\n\t"	// 0xfffffffe
            "movl %%edx,20(%%rdi)\n\t"	// 0xfffffffe
            "movl %%eax,24(%%rdi)\n\t"	// 0x00000001
            "movl %%eax,28(%%rdi)\n\t"	// 0x00000001
            "movl %%eax,32(%%rdi)\n\t"	// 0x00000001
            "movl %%eax,36(%%rdi)\n\t"	// 0x00000001
            "movl %%edx,40(%%rdi)\n\t"	// 0xfffffffe
            "movl %%edx,44(%%rdi)\n\t"	// 0xfffffffe
            "movl %%eax,48(%%rdi)\n\t"	// 0x00000001
            "movl %%eax,52(%%rdi)\n\t"	// 0x00000001
            "movl %%edx,56(%%rdi)\n\t"	// 0xfffffffe
            "movl %%edx,60(%%rdi)\n\t"	// 0xfffffffe

            // rotate left with carry, 
            // second loop eax is		 0x00000002
            // second loop edx is (~eax) 0xfffffffd
            "rcll $1, %%eax\n\t"		
            
            // Move current position forward 64-bytes (to start of next block)
            "lea 64(%%rdi), %%rdi\n\t"

            // Loop until end
            "dec %%rcx\n\t"
            "jnz  L100\n\t"

            : "=D" (p)
            : "D" (p), "c" (len), "a" (1)
            : "edx"
        );
#elif defined(MDE_CPU_AARCH64)
        __asm__ __volatile__(
            "mov x0, %0" "\n\t"
            "mov x1, %1" "\n\t"
            "ldr w2, =1" "\n\t"
            "L100:" "\n\t"
            "mvn w3, w2" "\n\t"
            "str w2, [x0, #0]" "\n\t"
            "str w2, [x0, #4]" "\n\t"
            "str w2, [x0, #8]" "\n\t"
            "str w2, [x0, #12]" "\n\t"
            "str w3, [x0, #16]" "\n\t"
            "str w3, [x0, #20]" "\n\t"
            "str w2, [x0, #24]" "\n\t"
            "str w2, [x0, #28]" "\n\t"
            "str w2, [x0, #32]" "\n\t"
            "str w2, [x0, #36]" "\n\t"
            "str w3, [x0, #40]" "\n\t"
            "str w3, [x0, #44]" "\n\t"
            "str w2, [x0, #48]" "\n\t"
            "str w2, [x0, #52]" "\n\t"
            "str w3, [x0, #56]" "\n\t"
            "str w3, [x0, #60]" "\n\t"
            "ror w2, w2, #31" "\n\t"
            "add x0, x0, #64" "\n\t"
            "subs x1, x1, #1" "\n\t"
            "bgt L100" "\n\t"
            "mov %0, x0" "\n\t"
            : "+r" (p)
            : "r" (len)
            : "x0", "x1", "w2", "w3", "memory"
            );
#endif
#elif _MSC_VER
#if defined(MDE_CPU_IA32) 
        _asm {
            mov edi,p
            mov ecx,len
            mov eax,1
            jmp L100

            ;.p2align 4,,7
            ALIGN 16
            L100:
            mov edx,eax
            not edx
            mov [edi+0],eax
            mov [edi+4],eax
            mov [edi+8],eax
            mov [edi+12],eax
            mov [edi+16],edx
            mov [edi+20],edx
            mov [edi+24],eax
            mov [edi+28],eax
            mov [edi+32],eax
            mov [edi+36],eax
            mov [edi+40],edx
            mov [edi+44],edx
            mov [edi+48],eax
            mov [edi+52],eax
            mov [edi+56],edx
            mov [edi+60],edx
            rcl eax,1
            lea edi,[edi+64]
            dec ecx
            jnz  L100
            mov p,edi
        }
#elif defined(MDE_CPU_X64)
        p = AsmBlockMove((UINT32 *)p, len);
#endif
#endif
        // do_tick();
        // BAILR

        MtUiDebugPrint(L"Performing block move\n");
        /* Now move the data around
         * First move the data up half of the segment size we are testing
         * Then move the data to the original location + 32 bytes
         */	
        start = (UINT32 *)(UINTN)Start;
#ifdef USB_WAR
        /* We can't do the block move test on low memory beacuase
            * BIOS USB support clobbers location 0x410 and 0x4e0
            */
        if (start < 0x4f0) {
            start = 0x4f0;
        }
#endif
        end = (UINT32 *)(UINTN)End;
        pe = end;
        p = start;

        pp = (UINT32*)((UINTN)p + (((UINTN)pe - (UINTN)p) / 2));
        len  = ((UINTN)pe - (UINTN)p) / 8;
        for (IterNumber = 0; IterNumber < IterCount; IterNumber++)
        {
            if (ProcNum == MPSupportGetBspId() && IterNumber & 0x1)
            {
                MtSupportTestTick(FALSE);
                MtUiSetPattern(*p);
            }

            if (MtSupportCheckTestAbort())
                return EFI_ABORTED;

#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
            asm __volatile__ (
                "cld\n"
                "jmp L110\n\t"

                ".p2align 4,,7\n\t"
                "L110:\n\t"

                //
                // At the end of all this 
                // - the second half equals the inital value of the first half
                // - the first half is right shifted 32-bytes (with wrapping)
                //

                // Move first half to second half
                "movl %1,%%edi\n\t" // Destionation, pp (mid point)
                "movl %0,%%esi\n\t" // Source, p (start point)
                "movl %2,%%ecx\n\t" // Length, len (size of a half in DWORDS)
                "rep\n\t"
                "movsl\n\t"

                // Move the second half, less the last 32-bytes. To the first half, offset plus 32-bytes
                "movl %0,%%edi\n\t"
                "addl $32,%%edi\n\t"	// Destination, p(start-point) plus 32 bytes
                "movl %1,%%esi\n\t"		// Source, pp(mid-point)
                "movl %2,%%ecx\n\t"
                "subl $8,%%ecx\n\t"		// Length, len(size of a half in DWORDS) minus 8 DWORDS (32 bytes)
                "rep\n\t"
                "movsl\n\t"

                // Move last 8 DWORDS (32-bytes) of the second half to the start of the first half
                "movl %0,%%edi\n\t"		// Destination, p(start-point)
                                        // Source, 8 DWORDS from the end of the second half, left over by the last rep/movsl
                "movl $8,%%ecx\n\t"		// Length, 8 DWORDS (32-bytes)
                "rep\n\t"
                "movsl\n\t"

                :: "g" (p), "g" (pp), "g" (len)
                : "edi", "esi", "ecx"
            );
#elif defined(MDE_CPU_X64)
            __movsd((void *)pp, (void *)p,len);
            __movsd((void*)(p+8), (void *)pp, len-8);
            __movsd((void*)p, (void *)(pe-8), 8);
#elif defined(MDE_CPU_AARCH64)
            _arm_block_move((void*)pp, (void*)p, len);
            _arm_block_move((void*)(p + 8), (void*)pp, len - 8);
            _arm_block_move((void*)p, (void*)(pe - 8), 8);
#endif
#elif _MSC_VER
#ifdef MDE_CPU_IA32
            _asm {
                cld
                jmp L110

                ;.p2align 4,,7
                ALIGN 16
                L110:
                mov edi,pp
                mov esi,p
                mov ecx,len
                rep	movsd
                mov edi,p
                add edi,32
                mov esi,pp
                mov ecx,len
                sub ecx,8
                rep movsd
                mov edi,p
                mov ecx,8
                rep	movsd
            }
#elif defined(MDE_CPU_X64)
            __movsd((void *)pp, (void *)p,len);
            __movsd((void*)(p+8), (void *)pp, len-8);
            __movsd((void*)p, (void *)(pe-8), 8);
#endif
#endif
        }

        MtUiDebugPrint(L"Checking data patterns\n");
        /* Now check the data
         * The error checking is rather crude.  We just check that the
         * adjacent words are the same.
         */
        start = (UINT32 *)(UINTN)Start;
#ifdef USB_WAR
        /* We can't do the block move test on low memory beacuase
            * BIOS USB support clobbers location 0x4e0 and 0x410
            */
        if (start < 0x4f0) {
            start = 0x4f0;
        }
#endif
        end = (UINT32 *)(UINTN)End;
        pe = end;
        p = start;

        for (; p < pe; p+=2) {
            UINT32 bad;
            if ((bad=*p) != *(p+1)) {
                if (MtSupportCheckTestAbort())
                    return EFI_ABORTED;

                MtSupportReportError32((UINTN)p, TestNum, bad, *(p + 1), 0);
                Result = EFI_TEST_FAILED;
            }
        }
    }
    
    return Result;
}