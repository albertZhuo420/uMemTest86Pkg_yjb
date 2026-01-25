// PassMark MemTest86
//
// Copyright (c) 2016
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
// Program:
//	MemTest86
//
// Module:
//	cputemp.c
//
// Author(s):
//	Keith Mah, Ian Robinson
//
// Description:
//	Functions to read the current CPU temperature
//
//   Based on source code from SysInfo

#include <Library/BaseLib.h>
#include <Library/Cpuid.h>
#include "cputemp.h"
#include "cpuinfo.h"
#include <cpuinfoMSRIntel.h>
#include <cpuinfoMSRAMD.h>

EFI_STATUS GetCPUTemp(CPUINFO *pCPUInfo, UINTN ProcNum, MSR_TEMP *CPUMSRTemp)
{
	if (StrCmp(L"GenuineIntel", pCPUInfo->manufacture) == 0)
		return GetIntelCPUTemps(pCPUInfo, ProcNum, CPUMSRTemp);
	else if (StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0 || StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0)
		return GetAMDCPUTemps(pCPUInfo, ProcNum, CPUMSRTemp);

	return EFI_UNSUPPORTED;
}
