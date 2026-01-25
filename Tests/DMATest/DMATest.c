//PassMark MemTest86
//
//Copyright (c) 2022
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
//	DMATest.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	[DMA test, disk transfer] - This test writes and reads a series of 
//  random numbers into disk via DMA.
//
//History
//	19 Jan 2022: Initial version

#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>
#include <Library/MemTestUiLib.h>
#include <Library/Rand.h>
#include<Protocol/BlockIo.h>

extern EFI_BLOCK_IO_PROTOCOL* DMATestPart;

EFI_STATUS
EFIAPI
RunDMATest (
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

	{
		int seed;
		volatile UINT32* p;
		volatile UINT32* start, * end;
		const UINT32 BlockSize = DMATestPart->Media->BlockSize > 0 ? DMATestPart->Media->BlockSize : 512;
		UINTN NumBytes;
		UINT32 num;
		EFI_STATUS Status;

		UINT32 p0 = 0x80;
		UINT32 p1 = 0;

		switch (Pattern)
		{
		case PAT_WALK01:
			p0 = 0x80;
			break;

		case PAT_USER:
			p0 = (UINT32)PatternUser;
			break;

		case PAT_RAND:
		case PAT_DEF:
		default:
			/* Initialize memory with initial sequence of random numbers.  */
			seed = rand(cpu);
			rand_seed(seed, cpu);
			p0 = seed;
			break;
		}

		if (Pattern == PAT_USER)
		{
			MtUiDebugPrint(L"Using fixed pattern 0x%08X", p0);
		}
		else
		{
			MtUiDebugPrint(L"Using random pattern (seed=0x%08X)", seed);
		}

		/* Initialize memory with the initial pattern.  */
		if (ProcNum == MPSupportGetBspId())
			MtUiSetPattern((UINTN)(UINT32)p0);

		start = (UINT32*)(UINTN)Start;
		end = (UINT32*)(UINTN)End;
		p = start;

		for (; p < end; p++)
		{
			switch (Pattern)
			{
			case PAT_WALK01:
			{
				p1 = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);
				p0 = p0 >> 1;
				if (p0 == 0)
					p0 = 0x80;
			}
			break;

			case PAT_USER:
				p1 = (UINT32)PatternUser;
				break;

			case PAT_RAND:
			case PAT_DEF:
			default:
				p1 = rand(cpu);
				break;
			}
			*p = p1;
		}

		if (ProcNum == MPSupportGetBspId())
			MtSupportTestTick(FALSE);

		if (MtSupportCheckTestAbort())
			return EFI_ABORTED;

		MtUiDebugPrint(L"Start DMA transfer from memory to disk\n");

		NumBytes = (UINTN)(end - start);
		Status = DMATestPart->WriteBlocks(DMATestPart, DMATestPart->Media->MediaId, 0, NumBytes, (void *)start);
		if (Status != EFI_SUCCESS)
		{
			MtUiDebugPrint(L"WriteBlocks failed: %r (start=%p, %d)\n", Status, start, NumBytes);
			return Status;
		}

		Status = DMATestPart->FlushBlocks(DMATestPart);
		if (Status != EFI_SUCCESS)
		{
			MtUiDebugPrint(L"FlushBlocks failed: %r\n", Status);
			return Status;
		}

		MtUiDebugPrint(L"DMA write complete. Start DMA transfer from disk back to memory\n");

		for (IterNumber = 0; IterNumber < IterCount; IterNumber++)
		{
			const UINTN SegmentSize = 0x100000;
			const UINTN NumSegs = NumBytes / SegmentSize;
			const UINTN RemBytes = NumBytes % SegmentSize;
			UINTN Seg;

			if (ProcNum == MPSupportGetBspId())
				MtSupportTestTick(FALSE);

			if (MtSupportCheckTestAbort())
				return EFI_ABORTED;

			MtUiDebugPrint(L"[Iter %d] Num Segments: %d (Rem bytes: %d)\n", IterNumber, NumSegs, RemBytes);

			MtUiDebugPrint(L"[Iter %d] Clear memory\n", IterNumber);

			// Clear memory
			SetMem((void *)start, NumBytes, 0);

			// Even segments
			for (Seg = 0; Seg < NumSegs; Seg += 2)
			{
				UINTN Offset = Seg * SegmentSize;
				EFI_LBA lba = Offset / BlockSize;

				if (ProcNum == MPSupportGetBspId())
					MtSupportTestTick(FALSE);

				if (MtSupportCheckTestAbort())
					return EFI_ABORTED;

				p = (UINT32*)(UINTN)(Start + Offset);
				MtUiDebugPrint(L"[Iter %d] Read disk blocks @ offset: %d (Size: %d)\n", IterNumber, Offset, SegmentSize);
				Status = DMATestPart->ReadBlocks(DMATestPart, DMATestPart->Media->MediaId, lba, SegmentSize, (void*)p);
				if (Status != EFI_SUCCESS)
				{
					MtUiDebugPrint(L"[Iter %d] ReadBlocks failed: %r (lba=%p, %d)\n", IterNumber, Status, p, SegmentSize);
					return Status;
				}
			}

			// Odd segments
			for (Seg = 1; Seg < NumSegs; Seg += 2)
			{
				UINTN Offset = Seg * SegmentSize;
				EFI_LBA lba = Offset / BlockSize;

				if (ProcNum == MPSupportGetBspId())
					MtSupportTestTick(FALSE);

				if (MtSupportCheckTestAbort())
					return EFI_ABORTED;

				p = (UINT32*)(UINTN)(Start + Offset);
				MtUiDebugPrint(L"[Iter %d] Read disk blocks @ offset: %d (Size: %d)\n", IterNumber, Offset, SegmentSize);
				Status = DMATestPart->ReadBlocks(DMATestPart, DMATestPart->Media->MediaId, lba, SegmentSize, (void*)p);
				if (Status != EFI_SUCCESS)
				{
					MtUiDebugPrint(L"[Iter %d] ReadBlocks failed: %r (lba=%p, %d)\n", IterNumber, Status, p, SegmentSize);
					return Status;
				}
			}

			if (RemBytes > 0)
			{
				UINTN Offset = NumSegs * SegmentSize;
				EFI_LBA lba = Offset / BlockSize;
				p = (UINT32*)(UINTN)(Start + Offset);
				MtUiDebugPrint(L"[Iter %d] Read disk blocks @ offset: %d (Size: %d)\n", IterNumber, Offset, RemBytes);
				Status = DMATestPart->ReadBlocks(DMATestPart, DMATestPart->Media->MediaId, lba, RemBytes, (void*)p);
				if (Status != EFI_SUCCESS)
				{
					MtUiDebugPrint(L"[Iter %d] ReadBlocks failed: %r (lba=%p, %d)\n", IterNumber, Status, p, RemBytes);
					return Status;
				}
			}

			MtUiDebugPrint(L"[Iter %d] DMA read complete\n", IterNumber);

			MtUiDebugPrint(L"[Iter %d] Check for initial pattern\n", IterNumber);

			switch (Pattern)
			{
			case PAT_WALK01:
				p0 = 0x80;
				break;

			case PAT_USER:
				p0 = (UINT32)PatternUser;
				break;

			case PAT_RAND:
			case PAT_DEF:
			default:
				rand_seed(seed, cpu);
				break;
			}

			start = (UINT32*)(UINTN)Start;
			end = (UINT32*)(UINTN)End;
			p = start;

			for (; p < end; p++)
			{
				UINT32 bad;

				switch (Pattern)
				{
				case PAT_WALK01:
				{
					num = p0 | (p0 << 8) | (p0 << 16) | (p0 << 24);
					p0 = p0 >> 1;
					if (p0 == 0)
						p0 = 0x80;
				}
				break;

				case PAT_USER:
					num = (UINT32)PatternUser;
					break;

				case PAT_RAND:
				case PAT_DEF:
				default:
					num = rand(cpu);
					break;
				}

				if ((bad = *p) != num) {
					if (MtSupportCheckTestAbort())
						return EFI_ABORTED;

					MtSupportReportError32((UINTN)p, TestNum, bad, num, 0);
					Result = EFI_TEST_FAILED;
				}
			}
		}
	}

	return Result;
}