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
//	cpuinfoMSRAMD.h
//
// Author(s):
//	Keith Mah, Ian Robinson
//
// Description:
//	Detects and gathers information about AMD CPU(s).
//	Including speed, manufacturer, capabilities, available L2 cache, etc..
//
//   Based on source code from SysInfo
#ifndef _CPUINFOMSRAMD_H_
#define _CPUINFOMSRAMD_H_

struct cpu_ident;
typedef struct _CPUINFO CPUINFO;
typedef struct _CPUMSRINFO CPUMSRINFO;
typedef struct _MSR_TEMP MSR_TEMP;

void GetAMDCacheInfo(CPUINFO *pCPUInfo);

void GetAMDFamilyMSRInfo(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

BOOLEAN GetAMD10Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD11Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD12Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD14Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD15Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD16Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD17Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetHygon18Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD19Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD1ATemp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN IsAMDFamily10h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily11h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily12h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily14h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily15h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily16h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily17h(CPUINFO *pCPUInfo);
BOOLEAN IsHygonFamily18h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily19h(CPUINFO *pCPUInfo);
BOOLEAN IsAMDFamily1Ah(CPUINFO *pCPUInfo);

EFI_STATUS GetAMDCPUTemps(CPUINFO *pCPUInfo, UINTN ProcNum, MSR_TEMP *CPUMSRTemp);

#endif
