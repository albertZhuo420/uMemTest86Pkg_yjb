//PassMark MemTest86
//
//Copyright (c) 2014-2016
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
//	TestResultsStorage.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Routine declarations for storing and retrieving test results from disk storage
//
//
//History
//	07 Aug 2014: Initial version

#ifndef _TEST_RESULTS_STORAGE_H
#define _TEST_RESULTS_STORAGE_H

#include <Uefi.h>
#include <YAMLLib/yaml.h>

typedef struct 
{
	int			iTestType;
	INT64		i64TestStartTime;

	CHAR16		szComputerName[256];
	UINT32		uiNumCPU;	//Number of CPUs
	CHAR16		szCPUType[64];
	CHAR16		szCPUSpeed[32];
} TestResultsMetaInfo;

typedef struct
{
	UINT16			m_Major;
	UINT16			m_Minor;
	UINT16			m_Build;
	CHAR16			m_SpecialBuildName[32];
	enum { 
		x86 = 32, 
		x64 = 64 
	} m_ptArchitecture;
} MTVersion;

typedef struct tagMEM_RESULTS {

	// Common data
	UINT64		TotalPhys;
	int			iTestMode;		// MEM_STEP or MEM_BLOCK
	UINT64		fAveSpeed;		// MB per Sec
	UINT64		fAveCPULoad;

	CHAR16		szModuleManuf[64];
	CHAR16		szModulePartNo[32];
	CHAR16		szRAMType[32];
	CHAR16		szRAMTiming[32];
	//CHAR16		szRAMSummary[128];

	// Read/write
	BOOLEAN		bRead;
	// Step size data
	UINT64		ArraySize;
	// Block size data
	UINT64		MinBlock;
	UINT64		MaxBlock;
	int			dwAccessDataSize;

} MEM_RESULTS, *LPMEM_RESULTS;

// Parse the RAM benchmark results from file
BOOLEAN LoadBenchResults(MT_HANDLE FileHandle, TestResultsMetaInfo *TestInfo, MTVersion *VersionInfo, MEM_RESULTS *Header, UINT64 SampleX[], UINT64 SampleY[], int *NumSamples);

// Save the RAM benchmark results to file
BOOLEAN SaveBenchResults(MT_HANDLE FileHandle, TestResultsMetaInfo *TestInfo, MEM_RESULTS *Header, UINT64 SampleX[], UINT64 SampleY[], int NumSamples);

BOOLEAN TimestampToEFITime(UINT64 Timestamp, EFI_TIME *EFITime);

#endif