#PassMark MemTest86
#
#Copyright (c) 2013-2016
#	This software is the confidential and proprietary information of PassMark
#	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
#	such Confidential Information and shall use it only in accordance with
#	the terms of the license agreement you entered into with PassMark
#	Software.
#
#Program:
#	MemTest86
#
#Module:
#	uMemTest86Pjg.dsc
#
#Author(s):
#	Keith Mah
#

[Defines]
  PLATFORM_NAME                  = MemTest86
  PLATFORM_GUID                  = 2CFC3A35-F4A7-49B1-9FBC-81504BE5AE14
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/MemTest86
  SUPPORTED_ARCHITECTURES        = IA32|IPF|X64|EBC|AARCH64
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT

[BuildOptions.Common]
  MSFT:*_*_*_CC_FLAGS    = /D FREE_RELEASE
  MSFT:*_*_*_DLINK_FLAGS	=	/ALIGN:4096
  GCC:*_*_*_CC_FLAGS     = -DFREE_RELEASE
  GCC:*_*_*_DLINK_FLAGS = -z common-page-size=0x1000

[LibraryClasses.common]
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  CpuidLib|uMemTest86Pkg/Library/CpuidLib/CpuidLib.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  FileHandleLib|MdePkg/Library/UefiFileHandleLib/UefiFileHandleLib.inf
  LibEg|uMemTest86Pkg/Library/libeg/libeg.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  MemTestRangesLib|uMemTest86Pkg/Library/RangesLib/RangesLib.inf
  MemTestMPSupportLib|uMemTest86Pkg/Library/MPSupportLib/MPSupportLib.inf
  NetLib|NetworkPkg/Library/DxeNetLib/DxeNetLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PointerLib|uMemTest86Pkg/Library/PointerLib/PointerLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  RandLib|uMemTest86Pkg/Library/RandLib/RandLib.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
  SysInfoLib|uMemTest86Pkg/Library/SysInfoLib/SysInfoLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
  UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
  YAMLLib|uMemTest86Pkg/Library/YAMLLib/YAMLLib.inf

[LibraryClasses.X64, LibraryClasses.IA32]
  MtrrLib|UefiCpuPkg/Library/MtrrLib/MtrrLib.inf
  TimerLib|MdePkg/Library/SecPeiDxeTimerLibCpu/SecPeiDxeTimerLibCpu.inf

[LibraryClasses.AARCH64]
  ArmGenericTimerCounterLib|ArmPkg/Library/ArmGenericTimerPhyCounterLib/ArmGenericTimerPhyCounterLib.inf
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  NULL|ArmPkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

  # Add support for GCC stack protector
  NULL|MdePkg/Library/BaseStackCheckLib/BaseStackCheckLib.inf

[Components.common]
  uMemTest86Pkg/uMemTest86.inf {
#    <PcdsFixedAtBuild>
#      gEfiMdePkgTokenSpaceGuid.PcdReportStatusCodePropertyMask|0x07
#      gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x2F
#      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000040
    <LibraryClasses>
	  NULL|uMemTest86Pkg/Tests/AddressWalkingOnes/AddressWalkingOnes.inf
	  NULL|uMemTest86Pkg/Tests/AddressOwn/AddressOwn.inf
	  NULL|uMemTest86Pkg/Tests/MovingInv/MovingInv.inf
	  NULL|uMemTest86Pkg/Tests/BlockMove/BlockMove.inf
	  NULL|uMemTest86Pkg/Tests/MovingInv32bitPattern/MovingInv32bitPattern.inf
	  NULL|uMemTest86Pkg/Tests/RandomNumSequence/RandomNumSequence.inf
	  NULL|uMemTest86Pkg/Tests/Modulo20/Modulo20.inf
	  NULL|uMemTest86Pkg/Tests/BitFade/BitFade.inf
	  NULL|uMemTest86Pkg/Tests/RandomNumSequence64/RandomNumSequence64.inf
	  NULL|uMemTest86Pkg/Tests/RandomNumSequence128/RandomNumSequence128.inf
	  NULL|uMemTest86Pkg/Tests/Hammer/Hammer.inf
	  NULL|uMemTest86Pkg/Tests/DMATest/DMATest.inf
  }

