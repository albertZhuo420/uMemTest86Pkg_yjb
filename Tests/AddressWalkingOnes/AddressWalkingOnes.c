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
//	AddressWalkingOnes.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Test 0 [Address test, walking ones, no cache] - Tests all address 
//  bits in all memory banks by using a walking ones address pattern.
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
#include <Library/MemTestRangesLib.h>
#include <Library/Rand.h>

#define ROUNDUP(value,mask)		((value + mask) & ~mask)

EFI_STATUS
EFIAPI
AddressWalkingOnes (
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

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint(L"Testing Memory Range: 0x%016lx - 0x%016lx\n", Start, End);

	if (Start == End)
		return EFI_SUCCESS;
#if 0
	//
	// Disable cache - seems to freeze on Mac (km)
	//
	AsmDisableCache();
#endif

	{
		int i, j, k;
		volatile UINT32 *p = 0;
		UINT32 p1 = Pattern;
		volatile UINT32 *pt;
		volatile UINT32 *end;
		UINT32 bad, mask;
		UINT64 bank;

		MtUiDebugPrint(L"Testing global address bits\n");
		/* Test the global address bits */
		for (p1=0, j=0; j<2; j++) {

			/* Set pattern in our lowest multiple of 0x20000 */
			p = (UINT32 *)((UINTN)ROUNDUP(Start, 0x000000000001ffffull));
			end = (UINT32 *)((UINTN)End);

			MtUiDebugPrint(L"Testing lowest multiple of 0x20000 (0x%p) with 0x%08x\n",p,p1);
			if (p >= end)
				break;

			*p = p1;
			if (ProcNum == MPSupportGetBspId())
				MtUiSetPattern(p1);

			/* Now write pattern compliment */
			p1 = ~p1;
	
			for (i=0; i<100; i++) {
				mask = 4;
				do {
					pt = (UINT32 *)((UINTN)p | mask);
					if ((UINTN)pt == (UINTN)p) {
						mask = mask << 1;
						continue;
					}
					if ((UINTN)pt >= (UINTN)end) {
						break;
					}

					*pt = p1;
					if ((bad = *p) != ~p1) {
						UINT32 num = ~p1;
						MtSupportReportError32((UINTN)p, TestNum, bad, num, 0);
						i = 1000;
						Result = EFI_TEST_FAILED;
					}
					mask = mask << 1;
				} while(mask);
			}
			// do_tick();
			// BAILR
		}

		MtUiDebugPrint(L"Testing rank address bits\n");
		/* Now check the address bits in each bank */
		/* If we have more than 8mb of memory then the bank size must be */
		/* bigger than 256k.  If so use 1mb for the bank size. */
		if (MtRangesGetTotalSize() > (0x800000 >> 12)) {
			bank = 0x100000;
		} else {
			bank = 0x40000;
		}
		MtUiDebugPrint(L"Bank size is 0x%lx\n",bank);
		for (p1=0, k=0; k<2; k++) {

			if (ProcNum == MPSupportGetBspId())
				MtUiSetPattern(p1);

			/* Force start address to be a multiple of 256k */
			p = (UINT32 *)(UINTN)ROUNDUP(Start, (bank - 1));
			end = (UINT32 *)(UINTN)End;
			while ((UINTN)p < (UINTN)end) {
				MtUiDebugPrint(L"Writing to address (0x%p) with 0x%08x\n",p,p1);
				*p = p1;

				p1 = ~p1;
				for (i=0; i<200; i++) {
					mask = 4;
					do {
						pt = (UINT32 *)
							((UINTN)p | mask);
						if ((UINTN)pt == (UINTN)p) {
							mask = mask << 1;
							continue;
						}
						if ((UINTN)pt >= (UINTN)end) {
							break;
						}

						*pt = p1;
						if ((bad = *p) != ~p1) {
							UINT32 num = ~p1;
							MtSupportReportError32((UINTN)p, TestNum, bad, num, 0);

							i = 200;
							Result = EFI_TEST_FAILED;
						}
						mask = mask << 1;
					} while(mask);
				}
				if (((UINTN)p + (UINTN)bank) > (UINTN)p) {
					p = (UINT32*)((UINTN)p + (UINTN)bank);
				} else {
					p = end;
				}
				p1 = ~p1;
			}

			p1 = ~p1;
		}
	}
    
#if 0
	//
	// Enable cache
	//
	AsmEnableCache();
#endif
    return Result;
}

EFI_STATUS
EFIAPI
RunAddressWalkingOnesTest(
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
	switch (Pattern)
	{
	case PAT_RAND:
		return AddressWalkingOnes(TestNum, ProcNum, Start, Length, IterCount, rand((int)ProcNum));
	case PAT_USER:
		return AddressWalkingOnes(TestNum, ProcNum, Start, Length, IterCount, (UINT32)PatternUser);
	case PAT_WALK01:
	{
		EFI_STATUS Result = EFI_SUCCESS;
		UINT32 p0 = 0x80, p1;
		int i;
		for (i = 0; i < 8; i++, p0 = p0 >> 1) {
			p1 = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);

			Result |= AddressWalkingOnes(TestNum, ProcNum, Start, Length, IterCount, p1);
			if (Result & EFI_ABORTED)
				return EFI_ABORTED;

			Result |= AddressWalkingOnes(TestNum, ProcNum, Start, Length, IterCount, ~p1);
			if (Result & EFI_ABORTED)
				return EFI_ABORTED;
		}
		return Result;
	}
	break;
	case PAT_DEF:
	default:
		return AddressWalkingOnes(TestNum, ProcNum, Start, Length, IterCount, 0);
	}
}