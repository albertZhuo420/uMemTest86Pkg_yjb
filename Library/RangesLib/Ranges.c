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
//	Ranges.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Memory range enumerating functions
//
//
//References:
//  https://github.com/jljusten/MemTestPkg
//

/** @file
  Memory range functions for Mem Test application

  Copyright (c) 2009, Intel Corporation
  Copyright (c) 2009, Jordan Justen
  All rights reserved.

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.  The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS"
  BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER
  EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemTestRangesLib.h>
#include <Library/MemTestUiLib.h>

#include <uMemTest86.h>

EFI_MEMORY_DESCRIPTOR       *mMemoryMap = NULL;
BOOLEAN						mMemoryMapLocked[1024];
UINTN                       mMemoryMapCount;
UINT64                      mPagesAllocated;
UINT64                      mPagesAvailable;


/**
  Returns the total size of all test memory ranges

  @return  The total size of all test memory ranges

**/
UINT64
MtRangesGetTotalSize (
  VOID
  )
{
  return MultU64x32 (mPagesAllocated, EFI_PAGE_SIZE);
}

/**
  Returns the total size of available memory ranges

  @return  The total size of available memory ranges

**/
UINT64
MtRangesGetAvailableSize (
  VOID
  )
{
  return MultU64x32 (mPagesAvailable, EFI_PAGE_SIZE);
}


/**
  Initializes the memory test ranges

  @param[in,out] Key     To retrieve the first range, set Key to 0 before calling.
                         To retrieve the next range, pass in the previous Key.
  @param[out]    Start   Start of the next memory range
  @param[out]    Length  Length of the next memory range

  @retval  EFI_NOT_FOUND  Indicates all ranges have been returned
  @retval  EFI_SUCCESS    The next memory range was returned

**/
EFI_STATUS
MtRangesGetNextRange (
  IN OUT INTN                  *Key,
  OUT    EFI_PHYSICAL_ADDRESS  *Start,
  OUT    UINT64                *Length
  )
{
  if (*Key < 0 || *Key >= (INTN)mMemoryMapCount) {
    return EFI_NOT_FOUND;
  }

  *Start = mMemoryMap[*Key].PhysicalStart;
  *Length = MultU64x32 (mMemoryMap[*Key].NumberOfPages, EFI_PAGE_SIZE);
  (*Key)++;

  return EFI_SUCCESS;
}


/**
  Lock memory range

  @param[in]    Start   Start of the memory range
  @param[in]    Length  Length of the memory range

  @retval  EFI_ACCESS_DENIED  The range could not be locked
  @retval  EFI_SUCCESS        The range was locked

**/
EFI_STATUS
EFIAPI
MtRangesLockRange (
  IN    EFI_PHYSICAL_ADDRESS  Start,
  IN    UINT64                Length
  )
{
  EFI_STATUS     Status;
  INTN Key;
  for (Key = 0; Key < (INTN)mMemoryMapCount; Key++)
  {
	  if (Start == mMemoryMap[Key].PhysicalStart)
		  break;
  }

  if (Key == (INTN)mMemoryMapCount)
	  return EFI_NOT_FOUND;

  Status = gBS->AllocatePages (
                  AllocateAddress,
                  EfiBootServicesData,
                  (UINTN)(EFI_SIZE_TO_PAGES (Length)),
                  &Start
                  );
  if (EFI_ERROR (Status)) {
    AsciiSPrint(gBuffer, BUF_SIZE, "MtRangesLockRange - AllocatePages(Start=0x%lx,Pages=%ld) retuned %r", Start, EFI_SIZE_TO_PAGES (Length),Status);
	MtSupportDebugWriteLine(gBuffer);
    return Status;
  }
  mPagesAvailable -= EFI_SIZE_TO_PAGES (Length);
  mMemoryMapLocked[Key] = TRUE;
  return EFI_SUCCESS;
}


/**
  Unlocks a memory range

  @param[in]    Start   Start of the memory range
  @param[in]    Length  Length of the memory range

**/
VOID
EFIAPI
MtRangesUnlockRange (
  IN    EFI_PHYSICAL_ADDRESS  Start,
  IN    UINT64                Length
  )
{
  INTN Key;
  for (Key = 0; Key < (INTN)mMemoryMapCount; Key++)
  {
	  if (Start == mMemoryMap[Key].PhysicalStart)
		  break;
  }

  if (Key == (INTN)mMemoryMapCount)
	  return;

  gBS->FreePages (Start, (UINTN)(EFI_SIZE_TO_PAGES (Length)));

  mPagesAvailable += EFI_SIZE_TO_PAGES (Length);
  mMemoryMapLocked[Key] = FALSE;
}

BOOLEAN
EFIAPI
MtRangesIsRangeLocked (
  IN    EFI_PHYSICAL_ADDRESS  Start,
  IN    UINT64                Length
  )
{
  INTN Key;
  for (Key = 0; Key < (INTN)mMemoryMapCount; Key++)
  {
	  if (Start == mMemoryMap[Key].PhysicalStart)
		  break;
  }

  if (Key == (INTN)mMemoryMapCount)
	  return FALSE;

  return mMemoryMapLocked[Key];
}

EFI_STATUS
ReadMemoryRanges (
  )
{
  EFI_STATUS                  Status;
  UINTN                       NumberOfEntries;
  UINTN                       Loop;
  UINTN                       Size;
  EFI_MEMORY_DESCRIPTOR       *MemoryMap;
  EFI_MEMORY_DESCRIPTOR       *MapEntry;
  UINTN                       MapKey;
  UINTN                       DescSize;
  UINT32                      DescVersion;
  UINT64                      Available;
  EFI_PHYSICAL_ADDRESS        PagesAddress;
  UINT64                      NumberOfPages;

  Size = 0;
  Status = gBS->GetMemoryMap (&Size, NULL, NULL, NULL, NULL);

  if (Status != EFI_BUFFER_TOO_SMALL)
  {
	  AsciiSPrint(gBuffer, BUF_SIZE, "ReadMemoryRanges - GetMemoryMap retuned %r (Size = %d)", Status, Size);
	  MtSupportDebugWriteLine(gBuffer);
	  return Status;
  }

  Size = Size * 2; // Double the size just in case it changes

  MemoryMap = (EFI_MEMORY_DESCRIPTOR*) AllocatePool (Size);
  if (MemoryMap == NULL) {
     AsciiSPrint(gBuffer, BUF_SIZE, "ReadMemoryRanges - Could not allocate a memory pool of size = %d", Size);
     MtSupportDebugWriteLine(gBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->GetMemoryMap (&Size, MemoryMap, &MapKey, &DescSize, &DescVersion);
  if (EFI_ERROR (Status)) {
	AsciiSPrint(gBuffer, BUF_SIZE, "ReadMemoryRanges - GetMemoryMap failed (%r)", Status);
    MtSupportDebugWriteLine(gBuffer);
    FreePool (MemoryMap);
    return Status;
  }

  if (DescVersion != EFI_MEMORY_DESCRIPTOR_VERSION) {
	AsciiSPrint(gBuffer, BUF_SIZE, "ReadMemoryRanges - Memory descriptor version (%d) different from expected (%d)", DescVersion, EFI_MEMORY_DESCRIPTOR_VERSION);
    MtSupportDebugWriteLine(gBuffer);
    FreePool (MemoryMap);
    return EFI_UNSUPPORTED;
  }

  NumberOfEntries = Size / DescSize;

  for (
      MapEntry = MemoryMap, Loop = 0, Available = 0;
      Loop < NumberOfEntries;
      MapEntry = (EFI_MEMORY_DESCRIPTOR*) ((UINT8*) MapEntry + DescSize),
        Loop++
    ) {
    if (MapEntry->Type == EfiConventionalMemory) {
      Available += MapEntry->NumberOfPages;
    }
  }

  AsciiSPrint(gBuffer, BUF_SIZE, "ReadMemoryRanges - Available Pages = %ld", Available);
  MtSupportDebugWriteLine(gBuffer);

  mMemoryMap = MemoryMap;
  mMemoryMapCount = 0;
  mPagesAllocated = 0;
  mPagesAvailable = 0;
  for (
      MapEntry = MemoryMap, Loop = 0;
      (Loop < NumberOfEntries) /* && (Available > (SIZE_1MB / EFI_PAGE_SIZE) )*/;
      MapEntry = (EFI_MEMORY_DESCRIPTOR*) ((UINT8*) MapEntry + DescSize), Loop++
    ) {
    if (MapEntry->Type == EfiConventionalMemory) {
      PagesAddress = MapEntry->PhysicalStart;
      NumberOfPages = MapEntry->NumberOfPages;
#if 0
      if (PagesAddress == 0ll && NumberOfPages > 1) {
        PagesAddress = PagesAddress + EFI_PAGE_SIZE;
        NumberOfPages--;
      }

      if (MapEntry->PhysicalStart != PagesAddress) {
        MapEntry->PhysicalStart = PagesAddress;
      }
      if (MapEntry->NumberOfPages != NumberOfPages) {
        MapEntry->NumberOfPages = NumberOfPages;
      }
#endif
      if (MapEntry != &mMemoryMap[mMemoryMapCount]) {
        CopyMem (&mMemoryMap[mMemoryMapCount], MapEntry, sizeof (*MapEntry));
      }
      mMemoryMapCount++;

      mPagesAllocated += NumberOfPages;
      Available -= NumberOfPages;
    }
  }

  mPagesAvailable = mPagesAllocated;

  SetMem(mMemoryMapLocked, sizeof(mMemoryMapLocked), 0);

#if 0
  if (mMemoryMapCount > 0) {	
    MtSupportDebugWriteLine("ReadMemoryRanges - Memory Test Ranges:");
  }
  else 
    MtSupportDebugWriteLine("ReadMemoryRanges - Warning: mMemoryMapCount = 0");

  for (Loop = 0; Loop < mMemoryMapCount; Loop++) {
    UINT64 SizeInBytes = MultU64x32 (mMemoryMap[Loop].NumberOfPages, EFI_PAGE_SIZE);
	AsciiSPrint(gBuffer, BUF_SIZE, "0x%012lx - 0x%012lx (0x%012lx)",
									  mMemoryMap[Loop].PhysicalStart,
									  mMemoryMap[Loop].PhysicalStart + SizeInBytes - 1,
									  SizeInBytes
									  );
	MtSupportDebugWriteLine(gBuffer);
  }
  if (mPagesAllocated > 0) {
    UINT64 SizeInBytes = MultU64x32 (mPagesAllocated, EFI_PAGE_SIZE);
    if (SizeInBytes >= SIZE_4GB) {
      AsciiSPrint(gBuffer, BUF_SIZE, "Total: 0x%lx (%ldGB)", SizeInBytes, DivU64x64Remainder (SizeInBytes, SIZE_1GB, NULL));
    } else if (SizeInBytes >= SIZE_2MB) {
      AsciiSPrint(gBuffer, BUF_SIZE, "Total: 0x%lx (%ldMB)", SizeInBytes, DivU64x32 (SizeInBytes, SIZE_1MB));
    } else if (SizeInBytes >= SIZE_2KB) {
      AsciiSPrint(gBuffer, BUF_SIZE, "Total: 0x%lx (%ldKB)", SizeInBytes, DivU64x32 (SizeInBytes, SIZE_1KB));
    } else {
	  AsciiSPrint(gBuffer, BUF_SIZE, "Total: 0x%lx (%ldB)", SizeInBytes, SizeInBytes);
	}

	MtSupportDebugWriteLine(gBuffer);
  }
#endif
  return EFI_SUCCESS;
}


/**
  Initializes the memory test ranges

**/
EFI_STATUS
MtRangesConstructor (
  )
{
  return ReadMemoryRanges ();
}


/**
  Decontructs the memory test ranges data structures

**/
EFI_STATUS
MtRangesDeconstructor (
  )
{
  if (mMemoryMap != NULL) {
    FreePool (mMemoryMap);
    mMemoryMap = NULL;
  }

  return EFI_SUCCESS;
}


