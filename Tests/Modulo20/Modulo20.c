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
//	Modulo20.c
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 8 [Modulo 20, ones&zeros] - Using the Modulo-X algorithm 
//  should uncover errors that are not detected by moving inversions 
//  due to cache and buffering interference with the the algorithm.
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

#define MOD_SZ		20
#define SPINSZ		0x2000000

EFI_STATUS
EFIAPI
Mod20 (
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
		int off, k, done;
		volatile UINT32 *p;
		volatile UINT32 *start, *end;

		/* Display the current pattern */
		if (ProcNum == MPSupportGetBspId())
			MtUiSetPattern(p1);

		for (off = 0; off < MOD_SZ; off++)
		{
			if (ProcNum == MPSupportGetBspId())
				MtSupportTestTick(FALSE);

			if (MtSupportCheckTestAbort())
				return EFI_ABORTED;

			/* Write every nth location with pattern */
			start = (UINT32 *)(UINTN)Start;
			end = (UINT32 *)(UINTN)End;
			end -= MOD_SZ;
			p = start+off;
			done = 0;

			MtUiDebugPrint(L"Writing 0x%08x at every %d-th location\n", p1, off);

			for (; p < end; p += MOD_SZ)
			{
				*p = p1;
			}

			/* Write the rest of memory "iter" times with the pattern complement */
			for (IterNumber = 0; IterNumber < IterCount; IterNumber++)
			{
				if (ProcNum == MPSupportGetBspId())
					MtSupportTestTick(FALSE);

				if (MtSupportCheckTestAbort())
					return EFI_ABORTED;

				start = (UINT32 *)(UINTN)Start;
				end = (UINT32 *)(UINTN)End;
				p = start;
				done = 0;
				k = 0;

				MtUiDebugPrint(L"Writing the complement 0x%08x at every other location\n", p2);

				for (; p < end; p++)
				{
					if (k != off) {
						*p = p2;
					}
					if (++k > MOD_SZ-1) {
						k = 0;
					}
				}
			}

			/* Now check every nth location */
			start = (UINT32 *)(UINTN)Start;
			end = (UINT32 *)(UINTN)End;
			p = start+off;
			done = 0;
			end -= MOD_SZ;	/* adjust the ending address */
			MtUiDebugPrint(L"Checking every %d-th location\n", off);
			for (; p < end; p += MOD_SZ)
			{
				UINT32 bad;
				if ((bad=*p) != p1) {
					MtSupportReportError32((UINTN)p, TestNum, bad, p1, 0);
					Result = EFI_TEST_FAILED;
				}
			}
		}
	}

    return Result;
}

// static const UINTN IterCount = 6;

EFI_STATUS
EFIAPI
RunModulo20Test (
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

			Result |= Mod20(TestNum, ProcNum, Start, Length, IterCount, p1);
			if (Result & EFI_ABORTED)
				return EFI_ABORTED;

			Result |= Mod20(TestNum, ProcNum, Start, Length, IterCount, ~p1);
			if (Result & EFI_ABORTED)
				return EFI_ABORTED;
		}
		return Result;
	}
	break;

	case PAT_USER:
	{
		p1 = (UINT32)PatternUser;

		Result |= Mod20(TestNum, ProcNum, Start, Length, IterCount, p1);
		if (Result & EFI_ABORTED)
			return EFI_ABORTED;

		Result |= Mod20(TestNum, ProcNum, Start, Length, IterCount, ~p1);
		if (Result & EFI_ABORTED)
			return EFI_ABORTED;
		return Result;
	}
	break;

	case PAT_RAND:
	case PAT_DEF:
	default:
	{
		int cpu = (int)ProcNum;
		UINT64 IterNum;
		for (IterNum = 0; IterNum < IterCount; IterNum++)
		{
			p1 = rand(cpu);

			Result |= Mod20(TestNum, ProcNum, Start, Length, 2, p1);
			if (Result & EFI_ABORTED)
				return EFI_ABORTED;

			Result |= Mod20(TestNum, ProcNum, Start, Length, 2, ~p1);
			if (Result & EFI_ABORTED)
				return EFI_ABORTED;
		}
		return Result;
	}
	break;

	}
}

