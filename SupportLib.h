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
//	SupportLib.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Includes and struct declarations for SupportLib.c
//
//References:
//  https://github.com/jljusten/MemTestPkg
//

/** @file
  Memory test support library

  Copyright (c) 2006 - 2009, Intel Corporation
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

#ifndef _SUPPORT_LIB_H_INCLUDED_
#define _SUPPORT_LIB_H_INCLUDED_

#include <Uefi.h>

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
#include <mmintrin.h>
#include <xmmintrin.h>  //SSE intrinsics
#include <emmintrin.h>  //SSE2 intrinsics
#include <smmintrin.h>  //SSE4.1 intrinsics
#elif defined(MDE_CPU_AARCH64)
#include <sse2neon.h>
#include <Library/ArmLib.h>
#endif

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/HiiLib.h>
#include <Library/Cpuid.h>
#include <Library/NetLib.h>
#include <Library/IoLib.h>
#include <Library/MtrrLib.h>

#include<Protocol/UnicodeCollation.h>
#include <Protocol/PxeBaseCode.h>
#include<Protocol/BlockIo.h>

#include <Guid/GlobalVariable.h>
#include <Guid/HardwareErrorVariable.h>
#include <Guid/Cper.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
// #include <Library/ShellLib.h>

#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>
#include <Library/MemTestRangesLib.h>
#include <Library/MemTestUiLib.h>
#include <Library/Rand.h>
#include <Library/libeg/BmLib.h>

#include <Tests/BitFade/BitFade.h>
#include <Tests/RandomNumSequence128/RandomNumSequence128.h>
#include <Tests/Hammer/Hammer.h>

#include <SysInfoLib/cpuinfo.h>
#include <SysInfoLib/cpuinfoMSRIntel.h>
#include <SysInfoLib/pci.h>
#include <SysInfoLib/smbus.h>
#include <SysInfoLib/SMBIOS.h>
#include <uMemTest86.h>

#include <SysInfoLib/controller.h>
#include <SysInfoLib/cputemp.h>

#include <Protocol/Tcp4.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Dns4.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Tls.h>

#include <imsdefect.h>

#include "AdvancedMemoryTest.h"

typedef struct {
  UINT32                   TestNum;
  TEST_MEM_RANGE           RangeTest;
  UINT32                   MaxCPUs;
  UINT64                   IterCount;
  VOID                     *Context;
  UINTN                    Align;
  TESTPAT                  Pattern;
  UINT64                   PatternUser;
} MEM_RANGE_TEST_DATA;

typedef struct {
  UINT32                   TestNum;
  TEST_MEM_RANGE           RangeTest1;
  TEST_MEM_RANGE           RangeTest2;
  UINT32                   MaxCPUs;
  UINT64                   IterCount;
  VOID                     *Context;
  UINTN                    Align;
  TESTPAT                  Pattern;
  UINT64                   PatternUser;
} BIT_FADE_TEST_DATA;

typedef struct {
  UINT32                   TestNum;
  TEST_MEM_RANGE           InitMem;
  TEST_MEM_RANGE           FastHammer;
  TEST_MEM_RANGE           SlowHammer;
  TEST_MEM_RANGE           VerifyMem;
  UINT32                   MaxCPUs;
  UINT64                   IterCount;
  VOID                     *Context;
  UINTN                    Align;
  TESTPAT                  Pattern;
  UINT64                   PatternUser;
  BOOLEAN                  MultiThreaded;
} HAMMER_TEST_DATA;

typedef struct {
  UINT32				TestNo;
  CHAR16*               NameStr;
  EFI_STRING_ID         NameStrId;
  CACHEMODE              CacheMode;
  RUN_MEM_TEST          RunMemTest;
  VOID                  *Context;
  UINTN					NumPassed;
  UINTN					NumCompleted;
  UINTN					NumErrors;
  UINTN					NumWarnings;
} MEM_TEST_INSTANCE;

enum {
	HANDLE_TYPE_DISK = 0,
	HANDLE_TYPE_PXE
};


#pragma pack(1)
typedef struct _FILE_DATA_HEADER {
	UINT8				FileHandleType;
} FILE_HANDLE_HEADER;
#pragma pack()

typedef struct _PXE_FILE_DATA {
	FILE_HANDLE_HEADER	Header;
	CHAR8				Filename[256];
	UINT64				FileSize;
	UINT64				OpenMode;
	UINT64				FilePointer;
	UINT8				*Buffer;
	UINTN				BufferSize;
} PXE_FILE_DATA;

typedef struct _DISK_FILE_DATA {
	FILE_HANDLE_HEADER	Header;
	EFI_FILE_PROTOCOL	*EfiFileProtocol;
} DISK_FILE_DATA;

#endif

