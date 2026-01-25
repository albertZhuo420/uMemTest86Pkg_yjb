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
//	RandomNumSequence.c
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 7 [Random number sequence] - This test writes a series of 
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

#define SPINSZ		0x2000000


EFI_STATUS
EFIAPI
RunRandomNumSequenceTest (
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
	int						cpu;
	UINT64                  IterNumber;

    //
    // Display range being tested
    //
    End = Start + Length;
    MtUiDebugPrint(L"Testing Memory Range: 0x%016lx - 0x%016lx\n", Start, End);

	if (Start == End)
		return EFI_SUCCESS;

	cpu = (int)ProcNum;

#if 0
	//
	// Enable cache
	//
	AsmEnableCache();
#endif

	for (IterNumber = 0; IterNumber < IterCount; IterNumber++)
	{
		int i, done, seed;
		volatile UINT32 *p;
		volatile UINT32 *start,*end;
		UINT32 num;

		if (ProcNum == MPSupportGetBspId())
			MtSupportTestTick(FALSE);

		if (MtSupportCheckTestAbort())
			return EFI_ABORTED;

		/* Initialize memory with initial sequence of random numbers.  */
		seed = rand(cpu);

		rand_seed(seed, cpu);

		MtUiDebugPrint(L"Writing random paterns to memory\n");
		/* Initialize memory with the initial pattern.  */
		if (ProcNum == MPSupportGetBspId())
			MtUiSetPattern((UINTN)(UINT32)seed);

		start = (UINT32 *)(UINTN)Start;
		end = (UINT32 *)(UINTN)End;
		p = start;
		done = 0;
		for (; p < end; p++)
		{
			*p = rand(cpu);
		}

		MtUiDebugPrint(L"Check for initial pattern, then write the complement.\n");
		/* Check for initial pattern and then
		 * write the complement for each memory location. Test from bottom
		 * up and then from the top down.  */
		for (i=0; i<2; i++) {
			rand_seed(seed, cpu);

			start = (UINT32 *)(UINTN)Start;
			end = (UINT32 *)(UINTN)End;
			p = start;
			done = 0;
			for (; p < end; p++)
			{
				UINT32 bad;
				num = rand(cpu);
				if (i) {
					num = ~num;
				}
				if ((bad=*p) != num) {
					if (MtSupportCheckTestAbort())
						return EFI_ABORTED;

					MtSupportReportError32((UINTN)p, TestNum, bad, num, 0);
					Result = EFI_TEST_FAILED;
				}
				*p = ~num;
			}
		}
	}

    return Result;
}


