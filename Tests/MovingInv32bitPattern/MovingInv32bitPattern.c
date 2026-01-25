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
//	MovingInv32bitPattern.c
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 6 [Moving inversions, 32 bit pat] - This is a variation of the 
//  moving inversions algorithm that shifts the data pattern left 
//  one bit for each successive address.
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
MovingInv32bitPattern (
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

    //
    // Display range being tested
    //
    End = Start + Length;

	if (Start == End)
		return EFI_SUCCESS;
#if 0
	//
	// Enable cache
	//
	AsmEnableCache();
#endif

	{
		int k=0, off=0;
		volatile UINT32 *p;
		volatile UINT32 *start, *end;
		UINT32 pat = 0;

		for (off = 0; off < 32; off++)
		{
			if (ProcNum == MPSupportGetBspId())
				MtSupportTestTick(FALSE);

			if (MtSupportCheckTestAbort())
				return EFI_ABORTED;

			/* Initialize memory with the initial pattern.  */
			start = (UINT32 *)(UINTN)Start;
			end = (UINT32 *)(UINTN)End;
			p = start;

			k = off;

			pat = LRotU32(p1,k);

			MtUiDebugPrint(L"Initializing memory starting with pattern: 0x%08x\n", pat);
			if (ProcNum == MPSupportGetBspId())
				MtUiSetPattern(pat);

			for (; p < end; p++)
			{
				/* Do a SPINSZ section of memory */
	/* Original C code replaced with hand tuned assembly code
		*			while (p < pe) {
		*				*p = pat;
		*				if (++k >= 32) {
		*					pat = lb;
		*					k = 0;
		*				} else {
		*					pat = pat << 1;
		*					pat |= sval;
		*				}
		*				p++;
		*			}
		*/
				*p = pat;
				k++;
				k &= 31;
				pat = LRotU32(pat,1);					
			}

			/* Do moving inversions test. Check for initial pattern and then
			 * write the complement for each memory location. Test from bottom
			 * up and then from the top down.  */
			for (IterNumber = 0; IterNumber < IterCount; IterNumber++)
			{
				start = (UINT32 *)(UINTN)Start;
				end = (UINT32 *)(UINTN)End;
				p = start;
				k = off;

				pat = LRotU32(p1,k);

				MtUiDebugPrint(L"Checking for initial pattern, then writing the complement\n");
				for (; p < end; p++)
				{
		/* Original C code replaced with hand tuned assembly code
		*				while (p < pe) {
		*					if ((bad=*p) != pat) {
		*						error((ulong*)p, pat, bad);
		*					}
		*					*p = ~pat;
		*					if (++k >= 32) {
		*						pat = lb;
		*						k = 0;
		*					} else {
		*						pat = pat << 1;
		*						pat |= sval;
		*					}
		*					p++;
		*				}
		*/
					UINT32 bad;
					if ((bad=*p) != pat) {
						if (MtSupportCheckTestAbort())
							return EFI_ABORTED;

						MtSupportReportError32((UINTN)p, TestNum, bad, pat, 0);
						Result = EFI_TEST_FAILED;
					}
					*p = ~pat;
					k++;
					k &= 31;
					pat = LRotU32(pat,1);					
				}

				/* Since we already adjusted k and the pattern this
					* code backs both up one step
					*/
		/* CDH start */
		/* Original C code replaced with hand tuned assembly code
			*		pat = lb;
			*		if ( 0 != (k = (k-1) & 31) ) {
			*			pat = (pat << k);
			*			if ( sval )
			*			pat |= ((sval << k) - 1);
			*		}
			*		k++;
			*/

				k--;
				k &= 31;
				pat = LRotU32(p1,k);
				k++;
				k &= 31;

				start = (UINT32 *)(UINTN)Start;
				end = (UINT32 *)(UINTN)End;
				p = end -1;
				MtUiDebugPrint(L"Checking for complement, then writing the initial pattern\n");

				for (; p >= start; p--)
				{					
		/* Original C code replaced with hand tuned assembly code
		*				do {
		*					if ((bad=*p) != ~pat) {
		*						error((ulong*)p, ~pat, bad);
		*					}
		*					*p = pat;
		*					if (--k <= 0) {
		*						pat = hb;
		*						k = 32;
		*					} else {
		*						pat = pat >> 1;
		*						pat |= p3;
		*					}
		*				} while (p-- > pe);
		*/
					UINT32 bad;
					if ((bad=*p) != ~pat) {
						if (MtSupportCheckTestAbort())
							return EFI_ABORTED;

						MtSupportReportError32((UINTN)p, TestNum, bad, ~pat, 0);
						Result = EFI_TEST_FAILED;
					}
					*p = pat;
					k--;
					k &= 31;
					pat = RRotU32(pat,1);

					// do_tick();
					// BAILR
				}
			}
		}
	}

    return Result;
}

//
//  AddressInit () - Registers test function for the Moving Bits test
//
EFI_STATUS
EFIAPI
RunMovingInv32bitPatternTest (
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

			Result |= MovingInv32bitPattern(TestNum, ProcNum, Start, Length, IterCount, p1);
			if (Result & EFI_ABORTED)
				return EFI_ABORTED;

			Result |= MovingInv32bitPattern(TestNum, ProcNum, Start, Length, IterCount, ~p1);
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
			Result |= MovingInv32bitPattern(TestNum, ProcNum, Start, Length, 2, p1);
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

		Result |= MovingInv32bitPattern(TestNum, ProcNum, Start, Length, IterCount, p1);
		if (Result & EFI_ABORTED)
			return EFI_ABORTED;

		Result |= MovingInv32bitPattern(TestNum, ProcNum, Start, Length, IterCount, ~p1);
		if (Result & EFI_ABORTED)
			return EFI_ABORTED;
		return Result;
	}
	break;
	}
}


