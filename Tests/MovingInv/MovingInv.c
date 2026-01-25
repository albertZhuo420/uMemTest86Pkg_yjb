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
//	MovingInv.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Test 2 [Moving inversions, ones&zeros] - This test uses the moving 
//  inversions algorithm with patterns of all ones and zeros.
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

#define SPINSZ		0x2000000

EFI_STATUS
EFIAPI
MovingInv (
    IN UINT32                   TestNum,
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN UINT32                   Pattern
)
{
    EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;
    UINT64                  IterNumber;

    UINT32					p1 = Pattern;
    UINT32					p2 = ~p1;

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
        volatile UINT32 *p = 0;
        volatile UINT32 *start,*end;

        /* Initialize memory with the initial pattern.  */
        if (ProcNum == MPSupportGetBspId())
            MtUiSetPattern(p1);

        start = (UINT32 *)(UINTN)Start;
        end = (UINT32 *)(UINTN)End;
        p = start;
        for (; p < end; p++)
        {
/* Original C code replaced with hand tuned assembly code */
            *p = p1;
        }

        /* Do moving inversions test. Check for initial pattern and then
         * write the complement for each memory location. Test from bottom
         * up and then from the top down.  */
        for (IterNumber = 0; IterNumber < IterCount; IterNumber++)
        {		
            if (ProcNum == MPSupportGetBspId())
                MtSupportTestTick(FALSE);

            if (MtSupportCheckTestAbort())
                return EFI_ABORTED;

            start = (UINT32 *)(UINTN)Start;
            end = (UINT32 *)(UINTN)End;
            p = start;

            MtUiDebugPrint(L"[Iter %d] [0x%016lx - 0x%016lx] Checking for initial pattern: 0x%08x, and writing complement: 0x%08x", IterNumber, Start, End, p1, p2);

            for (; p < end; p++)
            {
                UINT32 bad;
    /* Original C code replaced with hand tuned assembly code
    *				for (; p < pe; p++) {
    *					if ((bad=*p) != p1) {
    *						error((ulong*)p, p1, bad);
    *					}
    *					*p = p2;
    *				}
    */

                if ((bad=*p) != p1) {
                    if (MtSupportCheckTestAbort())
						return EFI_ABORTED;

                    MtSupportReportError32((UINTN)p, TestNum, bad, p1, 0);
                    Result = EFI_TEST_FAILED;
                }
                *p = p2;
            }

            start = (UINT32 *)(UINTN)Start;
            end = (UINT32 *)(UINTN)End;
            p = end - 1;

            MtUiDebugPrint(L"[Iter %d] [0x%016lx - 0x%016lx] Checking for complement: 0x%08x, and writing initial pattern: 0x%08x", IterNumber, End, Start, p2, p1);
            for (; p >= start; p--)
            {
                UINT32 bad;
    /* Original C code replaced with hand tuned assembly code
    *				do {
    *					if ((bad=*p) != p2) {
    *						error((ulong*)p, p2, bad);
    *					}
    *					*p = p1;
    *				} while (p-- > pe);
    */

                if ((bad=*p) != p2) {
                    if (MtSupportCheckTestAbort())
						return EFI_ABORTED;

                    MtSupportReportError32((UINTN)p, TestNum, bad, p2, 0);
                    Result = EFI_TEST_FAILED;
                }
                *p = p1;
            }
            MtUiDebugPrint(L"[Iter %d] Finished iteration %d", IterNumber, IterNumber);
        }
    }

    return Result;
}

EFI_STATUS
EFIAPI
RunMovingInvTest(
    IN UINT32                   TestNum,
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
    IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID* Context
)
{
    EFI_STATUS Result = EFI_SUCCESS;
    UINT32 p1 = 0;

    switch (Pattern)
    {
    case PAT_WALK01:
    {
        UINT32 p0 = 0x80;
        int i;
        for (i = 0; i < 8; i++, p0 = p0 >> 1) {
            p1 = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);

            Result |= MovingInv(TestNum, ProcNum, Start, Length, IterCount, p1);
            if (Result & EFI_ABORTED)
                return EFI_ABORTED;

            Result |= MovingInv(TestNum, ProcNum, Start, Length, IterCount, ~p1);
            if (Result & EFI_ABORTED)
                return EFI_ABORTED;
        }
        return Result;
    }
    break;

    case PAT_RAND:
    {
        int cpu = (int)ProcNum;
        UINT64 IterNum;
        for (IterNum = 0; IterNum < IterCount; IterNum++)
        {
            p1 = rand(cpu);
            Result |= MovingInv(TestNum, ProcNum, Start, Length, 2, p1);
            if (Result & EFI_ABORTED)
                return EFI_ABORTED;
        }
        return Result;
    }
    break;

    case PAT_USER:
    case PAT_DEF:
    default:
    {
        p1 = (UINT32)PatternUser;

        Result |= MovingInv(TestNum, ProcNum, Start, Length, IterCount, p1);
        if (Result & EFI_ABORTED)
            return EFI_ABORTED;

        Result |= MovingInv(TestNum, ProcNum, Start, Length, IterCount, ~p1);
        if (Result & EFI_ABORTED)
            return EFI_ABORTED;
        return Result;
    }
    break;
    }
}