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
//	AddressOwn.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Test 1 [Address test, own address] - Each address is written with 
//  its own address and then is checked for consistency.
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

EFI_STATUS
EFIAPI
RunAddressOwnTest (
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


    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint(L"[Proc %d] Testing Memory Range: 0x%016lx - 0x%016lx\n", ProcNum, Start, End);

	if (Start == End)
		return EFI_SUCCESS;
#if 0
	//
	// Enable cache
	//
	AsmEnableCache();
#endif

	{
		volatile UINTN *p = 0;
		volatile UINTN *pe;
		volatile UINTN *end, *start;

		/* Write each address with it's own address */
		start = (UINTN *)(UINTN)Start;
		end = (UINTN *)(UINTN)End;
		pe = (UINTN *)(UINTN)Start;
		p = start;
		for (; p < end; p++)
		{		
			if (p >= pe && ProcNum == MPSupportGetBspId())
			{
				MtUiSetPattern((UINTN)p);
				pe += SPINSZ;
			}
			*p = (UINTN)p;
		}

		/* Each address should have its own address */
		start = (UINTN *)(UINTN)Start;
		end = (UINTN *)(UINTN)End;
		p = start;
		for (; p < end; p++)
		{
			UINTN bad;
			if((bad = *p) != (UINTN)p) {
				MtSupportReportError((UINTN)p, TestNum, bad, (UINTN)p, 0);
				Result = EFI_TEST_FAILED;
			}
		}
	}

    return Result;
}