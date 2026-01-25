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
//	MemTestSupportLib.h
//
// Author(s):
//	Keith Mah
//
// Description:
//	Support functions for MemTest86
//
// References:
//   https://github.com/jljusten/MemTestPkg
//

/** @file
  Memory Test support library class interface

  Copyright (c) 2006 - 2009, Intel Corporation
  Copyright (c) 2009, Jordan Justen
  All rights reserved.

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.  The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS"
  BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER
  EXPRESS OR IMPLIED.

**/

#ifndef _MEM_TEST_SUPPORT_LIB_H_INCLUDED_
#define _MEM_TEST_SUPPORT_LIB_H_INCLUDED_

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
#include <xmmintrin.h> //SSE intrinsics
#include <emmintrin.h> //SSE2 intrinsics
#elif defined(MDE_CPU_AARCH64)
#include <sse2neon.h>
#endif

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Protocol/SimpleFileSystem.h>

#include <uMemTest86.h>

extern CHAR16 g_wszBuffer[BUF_SIZE];

#define EFI_TEST_FAILED 0x80001000

typedef void *MT_HANDLE;

// MtSupportInstallMemoryRangeTest
//
// Install simple (1 stage) memory test
EFI_STATUS
EFIAPI
MtSupportInstallMemoryRangeTest(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN TEST_MEM_RANGE TestRangeFunction,
    IN UINT32 MaxCPUs,
    IN UINT64 NumberOfIters,
    IN UINTN Align,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser,
    IN CACHEMODE CacheMode,
    IN VOID *Context);

// MtSupportInstallBitFadeTest
//
// Install bit fade memory test (includes 2 stages)
EFI_STATUS
EFIAPI
MtSupportInstallBitFadeTest(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN TEST_MEM_RANGE TestRangeFunction1,
    IN TEST_MEM_RANGE TestRangeFunction2,
    IN UINT32 MaxCPUs,
    IN UINT64 NumberOfIters,
    IN UINTN Align,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser,
    IN CACHEMODE CacheMode,
    IN VOID *Context);

enum
{
    HAMMER_SINGLE = 0,
    HAMMER_DOUBLE
};

// MtSupportInstallHammerTest
//
// Install row hammer memory test (includes multiple stages)
EFI_STATUS
EFIAPI
MtSupportInstallHammerTest(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN TEST_MEM_RANGE TestRangeFunction1,
    IN TEST_MEM_RANGE TestRangeFunction2,
    IN TEST_MEM_RANGE TestRangeFunction3,
    IN TEST_MEM_RANGE TestRangeFunction4,
    IN UINT32 MaxCPUs,
    IN UINT64 NumberOfIters,
    IN UINTN Align,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser,
    IN CACHEMODE CacheMode,
    IN VOID *Context);

// MtSupportInstallHammerTest_multithreaded
//
// Install row hammer memory test for simultaneous hammering on same memory range (includes multiple stages)
EFI_STATUS
EFIAPI
MtSupportInstallHammerTest_multithreaded(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN TEST_MEM_RANGE TestRangeFunction1,
    IN TEST_MEM_RANGE TestRangeFunction2,
    IN TEST_MEM_RANGE TestRangeFunction3,
    IN TEST_MEM_RANGE TestRangeFunction4,
    IN UINT32 MaxCPUs,
    IN UINT64 NumberOfIters,
    IN UINTN Align,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser,
    IN CACHEMODE CacheMode,
    IN VOID *Context);

typedef EFI_STATUS(EFIAPI *RUN_MEM_TEST)(
    IN VOID *Context);

// MtSupportInstallMemoryTest
//
// Install generic memory test
EFI_STATUS
EFIAPI
MtSupportInstallMemoryTest(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN CACHEMODE CacheMode,
    IN RUN_MEM_TEST MemTestFunction,
    IN VOID *Context);

// MtSupportNumTests
//
// Return number of tests installed
UINTN
EFIAPI
MtSupportNumTests();

// MtSupportNumPassed
//
// Return number of tests passed
UINTN
EFIAPI
MtSupportNumPassed();

// MtSupportTestsCompleted
//
// Return number of tests completed
UINTN
EFIAPI
MtSupportTestsCompleted();

enum
{
    RESULT_PASS,
    RESULT_INCOMPLETE_PASS,
    RESULT_FAIL = -1,
    RESULT_INCOMPLETE_FAIL = -2,
    RESULT_UNKNOWN = 2
};

// MtSupportGetTestResult
//
// Return the result of the last executed test
int
    EFIAPI
    MtSupportGetTestResult();

// MtSupportGetDimmTestResult
//
// Return the result of the specified DIMM of the last executed test
int
    EFIAPI
    MtSupportGetDimmTestResult(int dimm);

// MtSupportGetTestResultStr
//
// Return the result of the last executed test as a string
CHAR16 *
    EFIAPI
    MtSupportGetTestResultStr(
        int Result,
        int LangId,
        CHAR16 *Str,
        UINTN StrSize);

// MtSupportCurTest
//
// Return currently running test index
UINTN
EFIAPI
MtSupportCurTest();

// MtSupportCurPass
//
// Return current pass number
UINTN
EFIAPI
MtSupportCurPass();

enum
{
    CPU_DEFAULT = -1,
    CPU_SINGLE = 0,
    CPU_PARALLEL,
    CPU_RROBIN,
    CPU_SEQUENTIAL
};

// MtSupportCPUSelMode
//
// Return the current CPU selection mode
UINTN
EFIAPI
MtSupportCPUSelMode();

// MtSupportSetCPURunning
//
// Mark the specified processor as "running" (for displaying the CPU state)
VOID
    EFIAPI
    MtSupportSetCPURunning(
        UINTN ProcNum,
        BOOLEAN Running);

// MtSupportSetAllCPURunning
//
// Mark all processors as "running" (for displaying the CPU state)
VOID
    EFIAPI
    MtSupportSetAllCPURunning(BOOLEAN Running);

// MtSupportCPURunning
//
// Return TRUE if specified processor is running
BOOLEAN
EFIAPI
MtSupportCPURunning(UINTN ProcNum);

// MtSupportUninstallAllTests
//
// Uninstall all memory tests
EFI_STATUS
EFIAPI
MtSupportUninstallAllTests();

// MtSupportRunAllTests
//
// Execute all installed tests
EFI_STATUS
EFIAPI
MtSupportRunAllTests();

// MtSupportTestTick
//
// Update the test screen UI (eg. timer, CPU state, etc.)
VOID MtSupportTestTick(BOOLEAN UpdateErr);

// MtSupportPollECC
//
// Poll ECC registers for errors
VOID MtSupportPollECC();

// MtSupportGetCPUTemp
//
// Get the last CPU temperature reading
int MtSupportGetCPUTemp();

// MtSupportGetRAMTemp
//
// Get the last RAM temperature reading of the specified DIMM
int MtSupportGetRAMTemp(int dimm);

// MtSupportCheckTestAbort
//
// Return TRUE if user has aborted the tests
BOOLEAN
MtSupportCheckTestAbort();

// MtSupportCheckTestAbort
//
// End the test prematurely
VOID
    EFIAPI
        MtSupportAbortTesting(
            VOID);

// MtSupportSkipCurrentTest
//
// Skip the current test
VOID
    EFIAPI
        MtSupportSkipCurrentTest(
            VOID);

// MtSupportSetErrorsAsWarnings
//
// If TRUE, record all errors as warnings. Warnings are not reported directly in the UI but recorded for later use
VOID
    EFIAPI
    MtSupportSetErrorsAsWarnings(BOOLEAN ErrorsAsWarnings);

// MtSupportClearECCErrors
//
// Clear any ECC errors pending in hardware
VOID
    EFIAPI
    MtSupportClearECCErrors();

// MtSupportReportError
//
// Report the detected error + corresponding details
VOID
    EFIAPI
    MtSupportReportError(
        IN UINTN Address,
        IN UINT32 TestNo,
        IN UINTN Actual,
        IN UINTN Expected,
        IN int Type);

// MtSupportReportErrorVarSize
//
// Report the detected error + corresponding details
VOID
    EFIAPI
    MtSupportReportErrorVarSize(
        IN UINTN Address,
        IN UINT32 TestNo,
        IN void *Actual,
        IN void *Expected,
        IN UINT32 DataSize,
        IN int Type);

static __inline VOID
    EFIAPI
    MtSupportReportError32(
        IN UINTN Address,
        IN UINT32 TestNo,
        IN UINT32 Actual,
        IN UINT32 Expected,
        IN int Type)
{
    MtSupportReportErrorVarSize(Address, TestNo, &Actual, &Expected, sizeof(UINT32), Type);
}

static __inline VOID
    EFIAPI
    MtSupportReportError64(
        IN UINTN Address,
        IN UINT32 TestNo,
        IN UINT64 Actual,
        IN UINT64 Expected,
        IN int Type)
{
    MtSupportReportErrorVarSize(Address, TestNo, &Actual, &Expected, sizeof(UINT64), Type);
}

enum
{
    ECC_TYPE_READWRITE,
    ECC_TYPE_SCRUB,
    ECC_TYPE_INBAND
};

// MtSupportReportECCError
//
// Report the detected ECC error + corresponding details
VOID
    EFIAPI
    MtSupportReportECCError(
        IN UINTN Address,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm);

// MtSupportReportECCErrorType
//
// Report the ECC error type + corresponding details
VOID
    EFIAPI
    MtSupportReportECCErrorType(
        IN UINTN Address,
        IN UINT8 ECCType,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm);

// MtSupportReportIBECCError
//
// Report the detected IBECC error + corresponding details
VOID
    EFIAPI
    MtSupportReportIBECCError(
        IN UINTN Address,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm);

// MtSupportReportECCError_dimm
//
// Report the detected ECC error + corresponding DIMM details (eg. bank/rank/col/row address)
VOID
    EFIAPI
    MtSupportReportECCError_dimm(
        IN INT32 Col,
        IN INT32 Row,
        IN INT8 Bank,
        IN INT8 Rank,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm);

// MtSupportReportECCErrorType
//
// Report the ECC error type + corresponding DIMM details (eg. bank/rank/col/row address)
VOID
    EFIAPI
    MtSupportReportECCErrorType_dimm(
        IN INT32 Col,
        IN INT32 Row,
        IN INT8 Bank,
        IN INT8 Rank,
        IN UINT8 ECCType,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm);

// MtSupportReportIBECCError_dimm
//
// Report the detected IBECC error + corresponding DIMM details (eg. bank/rank/col/row address)
VOID
    EFIAPI
    MtSupportReportIBECCError_dimm(
        IN INT32 Col,
        IN INT32 Row,
        IN INT8 Bank,
        IN INT8 Rank,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm);

// MtSupportUpdateErrorDisp
//
// Update the UI with any new errors detected
BOOLEAN
EFIAPI
MtSupportUpdateErrorDisp();

// MtSupportUpdateWarningDisp
//
// Process any new warnings detected
VOID
    EFIAPI
    MtSupportUpdateWarningDisp();

// MtSupportReportCPUError
//
// Called when a CPU error is detected
VOID
    EFIAPI
    MtSupportReportCPUError(
        IN UINT32 TestNo,
        IN UINTN ProcNum);

// MtSupportGetNumErrors
//
// Get the number of errors in the last test run
UINT32
EFIAPI
MtSupportGetNumErrors();

// MtSupportGetNumWarns
//
// Get the number of warnings in the last test run
UINT32
EFIAPI
MtSupportGetNumWarns();

// MtSupportGetNumCorrECCErrors
//
// Get the number of corrected ECC errors in the last test run
UINT32
EFIAPI
MtSupportGetNumCorrECCErrors();

// MtSupportGetNumUncorrECCErrors
//
// Get the number of uncorrected ECC errors in the last test run
UINT32
EFIAPI
MtSupportGetNumUncorrECCErrors();

// MtSupportGetNumSlotErrors
//
// Get the number of errors for the given slot in the last test run
UINT32
EFIAPI
MtSupportGetNumSlotErrors(int slot);

// MtSupportGetNumUnknownSlotErrors
//
// Get the number of unknown slot errors for in the last test run
UINT32
EFIAPI
MtSupportGetNumUnknownSlotErrors();

// MtSupportGetNumSlotChipErrors
//
// Get the number of errors for the given slot and chip in the last test run
UINT32
EFIAPI
MtSupportGetNumSlotChipErrors(int slot, int chip);

// MtSupportReportErrorAddrIMS
//
// Report errors to iMS
EFI_STATUS
EFIAPI
MtSupportReportErrorAddrIMS();

VOID
    EFIAPI
    MtSupportPrintIMSDefectList(MT_HANDLE FileHandle);

// MtSupportOutputSysInfo
//
// Write the system information details to a text file
VOID
    EFIAPI
    MtSupportOutputSysInfo(MT_HANDLE FileHandle);

// MtSupportDisplayErrorSummary
//
// Display a summary of the testing results after completion
VOID
    EFIAPI
    MtSupportDisplayErrorSummary();

// MtSupportWriteReportHeader
//
// Write the header into an HTML report file
VOID
    EFIAPI
    MtSupportWriteReportHeader(MT_HANDLE FileHandle);

// MtSupportWriteReportSysInfo
//
// Write the system information into an HTML report file
VOID
    EFIAPI
    MtSupportWriteReportSysInfo(MT_HANDLE FileHandle);

// MtSupportWriteReportFooter
//
// Write the footer into an HTML report file
VOID
    EFIAPI
    MtSupportWriteReportFooter(MT_HANDLE FileHandle);

// MtSupportSaveHTMLCertificate
//
// Write the test results into an HTML report file
VOID
    EFIAPI
    MtSupportSaveHTMLCertificate(MT_HANDLE FileHandle);

// MtSupportSaveBinaryReport
//
// Write the test report into a binary file
EFI_STATUS
EFIAPI
MtSupportSaveBinaryReport(MT_HANDLE FileHandle);

EFI_STATUS
EFIAPI
MtSupportPMPConnect();

EFI_STATUS
EFIAPI
MtSupportPMPStatus();

EFI_STATUS
EFIAPI
MtSupportPMPTestResult();

UINT64
EFIAPI
MtSupportReadTSC();

// MtSupportGetTestStartTime
//
// Return the test start time
VOID
    EFIAPI
    MtSupportGetTestStartTime(EFI_TIME *StartTime);

// MtSupportGetTimeInMilliseconds
//
// Return the number of milliseconds elapsed since the test has started
UINT64
EFIAPI
MtSupportGetTimeInMilliseconds();

// efi_time
//
// Return the linux timestamp (in secs) given the EFI_TIME
UINT32
efi_time(EFI_TIME *ETime);

// MtSupportSetDefaultConfig
//
// Set to default configuration
VOID
    EFIAPI
    MtSupportSetDefaultConfig();

// MtSupportDebugWriteLine
//
// Write to debug file
EFI_STATUS
EFIAPI
MtSupportDebugWriteLine(
    char *Line);

// MtSupportUCDebugWriteLine
//
// Write to debug file
EFI_STATUS
EFIAPI
MtSupportUCDebugWriteLine(
    CHAR16 *Line);

// MtSupportUCDebugPrint
//
// Write to debug file
EFI_STATUS
EFIAPI
MtSupportUCDebugPrint(
    IN CONST CHAR16 *FormatString,
    ...);

// MtSupportFlushMPDebugLog
//
// Flush all debug entries written by APs to debug file
VOID
    EFIAPI
    MtSupportFlushMPDebugLog();

// MtSupportMPDebugPrint
//
// Function for APs to write to debug file (APs cannot write to debug file directly)
EFI_STATUS
EFIAPI
MtSupportMPDebugPrint(
    IN UINTN ProcNum,
    IN CONST CHAR8 *FormatString,
    ...);

#define DEBUG_FILE_HANDLE ((MT_HANDLE)(UINTN) - 1)
#define DBGLOG(ProcNum, ...)                         \
    if (ProcNum == MPSupportGetBspId())              \
    {                                                \
        AsciiFPrint(DEBUG_FILE_HANDLE, __VA_ARGS__); \
    }

// FPrint
//
// Write to file
EFI_STATUS
EFIAPI
FPrint(
    MT_HANDLE FileHandle,
    IN CONST CHAR16 *FormatString,
    ...);

// UnicodeCatf
//
// Write to file handle or string buffer
// BufferSize < 0 : FileHandleOrBuffer is MT_HANDLE
// BufferSize >= 0 : FileHandleOrBuffer is CHAR16*
EFI_STATUS
EFIAPI
UnicodeCatf(
    IN VOID *FileHandleOrBuffer,
    IN INTN BufferSize,
    IN CONST CHAR16 *FormatString,
    ...);

// AsciiFPrint
//
// Write to file
EFI_STATUS
EFIAPI
AsciiFPrint(
    MT_HANDLE FileHandle,
    IN CONST CHAR8 *FormatString,
    ...);

// AsciiCatf
//
// Write to file handle or string buffer
// BufferSize < 0 : FileHandleOrBuffer is MT_HANDLE
// BufferSize >= 0 : FileHandleOrBuffer is CHAR*
EFI_STATUS
EFIAPI
AsciiCatf(
    IN VOID *FileHandleOrBuffer,
    IN INTN BufferSize,
    IN CONST CHAR8 *FormatString,
    ...);

// MtSupportGetReportPrefix
//
// Get filename prefix for saving report files
CHAR16 *
MtSupportGetReportPrefix(CHAR16 *Buffer, UINTN BufferSize);

// MtSupportGetTimestampStr
//
// Get timestamp string for the given EFI_TIME
CHAR16 *MtSupportGetTimestampStr(EFI_TIME *Time);

#define EFI_FILE_MODE_TFTP (1ULL << 32)

// MtSupportOpenFile
//
// Open file for read/write
EFI_STATUS
MtSupportOpenFile(
    OUT MT_HANDLE *NewHandle,
    IN CHAR16 *FileName,
    IN UINT64 OpenMode,
    IN UINT64 Attributes);

// MtSupportCloseFile
//
// Close file
EFI_STATUS
MtSupportCloseFile(
    IN MT_HANDLE Handle);

// MtSupportDeleteFile
//
// Delete file
EFI_STATUS
MtSupportDeleteFile(
    IN MT_HANDLE Handle);

// MtSupportReadFile
//
// Read contents of file at current pointer into Buffer
EFI_STATUS
MtSupportReadFile(
    IN MT_HANDLE Handle,
    IN OUT UINTN *BufferSize,
    OUT VOID *Buffer);

// MtSupportWriteFile
//
// Write contents to file at current pointer from Buffer
EFI_STATUS
MtSupportWriteFile(
    IN MT_HANDLE Handle,
    IN OUT UINTN *BufferSize,
    IN VOID *Buffer);

// MtSupportSetPosition
//
// Set the current file pointer to the specified offset
EFI_STATUS
MtSupportSetPosition(
    IN MT_HANDLE Handle,
    IN UINT64 Position);

// MtSupportGetPosition
//
// Get the offset of the current file pointer
EFI_STATUS
MtSupportGetPosition(
    IN MT_HANDLE Handle,
    OUT UINT64 *Position);

// MtSupportGetFileSize
//
// Get the file size
EFI_STATUS
MtSupportGetFileSize(
    IN MT_HANDLE Handle,
    OUT UINT64 *FileSize);

// MtSupportFlushFile
//
// Write any uncommitted file changes to disk
EFI_STATUS
MtSupportFlushFile(
    IN MT_HANDLE Handle);

// MtSupportReadLine
//
// Read one line from a text file
EFI_STATUS
EFIAPI
MtSupportReadLine(
    IN MT_HANDLE Handle,
    IN OUT CHAR16 *Buffer,
    IN OUT UINTN *Size,
    IN BOOLEAN Truncate,
    IN OUT BOOLEAN *Ascii);

// MtSupportEof
//
// Return TRUE if end of file
BOOLEAN
EFIAPI
MtSupportEof(
    MT_HANDLE FileHandle);

// MtSupportEfiFileHandleToMtHandle
//
// Convert an EFI_FILE_HANDLE to MT_HANDLE
EFI_STATUS
EFIAPI
MtSupportEfiFileHandleToMtHandle(
    IN EFI_FILE_HANDLE EfiHandle,
    OUT MT_HANDLE *MtHandle);

// MtSupportGetEfiFileHandle
//
// Get the EFI_FILE_HANDLE from MT_HANDLE
EFI_STATUS
EFIAPI
MtSupportGetEfiFileHandle(
    IN MT_HANDLE MtHandle,
    IN EFI_FILE_HANDLE *EfiHandle);

// MtSupportReadTimeOfDay
//
// Read the time of day from file
BOOLEAN
EFIAPI
MtSupportReadTimeOfDay(MT_HANDLE Handle, EFI_TIME *Time);

// MtSupportReadBlacklist
//
// Read the blacklist from file
EFI_STATUS
EFIAPI
MtSupportReadBlacklist(MT_HANDLE FileHandle, BLACKLIST *Config);

// MtSupportSetDefaultTestConfig
//
// Set to default test configuration
VOID
    EFIAPI
    MtSupportSetDefaultTestConfig(MTCFG *Config, TESTCFG **pTestList, int *NumTests);

// MtSupportReadTestConfig
//
// Read custom test configuration from file
EFI_STATUS
EFIAPI
MtSupportReadTestConfig(MT_HANDLE Handle, TESTCFG **pTestList, int *NumTests);

// MtSupportReadConfig
//
// Read the memory test settings from configuration file
EFI_STATUS
EFIAPI
MtSupportReadConfig(MT_HANDLE Handle, MTCFG *Config, UINTN *ConfigSize, INTN *DefaultIdx, UINTN *Timeout);

// MtSupportInitConfig
//
// Set default configuration
void
    EFIAPI
    MtSupportInitConfig(MTCFG *Config);

// MtSupportReadConfig
//
// Write the memory test settings to the configuration file
EFI_STATUS
EFIAPI
MtSupportWriteConfig(MT_HANDLE FileHandle);

// MtSupportApplyConfig
//
// Apply the configuration settings to MemTest86
EFI_STATUS
EFIAPI
MtSupportApplyConfig(MTCFG *Config);

// MtSupportSaveLastConfigSelected
//
// Save the last configuration selected to file
EFI_STATUS
EFIAPI
MtSupportSaveLastConfigSelected(UINTN LastCfg);

EFI_STATUS
EFIAPI
MtSupportParseCommandArgs(CHAR16 *CommandLineArgs, UINTN CommandLineArgsSize, MTCFG *Config);

// MtSupportIsValidSPDFile
//
// Check whether the SPD file is valid
BOOLEAN
EFIAPI
MtSupportIsValidSPDFile(CHAR16 *FileName);

// MtSupportReadSPDFile
//
// Read the SPD values from file
EFI_STATUS
EFIAPI
MtSupportReadSPDFile(MT_HANDLE FileHandle, UINT8 *SPDBytes, UINT8 *BitMask, UINTN *NumSPDBytes);

// MtSupportCheckSPD
//
// Compare the SPD data to values from file
BOOLEAN
EFIAPI
MtSupportCheckSPD(CHAR16 *FileName, int *pFailedSPDMatchIdx, int *pFailedSPDMatchByte, UINT8 *pActualValue, UINT8 *pExpectedValue);

// MtSupportCheckMemSpeed
//
// Check configured memory speed with SPD profiles
BOOLEAN
EFIAPI
MtSupportCheckMemSpeed(CHAR16 *CfgMemSpeedBuf, UINTN CfgMemSpeedBufSize, CHAR16 *CheckSpeedBuf, UINTN CheckSpeedBufSize);

// MtSupportSkipDIMMResults
//
// Return TRUE if the DIMM results screen should not be displayed
BOOLEAN
EFIAPI
MtSupportSkipDIMMResults();

// MtSupportGetTSODInfo
//
// Poll the temperatures of all DIMMs. Return the number of DIMM temperatures read.
int
    EFIAPI
    MtSupportGetTSODInfo(int *DimmTemp, int DimmTempSize);

typedef struct _SPDINFO SPDINFO;
// memoryType
//
// Get the memory module, speed and bandwidth text string from the SPD details
void memoryType(int tCK, SPDINFO *spdinfo, CHAR16 *szwModuleType, CHAR16 *szwSpeed, CHAR16 *szwBandwidth, UINTN bufSize);

// getSPDSummaryLine
//
// Get a one-line RAM summary text string from the SPD details
CHAR16 *getSPDSummaryLine(SPDINFO *spdinfo, CHAR16 *szwRAMSummary, UINTN bufSize);

// getSPDProfileStr
//
// Get the SPD profile string for the given the timing parameters
CHAR16 *getSPDProfileStr(int memtype, int clkspeed, int tCK, int tAA, int tRCD, int tRP, int tRAS, const CHAR16 *pszVoltage, CHAR16 *Buffer, UINTN BufferSize);

// getTimingsStr
//
// Get the 4-tuple memory latency string (CL-tRCD-tRP-tRAS) for the given the timing parameters
CHAR16 *getTimingsStr(int memtype, int tCK, int tAA, int tRCD, int tRP, int tRAS, CHAR16 *Buffer, UINTN BufferSize);

typedef struct _SMBIOS_MEMORY_DEVICE SMBIOS_MEMORY_DEVICE;

// getSMBIOSMemoryType
//
// Get the memory module, speed and bandwidth text string from the SMBIOS details
void getSMBIOSMemoryType(SMBIOS_MEMORY_DEVICE *pSMBIOSMemDev, CHAR16 *szwModuleType, CHAR16 *szwSpeed, CHAR16 *szwBandwidth, UINTN bufSize);

// getSMBIOSRAMSummaryLine
//
// Get a one-line RAM summary text string from the SMBIOS details
CHAR16 *getSMBIOSRAMSummaryLine(SMBIOS_MEMORY_DEVICE *pSMBIOSMemDev, CHAR16 *szwRAMSummary, UINTN bufSize);

// getSPDProfileStr
//
// Get the RAM profile string from the SMBIOS details
CHAR16 *getSMBIOSRAMProfileStr(SMBIOS_MEMORY_DEVICE *pSMBIOSMemDev, CHAR16 *szwRAMSummary, UINTN bufSize);

// getRAMTypeString
//
// Get the RAM type string
void getRAMTypeString(CHAR16 *RAMType, UINTN RAMTypeSize);

// getMaxSPDInfoStr
//
// Get the maximum memory module, speed and bandwidth text string from the SPD details
void getMaxSPDInfoStr(SPDINFO *spdinfo, CHAR16 *szwRAMType, CHAR16 *szwXferSpeed, CHAR16 *szwMaxBandwidth, CHAR16 *szwTiming, CHAR16 *szwVoltage, UINTN bufSize);

// getMemConfig
//
// Get the configured memory settings used by the memory controller
void getMemConfig(CHAR16 *szwXferSpeed, CHAR16 *szwChannelMode, CHAR16 *szwTiming, CHAR16 *szwVoltage, CHAR16 *szwECC, UINTN bufSize);

// getMemConfigStr
//
// Get a one-line text string containing the currently configured memory timings
CHAR16 *getMemConfigStr(CHAR16 *szwRAMSummary, UINTN bufSize);

// getMemConfigSummaryLine
//
// Get a one-line text string containing the currently configured memory timings
void getMemConfigSummaryLine(CHAR16 *szwRAMSummary, UINTN bufSize);

// getCPUSpeedLine
//
// Get the CPU speed text string
void getCPUSpeedLine(int khzspeed, int khzspeedturbo, CHAR16 *szwCPUSpeed, UINTN bufSize);

// output_spd_info
//
// Write SPD details of specified DIMM to file
void output_spd_info(MT_HANDLE FileHandle, int dimm);

// output_smbios_info
//
// Write SMBIOS details of specified DIMM to file
void output_smbios_info(MT_HANDLE FileHandle, int dimm);

// display_memmap
//
// Display the EFI memory map
void display_memmap(MT_HANDLE FileHandle);

// change_save_location
//
// Change the report/log save location
void change_save_location();

UINT16
EFIAPI
MtSupportTestConfig();

VOID
    EFIAPI
    MtSupportEnableCache(BOOLEAN Enable);

UINT64 MtSupportMemSizeGB();

UINT64 MtSupportMemSizeMB();

CHAR16 *MtSupportHumanReadableSize(UINT64 SizeBytes, CHAR16 *Buffer, UINTN BufferSize);

CHAR16 *MtSupportGetSlotLabel(int slot, BOOLEAN LongName, CHAR16 *Buffer, UINTN BufferSize);

CHAR16 *MtSupportGetChipLabel(int chip, CHAR16 *Buf, UINTN BufSize);

BOOLEAN MtSupportECCEnabled();

void MtSupportGetECCFeatures(CHAR16 *Buffer, UINTN BufferSize);

int MtSupportGetNumRanks(int dimm);

int MtSupportGetChipWidth(int dimm);

UINT8 MtSupportGetFormFactor(int dimm);

UINT8 MtSupportGetMemoryFormFactor();

UINT8 MtSupportGetDimmsPerChannel();

CHAR16 *BytesToStrHex(const unsigned char *Bytes, UINTN BytesLen, CHAR16 *Buf, UINTN BufLen);

#endif
