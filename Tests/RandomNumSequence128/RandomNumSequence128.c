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
//	RandomNumSequence128.c
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 12 [Random number sequence] - This test writes a series of 
//  random numbers into memory.
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
#include <Library/Rand.h>
#include <Library/Cpuid.h>

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
#include <xmmintrin.h>	//SSE intrinsics
#include <emmintrin.h>	//SSE2 intrinsics
#include <smmintrin.h>  //SSE4.1 intrinsics
#elif defined(MDE_CPU_AARCH64)
#include <sse2neon.h>
#endif

#ifdef __GNUC__
#        define ALIGN(X) __attribute__ ((aligned(X)))
#elif defined(_MSC_VER)
#        define ALIGN(X) __declspec(align(X))
#else
#        error Unknown compiler, unknown alignment attribute!
#endif

extern struct cpu_ident cpu_id;					// results of execution cpuid instruction
extern BOOLEAN gNTLoadSupported;

BOOLEAN
EnableSSE ();

// #pragma optimize( "", off ) //Optimisation must be turned off 
EFI_STATUS
EFIAPI
RunRandomNumSequence128Test (
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
    ALIGN(16) unsigned int bad[8];
    ALIGN(16) unsigned int num[8];
    ALIGN(16) unsigned int cmpres[8];
    ALIGN(16) const unsigned int XOR_AND_MASK[8] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

    // workaround for unaligned stack. SSE variables must be 16-byte aligned
    __m128i					*pbad = (__m128i *)(((UINTN)bad + 0xF) & ~(0xF));
    __m128i					*pnum = (__m128i *)(((UINTN)num + 0xF) & ~(0xF));
    __m128i					*pcmpres = (__m128i *)(((UINTN)cmpres + 0xF) & ~(0xF));
    __m128i					*pXOR_AND_MASK = (__m128i *)(((UINTN)XOR_AND_MASK + 0xF) & ~(0xF));

    EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;
    int						cpu;
    UINT64                  IterNumber;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint(L"[Proc %d] Testing Memory Range: 0x%016lx - 0x%016lx\n", ProcNum, Start, End);

    if (Start == End)
        return EFI_SUCCESS;

    cpu = (int)ProcNum;

    EnableSSE();

    for (IterNumber = 0; IterNumber < IterCount; IterNumber++)
    {
        int i, seed;
        
        __m128i *pm128Cur = (__m128i *)(UINTN)Start;
        __m128i *pm128End = (__m128i *)(UINTN)End;

        if (ProcNum == MPSupportGetBspId())
            MtSupportTestTick(FALSE);

        if (MtSupportCheckTestAbort())
            return EFI_ABORTED;

        /* Initialize memory with initial sequence of random numbers.  */
        seed = rand(cpu);

        MtUiDebugPrint(L"[Proc %d] Writing random paterns to memory (Iter %d, 0x%016lx - 0x%016lx)\n", ProcNum, IterNumber, Start, End);

        if (ProcNum == MPSupportGetBspId())
        {
            rand_seed_sse(seed, cpu);
            rand_sse(cpu, (unsigned int *)pnum);
            MtUiSetPattern128((unsigned int *)pnum);
        }

        rand_seed_sse(seed, cpu);

        /* Computer random 128-bit number and store in memory*/
        for (pm128Cur = (__m128i *)(UINTN)Start; pm128Cur < pm128End; pm128Cur++) {
            rand_sse(cpu, (unsigned int *)pnum);

            if (IterNumber % 2 == 0)
                _mm_stream_si128(pm128Cur, *pnum);
            else
                _mm_store_si128(pm128Cur, *pnum);
        }			

        MtUiDebugPrint(L"[Proc %d] Check for initial pattern, then write the complement (Iter %d, 0x%016lx - 0x%016lx)\n", ProcNum, IterNumber, Start, End);
        /* Check for initial pattern and then
         * write the complement for each memory location. */
        for (i=0; i<2; i++)
        {
            rand_seed_sse(seed, cpu);

            for (pm128Cur = (__m128i *)(UINTN)Start; pm128Cur < pm128End; pm128Cur++) {
                unsigned short resmask;

                // Read the 128-bit value from memory
                if (IterNumber % 2 == 0 && gNTLoadSupported)
                    *pbad = _mm_stream_load_si128(pm128Cur);
                else
                    *pbad = _mm_load_si128(pm128Cur);
                
                // Get the expected 128-bit pattern
                rand_sse(cpu, (unsigned int *)pnum);

                // If second interation, check complement of pattern
                if (i == 1) *pnum = _mm_xor_si128(*pnum, *pXOR_AND_MASK);

                // Compare expected and actual value
                *pcmpres = _mm_cmpeq_epi32(*pbad, *pnum); // if equal, pcmpres = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }
                resmask = (unsigned short)_mm_movemask_epi8(*pcmpres); // resmask = 0xffff if equal. Otherwise, not equal

                if (resmask != 0xffff)
                {
                    if (MtSupportCheckTestAbort())
						return EFI_ABORTED;

                    MtUiDebugPrint(L"[Proc %d] Actual value (%016lX%016lX) does not match expected value (%016lX%016lX) [Res:%016lX%016lX, Mask:%04X]\n",
                        ProcNum,
                        pbad->m128i_u64[1], pbad->m128i_u64[0],
                        pnum->m128i_u64[1], pnum->m128i_u64[0],
                        pcmpres->m128i_u64[1], pcmpres->m128i_u64[0],
                        resmask);

                    MtSupportReportErrorVarSize((UINTN)pm128Cur, TestNum, pbad, pnum, 16, 0);
                    Result = EFI_TEST_FAILED;
                }
                
                // If first iteration, write the complement of 128-bit pattern to memory
                if (i == 0)
                {
                    *pnum = _mm_xor_si128(*pnum, *pXOR_AND_MASK);

                    if (IterNumber % 2 == 0)
                        _mm_stream_si128(pm128Cur, *pnum);
                    else
                        _mm_store_si128(pm128Cur, *pnum);
                }

            }
        }
    }

    return Result;
}
// #pragma optimize( "", on ) //Optimisation must be turned off 
