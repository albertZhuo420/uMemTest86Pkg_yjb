//PassMark MemTest86
//
//Copyright (c) 2016
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
//	SMBIOS.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Functions for parsing the SMBIOS table
//
//  Based on source code from EDK2

#ifndef __SMBIOS_H
#define __SMBIOS_H

#include <IndustryStandard/SmBios.h>

#define SMB_STRINGLEN				64

// SMBIOS Structure types
enum {

	E_BIOS_INFORMATION				= 0,
	E_SYSTEM_INFORMATION			= 1,
	E_BASEBOARD_INFORMATION			= 2,
	E_SYSTEMCHASSIS_INFORMATION		= 3,
	E_PROCESSOR_INFORMATION			= 4,
	E_MEMORYCONTROLLER_INFORMATION	= 5,
	E_MEMORYMODULE_INFORMATION		= 6,
	E_CACHE_INFORMATION				= 7,
	E_SYSTEM_SLOTS					= 9,
	E_ONBOARD_DEVICES_INFORMATION	= 10,
	E_PHYSICAL_MEMORY_ARRAY			= 16,
	E_MEMORY_DEVICE					= 17,
	E_MEMORY_ERROR_INFORMATION		= 18,

	E_ENDOFTABLE					= 127,
};

#pragma pack(1)
typedef struct {
	SMBIOS_TABLE_TYPE0		smb0;

	CHAR8		szVendor[SMB_STRINGLEN+1];
	CHAR8		szBIOSVersion[SMB_STRINGLEN+1];
	CHAR8		szBIOSReleaseDate[SMB_STRINGLEN+1];
} SMBIOS_BIOS_INFORMATION;

typedef struct {
	SMBIOS_TABLE_TYPE1		smb1;	

	CHAR8		szManufacturer[SMB_STRINGLEN+1];
	CHAR8		szProductName[SMB_STRINGLEN+1];
	CHAR8		szVersion[SMB_STRINGLEN+1];
	CHAR8		szSerialNumber[SMB_STRINGLEN+1];
	CHAR8		szSKUNumber[SMB_STRINGLEN+1];
	CHAR8		szFamily[SMB_STRINGLEN+1];
} SMBIOS_SYSTEM_INFORMATION;

typedef struct {
	SMBIOS_TABLE_TYPE2		smb2;
	UINT16                  ContainedObjectHandles[16];

	CHAR8		szManufacturer[SMB_STRINGLEN+1];
	CHAR8		szProductName[SMB_STRINGLEN+1];
	CHAR8		szVersion[SMB_STRINGLEN+1];
	CHAR8		szSerialNumber[SMB_STRINGLEN+1];
	CHAR8		szAssetTag[SMB_STRINGLEN+1];
	CHAR8		szLocationInChassis[SMB_STRINGLEN+1];
	
} SMBIOS_BASEBOARD_INFORMATION;

typedef struct _SMBIOS_MEMORY_DEVICE {
	SMBIOS_TABLE_TYPE17		smb17;
	
	// Strings
	CHAR8		szDeviceLocator[SMB_STRINGLEN+1];
	CHAR8		szBankLocator[SMB_STRINGLEN+1];
	CHAR8		szManufacturer[SMB_STRINGLEN+1];
	CHAR8		szSerialNumber[SMB_STRINGLEN+1];
	CHAR8		szAssetTag[SMB_STRINGLEN+1];
	CHAR8		szPartNumber[SMB_STRINGLEN+1];
	CHAR8		szFirmwareVersion[SMB_STRINGLEN + 1];

} SMBIOS_MEMORY_DEVICE;
#pragma pack()

UINT16 GetNumSMBIOSStruct(UINT8 byType);

int GetBiosInformationStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_BIOS_INFORMATION *pSMBIOSBIOSInfo);

int GetSystemInformationStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_SYSTEM_INFORMATION *pSMBIOSSysInfo);

int GetBaseboardInformationStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_BASEBOARD_INFORMATION *pSMBIOSBaseBoardInfo);

int GetMemoryDeviceStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_MEMORY_DEVICE *pSMBIOSMemDevice);


#endif