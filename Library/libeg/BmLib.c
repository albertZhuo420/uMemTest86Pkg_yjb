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
//	BmLib.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Utility functions used in the libeg library
//
//  Source code based on refit/libeg code base

/** @file
  Utility routines used by boot maintenance modules.

Copyright (c) 2004 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemTestSupportLib.h>

#include "BmLib.h"

/**

  Function opens and returns a file handle to the root directory of a volume.

  @param DeviceHandle    A handle for a device

  @return A valid file handle or NULL is returned

**/
EFI_FILE_HANDLE
EfiLibOpenRoot (
  IN EFI_HANDLE                   DeviceHandle
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
  EFI_FILE_HANDLE                 File;

  File = NULL;

  //
  // File the file system interface to the device
  //
  Status = gBS->HandleProtocol (
                  DeviceHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &Volume
                  );

  //
  // Open the root directory of the volume
  //
  if (!EFI_ERROR (Status)) {
    Status = Volume->OpenVolume (
                      Volume,
                      &File
                      );
  }
  //
  // Done
  //
  return EFI_ERROR (Status) ? NULL : File;
}

/**

  Function gets the file system information from an open file descriptor,
  and stores it in a buffer allocated from pool.


  @param FHand           The file handle.

  @return                A pointer to a buffer with file information.
  @retval                NULL is returned if failed to get Volume Label Info.

**/
EFI_FILE_SYSTEM_VOLUME_LABEL *
EfiLibFileSystemVolumeLabelInfo (
  IN EFI_FILE_HANDLE      FHand
  )
{
  EFI_STATUS    Status;
  EFI_FILE_SYSTEM_VOLUME_LABEL *VolumeInfo = NULL;
  UINTN         Size = 0;
  
  Status = FHand->GetInfo (FHand, &gEfiFileSystemVolumeLabelInfoIdGuid, &Size, VolumeInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    VolumeInfo = AllocateZeroPool (Size);
    Status = FHand->GetInfo (FHand, &gEfiFileSystemVolumeLabelInfoIdGuid, &Size, VolumeInfo);
  }
  
  return EFI_ERROR(Status)?NULL:VolumeInfo;  
}

/**
  Duplicate a string.

  @param Src             The source.

  @return A new string which is duplicated copy of the source.
  @retval NULL If there is not enough memory.

**/
CHAR16 *
EfiStrDuplicate (
  IN CHAR16   *Src
				 )
{
  CHAR16  *Dest;
  UINTN   Size;

  Size  = StrSize (Src); //at least 2bytes
  Dest  = AllocateZeroPool (Size);
//  ASSERT (Dest != NULL);
  if (Dest != NULL) {
    CopyMem (Dest, Src, Size);
  }

  return Dest;
}
//Compare strings case insensitive
INTN
EFIAPI
StriCmp (
		IN      CONST CHAR16              *FirstString,
		IN      CONST CHAR16              *SecondString
		)
{
	
	while ((*FirstString != L'\0') && ((*FirstString & ~0x20) == (*SecondString & ~0x20))) {
		FirstString++;
		SecondString++;
	}
	return *FirstString - *SecondString;
}


/**

  Function gets the file information from an open file descriptor, and stores it
  in a buffer allocated from pool.

  @param FHand           File Handle.

  @return                A pointer to a buffer with file information or NULL is returned

**/
EFI_FILE_INFO *
EfiLibFileInfo (
  IN EFI_FILE_HANDLE      FHand
  )
{
  EFI_STATUS    Status;
  EFI_FILE_INFO *FileInfo = NULL;
  UINTN         Size = 0;
  
  Status = FHand->GetInfo (FHand, &gEfiFileInfoGuid, &Size, FileInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileInfo = AllocateZeroPool (Size);
    Status = FHand->GetInfo (FHand, &gEfiFileInfoGuid, &Size, FileInfo);
  }
  
  return EFI_ERROR(Status)?NULL:FileInfo;
}

EFI_FILE_SYSTEM_INFO *
EfiLibFileSystemInfo (
                IN EFI_FILE_HANDLE      FHand
                )
{
  EFI_STATUS    Status;
  EFI_FILE_SYSTEM_INFO *FileSystemInfo = NULL;
  UINTN         Size = 0;
  
  Status = FHand->GetInfo (FHand, &gEfiFileSystemInfoGuid, &Size, FileSystemInfo);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileSystemInfo = AllocateZeroPool (Size);
    Status = FHand->GetInfo (FHand, &gEfiFileSystemInfoGuid, &Size, FileSystemInfo);
  }
  
  return EFI_ERROR(Status)?NULL:FileSystemInfo;
}

/**
  Function is used to determine the number of device path instances
  that exist in a device path.


  @param DevicePath      A pointer to a device path data structure.

  @return This function counts and returns the number of device path instances
          in DevicePath.

**/
UINTN
EfiDevicePathInstanceCount (
  IN EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  )
{
  UINTN Count;
  UINTN Size;

  Count = 0;
  while (GetNextDevicePathInstance (&DevicePath, &Size) != NULL) {
    Count += 1;
  }

  return Count;
}

/**
  Adjusts the size of a previously allocated buffer.


  @param OldPool         - A pointer to the buffer whose size is being adjusted.
  @param OldSize         - The size of the current buffer.
  @param NewSize         - The size of the new buffer.

  @return   The newly allocated buffer.
  @retval   NULL  Allocation failed.

**/
VOID *
EfiReallocatePool (
  IN VOID                 *OldPool,
  IN UINTN                OldSize,
  IN UINTN                NewSize
  )
{
  VOID  *NewPool;

  NewPool = NULL;
  if (NewSize != 0) {
    NewPool = AllocateZeroPool (NewSize);
  }

  if (OldPool != NULL) {
    if (NewPool != NULL) {
      CopyMem (NewPool, OldPool, OldSize < NewSize ? OldSize : NewSize);
    }

    FreePool (OldPool);
  }

  return NewPool;
}

/**
  Compare two EFI_TIME data.


  @param FirstTime       - A pointer to the first EFI_TIME data.
  @param SecondTime      - A pointer to the second EFI_TIME data.

  @retval  TRUE              The FirstTime is not later than the SecondTime.
  @retval  FALSE             The FirstTime is later than the SecondTime.

**/
BOOLEAN
TimeCompare (
  IN EFI_TIME               *FirstTime,
  IN EFI_TIME               *SecondTime
  )
{
  if (FirstTime->Year != SecondTime->Year) {
    return (BOOLEAN) (FirstTime->Year < SecondTime->Year);
  } else if (FirstTime->Month != SecondTime->Month) {
    return (BOOLEAN) (FirstTime->Month < SecondTime->Month);
  } else if (FirstTime->Day != SecondTime->Day) {
    return (BOOLEAN) (FirstTime->Day < SecondTime->Day);
  } else if (FirstTime->Hour != SecondTime->Hour) {
    return (BOOLEAN) (FirstTime->Hour < SecondTime->Hour);
  } else if (FirstTime->Minute != SecondTime->Minute) {
    return (BOOLEAN) (FirstTime->Minute < SecondTime->Minute);
  } else if (FirstTime->Second != SecondTime->Second) {
    return (BOOLEAN) (FirstTime->Second < SecondTime->Second);
  }

  return (BOOLEAN) (FirstTime->Nanosecond <= SecondTime->Nanosecond);
}

/**
  Get a string from the Data Hub record based on 
  a device path.

  @param DevPath         The device Path.

  @return A string located from the Data Hub records based on
          the device path.
  @retval NULL  If failed to get the String from Data Hub.

**/
/*
UINT16 *
EfiLibStrFromDatahub (
  IN EFI_DEVICE_PATH_PROTOCOL                 *DevPath
  )
{
  return NULL;
}*/


EFI_STATUS DirNextEntry(IN EFI_FILE *Directory, IN OUT EFI_FILE_INFO **DirEntry, IN UINTN FilterMode)
{
    EFI_STATUS Status;
    VOID *Buffer;
    UINTN LastBufferSize, BufferSize;
    INTN IterCount;
    
    for (;;) {
        
        // free pointer from last call
        if (*DirEntry != NULL) {
            FreePool(*DirEntry);
            *DirEntry = NULL;
        }
        
        // read next directory entry
        LastBufferSize = BufferSize = 256;
        Buffer = AllocateZeroPool (BufferSize);
        for (IterCount = 0; ; IterCount++) {
            Status = Directory->Read(Directory, &BufferSize, Buffer);
            if (Status != EFI_BUFFER_TOO_SMALL || IterCount >= 4)
                break;
            if (BufferSize <= LastBufferSize) {
                BufferSize = LastBufferSize * 2;
            }
			Buffer = ReallocatePool(LastBufferSize, BufferSize, Buffer);
            LastBufferSize = BufferSize;
        }
        if (EFI_ERROR(Status)) {
			AsciiFPrint(DEBUG_FILE_HANDLE, "Read dir failed (%r)", Status);
            FreePool(Buffer);
            break;
        }
        
        // check for end of listing
        if (BufferSize == 0) {    // end of directory listing
            FreePool(Buffer);
            break;
        }
        
        // entry is ready to be returned
        *DirEntry = (EFI_FILE_INFO *)Buffer;

		AsciiFPrint(DEBUG_FILE_HANDLE, "Found Dir entry (Size=%ld, FileSize=%ld, PhysSize=%ld, Attribute=%ld, Filenamelen=%d)", (*DirEntry)->Size, (*DirEntry)->FileSize, (*DirEntry)->PhysicalSize, (*DirEntry)->Attribute, (*DirEntry)->Size - OFFSET_OF(EFI_FILE_INFO, FileName));

      if (*DirEntry) {
        // filter results
        if (FilterMode == 1) {   // only return directories
          if (((*DirEntry)->Attribute & EFI_FILE_DIRECTORY))
            break;
        } else if (FilterMode == 2) {   // only return files
          if (((*DirEntry)->Attribute & EFI_FILE_DIRECTORY) == 0)
            break;
        } else                   // no filter or unknown filter -> return everything
          break;
      }
    }
    return Status;
}

VOID DirIterOpen(IN EFI_FILE *BaseDir, IN CHAR16 *RelativePath OPTIONAL, OUT REFIT_DIR_ITER *DirIter)
{
	SetMem(DirIter, sizeof(REFIT_DIR_ITER), 0);
    if (RelativePath == NULL) {
        DirIter->LastStatus = EFI_SUCCESS;
        DirIter->DirHandle = BaseDir;
        DirIter->CloseDirHandle = FALSE;
    } else {
        DirIter->LastStatus = BaseDir->Open(BaseDir, &(DirIter->DirHandle), RelativePath, EFI_FILE_MODE_READ, 0);
        DirIter->CloseDirHandle = DirIter->DirHandle != NULL ? TRUE : FALSE;
    }

	if (DirIter->LastStatus == EFI_SUCCESS)
	{
		DirIter->DirHandle->SetPosition(DirIter->DirHandle, 0);
	}
    DirIter->LastFileInfo = NULL;
}

BOOLEAN DirIterNext(IN OUT REFIT_DIR_ITER *DirIter, IN UINTN FilterMode, IN CHAR16 *FilePattern OPTIONAL,
                    OUT EFI_FILE_INFO **DirEntry)
{
    if (DirIter->LastFileInfo != NULL) {
        FreePool(DirIter->LastFileInfo);
        DirIter->LastFileInfo = NULL;
    }
    
    if (EFI_ERROR(DirIter->LastStatus))
        return FALSE;   // stop iteration
    
    for (;;) {
        DirIter->LastStatus = DirNextEntry(DirIter->DirHandle, &(DirIter->LastFileInfo), FilterMode);
        if (EFI_ERROR(DirIter->LastStatus))
            return FALSE;
        if (DirIter->LastFileInfo == NULL)  // end of listing
            return FALSE;
        if (FilePattern != NULL) {
			CHAR16 FileName[128];
			UINTN MaxLen = sizeof(FileName) / sizeof(FileName[0]) - 1;
			UINTN FileNameLen = (UINTN)(DirIter->LastFileInfo->Size - OFFSET_OF(EFI_FILE_INFO, FileName)) / sizeof(CHAR16);
			SetMem(FileName, sizeof(FileName), 0);

            if ((DirIter->LastFileInfo->Attribute & EFI_FILE_DIRECTORY))
                break;

			//if (MetaiMatch(DirIter->LastFileInfo->FileName, FilePattern))
            //    break;
			StrnCpyS(FileName, ARRAY_SIZE(FileName), DirIter->LastFileInfo->FileName, MIN(FileNameLen, MaxLen));
			if (StrStr(FileName, FilePattern) != NULL)
				break;
            
            // else continue loop
        } else
            break;
    }
    
    *DirEntry = DirIter->LastFileInfo;
    return TRUE;
}

EFI_STATUS DirIterClose(IN OUT REFIT_DIR_ITER *DirIter)
{
    if (DirIter->LastFileInfo != NULL) {
        FreePool(DirIter->LastFileInfo);
        DirIter->LastFileInfo = NULL;
    }
    if (DirIter->CloseDirHandle)
	{
        DirIter->DirHandle->Close(DirIter->DirHandle);
		DirIter->DirHandle = NULL;
	}

    return DirIter->LastStatus;
}
