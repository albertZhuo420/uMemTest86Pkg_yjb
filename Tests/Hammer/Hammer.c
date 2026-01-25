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
//	Hammer.h
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 13 [Hammer test] - The hammer test initializes all of memory with a pattern, 
// hammers (ie. repeatedly read) from 2 memory addresses and then checks for flipped bits in memory
//
//
//History
//	13 Aug 2014: Initial version

#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>
#include <Library/MemTestUiLib.h>
#include <Library/Rand.h>

#include "Hammer.h"

#if 0 // Not used. For calculating delay using NOPs
static UINT32		mMinHammerTime = (UINT32)-1;
static UINT64		mNOPDelayMs = 0;
static UINT64		mNOPLoops = 0;
#endif
extern UINTN clks_msec;
VOID
EFIAPI
AsmHammerTest (
IN VOID   *Addr1,					/* 1st address of the row pair to hammer */
IN VOID   *Addr2,					/* 2nd address of the row pair to hammer */
IN UINT64    toggle_count,			/* total number of times to hammer the row pair */
IN UINT64    delay_ticks			/* number of rdtsc cycles to delay after each row pair hammer */
);

UINT64
EFIAPI
AsmNOPLatencyTest (
IN UINT64    TickCount
);

UINT64
EFIAPI
AsmRdtscLatencyTest (
);

EFI_STATUS
EFIAPI
HammerTest_hammer (
    IN UINT32                   TestNum, 
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context,
    IN BOOLEAN					SlowHammer
    )
{
    EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;
    UINT64					start_time, end_time;
#if 0
    UINT32					DelayRequired = 0;
    UINT64					NOPLoopsRequired = 0;
#endif
    int minoffsetbit = 18;
    int offsetbit = minoffsetbit;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint (L"Single-sided Hammering Memory Range: 0x%016lx - 0x%016lx (%ld KB)\n", Start, End, DivU64x32(Length,1024));

    if (Start == End)
        return EFI_SUCCESS;

#if 0 // Not used. For calculating delay using NOPs
    if (SlowHammer)
    {
        if (mNOPLoops == 0 || mNOPDelayMs == 0)
        {
            mNOPLoops = MultU64x32(clks_msec,500);
            // start_time = MtSupportGetTimeInMilliseconds();
            mNOPDelayMs = DivU64x32(AsmNOPLatencyTest( mNOPLoops ), clks_msec);
            // end_time = MtSupportGetTimeInMilliseconds();
            // mNOPDelayMs = end_time - start_time;
            MtUiDebugPrint (L"NOP latency test finished (%ldms in %ld loops)\n", mNOPDelayMs, mNOPLoops);
        }

        MtUiDebugPrint (L"Min hammer time: %d\n", mMinHammerTime);
        DelayRequired = 640 - mMinHammerTime;
        MtUiDebugPrint (L"Delay required: %ld\n", DelayRequired);
        NOPLoopsRequired = DivU64x64Remainder(MultU64x32(mNOPLoops, DelayRequired), MultU64x32(mNOPDelayMs, NUM_ROW_TOGGLES), NULL);

        MtUiDebugPrint (L"NOP delay loops required: %ld\n", NOPLoopsRequired);
    }
#endif

    {
        volatile UINT32 *start,*end;
        
        UINT64 rowskip = IterCount;
        UINTN addr1;
        
        /* Do the hammering  */
        start = (UINT32 *)(UINTN)Start;
        end = (UINT32 *)(UINTN)End;

        // test every row-pair
        for (addr1=(UINTN)start; addr1 < (UINTN)end; addr1 += (UINTN)rowskip) {
            int toggle_count;
            UINTN addr2;
            UINT64 delay_cycles;

            if (((UINT64)addr1 + LShiftU64(1, minoffsetbit)) >= (UINTN)end)
                break;
            
            if (((UINT64)addr1 + LShiftU64(1, offsetbit)) >= (UINTN)end)
                offsetbit = minoffsetbit;

            addr2 = addr1 + (UINTN)LShiftU64(1,offsetbit);

            offsetbit++;

            // Add delay cycles between each hammer if doing slow hammer
            delay_cycles = SlowHammer ? DivU64x32(MultU64x32(clks_msec, REFI_MS), HAMMER_RATE_REFI) : 0;

            MtUiDebugPrint (L"Begin hammering 0x%lx and 0x%lx (Offset: %d, Toggle count: %d, Delay cycles: %ld)\n", (UINT64)addr1, (UINT64)addr2, offsetbit - 1, NUM_ROW_TOGGLES, delay_cycles);

            if (ProcNum == MPSupportGetBspId())
                MtSupportTestTick(FALSE);

            if (MtSupportCheckTestAbort())
                return EFI_ABORTED;

            start_time = MtSupportGetTimeInMilliseconds();

#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {
                asm volatile("movl (%0), %%eax" : : "r" (addr1) : "eax");
                asm volatile("movl (%0), %%ebx" : : "r" (addr2) : "ebx");
                asm volatile("clflush (%0)" : : "r" (addr1) : "memory");
                asm volatile("clflush (%0)" : : "r" (addr2) : "memory");
                
                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = AsmReadTsc() + delay_cycles;
                    while(AsmReadTsc() < end_ts);
                }
            }
#elif defined(MDE_CPU_X64)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {
                asm volatile("movq (%0), %%rax" : : "r" (addr1) : "rax");
                asm volatile("movq (%0), %%rbx" : : "r" (addr2) : "rbx");
                asm volatile("clflush (%0)" : : "r" (addr1) : "memory");
                asm volatile("clflush (%0)" : : "r" (addr2) : "memory");
                
                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = AsmReadTsc() + delay_cycles;
                    while(AsmReadTsc() < end_ts);
                }
            }
#elif defined(MDE_CPU_AARCH64)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {				
                __asm__ __volatile__("ldr x0, [%0]" : : "r" (addr1) : "x0");
                __asm__ __volatile__("ldr x1, [%0]" : : "r" (addr2) : "x1");
                __asm__ __volatile__("dc civac, %0\n\t" : : "r" (addr1) : "memory");
                __asm__ __volatile__("dc civac, %0\n\t" : : "r" (addr2) : "memory");

                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = MtSupportReadTSC() + delay_cycles;
                    while(MtSupportReadTSC() < end_ts);
                }
            }
#endif
#elif defined(_MSC_VER)
#if defined(MDE_CPU_IA32)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {
                _asm {
                    mov edi,addr1
                    mov edx,addr2
                    jmp L60
                    ;.p2align 4,,7
                    ALIGN 16

                    L60:
                    mov eax,[edi]
                    mov ebx,[edx]
                    clflush [edi]	
                    clflush [edx]
                    ;mfence
                }
                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = AsmReadTsc() + delay_cycles;
                    while(AsmReadTsc() < end_ts);
                }
            }
#elif defined(MDE_CPU_X64)
            toggle_count = NUM_ROW_TOGGLES;
            AsmHammerTest((void *)addr1,(void *)addr2,(UINT32)toggle_count, delay_cycles);

#endif
#endif
            end_time = MtSupportGetTimeInMilliseconds();
            MtUiDebugPrint (L"Hammering 0x%lx and 0x%lx finished (%ldms), Current timestamp: %ld\n", (UINT64)addr1, (UINT64)addr2, end_time - start_time, MtSupportReadTSC());

#if 0 // Not used. Find the minimum time to complete each row pair hammering
            if (SlowHammer == FALSE)
            {
                if (end_time - start_time < mMinHammerTime)
                    mMinHammerTime = (UINT32)(end_time - start_time);
            }

            offsetbit ++;
            if (offsetbit > 24)
                offsetbit = 18;
#endif
        }
    }
    
    return Result;
}


EFI_STATUS
EFIAPI
RunHammerTest_hammer_fast (
    IN UINT32                   TestNum, 
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context)
{
    return HammerTest_hammer(TestNum, ProcNum,Start,Length,IterCount,Pattern,PatternUser,Context,FALSE);
}


EFI_STATUS
EFIAPI
RunHammerTest_hammer_slow (
    IN UINT32                   TestNum, 
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context)
{
    return HammerTest_hammer(TestNum, ProcNum,Start,Length,IterCount,Pattern,PatternUser,Context,TRUE);
}

EFI_STATUS
EFIAPI
HammerTest_double_sided_hammer (
    IN UINT32                   TestNum, 
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context,
    IN BOOLEAN					SlowHammer
    )
{
    EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;
    UINT64					start_time, end_time;
#if 0
    UINT32					DelayRequired = 0;
    UINT64					NOPLoopsRequired = 0;
#endif

    int minoffsetbit = 18;
    int offsetbit = minoffsetbit;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint (L"Double-sided Hammering Memory Range: 0x%016lx - 0x%016lx (%ld KB)\n", Start, End, DivU64x32(Length,1024));

    if (Start == End)
        return EFI_SUCCESS;

#if 0 // Not used. For calculating delay using NOPs
    if (SlowHammer)
    {
        if (mNOPLoops == 0 || mNOPDelayMs == 0)
        {
            mNOPLoops = MultU64x32(clks_msec,500);
            // start_time = MtSupportGetTimeInMilliseconds();
            mNOPDelayMs = DivU64x32(AsmNOPLatencyTest( mNOPLoops ), clks_msec);
            // end_time = MtSupportGetTimeInMilliseconds();
            // mNOPDelayMs = end_time - start_time;
            MtUiDebugPrint (L"NOP latency test finished (%ldms in %ld loops)\n", mNOPDelayMs, mNOPLoops);
        }

        MtUiDebugPrint (L"Min hammer time: %d\n", mMinHammerTime);
        DelayRequired = 640 - mMinHammerTime;
        MtUiDebugPrint (L"Delay required: %ld\n", DelayRequired);
        NOPLoopsRequired = DivU64x64Remainder(MultU64x32(mNOPLoops, DelayRequired), MultU64x32(mNOPDelayMs, NUM_ROW_TOGGLES), NULL);

        MtUiDebugPrint (L"NOP delay loops required: %ld\n", NOPLoopsRequired);
    }
#endif

    {
        volatile UINT32 *start,*end;
        
        UINT64 rowskip = IterCount;
        UINTN addr;
        
        /* Do the hammering  */
        start = (UINT32 *)(UINTN)Start;
        end = (UINT32 *)(UINTN)End;

        // test every row+offset, row-offset
        for (addr=(UINTN)start + (UINTN)LShiftU64(1,offsetbit); addr < (UINTN)end; addr += (UINTN)rowskip) {
            int toggle_count;
            UINTN addr_prev, addr_next;
            UINT64 delay_cycles;

            if (((UINT64)addr + LShiftU64(1, minoffsetbit)) >= (UINTN)end)
                break;

            if ((UINT64)addr < LShiftU64(1, offsetbit))
                offsetbit = minoffsetbit;
            else if (((UINT64)addr - LShiftU64(1, offsetbit)) < (UINTN)start)
                offsetbit = minoffsetbit;
            else if (((UINT64)addr + LShiftU64(1, offsetbit)) >= (UINTN)end)
                offsetbit = minoffsetbit;

            addr_prev = addr - (UINTN)LShiftU64(1,offsetbit);
            addr_next = addr + (UINTN)LShiftU64(1,offsetbit);

            offsetbit++;

            // Add delay cycles between each hammer if doing slow hammer
            delay_cycles = SlowHammer ? DivU64x32(MultU64x32(clks_msec, REFI_MS), HAMMER_RATE_REFI) : 0;

            MtUiDebugPrint (L"[Proc %d] Begin hammering 0x%lx and 0x%lx (Offset: %d, Toggle count: %d, Delay cycles: %ld)\n", ProcNum, (UINT64)addr_prev, (UINT64)addr_next, offsetbit - 1, NUM_ROW_TOGGLES, delay_cycles);

            if (ProcNum == MPSupportGetBspId())
                MtSupportTestTick(FALSE);

            if (MtSupportCheckTestAbort())
                return EFI_ABORTED;

            start_time = MtSupportGetTimeInMilliseconds();
#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {
                asm volatile("movl (%0), %%eax" : : "r" (addr_prev) : "eax");
                asm volatile("movl (%0), %%ebx" : : "r" (addr_next) : "ebx");
                asm volatile("clflush (%0)" : : "r" (addr_prev) : "memory");
                asm volatile("clflush (%0)" : : "r" (addr_next) : "memory");

                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = AsmReadTsc() + delay_cycles;
                    while(AsmReadTsc() < end_ts);
                }
            }
#elif defined(MDE_CPU_X64)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {
                asm volatile("movq (%0), %%rax" : : "r" (addr_prev) : "rax");
                asm volatile("movq (%0), %%rbx" : : "r" (addr_next) : "rbx");
                asm volatile("clflush (%0)" : : "r" (addr_prev) : "memory");
                asm volatile("clflush (%0)" : : "r" (addr_next) : "memory");
                
                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = AsmReadTsc() + delay_cycles;
                    while(AsmReadTsc() < end_ts);
                }
            }
#elif defined(MDE_CPU_AARCH64)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {				
                __asm__ __volatile__("ldr x0, [%0]" : : "r" (addr_prev) : "x0");
                __asm__ __volatile__("ldr x1, [%0]" : : "r" (addr_next) : "x1");
                __asm__ __volatile__("dc civac, %0\n\t" : : "r" (addr_prev) : "memory");
                __asm__ __volatile__("dc civac, %0\n\t" : : "r" (addr_next) : "memory");

                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = MtSupportReadTSC() + delay_cycles;
                    while(MtSupportReadTSC() < end_ts);
                }
            }
#endif
#elif _MSC_VER
#if defined(MDE_CPU_IA32)
            for (toggle_count = 0; toggle_count < NUM_ROW_TOGGLES; toggle_count++) {
                _asm {
                    mov edi,addr_prev
                    mov edx,addr_next
                    jmp L60
                    ;.p2align 4,,7
                    ALIGN 16

                    L60:
                    mov eax,[edi]
                    mov ebx,[edx]
                    clflush [edi]	
                    clflush [edx]
                    ;mfence
                }
                if (delay_cycles != 0) // If slow hammer, read timestamp until we reach the required delay cycles
                {
                    UINT64 end_ts = AsmReadTsc() + delay_cycles;
                    while(AsmReadTsc() < end_ts);
                }
            }
#elif defined(MDE_CPU_X64)
            toggle_count = NUM_ROW_TOGGLES;
            AsmHammerTest((void *)addr_prev,(void *)addr_next,(UINT32)toggle_count, delay_cycles);
#endif
#endif
            end_time = MtSupportGetTimeInMilliseconds();
            MtUiDebugPrint(L"[Proc %d] Hammering 0x%lx and 0x%lx finished (%ldms), Current timestamp: %ld\n", ProcNum, (UINT64)addr_prev, (UINT64)addr_next, end_time - start_time, MtSupportReadTSC());

#if 0 // Not used. Find the minimum time to complete each row pair hammering
            if (SlowHammer == FALSE)
            {
                if (end_time - start_time < mMinHammerTime)
                    mMinHammerTime = (UINT32)(end_time - start_time);
            }

            offsetbit ++;
            if (offsetbit > 24)
                offsetbit = 18;
#endif
        }
    }
    
    return Result;
}


EFI_STATUS
EFIAPI
RunHammerTest_double_sided_hammer_fast (
    IN UINT32                   TestNum, 
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context)
{
    return HammerTest_double_sided_hammer(TestNum, ProcNum,Start,Length,IterCount,Pattern,PatternUser,Context,FALSE);
}


EFI_STATUS
EFIAPI
RunHammerTest_double_sided_hammer_slow (
    IN UINT32                   TestNum, 
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context)
{
    return HammerTest_double_sided_hammer(TestNum, ProcNum,Start,Length,IterCount,Pattern,PatternUser,Context,TRUE);
}


EFI_STATUS
EFIAPI
HammerTest_write (
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN TESTPAT                  Pattern,
    IN UINT32                   PatternUser
    )
{
    EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;
    int						cpu;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint (L"Writing Memory Range: 0x%016lx - 0x%016lx\n", Start, End);

    if (Start == End)
        return EFI_SUCCESS;

    cpu = (int)ProcNum;

#if 0
    //
    // Enable cache
    //
    AsmEnableCache();
#endif

    {
        volatile UINT32 *p;
        volatile UINT32 *pe;
        volatile UINT32 *start,*end;

        int seed = rand(cpu);

        rand_seed(seed, cpu);

        UINT32 p1 = seed;
        if (Pattern == PAT_USER)
        {
            MtUiDebugPrint(L"Using fixed pattern 0x%08X", (UINT32)(UINTN)PatternUser);
            p1 = PatternUser;
        }
        else
        {
            MtUiDebugPrint(L"Using random pattern (seed=0x%08X)", seed);
        }

        /* Display the current pattern */
        if (ProcNum == MPSupportGetBspId())
            MtUiSetPattern(p1);

        /* Initialize memory with the initial pattern.  */
        start = (UINT32 *)(UINTN)Start;
        end = (UINT32 *)(UINTN)End;
        pe = start;
        p = start;
        for (p=start; p<end; p++) {
            if (Pattern != PAT_USER)
                p1 = rand(cpu);
            *p = p1;
        }
    }

    return Result;
}

EFI_STATUS
EFIAPI
HammerTest_verify(
    IN UINT32                   TestNum,
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN TESTPAT                  Pattern,
    IN UINT32                   PatternUser
)
{
    EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;
    int						cpu;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint (L"Verifying Memory Range: 0x%016lx - 0x%016lx\n", Start, End);

    if (Start == End)
        return EFI_SUCCESS;

    cpu = (int)ProcNum;
#if 0
    //
    // Enable cache
    //
    AsmEnableCache();
#endif

    {
        volatile UINT32* p;
        volatile UINT32* pe;
        volatile UINT32 bad;
        volatile UINT32* start, * end;

        int seed = rand(cpu);

        rand_seed(seed, cpu);

        UINT32 p1 = seed;
        if (Pattern == PAT_USER)
            p1 = PatternUser;

        /* Display the current pattern */
        if (ProcNum == MPSupportGetBspId())
            MtUiSetPattern(p1);

        start = (UINT32*)(UINTN)Start;
        end = (UINT32*)(UINTN)End;
        pe = start;
        p = start;
        for (p = start; p < end; p++) {
            if (Pattern != PAT_USER)
                p1 = rand(cpu);

            if ((bad = *p) != p1) {
                if (MtSupportCheckTestAbort())
                    return EFI_ABORTED;

                MtSupportReportError32((UINTN)p, TestNum, bad, p1, 0);
                MtUiDebugPrint(L"  Failed At 0x%p: Expected:%08x Actual:%08x\n", p, p1, bad);
                Result = EFI_TEST_FAILED;
            }
        }
    }

    return Result;
}

EFI_STATUS
EFIAPI
RunHammerTest_write(
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
    return HammerTest_write(ProcNum, Start, Length, Pattern, (UINT32)PatternUser);		
}

EFI_STATUS
EFIAPI
RunHammerTest_verify (
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
    return HammerTest_verify(TestNum, ProcNum, Start, Length, Pattern, (UINT32)PatternUser);
}
