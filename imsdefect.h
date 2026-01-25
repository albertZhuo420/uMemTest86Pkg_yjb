//PassMark MemTest86
//
//Copyright (c) 2019
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
//	imsdefect.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Glue code for iMS interface
//
//

#ifndef _IMS_DEFECT_H_
#define _IMS_DEFECT_H_

#define IMS_MAX_DEFECT_ENTRIES      64
#define IMS_MEMORY_DEFECT_SOURCE_MEMTEST86		0x10

#pragma pack(1)
typedef struct _MEMORY_DEFECT {
  UINT8   Enable;			// Enable the defect or not. Set 1 enable;
  UINT8   Source;			// Which Program write the defect
  UINT32  Addr;				// Memory error address (M)
  UINT32  Length;			// Mask Size (M)
} MEMORY_DEFECT;

typedef struct _MEMORY_DEFECT_LIST {
  UINT16          Count;													// Number of defect entries logged
  UINT32          MaskSizeLimit;									// Max Mask memory size(M) 
  UINT32          TotalMaskSize;									// Mask memory size
  UINT32          MaskSizeBelow4G;								// Mask memory when address < 4G or not
  UINT32          UpdateDateTime;									// Update time
  MEMORY_DEFECT   Defect[IMS_MAX_DEFECT_ENTRIES];	// Defect entries, default maximum defect entries is 64
} MEMORY_DEFECT_LIST;

typedef struct _IMS_NVMEM_DEFECT_LIST {
  UINT64              Signature;									//	"_iMS"
  UINT16              Revision;										// Interface definition version, initial default version: 0x100
  UINT16              Crc16;											// 16-bit CRC checksum of the variable from start to end;
  UINT8               Crypt;											//
  MEMORY_DEFECT_LIST  MemDefectList;							// Defect List struct
} IMS_NVMEM_DEFECT_LIST;
#pragma pack()

#define EFI_VARIABLE_NON_VOLATILE                            0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS                      0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS                          0x00000004

#define IMS_DEFECT_LIST_NVAR_NAME L"NvMemDefectList"
#define IMS_DEFECT_LIST_NVAR_GUID \
  { 0x7E71233D, 0xF31F, 0x48F2, { 0xab, 0x37, 0xb7, 0x6c, 0xa7, 0xb8, 0x22, 0x78 } }

extern EFI_GUID gIMSDefectListNvarGuid;

BOOLEAN
EFIAPI
iMSIsAvailable();

EFI_STATUS
EFIAPI
iMSSetDefectList(IMS_NVMEM_DEFECT_LIST *DefectList);

EFI_STATUS
EFIAPI
iMSGetDefectList(IMS_NVMEM_DEFECT_LIST *DefectList);

#define IMS_VAR_NAME L"IMS"
#define IMS_VAR_GUID { 0xd8195d8b, 0x747b, 0x44ee, { 0xbd, 0x57, 0x98, 0xe7, 0xce, 0x0, 0x3, 0x51 } }
extern EFI_GUID   gIMSVarGuid;

//check if run from bios.
BOOLEAN
EFIAPI
iMSIsRunFromBios();

//add 08-01-2018
//modify 08-30-2018

#define IMS_TO_BIOS_HOB_GUID { 0xC4E11690, 0xB13C, 0x499F, {0xAB, 0x33, 0x19, 0x58, 0x4C, 0x09, 0xB8, 0x40 } }
extern EFI_GUID gIMSToBiosHobGuid;

#pragma pack(1)
typedef struct{
  UINT8 iMSFreeVersion;                //1 means free version, 0 means paid for  version
  UINT8 DimmIdErr;
  UINT8 BiosAreaErr;
  UINT8 MemErrFull;
  UINT32 iMSVersion;
  UINT32 iMSModuleBuildDate;
}IMS_TO_BIOS_HOB_ST;
#pragma pack()

EFI_STATUS
EFIAPI
iMSGetVersionFromHob(IMS_TO_BIOS_HOB_ST *iMSToBiosHob);

//add 08-30-2018
#define IMS_ADDRESS_TRANSLATION_PROTOCOL_GUID { 0x3188b642, 0x2060, 0x4f5a, { 0xa7, 0x4d, 0x84, 0xd6, 0x1e, 0xb3, 0xe9, 0x2 } }
extern EFI_GUID gIMSAddressTranslationDxeProtocolGuid;

typedef
EFI_STATUS
(EFIAPI *EFI_SYSTEM_ADDRESS_TO_DIMM_ADDRESS)(
  IN UINT64    SystemAddress,
  OUT  UINT8   *SocketId,
  OUT  UINT8   *MemoryControllerId,
  OUT  UINT8   *ChannelId,
  OUT  UINT8   *DimmSlot,
  OUT  UINT8   *DimmRank
  );

typedef struct _IMS_ADDRESS_TRANSLATION_PROTOCOL {
  EFI_SYSTEM_ADDRESS_TO_DIMM_ADDRESS    SystemAddressToDimmAddress;
} IMS_ADDRESS_TRANSLATION_PROTOCOL;

EFI_STATUS iMSSystemAddressToDimmAddress(
	EFI_PHYSICAL_ADDRESS PhysAddr,
	UINT8 *SocketId,
	UINT8 *MemoryControllerId,
	UINT8 *ChannelId,
	UINT8 *DimmSlot,
	UINT8 *DimmRank);

#endif