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
//	SMBIOS.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Functions for parsing the SMBIOS table
//
//  Based on source code from EDK2

/** @file
  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Guid/SmBios.h>
#include <IndustryStandard/SmBios.h>

#include <Library/MemTestSupportLib.h>

#include "SMBIOS.h"

STATIC SMBIOS_TABLE_ENTRY_POINT *mSmbiosTable = NULL;
STATIC SMBIOS_STRUCTURE_POINTER m_SmbiosStruct;
STATIC SMBIOS_STRUCTURE_POINTER *mSmbiosStruct = &m_SmbiosStruct;

static void trim(char* str)
{
    if(!str)
        return;

    char* ptr = str;
    UINTN len = AsciiStrLen(ptr);

    while(len-1 > 0 && ptr[len-1] == ' ')
        ptr[--len] = 0;

    while(*ptr && *ptr == ' ')
        ++ptr, --len;

    CopyMem(str, ptr, len + 1);
}

/**
  Function returns a system configuration table that is stored in the
  EFI System Table based on the provided GUID.

  @param[in]  TableGuid     A pointer to the table's GUID type.
  @param[in, out] Table     On exit, a pointer to a system configuration table.

  @retval EFI_SUCCESS      A configuration table matching TableGuid was found.
  @retval EFI_NOT_FOUND    A configuration table matching TableGuid was not found.
**/
EFI_STATUS
GetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
{
  UINTN Index;

  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (TableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      *Table = gST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Init the SMBIOS VIEW API's environment.

  @retval EFI_SUCCESS  Successful to init the SMBIOS VIEW Lib.
**/
EFI_STATUS
LibSmbiosInit (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Get SMBIOS table from System Configure table
  //
  Status = GetSystemConfigurationTable (&gEfiSmbiosTableGuid, (VOID**)&mSmbiosTable);

  if (mSmbiosTable == NULL) {
    // ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_CANNOT_GET_TABLE), gShellDebug1HiiHandle);
    return EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    // ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_GET_TABLE_ERROR), gShellDebug1HiiHandle, Status);
    return Status;
  }
  //
  // Init SMBIOS structure table address
  //
  mSmbiosStruct->Raw  = (UINT8 *) (UINTN) (mSmbiosTable->TableAddress);

  return EFI_SUCCESS;
}

/**
  Return SMBIOS string for the given string number.

  @param[in] Smbios         Pointer to SMBIOS structure.
  @param[in] StringNumber   String number to return. -1 is used to skip all strings and
                            point to the next SMBIOS structure.

  @return Pointer to string, or pointer to next SMBIOS strcuture if StringNumber == -1
**/
CHAR8*
LibGetSmbiosString (
  IN  SMBIOS_STRUCTURE_POINTER    *Smbios,
  IN  UINT16                      StringNumber
  )
{
  UINT16  Index;
  CHAR8   *String;


  //
  // Skip over formatted section
  //
  String = (CHAR8 *) (Smbios->Raw + Smbios->Hdr->Length);

  //
  // Look through unformated section
  //
  for (Index = 1; Index <= StringNumber; Index++) {
    if (StringNumber == Index) {
      return String;
    }
    //
    // Skip string
    //
    for (; *String != 0; String++);
    String++;

    if (*String == 0) {
      //
      // If double NULL then we are done.
      //  Return pointer to next structure in Smbios.
      //  if you pass in a -1 you will always get here
      //
      Smbios->Raw = (UINT8 *)++String;
      return NULL;
    }
  }

  return NULL;
}

/**
    Get SMBIOS structure for the given Handle,
    Handle is changed to the next handle or 0xFFFF when the end is
    reached or the handle is not found.

    @param[in, out] Handle     0xFFFF: get the first structure
                               Others: get a structure according to this value.
    @param[out] Buffer         The pointer to the pointer to the structure.
    @param[out] Length         Length of the structure.

    @retval DMI_SUCCESS   Handle is updated with next structure handle or
                          0xFFFF(end-of-list).

    @retval DMI_INVALID_HANDLE  Handle is updated with first structure handle or
                                0xFFFF(end-of-list).
**/
BOOLEAN
LibGetSmbiosStructure (
  IN UINT8        Type, 
  IN UINT16	      Index,
  OUT UINT8       **Buffer,
  OUT UINT16      *Length
  )
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINT8                     *Raw;
  UINT16                    Count = 0;

  if ((Buffer == NULL) || (Length == NULL)) {
    // ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_NO_BUFF_LEN_SPEC), gShellDebug1HiiHandle);
    return FALSE;
  }

  if (mSmbiosTable == NULL)
	  LibSmbiosInit();

  if (mSmbiosTable == NULL)
	  return FALSE;

  *Length       = 0;
  Smbios.Hdr    = mSmbiosStruct->Hdr;
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosTable->TableLength;
  while (Smbios.Raw < SmbiosEnd.Raw) {
    if (Smbios.Hdr->Type == Type) {
		if (Index == Count)
		{
		  Raw = Smbios.Raw;
		  //
		  // Walk to next structure
		  //
		  LibGetSmbiosString (&Smbios, (UINT16) (-1));
		  //
		  // Length = Next structure head - this structure head
		  //
		  *Length = (UINT16) (Smbios.Raw - Raw);
		  *Buffer = Raw;

		  return TRUE;
		}
		Count++;
    }
    //
    // Walk to next structure
    //
    LibGetSmbiosString (&Smbios, (UINT16) (-1));
  }

  return FALSE;
}

// Returns the amount of structures of type byType in the table

UINT16 GetNumSMBIOSStruct(UINT8 byType)
{
	UINT16 iCount = 0;

	while(1)
	{
		UINT8 * pbLinAddr;
		UINT16	iTotalLen	= 0;

		if(!LibGetSmbiosStructure(byType, iCount, &pbLinAddr, &iTotalLen))
			break;

		iCount++;
	}

	return iCount;
}


//
//////// Structure Parsing Functions /////////////////////////////////////////////
//

// Fills a SMB_BIOS_INFORMATION structure from the address pointed to by pbLinAddr
// and returns the size of all data read in bytes.

int GetBiosInformationStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_BIOS_INFORMATION *pSMBIOSBIOSInfo)
{
	UINT16	iTotalLen	= 0;
	int		iStringAreaLen	= 0;
	int		iIndex = 0;
	char szStringDesc[3][SMB_STRINGLEN+1];
	UINT16 i = 0;

	if (LibGetSmbiosStructure(E_BIOS_INFORMATION, iNum, &pbLinAddr, &iTotalLen))
	{
		SMBIOS_STRUCTURE_POINTER  Smbios;
		Smbios.Raw = pbLinAddr;

		CopyMem(&pSMBIOSBIOSInfo->smb0, pbLinAddr, Smbios.Hdr->Length);

		// String Area.
		// Get the strings (MAX 3)
		i = 0;
		while (i < 3)
		{
			CHAR8 *pszCurString;
			Smbios.Raw    = pbLinAddr;
			pszCurString = LibGetSmbiosString(&Smbios,i + 1);
			if (pszCurString == NULL)
				break;

			AsciiStrnCpyS(szStringDesc[i], SMB_STRINGLEN + 1, pszCurString, SMB_STRINGLEN);
			iStringAreaLen += (int)AsciiStrLen(pszCurString)+1;
			i++;
		}
		
		// Double Null terminator at end of string area.
		iStringAreaLen++;

		
		if(pSMBIOSBIOSInfo->smb0.Vendor)
		{
			iIndex = pSMBIOSBIOSInfo->smb0.Vendor - 1;
			if (iIndex >= 0 && iIndex < 3)
			{
				AsciiStrCpyS(pSMBIOSBIOSInfo->szVendor, sizeof(pSMBIOSBIOSInfo->szVendor), szStringDesc[iIndex]);
				trim(pSMBIOSBIOSInfo->szVendor);
			}
		}
	
		if(pSMBIOSBIOSInfo->smb0.BiosVersion)
		{
			iIndex = pSMBIOSBIOSInfo->smb0.BiosVersion - 1;
			if (iIndex >= 0 && iIndex < 3)
			{
				AsciiStrCpyS(pSMBIOSBIOSInfo->szBIOSVersion, sizeof(pSMBIOSBIOSInfo->szBIOSVersion), szStringDesc[iIndex]);
				trim(pSMBIOSBIOSInfo->szBIOSVersion);
			}
		}

		if(pSMBIOSBIOSInfo->smb0.BiosReleaseDate)
		{
			iIndex = pSMBIOSBIOSInfo->smb0.BiosReleaseDate - 1;
			if (iIndex >= 0 && iIndex < 3)
			{
				AsciiStrCpyS(pSMBIOSBIOSInfo->szBIOSReleaseDate, sizeof(pSMBIOSBIOSInfo->szBIOSReleaseDate), szStringDesc[iIndex]);
				trim(pSMBIOSBIOSInfo->szBIOSReleaseDate);
			}
		}
	}
	return iTotalLen;
}

// Fills a SMB_SYSTEM_INFORMATION structure from the address pointed to by pbLinAddr
// and returns the size of all data read in bytes.

int GetSystemInformationStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_SYSTEM_INFORMATION *pSMBIOSSysInfo)
{
	UINT16	iTotalLen	= 0;
	int			iStringAreaLen	= 0;

	char szStringDesc[4][SMB_STRINGLEN + 1];
	UINT16 i = 0;
	if (LibGetSmbiosStructure(E_SYSTEM_INFORMATION, iNum, &pbLinAddr, &iTotalLen))
	{
		SMBIOS_STRUCTURE_POINTER  Smbios;
		Smbios.Raw = pbLinAddr;

		CopyMem(&pSMBIOSSysInfo->smb1, pbLinAddr, Smbios.Hdr->Length);

		// String Area.

		// Get the strings (MAX 4)
		i = 0;
		while (i < 4)
		{
			CHAR8 *pszCurString;
			Smbios.Raw    = pbLinAddr;
			pszCurString = LibGetSmbiosString(&Smbios,i + 1);
			if (pszCurString == NULL)
				break;

			AsciiStrnCpyS(szStringDesc[i], SMB_STRINGLEN + 1, pszCurString, SMB_STRINGLEN);
			iStringAreaLen += (int)AsciiStrLen(pszCurString)+1;
			i++;
		}

		// Double Null terminator at end of string area.
		iStringAreaLen++;

		if(pSMBIOSSysInfo->smb1.Manufacturer)
		{
			AsciiStrCpyS(pSMBIOSSysInfo->szManufacturer, sizeof(pSMBIOSSysInfo->szManufacturer), szStringDesc[pSMBIOSSysInfo->smb1.Manufacturer - 1]);
			trim(pSMBIOSSysInfo->szManufacturer);
		}
		else
		{
			pSMBIOSSysInfo->szManufacturer[0] = '\0';
		}
	
		if(pSMBIOSSysInfo->smb1.ProductName)
		{
			AsciiStrCpyS(pSMBIOSSysInfo->szProductName, sizeof(pSMBIOSSysInfo->szProductName), szStringDesc[pSMBIOSSysInfo->smb1.ProductName - 1]);
			trim(pSMBIOSSysInfo->szProductName);
		}
		else
		{
			pSMBIOSSysInfo->szProductName[0] = '\0';
		}

		if(pSMBIOSSysInfo->smb1.Version)
		{
			AsciiStrCpyS(pSMBIOSSysInfo->szVersion, sizeof(pSMBIOSSysInfo->szVersion), szStringDesc[pSMBIOSSysInfo->smb1.Version - 1]);
			trim(pSMBIOSSysInfo->szVersion);
		}
		else
		{
			pSMBIOSSysInfo->szVersion[0] = '\0';
		}

		if(pSMBIOSSysInfo->smb1.SerialNumber)
		{
			AsciiStrCpyS(pSMBIOSSysInfo->szSerialNumber, sizeof(pSMBIOSSysInfo->szSerialNumber), szStringDesc[pSMBIOSSysInfo->smb1.SerialNumber - 1]);
			trim(pSMBIOSSysInfo->szSerialNumber);
		}
		else
		{
			pSMBIOSSysInfo->szSerialNumber[0] = '\0';
		}
	}
	return iTotalLen;	
}

int GetBaseboardInformationStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_BASEBOARD_INFORMATION *pSMBIOSBaseBoardInfo)
{
	UINT16	iTotalLen	= 0;
	int			iStringAreaLen	= 0;
	UINT16		i				= 0;

	char szStringDesc[6][SMB_STRINGLEN + 1];

	if (LibGetSmbiosStructure(E_BASEBOARD_INFORMATION, iNum, &pbLinAddr, &iTotalLen))
	{
		SMBIOS_STRUCTURE_POINTER  Smbios;
		Smbios.Raw = pbLinAddr;

		AsciiFPrint(DEBUG_FILE_HANDLE, "SMBIOS: Found SMBIOS BaseboardInformation (pbLinAddr=0x%p, FormattedLen=%d, iTotalLen=%d)", pbLinAddr, Smbios.Hdr->Length, iTotalLen);

		CopyMem(&pSMBIOSBaseBoardInfo->smb2, pbLinAddr, Smbios.Hdr->Length);

		// String Area.

		// Get the strings (MAX 6)
		i = 0;
		while (i < 6)
		{
			CHAR8 *pszCurString;
			Smbios.Raw    = pbLinAddr;
			pszCurString = LibGetSmbiosString(&Smbios,i + 1);
			if (pszCurString == NULL)
				break;

			AsciiStrnCpyS(szStringDesc[i], SMB_STRINGLEN + 1, pszCurString, SMB_STRINGLEN);
			iStringAreaLen += (int)AsciiStrLen(pszCurString)+1;
			i++;
		}
	
		// Double Null terminator at end of string area.
		iStringAreaLen++;

		if(pSMBIOSBaseBoardInfo->smb2.Manufacturer)
		{
			AsciiStrCpyS(pSMBIOSBaseBoardInfo->szManufacturer, sizeof(pSMBIOSBaseBoardInfo->szManufacturer),
				   szStringDesc[pSMBIOSBaseBoardInfo->smb2.Manufacturer - 1]);
			trim(pSMBIOSBaseBoardInfo->szManufacturer);
		}
		else
		{
			pSMBIOSBaseBoardInfo->szManufacturer[0] = '\0';
		}
	
		if(pSMBIOSBaseBoardInfo->smb2.ProductName)
		{
			AsciiStrCpyS(pSMBIOSBaseBoardInfo->szProductName, sizeof(pSMBIOSBaseBoardInfo->szProductName),
				   szStringDesc[pSMBIOSBaseBoardInfo->smb2.ProductName - 1]);
			trim(pSMBIOSBaseBoardInfo->szProductName);
		}
		else
		{
			pSMBIOSBaseBoardInfo->szProductName[0] = '\0';
		}

		if(pSMBIOSBaseBoardInfo->smb2.Version)
		{
			AsciiStrCpyS(pSMBIOSBaseBoardInfo->szVersion, sizeof(pSMBIOSBaseBoardInfo->szVersion),
				   szStringDesc[pSMBIOSBaseBoardInfo->smb2.Version - 1]);
			trim(pSMBIOSBaseBoardInfo->szVersion);
		}
		else
		{
			pSMBIOSBaseBoardInfo->szVersion[0] = '\0';
		}

		if(pSMBIOSBaseBoardInfo->smb2.SerialNumber)
		{
			AsciiStrCpyS(pSMBIOSBaseBoardInfo->szSerialNumber, sizeof(pSMBIOSBaseBoardInfo->szSerialNumber),
				   szStringDesc[pSMBIOSBaseBoardInfo->smb2.SerialNumber - 1]);
			trim(pSMBIOSBaseBoardInfo->szSerialNumber);
		}
		else
		{
			pSMBIOSBaseBoardInfo->szSerialNumber[0] = '\0';
		}

		if(pSMBIOSBaseBoardInfo->smb2.AssetTag)
		{
			AsciiStrCpyS(pSMBIOSBaseBoardInfo->szAssetTag, sizeof(pSMBIOSBaseBoardInfo->szAssetTag),
				   szStringDesc[pSMBIOSBaseBoardInfo->smb2.AssetTag - 1]);
			trim(pSMBIOSBaseBoardInfo->szAssetTag);
		}
		else
		{
			pSMBIOSBaseBoardInfo->szAssetTag[0] = '\0';
		}

		if(pSMBIOSBaseBoardInfo->smb2.LocationInChassis)
		{
			AsciiStrCpyS(pSMBIOSBaseBoardInfo->szLocationInChassis, sizeof(pSMBIOSBaseBoardInfo->szLocationInChassis),
				   szStringDesc[pSMBIOSBaseBoardInfo->smb2.LocationInChassis - 1]);
			trim(pSMBIOSBaseBoardInfo->szLocationInChassis);
		}
		else
		{
			pSMBIOSBaseBoardInfo->szLocationInChassis[0] = '\0';
		}
	}

	return iTotalLen;
}

// Fills a SMB_MEMORY_DEVICE structure from the address pointed to by 
// pbLinAddr and returns the size of all data read in bytes.

int GetMemoryDeviceStruct(UINT8 * pbLinAddr, UINT16 iNum, SMBIOS_MEMORY_DEVICE *pSMBIOSMemDevice)
{
	int		iStringAreaLen	= 0;
	UINT16	iTotalLen		= 0;
	UINT16	i				= 0;
#define SMBIOS_MAX_MEM_STRINGS 7
	CHAR8	szStringDesc[SMBIOS_MAX_MEM_STRINGS][SMB_STRINGLEN+1];
	int		iIndex = 0;

	AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct %d", iNum);

	if (LibGetSmbiosStructure(E_MEMORY_DEVICE, iNum, &pbLinAddr, &iTotalLen))
	{
		SMBIOS_STRUCTURE_POINTER  Smbios;
		Smbios.Raw = pbLinAddr;

		CopyMem(pSMBIOSMemDevice, pbLinAddr, Smbios.Hdr->Length);

		AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct Get strings (FormattedLen=%d, iTotalLen=%x)", pSMBIOSMemDevice->smb17.Hdr.Length, iTotalLen);

		i = 0;
		while (i < SMBIOS_MAX_MEM_STRINGS)
		{
			CHAR8 *pszCurString;
			Smbios.Raw    = pbLinAddr;
			pszCurString = LibGetSmbiosString(&Smbios,i + 1);
			if (pszCurString == NULL)
				break;

			AsciiStrnCpyS(szStringDesc[i], SMB_STRINGLEN + 1, pszCurString, SMB_STRINGLEN);
			iStringAreaLen += (int)AsciiStrLen(pszCurString)+1;
			i++;
		}

		// Double Null terminator at end of string area.
		iStringAreaLen++;

		AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: String area len=%d", iStringAreaLen);

		if(pSMBIOSMemDevice->smb17.DeviceLocator)
		{	
			iIndex = pSMBIOSMemDevice->smb17.DeviceLocator - 1;
			if (iIndex >= 0 && iIndex < SMBIOS_MAX_MEM_STRINGS)
			{
				AsciiStrCpyS(pSMBIOSMemDevice->szDeviceLocator, sizeof(pSMBIOSMemDevice->szDeviceLocator), szStringDesc[iIndex]);
				trim(pSMBIOSMemDevice->szDeviceLocator);
			}
			else
				AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: invalid index for device locator (%d)", iIndex);
		}

		if(pSMBIOSMemDevice->smb17.BankLocator)
		{
			iIndex = pSMBIOSMemDevice->smb17.BankLocator - 1;
			if (iIndex >= 0 && iIndex < SMBIOS_MAX_MEM_STRINGS)
			{
				AsciiStrCpyS(pSMBIOSMemDevice->szBankLocator, sizeof(pSMBIOSMemDevice->szBankLocator),szStringDesc[iIndex]);
				trim(pSMBIOSMemDevice->szBankLocator);
			}
			else
				AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: invalid index for bank locator(%d)", iIndex);
		}

		// Version 2.3+ only
		AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: parsing v2.3+ strings");

		if (pSMBIOSMemDevice->smb17.Manufacturer)
		{
			iIndex = pSMBIOSMemDevice->smb17.Manufacturer - 1;
			if (iIndex >= 0 && iIndex < SMBIOS_MAX_MEM_STRINGS)
			{
				AsciiStrCpyS(pSMBIOSMemDevice->szManufacturer, sizeof(pSMBIOSMemDevice->szManufacturer), szStringDesc[iIndex]);
				trim(pSMBIOSMemDevice->szManufacturer);
			}
			else
				AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: invalid index for manufacturer (%d)", iIndex);
		}
		if (pSMBIOSMemDevice->smb17.SerialNumber)
		{
			iIndex = pSMBIOSMemDevice->smb17.SerialNumber - 1;
			if (iIndex >= 0 && iIndex < SMBIOS_MAX_MEM_STRINGS)
			{
				AsciiStrCpyS(pSMBIOSMemDevice->szSerialNumber, sizeof(pSMBIOSMemDevice->szSerialNumber), szStringDesc[iIndex]);
				trim(pSMBIOSMemDevice->szSerialNumber);
			}
			else
				AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: invalid index for serial number (%d)", iIndex);
		}
		if (pSMBIOSMemDevice->smb17.AssetTag) 
		{
			iIndex = pSMBIOSMemDevice->smb17.AssetTag - 1;
			if (iIndex >= 0 && iIndex < SMBIOS_MAX_MEM_STRINGS)
			{
				AsciiStrCpyS(pSMBIOSMemDevice->szAssetTag, sizeof(pSMBIOSMemDevice->szAssetTag), szStringDesc[iIndex]);
				trim(pSMBIOSMemDevice->szAssetTag);
			}
			else
				AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: invalid index for asset tag (%d)", iIndex);
		}
		if (pSMBIOSMemDevice->smb17.PartNumber)
		{
			iIndex = pSMBIOSMemDevice->smb17.PartNumber - 1;
			if (iIndex >= 0 && iIndex < SMBIOS_MAX_MEM_STRINGS)
			{
				AsciiStrCpyS(pSMBIOSMemDevice->szPartNumber, sizeof(pSMBIOSMemDevice->szPartNumber), szStringDesc[iIndex]);
				trim(pSMBIOSMemDevice->szPartNumber);
			}
			else
				AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: invalid index for part number (%d)", iIndex);
		}

		AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: parsing v3.2+ strings");

		if (pSMBIOSMemDevice->smb17.FirmwareVersion)
		{
			iIndex = pSMBIOSMemDevice->smb17.FirmwareVersion - 1;
			if (iIndex >= 0 && iIndex < SMBIOS_MAX_MEM_STRINGS)
			{
				AsciiStrCpyS(pSMBIOSMemDevice->szFirmwareVersion, sizeof(pSMBIOSMemDevice->szFirmwareVersion), szStringDesc[iIndex]);
				trim(pSMBIOSMemDevice->szFirmwareVersion);
			}
			else
				AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct: invalid index for firmware version (%d)", iIndex);

		}

		AsciiFPrint(DEBUG_FILE_HANDLE, "DEBUG: Get SMB mem device Struct end");

	}

	return iTotalLen;
}

