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
//	imsdefect.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Glue code for iMS interface
//
//
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include "imsdefect.h"

extern
VOID *
EFIAPI
GetFirstGuidHob(
	IN CONST EFI_GUID         *Guid
);

EFI_GUID gIMSDefectListNvarGuid = IMS_DEFECT_LIST_NVAR_GUID;

EFI_GUID gIMSVarGuid = IMS_VAR_GUID;

EFI_GUID gIMSToBiosHobGuid = IMS_TO_BIOS_HOB_GUID;

EFI_GUID gIMSAddressTranslationDxeProtocolGuid = IMS_ADDRESS_TRANSLATION_PROTOCOL_GUID;


static BOOLEAN sIMSInit;
static BOOLEAN sIMSAvailable;
static BOOLEAN sIMSRunFromBIOS;
static UINT32 sIMSDefectListNvarAttr = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;
static IMS_NVMEM_DEFECT_LIST sIMSDefectList;
static IMS_TO_BIOS_HOB_ST sIMSToBiosHob;

VOID
iMSInit()
{
	EFI_STATUS	Status;
	UINT8		Data = 0;
	UINTN		Size;
	UINTN		dsz = sizeof(IMS_NVMEM_DEFECT_LIST);

	if (sIMSInit != FALSE)
		return;

	sIMSAvailable = FALSE;
	sIMSRunFromBIOS = FALSE;
	sIMSInit = TRUE;
	if (gRT->GetVariable(IMS_DEFECT_LIST_NVAR_NAME, &gIMSDefectListNvarGuid, &sIMSDefectListNvarAttr, &dsz, (VOID*)&sIMSDefectList) != EFI_SUCCESS)
		return;

	UINT8 *FirstHob = GetFirstGuidHob(&gIMSToBiosHobGuid);
	if (FirstHob == NULL)
		return;

	CopyMem(&sIMSToBiosHob, FirstHob + 0x18, sizeof(sIMSToBiosHob));


	Size = sizeof(UINT8);
	Status = gRT->GetVariable(
		IMS_VAR_NAME,
		&gIMSVarGuid,
		NULL,
		&Size,
		&Data
	);
	if (Status == EFI_SUCCESS) {
		if (Data == 0x55) {
			//it's run from bios, then clear the flag.
			Data = 0;
			Status = gRT->SetVariable(
				IMS_VAR_NAME,
				&gIMSVarGuid,
				EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
				Size,
				&Data
			);
			sIMSRunFromBIOS = TRUE;
		}
	}

	sIMSAvailable = TRUE;
}

BOOLEAN
EFIAPI
iMSIsAvailable()
{
	iMSInit();

	return sIMSAvailable;
}

unsigned long
mktime(const unsigned int year0, const unsigned int mon0,
	const unsigned int day, const unsigned int hour,
	const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	/* Must starts from year 2000 */
	if (year < 2000) year = 2000;

	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int)(mon -= 2)) {
		mon += 12;  /* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ((((unsigned long)
		(year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) +
		year * 365 - 719499//730456
		) * 24 + hour /* now have hours */
		) * 60 + min /* now have minutes */
		) * 60 + sec; /* finally seconds */
}

INT16 Crc16(char *ptr, int count)
{
	INT16 crc, i;
	crc = 0;
	while (--count >= 0)
	{
		crc = crc ^ (INT16)(int)*ptr++ << 8;
		for (i = 0; i < 8; ++i)
		{
			if (crc & 0x8000)
			{
				crc = crc << 1 ^ 0x1021;
			}
			else
			{
				crc = crc << 1;
			}
		}
	}
	return (crc & 0xFFFF);
}

void calCrc(IMS_NVMEM_DEFECT_LIST *imsvar)
{
	INT16 crc;

	imsvar->Crc16 = 0;
	crc = Crc16((char *)imsvar + 4, sizeof(IMS_NVMEM_DEFECT_LIST) - 4);
	imsvar->Crc16 = crc;
}

EFI_STATUS
EFIAPI
iMSSetDefectList(IMS_NVMEM_DEFECT_LIST *DefectList)
{
	EFI_TIME		Time;

	iMSInit();

	if (sIMSAvailable == FALSE)
		return EFI_UNSUPPORTED;

	CopyMem(&sIMSDefectList, DefectList, sizeof(sIMSDefectList));
	
	gRT->GetTime(&Time, NULL);
	sIMSDefectList.MemDefectList.UpdateDateTime = mktime(Time.Year, Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second);
	calCrc(&sIMSDefectList);

	return gRT->SetVariable(IMS_DEFECT_LIST_NVAR_NAME,
		&gIMSDefectListNvarGuid,
		sIMSDefectListNvarAttr,
		sizeof(IMS_NVMEM_DEFECT_LIST),
		(VOID*)&sIMSDefectList);
}

EFI_STATUS
EFIAPI
iMSGetDefectList(IMS_NVMEM_DEFECT_LIST *DefectList)
{
	iMSInit();

	if (sIMSAvailable == FALSE)
		return EFI_UNSUPPORTED;

	CopyMem(DefectList, &sIMSDefectList, sizeof(*DefectList));

	return EFI_SUCCESS;
}


EFI_STATUS iMSSystemAddressToDimmAddress(
	EFI_PHYSICAL_ADDRESS PhysAddr,
	UINT8 *SocketId,
	UINT8 *MemoryControllerId,
	UINT8 *ChannelId,
	UINT8 *DimmSlot,
	UINT8 *DimmRank)
{
	EFI_STATUS Status;
	IMS_ADDRESS_TRANSLATION_PROTOCOL *ImsAddrTransProtocol = NULL;

	iMSInit();

	if (sIMSAvailable == FALSE)
		return EFI_UNSUPPORTED;

	if (sIMSToBiosHob.iMSFreeVersion != 0)
		return EFI_INCOMPATIBLE_VERSION;

	Status = gBS->LocateProtocol(
		&gIMSAddressTranslationDxeProtocolGuid,
		NULL,
		(void **)&ImsAddrTransProtocol);
	if (EFI_ERROR(Status))
		return Status;

	Status = ImsAddrTransProtocol->SystemAddressToDimmAddress(
		PhysAddr,
		SocketId,
		MemoryControllerId,
		ChannelId,
		DimmSlot,
		DimmRank
	);

	return Status;
}

EFI_STATUS
EFIAPI
iMSGetVersionFromHob(IMS_TO_BIOS_HOB_ST *iMSToBiosHob)
{
	iMSInit();

	if (sIMSAvailable == FALSE)
		return EFI_UNSUPPORTED;

	CopyMem(iMSToBiosHob, &sIMSToBiosHob, sizeof(*iMSToBiosHob));
	return EFI_SUCCESS;
}

//check if run from bios.
BOOLEAN
EFIAPI
iMSIsRunFromBios()
{
	iMSInit();

	return sIMSRunFromBIOS;
}