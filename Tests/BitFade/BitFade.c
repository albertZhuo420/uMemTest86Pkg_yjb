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
//	BitFade.c
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 9 [Bit fade test, 2 patterns] - The bit fade test initializes 
//  all of memory with a pattern and then sleeps for a few minutes. 
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

EFI_STATUS
EFIAPI
WritePattern (
	IN UINT32                   TestNum,
	IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
	IN UINT32                   Pattern
    )
{
	EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint (L"Testing Memory Range: 0x%016lx - 0x%016lx\n", Start, End);

	if (Start == End)
		return EFI_SUCCESS;
#if 0
	//
	// Enable cache
	//
	AsmEnableCache();
#endif

	{
		volatile UINT32 *p;
		volatile UINT32 *start,*end;

		UINT32 p1 = Pattern;

		if (ProcNum == MPSupportGetBspId())
			MtUiSetPattern(p1);

		MtUiDebugPrint (L"Initialize memory with initial pattern: 0x%08x\n", p1);
		/* Initialize memory with the initial pattern.  */
		start = (UINT32 *)(UINTN)Start;
		end = (UINT32 *)(UINTN)End;
		p = start;
		for (; p<end; p++) {
			*p = p1;
		}
	}

    return Result;
}

EFI_STATUS
EFIAPI
VerifyPattern (
	IN UINT32                   TestNum, 
	IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
	IN UINT32                   Pattern
)
{
	EFI_STATUS				Result = EFI_SUCCESS;
    UINT64                  End = 0;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint (L"Testing Memory Range: 0x%016lx - 0x%016lx\n", Start, End);

#if 0
	//
	// Enable cache
	//
	AsmEnableCache();
#endif

	{
		volatile UINT32 *p;
		volatile UINT32 bad;
		volatile UINT32 *start,*end;

		UINT32 p1 = Pattern;

		if (ProcNum == MPSupportGetBspId())
			MtUiSetPattern(p1);

		MtUiDebugPrint(L"Verify data pattern still stored in memory\n");
		/* Make sure that nothing changed while sleeping */
		start = (UINT32 *)(UINTN)Start;
		end = (UINT32 *)(UINTN)End;
		p = start;
		for (; p<end; p++) {
			if ((bad=*p) != p1) {
				MtSupportReportError32((UINTN)p, TestNum, bad, p1, 0);
				MtUiDebugPrint (L"  Failed At 0x%p: Expected:%08x Actual:%08x\n", p, bad, p1);
				Result = EFI_TEST_FAILED;
			}
		}
	}

    return Result;
}

EFI_STATUS
EFIAPI
RunBitFadeTest_write(
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
	UINT32 p1 = 0;

	switch (Pattern)
	{
	case PAT_WALK01:
	{
		UINT32 p0 = 0x80;
		p1 = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);
	}
	break;

	case PAT_USER:
		p1 = (UINT32)PatternUser;
		break;

	case PAT_RAND:
		p1 = rand((int)ProcNum);
		break;
	case PAT_DEF:
	default:
		p1 = 0;	
		break;
	}

	return WritePattern(TestNum, ProcNum, Start, Length, p1);
}

EFI_STATUS
EFIAPI
RunBitFadeTest_verify(
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
	UINT32 p1 = 0;

	switch (Pattern)
	{
	case PAT_WALK01:
	{
		UINT32 p0 = 0x80;
		p1 = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);
	}
	break;

	case PAT_USER:
		p1 = (UINT32)PatternUser;
		break;

	case PAT_RAND:
		p1 = rand((int)ProcNum);
		break;
	case PAT_DEF:
	default:
		p1 = 0;
		break;
	}

	return VerifyPattern(TestNum, ProcNum, Start, Length, p1);
}