// PassMark MemTest86
//
// Copyright (c) 2013-2016
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
//	cputemp.h
//
// Author(s):
//	Keith Mah, Ian Robinson
//
// Description:
//	Functions to read the current CPU temperature
//
//   Based on source code from SysInfo

//
// History
//	27 Feb 2013: Initial version
#ifndef _CPUTEMP_H_
#define _CPUTEMP_H_

#define MIN_TEMP_SANITY 0

typedef struct _CPUINFO CPUINFO;

/// The raw temperature infomration from the CPU registers
typedef struct _MSR_TEMP
{
	BOOLEAN bDataCollected;
	UINT64 ullMSR0x19C; // Raw register
	int iPROCHOT1;
	int iPROCHOT2;
	int iResolutionInDegreesCelcius;
	int iReadingValid;
	int iDigitalReadout; // CPU temp value (yet to be adjusted, e.g. for tJunction)
	int iTemperature;	 // Calculated actual temperature
} MSR_TEMP;

EFI_STATUS GetCPUTemp(CPUINFO *pCPUInfo, UINTN ProcNum, MSR_TEMP *CPUMSRTemp);

#endif
