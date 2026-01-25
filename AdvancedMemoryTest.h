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
//	AdvancedMemoryTest.h
//
//Author(s):
//	Keith Mah
//
//Description:
//  This class is responsible for initialising and running a test which measures 
//  various memory access speeds.
//
//  Taken from PerformanceTest
//
//History
//	15 Jul 2014: Initial version

#ifndef _ADVANCED_MEMORY_TEST_H_
#define _ADVANCED_MEMORY_TEST_H_

#define MIN_BLOCK_SIZE				1024

#define MEM_WRITE					0
#define MEM_READ					1

// Memory test mode
enum E_MEM_TEST_MODE
{
	MEM_STEP		= 0,
	MEM_BLOCK		= 1,
};

#define	BYTE_BITS					8
#define WORD_BITS					16
#define DWORD_BITS					32
#define QWORD_BITS					64

enum E_TEST_RESULT
{
	NOT_STARTED		= 0,
	RUNNING			= 1,
	COMPLETE		= 2,
	END_OF_DISK		= 3,
	USER_STOPPED	= 4,
	ERR_TEST		= 101,
};

typedef struct {

	UINT64			dKBPerSecond;
	UINTN			StepSize;

} MEMORY_STATS, *LPMEMORY_STATS;

void Init();
void CleanupTest();
int	NewStepTest();
int	NewBlockTest();
BOOLEAN Step();
BOOLEAN StepBlock();
BOOLEAN UpdateStats(UINT64 flKBPerSecond, UINTN StepSize);

void SetDataSize(int nVal);

void SetReadWrite(BOOLEAN bVal);

BOOLEAN GetReadWrite();

void SetTestMode(int nVal);

int GetTestMode();

UINTN GetCurrentStepSize();

UINTN GetBlockSize();

int	GetDataSize();

UINTN GetArraySize();

UINTN GetBlockSize();

UINTN GetMaxBlockSize();

void SetAveSpeed(UINT64 fVal);

UINT64 GetAveSpeed();

UINT64 GetTotalKBPerSec();

void SetTestState(int nVal);

int GetTestState();

void SetResultsAvailable(BOOLEAN bVal);

UINTN GetNumSamples();

BOOLEAN GetSample(UINTN index, MEMORY_STATS *pSample);
#endif

