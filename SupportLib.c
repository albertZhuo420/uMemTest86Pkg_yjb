// PassMark MemTest86
//
// Copyright (c) 2013-2016
//   This software is the confidential and proprietary information of PassMark
//   Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//   such Confidential Information and shall use it only in accordance with
//   the terms of the license agreement you entered into with PassMark
//   Software.
//
// Program:
//   MemTest86
//
// Module:
//   SupportLib.c
//
// Author(s):
//   Keith Mah
//
// Description:
//   Support functions for MemTest86
//
//   Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
//
// References:
//   https://github.com/jljusten/MemTestPkg
//

/** @file
  Memory test support library

  Copyright (c) 2009, Intel Corporation
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

/* config.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */

/* error.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */

/* lib.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */

#include "SupportLib.h"
#include <Library/IntelPortingLib.h>
#include <Library/Udp4SocketLib.h>

#define INT_MAX 2147483647
#define INT_MIN (-2147483647 - 1)
#define FIELD_SIZEOF(t, f) (sizeof(((t *)0)->f))
#ifdef __GNUC__
#define ALIGN(X) __attribute__((aligned(X)))
#elif defined(_MSC_VER)
#define ALIGN(X) __declspec(align(X))
#else
#error Unknown compiler, unknown alignment attribute!
#endif

#define PAGE_OF(addr) ((UINTN)(RShiftU64(addr, 12))) // get the page number of an address

#define CEIL_SPD(dividend, divisor) ((dividend + divisor - 40) / divisor) // calculate the ceiling for SPD timings

#define AP_SIGNAL_WAIT_TIME 1000
#define BSP_WAIT_TIME_FACTOR 10
#define UI_UPDATE_PERIODMS 250 // UI update time in milliseconds
#define KEYBOARD_POLL_MS 100   // keyboard poll time in milliseconds
#define ECC_POLL_MS 1000       // calculate the ceiling for SPD timings
#define TEMP_POLL_MS 60000     // temperature poll time in milliseconds
#define TIMINGS_POLL_MS 60000  // memory timings poll time in milliseconds

#define RANDOMSEED 0x50415353 // random seed for the first pass

#define BOM_UTF16LE "\xff\xfe" // UTF16-LE BOM

// SyncEnableCacheAllAPs
//
// Enable/disable cache on all APs in synchronization
VOID SyncEnableCacheAllAPs(BOOLEAN Enable);

VOID SyncSetAllMtrrsAllProcs(MTRR_SETTINGS *AllMtrrs);

// From MtrrLib.c
extern VOID
MtrrLibInitializeMtrrMask(
    OUT UINT64 *MtrrValidBitsMask,
    OUT UINT64 *MtrrValidAddressMask);

STATIC MEM_TEST_INSTANCE *mTests = NULL; // array of installed tests
STATIC UINTN mTestCount = 0;             // number of installed tests
STATIC UINTN mMaxTestCount = 0;          // size of mTests array
STATIC UINTN mCurTestIdx = 0;            // index of currently running test
STATIC UINTN mCurPass = 0;               // current pass number
STATIC UINTN mPassCount = 0;             // # of tests passed
STATIC UINTN mTestsCompleted = 0;        // # of tests completed in current pass
STATIC BOOLEAN mAbortTesting = FALSE;    // TRUE if user wants to end the test
STATIC BOOLEAN mSkipTest = FALSE;        // TRUE if user wants to skip the current test
STATIC UINT64 mTestedMemSize = 0;        // total size of memory locked for testing

STATIC EFI_TIME mTestStartTime; // test start time
STATIC UINT64 mStartTime;       // test start timestamp
STATIC UINT32 mElapsedTime;     // test elapsed time in seconds

STATIC int mMinCPUTemp = INT_MAX;
STATIC int mMaxCPUTemp = INT_MIN;
STATIC int mCurCPUTemp = INT_MAX;
STATIC INT64 mSumCPUTemp;
STATIC INT64 mNumCPUTempSamples;

#define MAX_RAM_TEMP_DIMMS 64

STATIC int mMinRAMTemp[MAX_RAM_TEMP_DIMMS];
STATIC int mMaxRAMTemp[MAX_RAM_TEMP_DIMMS];
STATIC int mCurRAMTemp[MAX_RAM_TEMP_DIMMS];
STATIC INT64 mSumRAMTemp[MAX_RAM_TEMP_DIMMS];
STATIC INT64 mNumRAMTempSamples[MAX_RAM_TEMP_DIMMS];

typedef struct
{
    int SpeedMTs;
    UINT8 tCAS;
    UINT8 tRCD;
    UINT8 tRP;
    UINT8 tRAS;
} MEM_TIMINGS;

STATIC MEM_TIMINGS mMinMemTimings = {INT_MAX};
STATIC MEM_TIMINGS mMaxMemTimings = {INT_MIN};
STATIC MEM_TIMINGS mCurMemTimings = {0};

STATIC UINT64 mPrevUpdateTime = 0;
STATIC UINT64 mPrevKBPollTime = 0;
STATIC UINT64 mPrevEccPollTime = 0;
STATIC UINT64 mPrevTempPollTime = 0;
static UINT64 mPrevTimingsPollTime = 0;
STATIC UINT64 mPrevPMPStatusTime = 0;
STATIC UINT64 mPrevTestFlowPollTime = 0;

// For MTRR cache configuration
static UINT64 mMtrrValidBitsMask;
static UINT64 mMtrrValidAddressMask;
static MTRR_SETTINGS mCurMtrrs, mPrevMtrrs;
static int mFreeMtrrInd = -1;
extern BOOLEAN              gOknTestStart;
extern BOOLEAN              gOknTestPause;
extern UINT8                gOknTestReset;
extern UINT8                gOknTestStatus;
extern UDP4_SOCKET          *gOknUdpSocketTransmit;

extern UINT64 gAddrLimitLo;                   // lower address limit
extern UINT64 gAddrLimitHi;                   // upper address limit
extern UINT64 gMemRemMB;                      // Amount of memory (in MB) to leave unallocated
extern UINT64 gMinMemRangeMB;                 // Minimum size of memory range (in MB) to allocate
extern UINTN gCPUSelMode;                     // CPU selection mode
extern UINTN gSelCPUNo;                       // selected CPU number (single CPU mode)
extern UINTN gMaxCPUs;                        // Max number of CPUs
extern UINTN gNumPasses;                      // number of passes of the test sequence to perform
extern UINTN gCacheMode;                      // memory cache mode
extern BOOLEAN gECCPoll;                      // enable/disable poll for ECC errors
extern BOOLEAN gECCInject;                    // enable/disable ECC injection
extern BOOLEAN gTSODPoll;                     // enable/disable poll DIMM TSOD temperatures
extern CHAR16 gTestCfgFile[SHORT_STRING_LEN]; // filename containing a list of custom test configurations
extern TESTCFG *gCustomTestList;              // contains list of custom test configurations, if specified
extern int gNumCustomTests;                   // Size of gCustomTestList;
extern BOOLEAN gUsingCustomTests;             // if TRUE, using custom tests defined in test config file
extern BOOLEAN gPass1Full;                    // if TRUE, perform full test during the first pass (default is reduced test for first pass)
extern UINT64 gAddr2ChBits;                   // Bit mask of address bits to XOR to determine channel number
extern UINT64 gAddr2SlBits;                   // Bit mask of address bits to XOR to determine slot number
extern UINT64 gAddr2CSBits;                   // Bit mask of address bits to XOR to determine CS number
extern CHIPMAPCFG *gChipMapCfg;               // DRAM chip numbering map
extern int gNumChipMapCfg;                    // Size of gChipMapCfg
extern int gCurLang;                          // selected language
extern UINTN gReportNumErr;                   // maximum number of the most recent errors to display in the report
extern UINTN gReportNumWarn;                  // maximum number of the most recent warnings to display in the report
extern BOOLEAN gDisableSlowHammering;         // For DEBUG only. If TRUE, only hammer at the maximum rate.
extern UINTN gAutoMode;                       // 0 - default (user intervention required); 1 - tests will run and report saved without user intervention; 2 - tests will run automatically but prompts remain
extern BOOLEAN gAutoReport;                   // if TRUE, save report automatically (default)
extern UINTN gAutoReportFmt;                  // The report format when saving automatically
extern BOOLEAN gAutoPromptFail;               // if TRUE, display FAIL message box on errors even if gAutoReport==AUTOMODE_ON
extern CHAR16 gTCPServerIP[SHORT_STRING_LEN];
extern CHAR16 gTCPGatewayIP[SHORT_STRING_LEN];
extern UINT32 gTCPServerPort;
extern CHAR16 gTCPRequestLocation[LONG_STRING_LEN]; // The route location for console message handler. Default /mgtconsolemsghandler.php
extern CHAR16 gTCPClientIP[SHORT_STRING_LEN];
extern BOOLEAN gSkipSplash;                  // if TRUE, the 10 second splashscreen is skipped
extern UINTN gSkipDecode;                    // Policy for whether to skip the decode results screen. See SKIPDECODE_*
extern UINTN gExitMode;                      // 0 - reboot on exit; 1 - shutdown on exit; 2 - exit MemTest86 and return control to UEFI BIOS
extern BOOLEAN gDisableSPD;                  // if TRUE, disable SPD collection
extern UINTN gMinSPDs;                       // minimum number of detected DIMM SPDs before starting the test
extern INTN gExactSPDs[MAX_NUM_SPDS];        // exact number of detected DIMM SPDs before starting the test. This variable overwrites gMinSPDs if >= 0
extern INT64 gExactSPDSize;                  // exact total size of detected DIMM SPDs before starting the test.
extern BOOLEAN gCheckMemSPDSize;             // if TRUE, check to see if the system memory size is consistent with the total memory capacity of all SPDs detected
extern CHAR16 gSPDManuf[SHORT_STRING_LEN];   // substring to match the JEDEC manufacturer in the SPD before starting the test
extern CHAR16 gSPDPartNo[MODULE_PARTNO_LEN]; // substring to match the part number in the SPD before starting the test
extern BOOLEAN gSameSPDPartNo;               // if TRUE, check to see if all SPD part numbers match
extern BOOLEAN gSPDMatch;                    // if TRUE, check to see if SPD data match the values defined in the SPD.spd file
extern CHECKMEMSPEED *gCheckMemSpeed;        // Zero or more checks on active memory speed
extern int gNumCheckMemSpeed;                // Size of gCheckMemSpeed
extern BOOLEAN gCheckSPDSMBIOS;              // if TRUE, check if SPD and SMBIOS values are consistent
extern INTN gSPDReportByteLo;                // Lower byte of the range of the SPD raw bytes to display in the report
extern INTN gSPDReportByteHi;                // Upper byte of the range of the SPD raw bytes to display in the report
extern BOOLEAN gSPDReportExtSN;
extern int gFailedSPDMatchIdx;                 // if gSPDMatch is TRUE and SPD check has failed, contains the failed SPD DIMM #
extern int gFailedSPDMatchByte;                // if gSPDMatch is TRUE and SPD check has failed, contains the failed SPD byte #
extern UINT8 gFailedSPDActualValue;            // if gSPDMatch is TRUE and SPD check has failed, contains the failed SPD byte value
extern UINT8 gFailedSPDExpectedValue;          // if gSPDMatch is TRUE and SPD check has failed, contains the expected SPD byte value
extern BOOLEAN gTriggerOnErr;                  // if TRUE, show the address of the error structure before the start of the test
extern UINTN gReportPrefix;                    // 0 - default, 1 - Prefix filename with smbios baseboard serial number, 2 - Prefix filename with smbios system information serial number
extern CHAR16 gReportPrepend[MAX_PREPEND_LEN]; // Prepend report filename with custom string

// supported background colours allowable in the config file
const CHAR16 *SUPPORTED_BGCOLORS[] =
    {
        L"BLACK",
        L"BLUE",
        L"GREEN",
        L"CYAN",
        L"RED",
        L"MAGENTA",
        L"BROWN",
        L"LIGHTGRAY",
        NULL};

#ifdef PRO_RELEASE
static UINTN SUPPORTED_BGCOLORS_MAP[] =
    {
        EFI_BLACK,
        EFI_BLUE,
        EFI_GREEN,
        EFI_CYAN,
        EFI_RED,
        EFI_MAGENTA,
        EFI_BROWN,
        EFI_LIGHTGRAY};
#endif

extern UINTN gBgColour; // The background colour to use for the screen
extern UINTN gConsoleMode;
extern BOOLEAN gConsoleOnly;
extern BOOLEAN gHammerRandom;             // If TRUE, use random data patterns for hammering
extern UINT32 gHammerPattern;             // If gHammerRandom == FALSE, specifies the data pattern to use for hammering
extern UINTN gHammerMode;                 // Hammer mode: single or double-sided hammering
extern UINT64 gHammerStep;                // The step size to use when hammering addresses
extern const CHAR8 *mSupportedLangList[]; // list of supported languages
extern BOOLEAN mLangFontSupported[];      // whether the language characters are supported in the font charset
extern UINT32 gRestrictFlags;             // Contains any restrictions to apply based on the baseboard blacklist

extern BOOLEAN gMPSupported; // whether multiprocessor is supported for the system
extern BOOLEAN gDisableMP;
extern BOOLEAN gEnableHT;
extern UINTN gBitFadeSecs;
extern UINTN gMaxErrCount;
extern UINTN ConHeight;
extern UINTN ConWidth;
extern UINTN MaxUIWidth; // Maximum width within the console

extern BOOL gRunFromBIOS;         // if TRUE, MemTest86 is running from BIOS (ie. no file write support)
extern BOOL gTest12SingleCPU;     // if TRUE, run Test 12 in SINGLE CPU mode.
STATIC UINTN mSelProcNum;         // current BSP proc running the memory tests
STATIC UINT64 mProcLenTested = 0; // For keeping track of how much RAM tested on current CPU in sequential mode

STATIC BOOLEAN mCPURunning[MAX_CPUS * 64]; // whether the CPU is running/waiting. One cache line per CPU for performance
struct barrier_s mBarr;                    // barrier structure for synchronization

/* Error details */
struct xadr
{
    UINTN page;
    UINTN offset;
};

#ifdef _MSC_VER
#pragma warning(disable : 4324)
#endif
struct err_info
{
    // struct xadr   low_addr;
    // struct xadr   high_addr;
    UINTN low_addr;
    UINTN high_addr;
    UINTN ebits;
    int tbits;
    short min_bits;
    short max_bits;
    int maxl;
    UINTN eadr;
    ALIGN(16)
    __m128i exor;
    UINT32 etest;
    UINT32 data_err;
    UINT32 data_warn;
    UINT32 cor_err;
    UINT32 uncor_err;
    UINT64 ecpus[8];
    UINT32 slot_err[MAX_SLOTS];
    UINT32 slot_chip_err[MAX_SLOTS][MAX_CHIPS];
    UINT32 unknown_slot_err;
    // short         hdr_flag;
} erri;

STATIC SPIN_LOCK mErrorLock; // mutex for error structure

#define DEFAULT_ERRBUF_SIZE 64
#define DEFAULT_WARNBUF_SIZE 64
struct ERRLINE
{
    BOOLEAN Valid;
    EFI_TIME Time;
    UINTN ProcNum;
    UINT32 TestNo;
    INT32 Type;
    INT32 AddrType;
    union
    {
        UINTN PhysAddr;
        struct
        {
            INT32 col;
            INT32 row;
            INT8 bank;
            INT8 rank;
        } dimm;
    } AddrInfo;
    union
    {
        struct
        {
            ALIGN(16)
            __m128i Expected;
            ALIGN(16)
            __m128i Actual;
            ALIGN(16)
            __m128i ErrorBits;
        } DataErr;
        struct
        {
            BOOLEAN corrected;
            UINT8 type;  // ECC type
            INT32 syn;   // syndrome
            INT8 socket; // CPU socket
            INT8 chan;   // channel
            INT8 dimm;   // slot within channel
        } ECCErr;
    } ErrInfo;
    UINT32 ErrDataSize;
};

#define BADRAM_MAXPATNS 10

struct BADRAMPATN
{
    UINTN addr;
    UINTN mask;
};

STATIC struct BADRAMPATN mBADRAMPatn[BADRAM_MAXPATNS]; // circular buffer for storing error details
STATIC int mNumBADRAMPatn = 0;

#define MAXBADPFN 64
STATIC UINTN mBadPFNList[MAXBADPFN]; // circular buffer for storing error details
STATIC int mBadPFNListSize = 0;

#pragma pack(1)
struct ERRINFO
{
    UINT64 Signature;
    UINT64 PhysAddr;
    ALIGN(16)
    __m128i Expected;
    ALIGN(16)
    __m128i Actual;
    ALIGN(16)
    __m128i ErrorBits;
};

#define REPORT_BIN_SIG "MT86"
#define REPORT_BIN_MAJOR_REV 0x01
#define REPORT_BIN_MINOR_REV 0x01

typedef struct _TESTRESULT
{
    /* Byte 0 */ CHAR8 Signature[4];         // "MT86" signature
    /* Byte 4 */ UINT16 Revision;            // Current revision: 0x0101 (Upper byte: major version; Lower byte: minor version)
    /* Byte 6 */ UINT32 StartTime;           // Start test time in Unix time, accurate to 1 sec
    /* Byte 10 */ UINT32 ElapsedTime;        // Test elapsed time in seconds
    /* Byte 14 */ UINT64 RangeMin;           // Min address tested
    /* Byte 22 */ UINT64 RangeMax;           // Max address tested
    /* Byte 30 */ UINT16 CPUSelMode;         // 0 - Single CPU, 1 - Parallel, 2 - Round Robin, 3 - Sequential
    /* Byte 32 */ INT16 CPUTempMin;          // Min CPU temp for duration of test (-1 if N/A)
    /* Byte 34 */ INT16 CPUTempMax;          // Max CPU temp for duration of test (-1 if N/A)
    /* Byte 36 */ INT16 CPUTempAve;          // Ave CPU temp for duration of test (-1 if N/A)
    /* Byte 38 */ INT16 RAMTempMin;          // Min RAM temp for duration of test (-1 if N/A)
    /* Byte 40 */ INT16 RAMTempMax;          // Max RAM temp for duration of test (-1 if N/A)
    /* Byte 42 */ INT16 RAMTempAve;          // Ave RAM temp for duration of test (-1 if N/A)
    /* Byte 44 */ BOOLEAN ECCSupport;        // 0 if ECC not supported, != 0 if ECC supported
    /* Byte 45 */ INT8 TestResult;           // {0:PASS, 1:INCOMPLETE PASS, -1:FAIL, -2:INCOMPLETE FAIL}
    /* Byte 46 */ UINT32 ErrorCode;          // Error code
    /* Byte 50 */ UINT32 NumErrors;          // Number of errors from all tests. Must be 0 if Passed == True
    /* Byte 54 */ UINT64 MinErrorAddr;       // Lowest address that had an error
    /* Byte 62 */ UINT64 MaxErrorAddr;       // Highest address that had an error
    /* Byte 70 */ UINT64 ErrorBits;          // Bit coded field showing the bits in error
    /* Byte 78 */ UINT32 NumCorrECCErrors;   // Number of detected and corrected ECC errors from all tests. These errors are corrected, and therefore not included in NumErrors
    /* Byte 82 */ UINT32 NumUncorrECCErrors; // Number of detected but uncorrected ECC errors from all tests. These errors may be included in NumErrors
    /* Byte 86 */ UINT8 Reserved[14];        // For future use
    /* Byte 100 */ UINT16 NumTestsEnabled;   // Number of individual tests enabled. ie. Size of variable-sized array AllTests below
    /* Byte 102 */ struct
    {
        UINT8 TestNo;             // Test ID number
        UINT16 NumTestsPassed;    // Number tests passed for this test number
        UINT16 NumTestsCompleted; // Number tests completed for this test number
        UINT32 NumErrors;         // Number of errors detected for this test number
    } AllTests[1];
    /* Byte 102 + NumTestsEnabled * sizeof(AllTests[0]) */
    struct SLOTCHIP_ERRINFO
    {
        UINT16 SlotChipErrsArrCount; // Size of variable-sized array SlotChipErrs below
        struct
        {
            INT8 Slot;        // Slot number (-1 for unknown)
            INT8 Chip;        // Chip number (-1 for unknown)
            UINT32 NumErrors; // Number of errors detected for this slot/chip combination
        } SlotChipErrs[1];
    } SlotChipErrInfo;
} TESTRESULT;

typedef struct _TESTRESULT_1_0
{
    /* Byte 0 */ CHAR8 Signature[4];         // "MT86" signature
    /* Byte 4 */ UINT16 Revision;            // Current revision: 0x0100 (Upper byte: major version; Lower byte: minor version)
    /* Byte 6 */ UINT32 StartTime;           // Start test time in Unix time, accurate to 1 sec
    /* Byte 10 */ UINT32 ElapsedTime;        // Test elapsed time in seconds
    /* Byte 14 */ UINT64 RangeMin;           // Min address tested
    /* Byte 22 */ UINT64 RangeMax;           // Max address tested
    /* Byte 30 */ UINT16 CPUSelMode;         // 0 - Single CPU, 1 - Parallel, 2 - Round Robin, 3 - Sequential
    /* Byte 32 */ INT16 CPUTempMin;          // Min CPU temp for duration of test (-1 if N/A)
    /* Byte 34 */ INT16 CPUTempMax;          // Max CPU temp for duration of test (-1 if N/A)
    /* Byte 36 */ INT16 CPUTempAve;          // Ave CPU temp for duration of test (-1 if N/A)
    /* Byte 38 */ INT16 RAMTempMin;          // Min RAM temp for duration of test (-1 if N/A)
    /* Byte 40 */ INT16 RAMTempMax;          // Max RAM temp for duration of test (-1 if N/A)
    /* Byte 42 */ INT16 RAMTempAve;          // Ave RAM temp for duration of test (-1 if N/A)
    /* Byte 44 */ BOOLEAN ECCSupport;        // 0 if ECC not supported, != 0 if ECC supported
    /* Byte 45 */ INT8 TestResult;           // 0 if NumErrors > 0, 1 if NumErrors == 0, -1 if no tests were completed
    /* Byte 46 */ UINT32 ErrorCode;          // Error code
    /* Byte 50 */ UINT32 NumErrors;          // Number of errors from all tests. Must be 0 if Passed == True
    /* Byte 54 */ UINT64 MinErrorAddr;       // Lowest address that had an error
    /* Byte 62 */ UINT64 MaxErrorAddr;       // Highest address that had an error
    /* Byte 70 */ UINT64 ErrorBits;          // Bit coded field showing the bits in error
    /* Byte 78 */ UINT32 NumCorrECCErrors;   // Number of detected and corrected ECC errors from all tests. These errors are corrected, and therefore not included in NumErrors
    /* Byte 82 */ UINT32 NumUncorrECCErrors; // Number of detected but uncorrected ECC errors from all tests. These errors may be included in NumErrors
    /* Byte 86 */ UINT8 Reserved[14];        // For future use
    /* Byte 100 */ UINT16 NumTestsEnabled;   // Number of individual tests enabled. ie. Size of variable-sized array AllTests below
    /* Byte 102 */ struct
    {
        UINT8 TestNo;             // Test ID number
        UINT16 NumTestsPassed;    // Number tests passed for this test number
        UINT16 NumTestsCompleted; // Number tests completed for this test number
        UINT32 NumErrors;         // Number of errors detected for this test number
    } AllTests[1];
} TESTRESULT_1_0;
#pragma pack()

#ifdef PRO_RELEASE
#define ERRLINE_ERROR_SIG 0xdeadbeefdeadbeefull
#else
#define ERRLINE_ERROR_SIG 0x1ull
#endif

STATIC struct ERRLINE *mErrorBuf; // circular buffer for storing error details
STATIC UINT mErrorBufSize;        // size of mErrorBuf
STATIC UINT32 mErrorBufHead;      // head of buffer
STATIC UINT32 mErrorBufTail;      // tail of buffer
STATIC UINT32 mErrorBufFullCount; // errors dropped due to circular buffer being full
STATIC struct ERRINFO mLastError;
static BOOLEAN mTruncateErrLog;

STATIC __inline BOOLEAN IsErrorBufFull()
{
    return ((mErrorBufTail + 1) % mErrorBufSize) == mErrorBufHead;
}

STATIC struct ERRLINE *mWarningBuf; // circular buffer for storing warning details
STATIC UINT mWarningBufSize;        // size of mWarningBuf
STATIC UINT32 mWarningBufHead;      // head of buffer
STATIC UINT32 mWarningBufTail;      // tail of buffer

STATIC BOOLEAN mErrorsAsWarnings; // If TRUE, all memory errors are stored as warnings
STATIC BOOLEAN mErrorsIgnore;     // If TRUE, do not report memory errors

#define DEFAULT_CPUERRBUF_SIZE 10
struct CPUERR
{
    BOOLEAN Valid;
    EFI_TIME Time;
    UINTN ProcNum;
    UINT32 TestNo;
};
STATIC struct CPUERR *mCPUErrBuf; // circular buffer for storing CPU error details
STATIC UINT mCPUErrBufSize;       // size of mCPUErrBuf
STATIC UINT32 mCPUErrBufHead;     // head of buffer
STATIC UINT32 mCPUErrBufTail;     // tail of buffer

static BOOLEAN mCPUErrReported;

#if 0 // Not used
STATIC UINTN                mNumHammerErrors;
STATIC UINTN                mNumRowsHammered;
#endif
STATIC SPIN_LOCK mDebugLock;
#if 0 // Not used
#define DEBUGBUF_SIZE 32
STATIC CHAR8                mDebugBuf[DEBUGBUF_SIZE][128];

STATIC UINT32               mDebugBufHead;
STATIC UINT32               mDebugBufTail;
#endif

// #define DHCP_ENABLED // Defines if DHCP should be used to obtain an ip address for connecting to the management console
// #define DNS_ENABLED
#define MAX_TCP_BUFFER_SIZE 16384

// TCP Mangement Console Configuration information
typedef struct
{
    BOOL TCPAvaliable;
    BOOL DHCPAvaliable;
    BOOL DNSAvaliable;
    EFI_TCP4_CONNECTION_STATE TCPConnectionState;

    EFI_IPv4_ADDRESS SubnetMask;
    EFI_IPv4_ADDRESS ClientAddress;
    EFI_IPv4_ADDRESS RemoteAddress;

    EFI_TCP4_CONFIG_DATA TCPConfigData;

} PMP_TCP_CONFIGURATION;

PMP_TCP_CONFIGURATION gPMPConnectionConfig;

EFI_TCP4_PROTOCOL *TCP4 = NULL;
EFI_DHCP4_PROTOCOL *DHCP = NULL;
EFI_DNS4_PROTOCOL *DNS = NULL;
EFI_TLS_PROTOCOL *TLS = NULL;
EFI_IPv4_ADDRESS DHCPAddr;

EFI_GUID dhcpserviceguid = {0x9D9A39D8, 0xBD42, 0x4A73, {0xA4, 0xD5, 0x8E, 0xE9, 0x4B, 0xE1, 0x13, 0x80}};
EFI_GUID dhcpprotguid = {0x8A219718, 0x4EF5, 0x4761, {0x91, 0xC8, 0xC0, 0xF0, 0x4B, 0xDA, 0x9E, 0x56}};

/* Hardware details extern */
extern UINTN clks_msec;
extern UINT64 minaddr;
extern UINT64 maxaddr;

extern EFI_TIME gBootTime;
extern EFI_PXE_BASE_CODE_PROTOCOL *PXEProtocol; // Protocol to handle PXE booting
extern EFI_IP_ADDRESS ServerIP;                 // IP address of the PXE server
extern EFI_IP_ADDRESS ClientIP;                 // IP address of the client;
extern EFI_MAC_ADDRESS ClientMAC;               // MAC address of the client;
extern CHAR16 gTftpRoot[256];                   // The path in the PXE server to read/store MemTest86 related files
extern EFI_IP_ADDRESS gTFTPServerIp;
extern UINTN gTFTPStatusSecs; // Management console status report period
extern BOOLEAN gPMPDisable;   // Disable XML reporting to PXE server
extern BOOLEAN gTCPDisable;   // Disable TCP networking for mgmt console
extern BOOLEAN gDHCPDisable;
extern BOOLEAN gCloudEnable; // Connect to cloud instance with specified API key
extern CHAR16 gAPIKey[256];
extern BOOLEAN gRTCSync; // Sync real-time clock with PXE server

extern EFI_FILE *DataEfiDir;
extern EFI_FILE *SaveDir;
extern EFI_FILE *SelfDir;
extern EFI_HANDLE *FSHandleList;
extern UINTN NumFSHandles;
extern INTN SaveFSHandle;
extern EFI_BLOCK_IO_PROTOCOL *DMATestPart; // Block I/O Protocol for DMA test partition

extern BOOLEAN gDebugMode;
extern UINTN gVerbosity;
extern EFI_TPL gTPL;

extern struct cpu_ident cpu_id; // results of execution cpuid instruction
extern CPUINFO gCPUInfo;
// extern int l1_cache, l2_cache, l3_cache;
extern UINT64 memsize;

extern UINTN cpu_speed;
extern UINTN mem_speed;
extern UINTN mem_latency;
extern UINTN l1_speed;
extern UINTN l2_speed;
extern UINTN l3_speed;

extern CPUMSRINFO cpu_msr;
extern BOOLEAN gSIMDSupported;   // SIMD instructions supported
extern BOOLEAN gNTLoadSupported; // Non-temporal SIMD load supported

extern SPDINFO *g_MemoryInfo; // Var to hold info from smbus on memory
extern int g_numMemModules;
extern int g_numSMBIOSMem;
extern int g_numSMBIOSModules;
extern SMBIOS_MEMORY_DEVICE *g_SMBIOSMemDevices;
extern PCIINFO *g_TSODController;
extern int g_numTSODCtrl;
extern int g_numTSODModules;
extern int g_maxTSODModules; // size of g_MemoryInfo structure
extern TSODINFO *g_MemTSODInfo;

extern SMBIOS_SYSTEM_INFORMATION gSystemInfo;       // SMBIOS systeminformation
extern SMBIOS_BIOS_INFORMATION gBIOSInfo;           // SMBIOS BIOSinformation
extern SMBIOS_BASEBOARD_INFORMATION gBaseBoardInfo; // SMBIOS baseboardinformation

extern BLACKLIST gBlacklist;

extern void cpu_type(CHAR16 *szwCPUType, UINTN bufSize);
extern bool GetManNameFromJedecIDContCode(unsigned char jedecID, int numContCode, wchar_t *buffer, int maxBufSize);

int getval(int x, int y, int len, int decimalonly, UINTN *val);

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
VOID LogAllMtrrs(
    IN MTRR_SETTINGS *MtrrSetting);

//
// Lookup table used to print MTRRs
//
static CONST CHAR8 *mMtrrMemoryCacheTypeShortName[] = {
    "UC", // CacheUncacheable
    "WC", // CacheWriteCombining
    "R*", // Invalid
    "R*", // Invalid
    "WT", // CacheWriteThrough
    "WP", // CacheWriteProtected
    "WB", // CacheWriteBack
    "R*"  // Invalid
};

static CONST UINT8 CACHE_MODE_TO_MTRR_CACHE_TYPE[] = {CacheUncacheable, CacheWriteBack, CacheWriteCombining, CacheWriteThrough, CacheWriteProtected, CacheWriteBack};

static inline UINT64 Pow2Floor(UINT64 Val)
{
    Val--;
    Val |= Val >> 1;
    Val |= Val >> 2;
    Val |= Val >> 4;
    Val |= Val >> 8;
    Val |= Val >> 16;
    Val |= Val >> 32;
    Val++;
    Val >>= 1;
    return Val;
}
#endif

#if 0 // not used
extern
UINT64
EFIAPI
AsmAddrLatencyTest(
    IN VOID   *Addr1,
    IN VOID   *Addr2
);

VOID
MtSupportDetectAddressBits();
#endif

// RunOnAllMemory
//
// Run the specified callback function on all memory
STATIC
EFI_STATUS
EFIAPI
RunOnAllMemory(
    IN UINT32 TestNum,
    IN TEST_MEM_RANGE TestRangeFunction,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser);

// MtSupportTestTick
//
// Update the UI, check for keyboard presses and perform any necessary polling
VOID MtSupportTestTick(BOOLEAN UpdateErr)
{
    UINT64 CurTime = MtSupportGetTimeInMilliseconds();
    if (mPrevUpdateTime == 0 ||
        (CurTime - mPrevUpdateTime > UI_UPDATE_PERIODMS))
    {
        MtUiUpdateTime();
        MtUiUpdateCPUState();
        MtUiUpdatePattern(FALSE);
        if (UpdateErr)
            MtSupportUpdateErrorDisp();
#ifdef APDEBUG
        MtUiAPFlushBuf();
#endif
        mPrevUpdateTime = CurTime;
    }

    if (mPrevTimingsPollTime == 0 ||
        (CurTime - mPrevTimingsPollTime) > TIMINGS_POLL_MS)
    {
        unsigned int memclk;
        unsigned char cas, rcd, rp, ras;
        if (get_mem_ctrl_timings(&memclk, NULL, &cas, &rcd, &rp, &ras) == 0)
        {
            mCurMemTimings.SpeedMTs = memclk * 2;
            mCurMemTimings.tCAS = cas;
            mCurMemTimings.tRCD = rcd;
            mCurMemTimings.tRP = rp;
            mCurMemTimings.tRAS = ras;
            if (mCurMemTimings.SpeedMTs > mMaxMemTimings.SpeedMTs)
                CopyMem(&mMaxMemTimings, &mCurMemTimings, sizeof(mMaxMemTimings));
            if (mCurMemTimings.SpeedMTs < mMinMemTimings.SpeedMTs)
                CopyMem(&mMinMemTimings, &mCurMemTimings, sizeof(mMinMemTimings));

            AsciiFPrint(DEBUG_FILE_HANDLE, "Current mem timings: %d MT/s (%d-%d-%d-%d)", mCurMemTimings.SpeedMTs, mCurMemTimings.tCAS, mCurMemTimings.tRCD, mCurMemTimings.tRP, mCurMemTimings.tRAS);
        }
        mPrevTimingsPollTime = CurTime;
    }

    if (mPrevTempPollTime == 0 ||
        (CurTime - mPrevTempPollTime) > TEMP_POLL_MS)
    {
        if (gTSODPoll)
        {
#ifdef TSOD_DEBUG
            int NumTSOD = g_numSMBIOSMem > 4 ? 4 : g_numSMBIOSMem;

            for (int i = 0; i < NumTSOD; i++)
            {
                mCurRAMTemp[i] = (int)(rand(0) % 200000);
            }
#else
            int NumTSOD = MtSupportGetTSODInfo(mCurRAMTemp, sizeof(mCurRAMTemp) / sizeof(mCurRAMTemp[0]));
#endif
            for (int i = 0; i < NumTSOD && i < MAX_RAM_TEMP_DIMMS; i++)
            {
                int RAMTemp = mCurRAMTemp[i];
                // Update RAM temperature
                if (ABS(RAMTemp) < MAX_TSOD_TEMP)
                {
                    if (RAMTemp > mMaxRAMTemp[i])
                        mMaxRAMTemp[i] = RAMTemp;
                    if (RAMTemp < mMinRAMTemp[i])
                        mMinRAMTemp[i] = RAMTemp;
                    mSumRAMTemp[i] += RAMTemp;
                    mNumRAMTempSamples[i]++;

                    AsciiFPrint(DEBUG_FILE_HANDLE, "DIMM%d temperature: %d.%03dC", i, mCurRAMTemp[i] / 1000, ABS(mCurRAMTemp[i] % 1000));

                    if (i == 0)
                        MtUiUpdateRAMTemp(RAMTemp);
                }
            }
        }
        // Update CPU temperature
        MSR_TEMP CPUTemp;
        SetMem(&CPUTemp, sizeof(CPUTemp), 0);

        if (GetCPUTemp(&gCPUInfo, MPSupportGetBspId(), &CPUTemp) == EFI_SUCCESS)
        {
            mCurCPUTemp = CPUTemp.iTemperature;
            if (CPUTemp.iTemperature > mMaxCPUTemp)
                mMaxCPUTemp = CPUTemp.iTemperature;
            if (CPUTemp.iTemperature < mMinCPUTemp)
                mMinCPUTemp = CPUTemp.iTemperature;
            mSumCPUTemp += CPUTemp.iTemperature;
            mNumCPUTempSamples++;

            AsciiFPrint(DEBUG_FILE_HANDLE, "Current CPU temperature: %dC", mCurCPUTemp);

            MtUiUpdateCPUTemp(CPUTemp.iTemperature);
        }
        mPrevTempPollTime = CurTime;
    }

    if (mPrevKBPollTime == 0 ||
        (CurTime - mPrevKBPollTime > KEYBOARD_POLL_MS))
    {
        if (gBS->CheckEvent(gST->ConIn->WaitForKey) == EFI_SUCCESS)
        {
            EFI_INPUT_KEY Key;
            gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
            if (Key.UnicodeChar == L'c' || Key.ScanCode == 0x17)
                MtUiShowMenu();
        }
        mPrevKBPollTime = CurTime;
    }
#if 0
#ifdef SITE_EDITION
    if (mPrevPMPStatusTime == 0 ||
        ((CurTime - mPrevPMPStatusTime) > (gTFTPStatusSecs * 1000)))
    {
        EFI_STATUS Status = MtSupportPMPStatus();
        if (Status != EFI_SUCCESS)
        {
            CHAR16 TempBuf[128];
            MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_PMP), TempBuf, sizeof(TempBuf)), Status);
        }
        gBS->SetWatchdogTimer(0, 0, 0, NULL); // Workaround for iPXE that re-enables the watchdog timer, causing system to reset after 5 min. See http://forum.ipxe.org/showthread.php?tid=7553,
        mPrevPMPStatusTime = CurTime;
    }
#endif
#endif
    if (mPrevTestFlowPollTime == 0 || ((CurTime - mPrevTestFlowPollTime) > (500)))
    {   
        if (gOknTestPause) {
            while (1) {
                gBS->Stall(500000);
                if (!gOknTestPause) {
                    break;
                }
            }
        }
        mPrevTestFlowPollTime = CurTime;
    }
}

VOID MtSupportPollECC()
{
    if (gECCPoll)
    {
        if (mPrevEccPollTime == 0 ||
            (MtSupportGetTimeInMilliseconds() - mPrevEccPollTime) > ECC_POLL_MS)
        {
            /* Poll for ECC errors */
            poll_errors();

            mPrevEccPollTime = MtSupportGetTimeInMilliseconds();
        }
    }
}

// MtSupportCheckTestAbort
//
// Check whether the current test has been aborted
BOOLEAN
MtSupportCheckTestAbort()
{
    return (mAbortTesting || mSkipTest || (MtSupportGetNumErrors() >= gMaxErrCount));
}

int MtSupportGetCPUTemp()
{
    return mCurCPUTemp;
}

int MtSupportGetRAMTemp(int dimm)
{
    if (dimm >= MAX_RAM_TEMP_DIMMS)
        return MAX_TSOD_TEMP;

    return mCurRAMTemp[dimm];
}

// MtSupportDisplayHwErrRec
//
// Display the EFI hardware error records. Not used
VOID MtSupportDisplayHwErrRec()
{
    UINT32 Attr = 0;
    UINT16 HwErrRecSupportVal = 0;
    BOOLEAN HwErrRecSupport = FALSE;
    UINTN Size = sizeof(HwErrRecSupportVal);
    CHAR16 HwErrRecName[64];
    UINTN HwErrRecNameSize = sizeof(HwErrRecName);
    EFI_GUID HwErrRecGuid = gEfiHardwareErrorVariableGuid;

    EFI_STATUS Status;

    SetMem(HwErrRecName, sizeof(HwErrRecName), 0);

    // Check whether hardware error records are supported
    Status = gRT->GetVariable(L"HwErrRecSupport", &gEfiGlobalVariableGuid, &Attr, &Size, (void *)&HwErrRecSupportVal);
    if (Status == EFI_SUCCESS)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "HwErrRecSupport=%d", HwErrRecSupportVal);
        MtSupportDebugWriteLine(gBuffer);
        HwErrRecSupport = (HwErrRecSupportVal == 1);
    }
    else
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "GetVariable(HwErrRecSupport) failed - %r", Status);
        MtSupportDebugWriteLine(gBuffer);
        return;
    }

    while (1)
    {
        static unsigned char CPERBuf[256];
        UINTN CPERBufSize = 256;

        HwErrRecNameSize = sizeof(HwErrRecName);
        Status = gRT->GetNextVariableName(&HwErrRecNameSize, HwErrRecName, &HwErrRecGuid);
        if (EFI_ERROR(Status))
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "GetNextVariableName(%s) failed - %r", HwErrRecName, Status);
            MtSupportDebugWriteLine(gBuffer);
            if (Status == EFI_NOT_FOUND)
                break;
            else
                continue;
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "%s [%g]", HwErrRecName, &HwErrRecGuid);
        MtSupportDebugWriteLine(gBuffer);

        // Check if current variable is a hardware error variable
        if (CompareGuid(&HwErrRecGuid, &gEfiHardwareErrorVariableGuid))
        {
            // Rertrieve the variable
            Status = gRT->GetVariable(HwErrRecName, &HwErrRecGuid, &Attr, &CPERBufSize, (void *)CPERBuf);
            if (Status == EFI_SUCCESS)
            {
                EFI_COMMON_ERROR_RECORD_HEADER *CPERHeader = (EFI_COMMON_ERROR_RECORD_HEADER *)CPERBuf;

                // Check signature
                if (AsciiStrnCmp((CHAR8 *)(&CPERHeader->SignatureStart), "CPER", 4) == 0)
                {
                    int i = 0;
                    AsciiSPrint(gBuffer, BUF_SIZE, "[%s] ** Error Record Header **\r\n"
                                                   "Record Length - %d\r\n"
                                                   "Section Count - %d\r\n"
                                                   "Error Severity - %d\r\n"
                                                   "Timestamp - %d-%d-%d %d:%d:%d\r\n"
                                                   "Platform ID - %g\r\n"
                                                   "Partition ID - %g\r\n"
                                                   "Creator ID - %g\r\n"
                                                   "Notification Type - %g\r\n"
                                                   "Record ID - %ld\r\n"
                                                   "Flags - %08X\r\n"
                                                   "Persistence Info - %ld",
                                HwErrRecName,
                                CPERHeader->RecordLength,
                                CPERHeader->SectionCount,
                                CPERHeader->ErrorSeverity,
                                CPERHeader->TimeStamp.Year, CPERHeader->TimeStamp.Month, CPERHeader->TimeStamp.Day, CPERHeader->TimeStamp.Hours, CPERHeader->TimeStamp.Minutes, CPERHeader->TimeStamp.Seconds,
                                CPERHeader->PlatformID,
                                CPERHeader->PartitionID,
                                CPERHeader->CreatorID,
                                CPERHeader->NotificationType,
                                CPERHeader->RecordID,
                                CPERHeader->Flags,
                                CPERHeader->PersistenceInfo);

                    MtSupportDebugWriteLine(gBuffer);

                    for (i = 0; i < CPERHeader->SectionCount; i++)
                    {
                        CHAR16 TextBuf[128];
                        CHAR16 TempBuf[32];

                        EFI_ERROR_SECTION_DESCRIPTOR *SectDesc = (EFI_ERROR_SECTION_DESCRIPTOR *)(CPERBuf + sizeof(EFI_COMMON_ERROR_RECORD_HEADER) + sizeof(EFI_ERROR_SECTION_DESCRIPTOR) * i);
                        EFI_GUID SectionGuid;
                        CopyMem(&SectionGuid, &SectDesc->SectionType, sizeof(SectionGuid));

                        AsciiSPrint(gBuffer, BUF_SIZE, "[%s] ** Section Descriptor **\r\n"
                                                       "Section Offset - %d\r\n"
                                                       "Section Length - %d\r\n"
                                                       "Validation Bits - %02X\r\n"
                                                       "Flags - %08X\r\n"
                                                       "Section Type - %g\r\n"
                                                       "FRU ID - %g\r\n"
                                                       "Section Severity - %d\r\n"
                                                       "FRU Text - %a\r\n",
                                    HwErrRecName,
                                    SectDesc->SectionOffset,
                                    SectDesc->SectionLength,
                                    SectDesc->SecValidMask,
                                    SectDesc->SectionFlags,
                                    SectDesc->SectionType,
                                    SectDesc->FruId,
                                    SectDesc->Severity,
                                    SectDesc->FruString);

                        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d-%d-%d %d:%d:%d",
                                      CPERHeader->TimeStamp.Year, CPERHeader->TimeStamp.Month, CPERHeader->TimeStamp.Day, CPERHeader->TimeStamp.Hours, CPERHeader->TimeStamp.Minutes, CPERHeader->TimeStamp.Seconds);

                        if (CompareGuid(&SectionGuid, &gEfiProcessorGenericErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [Processor]");
                        else if (CompareGuid(&SectionGuid, &gEfiProcessorSpecificErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [Processor]");
                        else if (CompareGuid(&SectionGuid, &gEfiPcieErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [PCIe]");
                        else if (CompareGuid(&SectionGuid, &gEfiFirmwareErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [Firmware]");
                        else if (CompareGuid(&SectionGuid, &gEfiPciBusErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [PCI Bus]");
                        else if (CompareGuid(&SectionGuid, &gEfiPciDevErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [PCI Device]");
                        else if (CompareGuid(&SectionGuid, &gEfiDMArGenericErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [DMAr]");
                        else if (CompareGuid(&SectionGuid, &gEfiDirectedIoDMArErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [DMAr Directed Io]");
                        else if (CompareGuid(&SectionGuid, &gEfiIommuDMArErrorSectionGuid))
                            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" [DMAr IOMMU]");
                        else if (CompareGuid(&SectionGuid, &gEfiPlatformMemoryErrorSectionGuid))
                        {
                            STATIC CHAR16 ERRTYPE_STR[][32] =
                                {
                                    L"Unknown",
                                    L"None",
                                    L"ECC SBE",
                                    L"ECC MBE",
                                    L"Chipkill SS",
                                    L"Chipkill MS",
                                    L"Master abort",
                                    L"Target abort",
                                    L"Parity",
                                    L"Watchdog",
                                    L"Invalid address",
                                    L"Mirror broken",
                                    L"Memory sparing",
                                    L"Scrub corrected",
                                    L"Scrub uncorrected",
                                    L"Phys mem map-out",
                                };

                            EFI_PLATFORM_MEMORY_ERROR_DATA *MemError = (EFI_PLATFORM_MEMORY_ERROR_DATA *)(CPERBuf + SectDesc->SectionOffset);

                            UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d-%d-%d %d:%d:%d [Memory]\n",
                                          CPERHeader->TimeStamp.Year, CPERHeader->TimeStamp.Month, CPERHeader->TimeStamp.Day, CPERHeader->TimeStamp.Hours, CPERHeader->TimeStamp.Minutes, CPERHeader->TimeStamp.Seconds);

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_ERROR_TYPE_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), ERRTYPE_STR[MemError->ErrorType]);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_PHY_ADDRESS_VALID)
                            {
                                UINT64 Mask = (UINT64)-1;

                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                if (MemError->ValidFields & EFI_PLATFORM_MEMORY_PHY_ADDRESS_MASK_VALID)
                                    Mask = MemError->PhysicalAddressMask;

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%p", MemError->PhysicalAddress & Mask);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_NODE_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", MemError->Node);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_CARD_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", MemError->Card);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_MODULE_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", MemError->ModuleRank);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_BANK_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", MemError->Bank);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_DEVICE_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", MemError->Device);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_ROW_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", MemError->Row);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            if (MemError->ValidFields & EFI_PLATFORM_MEMORY_COLUMN_VALID)
                            {
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" ");

                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", MemError->Column);
                                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
                            }

                            AsciiSPrint(gBuffer, BUF_SIZE, "[%s] ** Memory Error Section **\r\n"
                                                           "Validation Bits - %016lx\r\n"
                                                           "Error Status.Type - %d\r\n"
                                                           "Physical Address - %016lx\r\n"
                                                           "Physical Address Mask - %016lx\r\n"
                                                           "Node - %d\r\n"
                                                           "Card - %d\r\n"
                                                           "Module - %d\r\n"
                                                           "Bank - %d\r\n"
                                                           "Device - %d\r\n"
                                                           "Row - %d\r\n"
                                                           "Column - %d\r\n"
                                                           "Bit Position - %d\r\n"
                                                           "Requestor ID - %016lx\r\n"
                                                           "Responder ID - %016lx\r\n"
                                                           "Target ID - %016lx\r\n"
                                                           "Memory Error Type - %d\r\n"
                                                           "Rank - %d\r\n"
                                                           "Card handle - %d\r\n"
                                                           "Module handle - %d\r\n",
                                        HwErrRecName,
                                        MemError->ValidFields,
                                        MemError->ErrorStatus.Type,
                                        MemError->PhysicalAddress,
                                        MemError->PhysicalAddressMask,
                                        MemError->Node,
                                        MemError->Card,
                                        MemError->ModuleRank,
                                        MemError->Bank,
                                        MemError->Device,
                                        MemError->Row,
                                        MemError->Column,
                                        MemError->BitPosition,
                                        MemError->RequestorId,
                                        MemError->ResponderId,
                                        MemError->TargetId,
                                        MemError->ErrorType,
                                        MemError->RankNum,
                                        MemError->CardHandle,
                                        MemError->ModuleHandle);
                        }
                        Print(TextBuf);
                        Print(L"\n");
                    }
                }
            }
            else
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "GetVariable(%s) failed - %r", HwErrRecName, Status);
                MtSupportDebugWriteLine(gBuffer);
            }
        }
    }
}

// LockAllMemRanges
//
// Run a generic, single-staged memory test
VOID
    EFIAPI
    LockAllMemRanges(
        VOID)
{
    INTN Key = 0;
    UINT64 AvailSize = MtRangesGetTotalSize();
    UINT64 MinAvailSize = MultU64x32(gMemRemMB, SIZE_1MB);
    UINT64 MinRangeSize = MultU64x32(gMinMemRangeMB, SIZE_1MB);

    mTestedMemSize = 0;

    // Lock memory first
    MtSupportDebugWriteLine("Locking all memory ranges first...");

    while (TRUE)
    {
        EFI_STATUS Status;
        EFI_PHYSICAL_ADDRESS Start;
        UINT64 Length;

        Status = MtRangesGetNextRange(
            &Key,
            &Start,
            &Length);
        if (Status == EFI_NOT_FOUND)
        {
            break;
        }

        if (Start + Length <= gAddrLimitLo)
            continue;

        if (Start >= gAddrLimitHi)
            continue;

        // Reduce range if Start + Length > gAddrLimitHi
        if (Start + Length > gAddrLimitHi)
        {
            UINT64 NewLength = gAddrLimitHi - Start;

            AsciiSPrint(gBuffer, BUF_SIZE, "Range is greater than max (0x%p). Reducing memory range from 0x%lX - 0x%lX (%dKB) => 0x%lX - 0x%lX (%dKB) ", gAddrLimitHi, Start, Start + Length, DivU64x32(Length, SIZE_1KB), Start, Start + NewLength, DivU64x32(NewLength, SIZE_1KB));
            MtSupportDebugWriteLine(gBuffer);

            Length = NewLength;
        }

        // Reduce range if remaining is less than minimum available size
        if ((AvailSize - Length) < MinAvailSize)
        {
            UINT64 NewLength = AvailSize > MinAvailSize ? AvailSize - MinAvailSize : 0;

            AsciiSPrint(gBuffer, BUF_SIZE, "Remaining memory is less than %ldMB. Reducing memory range from 0x%lX - 0x%lX (%dKB) => 0x%lX - 0x%lX (%dKB) ", gMemRemMB, Start, Start + Length, DivU64x32(Length, SIZE_1KB), Start, Start + NewLength, DivU64x32(NewLength, SIZE_1KB));
            MtSupportDebugWriteLine(gBuffer);

            Length = NewLength;
        }

        // Skip ranges less than MINRANGEMB. Maybe conflicting with drivers?
        // See http://holly/cerb5/index.php/display/XVA-81116-572
        // https://www.passmark.com/forum/memtest86/42571-trouble-with-12-cores-xeon-processor
        if (Length <= MinRangeSize)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Skipping memory range 0x%lX - 0x%lX (%dKB). Range too small.", Start, Start + Length, DivU64x32(Length, SIZE_1KB));
            MtSupportDebugWriteLine(gBuffer);
            continue;
        }

        Status = MtRangesLockRange(Start, Length);
        if (EFI_ERROR(Status))
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not lock memory range 0x%lX - 0x%lX (%r) (%dKB of available memory left)", Start, Start + Length, Status, DivU64x32(AvailSize, SIZE_1KB));
            MtSupportDebugWriteLine(gBuffer);
        }
        else
        {
            EFI_PHYSICAL_ADDRESS Start2;
            UINT64 Length2;

            AvailSize -= Length;

            AsciiSPrint(gBuffer, BUF_SIZE, "Memory range locked: 0x%lX - 0x%lX (%dKB of available memory left)", Start, Start + Length, DivU64x32(AvailSize, SIZE_1KB));
            MtSupportDebugWriteLine(gBuffer);

            // Add up the total size of memory to be tested (for progress bar)
            if (Start < gAddrLimitLo)
                Start2 = gAddrLimitLo;
            else
                Start2 = Start;

            if (Start + Length > gAddrLimitHi)
                Length2 = gAddrLimitHi - Start2;
            else
                Length2 = Start + Length - Start2;

            mTestedMemSize += Length2;
        }
    }
    MtSupportDebugWriteLine("All memory ranges successfully locked");
}

STATIC
BOOLEAN
EFIAPI
GetNextLockedRange(
    OUT EFI_PHYSICAL_ADDRESS *Start,
    OUT UINT64 *Length)
{
    UINT64 AvailSize = MtRangesGetTotalSize();
    UINT64 MinAvailSize = MultU64x32(gMemRemMB, SIZE_1MB);

    INTN CurKey = 0;

    while (TRUE)
    {

        EFI_STATUS Status;
        EFI_PHYSICAL_ADDRESS CurStart;
        UINT64 CurLength;

        Status = MtRangesGetNextRange(
            &CurKey,
            &CurStart,
            &CurLength);

        if (Status == EFI_NOT_FOUND)
        {
            return FALSE;
        }

        if (CurStart + CurLength <= gAddrLimitLo)
            continue;

        if (CurStart >= gAddrLimitHi)
            continue;

        // Reduce range if Start + Length > gAddrLimitHi
        if (CurStart + CurLength > gAddrLimitHi)
        {
            UINT64 NewLength = gAddrLimitHi - CurStart;

            CurLength = NewLength;
        }

        // Reduce range if remaining is less than minimum available size
        if ((AvailSize - CurLength) < MinAvailSize)
        {
            UINT64 NewLength = AvailSize > MinAvailSize ? AvailSize - MinAvailSize : 0;

            CurLength = NewLength;
        }

        if (MtRangesIsRangeLocked(CurStart, CurLength))
        {
            if (CurStart >= *Start)
            {
                *Start = CurStart;
                *Length = CurLength;
                return TRUE;
            }
            AvailSize -= CurLength;
        }
    }
    return FALSE;
}

VOID
    EFIAPI
    UnlockAllMemRanges(
        VOID)
{
    EFI_PHYSICAL_ADDRESS Start = 0;
    UINT64 Length = 0;

    MtSupportDebugWriteLine("Cleanup - Unlocking all memory ranges...");

    while (TRUE)
    {
        if (GetNextLockedRange(
                &Start,
                &Length) == FALSE)
            break;

        MtRangesUnlockRange(Start, Length);
    }

    MtSupportDebugWriteLine("All memory ranges successfully unlocked");

    mTestedMemSize = 0;
}

// RunMemoryRangeTest
//
// Run a generic, single-staged memory test
STATIC
EFI_STATUS
EFIAPI
RunMemoryRangeTest(
    IN VOID *Context)
{
    EFI_STATUS Status;
    EFI_STATUS TestResult;
    BOOLEAN AbortTest = FALSE;
    EFI_PHYSICAL_ADDRESS Start;
    UINT64 Length;
    EFI_PHYSICAL_ADDRESS CurStart;
    UINT64 LengthTested;
    UINT64 SubRangeLength;

    UINT64 TotalLengthTested;
    UINT64 DebugLenTested = 0;

    MEM_RANGE_TEST_DATA *Test = (MEM_RANGE_TEST_DATA *)Context;
    UINT32 NumProcs = MIN(Test->MaxCPUs, (UINT32)MPSupportGetMaxProcessors());
    UINT32 NumProcsEn = MIN(Test->MaxCPUs, (UINT32)MPSupportGetNumEnabledProcessors());

    CHAR16 TempBuf[128];

    //  UINT32              MTRRNum = 0;

    AsciiSPrint(gBuffer, BUF_SIZE, "Start memory range test (0x%lX - 0x%lX)", gAddrLimitLo, gAddrLimitHi);
    MtSupportDebugWriteLine(gBuffer);

    // MtUiSetIterationCount(Test->IterCount);
    MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);
    MtUiUpdateCPUState();

    MtUiSetProgressTotal(mTestedMemSize);
    MtUiUpdateProgress(0);
#if 0
    if (!gEnableCache)
    {
        MTRRNum = FindFirstUnusedMTRR();
        AsciiSPrint(gBuffer, BUF_SIZE, "First unused MTRR: 0x%x", MTRRNum);
        MtSupportDebugWriteLine(gBuffer);
    }
#endif

    CACHEMODE CacheMode = mTests[mCurTestIdx].CacheMode == CACHEMODE_DEFAULT ? gCacheMode : (BOOLEAN)mTests[mCurTestIdx].CacheMode;

    mMtrrValidBitsMask = 0;
    mMtrrValidAddressMask = 0;
    mFreeMtrrInd = -1;

    (VOID) mCurMtrrs;
    (VOID) mPrevMtrrs;

    if (CacheMode != CACHEMODE_DEFAULT)
    {
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
        if (cpu_id.fid.bits.mtrr)
        {
            MtrrLibInitializeMtrrMask(&mMtrrValidBitsMask, &mMtrrValidAddressMask);
            MtrrGetAllMtrrs(&mCurMtrrs);
            CopyMem(&mPrevMtrrs, &mCurMtrrs, sizeof(mPrevMtrrs));

            for (int Index = 0; Index < ARRAY_SIZE(mCurMtrrs.Variables.Mtrr); Index++)
            {
                if ((mCurMtrrs.Variables.Mtrr[Index].Mask & BIT11) == 0)
                {
                    mFreeMtrrInd = Index;
                    break;
                }
            }

            if (mFreeMtrrInd >= 0 && mFreeMtrrInd < (int)GetVariableMtrrCount())
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Found free MTRR variable register @ %d", mFreeMtrrInd);
            }
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "No free MTRR variable register available. Unable to set cache mode");
                mFreeMtrrInd = -1;
            }
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Disabling memory cache");
            SyncEnableCacheAllAPs(FALSE);
        }
#else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Disabling memory cache");
        SyncEnableCacheAllAPs(FALSE);
#endif
    }

    // Now go through all available memory ranges and run the test
    Status = EFI_SUCCESS;
    TestResult = EFI_SUCCESS;
    Start = 0;
    Length = 0;
    TotalLengthTested = 0;
    while (TRUE)
    {
        if (GetNextLockedRange(
                &Start,
                &Length) == FALSE)
            break;

        if (Start + Length > gAddrLimitLo && Start < gAddrLimitHi)
        {
            // Update the UI with the address range to test
            MtUiSetAddressRange(Start >= gAddrLimitLo ? Start : gAddrLimitLo,
                                Start + Length <= gAddrLimitHi ? Start + Length : gAddrLimitHi);
#if 0
            if (!gEnableCache)
            {
                if (ResetCacheAttributes(MTRRNum) != EFI_SUCCESS)
                {
                    MtSupportDebugWriteLine("Reset cache attributes failed");
                }
                if (SetCacheAttributes(Start, Length, EFI_CACHE_UNCACHEABLE, MTRRNum) != EFI_SUCCESS)
                {
                    MtSupportDebugWriteLine("Set cache attributes failed");
                }
                TraverseMTRR();
            }
#endif

            // Divide each memory range into smaller subranges
            CurStart = Start;
            LengthTested = 0;
            while (LengthTested < Length)
            {
                MtSupportTestTick(FALSE);
                MtSupportPollECC();

                if (MtSupportCheckTestAbort())
                {
                    TestResult = EFI_ABORTED;
                    AbortTest = TRUE;
                    break;
                }

                if (EFI_ERROR(TestResult) && TestResult != EFI_TEST_FAILED)
                {
                    AbortTest = TRUE;
                    break;
                }

                // Use smaller segment size when cache is disabled for better UI responsiveness
                const UINT64 CPUSegSize = CacheMode != CACHEMODE_DEFAULT ? SIZE_8MB : SIZE_64MB;
                if (gCPUSelMode == CPU_PARALLEL)
                    SubRangeLength = MIN(MultU64x32(CPUSegSize, NumProcsEn), Length - LengthTested);
                else
                    SubRangeLength = MIN(CPUSegSize, Length - LengthTested);

                if (CurStart + SubRangeLength > gAddrLimitLo && CurStart < gAddrLimitHi)
                {
                    EFI_PHYSICAL_ADDRESS Start2;
                    UINT64 SubRangeLength2;

                    EFI_STATUS BSPResult;
                    EFI_EVENT APEvent[MAX_CPUS];
                    UINT64 BSPTestTime;
                    UINT64 APWaitTime[MAX_CPUS];
                    BOOLEAN bAPFinished = TRUE;
                    UINTN ProcNum;
                    UINT64 WaitStartTime = 0;
                    UINT64 CheckPointTime = 0;
                    UINT64 AbortStartTime = 0;
                    BOOLEAN ErrUpdated = FALSE;

                    if (CurStart < gAddrLimitLo)
                        Start2 = gAddrLimitLo;
                    else
                        Start2 = CurStart;

                    if (CurStart + SubRangeLength > gAddrLimitHi)
                        SubRangeLength2 = gAddrLimitHi - Start2;
                    else
                        SubRangeLength2 = CurStart + SubRangeLength - Start2;

                    if (gVerbosity > 0)
                    {
                        // Print the current progress every ~256MB
                        if (DebugLenTested == 0)
                        {
                            UINT64 End = Start2 + SIZE_256MB;
                            if (End > Start + Length)
                                End = Start + Length;
                            if (End > gAddrLimitHi)
                                End = gAddrLimitHi;
                            AsciiSPrint(gBuffer, BUF_SIZE, "Testing memory ranges: 0x%p - 0x%p (%ldMB)", Start2, End, DivU64x32((End - Start2), SIZE_1MB));
                            MtSupportDebugWriteLine(gBuffer);
                        }
                        DebugLenTested += SubRangeLength2;
                        if (DebugLenTested >= SIZE_256MB)
                            DebugLenTested = 0;
                    }
                    SetMem(APEvent, sizeof(APEvent), 0);
                    SetMem(APWaitTime, sizeof(APWaitTime), 0);

                    // If sequential CPU mode, check whether we have tested >= 256MB. If so, switch to next processor
                    if (gCPUSelMode == CPU_SEQUENTIAL)
                    {
                        if (mProcLenTested >= SIZE_256MB)
                        {
                            do
                            {
                                mSelProcNum = (mSelProcNum + 1) % NumProcs;
                            } while (MPSupportIsProcEnabled(mSelProcNum) == FALSE);

                            // Only 1 CPU is running at one time. Set it as the BSP
                            if (mSelProcNum != MPSupportGetBspId())
                            {
                                MtSupportSetCPURunning(MPSupportGetBspId(), FALSE);

                                AsciiSPrint(gBuffer, BUF_SIZE, "RunMemoryRangeTest - Switching BSP from Proc# %d --> %d", MPSupportGetBspId(), mSelProcNum);
                                MtSupportDebugWriteLine(gBuffer);

                                Status = MPSupportSwitchBSP(mSelProcNum);
                                if (EFI_ERROR(Status))
                                {
                                    AsciiSPrint(gBuffer, BUF_SIZE, "RunMemoryRangeTest - Could not switch BSP to Proc#%d (%r).", mSelProcNum, Status);
                                    MtSupportDebugWriteLine(gBuffer);
                                    MtSupportReportCPUError(mTests[mCurTestIdx].TestNo, mSelProcNum);

                                    // Report to user
                                    if (mCPUErrReported == FALSE)
                                        MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_CPU_START), TempBuf, sizeof(TempBuf)), mSelProcNum);
                                    mCPUErrReported = TRUE;
                                }
                            }
                            mProcLenTested = 0;
                        }
                        mProcLenTested += SubRangeLength2;
                    }

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
                    if (CacheMode != CACHEMODE_DEFAULT)
                    {
                        if (cpu_id.fid.bits.mtrr)
                        {
                            if (mFreeMtrrInd >= 0)
                            {
                                UINT64 Base = mCurMtrrs.Variables.Mtrr[mFreeMtrrInd].Base & mMtrrValidAddressMask;
                                UINT64 Mask = mCurMtrrs.Variables.Mtrr[mFreeMtrrInd].Mask & mMtrrValidAddressMask;

                                if ((mCurMtrrs.Variables.Mtrr[mFreeMtrrInd].Mask & BIT11) == 0 ||
                                    (CurStart & Mask) != (Base & Mask))
                                {
                                    // Get largest power of 2 no larger than the size of remaining memory to be tested
                                    UINT64 Align = Pow2Floor(Length - LengthTested);

                                    // Limit MTRR range to 1GB
                                    Align = Align > SIZE_1GB ? SIZE_1GB : Align;
                                    mCurMtrrs.Variables.Mtrr[mFreeMtrrInd].Base = ((CurStart & ~(Align - 1)) & mMtrrValidAddressMask) | CACHE_MODE_TO_MTRR_CACHE_TYPE[CacheMode];
                                    mCurMtrrs.Variables.Mtrr[mFreeMtrrInd].Mask = (~(Align - 1) & mMtrrValidAddressMask) | BIT11;

                                    AsciiFPrint(DEBUG_FILE_HANDLE, "Setting MTTR[%d] to: Base: %016lx Mask: %016lx Type:%d (CurStart: %lx Align: %lx)", mFreeMtrrInd, mCurMtrrs.Variables.Mtrr[mFreeMtrrInd].Base, mCurMtrrs.Variables.Mtrr[mFreeMtrrInd].Mask, CACHE_MODE_TO_MTRR_CACHE_TYPE[CacheMode], CurStart, Align);
                                    if (NumProcsEn > 1)
                                        SyncSetAllMtrrsAllProcs(&mCurMtrrs);
                                    else
                                        MtrrSetAllMtrrs(&mCurMtrrs);
                                }
                            }
                        }
                    }
#endif

                    if (gCPUSelMode == CPU_PARALLEL)
                    {
                        // Divide the memory range amongst each CPU
                        UINT64 CurCPUStart = (Start2 + (Test->Align - 1)) & ~((UINT64)Test->Align - 1);
                        UINT64 CPUMemRangeLen = DivU64x32(Start2 + SubRangeLength2 - CurCPUStart, NumProcsEn) & ~((UINT64)Test->Align - 1);

                        // If the range per CPU is too small, run only on BSP processor
                        if (CPUMemRangeLen > SIZE_64KB)
                        {
                            UINTN i;
                            // We are running in parallel, start the AP's first
                            for (i = 0; i < NumProcs; i++)
                            {
                                // Allocate memory ranges 1 -> n for odd passes, n -> 1 for even passes
                                ProcNum = mCurPass % 2 == 1 ? i : NumProcs - i - 1;
                                if (ProcNum != MPSupportGetBspId() &&
                                    MPSupportIsProcEnabled(ProcNum))
                                {
                                    Status = MPSupportStartMemTestAP(Test->TestNum, ProcNum, &APEvent[ProcNum], Test->RangeTest, CurCPUStart, CPUMemRangeLen, mCurPass == 1 && gPass1Full == FALSE ? DivU64x32(Test->IterCount, 3) : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);
                                    if (Status == EFI_SUCCESS)
                                    {
                                        CurCPUStart += CPUMemRangeLen;
                                    }
                                    else
                                    {
                                        AsciiSPrint(gBuffer, BUF_SIZE, "RunMemoryRangeTest - Could not start AP#%d 0x%016lx - 0x%016lx (%r).", ProcNum, CurCPUStart, CurCPUStart + CPUMemRangeLen, Status);
                                        MtSupportDebugWriteLine(gBuffer);
                                        MtSupportReportCPUError(mTests[mCurTestIdx].TestNo, ProcNum);

                                        // Report to user
                                        if (mCPUErrReported == FALSE)
                                            MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_CPU_START), TempBuf, sizeof(TempBuf)), ProcNum);
                                        mCPUErrReported = TRUE;
                                    }
                                }
                            }
                        }

                        MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);

                        // Finally, run test on BSP on any remaining segments
                        BSPTestTime = MtSupportGetTimeInMilliseconds();
                        BSPResult = Test->RangeTest(Test->TestNum, MPSupportGetBspId(), CurCPUStart, (Start2 + SubRangeLength2 - CurCPUStart) & ~((UINT64)Test->Align - 1), mCurPass == 1 && gPass1Full == FALSE ? DivU64x32(Test->IterCount, 3) : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);
                        BSPTestTime = MtSupportGetTimeInMilliseconds() - BSPTestTime;
                    }
                    else
                    {
                        UINT64 AlignedStart = (Start2 + (Test->Align - 1)) & ~((UINT64)Test->Align - 1);
                        UINT64 AlignedLen = (Start2 + SubRangeLength2 - AlignedStart) & ~((UINT64)Test->Align - 1);

                        MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);

                        // Finally, run test on BSP
                        BSPTestTime = MtSupportGetTimeInMilliseconds();
                        BSPResult = Test->RangeTest(Test->TestNum, MPSupportGetBspId(), AlignedStart, AlignedLen, mCurPass == 1 && gPass1Full == FALSE ? DivU64x32(Test->IterCount, 3) : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);
                        BSPTestTime = MtSupportGetTimeInMilliseconds() - BSPTestTime;
                    }

                    // Now check the result
                    WaitStartTime = MtSupportGetTimeInMilliseconds();
                    CheckPointTime = 10000;
                    ErrUpdated = FALSE;
                    do
                    {
                        bAPFinished = TRUE;

                        UINT64 WaitedMs = MtSupportGetTimeInMilliseconds() - WaitStartTime;

                        // Check the result for all processors, if necessary
                        for (ProcNum = 0; ProcNum < NumProcs; ProcNum++)
                        {
                            if (ProcNum == MPSupportGetBspId()) // BSP result
                            {
                                TestResult |= BSPResult;
                            }
                            else
                            {
                                if (APEvent[ProcNum]) // If parallel mode, APEvent will be non-NULL
                                {
                                    Status = gBS->CheckEvent(APEvent[ProcNum]);

                                    if (Status == EFI_NOT_READY) // CPU finished event has not been signalled
                                    {
                                        if (MPSupportGetMemTestAPFinished(ProcNum) != FALSE) // CPU has finished the memory test, event should be signalled so check again next iteration
                                        {
                                            if (APWaitTime[ProcNum] == 0)
                                                APWaitTime[ProcNum] = MtSupportGetTimeInMilliseconds();

                                            if ((MtSupportGetTimeInMilliseconds() - APWaitTime[ProcNum]) > AP_SIGNAL_WAIT_TIME) // Event still hasn't been signalled after 1 second
                                            {
                                                AsciiSPrint(gBuffer, BUF_SIZE, "RunMemoryRangeTest - CPU #%d completed but did not signal (test time = %ldms, event wait time = %ldms, result = %r) (BSP test time = %ldms)", ProcNum, MPSupportGetMemTestAPTestTime(ProcNum), MtSupportGetTimeInMilliseconds() - APWaitTime[ProcNum], MPSupportGetMemTestAPResult(ProcNum), BSPTestTime);
                                                MtSupportDebugWriteLine(gBuffer);

                                                AsciiSPrint(gBuffer, BUF_SIZE, "WARNING - possible multiprocessing bug in BIOS");
                                                MtSupportDebugWriteLine(gBuffer);

                                                TestResult |= MPSupportGetMemTestAPResult(ProcNum);
                                                gBS->CloseEvent(APEvent[ProcNum]);
                                                APEvent[ProcNum] = NULL;
                                            }
                                            else // Finished, but event hasn't been triggered. Wait at most 1 second to be triggered
                                            {
                                                bAPFinished = FALSE;
                                            }
                                        }
                                        else // CPU has not finished the memory test
                                        {
                                            if (WaitedMs > CheckPointTime)
                                            {
                                                AsciiSPrint(gBuffer, BUF_SIZE, "WARNING - waited for %lds for CPU #%d to finish (BSP test time = %ldms)", DivU64x32(WaitedMs, 1000), ProcNum, BSPTestTime);
                                                MtSupportDebugWriteLine(gBuffer);

                                                if (ErrUpdated)
                                                {
                                                    CheckPointTime = WaitedMs + 10000;
                                                    AsciiSPrint(gBuffer, BUF_SIZE, "RunMemoryRangeTest - increasing wait time to %lds due to detected memory errors", DivU64x32(CheckPointTime, 1000));
                                                    MtSupportDebugWriteLine(gBuffer);
                                                }
                                            }

                                            if (WaitedMs > BSPTestTime * BSP_WAIT_TIME_FACTOR &&
                                                WaitedMs > CheckPointTime)
                                            {
                                                AsciiSPrint(gBuffer, BUF_SIZE, "RunMemoryRangeTest - aborted waiting for CPU #%d to finish (Wait time = %ldms, BSP test time = %ldms)", ProcNum, WaitedMs, BSPTestTime);
                                                MtSupportDebugWriteLine(gBuffer);

                                                TestResult |= MPSupportGetMemTestAPResult(ProcNum);
                                                gBS->CloseEvent(APEvent[ProcNum]);
                                                APEvent[ProcNum] = NULL;
                                            }
                                            else
                                                bAPFinished = FALSE;
                                        }
                                    }
                                    else if (Status == EFI_SUCCESS) // CPU finished event has been signalled
                                    {
                                        if (MPSupportGetMemTestAPFinished(ProcNum) == FALSE) // CPU has not finished the memory test, maybe timed out or serious error?
                                        {
                                            AsciiSPrint(gBuffer, BUF_SIZE, "RunMemoryRangeTest - CPU #%d timed out, test time = %ldms (BSP test time = %ldms)", ProcNum, MPSupportGetMemTestAPTestTime(ProcNum), BSPTestTime);
                                            MtSupportDebugWriteLine(gBuffer);
                                        }

                                        TestResult |= MPSupportGetMemTestAPResult(ProcNum);
                                        gBS->CloseEvent(APEvent[ProcNum]);
                                        APEvent[ProcNum] = NULL;
                                    }
                                    else
                                    {
                                        AsciiSPrint(gBuffer, BUF_SIZE, "WARNING - Unexpected error waiting for CPU #%d to finish: %r", ProcNum, Status);
                                        MtSupportDebugWriteLine(gBuffer);
                                    }
                                }
                            }
                        }

                        if (WaitedMs > CheckPointTime)
                            CheckPointTime += 10000;

                        MtSupportTestTick(FALSE);
                        ErrUpdated = MtSupportUpdateErrorDisp();

                        // If not finished, check if aborted
                        if (bAPFinished == FALSE && MtSupportCheckTestAbort())
                        {
                            // We wait at most 5 seconds for AP's to finish
                            if (AbortStartTime == 0)
                                AbortStartTime = MtSupportGetTimeInMilliseconds();
                            else if (MtSupportGetTimeInMilliseconds() - AbortStartTime > 5000)
                            {
                                bAPFinished = TRUE;
                                // Clean up event handle
                                for (ProcNum = 0; ProcNum < NumProcs; ProcNum++)
                                {
                                    if (APEvent[ProcNum])
                                    {
                                        gBS->CloseEvent(APEvent[ProcNum]);
                                        APEvent[ProcNum] = NULL;

                                        AsciiSPrint(gBuffer, BUF_SIZE, "CPU #%d did not abort properly.", ProcNum);
                                        MtSupportDebugWriteLine(gBuffer);
                                    }
                                }
                            }
                        }
                    } while (bAPFinished == FALSE);
                    MtSupportPollECC();
                    MtSupportUpdateErrorDisp();

                    TotalLengthTested += SubRangeLength2;
                    MtUiUpdateProgress(TotalLengthTested);
                }
                CurStart += SubRangeLength;
                LengthTested += SubRangeLength;
            }
        }
        if (AbortTest)
            break;

        Start += Length;
    }

    if (CacheMode != CACHEMODE_DEFAULT)
    {
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
        if (cpu_id.fid.bits.mtrr)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Restoring MTRR registers to previous value");
            SyncSetAllMtrrsAllProcs(&mPrevMtrrs);
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Enabling memory cache");
            SyncEnableCacheAllAPs(TRUE);
        }
#else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Enabling memory cache");
        SyncEnableCacheAllAPs(TRUE);
#endif
    }

#if 0
    if (!gEnableCache)
    {
        if (ResetCacheAttributes(MTRRNum) != EFI_SUCCESS)
        {
            MtSupportDebugWriteLine("Reset cache attributes failed");
        }
    }
#endif
    return TestResult;
}

// RunBitFadeTest
//
// Run a two-staged bit fade memory test
STATIC
EFI_STATUS
EFIAPI
RunBitFadeTest(
    IN VOID *Context)
{
    EFI_STATUS Status;
    EFI_STATUS TestResult;
    BIT_FADE_TEST_DATA *Test;
    EFI_PHYSICAL_ADDRESS Start;
    UINT64 Length;
    UINT64 LengthTested;
    UINT64 TotalLengthTested;
    UINT64 DebugLenTested = 0;
    UINT64 SubRangeLength;
    UINT32 Pattern;
    CHAR16 TempBuf[128];
    int Pass;

    Test = (BIT_FADE_TEST_DATA *)Context;

    AsciiSPrint(gBuffer, BUF_SIZE, "Start bit fade test (0x%lX - 0x%lX)", gAddrLimitLo, gAddrLimitHi);
    MtSupportDebugWriteLine(gBuffer);

    // MtUiSetIterationCount(Test->IterCount);
    MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);
    MtUiUpdateCPUState();

    Status = EFI_SUCCESS;
    TestResult = EFI_SUCCESS;

    if (Test->Pattern == PAT_RAND)
        Pattern = rand((int)MPSupportGetBspId());
    else
        Pattern = (UINT32)(UINTN)Test->PatternUser;

    // Set the progress bar to be the 4 x memory size to be tested because we are going through all memory 4 times
    // Pattern 00000000 - Writing (25%), Verifying (25%)
    // Pattern FFFFFFFF - Writing (25%), Verifying (25%)
    MtUiSetProgressTotal(MultU64x32(mTestedMemSize, 4));
    MtUiUpdateProgress(0);

    TotalLengthTested = 0;
    for (Pass = 0; Pass < 2; Pass++)
    {
        UINTN Stage;
        // Stage 0 - Writing/Sleeping
        // Stage 1 - Verifying
        for (Stage = 0; Stage < 2; Stage++)
        {
            Start = 0;
            Length = 0;

            MtUiSetTestName(mTests[mCurTestIdx].NameStr ? mTests[mCurTestIdx].NameStr : GetStringById(mTests[mCurTestIdx].NameStrId, TempBuf, sizeof(TempBuf)));

            AsciiSPrint(gBuffer, BUF_SIZE, "Starting Stage %d, Pattern=%08X", Stage, Pattern);
            MtSupportDebugWriteLine(gBuffer);

            while (TRUE)
            {
                if (GetNextLockedRange(
                        &Start,
                        &Length) == FALSE)
                    break;

                if (Start + Length > gAddrLimitLo && Start < gAddrLimitHi)
                {
                    EFI_PHYSICAL_ADDRESS CurStart;

                    // Update the UI with the address range to test
                    MtUiSetAddressRange(Start >= gAddrLimitLo ? Start : gAddrLimitLo,
                                        Start + Length <= gAddrLimitHi ? Start + Length : gAddrLimitHi);

                    CurStart = Start;
                    LengthTested = 0;
                    while (LengthTested < Length)
                    {
                        MtSupportTestTick(FALSE);
                        MtSupportPollECC();

                        if (MtSupportCheckTestAbort())
                        {
                            TestResult = EFI_ABORTED;
                            goto Cleanup;
                        }

                        if (EFI_ERROR(TestResult) && TestResult != EFI_TEST_FAILED)
                            goto Cleanup;

                        SubRangeLength = MIN(SIZE_64MB, Length - LengthTested);

                        if (CurStart + SubRangeLength > gAddrLimitLo && CurStart < gAddrLimitHi)
                        {
                            EFI_PHYSICAL_ADDRESS Start2;
                            UINT64 SubRangeLength2;

                            if (CurStart < gAddrLimitLo)
                                Start2 = gAddrLimitLo;
                            else
                                Start2 = CurStart;

                            if (CurStart + SubRangeLength > gAddrLimitHi)
                                SubRangeLength2 = gAddrLimitHi - Start2;
                            else
                                SubRangeLength2 = CurStart + SubRangeLength - Start2;

                            MtUiSetAddressRange(Start2, Start2 + SubRangeLength2);

                            if (gVerbosity > 0)
                            {
                                // Print the current progress every ~256MB
                                if (DebugLenTested == 0)
                                {
                                    UINT64 End = Start2 + SIZE_256MB;
                                    if (End > Start + Length)
                                        End = Start + Length;
                                    if (End > gAddrLimitHi)
                                        End = gAddrLimitHi;
                                    AsciiSPrint(gBuffer, BUF_SIZE, "Testing memory ranges: 0x%p - 0x%p (%ldMB)", Start2, End, DivU64x32((End - Start2), SIZE_1MB));
                                    MtSupportDebugWriteLine(gBuffer);
                                }
                                DebugLenTested += SubRangeLength2;
                                if (DebugLenTested >= SIZE_256MB)
                                    DebugLenTested = 0;
                            }

                            if (Stage == 0)
                                TestResult |= Test->RangeTest1(Test->TestNum, MPSupportGetBspId(), Start2, SubRangeLength2, mCurPass == 1 && gPass1Full == FALSE ? DivU64x32(Test->IterCount, 3) : Test->IterCount, PAT_USER, Pattern, Test->Context);
                            else
                                TestResult |= Test->RangeTest2(Test->TestNum, MPSupportGetBspId(), Start2, SubRangeLength2, mCurPass == 1 && gPass1Full == FALSE ? DivU64x32(Test->IterCount, 3) : Test->IterCount, PAT_USER, Pattern, Test->Context);

                            TotalLengthTested += SubRangeLength2;
                            MtUiUpdateProgress(TotalLengthTested);
                        }

                        CurStart += SubRangeLength;
                        LengthTested += SubRangeLength;
                    }
                }
                Start += Length;
            }

            AsciiSPrint(gBuffer, BUF_SIZE, "Finished Stage %d, Pattern=%08X", Stage, Pattern);
            MtSupportDebugWriteLine(gBuffer);

            // Sleep for 5 minutes before verifying
            if (Stage == 0)
            {
                UINT64 SleepTimeMs = MultU64x32(gBitFadeSecs, 1000);
                UINT64 SleepStartTime = MtSupportGetTimeInMilliseconds();
                UINT32 TotalSleepTime = 0;

                FPrint(DEBUG_FILE_HANDLE, L"Sleep start time: %ld", SleepStartTime);
                FPrint(DEBUG_FILE_HANDLE, L"Sleep time: %ld", SleepTimeMs);

                MtSupportSetCPURunning(MPSupportGetBspId(), FALSE);
                while (MtSupportGetTimeInMilliseconds() < SleepStartTime + SleepTimeMs)
                {
                    CHAR16 SleepStatus[64];
                    UnicodeSPrint(SleepStatus, sizeof(SleepStatus), GetStringById(STRING_TOKEN(STR_TEST_10_SLEEP), TempBuf, sizeof(TempBuf)), DivU64x32(SleepStartTime + SleepTimeMs - MtSupportGetTimeInMilliseconds(), 1000));
                    if (TotalSleepTime == 0)
                    {
                        FPrint(DEBUG_FILE_HANDLE, SleepStatus);
                    }
                    MtUiSetTestName(SleepStatus);

                    MtSupportTestTick(FALSE);
                    MtSupportPollECC();

                    if (MtSupportCheckTestAbort())
                    {
                        TestResult = EFI_ABORTED;
                        goto Cleanup;
                    }

                    gBS->Stall(100000);
                    TotalSleepTime += 100;

                    if (TotalSleepTime % 30000 == 0)
                    {
                        AsciiSPrint(gBuffer, BUF_SIZE, "Slept for %ld seconds", DivU64x32(MtSupportGetTimeInMilliseconds() - SleepStartTime, 1000));
                        MtSupportDebugWriteLine(gBuffer);
                    }
                }
                MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);
            }
        }

        Pattern = ~Pattern;
    }

Cleanup:
    return TestResult;
}

// RunHammerTest
//
// Run a three-staged row hammer test
STATIC
EFI_STATUS
EFIAPI
RunHammerTest(
    IN VOID *Context)
{
    EFI_STATUS Status;
    EFI_STATUS TestResult;
    EFI_PHYSICAL_ADDRESS Start;
    UINT64 Length;
    UINT64 LengthTested;
    UINT64 TotalLengthTested;
    UINT64 DebugLenTested = 0;
    UINT64 NextRangeStart;
    UINT64 SkipLength;
    UINT64 SubRangeLength;
    UINTN Stage;
    UINT32 Pattern;

    HAMMER_TEST_DATA *Test = (HAMMER_TEST_DATA *)Context;
    UINT32 NumProcs = MIN(Test->MaxCPUs, (UINT32)MPSupportGetMaxProcessors());
    UINT32 NumProcsEn = MIN(Test->MaxCPUs, (UINT32)MPSupportGetNumEnabledProcessors());

    CHAR16 TempBuf[128];

    BOOLEAN DisplayWarn = FALSE;
    BOOLEAN DoSlowHammering = FALSE;
    AsciiSPrint(gBuffer, BUF_SIZE, "Start %s hammer test (0x%lX - 0x%lX, Step size: 0x%p)", gHammerMode == HAMMER_DOUBLE ? L"double-sided" : L"singled-sided", gAddrLimitLo, gAddrLimitHi, Test->IterCount);
    MtSupportDebugWriteLine(gBuffer);

    if (Test->PatternUser == PAT_USER)
        AsciiSPrint(gBuffer, BUF_SIZE, "Using fixed pattern 0x%08X", (UINT32)(UINTN)Test->PatternUser);
    else
        AsciiSPrint(gBuffer, BUF_SIZE, "Using random pattern");
    MtSupportDebugWriteLine(gBuffer);

    // MtUiSetIterationCount(Test->IterCount);
    MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);
    MtUiUpdateCPUState();

    Status = EFI_SUCCESS;
    TestResult = EFI_SUCCESS;

    Pattern = (UINT32)(UINTN)Test->Context;

    // Pattern Random - Init (0%), Hammering (100%), Verifying (0%)
    MtUiSetProgressTotal(mTestedMemSize);
    MtUiUpdateProgress(0);

    // Check if we have a large amount of RAM to hammer (ie. >= 64GB). If so, we need to skip some ranges.
    // We work with max 512MB segments and for each segment, go through the cycle of 1) Initialize all memory 2) Hammer rows 2) Verify all memory
    // To determine if we need to skip some ranges, we set a maximum of 64 segments. If there are more than 64 segments of 512MB to be tested, we need to skip some ranges.
    // We calculate the start of the next segment to be tested based on the skip length (NextRangeStart). We skip all segments that fall below this value. Once we reach NextRangeStart,
    // the current segment shall be tested and the next NextRangeStart shall be calculated.

    UINT64 MinCPUSeg = SIZE_512MB; // min size to allocate for each thread (512MB)
    // Segment size
    //  Parallel mode + each thread has own range: # threads * MinCPUSeg
    //  Everything else: 512MB
    UINT64 SegSize = gCPUSelMode == CPU_PARALLEL && Test->MultiThreaded == FALSE ? NumProcsEn * MinCPUSeg : SIZE_512MB;
    // 512MB <= segment size <= 2GB
    SegSize = MAX(SIZE_512MB, SegSize);
    SegSize = MIN(SIZE_2GB, SegSize);

    // Apply cap on # of segments
    const UINT64 MAXSEGS = 32;
    UINT64 NumSegs = DivU64x64Remainder(mTestedMemSize, SegSize, NULL);
    if (NumSegs > MAXSEGS)
    {
        NumSegs = MAXSEGS;
        SkipLength = DivU64x64Remainder(mTestedMemSize, NumSegs, NULL);
    }
    else
    {
        SkipLength = 0;
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "# of segments to hammer: %ld (SegSize=%ldMB, SkipSize=%ldMB)", NumSegs, DivU64x32(SegSize, SIZE_1MB), DivU64x32(SkipLength, SIZE_1MB));
    MtSupportDebugWriteLine(gBuffer);

    TotalLengthTested = 0;
    NextRangeStart = 0;

    Start = 0;
    Length = 0;
    while (TRUE)
    {
        if (GetNextLockedRange(
                &Start,
                &Length) == FALSE)
            break;

        if (Start + Length > gAddrLimitLo && Start < gAddrLimitHi)
        {
            EFI_PHYSICAL_ADDRESS CurStart;

            // Update the UI with the address range to test
            MtUiSetAddressRange(Start >= gAddrLimitLo ? Start : gAddrLimitLo,
                                Start + Length <= gAddrLimitHi ? Start + Length : gAddrLimitHi);

            // Divide the memory range into subranges
            CurStart = Start;
            LengthTested = 0;
            while (LengthTested < Length)
            {
                SubRangeLength = MIN(SegSize, Length - LengthTested);

                if (CurStart + SubRangeLength > gAddrLimitLo && CurStart < gAddrLimitHi)
                {
                    EFI_PHYSICAL_ADDRESS Start2;
                    UINT64 SubRangeLength2;
                    BOOLEAN RepeatPass = FALSE;

                    if (CurStart < gAddrLimitLo)
                        Start2 = gAddrLimitLo;
                    else
                        Start2 = CurStart;

                    if (CurStart + SubRangeLength > gAddrLimitHi)
                        SubRangeLength2 = gAddrLimitHi - Start2;
                    else
                        SubRangeLength2 = CurStart + SubRangeLength - Start2;

                    MtUiSetAddressRange(Start2, Start2 + SubRangeLength2);

                    if (gVerbosity > 0)
                    {
                        // Print the current progress every ~256MB
                        if (DebugLenTested == 0)
                        {
                            UINT64 End = Start2 + SIZE_256MB;
                            if (End > Start + Length)
                                End = Start + Length;
                            if (End > gAddrLimitHi)
                                End = gAddrLimitHi;
                            AsciiSPrint(gBuffer, BUF_SIZE, "Testing memory ranges: 0x%p - 0x%p (%ldMB)", Start2, End, DivU64x32((End - Start2), SIZE_1MB));
                            MtSupportDebugWriteLine(gBuffer);
                        }
                        DebugLenTested += SubRangeLength2;
                        if (DebugLenTested >= SIZE_256MB)
                            DebugLenTested = 0;
                    }

                    // We are potentially doing 2 passes of hammering: 1) Fast (maximum) 2) Slow (200K)
                    do
                    {
                        int seed = rand(0);

                        MtSupportSetErrorsAsWarnings(!gDisableSlowHammering && DoSlowHammering == FALSE); // If we are doing fast hammering, don't display errors immediately. Instead save as warnings

                        // Check if we need to skip some ranges.
                        if (Start2 < NextRangeStart)
                            break;
                        else
                            NextRangeStart = Start2 + SkipLength; // Calculate next range to test. If SkipLength == 0, no ranges shall be skipped

                        if (SubRangeLength2 <= SIZE_8MB) // Don't bother testing ranges less than 8MB because the region is not big enough to do any significant hammering
                            break;

                        // For each memory range, do 3 stages: 1) Clear all memory 2) Hammer memory range 3) Verify all memory
                        // We don't know if the row hammer errors will appear outside of the tested range or not so we need to clear and verify all memory every time
                        for (Stage = 0; Stage < 3; Stage++)
                        {
                            EFI_STRING_ID TestStatusStrId;

                            MtSupportTestTick(FALSE);
                            MtSupportPollECC();

                            if (MtSupportCheckTestAbort())
                            {
                                TestResult = EFI_ABORTED;
                                goto Cleanup;
                            }

                            if (EFI_ERROR(TestResult) && TestResult != EFI_TEST_FAILED)
                                goto Cleanup;

                            if (Stage == 0)
                                TestStatusStrId = STRING_TOKEN(STR_TEST_13_INIT);
                            else if (Stage == 1)
                                TestStatusStrId = STRING_TOKEN(STR_TEST_13_HAMMER);
                            else
                                TestStatusStrId = STRING_TOKEN(STR_TEST_13_VERIFY);

                            MtUiSetTestName(GetStringById(TestStatusStrId, TempBuf, sizeof(TempBuf)));

                            rand_seed(seed, (int)MPSupportGetBspId());
                            if (Stage == 0) // Initialize pattern on all memory, use only BSP
                            {
                                RunOnAllMemory(Test->TestNum, Test->InitMem, Test->Pattern, Test->PatternUser);
                            }
                            else if (Stage == 1) // Hammering just the range, can run in parallel
                            {
                                EFI_STATUS BSPResult;
                                EFI_EVENT APEvent[MAX_CPUS];
                                UINT64 BSPTestTime;
                                UINT64 APWaitTime[MAX_CPUS];
                                BOOLEAN bAPFinished = TRUE;
                                UINTN ProcNum;
                                UINT64 WaitStartTime = 0;
                                UINT64 CheckPointTime = 0;
                                UINT64 AbortStartTime = 0;
                                BOOLEAN ErrUpdated = FALSE;

                                SetMem(APEvent, sizeof(APEvent), 0);
                                SetMem(APWaitTime, sizeof(APWaitTime), 0);

                                if (gCPUSelMode == CPU_SEQUENTIAL)
                                {
                                    if (mProcLenTested >= SIZE_256MB)
                                    {
                                        do
                                        {
                                            mSelProcNum = (mSelProcNum + 1) % NumProcs;
                                        } while (MPSupportIsProcEnabled(mSelProcNum) == FALSE);

                                        // Only 1 CPU is running at one time. Set it as the BSP
                                        if (mSelProcNum != MPSupportGetBspId())
                                        {
                                            MtSupportSetCPURunning(MPSupportGetBspId(), FALSE);

                                            AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - Switching BSP from Proc# %d --> %d", MPSupportGetBspId(), mSelProcNum);
                                            MtSupportDebugWriteLine(gBuffer);

                                            Status = MPSupportSwitchBSP(mSelProcNum);
                                            if (EFI_ERROR(Status))
                                            {
                                                AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - Could not switch BSP to Proc#%d (%r).", mSelProcNum, Status);
                                                MtSupportDebugWriteLine(gBuffer);
                                                MtSupportReportCPUError(mTests[mCurTestIdx].TestNo, mSelProcNum);

                                                // Report to user
                                                if (mCPUErrReported == FALSE)
                                                    MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_CPU_START), TempBuf, sizeof(TempBuf)), mSelProcNum);
                                                mCPUErrReported = TRUE;
                                            }
                                        }
                                        mProcLenTested = 0;
                                    }
                                    mProcLenTested += SubRangeLength2;
                                }

                                if (gCPUSelMode == CPU_PARALLEL)
                                {
                                    UINT64 CurCPUStart = (Start2 + (Test->Align - 1)) & ~((UINT64)Test->Align - 1);
                                    UINT64 CPUMemRangeLen = (Start2 + SubRangeLength2 - CurCPUStart) & ~((UINT64)Test->Align - 1);
                                    UINT32 NumProcsUsed = NumProcsEn;

                                    if (Test->MultiThreaded == FALSE || DoSlowHammering) // All threads are hammering the same region unless we are doing slow hammering in which we'll split the region amongst the threads
                                    {
                                        // Ensure that at least 8xStepSize is available for each CPU for hammering, as address pairs must be within the range
                                        NumProcsUsed = (UINT32)MIN(DivU64x64Remainder(Start2 + SubRangeLength2 - CurCPUStart, MinCPUSeg, NULL), NumProcsEn);
                                        if (NumProcsUsed == 0) // If range is too small to split between CPUs, just run it on a single CPU
                                            NumProcsUsed = 1;
                                        CPUMemRangeLen = DivU64x32(Start2 + SubRangeLength2 - CurCPUStart, NumProcsUsed) & ~((UINT64)Test->Align - 1);
                                    }

                                    if (NumProcsUsed > 1)
                                    {
                                        UINT32 NumProcsStarted = 1;
                                        UINTN i;

                                        AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - Running hammer test on %d CPUs (%ldMB each) [0x%lx - 0x%lx]", NumProcsUsed, DivU64x32(CPUMemRangeLen, SIZE_1MB), Start2, Start2 + SubRangeLength2);
                                        MtSupportDebugWriteLine(gBuffer);

                                        // We are running in parallel, start the AP's first
                                        for (i = 0; i < NumProcs && NumProcsStarted < NumProcsUsed; i++)
                                        {
                                            // Allocate memory ranges 1 -> n for odd passes, n -> 1 for even passes
                                            ProcNum = mCurPass % 2 == 1 ? i : NumProcs - i - 1;

                                            if (ProcNum != MPSupportGetBspId() &&
                                                MPSupportIsProcEnabled(ProcNum))
                                            {
                                                Status = MPSupportStartMemTestAP(Test->TestNum, ProcNum, &APEvent[ProcNum], DoSlowHammering ? Test->SlowHammer : Test->FastHammer, CurCPUStart, CPUMemRangeLen, mCurPass == 1 && gPass1Full == FALSE ? Test->IterCount << 2 : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);
                                                if (Status == EFI_SUCCESS)
                                                {
                                                    NumProcsStarted++;
                                                    if (Test->MultiThreaded == FALSE || DoSlowHammering)
                                                        CurCPUStart += CPUMemRangeLen;
                                                }
                                                else
                                                {
                                                    AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - Could not start AP#%d 0x%016lx - 0x%016lx (%r).", ProcNum, CurCPUStart, CurCPUStart + CPUMemRangeLen, Status);
                                                    MtSupportDebugWriteLine(gBuffer);

                                                    MtSupportReportCPUError(mTests[mCurTestIdx].TestNo, ProcNum);

                                                    // Report to user
                                                    if (mCPUErrReported == FALSE)
                                                        MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_CPU_START), TempBuf, sizeof(TempBuf)), ProcNum);
                                                    mCPUErrReported = TRUE;
                                                }
                                            }
                                        }
                                    }

                                    MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);

                                    // Finally, run test on BSP on any remaining segments
                                    BSPTestTime = MtSupportGetTimeInMilliseconds();
                                    if (DoSlowHammering)
                                        BSPResult = Test->SlowHammer(Test->TestNum, MPSupportGetBspId(), CurCPUStart, (Start2 + SubRangeLength2 - CurCPUStart) & ~((UINT64)Test->Align - 1), mCurPass == 1 && gPass1Full == FALSE ? Test->IterCount << 2 : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);
                                    else
                                        BSPResult = Test->FastHammer(Test->TestNum, MPSupportGetBspId(), CurCPUStart, (Start2 + SubRangeLength2 - CurCPUStart) & ~((UINT64)Test->Align - 1), mCurPass == 1 && gPass1Full == FALSE ? Test->IterCount << 2 : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);

                                    BSPTestTime = MtSupportGetTimeInMilliseconds() - BSPTestTime;
                                }
                                else
                                {
                                    UINT64 AlignedStart = (Start2 + (Test->Align - 1)) & ~((UINT64)Test->Align - 1);
                                    UINT64 AlignedLen = (Start2 + SubRangeLength2 - AlignedStart) & ~((UINT64)Test->Align - 1);

                                    MtSupportSetCPURunning(MPSupportGetBspId(), TRUE);

                                    // Finally, run test on BSP
                                    BSPTestTime = MtSupportGetTimeInMilliseconds();
                                    if (DoSlowHammering)
                                        BSPResult = Test->SlowHammer(Test->TestNum, MPSupportGetBspId(), AlignedStart, AlignedLen, mCurPass == 1 && gPass1Full == FALSE ? Test->IterCount << 2 : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);
                                    else
                                        BSPResult = Test->FastHammer(Test->TestNum, MPSupportGetBspId(), AlignedStart, AlignedLen, mCurPass == 1 && gPass1Full == FALSE ? Test->IterCount << 2 : Test->IterCount, Test->Pattern, Test->PatternUser, Test->Context);

                                    BSPTestTime = MtSupportGetTimeInMilliseconds() - BSPTestTime;
                                }

                                // Now check the result
                                WaitStartTime = MtSupportGetTimeInMilliseconds();
                                CheckPointTime = 10000;
                                ErrUpdated = FALSE;
                                do
                                {
                                    bAPFinished = TRUE;

                                    UINT64 WaitedMs = MtSupportGetTimeInMilliseconds() - WaitStartTime;
                                    // Check the result on all APs, if necessary
                                    for (ProcNum = 0; ProcNum < NumProcs; ProcNum++)
                                    {
                                        if (ProcNum == MPSupportGetBspId())
                                        {
                                            TestResult |= BSPResult;
                                        }
                                        else
                                        {
                                            if (APEvent[ProcNum]) // If running in parallel, APEvent will be non-NULL
                                            {
                                                Status = gBS->CheckEvent(APEvent[ProcNum]);

                                                if (Status == EFI_NOT_READY) // CPU finished event has not been signalled
                                                {
                                                    if (MPSupportGetMemTestAPFinished(ProcNum) != FALSE) // CPU has finished the memory test, event should be signalled so check again next iteration
                                                    {
                                                        if (APWaitTime[ProcNum] == 0)
                                                            APWaitTime[ProcNum] = MtSupportGetTimeInMilliseconds();

                                                        if ((MtSupportGetTimeInMilliseconds() - APWaitTime[ProcNum]) > AP_SIGNAL_WAIT_TIME) // Event still hasn't been signalled after 1 second
                                                        {
                                                            AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - CPU #%d completed but did not signal (test time = %ldms, event wait time = %ldms, result = %r) (BSP test time = %ldms)", ProcNum, MPSupportGetMemTestAPTestTime(ProcNum), MtSupportGetTimeInMilliseconds() - APWaitTime[ProcNum], MPSupportGetMemTestAPResult(ProcNum), BSPTestTime);
                                                            MtSupportDebugWriteLine(gBuffer);

                                                            TestResult |= MPSupportGetMemTestAPResult(ProcNum);
                                                            gBS->CloseEvent(APEvent[ProcNum]);
                                                            APEvent[ProcNum] = NULL;
                                                        }
                                                        else // Finished, but event hasn't been triggered. Wait at most 1 second to be triggered
                                                        {
                                                            bAPFinished = FALSE;
                                                        }
                                                    }
                                                    else // CPU has not finished the memory test
                                                    {
                                                        if (WaitedMs > CheckPointTime)
                                                        {
                                                            AsciiSPrint(gBuffer, BUF_SIZE, "WARNING - waited for %lds for CPU #%d to finish (BSP test time = %ldms)", DivU64x32(WaitedMs, 1000), ProcNum, BSPTestTime);
                                                            MtSupportDebugWriteLine(gBuffer);

                                                            if (ErrUpdated)
                                                            {
                                                                CheckPointTime = WaitedMs + 10000;
                                                                AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - increasing wait time to %lds due to detected memory errors", DivU64x32(CheckPointTime, 1000));
                                                                MtSupportDebugWriteLine(gBuffer);
                                                            }
                                                        }

                                                        if (WaitedMs > BSPTestTime * BSP_WAIT_TIME_FACTOR &&
                                                            WaitedMs > CheckPointTime)
                                                        {
                                                            AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - aborted waiting for CPU #%d to finish (Wait time = %ldms, BSP test time = %ldms).", ProcNum, WaitedMs, BSPTestTime);
                                                            MtSupportDebugWriteLine(gBuffer);

                                                            TestResult |= MPSupportGetMemTestAPResult(ProcNum);
                                                            gBS->CloseEvent(APEvent[ProcNum]);
                                                            APEvent[ProcNum] = NULL;
                                                        }
                                                        else
                                                            bAPFinished = FALSE;
                                                    }
                                                }
                                                else if (Status == EFI_SUCCESS) // CPU finished event has been signalled
                                                {
                                                    if (MPSupportGetMemTestAPFinished(ProcNum) == FALSE) // CPU has not finished the memory test, maybe timed out or serious error?
                                                    {
                                                        AsciiSPrint(gBuffer, BUF_SIZE, "RunHammerTest - CPU #%d timed out, test time = %ldms (BSP test time = %ldms)", ProcNum, MPSupportGetMemTestAPTestTime(ProcNum), BSPTestTime);
                                                        MtSupportDebugWriteLine(gBuffer);
                                                    }

                                                    TestResult |= MPSupportGetMemTestAPResult(ProcNum);
                                                    gBS->CloseEvent(APEvent[ProcNum]);
                                                    APEvent[ProcNum] = NULL;
                                                }
                                                else
                                                {
                                                    AsciiSPrint(gBuffer, BUF_SIZE, "WARNING - Unexpected error waiting for CPU #%d to finish: %r", ProcNum, Status);
                                                    MtSupportDebugWriteLine(gBuffer);
                                                }
                                            }
                                        }
                                    }

                                    if (WaitedMs > CheckPointTime)
                                        CheckPointTime += 10000;

                                    MtSupportTestTick(FALSE);
                                    ErrUpdated = MtSupportUpdateErrorDisp();

                                    // If not finished, check if aborted
                                    if (bAPFinished == FALSE && MtSupportCheckTestAbort())
                                    {
                                        // We wait at most 5 seconds for AP's to finish
                                        if (AbortStartTime == 0)
                                            AbortStartTime = MtSupportGetTimeInMilliseconds();
                                        else if (MtSupportGetTimeInMilliseconds() - AbortStartTime > 5000)
                                        {
                                            bAPFinished = TRUE;
                                            // Clean up event handle
                                            for (ProcNum = 0; ProcNum < NumProcs; ProcNum++)
                                            {
                                                if (APEvent[ProcNum])
                                                {
                                                    gBS->CloseEvent(APEvent[ProcNum]);
                                                    APEvent[ProcNum] = NULL;

                                                    AsciiSPrint(gBuffer, BUF_SIZE, "CPU #%d did not abort properly.", ProcNum);
                                                    MtSupportDebugWriteLine(gBuffer);
                                                }
                                            }
                                        }
                                    }
                                } while (bAPFinished == FALSE);
                                MtSupportUpdateErrorDisp();
                                MtSupportPollECC();
                            }
                            else if (Stage == 2) // Verify pattern on all memory, use only BSP
                            {
                                EFI_STATUS HammerResult = RunOnAllMemory(Test->TestNum, Test->VerifyMem, Test->Pattern, Test->PatternUser);

                                if (DoSlowHammering == FALSE) // If we are doing fast hammering, don't display errors. Prepare to repeat test for second, slow hammering
                                {
                                    MtSupportUpdateWarningDisp();

                                    if (!gDisableSlowHammering && HammerResult == EFI_TEST_FAILED) // Failed fast hammering. Repeat same test using slow hammering
                                    {
                                        DoSlowHammering = TRUE;
                                        DisplayWarn = TRUE;
                                        RepeatPass = TRUE;
                                        AsciiSPrint(gBuffer, BUF_SIZE, "Fast hammering test failed. Repeat test again with slow hammering. Slow hammering shall performed for the remainder of the test.");
                                        MtSupportDebugWriteLine(gBuffer);
                                    }
                                    else
                                        TestResult |= HammerResult;
                                }
                                else // We are doing slow hammering
                                {
                                    MtSupportUpdateErrorDisp();
                                    if (HammerResult == EFI_TEST_FAILED) // Failed slow hammering, errors are legit
                                    {
                                        DisplayWarn = FALSE; // Errors are real, don't display warning
                                    }
                                    else // Only failed fast hammering. Display warning message
                                    {
                                        // Print warning message. Do this only once
                                        if (DisplayWarn)
                                        {
                                            MtUiPrint(EFI_BROWN, GetStringById(STRING_TOKEN(STR_WARNING_HAMMER), TempBuf, sizeof(TempBuf)));
                                            DisplayWarn = FALSE; // Display warning only once

                                            AsciiSPrint(gBuffer, BUF_SIZE, "Slow hammering test passed. Warnings reported");
                                            MtSupportDebugWriteLine(gBuffer);
                                        }
                                    }

                                    TestResult |= HammerResult;
                                    RepeatPass = FALSE;
                                }
                            }
                        }

                    } while (RepeatPass != FALSE);

                    TotalLengthTested += SubRangeLength2;
                    MtUiUpdateProgress(TotalLengthTested);
                }

                CurStart += SubRangeLength;
                LengthTested += SubRangeLength;
            }
        }

        Start += Length;
    }

#if 0
    if (Pattern == (UINT32)(UINTN)Test->Context)
        Pattern = ~Pattern;
    else
        break;
#endif

Cleanup:
    MtSupportSetErrorsAsWarnings(FALSE);
    return TestResult;
}

// RunOnAllMemory
//
// Helper function for RunHammerTest. Run specified callback function on all memory ranges
STATIC
EFI_STATUS
EFIAPI
RunOnAllMemory(
    IN UINT32 TestNum,
    IN TEST_MEM_RANGE TestRangeFunction,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser)
{
    EFI_STATUS TestResult;
    EFI_PHYSICAL_ADDRESS Start;
    UINT64 Length;
    UINT64 LengthTested;
    UINT64 SubRangeLength;

    TestResult = EFI_SUCCESS;
    Start = 0;
    Length = 0;
    while (TRUE)
    {
        if (GetNextLockedRange(
                &Start,
                &Length) == FALSE)
            break;

        if (Start + Length > gAddrLimitLo && Start < gAddrLimitHi)
        {
            LengthTested = 0;
            while (LengthTested < Length)
            {
                SubRangeLength = MIN(SIZE_32MB, Length - LengthTested);

                MtSupportTestTick(FALSE);
                MtSupportPollECC();

                if (MtSupportCheckTestAbort())
                    return TestResult;

                if (Start + SubRangeLength > gAddrLimitLo && Start < gAddrLimitHi)
                {
                    EFI_PHYSICAL_ADDRESS Start2;
                    UINT64 SubRangeLength2;

                    if (Start < gAddrLimitLo)
                        Start2 = gAddrLimitLo;
                    else
                        Start2 = Start;

                    if (Start + SubRangeLength > gAddrLimitHi)
                        SubRangeLength2 = gAddrLimitHi - Start2;
                    else
                        SubRangeLength2 = Start + SubRangeLength - Start2;

                    TestResult |= TestRangeFunction(TestNum, MPSupportGetBspId(), Start2, SubRangeLength2, 1, Pattern, PatternUser, NULL);
                }

                Start += SubRangeLength;
                LengthTested += SubRangeLength;
            }
        }
        Start += Length;
    }

    return TestResult;
}

#if 0 // hammer test row address bits experimentation. Not used
VOID
MtSupportDetectAddressBits()
{
    INTN                  Key = 0;
    EFI_PHYSICAL_ADDRESS  Start, MaxStart = 0;
    UINT64                Length, MaxLength = 0;
    EFI_STATUS            Status;
    UINT64                Latency[64];
    int                   i;

    SetMem(Latency, sizeof(Latency), 0);

    MtRangesConstructor();

    while (TRUE)
    {
        Status = MtRangesGetNextRange(
            &Key,
            &Start,
            &Length
        );
        if (Status == EFI_NOT_FOUND) {
            break;
        }

        if (Length > MaxLength)
        {
            MaxStart = Start;
            MaxLength = Length;
        }
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "Locking address range 0x%lx - 0x%lx", MaxStart, MaxStart + MaxLength);
    MtSupportDebugWriteLine(gBuffer);

    Status = MtRangesLockRange(MaxStart, MaxLength);
    if (Status != EFI_SUCCESS)
    {
        MtRangesDeconstructor();
        return;
    }

    for (i = 0; LShiftU64(1, i) < maxaddr; i++)
    {
        if ((MaxStart | LShiftU64(1, i)) < (MaxStart + MaxLength))
        {
            UINT8 *Addr1 = (UINT8 *)(UINTN)MaxStart;
            UINT8 *Addr2 = (UINT8 *)(UINTN)(MaxStart | LShiftU64(1, i));
            UINT64 TotalTime;
            int j;

            AsciiSPrint(gBuffer, BUF_SIZE, "Bit %d - Testing addresses 0x%p and 0x%p", i, Addr1, Addr2);
            MtSupportDebugWriteLine(gBuffer);
            AsciiPrint(gBuffer);
            TotalTime = 0;
#ifdef MDE_CPU_X64
            for (j = 0; j < 10000000; j++)
                TotalTime += AsmAddrLatencyTest(Addr1, Addr2);
#elif defined(MDE_CPU_IA32)
            for (j = 0; j < 10000000; j++)
            {
                UINT32 time_low, time_high;
                _asm {
                    mov esi, Addr1
                    mov edi, Addr2
                    clflush[esi]
                    clflush[edi]
                    jmp L60
                    ; .p2align 4, , 7
                    ALIGN 16

                    L60:
                    rdtsc
                        mov     ebx, eax
                        mov     ecx, edx

                        mov eax, [esi]
                        mov eax, [edi]

                        rdtsc
                        sub eax, ebx
                        sbb edx, ecx
                        mov eax, time_low
                        mov edx, time_high
                }
                TotalTime += LShiftU64(time_high, 32) | time_low;
            }
#endif
            Latency[i] = DivU64x32(TotalTime, clks_msec);
        }
    }

    for (i = 0; i < 64; i++)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Bit %d - Latency: %d ms", i, Latency[i]);
        MtSupportDebugWriteLine(gBuffer);
        AsciiPrint(gBuffer);
    }

    /*STEP 2: Detect column address bits*/
    /*Based on the idea that bank parallelism outperforms row miss */
    for (i = 0; i <= 31; i++)
    {
        if ((MaxStart | LShiftU64(1, i) | LShiftU64(1, 28)) < (MaxStart + MaxLength))
        {
            UINT8 *Addr1 = (UINT8 *)(UINTN)MaxStart;
            UINT8 *Addr2 = (UINT8 *)(UINTN)(MaxStart | LShiftU64(1, i) | LShiftU64(1, 28));
            UINT64 TotalTime;
            int j;

            AsciiSPrint(gBuffer, BUF_SIZE, "Bit %d - Testing addresses 0x%p and 0x%p", i, Addr1, Addr2);
            MtSupportDebugWriteLine(gBuffer);
            AsciiPrint(gBuffer);

            TotalTime = 0;
#ifdef MDE_CPU_X64
            for (j = 0; j < 10000000; j++)
                TotalTime += AsmAddrLatencyTest(Addr1, Addr2);
#elif defined(MDE_CPU_IA32)
            for (j = 0; j < 10000000; j++)
            {
                UINT32 time_low, time_high;
                _asm {
                    mov esi, Addr1
                    mov edi, Addr2
                    clflush[esi]
                    clflush[edi]
                    jmp L61
                    ; .p2align 4, , 7
                    ALIGN 16

                    L61:
                    rdtsc
                        mov     ebx, eax
                        mov     ecx, edx

                        mov eax, [esi]
                        mov eax, [edi]

                        rdtsc
                        sub eax, ebx
                        sbb edx, ecx
                        mov eax, time_low
                        mov edx, time_high
                }
                TotalTime += LShiftU64(time_high, 32) | time_low;
            }
#endif

            Latency[i] = DivU64x32(TotalTime, clks_msec);
        }
    }

    for (i = 0; i <= 31; i++)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Bit %d - Latency: %d ms", i, Latency[i]);
        MtSupportDebugWriteLine(gBuffer);
        AsciiPrint(gBuffer);
    }

    MtRangesUnlockRange(MaxStart, MaxLength);
    MtRangesDeconstructor();
}
#endif

#if 1 // not used
EFI_STATUS
EFIAPI
MtSupportInstallAllTests()
{
    int i;
    for (i = 0; i < gNumCustomTests; i++)
    {
        EFI_STATUS Status = EFI_SUCCESS;
        if (gCustomTestList[i].RunMemTest2 == NULL)
        {
            Status = MtSupportInstallMemoryRangeTest(
                gCustomTestList[i].TestNo,
                gCustomTestList[i].NameStr,
                gCustomTestList[i].NameStrId,
                gCustomTestList[i].RunMemTest1,
                gCustomTestList[i].MaxCPUs,
                gCustomTestList[i].Iterations,
                gCustomTestList[i].Align,
                gCustomTestList[i].Pattern,
                gCustomTestList[i].PatternUser,
                gCustomTestList[i].CacheMode,
                gCustomTestList[i].Context);
        }
        else if (gCustomTestList[i].RunMemTest3 == NULL)
        {
            Status = MtSupportInstallBitFadeTest(
                gCustomTestList[i].TestNo,
                gCustomTestList[i].NameStr,
                gCustomTestList[i].NameStrId,
                gCustomTestList[i].RunMemTest1,
                gCustomTestList[i].RunMemTest2,
                gCustomTestList[i].MaxCPUs,
                gCustomTestList[i].Iterations,
                gCustomTestList[i].Align,
                gCustomTestList[i].Pattern,
                gCustomTestList[i].PatternUser,
                gCustomTestList[i].CacheMode,
                gCustomTestList[i].Context);
        }
        else
        {
            Status = MtSupportInstallHammerTest(
                gCustomTestList[i].TestNo,
                gCustomTestList[i].NameStr,
                gCustomTestList[i].NameStrId,
                gCustomTestList[i].RunMemTest1,
                gCustomTestList[i].RunMemTest2,
                gCustomTestList[i].RunMemTest3,
                gCustomTestList[i].RunMemTest4,
                gCustomTestList[i].MaxCPUs,
                gCustomTestList[i].Iterations,
                gCustomTestList[i].Align,
                gCustomTestList[i].Pattern,
                gCustomTestList[i].PatternUser,
                gCustomTestList[i].CacheMode,
                gCustomTestList[i].Context);
        }

        if (Status != EFI_SUCCESS)
            return Status;
    }
    return EFI_SUCCESS;
}
#endif

// MtSupportInstallMemoryRangeTest
//
// Add specified memory test to the current test suite.
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
    IN VOID *Context)
{
    EFI_STATUS Status;
    MEM_RANGE_TEST_DATA *NewInstance;
    UINTN i;

    // Check if already installed
    for (i = 0; i < mTestCount; i++)
    {
        if (mTests[i].TestNo == TestNo)
            return EFI_INVALID_PARAMETER;
    }

    NewInstance = (MEM_RANGE_TEST_DATA *)AllocatePool(sizeof(*NewInstance));
    if (NewInstance == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    NewInstance->TestNum = TestNo;
    NewInstance->RangeTest = TestRangeFunction;
    NewInstance->MaxCPUs = MaxCPUs;
    NewInstance->IterCount = NumberOfIters;
    NewInstance->Align = Align;
    NewInstance->Pattern = Pattern;
    NewInstance->PatternUser = PatternUser;
    NewInstance->Context = Context;

    Status = MtSupportInstallMemoryTest(
        TestNo,
        NameStr,
        NameStrId,
        CacheMode,
        RunMemoryRangeTest,
        (VOID *)NewInstance);
    if (EFI_ERROR(Status))
    {
        FreePool(NewInstance);
        return Status;
    }

    return Status;
}

// MtSupportInstallBitFadeTest
//
// Add specified bit fade test to the current test suite.
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
    IN VOID *Context)
{
    EFI_STATUS Status;
    BIT_FADE_TEST_DATA *NewInstance;
    UINTN i;

    // Check if already installed
    for (i = 0; i < mTestCount; i++)
    {
        if (mTests[i].TestNo == TestNo)
            return EFI_INVALID_PARAMETER;
    }

    NewInstance = (BIT_FADE_TEST_DATA *)AllocatePool(sizeof(*NewInstance));
    if (NewInstance == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    NewInstance->TestNum = TestNo;
    NewInstance->RangeTest1 = TestRangeFunction1;
    NewInstance->RangeTest2 = TestRangeFunction2;
    NewInstance->MaxCPUs = MaxCPUs;
    NewInstance->IterCount = NumberOfIters;
    NewInstance->Align = Align;
    NewInstance->Pattern = Pattern;
    NewInstance->PatternUser = PatternUser;

    NewInstance->Context = Context;

    Status = MtSupportInstallMemoryTest(
        TestNo,
        NameStr,
        NameStrId,
        CacheMode,
        RunBitFadeTest,
        (VOID *)NewInstance);
    if (EFI_ERROR(Status))
    {
        FreePool(NewInstance);
        return Status;
    }

    return Status;
}

// MtSupportInstallHammerTest
//
// Add specified hammer test to the current test suite.
EFI_STATUS
EFIAPI
MtSupportInstallHammerTest(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN TEST_MEM_RANGE InitMemoryFunction,
    IN TEST_MEM_RANGE FastHammerFunction,
    IN TEST_MEM_RANGE SlowHammerFunction,
    IN TEST_MEM_RANGE VerifyMemoryFunction,
    IN UINT32 MaxCPUs,
    IN UINT64 NumberOfIters,
    IN UINTN Align,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser,
    IN CACHEMODE CacheMode,
    IN VOID *Context)
{
    EFI_STATUS Status;
    HAMMER_TEST_DATA *NewInstance;
    UINTN i;

    // Check if already installed
    for (i = 0; i < mTestCount; i++)
    {
        if (mTests[i].TestNo == TestNo)
            return EFI_INVALID_PARAMETER;
    }

    NewInstance = (HAMMER_TEST_DATA *)AllocatePool(sizeof(*NewInstance));
    if (NewInstance == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    NewInstance->TestNum = TestNo;
    NewInstance->InitMem = InitMemoryFunction;
    NewInstance->FastHammer = FastHammerFunction;
    NewInstance->SlowHammer = SlowHammerFunction;
    NewInstance->VerifyMem = VerifyMemoryFunction;
    NewInstance->MaxCPUs = MaxCPUs;
    NewInstance->IterCount = NumberOfIters;
    NewInstance->Align = Align;
    NewInstance->Pattern = Pattern;
    NewInstance->PatternUser = PatternUser;
    NewInstance->Context = Context;
    NewInstance->MultiThreaded = FALSE;

    Status = MtSupportInstallMemoryTest(
        TestNo,
        NameStr,
        NameStrId,
        CacheMode,
        RunHammerTest,
        (VOID *)NewInstance);
    if (EFI_ERROR(Status))
    {
        FreePool(NewInstance);
        return Status;
    }

    return Status;
}

// MtSupportInstallHammerTest_multithreaded
//
// Add specified hammer test to the current test suite.
EFI_STATUS
EFIAPI
MtSupportInstallHammerTest_multithreaded(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN TEST_MEM_RANGE InitMemoryFunction,
    IN TEST_MEM_RANGE FastHammerFunction,
    IN TEST_MEM_RANGE SlowHammerFunction,
    IN TEST_MEM_RANGE VerifyMemoryFunction,
    IN UINT32 MaxCPUs,
    IN UINT64 NumberOfIters,
    IN UINTN Align,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser,
    IN CACHEMODE CacheMode,
    IN VOID *Context)
{
    EFI_STATUS Status;
    HAMMER_TEST_DATA *NewInstance;
    UINTN i;

    // Check if already installed
    for (i = 0; i < mTestCount; i++)
    {
        if (mTests[i].TestNo == TestNo)
            return EFI_INVALID_PARAMETER;
    }

    NewInstance = (HAMMER_TEST_DATA *)AllocatePool(sizeof(*NewInstance));
    if (NewInstance == NULL)
    {
        return EFI_OUT_OF_RESOURCES;
    }

    NewInstance->TestNum = TestNo;
    NewInstance->InitMem = InitMemoryFunction;
    NewInstance->FastHammer = FastHammerFunction;
    NewInstance->SlowHammer = SlowHammerFunction;
    NewInstance->VerifyMem = VerifyMemoryFunction;
    NewInstance->MaxCPUs = MaxCPUs;
    NewInstance->IterCount = NumberOfIters;
    NewInstance->Align = Align;
    NewInstance->Pattern = Pattern;
    NewInstance->PatternUser = PatternUser;
    NewInstance->Context = Context;
    NewInstance->MultiThreaded = TRUE;

    Status = MtSupportInstallMemoryTest(
        TestNo,
        NameStr,
        NameStrId,
        CacheMode,
        RunHammerTest,
        (VOID *)NewInstance);
    if (EFI_ERROR(Status))
    {
        FreePool(NewInstance);
        return Status;
    }

    return Status;
}

// MtSupportInstallMemoryTest
//
// Add specified test to the current test suite.
EFI_STATUS
EFIAPI
MtSupportInstallMemoryTest(
    IN UINT32 TestNo,
    IN CHAR16 *NameStr,
    IN EFI_STRING_ID NameStrId,
    IN CACHEMODE CacheMode,
    IN RUN_MEM_TEST MemTestFunction,
    IN VOID *Context)
{
    MEM_TEST_INSTANCE *NewTests;
    UINTN NewMaxTests;

    if (mTestCount >= mMaxTestCount)
    {
        NewMaxTests = mMaxTestCount + 32;
        NewTests =
            (MEM_TEST_INSTANCE *)AllocateZeroPool(
                sizeof(MEM_TEST_INSTANCE) *
                NewMaxTests);
        if (NewTests == NULL)
        {
            return EFI_OUT_OF_RESOURCES;
        }

        if (mTests != NULL && mMaxTestCount != 0)
        {
            CopyMem(NewTests, mTests, sizeof(MEM_TEST_INSTANCE) * mMaxTestCount);
            FreePool(mTests);
        }

        mMaxTestCount = NewMaxTests;
        mTests = NewTests;
    }

    mTests[mTestCount].TestNo = TestNo;
    mTests[mTestCount].NameStr = NameStr;
    mTests[mTestCount].NameStrId = NameStrId;
    mTests[mTestCount].CacheMode = CacheMode;
    mTests[mTestCount].RunMemTest = MemTestFunction;
    mTests[mTestCount].Context = Context;
    mTests[mTestCount].NumErrors = 0;
    mTestCount++;

    return EFI_SUCCESS;
}

// MtSupportNumTests
//
// Return the number of installed tests in the test suite
UINTN
EFIAPI
MtSupportNumTests()
{
    return mTestCount;
}

// MtSupportNumPassed
//
// Return the number of tests that passed in current test suite run
UINTN
EFIAPI
MtSupportNumPassed()
{
    return mPassCount;
}

// MtSupportTestsCompleted
//
// Return the number of completed tests in current test suite run
UINTN
EFIAPI
MtSupportTestsCompleted()
{
    return mTestsCompleted;
}

int
    EFIAPI
    MtSupportGetTestResult()
{
    BOOLEAN AllTestsCompleted = MtSupportTestsCompleted() == (gNumPasses * MtSupportNumTests());
    BOOLEAN MaxErrorsExceeded = MtSupportGetNumErrors() >= gMaxErrCount;
    if (AllTestsCompleted || MaxErrorsExceeded)
    {
        if (MtSupportGetNumErrors() == 0)
            return RESULT_PASS;
        else
            return RESULT_FAIL;
    }

    if (MtSupportGetNumErrors() == 0)
        return RESULT_INCOMPLETE_PASS;

    return RESULT_INCOMPLETE_FAIL;
}

int
    EFIAPI
    MtSupportGetDimmTestResult(int dimm)
{
    BOOLEAN AllTestsCompleted = MtSupportTestsCompleted() == (gNumPasses * MtSupportNumTests());
    BOOLEAN MaxErrorsExceeded = MtSupportGetNumErrors() >= gMaxErrCount;
    BOOLEAN DimmErrors = MtSupportGetNumSlotErrors(dimm) > 0;
    BOOLEAN UnknownErrors = MtSupportGetNumUnknownSlotErrors() > 0;

    if (DimmErrors)
    {
        if (AllTestsCompleted || MaxErrorsExceeded)
            return RESULT_FAIL;
        else
            return RESULT_INCOMPLETE_FAIL;
    }
    else
    {
        if (UnknownErrors)
            return RESULT_UNKNOWN;
        else if (AllTestsCompleted)
            return RESULT_PASS;
        else
            return RESULT_INCOMPLETE_PASS;
    }
}

CHAR16 *
    EFIAPI
    MtSupportGetTestResultStr(
        int Result,
        int LangId,
        CHAR16 *Str,
        UINTN StrSize)
{
    switch (Result)
    {
    case RESULT_PASS:
        return GetStringFromPkg(LangId, STRING_TOKEN(STR_PASS), Str, StrSize);
    case RESULT_FAIL:
        return GetStringFromPkg(LangId, STRING_TOKEN(STR_FAIL), Str, StrSize);
    case RESULT_INCOMPLETE_PASS:
        return GetStringFromPkg(LangId, STRING_TOKEN(STR_INCOMPLETE_PASS), Str, StrSize);
    case RESULT_INCOMPLETE_FAIL:
        return GetStringFromPkg(LangId, STRING_TOKEN(STR_INCOMPLETE_FAIL), Str, StrSize);
    case RESULT_UNKNOWN:
    default:
        return GetStringFromPkg(LangId, STRING_TOKEN(STR_MENU_UNKNOWN), Str, StrSize);
    }
}

// MtSupportCurIterCount
//
// (Not used)
UINT64
EFIAPI
MtSupportCurIterCount()
{
    if (mTests[mCurTestIdx].RunMemTest == RunBitFadeTest)
        return ((BIT_FADE_TEST_DATA *)mTests[mCurTestIdx].Context)->IterCount;

    return ((MEM_RANGE_TEST_DATA *)mTests[mCurTestIdx].Context)->IterCount;
}

// MtSupportCurTest
//
// Return the index number of current test
UINTN
EFIAPI
MtSupportCurTest()
{
    return mCurTestIdx;
}

// MtSupportCurPass
//
// Return the current pass number
UINTN
EFIAPI
MtSupportCurPass()
{
    return mCurPass;
}

// MtSupportCPUSelMode
//
// Return the total number of errors in current test run
UINTN
EFIAPI
MtSupportCPUSelMode()
{
    return gCPUSelMode;
}

// MtSupportSetAllCPURunning
//
// Set the state for all CPUs
VOID
    EFIAPI
    MtSupportSetAllCPURunning(BOOLEAN Running)
{
    SetMem(mCPURunning, sizeof(mCPURunning), Running);
}

// MtSupportSetCPURunning
//
// Set the state for specified CPU
VOID
    EFIAPI
    MtSupportSetCPURunning(UINTN ProcNum,
                           BOOLEAN Running)
{
    if (ProcNum < MAX_CPUS)
        mCPURunning[ProcNum * 64] = Running;
}

// MtSupportCPURunning
//
// Get the state for specified CPU
BOOLEAN
EFIAPI
MtSupportCPURunning(UINTN ProcNum)
{
    if (ProcNum < MAX_CPUS)
        return mCPURunning[ProcNum * 64];

    return FALSE;
}

// MtSupportUninstallAllTests
//
// Remove all installed test from test suite
EFI_STATUS
EFIAPI
MtSupportUninstallAllTests()
{
    if (mTests != NULL)
    {
        int i;
        for (i = 0; i < (int)mTestCount; i++)
        {
            if (mTests[i].Context)
                FreePool(mTests[i].Context);
        }
        FreePool(mTests);
    }
    mTests = NULL;
    mTestCount = 0;
    mMaxTestCount = 0;

    return EFI_SUCCESS;
}

// MtSupportRunAllTests
//
// Run current test suite
EFI_STATUS
EFIAPI
MtSupportRunAllTests()
{
    EFI_STATUS Status;
    EFI_STATUS ReturnStatus;
    EFI_TIME CurTime;
    int i;

    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[128];

    // Initialize variables
    AsciiFPrint(DEBUG_FILE_HANDLE, "Initializing spin lock (Align=%d)", GetSpinLockProperties());

    InitializeSpinLock(&mErrorLock);
    InitializeSpinLock(&mDebugLock);
#ifdef APDEBUG
    MtUiAPInitBuf();
#endif
    mAbortTesting = FALSE;
    mPassCount = 0;
    mCurTestIdx = 0;
    mCurPass = 1;

    mMinCPUTemp = INT_MAX;
    mMaxCPUTemp = INT_MIN;
    mCurCPUTemp = INT_MAX;
    mSumCPUTemp = 0;
    mNumCPUTempSamples = 0;
    for (i = 0; i < MAX_RAM_TEMP_DIMMS; i++)
    {
        mMinRAMTemp[i] = MAX_TSOD_TEMP;
        mMaxRAMTemp[i] = MIN_TSOD_TEMP;
        mCurRAMTemp[i] = MAX_TSOD_TEMP;
    }
    SetMem(mSumRAMTemp, sizeof(mSumRAMTemp), 0);
    SetMem(mNumRAMTempSamples, sizeof(mNumRAMTempSamples), 0);

    SetMem(&mCurMemTimings, sizeof(mCurMemTimings), 0);
    SetMem(&mMinMemTimings, sizeof(mMinMemTimings), 0);
    mMinMemTimings.SpeedMTs = INT_MAX;
    SetMem(&mMaxMemTimings, sizeof(mMaxMemTimings), 0);
    mMaxMemTimings.SpeedMTs = INT_MIN;

    mPrevUpdateTime = 0;
    mPrevKBPollTime = 0;
    mPrevEccPollTime = 0;
    mPrevTempPollTime = 0;
    mPrevPMPStatusTime = 0;
    mPrevTestFlowPollTime = 0;

    SetMem(&erri, sizeof(erri), 0);
    erri.low_addr = (UINTN)-1;
    if (mErrorBuf == NULL)
    {
        if (gReportNumErr < DEFAULT_ERRBUF_SIZE)
            mErrorBufSize = DEFAULT_ERRBUF_SIZE;
        else
            mErrorBufSize = (UINT)gReportNumErr;
        mErrorBuf = (struct ERRLINE *)AllocatePool(sizeof(struct ERRLINE) * mErrorBufSize);
    }
    SetMem(mErrorBuf, sizeof(struct ERRLINE) * mErrorBufSize, 0);
    mErrorBufHead = mErrorBufTail = 0;
    mTruncateErrLog = FALSE;
    if (mWarningBuf == NULL)
    {
        if (gReportNumWarn < DEFAULT_WARNBUF_SIZE)
            mWarningBufSize = DEFAULT_WARNBUF_SIZE;
        else
            mWarningBufSize = (UINT)gReportNumWarn;
        mWarningBuf = (struct ERRLINE *)AllocatePool(sizeof(struct ERRLINE) * mWarningBufSize);
    }
    SetMem(mWarningBuf, sizeof(struct ERRLINE) * mWarningBufSize, 0);
    mWarningBufHead = mWarningBufTail = 0;
    if (mCPUErrBuf == NULL)
    {
        mCPUErrBufSize = DEFAULT_CPUERRBUF_SIZE;

        mCPUErrBuf = (struct CPUERR *)AllocatePool(sizeof(struct CPUERR) * mCPUErrBufSize);
    }
    SetMem(mCPUErrBuf, sizeof(struct CPUERR) * mCPUErrBufSize, 0);
    mCPUErrBufHead = mCPUErrBufTail = 0;
    mCPUErrReported = FALSE;
    mErrorBufFullCount = 0;
    MtSupportClearECCErrors();
    MtSupportSetErrorsAsWarnings(FALSE);
    mTestsCompleted = 0;
    for (i = 0; i < (int)mTestCount; i++)
    {
        mTests[i].NumErrors = 0;
        mTests[i].NumPassed = 0;
        mTests[i].NumCompleted = 0;
    }

    ReturnStatus = EFI_SUCCESS;
#if 0
    TraverseMTRR();
#endif

    if (gTriggerOnErr)
    {
        EFI_INPUT_KEY key;
        CHAR16 TempBuf3[64];

        SetMem(&key, sizeof(key), 0);

        UnicodeSPrint(TempBuf2, sizeof(TempBuf2), L"0x%lx", (UINTN)&mLastError);
        MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                      GetStringById(STRING_TOKEN(STR_MEM_ERROR_TRIGGER), TempBuf, sizeof(TempBuf)),
                      L"",
                      TempBuf2,
                      L"",
                      GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf3, sizeof(TempBuf3)),
                      NULL);
    }

    // Set the start time
    mStartTime = MtSupportReadTSC();
    gRT->GetTime(&mTestStartTime, NULL);
    mElapsedTime = 0;

    AsciiSPrint(gBuffer, BUF_SIZE, "*** TEST SESSION - %d-%02d-%02d %02d:%02d:%02d ***", mTestStartTime.Year, mTestStartTime.Month, mTestStartTime.Day, mTestStartTime.Hour, mTestStartTime.Minute, mTestStartTime.Second);
    MtSupportDebugWriteLine(gBuffer);

    if (efi_time(&mTestStartTime) == 0)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Warning: time obtained from EFI_GET_TIME may be invalid");
        MtSupportDebugWriteLine(gBuffer);
    }
    AsciiSPrint(gBuffer, BUF_SIZE, "CPU selection mode = %d", gCPUSelMode);
    MtSupportDebugWriteLine(gBuffer);

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    LogAllMtrrs(NULL);
#endif

    MtUiPrintReset();
    MtUiDisplayTestStatus();

    mSelProcNum = gCPUSelMode == CPU_SINGLE ? gSelCPUNo : MPSupportGetBspId();

    // Set the random seed
    rand_init((int)MPSupportGetMaxProcessors());

    // Read memory map
    // MtRangesConstructor();

    // LockAllMemRanges();

    for (mCurPass = 1; mCurPass <= gNumPasses; mCurPass++)
    {
        UINT64 ExecTime;

        MtUiSetPassNo(mCurPass);

        AsciiSPrint(gBuffer, BUF_SIZE, "Starting pass #%d (of %d)", mCurPass, gNumPasses);
        MtSupportDebugWriteLine(gBuffer);
        for (mCurTestIdx = 0; mCurTestIdx < mTestCount; mCurTestIdx++)
        {
            // UINT64 OldMtrr = 0;
            UINT32 NumProcs = MIN(gCustomTestList[mTests[mCurTestIdx].TestNo].MaxCPUs, (UINT32)MPSupportGetMaxProcessors());
            // UINT32 NumProcsEn = MIN(gCustomTestList[mTests[mCurTestIdx].TestNo].MaxCPUs, (UINT32)MPSupportGetNumEnabledProcessors());

            // Reset variables for each pass
            mSkipTest = FALSE;
            MtUiSetTestsCompleted(mCurTestIdx, MtSupportNumTests());
            MtUiSetTestName(mTests[mCurTestIdx].NameStr ? mTests[mCurTestIdx].NameStr : GetStringById(mTests[mCurTestIdx].NameStrId, TempBuf, sizeof(TempBuf)));
            MtUiUpdateProgress(0);
            MtUiSetAddressRange(0, 0);
            MtUiSetPattern(0);
            MtSupportTestTick(FALSE);
            MtSupportPollECC();

            AsciiSPrint(gBuffer, BUF_SIZE, "Running test #%d (%s)", mTests[mCurTestIdx].TestNo, mTests[mCurTestIdx].NameStr ? mTests[mCurTestIdx].NameStr : GetStringFromPkg(0, mTests[mCurTestIdx].NameStrId, TempBuf, sizeof(TempBuf)));
            MtSupportDebugWriteLine(gBuffer);

            // Switch the BSP, if required
            if (mSelProcNum != MPSupportGetBspId())
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "MtSupportRunAllTests - Switching BSP from Proc# %d --> %d", MPSupportGetBspId(), mSelProcNum);
                MtSupportDebugWriteLine(gBuffer);

                Status = MPSupportSwitchBSP(mSelProcNum);
                if (EFI_ERROR(Status))
                {
                    AsciiSPrint(gBuffer, BUF_SIZE, "MtSupportRunAllTests - Could not switch BSP to Proc#%d (%r).", mSelProcNum, Status);
                    MtSupportDebugWriteLine(gBuffer);
                    MtSupportReportCPUError(mTests[mCurTestIdx].TestNo, mSelProcNum);

                    // Report to user
                    if (mCPUErrReported == FALSE)
                        MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_CPU_START), TempBuf, sizeof(TempBuf)), mSelProcNum);
                    mCPUErrReported = TRUE;
                }
            }

            MtSupportSetAllCPURunning(FALSE);

            // Do any ECC injections before each test
            if (gECCInject)
            {
                BOOLEAN ret = TRUE;
                UINT8 TextColour = gST->ConOut->Mode->Attribute & 0x0F;

                MtSupportDebugWriteLine("MtSupportRunAllTests - Injecting ECC error");

                if (AsciiStrCmp(get_mem_ctrl_name(0), "AMD 15h") == 0 || AsciiStrCmp(get_mem_ctrl_name(0), "AMD 16h") == 0 ||
                    AsciiStrCmp(get_mem_ctrl_name(0), "AMD Steppe Eagle") == 0 || AsciiStrCmp(get_mem_ctrl_name(0), "AMD 15h (30h-3fh)") == 0 ||
                    AsciiStrCmp(get_mem_ctrl_name(0), "AMD Merlin Falcon (60h-6fh)") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_amd64(0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Nehalem") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Lynnfield") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Westmere") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_nhm(0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel E3-1200v3") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_e3haswell(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel E3-1200") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel E3-1200v2") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Ivy Bridge") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_e3sb(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Broadwell-H") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_broadwell_h(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Skylake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_skylake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Atom C2000 SoC Transaction Router") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_c2000atom(0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Apollo Lake SoC") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_apollolake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Kaby Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_kabylake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Coffee Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_coffeelake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Comet Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_cometlake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Ice Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_icelake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Rocket Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_rocketlake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Tiger Lake H") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_tigerlake_h(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Tiger Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_tigerlake(0, 0, 1);
                }
                else if (AsciiStrStr(get_mem_ctrl_name(0), "Intel Alder Lake") != NULL || AsciiStrCmp(get_mem_ctrl_name(0), "Intel Raptor Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_alderlake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Meteor Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Arrow Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Lunar Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_meteorlake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Elkhart Lake") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_elkhartlake(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Broadwell-DE (IMC 0)") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_broadwell(0, 0, 1);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Sandy Bridge-E") == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    inject_e5sb(0, 0, 1);
                }
                else if (AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen (", AsciiStrLen("AMD Ryzen (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 2 (", AsciiStrLen("AMD Ryzen Zen 2 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 3 (", AsciiStrLen("AMD Ryzen Zen 3 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(0), "Hygon Dhyana", AsciiStrLen("Hygon Dhyana")) == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_ryzen(0, 0, 1);
                }
                else if (AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 4 (", AsciiStrLen("AMD Ryzen Zen 4 (")) == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_ryzen_zen4(0, 0, 1);
                }
                else if (AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 5 (", AsciiStrLen("AMD Ryzen Zen 5 (")) == 0)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_STATUS_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                    ret = inject_ryzen_zen5(0, 0, 1);
                }

                if (ret == FALSE)
                {
                    MtUiPrint(TextColour, GetStringById(STRING_TOKEN(STR_WARN_ECC_INJECT), TempBuf, sizeof(TempBuf)), get_mem_ctrl_name(0));
                }
            }

#if 0 // for testing ECC injection display
            {
                UINTN ErrAddr;

                int seed = rand(0);

                rand64_seed(seed, 0);
                ErrAddr = (UINTN)(rand64(0) & (maxaddr - 1));
                MtSupportReportError(ErrAddr & ~(sizeof(void *) - 1), mTests[mCurTestIdx].TestNo, 0xAAAAAAAA, 0xFFFFFFFF, 0);
                ErrAddr = (UINTN)(rand64(0) & (maxaddr - 1));
                MtSupportReportError32(ErrAddr & ~3, mTests[mCurTestIdx].TestNo, 0x55555555, 0xFFFFFFFF, 0);
                ErrAddr = (UINTN)(rand64(0) & (maxaddr - 1));
                MtSupportReportError64(ErrAddr & ~7, mTests[mCurTestIdx].TestNo, 0x1248124880008000, 0x0, 0);
                ErrAddr = (UINTN)(rand64(0) & (maxaddr - 1));
                MtSupportReportECCError(ErrAddr, FALSE, 0x0fdd, 2, 1);
                ErrAddr = (UINTN)(rand64(0) & (maxaddr - 1));
                MtSupportReportECCError(ErrAddr, TRUE, -1, 3, -1);
                ErrAddr = (UINTN)(rand64(0) & (maxaddr - 1));
                MtSupportReportIBECCError(ErrAddr, TRUE, 0xbbac, -1, -1);
                MtSupportReportECCError_dimm(0x1ff, 0x1ff, 0xffff, 0xffff, FALSE, 0xfee, 1, 1);
                MtSupportReportECCError_dimm(-1, -1, -1, 5, FALSE, -1, -1, -1);
            }
#endif

            if (mCurPass == 1) // Use fixed random seed on first pass
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "MtSupportRunAllTests - Setting random seed to 0x%08X", RANDOMSEED);
                MtSupportDebugWriteLine(gBuffer);

                for (i = 0; i < (int)NumProcs; i++)
                    rand_seed(RANDOMSEED, i);
            }
            else
            {
                unsigned int seed = (unsigned int)(MtSupportReadTSC() & 0xffffffffull);

                AsciiSPrint(gBuffer, BUF_SIZE, "MtSupportRunAllTests - Setting random seed to 0x%08X", seed);
                MtSupportDebugWriteLine(gBuffer);

                for (i = 0; i < (int)NumProcs; i++)
                    rand_seed(seed, i);
            }

            AsciiSPrint(gBuffer, BUF_SIZE, "MtSupportRunAllTests - Start time: %ld ms", MtSupportGetTimeInMilliseconds());
            MtSupportDebugWriteLine(gBuffer);

            ExecTime = MtSupportGetTimeInMilliseconds();
            Status = mTests[mCurTestIdx].RunMemTest(mTests[mCurTestIdx].Context);
            ExecTime = MtSupportGetTimeInMilliseconds() - ExecTime;

            AsciiSPrint(gBuffer, BUF_SIZE, "MtSupportRunAllTests - Test execution time: %d.%03ds (Test %d cumulative error count: %d, buffer full count: %d)", (UINT32)DivU64x32(ExecTime, 1000), ModU64x32(ExecTime, 1000), mTests[mCurTestIdx].TestNo, mTests[mCurTestIdx].NumErrors, mErrorBufFullCount);
            MtSupportDebugWriteLine(gBuffer);

            MtSupportSetAllCPURunning(FALSE);

            // Stop ECC injections after test
            if (gECCInject)
            {
                if (AsciiStrCmp(get_mem_ctrl_name(0), "AMD 15h") == 0 || AsciiStrCmp(get_mem_ctrl_name(0), "AMD 16h") == 0 ||
                    AsciiStrCmp(get_mem_ctrl_name(0), "AMD Steppe Eagle") == 0 || AsciiStrCmp(get_mem_ctrl_name(0), "AMD 15h (30h-3fh)") == 0 ||
                    AsciiStrCmp(get_mem_ctrl_name(0), "AMD Merlin Falcon (60h-6fh)") == 0)
                {
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Nehalem") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Lynnfield") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Westmere") == 0)
                {
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel E3-1200v3") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_e3haswell(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel E3-1200") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel E3-1200v2") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Ivy Bridge") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_e3sb(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Broadwell-H") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_broadwell_h(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Skylake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_skylake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Atom C2000 SoC Transaction Router") == 0)
                {
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Apollo Lake SoC") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_apollolake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Kaby Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_kabylake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Coffee Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_coffeelake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Comet Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_cometlake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Ice Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_icelake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Rocket Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_rocketlake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Tiger Lake H") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_tigerlake_h(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Tiger Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_tigerlake(0, 0, 0);
                }
                else if (AsciiStrStr(get_mem_ctrl_name(0), "Intel Alder Lake") != NULL || AsciiStrCmp(get_mem_ctrl_name(0), "Intel Raptor Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_alderlake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Meteor Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Arrow Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(0), "Intel Lunar Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_meteorlake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Elkhart Lake") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_elkhartlake(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Broadwell-DE (IMC 0)") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_broadwell(0, 0, 0);
                }
                else if (AsciiStrCmp(get_mem_ctrl_name(0), "Intel Sandy Bridge-E") == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_e5sb(0, 0, 0);
                }
                else if (AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen (", AsciiStrLen("AMD Ryzen (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 2 (", AsciiStrLen("AMD Ryzen Zen 2 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 3 (", AsciiStrLen("AMD Ryzen Zen 3 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(0), "Hygon Dhyana", AsciiStrLen("Hygon Dhyana")) == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_ryzen(0, 0, 0);
                }
                else if (AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 4 (", AsciiStrLen("AMD Ryzen Zen 4 (")) == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_ryzen_zen4(0, 0, 0);
                }
                else if (AsciiStrnCmp(get_mem_ctrl_name(0), "AMD Ryzen Zen 5 (", AsciiStrLen("AMD Ryzen Zen 5 (")) == 0)
                {
                    MtSupportDebugWriteLine("MtSupportRunAllTests - Stopping ECC injection");
                    inject_ryzen_zen5(0, 0, 0);
                }
            }

            // Check for test abort
            if (mAbortTesting)
            {
                gRT->GetTime(&CurTime, NULL);

                mElapsedTime = efi_time(&CurTime) - efi_time(&mTestStartTime);

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted");
                MtSupportDebugWriteLine(gBuffer);

                ReturnStatus = EFI_ABORTED;
                goto Error;
            }
            if (MtSupportGetNumErrors() >= gMaxErrCount)
            {
                EFI_INPUT_KEY key;
                SetMem(&key, sizeof(key), 0);

                gRT->GetTime(&CurTime, NULL);

                mElapsedTime = efi_time(&CurTime) - efi_time(&mTestStartTime);

                AsciiSPrint(gBuffer, BUF_SIZE, "Number of errors exceed maximum error count of %d", gMaxErrCount);
                MtSupportDebugWriteLine(gBuffer);

                MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_MAXERRORS), TempBuf, sizeof(TempBuf)));

                ReturnStatus = EFI_ABORTED;
                goto Error;
            }

            if (Status == EFI_SUCCESS)
            {
                mPassCount++;
                mTests[mCurTestIdx].NumPassed++;
            }

            if (Status == EFI_SUCCESS || Status == EFI_TEST_FAILED)
            {
                mTests[mCurTestIdx].NumCompleted++;
                mTestsCompleted++;
            }
            else
            {
                if (Status != EFI_ABORTED)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "Test %d aborted due to unexpected error: %r", mTests[mCurTestIdx].TestNo, Status);

                    // Report to user
                    MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ERROR_UNEXPECTED), TempBuf, sizeof(TempBuf)), mTests[mCurTestIdx].TestNo, Status);
                }
            }

            // Select next CPU, if round robin
            if (gCPUSelMode == CPU_RROBIN)
            {
                do
                {
                    mSelProcNum = (mSelProcNum + 1) % MPSupportGetMaxProcessors();
                } while (MPSupportIsProcEnabled(mSelProcNum) == FALSE);
            }
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "Finished pass #%d (of %d) (Cumulative error count: %d, buffer full count: %d)", mCurPass, gNumPasses, MtSupportGetNumErrors(), mErrorBufFullCount);
        MtSupportDebugWriteLine(gBuffer);

        MtUiPrint(gST->ConOut->Mode->Attribute & 0x0F, GetStringById(STRING_TOKEN(STR_FINISHED_PASS), TempBuf, sizeof(TempBuf)), mCurPass, gNumPasses, MtSupportGetNumErrors(), MtSupportGetNumCorrECCErrors());
    }

    MtUiSetTestsCompleted(MtSupportNumTests(), MtSupportNumTests());

    gRT->GetTime(&CurTime, NULL);

    mElapsedTime = efi_time(&CurTime) - efi_time(&mTestStartTime);

Error:
    // MtUiPrint(gST->ConOut->Mode->Attribute & 0x0F, GetStringById(STRING_TOKEN(STR_MEM_RELEASE), TempBuf, sizeof(TempBuf)));
    // UnlockAllMemRanges();

    // Clean up memory map
    // MtRangesDeconstructor();

    // Cleanup rand lib
    rand_cleanup();

    MtUiPrint(gST->ConOut->Mode->Attribute & 0x0F, GetStringById(STRING_TOKEN(STR_BENCH_TEST_COMPLETE), TempBuf, sizeof(TempBuf)));

    return ReturnStatus;
}

// MtSupportAbortTesting
//
// End the current test suite run
VOID
    EFIAPI
    MtSupportAbortTesting(
        VOID)
{
    mAbortTesting = TRUE;
}

// MtSupportSkipCurrentTest
//
// Skip the current test and proceed to the next test
VOID
    EFIAPI
    MtSupportSkipCurrentTest(
        VOID)
{
    mSkipTest = TRUE;
}

// update_err_counts
//
// Update the error count variables
static void update_err_counts(UINT32 testno)
{
    int i;

    if (mErrorsAsWarnings)
        erri.data_warn++;
    else
        erri.data_err++;

    for (i = 0; i < (int)mTestCount; i++)
    {
        if (mTests[i].TestNo == testno)
        {
            if (mErrorsAsWarnings)
                mTests[i].NumWarnings++;
            else
                mTests[i].NumErrors++;
            return;
        }
    }
}

#if 0 // not used
// AddDataErrorToBuf
// 
// Add memory data error to the error buffer
VOID
EFIAPI
AddDataErrorToBuf(
    IN UINT32                   TestNo,
    IN UINTN                    ProcNum,
    IN UINTN                    Address,
    IN UINTN                    Actual,
    IN UINTN                    Expected,
    IN UINTN                    ErrBits
)
{
    ALIGN(16) unsigned int actual128[8];
    __m128i* pactual128 = (__m128i*)(((UINTN)actual128 + 0xF) & ~(0xF));

    ALIGN(16) unsigned int expected128[8];
    __m128i* pexpected128 = (__m128i*)(((UINTN)expected128 + 0xF) & ~(0xF));

    ALIGN(16) unsigned int errbits128[8];
    __m128i* perrbits128 = (__m128i*)(((UINTN)errbits128 + 0xF) & ~(0xF));

    SetMem(actual128, sizeof(actual128), 0);
    SetMem(expected128, sizeof(expected128), 0);
    SetMem(errbits128, sizeof(errbits128), 0);

    CopyMem(pactual128, &Actual, sizeof(UINTN));
    CopyMem(pexpected128, &Expected, sizeof(UINTN));
    CopyMem(errbits128, &ErrBits, sizeof(UINTN));

    MmioWrite64((UINTN)&mLastError.Signature, ERRLINE_ERROR_SIG);
    mLastError.PhysAddr = Address;

    CopyMem(&mLastError.Actual, pactual128, sizeof(mLastError.Actual));
    CopyMem(&mLastError.Expected, pexpected128, sizeof(mLastError.Expected));
    CopyMem(&mLastError.ErrorBits, errbits128, sizeof(mLastError.ErrorBits));

    if (((mErrorBufTail + 1) % mErrorBufSize) != mErrorBufHead)
    {
        mErrorBuf[mErrorBufTail].Valid = TRUE;
        gRT->GetTime(&mErrorBuf[mErrorBufTail].Time, NULL);
        mErrorBuf[mErrorBufTail].TestNo = TestNo;
        mErrorBuf[mErrorBufTail].ProcNum = ProcNum;
        mErrorBuf[mErrorBufTail].Type = 0;
        mErrorBuf[mErrorBufTail].AddrType = 0;
        mErrorBuf[mErrorBufTail].AddrInfo.PhysAddr = Address;

        pAct = (UINTN*)&mErrorBuf[mErrorBufTail].ErrInfo.DataErr.Actual;
        pExp = (UINTN*)&mErrorBuf[mErrorBufTail].ErrInfo.DataErr.Expected;
        pErr = (UINTN*)&mErrorBuf[mErrorBufTail].ErrInfo.DataErr.ErrorBits;

        *pAct = Actual;
        *pExp = Expected;
        *pErr = ErrBits;
        mErrorBuf[mErrorBufTail].ErrDataSize = sizeof(UINTN);

        mErrorBufTail = (mErrorBufTail + 1) % mErrorBufSize;
    }
}
#endif

// AddDataErrorToBufVarSize
//
// Add memory data error to the error buffer
VOID
    EFIAPI
    AddDataErrorToBufVarSize(
        IN UINT32 TestNo,
        IN UINTN ProcNum,
        IN UINTN Address,
        IN void *Actual,
        IN void *Expected,
        IN void *ErrBits,
        IN UINT32 DataSize)
{
    ALIGN(16)
    unsigned int actual128[8];
    __m128i *pactual128 = (__m128i *)(((UINTN)actual128 + 0xF) & ~(0xF));

    ALIGN(16)
    unsigned int expected128[8];
    __m128i *pexpected128 = (__m128i *)(((UINTN)expected128 + 0xF) & ~(0xF));

    ALIGN(16)
    unsigned int errbits128[8];
    __m128i *perrbits128 = (__m128i *)(((UINTN)errbits128 + 0xF) & ~(0xF));
    DIMM_ADDRESS_DETAIL Detail = {0};
    if (SysToDimm(Address, &Detail) == 0) {
        Detail.Type = DIMM_ERROR_TYPE_DATA;
    } else {
        Detail.Type = DIMM_ERROR_TYPE_UNKOWN;
    }
    EnqueueError(&gOknDimmErrorQueue, &Detail);

    SetMem(actual128, sizeof(actual128), 0);
    SetMem(expected128, sizeof(expected128), 0);
    SetMem(errbits128, sizeof(errbits128), 0);

    CopyMem(pactual128, Actual, DataSize);
    CopyMem(pexpected128, Expected, DataSize);
    CopyMem(perrbits128, ErrBits, DataSize);

    MmioWrite64((UINTN)&mLastError.Signature, ERRLINE_ERROR_SIG);
    mLastError.PhysAddr = Address;

    CopyMem(&mLastError.Actual, pactual128, sizeof(mLastError.Actual));
    CopyMem(&mLastError.Expected, pexpected128, sizeof(mLastError.Expected));
    CopyMem(&mLastError.ErrorBits, perrbits128, sizeof(mLastError.ErrorBits));

    if (((mErrorBufTail + 1) % mErrorBufSize) != mErrorBufHead)
    {
        mErrorBuf[mErrorBufTail].Valid = TRUE;
        // gRT->GetTime(&mErrorBuf[mErrorBufTail].Time, NULL);
        mErrorBuf[mErrorBufTail].TestNo = TestNo;
        mErrorBuf[mErrorBufTail].ProcNum = ProcNum;
        mErrorBuf[mErrorBufTail].Type = 0;
        mErrorBuf[mErrorBufTail].AddrType = 0;
        mErrorBuf[mErrorBufTail].AddrInfo.PhysAddr = Address;

        CopyMem(&mErrorBuf[mErrorBufTail].ErrInfo.DataErr.Actual, pactual128, sizeof(mErrorBuf[mErrorBufTail].ErrInfo.DataErr.Actual));
        CopyMem(&mErrorBuf[mErrorBufTail].ErrInfo.DataErr.Expected, pexpected128, sizeof(mErrorBuf[mErrorBufTail].ErrInfo.DataErr.Expected));
        CopyMem(&mErrorBuf[mErrorBufTail].ErrInfo.DataErr.ErrorBits, perrbits128, sizeof(mErrorBuf[mErrorBufTail].ErrInfo.DataErr.ErrorBits));
        mErrorBuf[mErrorBufTail].ErrDataSize = DataSize;

        mErrorBufTail = (mErrorBufTail + 1) % mErrorBufSize;
    }
}

#if 0 // not used
// AddDataWarningToBuf
// 
// Add memory data warning to the warning buffer
VOID
EFIAPI
AddDataWarningToBuf(
    IN UINT32                   TestNo,
    IN UINTN                    ProcNum,
    IN UINTN                    Address,
    IN UINTN                    Actual,
    IN UINTN                    Expected,
    IN UINTN                    ErrBits
)
{
    if (((mWarningBufTail + 1) % mWarningBufSize) != mWarningBufHead)
    {
        UINTN* pAct = (UINTN*)&mWarningBuf[mWarningBufTail].ErrInfo.DataErr.Actual;
        UINTN* pExp = (UINTN*)&mWarningBuf[mWarningBufTail].ErrInfo.DataErr.Expected;
        UINTN* pErr = (UINTN*)&mWarningBuf[mWarningBufTail].ErrInfo.DataErr.ErrorBits;

        mWarningBuf[mWarningBufTail].Valid = TRUE;
        gRT->GetTime(&mWarningBuf[mWarningBufTail].Time, NULL);
        mWarningBuf[mWarningBufTail].TestNo = TestNo;
        mWarningBuf[mWarningBufTail].ProcNum = ProcNum;
        mWarningBuf[mWarningBufTail].Type = 0;
        mWarningBuf[mWarningBufTail].AddrType = 0;
        mWarningBuf[mWarningBufTail].AddrInfo.PhysAddr = Address;

        *pAct = Actual;
        *pExp = Expected;
        *pErr = ErrBits;
        mWarningBuf[mWarningBufTail].ErrDataSize = sizeof(UINTN);
        
        mWarningBufTail = (mWarningBufTail + 1) % mWarningBufSize;
    }
}
#endif

VOID
    EFIAPI
    AddDataWarningToBufVarSize(
        IN UINT32 TestNo,
        IN UINTN ProcNum,
        IN UINTN Address,
        IN void *Actual,
        IN void *Expected,
        IN void *ErrBits,
        IN UINT32 DataSize)
{
    ALIGN(16)
    unsigned int actual128[8];
    __m128i *pactual128 = (__m128i *)(((UINTN)actual128 + 0xF) & ~(0xF));

    ALIGN(16)
    unsigned int expected128[8];
    __m128i *pexpected128 = (__m128i *)(((UINTN)expected128 + 0xF) & ~(0xF));

    ALIGN(16)
    unsigned int errbits128[8];
    __m128i *perrbits128 = (__m128i *)(((UINTN)errbits128 + 0xF) & ~(0xF));

    SetMem(actual128, sizeof(actual128), 0);
    SetMem(expected128, sizeof(expected128), 0);
    SetMem(errbits128, sizeof(errbits128), 0);

    CopyMem(pactual128, Actual, DataSize);
    CopyMem(pexpected128, Expected, DataSize);
    CopyMem(perrbits128, ErrBits, DataSize);

    if (((mWarningBufTail + 1) % mWarningBufSize) != mWarningBufHead)
    {
        mWarningBuf[mWarningBufTail].Valid = TRUE;
        // gRT->GetTime(&mWarningBuf[mWarningBufTail].Time, NULL);
        mWarningBuf[mWarningBufTail].TestNo = TestNo;
        mWarningBuf[mWarningBufTail].ProcNum = ProcNum;
        mWarningBuf[mWarningBufTail].Type = 0;
        mWarningBuf[mWarningBufTail].AddrType = 0;
        mWarningBuf[mWarningBufTail].AddrInfo.PhysAddr = Address;

        CopyMem(&mWarningBuf[mWarningBufTail].ErrInfo.DataErr.Actual, pactual128, sizeof(mWarningBuf[mWarningBufTail].ErrInfo.DataErr.Actual));
        CopyMem(&mWarningBuf[mWarningBufTail].ErrInfo.DataErr.Expected, pexpected128, sizeof(mWarningBuf[mWarningBufTail].ErrInfo.DataErr.Expected));
        CopyMem(&mWarningBuf[mWarningBufTail].ErrInfo.DataErr.ErrorBits, perrbits128, sizeof(mWarningBuf[mWarningBufTail].ErrInfo.DataErr.ErrorBits));
        mWarningBuf[mWarningBufTail].ErrDataSize = DataSize;

        mWarningBufTail = (mWarningBufTail + 1) % mWarningBufSize;
    }
}

// AddECCErrorToBuf
//
// Add memory ECC error to the error buffer
VOID
    EFIAPI
    AddECCErrorToBuf(
        IN UINT32 TestNo,
        IN UINTN ProcNum,
        IN UINTN Address,
        IN UINT8 ECCType,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 Socket,
        IN INT8 Channel,
        IN INT8 dimm)
{
    if (((mErrorBufTail + 1) % mErrorBufSize) != mErrorBufHead)
    {
        mErrorBuf[mErrorBufTail].Valid = TRUE;
        // gRT->GetTime(&mErrorBuf[mErrorBufTail].Time, NULL);
        mErrorBuf[mErrorBufTail].TestNo = TestNo;
        mErrorBuf[mErrorBufTail].ProcNum = ProcNum;
        mErrorBuf[mErrorBufTail].Type = 2;
        mErrorBuf[mErrorBufTail].AddrType = 0;
        mErrorBuf[mErrorBufTail].AddrInfo.PhysAddr = Address;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.type = ECCType;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.corrected = Corrected;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.syn = Syndrome;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.socket = Socket;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.chan = Channel;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.dimm = dimm;

        mErrorBufTail = (mErrorBufTail + 1) % mErrorBufSize;
    }
}

// AddECCErrorToBuf_dimm
//
// Add memory ECC error to the error buffer
VOID
    EFIAPI
    AddECCErrorToBuf_dimm(
        IN UINT32 TestNo,
        IN UINTN ProcNum,
        IN INT32 Col,
        IN INT32 Row,
        IN INT8 Bank,
        IN INT8 Rank,
        IN UINT8 ECCType,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 Socket,
        IN INT8 Channel,
        IN INT8 dimm)
{
    if (((mErrorBufTail + 1) % mErrorBufSize) != mErrorBufHead)
    {
        mErrorBuf[mErrorBufTail].Valid = TRUE;
        // gRT->GetTime(&mErrorBuf[mErrorBufTail].Time, NULL);
        mErrorBuf[mErrorBufTail].TestNo = TestNo;
        mErrorBuf[mErrorBufTail].ProcNum = ProcNum;
        mErrorBuf[mErrorBufTail].Type = 2;
        mErrorBuf[mErrorBufTail].AddrType = 1;
        mErrorBuf[mErrorBufTail].AddrInfo.dimm.col = Col;
        mErrorBuf[mErrorBufTail].AddrInfo.dimm.row = Row;
        mErrorBuf[mErrorBufTail].AddrInfo.dimm.rank = Rank;
        mErrorBuf[mErrorBufTail].AddrInfo.dimm.bank = Bank;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.type = ECCType;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.corrected = Corrected;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.syn = Syndrome;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.socket = Socket;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.chan = Channel;
        mErrorBuf[mErrorBufTail].ErrInfo.ECCErr.dimm = dimm;

        mErrorBufTail = (mErrorBufTail + 1) % mErrorBufSize;
    }
}

static __inline void m128i_to_u64_array(__m128i *m128i, UINT64 *u64a)
{
    UINT64 *p = (UINT64 *)m128i;
#if defined(MDE_CPU_AARCH64)
    u64a[0] = *p;
    u64a[1] = *(p + 1);
#else
    u64a[0] = *(p + 1);
    u64a[1] = *p;
#endif
}

// AddParityErrorToBuf
//
// (Not used)
VOID
    EFIAPI
    AddParityErrorToBuf(
        IN UINT32 TestNo,
        IN UINTN ProcNum,
        IN UINTN Address)
{
    if (((mErrorBufTail + 1) % mErrorBufSize) != mErrorBufHead)
    {
        mErrorBuf[mErrorBufTail].Valid = TRUE;
        // gRT->GetTime(&mErrorBuf[mErrorBufTail].Time, NULL);
        mErrorBuf[mErrorBufTail].TestNo = TestNo;
        mErrorBuf[mErrorBufTail].ProcNum = ProcNum;
        mErrorBuf[mErrorBufTail].Type = 1;
        mErrorBuf[mErrorBufTail].AddrType = 0;
        mErrorBuf[mErrorBufTail].AddrInfo.PhysAddr = Address;

        mErrorBufTail = (mErrorBufTail + 1) % mErrorBufSize;
    }
}

static __inline int MC2SMBIOSSlot(int ch, int sl)
{
    // Check memory controller
    int DimmsPerChannel = MtSupportGetDimmsPerChannel();

    UINT32 family = CPUID_FAMILY((&cpu_id));
    UINT32 model = CPUID_MODEL((&cpu_id));
    if ((family == 0x17 &&
         model >= 0x71 && model <= 0x7f) ||
        (family == 0x19 &&
         model >= 0x21 && model <= 0x2f)) // AMD Ryzen Zen 2
    {
        // ch0 -> Channel B
        // ch1 -> Channel A

        // We re-map SMBIOS for ASUS AM4 motherboards to match physical layout (B1, B2, A1, A2)
        // See https://dlcdnets.asus.com/pub/ASUS/mb/SocketAM4/ROG_STRIX_B550-F_GAMING_WIFI_II/E18592_ROG_STRIX_B550-F_GAMING_WI-FI_II_UM_WEB.pdf?model=ROG%20STRIX%20B550-F%20GAMING%20WIFI%20II
        if (AsciiStrStr(gBaseBoardInfo.szManufacturer, "ASUSTeK") != NULL)
        {
            return ch * DimmsPerChannel + sl;
        }

        // For other boards, the physical layout is the same as SMBIOS order (A1, A2, B1, B2)
        const int CH_MAP[] = {1, 0};
        const int MAX_CH = sizeof(CH_MAP) / sizeof(CH_MAP[0]);
        if (ch < MAX_CH)
        {
            return CH_MAP[ch] * DimmsPerChannel + sl;
        }
    }
    else if ((family == 0x19 &&
              model >= 0x40 && model <= 0x4f)) // AMD Ryzen Zen 3+ (Rembrandt)
    {
        // Zen 3+ has 4 UMCs
        if (g_numSMBIOSMem == 2)
        {
            // UMC0-1 -> Channel A
            // UMC2-3 -> Channel B
            return ch / g_numSMBIOSMem;
        }
    }
    else if (SysInfo_IsArrowLake(&gCPUInfo)) // Meteor Lake
    {
        if (DimmsPerChannel == 2)
        {
            // ch0 -> DIMM 2-3
            // ch1 -> DIMM 0-1

            // We re-map SMBIOS to match physical layout
            // so we need to re-map ch0 -> 1, ch1 -> 0
            const int CH_MAP[] = {1, 0};
            if (ch < ARRAY_SIZE(CH_MAP))
            {
                return CH_MAP[ch] * DimmsPerChannel + sl;
            }
        }
    }
    return ch * DimmsPerChannel + sl;
}

VOID GetErrorSlotStr(struct ERRLINE *ErrRec, int *pchan, int *pslot, int *pchip, CHAR16 *Buf, UINTN BufSize)
{
    SetMem(Buf, BufSize, 0);

    if (ErrRec->Type == 2)
    {
        CHAR16 SocketBuf[16];
        CHAR16 TempBuf[16];
        int socket = ErrRec->ErrInfo.ECCErr.socket;
        int chan = ErrRec->ErrInfo.ECCErr.chan;
        int dimm = ErrRec->ErrInfo.ECCErr.dimm;

        // Workaround for Supermicro H12SSL-NT
        // See https://cerb.passmark.com/profiles/ticket/HFH-44296-856
        if (AsciiStrCmp(gBaseBoardInfo.szProductName, "H12SSL-NT") == 0)
        {
            const int CH_MAP[] = {0, 1, 3, 2, 6, 7, 5, 4};
            const int MAX_CH = sizeof(CH_MAP) / sizeof(CH_MAP[0]);
            if (chan < MAX_CH)
                chan = CH_MAP[chan];
        }

        if (socket >= 0)
            UnicodeSPrint(SocketBuf, sizeof(SocketBuf), L"N%d-", socket);
        else
            SocketBuf[0] = L'\0';

        if (chan >= 0 && dimm >= 0)
            UnicodeSPrint(Buf, BufSize, L"%s%d-%d", SocketBuf, chan, dimm);
        else if (chan >= 0)
            UnicodeSPrint(Buf, BufSize, L"%s%d-X", SocketBuf, chan);
        else
            StrCpyS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
    }
    else
    {
        int ch = -1, sl = -1, chip = -1;
        UINT64 ebits = 0;
        UINT32 TestNo = ErrRec->TestNo;

        if (gCustomTestList[TestNo].NameStrId != STRING_TOKEN(STR_TEST_6_NAME) && gCustomTestList[TestNo].NameStrId != STRING_TOKEN(STR_TEST_14_NAME))
        {
            if (gAddr2ChBits != 0 || gAddr2SlBits != 0 || gAddr2CSBits != 0)
            {
                UINT64 ChBits = gAddr2ChBits & (UINT64)ErrRec->AddrInfo.PhysAddr;
                UINT64 SlBits = gAddr2SlBits & (UINT64)ErrRec->AddrInfo.PhysAddr;
                UINT64 CSBits = gAddr2CSBits & (UINT64)ErrRec->AddrInfo.PhysAddr;
                BOOLEAN Channel = 0;
                BOOLEAN Slot = 0;
                BOOLEAN CS = 0;
                while (ChBits)
                {
                    Channel = !Channel;
                    ChBits = ChBits & (ChBits - 1);
                }
                while (SlBits)
                {
                    Slot = !Slot;
                    SlBits = SlBits & (SlBits - 1);
                }
                while (CSBits)
                {
                    CS = !CS;
                    CSBits = CSBits & (CSBits - 1);
                }
                UnicodeSPrint(Buf, BufSize, L"/%s-%d-%d", Channel ? L"B" : L"A", Slot, CS);
            }

            switch (ErrRec->ErrDataSize)
            {
            case 4:
                ebits = *((UINT32 *)&ErrRec->ErrInfo.DataErr.ErrorBits);
                break;
            case 8:
                ebits = *((UINT64 *)&ErrRec->ErrInfo.DataErr.ErrorBits);
                break;
            case 16:
            {
                UINT64 ErrorBits[2];
                m128i_to_u64_array(&ErrRec->ErrInfo.DataErr.ErrorBits, ErrorBits);

                ebits = ErrorBits[0];
                ebits |= ErrorBits[1];
            }
            break;
            }

            if (decode_addr(ErrRec->AddrInfo.PhysAddr, ebits, ErrRec->ErrDataSize, MtSupportGetMemoryFormFactor(), NULL, &ch, &sl, NULL, &chip, NULL, NULL, NULL, NULL) == 0)
            {
                CHAR16 SlotStr[8];
                MtSupportGetSlotLabel(MC2SMBIOSSlot(ch, sl), FALSE, SlotStr, sizeof(SlotStr));
                UnicodeSPrint(Buf, BufSize, L"/%s", SlotStr);

                if (chip >= 0 && chip < MAX_CHIPS)
                {
                    CHAR16 ChipStr[8];
                    UnicodeCatf(Buf, BufSize, L"-%s", MtSupportGetChipLabel(chip, ChipStr, sizeof(ChipStr)));
                }
            }
        }

        if (pchan)
            *pchan = ch;
        if (pslot)
            *pslot = sl;
        if (pchip)
            *pchip = chip;
    }
}

VOID GetECCSyndStr(struct ERRLINE *ErrRec, CHAR16 *Buf, UINTN BufSize)
{
    CHAR16 TempBuf[16];

    SetMem(Buf, BufSize, 0);

    if (mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.syn < 0)
        StrCpyS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
    else
        UnicodeSPrint(Buf, BufSize, L"%04X", mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.syn);
}

CHAR16 *
GetECCTypeLangString(UINT8 ECCType, int LangId, CHAR16 *Buf, UINTN BufSize)
{
    SetMem(Buf, BufSize, 0);

    switch (ECCType)
    {
    case ECC_TYPE_READWRITE:
        GetStringFromPkg(LangId, STRING_TOKEN(STR_ECC_ERRORS), Buf, BufSize);
        break;
    case ECC_TYPE_INBAND:
        GetStringFromPkg(LangId, STRING_TOKEN(STR_IBECC_ERRORS), Buf, BufSize);
        break;
    case ECC_TYPE_SCRUB:
        GetStringFromPkg(LangId, STRING_TOKEN(STR_SCRUB_ECC_ERRORS), Buf, BufSize);
        break;
    default:
        break;
    }
    return Buf;
}

CHAR16 *
GetECCTypeString(UINT8 ECCType, CHAR16 *Buf, UINTN BufSize)
{
    return GetECCTypeLangString(ECCType, gCurLang, Buf, BufSize);
}

CHAR16 *
GetECCTypeStringEN(UINT8 ECCType, CHAR16 *Buf, UINTN BufSize)
{
    return GetECCTypeLangString(ECCType, 0, Buf, BufSize);
}

VOID GetDimmAddrStr(struct ERRLINE *ErrRec, CHAR16 *Buf, UINTN BufSize)
{
    CHAR16 TempBuf[16];
    int socket = ErrRec->ErrInfo.ECCErr.socket;
    int chan = ErrRec->ErrInfo.ECCErr.chan;
    int dimm = ErrRec->ErrInfo.ECCErr.dimm;

    // Workaround for Supermicro H12SSL-NT
    // See https://cerb.passmark.com/profiles/ticket/HFH-44296-856
    if (AsciiStrCmp(gBaseBoardInfo.szProductName, "H12SSL-NT") == 0)
    {
        const int CH_MAP[] = {0, 1, 3, 2, 6, 7, 5, 4};
        const int MAX_CH = sizeof(CH_MAP) / sizeof(CH_MAP[0]);
        if (chan < MAX_CH)
            chan = CH_MAP[chan];
    }

    StrCpyS(Buf, BufSize / sizeof(Buf[0]), L"(");

    CHAR16 SocketBuf[16];
    if (socket >= 0)
        UnicodeSPrint(SocketBuf, sizeof(SocketBuf), L"N%d-", socket);
    else
        SocketBuf[0] = L'\0';

    if (chan < 0)
    {
        StrCatS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
        StrCatS(Buf, BufSize / sizeof(Buf[0]), L",");
    }
    else
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%s%d,", SocketBuf, chan);
        StrCatS(Buf, BufSize / sizeof(Buf[0]), TempBuf);
    }

    if (dimm < 0)
    {
        StrCatS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
        StrCatS(Buf, BufSize / sizeof(Buf[0]), L",");
    }
    else
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%x,", dimm);
        StrCatS(Buf, BufSize / sizeof(Buf[0]), TempBuf);
    }

    if (ErrRec->AddrInfo.dimm.rank < 0)
    {
        StrCatS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
        StrCatS(Buf, BufSize / sizeof(Buf[0]), L",");
    }
    else
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%x,", ErrRec->AddrInfo.dimm.rank);
        StrCatS(Buf, BufSize / sizeof(Buf[0]), TempBuf);
    }

    if (ErrRec->AddrInfo.dimm.bank < 0)
    {
        StrCatS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
        StrCatS(Buf, BufSize / sizeof(Buf[0]), L",");
    }
    else
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%x,", ErrRec->AddrInfo.dimm.bank);
        StrCatS(Buf, BufSize / sizeof(Buf[0]), TempBuf);
    }

    if (ErrRec->AddrInfo.dimm.row < 0)
    {
        StrCatS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
        StrCatS(Buf, BufSize / sizeof(Buf[0]), L",");
    }
    else
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%x,", ErrRec->AddrInfo.dimm.row);
        StrCatS(Buf, BufSize / sizeof(Buf[0]), TempBuf);
    }

    if (ErrRec->AddrInfo.dimm.col < 0)
    {
        StrCatS(Buf, BufSize / sizeof(Buf[0]), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
        StrCatS(Buf, BufSize / sizeof(Buf[0]), L")");
    }
    else
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%x)", ErrRec->AddrInfo.dimm.col);
        StrCatS(Buf, BufSize / sizeof(Buf[0]), TempBuf);
    }
}

// MtSupportUpdateErrorDisp
//
// Display any errors in the error buffer to the UI
BOOLEAN
EFIAPI
MtSupportUpdateErrorDisp()
{
    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[128];
    BOOLEAN bUpdated = FALSE;

    // Go through the error circular buffer and display any new errors
    AcquireSpinLock(&mErrorLock);

    UINTN TotalErrCount = MtSupportGetNumErrors() + MtSupportGetNumCorrECCErrors() + MtSupportGetNumUncorrECCErrors();

    // Update error count in the UI
    MtUiSetErrorCount(MtSupportGetNumErrors());

    for (; mErrorBufHead != mErrorBufTail; mErrorBufHead = (mErrorBufHead + 1) % mErrorBufSize)
    {
        int ch = -1, sl = -1, chip = -1;
        CHAR16 ChBuf[16];
        CHAR16 DimmAddrBuf[128];
        DimmAddrBuf[0] = L'\0';

        gBuffer[0] = L'\0';

        gRT->GetTime(&mErrorBuf[mErrorBufHead].Time, NULL);
        if (mErrorBuf[mErrorBufHead].AddrType == 0)
        {
            UINT8 SocketId = 0xff;
            UINT8 MemoryControllerId = 0xff;
            UINT8 ChannelId = 0xff;
            UINT8 DimmSlot = 0xff;
            UINT8 DimmRank = 0xff;

            if (iMSSystemAddressToDimmAddress(mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, &SocketId, &MemoryControllerId, &ChannelId, &DimmSlot, &DimmRank) == EFI_SUCCESS)
                UnicodeSPrint(DimmAddrBuf, sizeof(DimmAddrBuf), L"{Socket: %d, Ctrl: %d, ChId: %d, DimmSlot: %d, DimmRank: %d}", SocketId, MemoryControllerId, ChannelId, DimmSlot, DimmRank);
        }

        GetErrorSlotStr(&mErrorBuf[mErrorBufHead], &ch, &sl, &chip, ChBuf, sizeof(ChBuf));

        if (mErrorBuf[mErrorBufHead].Type == 2)
        {
            CHAR16 SynBuf[16];

            GetECCSyndStr(&mErrorBuf[mErrorBufHead], SynBuf, sizeof(SynBuf));

            /* ECC error */
            if (mErrorBuf[mErrorBufHead].AddrType == 0)
            {
                MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ECC_ERRORS_INFO_PHYSADDR), TempBuf, sizeof(TempBuf)), GetECCTypeString(mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)), mErrorBuf[mErrorBufHead].TestNo, (UINT64)mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, ChBuf);
                AsciiSPrint(gBuffer, BUF_SIZE, "[MEM ERROR - %s] Test: %d, Address: %lx%s, ECC Corrected: %s, Syndrome: %s, Channel/Slot: %s", GetECCTypeStringEN(mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)), mErrorBuf[mErrorBufHead].TestNo, (UINT64)mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, DimmAddrBuf, mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.corrected ? L"yes" : L"no", SynBuf, ChBuf);
            }
            else
            {
                CHAR16 AddrBuf[32];
                GetDimmAddrStr(&mErrorBuf[mErrorBufHead], AddrBuf, sizeof(AddrBuf));

                // MtUiPrint(L"%3d   (%x,%x,%x,%x)        %s    %04X   ECC   %2d   %4d", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].AddrInfo.dimm.col, mErrorBuf[mErrorBufHead].AddrInfo.dimm.row, mErrorBuf[mErrorBufHead].AddrInfo.dimm.rank, mErrorBuf[mErrorBufHead].AddrInfo.dimm.bank, mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.corrected ? L"  corrected" : L"uncorrected", mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.syn, mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.chan, mErrorBuf[mErrorBufHead].ProcNum);
                if (mErrorBuf[mErrorBufHead].AddrInfo.dimm.col < 0 ||
                    mErrorBuf[mErrorBufHead].AddrInfo.dimm.row < 0 ||
                    mErrorBuf[mErrorBufHead].AddrInfo.dimm.rank < 0 ||
                    mErrorBuf[mErrorBufHead].AddrInfo.dimm.bank < 0)
                    MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ECC_ERRORS_INFO_SLOT), TempBuf, sizeof(TempBuf)), GetECCTypeString(mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)), mErrorBuf[mErrorBufHead].TestNo, ChBuf);
                else
                    MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_ECC_ERRORS_INFO_DIMM), TempBuf, sizeof(TempBuf)), GetECCTypeString(mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)), mErrorBuf[mErrorBufHead].TestNo, AddrBuf);
                AsciiSPrint(gBuffer, BUF_SIZE, "[MEM ERROR - %s] Test: %d, (Chan,Slot,Rank,Bank,Row,Col): %s, ECC Corrected: %s, Syndrome: %s, Channel/Slot: %s", GetECCTypeStringEN(mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)), mErrorBuf[mErrorBufHead].TestNo, AddrBuf, mErrorBuf[mErrorBufHead].ErrInfo.ECCErr.corrected ? L"yes" : L"no", SynBuf, ChBuf);
            }

            if (AsciiStrCmp(gBaseBoardInfo.szProductName, "H12SSL-NT") == 0)
            {
                CHAR8 AsciiBuf[64];
                AsciiSPrint(AsciiBuf, sizeof(AsciiBuf), " ** \"%a\" requires IMC channel remapping **", gBaseBoardInfo.szProductName);
                AsciiStrCatS(gBuffer, sizeof(gBuffer) / sizeof(gBuffer[0]), AsciiBuf);
            }
        }
        else if (mErrorBuf[mErrorBufHead].Type == 1)
        {
            // MtUiPrint(L"%3d   % 16lx         Parity error detected           %4d", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, mErrorBuf[mErrorBufHead].ProcNum);
            MtUiPrint(EFI_LIGHTRED, L"[Parity] Test: %d Addr: %lx CPU: %d", mErrorBuf[mErrorBufHead].TestNo, (UINT64)mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, mErrorBuf[mErrorBufHead].ProcNum);
            AsciiSPrint(gBuffer, BUF_SIZE, "[MEM ERROR - Parity] Test: %d, CPU: %d, Address: %lx%s", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].ProcNum, (UINT64)mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, DimmAddrBuf);
        }
        else
        {
            switch (mErrorBuf[mErrorBufHead].ErrDataSize)
            {
            case 4:
            {
                UINT32 *pAct = (UINT32 *)&mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Actual;
                UINT32 *pExp = (UINT32 *)&mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Expected;

                // MtUiPrint(L"%3d   % 16lx           %08X           %08X   %4d", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Expected, mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Actual, mErrorBuf[mErrorBufHead].ProcNum);
                MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_MEM_ERRORS_INFO_32BIT), TempBuf, sizeof(TempBuf)), mErrorBuf[mErrorBufHead].TestNo, (UINT64)mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, ChBuf, *pExp, *pAct, mErrorBuf[mErrorBufHead].ProcNum);
                AsciiSPrint(gBuffer, BUF_SIZE, "[MEM ERROR - Data] Test: %d, CPU: %d, Address: %lx%s%s, Expected: %08X, Actual: %08X", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].ProcNum, (UINT64)mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, ChBuf, DimmAddrBuf, *pExp, *pAct);
            }
            break;
            case 8:
            {
                UINT64 *pAct = (UINT64 *)&mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Actual;
                UINT64 *pExp = (UINT64 *)&mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Expected;

                // MtUiPrint(L"%3d   % 16lx   %016lX   %016lX   %4d", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Expected, mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Actual, mErrorBuf[mErrorBufHead].ProcNum);
                MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_MEM_ERRORS_INFO_64BIT), TempBuf, sizeof(TempBuf)), mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, ChBuf, *pExp, *pAct, mErrorBuf[mErrorBufHead].ProcNum);
                AsciiSPrint(gBuffer, BUF_SIZE, "[MEM ERROR - Data] Test: %d, CPU: %d, Address: %lx%s%s, Expected: %016lX, Actual: %016lX", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].ProcNum, mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, ChBuf, DimmAddrBuf, *pExp, *pAct);
            }
            break;
            case 16:
            {
                int i;

                UINT64 Actual[2];
                UINT64 Expected[2];
                m128i_to_u64_array(&mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Actual, Actual);
                m128i_to_u64_array(&mErrorBuf[mErrorBufHead].ErrInfo.DataErr.Expected, Expected);

                for (i = 0; i < 2; i++)
                {
                    if (Actual[i] != Expected[i])
                    {
                        MtUiPrint(EFI_LIGHTRED, GetStringById(STRING_TOKEN(STR_MEM_ERRORS_INFO_64BIT), TempBuf, sizeof(TempBuf)), mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr + i * sizeof(UINT64), ChBuf, Expected[i], Actual[i], mErrorBuf[mErrorBufHead].ProcNum);
                    }
                }

                AsciiSPrint(gBuffer, BUF_SIZE, "[MEM ERROR - Data] Test: %d, CPU: %d, Address: %lx%s%s, Expected: %016lX%016lX, Actual: %016lX%016lX", mErrorBuf[mErrorBufHead].TestNo, mErrorBuf[mErrorBufHead].ProcNum, mErrorBuf[mErrorBufHead].AddrInfo.PhysAddr, ChBuf, DimmAddrBuf, Expected[0], Expected[1], Actual[0], Actual[1]);
            }
            break;
            }

            if (ch >= 0 && sl >= 0)
            {
                int slot = MC2SMBIOSSlot(ch, sl);
                if (slot < MAX_SLOTS)
                {
                    erri.slot_err[slot]++;

                    if (chip >= 0)
                    {
                        if (chip < MAX_CHIPS)
                            erri.slot_chip_err[slot][chip]++;
                    }
                }
            }
            else
                erri.unknown_slot_err++;
        }

        // Log to file
        if (mTruncateErrLog == FALSE)
        {
            MtSupportDebugWriteLine(gBuffer);

            if (TotalErrCount >= 500)
            {
                // AsciiFPrint(DEBUG_FILE_HANDLE, "[MEM ERROR] Truncating due to too many errors (Total errs: %d)", TotalErrCount);
                mTruncateErrLog = TRUE;
            }
        }
        bUpdated = TRUE;
    }
    ReleaseSpinLock(&mErrorLock);
    return bUpdated;
}

// MtSupportUpdateWarningDisp
//
// Display any warning in the warning buffer to the UI
VOID
    EFIAPI
    MtSupportUpdateWarningDisp()
{
    if (mWarningBufHead == mWarningBufTail)
        return;

    // Go through the warning circular buffer and log them to the log file
    AcquireSpinLock(&mErrorLock);
    for (; mWarningBufHead != mWarningBufTail; mWarningBufHead = (mWarningBufHead + 1) % mWarningBufSize)
    {
        CHAR16 ChBuf[16];

        CHAR16 DimmAddrBuf[128];
        DimmAddrBuf[0] = L'\0';

        gRT->GetTime(&mWarningBuf[mWarningBufHead].Time, NULL);
        if (mWarningBuf[mWarningBufHead].AddrType == 0)
        {
            UINT8 SocketId = 0xff;
            UINT8 MemoryControllerId = 0xff;
            UINT8 ChannelId = 0xff;
            UINT8 DimmSlot = 0xff;
            UINT8 DimmRank = 0xff;

            if (iMSSystemAddressToDimmAddress(mWarningBuf[mWarningBufHead].AddrInfo.PhysAddr, &SocketId, &MemoryControllerId, &ChannelId, &DimmSlot, &DimmRank) == EFI_SUCCESS)
                UnicodeSPrint(DimmAddrBuf, sizeof(DimmAddrBuf), L"{Socket: %d, Ctrl: %d, ChId: %d, DimmSlot: %d, DimmRank: %d}", SocketId, MemoryControllerId, ChannelId, DimmSlot, DimmRank);
        }

        SetMem(ChBuf, sizeof(ChBuf), 0);
        if (gAddr2ChBits != 0 || gAddr2SlBits != 0 || gAddr2CSBits != 0)
        {
            UINT64 ChBits = gAddr2ChBits & (UINT64)mWarningBuf[mWarningBufHead].AddrInfo.PhysAddr;
            UINT64 SlBits = gAddr2SlBits & (UINT64)mWarningBuf[mWarningBufHead].AddrInfo.PhysAddr;
            UINT64 CSBits = gAddr2CSBits & (UINT64)mWarningBuf[mWarningBufHead].AddrInfo.PhysAddr;
            BOOLEAN Channel = 0;
            BOOLEAN Slot = 0;
            BOOLEAN CS = 0;
            while (ChBits)
            {
                Channel = !Channel;
                ChBits = ChBits & (ChBits - 1);
            }
            while (SlBits)
            {
                Slot = !Slot;
                SlBits = SlBits & (SlBits - 1);
            }
            while (CSBits)
            {
                CS = !CS;
                CSBits = CSBits & (CSBits - 1);
            }
            UnicodeSPrint(ChBuf, sizeof(ChBuf), L"/%s%d%d", Channel ? L"B" : L"A", Slot, CS);
        }

        switch (mWarningBuf[mWarningBufHead].ErrDataSize)
        {
        case 4:
        {
            UINT32 *pAct = (UINT32 *)&mWarningBuf[mWarningBufHead].ErrInfo.DataErr.Actual;
            UINT32 *pExp = (UINT32 *)&mWarningBuf[mWarningBufHead].ErrInfo.DataErr.Expected;

            AsciiSPrint(gBuffer, BUF_SIZE, "[MEM WARNING] Test: %d, CPU: %d, Address: %lx%s%s, Expected: %08X, Actual: %08X", mWarningBuf[mWarningBufHead].TestNo, mWarningBuf[mWarningBufHead].ProcNum, (UINT64)mWarningBuf[mWarningBufHead].AddrInfo.PhysAddr, ChBuf, DimmAddrBuf, *pExp, *pAct);
        }
        break;
        case 8:
        {
            UINT64 *pAct = (UINT64 *)&mWarningBuf[mWarningBufHead].ErrInfo.DataErr.Actual;
            UINT64 *pExp = (UINT64 *)&mWarningBuf[mWarningBufHead].ErrInfo.DataErr.Expected;
            AsciiSPrint(gBuffer, BUF_SIZE, "[MEM WARNING] Test: %d, CPU: %d, Address: %lx%s%s, Expected: %016lX, Actual: %016lX", mWarningBuf[mWarningBufHead].TestNo, mWarningBuf[mWarningBufHead].ProcNum, mWarningBuf[mWarningBufHead].AddrInfo.PhysAddr, ChBuf, DimmAddrBuf, *pExp, *pAct);
        }
        break;
        case 16:
        {
            UINT64 Actual[2];
            UINT64 Expected[2];
            m128i_to_u64_array(&mWarningBuf[mWarningBufHead].ErrInfo.DataErr.Actual, Actual);
            m128i_to_u64_array(&mWarningBuf[mWarningBufHead].ErrInfo.DataErr.Expected, Expected);

            AsciiSPrint(gBuffer, BUF_SIZE, "[MEM WARNING] Test: %d, CPU: %d, Address: %lx%s%s, Expected: %016lX%016lX, Actual: %016lX%016lX", mWarningBuf[mWarningBufHead].TestNo, mWarningBuf[mWarningBufHead].ProcNum, mWarningBuf[mWarningBufHead].AddrInfo.PhysAddr, ChBuf, DimmAddrBuf, Expected[0], Expected[1], Actual[0], Actual[1]);
            break;
        }
        }
        // Log to file
        MtSupportDebugWriteLine(gBuffer);
    }
    ReleaseSpinLock(&mErrorLock);
}

#define COMBINE_MASK(addr1, mask1, addr2, mask2) (((addr1 & mask1) & (addr2 & mask2)) | ((~addr1 & mask1) & (~addr2 & mask2)))
#define MASK_SIZE(mask) (((UINTN) - 1) - (UINTN)mask + 1)

/* Find the cheapest array index to extend with the given adr/mask pair.
 * Return -1 if nothing below the given minimum cost can be found.
 */
int find_cheapest_index(UINTN adr1, UINTN mask1, UINTN mincost, int excludeidx)
{
    int i = mNumBADRAMPatn;
    int idx = -1;
    while (i-- > 0)
    {
        UINTN origcost = MASK_SIZE(mBADRAMPatn[i].mask);
        UINTN mask = COMBINE_MASK(mBADRAMPatn[i].addr, mBADRAMPatn[i].mask, adr1, mask1);
        UINTN newcost = MASK_SIZE(mask) - origcost;
        if (newcost < mincost && i != excludeidx)
        {
            mincost = newcost;
            idx = i;
        }
    }
    return idx;
}

void try_combine_patn(int idx)
{
    // find any existing pattern/mask that we can combine with
    int curidx = idx;
    int newidx;
    while ((newidx = find_cheapest_index(mBADRAMPatn[curidx].addr, mBADRAMPatn[curidx].mask, 1 + MASK_SIZE(mBADRAMPatn[curidx].mask), curidx)) >= 0)
    {
        // combine the 2 pattern/masks
        UINTN cadr = mBADRAMPatn[newidx].addr | mBADRAMPatn[curidx].addr;
        UINTN cmask = COMBINE_MASK(mBADRAMPatn[newidx].addr, mBADRAMPatn[newidx].mask,
                                   mBADRAMPatn[curidx].addr, mBADRAMPatn[curidx].mask);
        newidx = MIN(newidx, curidx); // new slot
        curidx = MAX(newidx, curidx); // extra slot
        mBADRAMPatn[newidx].addr = cadr & cmask;
        mBADRAMPatn[newidx].mask = cmask;

        // we can remove the extra slot. Replace with the last slot
        mBADRAMPatn[curidx].addr = mBADRAMPatn[mNumBADRAMPatn - 1].addr;
        mBADRAMPatn[curidx].mask = mBADRAMPatn[mNumBADRAMPatn - 1].mask;

        mNumBADRAMPatn--;

        // keep going until we can't combine anymore
        curidx = newidx;
    }
}

VOID
    EFIAPI
    add_badram_address(UINTN adr, UINTN mask)
{
    if (find_cheapest_index(adr, mask, 1, -1) != -1)
        return;

    if (mNumBADRAMPatn < BADRAM_MAXPATNS)
    {
        mBADRAMPatn[mNumBADRAMPatn].addr = adr;
        mBADRAMPatn[mNumBADRAMPatn].mask = mask;

        mNumBADRAMPatn++;
        try_combine_patn(mNumBADRAMPatn - 1);
    }
    else
    {
        int idx = find_cheapest_index(adr, mask, ~(UINTN)0, -1);
        UINTN cadr = mBADRAMPatn[idx].addr | adr;
        UINTN cmask = COMBINE_MASK(mBADRAMPatn[idx].addr, mBADRAMPatn[idx].mask,
                                   adr, mask);
        mBADRAMPatn[idx].addr = cadr & cmask;
        mBADRAMPatn[idx].mask = cmask;
        try_combine_patn(idx);
    }
}

CHAR16 *
    EFIAPI
    get_badram_string(CHAR16 *StrBuf, UINTN StrBufSize)
{
    int i;
    StrBuf[0] = L'\0';
    for (i = 0; i < mNumBADRAMPatn; i++)
    {
        CHAR16 TempBuf[64];

        if (sizeof(void *) > 4)
        {
            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"0x%015lx,0x%015lx", mBADRAMPatn[i].addr, mBADRAMPatn[i].mask);
        }
        else
        {
            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"0x%08x,0x%08x", mBADRAMPatn[i].addr, mBADRAMPatn[i].mask);
        }
        if (i > 0)
            StrCatS(StrBuf, StrBufSize, L",");

        StrCatS(StrBuf, StrBufSize, TempBuf);
    }
    return StrBuf;
}

CHAR16 *
    EFIAPI
    get_badaddresslist_string(CHAR16 *StrBuf, UINTN StrBufSize)
{
    int i;
    StrBuf[0] = L'\0';
    for (i = 0; i < mBadPFNListSize; i++)
    {
        CHAR16 TempBuf[64];

        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"0x%p", mBadPFNList[i]);
        if (i > 0)
            StrCatS(StrBuf, StrBufSize, L" ");

        StrCatS(StrBuf, StrBufSize, TempBuf);
    }
    return StrBuf;
}

VOID
    EFIAPI
    add_bad_pfn(UINTN pfn)
{
    int i;
    if (mBadPFNListSize >= MAXBADPFN)
        return;

    for (i = 0; i < mBadPFNListSize; i++)
    {
        if (mBadPFNList[i] == pfn)
            return;
    }

    mBadPFNList[mBadPFNListSize++] = pfn;
}

#if 0 // not used
// process_err
// 
// Process the memory error
VOID
EFIAPI
process_err(
    IN UINTN                    ProcNum,
    IN UINTN                    Address,
    IN UINT32                   TestNo,
    IN UINTN                    Actual,
    IN UINTN                    Expected,
    IN int                      Type
)
{
    int i;
    short n;
    UINTN page, offset, addr;
    UINTN mb;
    UINTN xor;
    int len = 1;

    update_err_counts(TestNo);

    xor = Expected ^ Actual;

    page = PAGE_OF(Address);
    offset = Address & 0xFFF;

    if (mErrorsAsWarnings == FALSE)
    {
        /* Calc upper and lower error addresses */
        if (erri.low_addr > Address) {
            erri.low_addr = Address;
        }
        if (erri.high_addr < Address) {
            erri.high_addr = Address;
        }

        /* Calc bits in error */
        for (i = 0, n = 0; i < sizeof(void*) * 8; i++) {
            if (xor >> i & 1) {
                n++;
            }
        }

        erri.tbits += n;

        if (n > erri.max_bits) {
            erri.max_bits = n;
        }
        if (n < erri.min_bits) {
            erri.min_bits = n;
        }
        if (erri.ebits ^ xor) {
        }
        erri.ebits |= xor;

        /* Calc max contig errors */
        len = 1;
        if (Address == erri.eadr + 4 ||
            Address == erri.eadr - 4) {
            len++;
        }
        if (len > erri.maxl) {
            erri.maxl = len;
        }

        addr = page << 12 | offset;
        mb = page >> 8;

        /* Don't display duplicate errors */
        if (Address == erri.eadr &&
            (unsigned long long)xor == *((unsigned long long *)(&erri.exor)) &&
            TestNo == erri.etest) {
            return;
        }

        erri.eadr = Address;
        erri.etest = TestNo;

        add_badram_address(Address & ~((UINTN)EFI_PAGE_SIZE - 1), ~((UINTN)EFI_PAGE_SIZE -1));
        add_bad_pfn(page);
    }

    if (Type == 1) {
        AddParityErrorToBuf(TestNo, ProcNum, Address);
    }
    else {
        if (mErrorsAsWarnings == FALSE)
        {
            AddDataErrorToBuf(TestNo, ProcNum, Address, Actual, Expected, xor);
            *((unsigned long long*)(&erri.exor)) = (unsigned long long)xor;
        }
        else
        {
            AddDataWarningToBuf(TestNo, ProcNum, Address, Actual, Expected, xor);
        }
    }

}
#endif

#ifdef __GNUC__
#define __popcnt __builtin_popcount
#ifdef MDE_CPU_X64
#define __popcnt64 __builtin_popcountll
#endif
#endif

#ifdef MDE_CPU_IA32
static __inline unsigned long long __popcnt64(
    unsigned long long value)
{
    return __popcnt(*((unsigned int *)&value)) + __popcnt(*(((unsigned int *)&value) + 1));
}
#endif

static __inline int popcnt128(__m128i *n)
{
#ifdef MDE_CPU_X64
    return (int)__popcnt64(_mm_extract_epi64(*n, 0)) + (int)__popcnt64(_mm_extract_epi64(*n, 1));
#elif defined(MDE_CPU_IA32)
    return (int)__popcnt(_mm_extract_epi32(*n, 0)) + (int)__popcnt(_mm_extract_epi32(*n, 1)) + (int)__popcnt(_mm_extract_epi32(*n, 2)) + (int)__popcnt(_mm_extract_epi32(*n, 3));
#elif defined(MDE_CPU_AARCH64)
    int c = 0;
    __asm__("LD1 {v0.2D}, [%1]            \n\t"
            "CNT v0.16b, v0.16b               \n\t"
            "UADDLV h1, v0.16b                \n\t"
            "UMOV x0, v1.d[0]                 \n\t"
            "ADD %0, x0, %0                   \n\t"
            : "+r"(c), "+r"(n)::"x0", "v0", "v1");
    return c;
#endif
}

static __inline int numsetbits32(unsigned int i)
{
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    if (((cpu_id.fid.uint32_array[1] >> 23) & 0x01) == 0)
#endif
    {
        // Java: use >>> instead of >>
        // C or C++: use uint32_t
        i = i - ((i >> 1) & 0x55555555);
        i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
        return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
    }
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    return __popcnt(i);
#endif
}

static __inline int numsetbits64(unsigned long long i)
{
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    if (((cpu_id.fid.uint32_array[1] >> 23) & 0x01) == 0)
#endif
    {
        i = i - ((i >> 1) & 0x5555555555555555ULL);
        i = (i & 0x3333333333333333ULL) + ((i >> 2) & 0x3333333333333333ULL);
        return (int)(MultU64x64(((i + (i >> 4)) & 0xF0F0F0F0F0F0F0FULL), 0x101010101010101ULL) >> 56);
    }
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    return (int)__popcnt64(i);
#endif
}

static __inline int numsetbits128(__m128i *n)
{
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    if (((cpu_id.fid.uint32_array[1] >> 23) & 0x01) == 0)
    {
#if defined(MDE_CPU_IA32)
        return numsetbits32(_mm_extract_epi32(*n, 0)) + numsetbits32(_mm_extract_epi32(*n, 1)) + numsetbits32(_mm_extract_epi32(*n, 2)) + numsetbits32(_mm_extract_epi32(*n, 3));
#elif defined(MDE_CPU_X64)
        return numsetbits64(_mm_extract_epi64(*n, 0)) + numsetbits64(_mm_extract_epi64(*n, 1));
#endif
    }
#endif
    return (int)popcnt128(n);
}

// process_err_varsize
//
// Process the memory error
VOID
    EFIAPI
    process_err_varsize(
        IN UINTN ProcNum,
        IN UINTN Address,
        IN UINT32 TestNo,
        IN void *Actual,
        IN void *Expected,
        IN UINT32 DataSize,
        IN int Type)
{
    short n;
    UINTN page, offset, addr;
    UINTN mb;

    UINT32 actual32;
    UINT64 actual64;

    UINT32 expected32;
    UINT64 expected64;

    ALIGN(16)
    unsigned int xor128[8];
    __m128i *pxor128 = (__m128i *)(((UINTN)xor128 + 0xF) & ~(0xF));

    int len = 1;

    if (MtSupportGetNumErrors() >= gMaxErrCount)
        return;

    SetMem(xor128, sizeof(xor128), 0);

    update_err_counts(TestNo);

    switch (DataSize)
    {
    case 4:
    {
        actual32 = *((UINT32 *)Actual);
        expected32 = *((UINT32 *)Expected);
        UINT32 xor32 = actual32 ^ expected32;

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
        *pxor128 = _mm_setr_epi32(xor32, 0, 0, 0);

#elif defined(MDE_CPU_AARCH64)
        *((UINT32 *)pxor128) = xor32;
#endif
        /* Calc bits in error */
        n = (short)numsetbits32(xor32);
    }
    break;
    case 8:
    {
        actual64 = *((UINT64 *)Actual);
        expected64 = *((UINT64 *)Expected);
        UINT64 xor64 = actual64 ^ expected64;

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
        *pxor128 = _mm_set_epi64x(0, xor64);
#elif defined(MDE_CPU_AARCH64)
        *((UINT64 *)pxor128) = xor64;
#endif
        /* Calc bits in error */
        n = (short)numsetbits64(xor64);
    }
    break;
    case 16:
    {
        *pxor128 = _mm_xor_si128(*((__m128i *)Actual), *((__m128i *)Expected));
        /* Calc bits in error */
        n = (short)numsetbits128(pxor128);
    }
    break;
    default:
        return;
    }

    page = PAGE_OF(Address);
    offset = Address & 0xFFF;

    if (mErrorsAsWarnings == FALSE)
    {
        /* Calc upper and lower error addresses */
        if (erri.low_addr > Address)
        {
            erri.low_addr = Address;
        }
        if (erri.high_addr < Address)
        {
            erri.high_addr = Address;
        }

        switch (DataSize)
        {
        case 4:
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
            erri.ebits |= _mm_extract_epi32(*pxor128, 0);
#elif defined(MDE_CPU_AARCH64)
            erri.ebits |= *((UINT32 *)pxor128);
#endif
            break;
        case 8:
#if defined(MDE_CPU_X64)
            erri.ebits |= _mm_extract_epi64(*pxor128, 0);
#elif defined(MDE_CPU_IA32)
            erri.ebits |= _mm_extract_epi32(*pxor128, 0);
            erri.ebits |= _mm_extract_epi32(*pxor128, 1);
#elif defined(MDE_CPU_AARCH64)
            if (sizeof(void *) > 4)
                erri.ebits |= *((UINT64 *)pxor128);
            else
            {
                erri.ebits |= *((UINT32 *)pxor128);
                erri.ebits |= *((UINT32 *)pxor128 + 1);
            }
#endif
            break;
        case 16:
#if defined(MDE_CPU_X64)
            erri.ebits |= _mm_extract_epi64(*pxor128, 0);
            erri.ebits |= _mm_extract_epi64(*pxor128, 1);
#elif defined(MDE_CPU_IA32)
            erri.ebits |= _mm_extract_epi32(*pxor128, 0);
            erri.ebits |= _mm_extract_epi32(*pxor128, 1);
            erri.ebits |= _mm_extract_epi32(*pxor128, 2);
            erri.ebits |= _mm_extract_epi32(*pxor128, 3);
#elif defined(MDE_CPU_AARCH64)
            if (sizeof(void *) > 4)
            {
                erri.ebits |= *((UINT64 *)pxor128);
                erri.ebits |= *((UINT64 *)pxor128 + 1);
            }
            else
            {
                erri.ebits |= *((UINT32 *)pxor128);
                erri.ebits |= *((UINT32 *)pxor128 + 1);
                erri.ebits |= *((UINT32 *)pxor128 + 2);
                erri.ebits |= *((UINT32 *)pxor128 + 3);
            }
#endif
            break;
        default:
            return;
        }
        erri.tbits += n;

        if (n > erri.max_bits)
        {
            erri.max_bits = n;
        }
        if (n < erri.min_bits)
        {
            erri.min_bits = n;
        }

        /* Calc max contig errors */
        len = 1;
        if (Address == erri.eadr + DataSize ||
            Address == erri.eadr - DataSize)
        {
            len++;
        }
        if (len > erri.maxl)
        {
            erri.maxl = len;
        }

        addr = page << 12 | offset;
        mb = page >> 8;

        /* Don't display duplicate errors */
        if (Address == erri.eadr &&
            CompareMem(pxor128, &erri.exor, sizeof(*pxor128)) == 0 &&
            TestNo == erri.etest)
        {
            return;
        }

        int idx = (int)(ProcNum / (sizeof(erri.ecpus[0]) * 8));
        int bit = ProcNum % (sizeof(erri.ecpus[0]) * 8);
        erri.ecpus[idx] |= LShiftU64(1ULL, bit);
        erri.eadr = Address;
        erri.etest = TestNo;
        add_badram_address(Address & ~((UINTN)EFI_PAGE_SIZE - 1), ~((UINTN)EFI_PAGE_SIZE - 1));
        add_bad_pfn(page);
    }

    if (Type == 1)
    {
        AddParityErrorToBuf(TestNo, ProcNum, Address);
    }
    else
    {
        if (mErrorsAsWarnings == FALSE)
        {
            AddDataErrorToBufVarSize(TestNo, ProcNum, Address, Actual, Expected, pxor128, DataSize);
            CopyMem(&erri.exor, pxor128, sizeof(erri.exor));
        }
        else
        {
            AddDataWarningToBufVarSize(TestNo, ProcNum, Address, Actual, Expected, pxor128, DataSize);
        }
    }
}

// process_ecc_err
//
// Process the ECC memory error
VOID
    EFIAPI
    process_ecc_err(
        IN UINTN ProcNum,
        IN UINTN Address,
        IN UINT32 TestNo,
        IN UINT8 ECCType,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm)
{
    if (Corrected)
        erri.cor_err++;
    else
        erri.uncor_err++;

    AddECCErrorToBuf(TestNo, ProcNum, Address, ECCType, Corrected, Syndrome, socket, channel, dimm);
}

// process_ecc_err_dimm
//
// Process the ECC memory error
VOID
    EFIAPI
    process_ecc_err_dimm(
        IN UINTN ProcNum,
        IN INT32 Col,
        IN INT32 Row,
        IN INT8 Bank,
        IN INT8 Rank,
        IN INT32 TestNo,
        IN UINT8 ECCType,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm)
{

    if (Corrected)
        erri.cor_err++;
    else
        erri.uncor_err++;

    AddECCErrorToBuf_dimm(TestNo, ProcNum, Col, Row, Bank, Rank, ECCType, Corrected, Syndrome, socket, channel, dimm);
}

// MtSupportReportError
//
// Called by the executing CPU when a memory error is detected
VOID
    EFIAPI
    MtSupportReportError(
        IN UINTN Address,
        IN UINT32 TestNo,
        IN UINTN Actual,
        IN UINTN Expected,
        IN int Type)
{
    if (mErrorsIgnore)
        return;

    UINTN ProcNum = MPSupportWhoAmI();
    BOOLEAN Processed = FALSE;
    int BufFullCount = 0;

    while (Processed == FALSE)
    {
        AcquireSpinLock(&mErrorLock);

        if (!IsErrorBufFull())
        {
            process_err_varsize(ProcNum, Address, TestNo, &Actual, &Expected, sizeof(UINTN), Type);
            ReleaseSpinLock(&mErrorLock);
            Processed = TRUE;
        }
        else
        {
            if (BufFullCount == 0)
                mErrorBufFullCount++;
            BufFullCount++;
            ReleaseSpinLock(&mErrorLock);

            if (ProcNum == MPSupportGetBspId())
            {
                MtSupportUpdateErrorDisp();
                MtSupportUpdateWarningDisp();
            }
            else
            {
                UINT64 end_ts = MtSupportReadTSC() + clks_msec;
                while (MtSupportReadTSC() < end_ts)
                    ;
            }
        }
    }
    if (ProcNum == MPSupportGetBspId())
    {
        MtSupportUpdateErrorDisp();
        MtSupportUpdateWarningDisp();
    }
}

// MtSupportReportErrorVarSize
//
// Called by the executing CPU when a memory error is detected
VOID
    EFIAPI
    MtSupportReportErrorVarSize(
        IN UINTN Address,
        IN UINT32 TestNo,
        IN void *Actual,
        IN void *Expected,
        IN UINT32 DataSize,
        IN int Type)
{
    if (mErrorsIgnore)
        return;

    UINTN ProcNum = MPSupportWhoAmI();
    BOOLEAN Processed = FALSE;
    int BufFullCount = 0;

    while (Processed == FALSE)
    {
        AcquireSpinLock(&mErrorLock);

        if (!IsErrorBufFull())
        {
            process_err_varsize(ProcNum, Address, TestNo, Actual, Expected, DataSize, Type);
            ReleaseSpinLock(&mErrorLock);
            Processed = TRUE;
        }
        else
        {
            if (BufFullCount == 0)
                mErrorBufFullCount++;
            BufFullCount++;
            ReleaseSpinLock(&mErrorLock);

            if (ProcNum == MPSupportGetBspId())
            {
                MtSupportUpdateErrorDisp();
                MtSupportUpdateWarningDisp();
            }
            else
            {
                UINT64 end_ts = MtSupportReadTSC() + clks_msec;
                while (MtSupportReadTSC() < end_ts)
                    ;
            }
        }
    }

    if (ProcNum == MPSupportGetBspId())
    {
        MtSupportUpdateErrorDisp();
        MtSupportUpdateWarningDisp();
    }
}

VOID
    EFIAPI
    MtSupportReportECCErrorType(
        IN UINTN Address,
        IN UINT8 ECCType,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm)
{
    if (mErrorsIgnore)
        return;

    UINTN ProcNum = MPSupportGetBspId(); // MPSupportWhoAmI();
    UINT32 TestNo = mTests[mCurTestIdx].TestNo;
    BOOLEAN Processed = FALSE;
    int BufFullCount = 0;

    while (Processed == FALSE)
    {
        AcquireSpinLock(&mErrorLock);

        if (!IsErrorBufFull())
        {
            process_ecc_err(ProcNum, Address, TestNo, ECCType, Corrected, Syndrome, socket, channel, dimm);
            ReleaseSpinLock(&mErrorLock);
            Processed = TRUE;
        }
        else
        {
            if (BufFullCount == 0)
                mErrorBufFullCount++;
            BufFullCount++;
            ReleaseSpinLock(&mErrorLock);

            if (ProcNum == MPSupportGetBspId())
            {
                MtSupportUpdateErrorDisp();
            }
            else
            {
                UINT64 end_ts = MtSupportReadTSC() + clks_msec;
                while (MtSupportReadTSC() < end_ts)
                    ;
            }
        }
    }

    if (ProcNum == MPSupportGetBspId())
    {
        MtSupportUpdateErrorDisp();
    }
}

// MtSupportReportECCError
//
// Called by the executing CPU when an ECC memory error is detected
VOID
    EFIAPI
    MtSupportReportECCError(
        IN UINTN Address,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm)
{
    MtSupportReportECCErrorType(Address, ECC_TYPE_READWRITE, Corrected, Syndrome, socket, channel, dimm);
}

VOID
    EFIAPI
    MtSupportReportIBECCError(
        IN UINTN Address,
        IN BOOLEAN Corrected,
        IN INT32 Syndrome,
        IN INT8 socket,
        IN INT8 channel,
        IN INT8 dimm)
{
    MtSupportReportECCErrorType(Address, ECC_TYPE_INBAND, Corrected, Syndrome, socket, channel, dimm);
}

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
        IN INT8 dimm)
{
    if (mErrorsIgnore)
        return;

    UINTN ProcNum = MPSupportGetBspId(); // MPSupportWhoAmI();
    UINT32 TestNo = mTests[mCurTestIdx].TestNo;
    BOOLEAN Processed = FALSE;
    int BufFullCount = 0;

    while (Processed == FALSE)
    {
        AcquireSpinLock(&mErrorLock);

        if (!IsErrorBufFull())
        {
            process_ecc_err_dimm(ProcNum, Col, Row, Bank, Rank, TestNo, ECCType, Corrected, Syndrome, socket, channel, dimm);
            ReleaseSpinLock(&mErrorLock);
            Processed = TRUE;
        }
        else
        {
            if (BufFullCount == 0)
                mErrorBufFullCount++;
            BufFullCount++;
            ReleaseSpinLock(&mErrorLock);

            if (ProcNum == MPSupportGetBspId())
            {
                MtSupportUpdateErrorDisp();
            }
            else
            {
                UINT64 end_ts = MtSupportReadTSC() + clks_msec;
                while (MtSupportReadTSC() < end_ts)
                    ;
            }
        }
    }

    if (ProcNum == MPSupportGetBspId())
    {
        MtSupportUpdateErrorDisp();
    }
}

// MtSupportReportECCError_dimm
//
// Called by the executing CPU when an ECC memory error is detected
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
        IN INT8 dimm)
{
    MtSupportReportECCErrorType_dimm(Col, Row, Bank, Rank, ECC_TYPE_READWRITE, Corrected, Syndrome, socket, channel, dimm);
}

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
        IN INT8 dimm)
{
    MtSupportReportECCErrorType_dimm(Col, Row, Bank, Rank, ECC_TYPE_INBAND, Corrected, Syndrome, socket, channel, dimm);
}

// MtSupportSetErrorsAsWarnings
//
// Used during Test 13 Row hammer test. Mark all detected errors as warnings.
VOID
    EFIAPI
    MtSupportSetErrorsAsWarnings(BOOLEAN ErrorsAsWarnings)
{
    mErrorsAsWarnings = ErrorsAsWarnings;
}

// MtSupportClearECCErrors
//
// Clear any ECC errors pending in hardware
VOID
    EFIAPI
    MtSupportClearECCErrors()
{
    mErrorsIgnore = TRUE;
    poll_errors();
    mErrorsIgnore = FALSE;
}

// MtSupportReportCPUError
//
// Called when a CPU error is detected
VOID
    EFIAPI
    MtSupportReportCPUError(
        IN UINT32 TestNo,
        IN UINTN ProcNum)
{
    mCPUErrBuf[mCPUErrBufTail].Valid = TRUE;
    gRT->GetTime(&mCPUErrBuf[mCPUErrBufTail].Time, NULL);
    mCPUErrBuf[mCPUErrBufTail].TestNo = TestNo;
    mCPUErrBuf[mCPUErrBufTail].ProcNum = ProcNum;

    mCPUErrBufTail = (mCPUErrBufTail + 1) % mCPUErrBufSize;

    if (mCPUErrBufTail == mCPUErrBufHead)
        mCPUErrBufHead = (mCPUErrBufHead + 1) % mCPUErrBufSize;
}

// MtSupportGetNumErrors
//
// Get the number of errors in the last test run
UINT32
EFIAPI
MtSupportGetNumErrors()
{
#ifdef MEM_DECODE_DEBUG
    if (1)
        return 1023;
#endif
    return erri.data_err;
}

// MtSupportGetNumWarns
//
// Get the number of warnings in the last test run
UINT32
EFIAPI
MtSupportGetNumWarns()
{
    return erri.data_warn;
}

// MtSupportGetNumCorrECCErrors
//
// Get the number of corrected ECC errors in the last test run
UINT32
EFIAPI
MtSupportGetNumCorrECCErrors()
{
    return erri.cor_err;
}

// MtSupportGetNumUncorrECCErrors
//
// Get the number of uncorrected ECC errors in the last test run
UINT32
EFIAPI
MtSupportGetNumUncorrECCErrors()
{
    return erri.uncor_err;
}

UINT32
EFIAPI
MtSupportGetNumSlotErrors(int slot)
{
    if (slot < 0 || slot >= MAX_SLOTS)
        return 0;

#ifdef MEM_DECODE_DEBUG
    if (slot == 0 || slot == 3)
        return 1093;
#endif
    return erri.slot_err[slot];
}

UINT32
EFIAPI
MtSupportGetNumUnknownSlotErrors()
{
#ifdef MEM_DECODE_DEBUG
    return 33;
#endif
    return erri.unknown_slot_err;
}

UINT32
EFIAPI
MtSupportGetNumSlotChipErrors(int slot, int chip)
{
    if (slot < 0 || slot >= MAX_SLOTS)
        return 0;

    if (chip < 0 || chip >= MAX_CHIPS)
        return 0;

#ifdef MEM_DECODE_DEBUG
    if (slot == 0 && (chip == 0 || chip == 8))
        return 50;
    else if (slot == 0 && (chip == 3 || chip == 11))
        return 840;
    else if (slot == 0 && (chip == 6 || chip == 14))
        return 120;
    else if (slot == 3 && (chip == 1 || chip == 9))
        return 83;
#endif

    return erri.slot_chip_err[slot][chip];
}

EFI_STATUS
EFIAPI
MtSupportReportErrorAddrIMS()
{
    EFI_STATUS Status = EFI_SUCCESS;
    IMS_NVMEM_DEFECT_LIST iMSDefectList;
    UINT32 BufIdx = mErrorBufTail > 0 ? mErrorBufTail - 1 : mErrorBufSize - 1;
    UINTN logCount = 0;
    int NumDefectsAdded = 0;

    AsciiFPrint(DEBUG_FILE_HANDLE, "Reporting error addresses to iMS");

    if (!iMSIsAvailable())
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Cannot report error addresses to iMS - iMS support is unavailable");
        return EFI_NOT_FOUND;
    }

    iMSGetDefectList(&iMSDefectList);

    while (mErrorBuf[BufIdx].Valid && logCount < mErrorBufSize)
    {
        BOOLEAN bExists = FALSE;
        int i;

        UINT32 PhysAddrMB = (UINT32)RShiftU64(mErrorBuf[BufIdx].AddrInfo.PhysAddr, 20);

        AsciiFPrint(DEBUG_FILE_HANDLE, "Checking if 0x%p exists within iMS defect list", LShiftU64(PhysAddrMB, 20));

        // Check if exists
        for (i = 0; i < iMSDefectList.MemDefectList.Count && i < IMS_MAX_DEFECT_ENTRIES; i++)
        {
            UINT32 StartAddrMB = iMSDefectList.MemDefectList.Defect[i].Addr;
            UINT32 EndAddrMB = StartAddrMB + iMSDefectList.MemDefectList.Defect[i].Length;
            if (PhysAddrMB >= StartAddrMB && PhysAddrMB < EndAddrMB)
            {
                bExists = TRUE;
                AsciiFPrint(DEBUG_FILE_HANDLE, "0x%p exists in iMS defect list (0x%p - 0x%p) [Index: %d]", LShiftU64(PhysAddrMB, 20), LShiftU64(StartAddrMB, 20), LShiftU64(EndAddrMB, 20), i);
                break;
            }
        }

        if (!bExists)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "0x%p not found. Adding to iMS defect list (0x%p - 0x%p) [Index: %d]", LShiftU64(PhysAddrMB, 20), LShiftU64(PhysAddrMB, 20), LShiftU64(PhysAddrMB + 1, 20), iMSDefectList.MemDefectList.Count);

            if (iMSDefectList.MemDefectList.Count >= IMS_MAX_DEFECT_ENTRIES)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Cannot add to iMS defect list - max entries reached (%d)", iMSDefectList.MemDefectList.Count);
                break;
            }

            iMSDefectList.MemDefectList.Defect[iMSDefectList.MemDefectList.Count].Addr = PhysAddrMB;
            iMSDefectList.MemDefectList.Defect[iMSDefectList.MemDefectList.Count].Length = 1;
            iMSDefectList.MemDefectList.Defect[iMSDefectList.MemDefectList.Count].Source = IMS_MEMORY_DEFECT_SOURCE_MEMTEST86;
            iMSDefectList.MemDefectList.Count++;
            NumDefectsAdded++;
        }

        BufIdx = BufIdx > 0 ? BufIdx - 1 : mErrorBufSize - 1;
        logCount++;
    }

    if (NumDefectsAdded > 0)
    {
        return iMSSetDefectList(&iMSDefectList);
    }

    return Status;
}

VOID
    EFIAPI
    MtSupportPrintIMSDefectList(MT_HANDLE FileHandle)
{
    IMS_NVMEM_DEFECT_LIST iMSDefectList;
    CHAR16 TempBuf[128];
    CHAR16 DimmAddrBuf[128];
    CHAR16 SourceBuf[128];

    int i;

    if (!iMSIsAvailable())
    {
        return;
    }

    iMSGetDefectList(&iMSDefectList);

    if (FileHandle == NULL)
    {
        gST->ConOut->ClearScreen(gST->ConOut);
        gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);
    }

    FPrint(FileHandle, L"iMS Defect List (%d entries)\n", iMSDefectList.MemDefectList.Count <= IMS_MAX_DEFECT_ENTRIES ? iMSDefectList.MemDefectList.Count : IMS_MAX_DEFECT_ENTRIES);
    for (i = 0; i < iMSDefectList.MemDefectList.Count && i < IMS_MAX_DEFECT_ENTRIES; i++)
    {
        UINT64 StartAddr = LShiftU64(iMSDefectList.MemDefectList.Defect[i].Addr, 20);
        UINT64 EndAddr = StartAddr + LShiftU64(iMSDefectList.MemDefectList.Defect[i].Length, 20) - 1;

        UINT8 SocketId = 0xff;
        UINT8 MemoryControllerId = 0xff;
        UINT8 ChannelId = 0xff;
        UINT8 DimmSlot = 0xff;
        UINT8 DimmRank = 0xff;

        DimmAddrBuf[0] = L'\0';
        SourceBuf[0] = L'\0';

        if (FileHandle != NULL && iMSSystemAddressToDimmAddress(StartAddr, &SocketId, &MemoryControllerId, &ChannelId, &DimmSlot, &DimmRank) == EFI_SUCCESS)
            UnicodeSPrint(DimmAddrBuf, sizeof(DimmAddrBuf), L"{Socket: %d, Ctrl: %d, ChId: %d, DimmSlot: %d, DimmRank: %d} ", SocketId, MemoryControllerId, ChannelId, DimmSlot, DimmRank);

        if (iMSDefectList.MemDefectList.Defect[i].Source == IMS_MEMORY_DEFECT_SOURCE_MEMTEST86)
            UnicodeSPrint(SourceBuf, sizeof(SourceBuf), L"{Source: %s} ", PROGRAM_NAME);
        else
            UnicodeSPrint(SourceBuf, sizeof(SourceBuf), L"{Source: %d} ", iMSDefectList.MemDefectList.Defect[i].Source);

        FPrint(FileHandle, L"0x%012lx - 0x%012lx (%dMB) %s%s [%d]\n",
               StartAddr,
               EndAddr,
               iMSDefectList.MemDefectList.Defect[i].Length,
               DimmAddrBuf,
               SourceBuf,
               i);

        if (FileHandle == NULL && i != 0 && i % (ConHeight - 5) == 0)
        {
            FPrint(FileHandle, L"\n");
            FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)));
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
            gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);
        }
    }

    if (FileHandle == NULL)
    {
        FPrint(FileHandle, L"\n");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)));
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }
}

#if 0 // not used
VOID
EFIAPI
MtSupportReportHammerResults(
    IN UINT32                   TestNo,
    IN UINTN                    NumErrors,
    IN UINTN                    NumRowsHammered
)
{
    UINTN ProcNum = MPSupportWhoAmI();
    AcquireSpinLock(&mErrorLock);
    mNumHammerErrors += NumErrors;
    mNumRowsHammered += NumRowsHammered;
    ReleaseSpinLock(&mErrorLock);
    if (ProcNum == MPSupportGetBspId())
    {
        MtSupportUpdateHammerResults();
    }
}

VOID
EFIAPI
MtSupportUpdateHammerResults(
)
{
    CHAR16 HammerStatus[64];
    UINT32 Percent = MtSupportNumRowsHammered() > 0 ? ((UINT32)MtSupportNumHammerErrors() * 1000) / (UINT32)MtSupportNumRowsHammered() : 0;
    UnicodeSPrint(HammerStatus, sizeof(HammerStatus), L"Test 13 [Hammer test] - ErrRate %d/%d (%d.%d%%)", MtSupportNumHammerErrors(), MtSupportNumRowsHammered(), Percent / 10, Percent % 10);
    MtUiSetTestName(HammerStatus);
}

UINTN
EFIAPI
MtSupportNumHammerErrors(
)
{
    return mNumHammerErrors;
}

UINTN
EFIAPI
MtSupportNumRowsHammered(
)
{
    return mNumRowsHammered;
}
#endif

UINT64
EFIAPI
MtSupportReadTSC()
{
#if defined(MDE_CPU_X64) || defined(MDE_CPU_IA32)
    return AsmReadTsc();
#elif defined(MDE_CPU_AARCH64)
    UINT64 pmccntr;
    __asm__ __volatile__("isb; mrs %0, pmccntr_el0" : "=r"(pmccntr));

    return pmccntr;
    // ASM VOLATILE("MRC p15, 0, %0, c9, c13, 0\n\t" : "=r" (cycles));
    // return ArmReadCntPct();
#else
    return (UINT64)-1;
#endif
}

// MtSupportGetTestStartTime
//
// Get the time when the test suite run was started
VOID
    EFIAPI
    MtSupportGetTestStartTime(EFI_TIME *StartTime)
{
    CopyMem(StartTime, &mTestStartTime, sizeof(mTestStartTime));
}

// MtSupportGetTimeInMilliseconds
//
// Get the elapsed time in milliseconds since the test started
UINT64
EFIAPI
MtSupportGetTimeInMilliseconds()
{
    UINT64 t;

    /* We can't do the elapsed time unless the rdtsc instruction
     * is supported
     */

    if (MtSupportReadTSC() >= mStartTime)
        t = MtSupportReadTSC() - mStartTime;
    else // overflow?
        t = ((UINT64)-1) - (mStartTime - MtSupportReadTSC() + 1);

    t = DivU64x64Remainder(t, clks_msec, NULL);

    return t;
}

/*
 * Source: http://web.mit.edu/freebsd/head/sys/boot/efi/libefi/time.c
 */
/*-
 * Copyright (c) 1999, 2000
 * Intel Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *    This product includes software developed by Intel Corporation and
 *    its contributors.
 *
 * 4. Neither the name of Intel Corporation or its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define isleap(y) (((y) % 4) == 0 && (((y) % 100) != 0 || ((y) % 400) == 0))
#define SECSPERHOUR (60 * 60)
#define SECSPERDAY (24 * SECSPERHOUR)

UINT32
efi_time(EFI_TIME *ETime)
{
    /*
    //  These arrays give the cumulative number of days up to the first of the
    //  month number used as the index (1 -> 12) for regular and leap years.
    //  The value at index 13 is for the whole year.
    */
    static const UINT32 CumulativeDays[2][14] = {
        {0,
         0,
         31,
         31 + 28,
         31 + 28 + 31,
         31 + 28 + 31 + 30,
         31 + 28 + 31 + 30 + 31,
         31 + 28 + 31 + 30 + 31 + 30,
         31 + 28 + 31 + 30 + 31 + 30 + 31,
         31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
         31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
         31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
         31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
         31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31},
        {0,
         0,
         31,
         31 + 29,
         31 + 29 + 31,
         31 + 29 + 31 + 30,
         31 + 29 + 31 + 30 + 31,
         31 + 29 + 31 + 30 + 31 + 30,
         31 + 29 + 31 + 30 + 31 + 30 + 31,
         31 + 29 + 31 + 30 + 31 + 30 + 31 + 31,
         31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
         31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
         31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30,
         31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31}};

    UINT32 UTime;
    int Year;

    /*
    //  Do a santity check
    */
    if (ETime->Year < 1998 || ETime->Year > 2099 ||
        ETime->Month == 0 || ETime->Month > 12 ||
        ETime->Day == 0 || ETime->Month > 31 ||
        ETime->Hour > 23 ||
        ETime->Minute > 59 ||
        ETime->Second > 59 ||
        ETime->TimeZone < -1440 ||
        (ETime->TimeZone > 1440 && ETime->TimeZone != 2047))
    {
        return (0);
    }

    /*
    // Years
    */
    UTime = 0;
    for (Year = 1970; Year != ETime->Year; ++Year)
    {
        UTime += (CumulativeDays[isleap(Year)][13] * SECSPERDAY);
    }

    /*
    // UTime should now be set to 00:00:00 on Jan 1 of the file's year.
    //
    // Months
    */
    UTime += (CumulativeDays[isleap(ETime->Year)][ETime->Month] * SECSPERDAY);

    /*
    // UTime should now be set to 00:00:00 on the first of the file's month and year
    //
    // Days -- Don't count the file's day
    */
    UTime += (((ETime->Day > 0) ? ETime->Day - 1 : 0) * SECSPERDAY);

    /*
    // Hours
    */
    UTime += (ETime->Hour * SECSPERHOUR);

    /*
    // Minutes
    */
    UTime += (ETime->Minute * 60);

    /*
    // Seconds
    */
    UTime += ETime->Second;

    /*
    //  EFI time is repored in local time.  Adjust for any time zone offset to
    //  get true UT
    */
    if (ETime->TimeZone != EFI_UNSPECIFIED_TIMEZONE)
    {
        /*
        //  TimeZone is kept in minues...
        */
        UTime += (ETime->TimeZone * 60);
    }

    return UTime - 28800;
}

// memoryType
//
// Get the RAM module type, speed, and bandwidth strings given the SPD data
void memoryType(int tCK, SPDINFO *spdinfo, CHAR16 *szwModuleType, CHAR16 *szwSpeed, CHAR16 *szwBandwidth, UINTN bufSize)
{
    const CHAR16 *SPD_TYPE_TABLE[] = {
        L"Reserved",
        L"Fast Page Mode",
        L"EDO",
        L"Pipelined Nibble",
        L"SDRAM",
        L"ROM",
        L"DDR SGRAM",
        L"DDR",
        L"DDR2",
        L"DDR2FB",
        L"DDR2FB-PROBE",
        L"DDR3",
        L"DDR4",
        L"Reserved",
        L"DDR4E",
        L"LPDDR3",
        L"LPDDR4",
        L"LPDDR4X",
        L"DDR5",
        L"LPDDR5",
        L"DDR5 NVDIMM-P",
        L"LPDDR5X"};

    int pcclk;
    int clkspeed;
    UnicodeSPrint(szwSpeed, bufSize, L"UNKNOWN");
    UnicodeSPrint(szwBandwidth, bufSize, L"UNKNOWN");

    clkspeed = spdinfo->type == SPDINFO_MEMTYPE_SDRAM ? 1000000 / tCK : (2 * 1000000) / tCK;
    // Fix rounding errors
    if ((clkspeed % 100) < 10)
        clkspeed = (clkspeed / 100) * 100;
    else if (clkspeed % 100 == 32)
        clkspeed += 1;

    if (spdinfo->type >= 0 && spdinfo->type <= SPDINFO_MEMTYPE_LPDDR5X)
        StrCpyS(szwModuleType, bufSize / sizeof(CHAR16), SPD_TYPE_TABLE[spdinfo->type]);
    else
        StrCpyS(szwModuleType, bufSize / sizeof(CHAR16), L"UNKNOWN");

    switch (spdinfo->type)
    {
    case SPDINFO_MEMTYPE_SDRAM:
        pcclk = (1000000 * spdinfo->txWidth) / (8 * tCK);
        if ((pcclk % 100) >= 50) // Round properly
            pcclk += 100;
        pcclk = pcclk - (pcclk % 100);
        StrCpyS(szwSpeed, bufSize / sizeof(CHAR16), spdinfo->szModulespeed);
        UnicodeSPrint(szwSpeed, bufSize, L"PC%d", (int)clkspeed);
        UnicodeSPrint(szwBandwidth, bufSize, L"%dMB/s", pcclk);

        break;
    case SPDINFO_MEMTYPE_DDR:
        pcclk = (2 * 1000000 * spdinfo->txWidth) / (8 * tCK);
        if ((pcclk % 100) >= 50) // Round properly
            pcclk += 100;
        pcclk = pcclk - (pcclk % 100);
        ;
        UnicodeSPrint(szwSpeed, bufSize, L"DDR-%d", (int)clkspeed);
        UnicodeSPrint(szwBandwidth, bufSize, L"PC%d", pcclk);

        break;
    case SPDINFO_MEMTYPE_DDR2:
        pcclk = (2 * 1000000 * spdinfo->txWidth) / (8 * tCK);
        // Round down to comply with Jedec
        pcclk = pcclk - (pcclk % 100);
        UnicodeSPrint(szwSpeed, bufSize, L"DDR2-%d", (int)clkspeed);
        UnicodeSPrint(szwBandwidth, bufSize, L"PC2-%d", pcclk);

        break;
    case SPDINFO_MEMTYPE_DDR2FB:
    case SPDINFO_MEMTYPE_DDR2FBPROBE:
        pcclk = (2 * 1000000 * spdinfo->txWidth) / (8 * tCK);
        pcclk = (pcclk / 100) * 100; // SI101011 - truncate the speed e.g. DDR3 533MHz is PC3-8533, but should be displayed as PC3-8500
        UnicodeSPrint(szwSpeed, bufSize, L"DDR2-%d", (int)clkspeed);
        UnicodeSPrint(szwBandwidth, bufSize, L"PC-%d", pcclk);

        break;
    case SPDINFO_MEMTYPE_DDR3:
    case SPDINFO_MEMTYPE_LPDDR3:
        pcclk = (2 * 1000000 * spdinfo->txWidth) / (8 * tCK);
        pcclk = (pcclk / 100) * 100; // SI101011 - truncate the speed e.g. DDR3 533MHz is PC3-8533, but should be displayed as PC3-8500
        UnicodeSPrint(szwSpeed, bufSize, L"DDR3-%d", (int)clkspeed);
        UnicodeSPrint(szwBandwidth, bufSize, L"PC3-%d", pcclk);

        break;
    case SPDINFO_MEMTYPE_DDR4:
    case SPDINFO_MEMTYPE_DDR4E:
    case SPDINFO_MEMTYPE_LPDDR4:
    case SPDINFO_MEMTYPE_LPDDR4X:
        pcclk = (2 * 1000000 * spdinfo->txWidth) / (8 * tCK);
        pcclk = (pcclk / 100) * 100; // SI101011 - truncate the speed e.g. DDR3 533MHz is PC3-8533, but should be displayed as PC3-8500
        UnicodeSPrint(szwSpeed, bufSize, L"DDR4-%d", (int)clkspeed);
        UnicodeSPrint(szwBandwidth, bufSize, L"PC4-%d", pcclk);

        break;

    case SPDINFO_MEMTYPE_DDR5:
    case SPDINFO_MEMTYPE_LPDDR5:
    case SPDINFO_MEMTYPE_LPDDR5X:
        pcclk = (2 * 1000000 * 8) / tCK;
        pcclk = (pcclk / 100) * 100; // SI101011 - truncate the speed e.g. DDR3 533MHz is PC3-8533, but should be displayed as PC3-8500
        UnicodeSPrint(szwSpeed, bufSize, L"DDR5-%d", (int)clkspeed);
        UnicodeSPrint(szwBandwidth, bufSize, L"PC5-%d", pcclk);

        break;
    default:
        break;
    }
}

typedef enum _SPDPROFILE
{
    SPD_JEDEC,
    SPD_XMP,
    SPD_EPP,
    SPD_EXPO
} SPDPROFILE;

// getMaxSPDInfo
//
// Get the maximum RAM module type, speed, and bandwidth strings given the SPD data
SPDPROFILE getMaxSPDInfo(SPDINFO *spdinfo, int *ptCK, int *pclkspeed, int *ptAA, int *ptRCD, int *ptRP, int *ptRAS, CHAR16 **ppszVoltage)
{
    int tCK = spdinfo->tCK;
    int clkspeed = spdinfo->clkspeed;
    int tAA = spdinfo->tAA;
    int tRCD = spdinfo->tRCD;
    int tRP = spdinfo->tRP;
    int tRAS = spdinfo->tRAS;
    CHAR16 *pszVoltage = spdinfo->moduleVoltage;

    SPDPROFILE profile = SPD_JEDEC;

    if (spdinfo->type == SPDINFO_MEMTYPE_DDR5)
    {
        if (spdinfo->specific.DDR5SDRAM.XMPSupported)
        {
            int j;
            for (j = 0; j < MAX_XMP3_PROFILES; j++)
            {
                if (spdinfo->specific.DDR5SDRAM.XMP.profile[j].enabled)
                {
                    if (spdinfo->specific.DDR5SDRAM.XMP.profile[j].tCK <= tCK)
                    {
                        clkspeed = spdinfo->specific.DDR5SDRAM.XMP.profile[j].clkspeed;
                        tCK = spdinfo->specific.DDR5SDRAM.XMP.profile[j].tCK;
                        tAA = spdinfo->specific.DDR5SDRAM.XMP.profile[j].tAA;
                        tRCD = spdinfo->specific.DDR5SDRAM.XMP.profile[j].tRCD;
                        tRP = spdinfo->specific.DDR5SDRAM.XMP.profile[j].tRP;
                        tRAS = spdinfo->specific.DDR5SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = spdinfo->specific.DDR5SDRAM.XMP.profile[j].moduleVDD;
                        profile = SPD_XMP;
                    }
                }
            }
        }

        if (spdinfo->specific.DDR5SDRAM.EXPOSupported)
        {
            int j;
            for (j = 0; j < MAX_EXPO_PROFILES; j++)
            {
                if (spdinfo->specific.DDR5SDRAM.EXPO.profile[j].enabled)
                {
                    if (spdinfo->specific.DDR5SDRAM.EXPO.profile[j].tCK <= tCK)
                    {
                        clkspeed = spdinfo->specific.DDR5SDRAM.EXPO.profile[j].clkspeed;
                        tCK = spdinfo->specific.DDR5SDRAM.EXPO.profile[j].tCK;
                        tAA = spdinfo->specific.DDR5SDRAM.EXPO.profile[j].tAA;
                        tRCD = spdinfo->specific.DDR5SDRAM.EXPO.profile[j].tRCD;
                        tRP = spdinfo->specific.DDR5SDRAM.EXPO.profile[j].tRP;
                        tRAS = spdinfo->specific.DDR5SDRAM.EXPO.profile[j].tRAS;
                        pszVoltage = spdinfo->specific.DDR5SDRAM.EXPO.profile[j].moduleVDD;
                        profile = SPD_EXPO;
                    }
                }
            }
        }
    }
    else if (spdinfo->type == SPDINFO_MEMTYPE_DDR3)
    {
        if (spdinfo->specific.DDR3SDRAM.XMPSupported)
        {
            int j;
            for (j = 0; j < 2; j++)
            {
                if (spdinfo->specific.DDR3SDRAM.XMP.profile[j].enabled)
                {
                    if (spdinfo->specific.DDR3SDRAM.XMP.profile[j].tCK <= tCK)
                    {
                        clkspeed = spdinfo->specific.DDR3SDRAM.XMP.profile[j].clkspeed;
                        tCK = spdinfo->specific.DDR3SDRAM.XMP.profile[j].tCK;
                        tAA = spdinfo->specific.DDR3SDRAM.XMP.profile[j].tAA;
                        tRCD = spdinfo->specific.DDR3SDRAM.XMP.profile[j].tRCD;
                        tRP = spdinfo->specific.DDR3SDRAM.XMP.profile[j].tRP;
                        tRAS = spdinfo->specific.DDR3SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = spdinfo->specific.DDR3SDRAM.XMP.profile[j].moduleVdd;
                        profile = SPD_XMP;
                    }
                }
            }
        }
    }
    else if (spdinfo->type == SPDINFO_MEMTYPE_DDR4)
    {
        if (spdinfo->specific.DDR4SDRAM.XMPSupported)
        {
            int j;
            for (j = 0; j < 2; j++)
            {
                if (spdinfo->specific.DDR4SDRAM.XMP.profile[j].enabled)
                {
                    if (spdinfo->specific.DDR4SDRAM.XMP.profile[j].tCK <= tCK)
                    {
                        clkspeed = spdinfo->specific.DDR4SDRAM.XMP.profile[j].clkspeed;
                        tCK = spdinfo->specific.DDR4SDRAM.XMP.profile[j].tCK;
                        tAA = spdinfo->specific.DDR4SDRAM.XMP.profile[j].tAA;
                        tRCD = spdinfo->specific.DDR4SDRAM.XMP.profile[j].tRCD;
                        tRP = spdinfo->specific.DDR4SDRAM.XMP.profile[j].tRP;
                        tRAS = spdinfo->specific.DDR4SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = spdinfo->specific.DDR4SDRAM.XMP.profile[j].moduleVdd;
                        profile = SPD_XMP;
                    }
                }
            }
        }
    }
    else if (spdinfo->type == SPDINFO_MEMTYPE_DDR2)
    {
        if (spdinfo->specific.DDR2SDRAM.EPPSupported)
        {
            if (spdinfo->specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_ABBR)
            {
                int j;
                for (j = 0; j < 4; j++)
                {
                    if (spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tCK <= tCK)
                    {
                        clkspeed = spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].clkspeed;
                        tCK = spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tCK;
                        tAA = spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].CASSupported * tCK;
                        tRCD = spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRCD;
                        tRP = spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRP;
                        tRAS = spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRAS;
                        pszVoltage = spdinfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].voltageLevel;
                        profile = SPD_EPP;
                    }
                }
            }
            else if (spdinfo->specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_FULL)
            {
                int j;
                for (j = 0; j < 2; j++)
                {
                    if (spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tCK <= tCK)
                    {
                        clkspeed = spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].clkspeed;
                        tCK = spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tCK;
                        tAA = spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].CASSupported * tCK;
                        tRCD = spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRCD;
                        tRP = spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRP;
                        tRAS = spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRAS;
                        pszVoltage = spdinfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[j].voltageLevel;
                        profile = SPD_EPP;
                    }
                }
            }
        }
    }

    if (ptCK)
        *ptCK = tCK;
    if (pclkspeed)
        *pclkspeed = clkspeed;
    if (ptAA)
        *ptAA = tAA;
    if (ptRCD)
        *ptRCD = tRCD;
    if (ptRP)
        *ptRP = tRP;
    if (ptRAS)
        *ptRAS = tRAS;
    if (ppszVoltage)
        *ppszVoltage = pszVoltage;

    return profile;
}

void getMaxSPDInfoStr(SPDINFO *spdinfo, CHAR16 *szwRAMType, CHAR16 *szwXferSpeed, CHAR16 *szwMaxBandwidth, CHAR16 *szwTiming, CHAR16 *szwVoltage, UINTN bufSize)
{
    // Get Common Names etc...
    int tCK;
    int clkspeed;
    int tAA;
    int tRCD;
    int tRP;
    int tRAS;
    CHAR16 *pszVoltage;
    CHAR16 szwMaxXferSpeed[32];

    SPDPROFILE profile = getMaxSPDInfo(spdinfo, &tCK, &clkspeed, &tAA, &tRCD, &tRP, &tRAS, &pszVoltage);

    memoryType(tCK, spdinfo, szwRAMType, szwMaxXferSpeed, szwMaxBandwidth, bufSize);

    getTimingsStr(spdinfo->type, tCK, tAA, tRCD, tRP, tRAS, szwTiming, bufSize);

    if (profile == SPD_XMP)
        StrCatS(szwRAMType, bufSize / sizeof(CHAR16), L" XMP");
    else if (profile == SPD_EXPO)
        StrCatS(szwRAMType, bufSize / sizeof(CHAR16), L" EXPO");
    else if (profile == SPD_EPP)
        StrCatS(szwRAMType, bufSize / sizeof(CHAR16), L" EPP");

    UnicodeSPrint(szwXferSpeed, bufSize, L"%d", clkspeed * 2); // * 2 for DDR
    StrCpyS(szwVoltage, bufSize / sizeof(CHAR16), pszVoltage);
}

// getSPDSummaryLine
//
// Get the one-line summary string given the SPD data
CHAR16 *getSPDSummaryLine(SPDINFO *spdinfo, CHAR16 *szwRAMSummary, UINTN bufSize)
{
    CHAR16 szBuffer[128];
    CHAR16 szwRAMType[32];
    CHAR16 szwXferSpeed[32];
    CHAR16 szwMaxBandwidth[32];
    CHAR16 szwTiming[32];
    CHAR16 szwVoltage[32];
    CHAR16 szwSize[32];

    BOOLEAN ECCEn = FALSE;

    getMaxSPDInfoStr(spdinfo, szwRAMType, szwXferSpeed, szwMaxBandwidth, szwTiming, szwVoltage, sizeof(szwRAMType));

    if (spdinfo->size >= 1024)
        UnicodeSPrint(szwSize, sizeof(szwSize), L"%dGB", spdinfo->size / 1024);
    else
        UnicodeSPrint(szwSize, sizeof(szwSize), L"%dMB", spdinfo->size);

    ECCEn = (spdinfo->ecc != FALSE) && MtSupportECCEnabled();
    UnicodeSPrint(szBuffer, sizeof(szBuffer), L"%s %s %dRx%d %s%s", szwSize, szwRAMType, spdinfo->numRanks, spdinfo->deviceWidth, ECCEn ? L"ECC " : L"", szwMaxBandwidth);

    StrnCpyS(szwRAMSummary, bufSize / sizeof(CHAR16), szBuffer, bufSize / sizeof(CHAR16) - 1);
    szwRAMSummary[bufSize / sizeof(CHAR16) - 1] = L'\0';
    return szwRAMSummary;
}

CHAR16 *getSPDProfileStr(int memtype, int clkspeed, int tCK, int tAA, int tRCD, int tRP, int tRAS, const CHAR16 *pszVoltage, CHAR16 *Buffer, UINTN BufferSize)
{
    CHAR16 szTimings[32];

    UnicodeSPrint(Buffer, BufferSize, L"%dMT/s %s %s",
                  clkspeed * 2,
                  getTimingsStr(memtype, tCK, tAA, tRCD, tRP, tRAS, szTimings, sizeof(szTimings)),
                  pszVoltage);
    return Buffer;
}

VOID convertMemTimings(int memtype, int tCK, int *tAA, int *tRCD, int *tRP, int *tRAS)
{
    if (memtype == SPDINFO_MEMTYPE_DDR5)
    {
        // See JESD400-5 DDR5 Serial Presence Detect (SPD) - Rounding Algorithms
        *tAA = (int)(((*tAA * 997 / tCK) + 1000) / 1000);
        // Systems may prefer to use other methods to calculate CAS Latency settings, including applying the
        // Rounding Algorithm on tAAmin(SPD bytes 30~31) to determine the desired CL setting, rounding up if
        // necessary to ensure that only even nCK values are used,
        *tAA += *tAA % 2;
        *tRCD = (int)(((*tRCD * 997 / tCK) + 1000) / 1000);
        *tRP = (int)(((*tRP * 997 / tCK) + 1000) / 1000);
        *tRAS = (int)(((*tRAS * 997 / tCK) + 1000) / 1000);
    }
    else
    {
        *tAA = (int)CEIL_SPD(*tAA, tCK);
        *tRCD = (int)CEIL_SPD(*tRCD, tCK);
        *tRP = (int)CEIL_SPD(*tRP, tCK);
        *tRAS = (int)CEIL_SPD(*tRAS, tCK);
    }
}
CHAR16 *getTimingsStr(int memtype, int tCK, int tAA, int tRCD, int tRP, int tRAS, CHAR16 *Buffer, UINTN BufferSize)
{
    convertMemTimings(memtype, tCK, &tAA, &tRCD, &tRP, &tRAS);

    UnicodeSPrint(Buffer, BufferSize, L"%d-%d-%d-%d",
                  tAA,
                  tRCD,
                  tRP,
                  tRAS);
    return Buffer;
}

void getMemConfig(CHAR16 *szwXferSpeed, CHAR16 *szwChannelMode, CHAR16 *szwTiming, CHAR16 *szwVoltage, CHAR16 *szwECC, UINTN bufSize)
{
    CHAR16 TempBuf[128];
    szwXferSpeed[0] = szwChannelMode[0] = szwTiming[0] = szwVoltage[0] = szwECC[0] = L'\0';

    unsigned int memclk, ctrlclk;
    unsigned char cas, rcd, rp, ras;
    if (get_mem_ctrl_timings(&memclk, &ctrlclk, &cas, &rcd, &rp, &ras) == 0)
    {
        UnicodeSPrint(szwXferSpeed, bufSize, L"%dMT/s", memclk * 2);
        UnicodeSPrint(szwTiming, bufSize, L"%d-%d-%d-%d", cas, rcd, rp, ras);
    }
    else
    {
        for (int i = 0; i < g_numSMBIOSMem; i++)
        {
            if (g_SMBIOSMemDevices[i].smb17.Size != 0)
            {
                UINT32 SpeedMTS;
                if (g_SMBIOSMemDevices[i].smb17.ConfiguredMemoryClockSpeed == 0xffff) // Need to use Extended Speed field
                    SpeedMTS = g_SMBIOSMemDevices[i].smb17.ExtendedConfiguredMemorySpeed;
                else
                    SpeedMTS = g_SMBIOSMemDevices[i].smb17.ConfiguredMemoryClockSpeed;

                if (SpeedMTS != 0)
                    UnicodeSPrint(szwXferSpeed, bufSize, L"%dMT/s", SpeedMTS);

                break;
            }
        }
    }

    for (int i = 0; i < g_numSMBIOSMem; i++)
    {
        if (g_SMBIOSMemDevices[i].smb17.Size != 0)
        {
            if (g_SMBIOSMemDevices[i].smb17.ConfiguredVoltage != 0)
                UnicodeSPrint(szwVoltage, bufSize, L"%d.%03dV", g_SMBIOSMemDevices[i].smb17.ConfiguredVoltage / 1000, g_SMBIOSMemDevices[i].smb17.ConfiguredVoltage % 1000);

            break;
        }
    }
    unsigned int chmode = get_mem_ctrl_channel_mode();
    if (chmode > 0)
    {
        UnicodeSPrint(szwChannelMode, bufSize, L"x%d %s", chmode, GetStringById(STRING_TOKEN(STR_CHANNEL), TempBuf, sizeof(TempBuf)));
    }

    // ECC Enabled
    MtSupportGetECCFeatures(szwECC, bufSize);
}

// getMemConfigStr
//
// Get memory configuration string
CHAR16 *getMemConfigStr(CHAR16 *szwRAMSummary, UINTN bufSize)
{
    CHAR16 szwRAMType[32];
    CHAR16 szwXferSpeed[32];
    CHAR16 szwChannelMode[32];
    CHAR16 szwTiming[32];
    CHAR16 szwVoltage[32];
    CHAR16 szwECC[32];

    getRAMTypeString(szwRAMType, sizeof(szwRAMType));
    getMemConfig(szwXferSpeed, szwChannelMode, szwTiming, szwVoltage, szwECC, sizeof(szwXferSpeed));

    UnicodeSPrint(szwRAMSummary, bufSize, L"%s %s%s", szwRAMType, MtSupportECCEnabled() ? L"ECC " : L"", szwXferSpeed);
    if (szwChannelMode[0] != L'\0')
        UnicodeCatf(szwRAMSummary, bufSize, L" / %s", szwChannelMode);
    if (szwTiming[0] != L'\0')
        UnicodeCatf(szwRAMSummary, bufSize, L" / %s", szwTiming);
    if (szwVoltage[0] != L'\0')
        UnicodeCatf(szwRAMSummary, bufSize, L" / %s", szwVoltage);
    return szwRAMSummary;
}

// getMemConfigSummaryLine
//
// Get a one-line text string containing the currently configured memory timings
void getMemConfigSummaryLine(CHAR16 *szwRAMSummary, UINTN bufSize)
{
    CHAR16 szBuffer[128];
    CHAR16 szwRAMType[32];

    CHAR16 szClkSpeed[32];
    CHAR16 szChannelMode[32];
    CHAR16 szTiming[32];
    CHAR16 szVoltage[32];
    CHAR16 szECC[32];

    char modulePartNo[MODULE_PARTNO_LEN];
    CHAR16 jedecManuf[SHORT_STRING_LEN];

    getMemConfig(szClkSpeed, szChannelMode, szTiming, szVoltage, szECC, sizeof(szClkSpeed));

    jedecManuf[0] = modulePartNo[0] = '\0';
    if (g_numMemModules > 0) // Use SPD info if available
    {
        StrCpyS(jedecManuf, sizeof(jedecManuf) / sizeof(jedecManuf[0]), g_MemoryInfo[0].jedecManuf);

        // Special case for tspglobal. They want to show the part number in the summary line
        if (StrnCmp(jedecManuf, L"Essencore", StrLen(L"Essencore")) == 0)
        {
            jedecManuf[5] = L'\0'; // shorten to "Essen"
        }
        AsciiStrCpyS(modulePartNo, sizeof(modulePartNo) / sizeof(modulePartNo[0]), g_MemoryInfo[0].modulePartNo);
    }
    else if (g_numSMBIOSModules > 0) // Use SMBIOS info if available
    {
        for (int i = 0; i < g_numSMBIOSMem; i++)
        {
            if (g_SMBIOSMemDevices[i].smb17.Size != 0)
            {
                AsciiStrToUnicodeStrS(g_SMBIOSMemDevices[i].szManufacturer, jedecManuf, sizeof(jedecManuf) / sizeof(jedecManuf[0]));
                AsciiStrCpyS(modulePartNo, sizeof(modulePartNo) / sizeof(modulePartNo[0]), g_SMBIOSMemDevices[i].szPartNumber);
                break;
            }
        }
    }

    getRAMTypeString(szwRAMType, sizeof(szwRAMType));

    UnicodeSPrint(szBuffer, sizeof(szBuffer), L"%s %s%s", szwRAMType, MtSupportECCEnabled() ? L"ECC " : L"", szClkSpeed);

    if (szChannelMode[0] != L'\0')
        UnicodeCatf(szBuffer, sizeof(szBuffer), L" / %s", szChannelMode);
    if (jedecManuf[0] != L'\0' && modulePartNo[0] != '\0')
        UnicodeCatf(szBuffer, sizeof(szBuffer), L" / %s %a", jedecManuf, modulePartNo);

    StrnCpyS(szwRAMSummary, bufSize / sizeof(CHAR16), szBuffer, bufSize / sizeof(CHAR16) - 1);
    szwRAMSummary[bufSize / sizeof(CHAR16) - 1] = L'\0';
}

// getSMBIOSMemoryType
//
// Get the RAM module type, speed, and bandwidth strings given the SMBIOS data
void getSMBIOSMemoryType(SMBIOS_MEMORY_DEVICE *pSMBIOSMemDev, CHAR16 *szwModuleType, CHAR16 *szwSpeed, CHAR16 *szwBandwidth, UINTN bufSize)
{
    const CHAR16 *SMBIOS_MEM_DEVICE_TYPE[] = {
        L"",
        L"Other",
        L"Unknown",
        L"DRAM",
        L"EDRAM",
        L"VRAM",
        L"SRAM",
        L"RAM",
        L"ROM",
        L"FLASH",
        L"EEPROM",
        L"FEPROM",
        L"EPROM",
        L"CDRAM",
        L"3DRAM",
        L"SDRAM",
        L"SGRAM",
        L"RDRAM",
        L"DDR",
        L"DDR2",
        L"DDR2 FB-DIMM",
        L"Reserved",
        L"Reserved",
        L"Reserved",
        L"DDR3",
        L"FBD2",
        L"DDR4",
        L"LPDDR",
        L"LPDDR2",
        L"LPDDR3",
        L"LPDDR4",
        L"Logical non-volatile device",
        L"HBM",
        L"HBM2",
        L"DDR5",
        L"LPDDR5",
        L"HBM3"};

    if (pSMBIOSMemDev->smb17.MemoryType < sizeof(SMBIOS_MEM_DEVICE_TYPE) / sizeof(SMBIOS_MEM_DEVICE_TYPE[0]))
        UnicodeSPrint(szwModuleType, bufSize, SMBIOS_MEM_DEVICE_TYPE[pSMBIOSMemDev->smb17.MemoryType]);
    else
        UnicodeSPrint(szwModuleType, bufSize, L"");

    if (pSMBIOSMemDev->smb17.Speed > 0)
    {
        int pcclk;
        UnicodeSPrint(szwSpeed, bufSize, L"%d", pSMBIOSMemDev->smb17.Speed);

        pcclk = pSMBIOSMemDev->smb17.Speed * 8;
        pcclk = (pcclk / 100) * 100;

        if (StrStr(szwModuleType, L"DDR2"))
            UnicodeSPrint(szwBandwidth, bufSize, L"PC2-%d", pcclk);
        else if (StrStr(szwModuleType, L"DDR3"))
            UnicodeSPrint(szwBandwidth, bufSize, L"PC3-%d", pcclk);
        else if (StrStr(szwModuleType, L"DDR4"))
            UnicodeSPrint(szwBandwidth, bufSize, L"PC4-%d", pcclk);
        else if (StrStr(szwModuleType, L"DDR5"))
            UnicodeSPrint(szwBandwidth, bufSize, L"PC5-%d", pcclk);
        else
            UnicodeSPrint(szwBandwidth, bufSize, L"PC-%d", pcclk);
    }
    else
    {
        UnicodeSPrint(szwSpeed, bufSize, L"");
        UnicodeSPrint(szwBandwidth, bufSize, L"");
    }
}

// getSMBIOSRAMSummaryLine
//
// Get the one-line summary string given the SMBIOS data
CHAR16 *getSMBIOSRAMSummaryLine(SMBIOS_MEMORY_DEVICE *pSMBIOSMemDev, CHAR16 *szwRAMSummary, UINTN bufSize)
{
    CHAR16 szBuffer[128];
    CHAR16 szwRAMType[32];
    CHAR16 szwXferSpeed[32];
    CHAR16 szwMaxBandwidth[32];
    CHAR16 szwSize[32];

    UINT64 size = 0;
    BOOLEAN ECCEn = FALSE;

    int NumRanks = 1;

    getSMBIOSMemoryType(pSMBIOSMemDev, szwRAMType, szwXferSpeed, szwMaxBandwidth, sizeof(szwRAMType));

    if (pSMBIOSMemDev->smb17.Size == 0x7fff) // Need to use Extended Size field
    {
        size = (UINT64)pSMBIOSMemDev->smb17.ExtendedSize * SIZE_1MB;
    }
    else
    {
        if (BitExtract(pSMBIOSMemDev->smb17.Size, 15, 15) == 0) // MB - bit 15 is the unit MB or KB
            size = (UINT64)pSMBIOSMemDev->smb17.Size * SIZE_1MB;
        else // KB
            size = (UINT64)pSMBIOSMemDev->smb17.Size * SIZE_1KB;
    }

    if (size >= SIZE_1GB)
        UnicodeSPrint(szwSize, sizeof(szwSize), L"%ldGB", DivU64x32(size, SIZE_1GB));
    else if (size >= SIZE_1MB)
        UnicodeSPrint(szwSize, sizeof(szwSize), L"%ldMB", DivU64x32(size, SIZE_1MB));
    else
        UnicodeSPrint(szwSize, sizeof(szwSize), L"%ldKB", DivU64x32(size, SIZE_1KB));

    if (pSMBIOSMemDev->smb17.Attributes & 0xF) // bits 3-0: rank Value=0 for unknown rank information
        NumRanks = pSMBIOSMemDev->smb17.Attributes & 0xF;

    if (pSMBIOSMemDev->smb17.TotalWidth != 0xffff && pSMBIOSMemDev->smb17.DataWidth != 0xffff)
        ECCEn = (pSMBIOSMemDev->smb17.TotalWidth > pSMBIOSMemDev->smb17.DataWidth) && MtSupportECCEnabled();

    UnicodeSPrint(szBuffer, sizeof(szBuffer), L"%s %s %dR %s%s", szwSize, szwRAMType, NumRanks, ECCEn ? L"ECC " : L"", szwMaxBandwidth);

    StrnCpyS(szwRAMSummary, bufSize / sizeof(CHAR16), szBuffer, bufSize / sizeof(CHAR16) - 1);
    szwRAMSummary[bufSize / sizeof(CHAR16) - 1] = L'\0';
    return szwRAMSummary;
}

CHAR16 *getSMBIOSRAMProfileStr(SMBIOS_MEMORY_DEVICE *pSMBIOSMemDev, CHAR16 *Buffer, UINTN BufferSize)
{
    UINT32 SpeedMTS = 0;
    if (pSMBIOSMemDev->smb17.Speed == 0xffff) // Need to use Extended Speed field
    {
        SpeedMTS = pSMBIOSMemDev->smb17.ExtendedSpeed;
    }
    else
    {
        SpeedMTS = pSMBIOSMemDev->smb17.Speed;
    }

    UINT16 Millivolts = 0;
    if (pSMBIOSMemDev->smb17.ConfiguredVoltage != 0)
        Millivolts = pSMBIOSMemDev->smb17.ConfiguredVoltage;
    else if (pSMBIOSMemDev->smb17.MaximumVoltage != 0)
        Millivolts = pSMBIOSMemDev->smb17.MaximumVoltage;
    else if (pSMBIOSMemDev->smb17.MinimumVoltage != 0)
        Millivolts = pSMBIOSMemDev->smb17.MinimumVoltage;

    CHAR16 szVoltage[16];
    if (Millivolts > 0)
    {
        if (Millivolts % 100 == 0)
            UnicodeSPrint(szVoltage, sizeof(szVoltage), L"%d.%dV", Millivolts / 1000, (Millivolts % 1000) / 100);
        else if (Millivolts % 10 == 0)
            UnicodeSPrint(szVoltage, sizeof(szVoltage), L"%d.%02dV", Millivolts / 1000, (Millivolts % 1000) / 10);
        else
            UnicodeSPrint(szVoltage, sizeof(szVoltage), L"%d.%03dV", Millivolts / 1000, Millivolts % 1000);
    }
    else
        szVoltage[0] = L'\0';

    UnicodeSPrint(Buffer, BufferSize, L"%dMT/s %s",
                  SpeedMTS,
                  szVoltage);
    return Buffer;
}

void getRAMTypeString(CHAR16 *RAMType, UINTN RAMTypeSize)
{
    CHAR16 szwRAMType[32];
    CHAR16 szwXferSpeed[32];
    CHAR16 szwMaxBandwidth[32];

    StrCpyS(RAMType, RAMTypeSize / sizeof(RAMType[0]), L"");
    for (int i = 0; i < g_numSMBIOSMem; i++)
    {
        if (g_SMBIOSMemDevices[i].smb17.Size != 0)
        {
            getSMBIOSMemoryType(&g_SMBIOSMemDevices[i], szwRAMType, szwXferSpeed, szwMaxBandwidth, sizeof(szwRAMType));
            StrCpyS(RAMType, RAMTypeSize / sizeof(RAMType[0]), szwRAMType);
            return;
        }
    }
}
// PopulateSDRAM
//
// Output the SDRAM SPD details
void PopulateSDRAM(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"SDRAM");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"SDRAM Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Data Access Time from Clock, tAC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tAC / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tAC % 1000);
    FPrint(FileHandle, L"  Clock Cycle Time at Medium CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tCKmed / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tCKmed % 1000);
    FPrint(FileHandle, L"  Data Access Time at Medium CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tACmed / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tACmed % 1000);
    FPrint(FileHandle, L"  Clock Cycle Time at Short CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tCKshort / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tCKshort % 1000);
    FPrint(FileHandle, L"  Data Access Time at Short CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tACshort / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tACshort % 1000);
    FPrint(FileHandle, L"  Address/Command Setup Time Before Clock, tIS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tIS / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tIS % 1000);
    FPrint(FileHandle, L"  Address/Command Hold Time After Clock, tIH (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tIH / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tIH % 1000);
    FPrint(FileHandle, L"  Data Input Setup Time Before Strobe, tDS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tDS / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tDS % 1000);
    FPrint(FileHandle, L"  Data Input Hold Time After Strobe, tDH (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.SDRSDRAM.tDH / 1000, g_MemoryInfo[dimm].specific.SDRSDRAM.tDH % 1000);

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"SDRAM");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  CS Latencies Supported: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.CSSupported);
    FPrint(FileHandle, L"  WE Latencies Supported: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.WESupported);
    FPrint(FileHandle, L"  Burst Lengths Supported: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.BurstLengthsSupported);
    FPrint(FileHandle, L"  Refresh Rate: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.RefreshRate);
    FPrint(FileHandle, L"  Buffered Address/Control Inputs: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.buffered ? L"Yes" : L"No");
    FPrint(FileHandle, L"  On-card PLL: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.OnCardPLL ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Buffered DQMB Inputs: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.BufferedDQMB ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Registered DQMB Inputs: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.RegisteredDQMB ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Differential Clock Input: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.DiffClockInput ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Early RAS# Precharge Supported: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.EarlyRASPrechargeSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Auto-Precharge Supported: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.AutoPrechargeSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Precharge All Supported: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.PrechargeAllSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Write/Read Burst Supported: %s\n", g_MemoryInfo[dimm].specific.SDRSDRAM.WriteReadBurstSupported ? L"Yes" : L"No");
}

// PopulateDDR1
//
// Output the DDR SPD details
void PopulateDDR1(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"DDR1 Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Data Access Time from Clock, tAC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tAC / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tAC % 1000);
    FPrint(FileHandle, L"  Clock Cycle Time at Medium CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tCKmed / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tCKmed % 1000);
    FPrint(FileHandle, L"  Data Access Time at Medium CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tACmed / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tACmed % 1000);
    FPrint(FileHandle, L"  Clock Cycle Time at Short CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tCKshort / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tCKshort % 1000);
    FPrint(FileHandle, L"  Data Access Time at Short CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tACshort / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tACshort % 1000);
    FPrint(FileHandle, L"  Maximum Clock Cycle Time (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tCKmax / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tCKmax % 1000);
    FPrint(FileHandle, L"  Address/Command Setup Time Before Clock, tIS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tIS / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tIS % 1000);
    FPrint(FileHandle, L"  Address/Command Hold Time After Clock, tIH (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tIH / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tIH % 1000);
    FPrint(FileHandle, L"  Data Input Setup Time Before Strobe, tDS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tDS / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tDS % 1000);
    FPrint(FileHandle, L"  Data Input Hold Time After Strobe, tDH (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tDH / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tDH % 1000);
    FPrint(FileHandle, L"  Maximum Skew Between DQS and DQ Signals (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tDQSQ / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tDQSQ % 1000);
    FPrint(FileHandle, L"  Maximum Read Data hold Skew Factor (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.tQHS / 1000, g_MemoryInfo[dimm].specific.DDR1SDRAM.tQHS % 1000);

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  CS Latencies Supported: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.CSSupported);
    FPrint(FileHandle, L"  WE Latencies Supported: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.WESupported);
    FPrint(FileHandle, L"  Burst Lengths Supported: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.BurstLengthsSupported);
    FPrint(FileHandle, L"  Refresh Rate: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.RefreshRate);
    FPrint(FileHandle, L"  Buffered Address/Control Inputs: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.buffered ? L"Yes" : L"No");
    FPrint(FileHandle, L"  On-card PLL: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.OnCardPLL ? L"Yes" : L"No");
    FPrint(FileHandle, L"  FET Switch On-card Enable: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.FETOnCardEnable ? L"Yes" : L"No");
    FPrint(FileHandle, L"  FET Switch External Enable: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.FETExtEnable ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Differential Clock Input: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.DiffClockInput ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Weak Driver Included: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.WeakDriverIncluded ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Concurrent Auto Precharge Supported: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.ConcAutoPrecharge ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Fast AP Supported: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.FastAPSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Module Bank Density: %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.bankDensity);
    FPrint(FileHandle, L"  Module Height (mm): %s\n", g_MemoryInfo[dimm].specific.DDR1SDRAM.moduleHeight);
}

// PopulateDDR2
//
// Output the DDR2 SPD details
void PopulateDDR2(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR2");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"DDR2 Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Data Access Time from Clock, tAC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tAC / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tAC % 1000);
    FPrint(FileHandle, L"  Clock Cycle Time at Medium CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tCKmed / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tCKmed % 1000);
    FPrint(FileHandle, L"  Data Access Time at Medium CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tACmed / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tACmed % 1000);
    FPrint(FileHandle, L"  Clock Cycle Time at Short CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tCKshort / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tCKshort % 1000);
    FPrint(FileHandle, L"  Data Access Time at Short CAS Latency (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tACshort / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tACshort % 1000);
    FPrint(FileHandle, L"  Maximum Clock Cycle Time (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tCKmax / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tCKmax % 1000);
    FPrint(FileHandle, L"  Write Recover Time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tWR / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tWR % 1000);
    FPrint(FileHandle, L"  Internal Write to Read Command Delay, tWTR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tWTR / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tWTR % 1000);
    FPrint(FileHandle, L"  Internal Read to Precharge Command Delay, tRTP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tRTP / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tRTP % 1000);
    FPrint(FileHandle, L"  Address/Command Setup Time Before Clock, tIS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tIS / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tIS % 1000);
    FPrint(FileHandle, L"  Address/Command Hold Time After Clock, tIH (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tIH / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tIH % 1000);
    FPrint(FileHandle, L"  Data Input Setup Time Before Strobe, tDS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tDS / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tDS % 1000);
    FPrint(FileHandle, L"  Data Input Hold Time After Strobe, tDH (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tDH / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tDH % 1000);
    FPrint(FileHandle, L"  Maximum Skew Between DQS and DQ Signals (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tDQSQ / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tDQSQ % 1000);
    FPrint(FileHandle, L"  Maximum Read Data hold Skew Factor (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tQHS / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tQHS % 1000);
    FPrint(FileHandle, L"  PLL Relock Time (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.tPLLRelock / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.tPLLRelock % 1000);

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR2");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  DRAM Package Type: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.DRAMPackage);
    FPrint(FileHandle, L"  Burst Lengths Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.BurstLengthsSupported);
    FPrint(FileHandle, L"  Refresh Rate: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.RefreshRate);
    FPrint(FileHandle, L"  # of PLLS on DIMM: %d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.numPLLs);
    FPrint(FileHandle, L"  FET Switch External Enable: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.FETExtEnable ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Analysis Probe Installed: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.AnalysisProbeInstalled ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Weak Driver Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.WeakDriverSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  50 Ohm ODT Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM._50ohmODTSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Partial Array Self Refresh Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.PASRSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Module Type: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.moduleType);
    FPrint(FileHandle, L"  Module Height (mm): %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.moduleHeight);
}

// PopulateDDR2FB
//
// Output the DDR2FB SPD details
void PopulateDDR2FB(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR2FB");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"DDR2FB Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Maximum Clock Cycle Time (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tCKmax / 1000, g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tCKmax % 1000);
    FPrint(FileHandle, L"  Write Recover Time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tWR / 1000, g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tWR % 1000);
    FPrint(FileHandle, L"  Internal Write to Read Command Delay, tWTR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tWTR / 1000, g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tWTR % 1000);
    FPrint(FileHandle, L"  Internal Read to Precharge Command Delay, tRTP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tRTP / 1000, g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tRTP % 1000);

    FPrint(FileHandle, L"  Minimum Four Activate Window Delay (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tFAW / 1000, g_MemoryInfo[dimm].specific.DDR2FBSDRAM.tFAW % 1000);
    FPrint(FileHandle, L"  Write Recovery Times Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.WRSupported);
    FPrint(FileHandle, L"  Write Latencies Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.WESupported);
    FPrint(FileHandle, L"  Additive Latencies Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.ALSupported);
    FPrint(FileHandle, L"  Burst Lengths Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.BurstLengthsSupported);
    FPrint(FileHandle, L"  Refresh Rate: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.RefreshRate);

    FPrint(FileHandle, L"  Terminations Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.TerminationsSupported);
    FPrint(FileHandle, L"  Drivers Supported: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.DriversSupported);
    FPrint(FileHandle, L"  Module Type: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.moduleType);
    FPrint(FileHandle, L"  Module Height (mm): %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.moduleHeight);
    FPrint(FileHandle, L"  Module Thickness: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.moduleThickness);
    FPrint(FileHandle, L"  DRAM Manufacture: %s\n", g_MemoryInfo[dimm].specific.DDR2FBSDRAM.DRAMManuf);
}

// PopulateDDR3
//
// Output the DDR3 SPD details
void PopulateDDR3(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR3");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"DDR3 Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Write Recover Time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.tWR / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.tWR % 1000);
    FPrint(FileHandle, L"  Internal Write to Read Command Delay, tWTR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.tWTR / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.tWTR % 1000);
    FPrint(FileHandle, L"  Internal Read to Precharge Command Delay, tRTP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.tRTP / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.tRTP % 1000);
    FPrint(FileHandle, L"  Minimum Four Activate Window Delay (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.tFAW / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.tFAW % 1000);
    FPrint(FileHandle, L"  RZQ / 6 Supported: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.RZQ6Supported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  RZQ / 7 Supported: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.RZQ7Supported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  DLL-Off Mode Supported: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.DLLOffModeSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Maximum Operating Temperature Range (C): 0-%dC\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.OperatingTempRange);
    FPrint(FileHandle, L"  Refresh Rate at Extended Operating Temperature Range: %dX\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.RefreshRateExtTempRange);
    FPrint(FileHandle, L"  Auto-Self Refresh Supported: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.autoSelfRefresh ? L"Yes" : L"No");
    FPrint(FileHandle, L"  On-die Thermal Sensor Readout Supported: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.onDieThermalSensorReadout ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Partial Array Self Refresh Supported: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.partialArraySelfRefresh ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Thermal Sensor Present: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.thermalSensorPresent ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Non-standard SDRAM Type: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.nonStdSDRAMType);
    FPrint(FileHandle, L"  Module Type: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleType);
    FPrint(FileHandle, L"  Module Height (mm): %d - %d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleHeight - 1, g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleHeight);
    FPrint(FileHandle, L"  Module Thickness (mm): front %d-%d , back %d-%d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleThicknessFront - 1, g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleThicknessFront, g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleThicknessBack - 1, g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleThicknessBack);
    FPrint(FileHandle, L"  Module Width (mm): %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleWidth);
    FPrint(FileHandle, L"  Reference Raw Card Used: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleRefCard);
    FPrint(FileHandle, L"  DRAM Manufacture: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.DRAMManuf);

    if (StrCmp(g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleType, L"RDIMM") == 0 ||
        StrCmp(g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleType, L"Mini-RDIMM") == 0 ||
        StrCmp(g_MemoryInfo[dimm].specific.DDR3SDRAM.moduleType, L"72b-SO-RDIMM") == 0)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR3 RDIMM");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }
        FPrint(FileHandle, L"  # of DRAM Rows: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR3SDRAM.numDRAMRows - 1));
        FPrint(FileHandle, L"  # of Registers: %d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.numRegisters);
        FPrint(FileHandle, L"  Register Manufacturer: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.regManuf);
        FPrint(FileHandle, L"  Register Type: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.regDeviceType);
        FPrint(FileHandle, L"  Register Revision: 0x%02X\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.regRev);

        FPrint(FileHandle, L"  Command/Address A Outputs Drive Strength: %s", g_MemoryInfo[dimm].specific.DDR3SDRAM.cmdAddrADriveStrength);
        FPrint(FileHandle, L"  Command/Address B Outputs Drive Strength: %s", g_MemoryInfo[dimm].specific.DDR3SDRAM.cmdAddrBDriveStrength);
        FPrint(FileHandle, L"  Control Signals A Outputs Drive Strength: %s", g_MemoryInfo[dimm].specific.DDR3SDRAM.ctrlSigADriveStrength);
        FPrint(FileHandle, L"  Control Signals B Outputs Drive Strength: %s", g_MemoryInfo[dimm].specific.DDR3SDRAM.ctrlSigBDriveStrength);
        FPrint(FileHandle, L"  Y1/Y1# and Y3/Y3# Clock Outputs Drive Strength: %s", g_MemoryInfo[dimm].specific.DDR3SDRAM.clkY1Y3DriveStrength);
        FPrint(FileHandle, L"  Y0/Y0# and Y2/Y2# Clock Outputs Drive Strength: %s", g_MemoryInfo[dimm].specific.DDR3SDRAM.clkY0Y2DriveStrength);
    }
}

// PopulateDDR4
//
// Output the DDR4 SPD details
void PopulateDDR4(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR4");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"DDR4 Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Maximum Clock Cycle Time, tCKmax (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tCKmax / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.tCKmax % 1000);
    FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC2 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tRFC2 / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.tRFC2 % 1000);
    FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC4 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tRFC4 / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.tRFC4 % 1000);
    FPrint(FileHandle, L"  Minimum Activate to Activate Delay diff. bank group, tRRD_Smin (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tRRD_S / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.tRRD_S % 1000);
    FPrint(FileHandle, L"  Minimum Activate to Activate Delay same bank group, tRRD_Lmin (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tRRD_L / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.tRRD_L % 1000);
    FPrint(FileHandle, L"  Minimum CAS to CAS Delay Time same bank group, tCCD_Lmin (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tCCD_L / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.tCCD_L % 1000);
    FPrint(FileHandle, L"  Minimum Four Activate Window Delay (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tFAW / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.tFAW % 1000);
    FPrint(FileHandle, L"  Maximum Activate Window in units of tREFI: %d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.tMAW);
    FPrint(FileHandle, L"  Thermal Sensor Present: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.thermalSensorPresent ? L"Yes" : L"No");

    FPrint(FileHandle, L"  DRAM Stepping: %d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.DRAMStepping);
    FPrint(FileHandle, L"  DRAM Manufacture: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.DRAMManuf);
    FPrint(FileHandle, L"  SDRAM Package Type: %s, %d die, %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.SDRAMPkgType.monolithic ? L"Monolithic" : L"Non-monolithic", g_MemoryInfo[dimm].specific.DDR4SDRAM.SDRAMPkgType.dieCount, g_MemoryInfo[dimm].specific.DDR4SDRAM.SDRAMPkgType.multiLoadStack ? L"Multi load stack" : L"Single load stack");
    FPrint(FileHandle, L"  Maxium Activate Count (MAC): %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.maxActivateCount);
    FPrint(FileHandle, L"  Post Package Repair Supported: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.PPRSupported ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Module Type: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType);

    if (StrCmp(g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType, L"LRDIMM") == 0 ||
        StrCmp(g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType, L"RDIMM") == 0 ||
        StrCmp(g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType, L"UDIMM") == 0 ||
        StrCmp(g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType, L"SO-DIMM") == 0)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR4");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }
        FPrint(FileHandle, L"  Module Height (mm): %d - %d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleHeight - 1, g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleHeight);
        FPrint(FileHandle, L"  Module Thickness (mm): front %d-%d , back %d-%d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleThicknessFront - 1, g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleThicknessFront, g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleThicknessBack - 1, g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleThicknessBack);
        FPrint(FileHandle, L"  Reference Raw Card Used: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleRefCard);
    }

    if (StrCmp(g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType, L"LRDIMM") == 0 ||
        StrCmp(g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType, L"RDIMM") == 0)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR4 RDIMM");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  # of DRAM Rows: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR4SDRAM.numDRAMRows - 1));
        FPrint(FileHandle, L"  # of Registers: %d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.numRegisters);
        FPrint(FileHandle, L"  Heat Spreader Solution Present: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.heatSpreaderSolution ? L"Yes" : L"No");

        FPrint(FileHandle, L"  Register Manufacturer: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.regManuf);
        FPrint(FileHandle, L"  Register Revision: 0x%02X\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.regRev);

        FPrint(FileHandle, L"  Chip Select Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.CSDriveStrength);
        FPrint(FileHandle, L"  Command/Address Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.cmdAddrDriveStrength);
        FPrint(FileHandle, L"  ODT Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTDriveStrength);
        FPrint(FileHandle, L"  CKE Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.CKEDriveStrength);
        FPrint(FileHandle, L"  Y0,Y2 Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.Y0Y2DriveStrength);
        FPrint(FileHandle, L"  Y1,Y3 Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.Y1Y3DriveStrength);
    }

    if (StrCmp(g_MemoryInfo[dimm].specific.DDR4SDRAM.moduleType, L"LRDIMM") == 0)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR4 LRDIMM");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Data Buffer revision: 0x%02X\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.databufferRev);

        FPrint(FileHandle, L"  MDQ Drive Strength for data rate <= 1866: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.MDQDriveStrength1866);
        FPrint(FileHandle, L"  MDQ Read Termination Strength for data rate <= 1866: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.MDQReadTermStrength1866);
        FPrint(FileHandle, L"  DRAM Drive Strength for data rate <= 1866: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.DRAMDriveStrength1866);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_WR) for data rate <= 1866: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttWR1866);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_NOM) for data rate <= 1866: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttNom1866);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_PARK), Package Ranks 0 & 1 for data rate <= 1866: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttPARK1866_R0R1);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_PARK), Package Ranks 2 & 3 for data rate <= 1866: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttPARK1866_R2R3);

        FPrint(FileHandle, L"  MDQ Drive Strength for 1866 < data rate <= 2400: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.MDQDriveStrength2400);
        FPrint(FileHandle, L"  MDQ Read Termination Strength for 1866 < data rate <= 2400: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.MDQReadTermStrength2400);
        FPrint(FileHandle, L"  DRAM Drive Strength for 1866 < data rate <= 2400: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.DRAMDriveStrength2400);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_WR) for 1866 < data rate <= 2400: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttWR2400);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_NOM) for 1866 < data rate <= 2400\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttNom2400);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_PARK), Package Ranks 0 & 1 for 1866 < data rate <= 2400: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttPARK2400_R0R1);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_PARK), Package Ranks 2 & 3 for 1866 < data rate <= 2400: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttPARK2400_R2R3);

        FPrint(FileHandle, L"  MDQ Drive Strength for 2400 < data rate <= 3200: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.MDQDriveStrength3200);
        FPrint(FileHandle, L"  MDQ Read Termination Strength for 2400 < data rate <= 3200: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.MDQReadTermStrength3200);
        FPrint(FileHandle, L"  DRAM Drive Strength for 2400 < data rate <= 3200: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.DRAMDriveStrength3200);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_WR) for 2400 < data rate <= 3200: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttWR3200);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_NOM) for 2400 < data rate <= 3200: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttNom3200);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_PARK), Package Ranks 0 & 1 for 2400 < data rate <= 3200: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttPARK3200_R0R1);
        FPrint(FileHandle, L"  DRAM ODT Strength (RTT_PARK), Package Ranks 2 & 3 for 2400 < data rate <= 3200: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.ODTRttPARK3200_R2R3);
    }
}

// PopulateDDR5
//
// Output the DDR5 SPD details
void PopulateDDR5(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"DDR5 Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Maximum Clock Cycle time, tCKmax (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.tCKmax / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.tCKmax % 1000);
    FPrint(FileHandle, L"  Minimum Write Recovery time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.tWR / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.tWR % 1000);
    FPrint(FileHandle, L"  Minimum Recovery Delay time, tRFC2 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC2 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC2 % 1000);
    FPrint(FileHandle, L"  Minimum Recovery Delay time, tRFC4 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC4 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC4 % 1000);
    FPrint(FileHandle, L"  Minimum Recovery Delay time (diff. logical rank) tRFC1_dlr (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC1_dlr / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC1_dlr % 1000);
    FPrint(FileHandle, L"  Minimum Recovery Delay time (diff. logical rank) tRFC2_dlr (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC2_dlr / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFC2_dlr % 1000);
    FPrint(FileHandle, L"  Minimum Recovery Delay time (diff. logical rank) tRFCsb_dlr (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFCsb_dlr / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.tRFCsb_dlr % 1000);

    FPrint(FileHandle, L"  Module information SPD revision: 0x%02X\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleSPDRev);
    FPrint(FileHandle, L"  SPD present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  SPD device type: %s\n", SPD_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  SPD Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecID);
        }
    }

    FPrint(FileHandle, L"  PMIC 0 present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  PMIC 0 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  PMIC 0 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID);
        }
    }
    FPrint(FileHandle, L"  PMIC 1 present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  PMIC 1 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  PMIC 1 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID);
        }
    }
    FPrint(FileHandle, L"  PMIC 2 present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  PMIC 2 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  PMIC 2 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID);
        }
    }
    FPrint(FileHandle, L"  Thermal Sensor 0 present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed0 ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Thermal Sensor 1 present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed1 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  Thermal Sensor device type: %s\n", TS_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  Thermal Sensor Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID);
        }
    }

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 Module");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  Module Type: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleType);
    FPrint(FileHandle, L"  Module Height (mm): %d - %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleHeight - 1, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleHeight);
    FPrint(FileHandle, L"  Module Thickness (mm): front %d-%d , back %d-%d \n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleThicknessFront - 1, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleThicknessFront, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleThicknessBack - 1, g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleThicknessBack);

    FPrint(FileHandle, L"  Module reference card: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.moduleRefCard);
    FPrint(FileHandle, L"  # DRAM Rows: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.DIMMAttributes.bits.NumDRAMRows - 1));
    FPrint(FileHandle, L"  Heat spreader installed: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.DIMMAttributes.bits.HeatSpreader ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Operating Temperature Range: %s\n", OPERATING_TEMP_RANGE[g_MemoryInfo[dimm].specific.DDR5SDRAM.DIMMAttributes.bits.OperatingTemperatureRange]);
    FPrint(FileHandle, L"  Rank Mix: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.ModuleOrganization.bits.RankMix ? L"Asymmetrical" : L"Symmetrical");
    FPrint(FileHandle, L"  Number of Package Ranks per Sub-Channel: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.ModuleOrganization.bits.NumPackageRanksPerSubChannel + 1);
    FPrint(FileHandle, L"  Number of Sub-Channels per DIMM: %d\n", 1 << g_MemoryInfo[dimm].specific.DDR5SDRAM.MemoryChannelBusWidth.bits.NumSubChannelsPerDIMM);
    FPrint(FileHandle, L"  Bus width extension per Sub-Channel: %d bits\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.MemoryChannelBusWidth.bits.BusWidthExtensionPerSubChannel * 4);
    FPrint(FileHandle, L"  Primary bus width per Sub-Channel: %d bits\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.MemoryChannelBusWidth.bits.PrimaryBusWidthPerSubChannel + 3));

    FPrint(FileHandle, L"  DRAM Stepping: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.DRAMStepping);
    FPrint(FileHandle, L"  DRAM Manufacture: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.DRAMManuf);

    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMDensityPackage.bits.diePerPackage == 0)
        FPrint(FileHandle, L"  SDRAM Package Type: Monolithic SDRAM\n");
    else
        FPrint(FileHandle, L"  SDRAM Package Type: %d die 3DS\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMDensityPackage.bits.diePerPackage - 1));

    FPrint(FileHandle, L"  SDRAM Density Per Die: %dGb\n", 4 * g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMDensityPackage.bits.densityPerDie);
    FPrint(FileHandle, L"  SDRAM Column Address Bits: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAddressing.bits.colAddrBits + 10);
    FPrint(FileHandle, L"  SDRAM Row Address Bits: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAddressing.bits.rowAddrBits + 16);
    FPrint(FileHandle, L"  SDRAM Device Width: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMIOWidth.bits.IOWidth + 2));
    FPrint(FileHandle, L"  SDRAM Bank Groups: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMBankGroups.bits.bankGroups));
    FPrint(FileHandle, L"  SDRAM Banks Per Bank Group: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMBankGroups.bits.banksPerGroup));

    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.ModuleOrganization.bits.RankMix) // Asymmetrical
    {
        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMDensityPackage.bits.diePerPackage == 0)
            FPrint(FileHandle, L"  Second SDRAM Package Type: Monolithic SDRAM\n");
        else
            FPrint(FileHandle, L"  Second SDRAM Package Type: %d die 3DS\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMDensityPackage.bits.diePerPackage - 1));

        FPrint(FileHandle, L"  Second SDRAM Density Per Die: %dGb\n", 4 * g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMDensityPackage.bits.densityPerDie);
        FPrint(FileHandle, L"  Second SDRAM Column Address Bits: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAddressing.bits.colAddrBits + 10);
        FPrint(FileHandle, L"  Second SDRAM Row Address Bits: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAddressing.bits.rowAddrBits + 16);
        FPrint(FileHandle, L"  Second SDRAM Device Width: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMIOWidth.bits.IOWidth + 2));
        FPrint(FileHandle, L"  Second SDRAM Bank Groups: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMBankGroups.bits.bankGroups));
        FPrint(FileHandle, L"  Second SDRAM Banks Per Bank Group: %d\n", 1 << (g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMBankGroups.bits.banksPerGroup));
    }

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 RFM");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  First SDRAM RFM RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  First SDRAM RFM RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  First SDRAM RFM Required: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RFMRequired ? L"yes" : L"no");
    FPrint(FileHandle, L"  First SDRAM RFM RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    FPrint(FileHandle, L"  Second SDRAM RFM RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  Second SDRAM RFM RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  Second SDRAM RFM Required: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RFMRequired ? L"yes" : L"no");
    FPrint(FileHandle, L"  Second SDRAM RFM RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    FPrint(FileHandle, L"  First SDRAM ARFM Level A RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  First SDRAM ARFM Level A RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  First SDRAM ARFM Level A supported: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.ARFMLevelSupported ? L"yes" : L"no");
    FPrint(FileHandle, L"  First SDRAM ARFM Level A RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    FPrint(FileHandle, L"  Second SDRAM ARFM Level A RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  Second SDRAM ARFM Level A RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  Second SDRAM ARFM Level A supported: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.ARFMLevelSupported ? L"yes" : L"no");
    FPrint(FileHandle, L"  Second SDRAM ARFM Level A RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 RFM");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  First SDRAM ARFM Level B RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  First SDRAM ARFM Level B RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  First SDRAM ARFM Level B supported: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.ARFMLevelSupported ? L"yes" : L"no");
    FPrint(FileHandle, L"  First SDRAM ARFM Level B RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    FPrint(FileHandle, L"  Second SDRAM ARFM Level B RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  Second SDRAM ARFM Level B RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  Second SDRAM ARFM Level B supported: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.ARFMLevelSupported ? L"yes" : L"no");
    FPrint(FileHandle, L"  Second SDRAM ARFM Level B RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    FPrint(FileHandle, L"  First SDRAM ARFM Level C RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  First SDRAM ARFM Level C RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  First SDRAM ARFM Level C supported: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.ARFMLevelSupported ? L"yes" : L"no");
    FPrint(FileHandle, L"  First SDRAM ARFM Level C RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    FPrint(FileHandle, L"  Second SDRAM ARFM Level C RAAMMT: %dX (FGR: %dX)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT * 2);
    FPrint(FileHandle, L"  Second SDRAM ARFM Level C RAAIMT: %d (FGR: %d)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 8, g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 4);
    FPrint(FileHandle, L"  Second SDRAM ARFM Level C supported: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.ARFMLevelSupported ? L"yes" : L"no");
    FPrint(FileHandle, L"  Second SDRAM ARFM Level C RAA Counter Decrement per REF command: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RFMRAACounterDecrementPerREFcommand ? L"RAAIMT" : L"RAAIMT / 2");

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 Features");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  sPPR Granularity: One repair element per %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.BL32PostPackageRepair.bits.sPPRGranularity ? L"bank" : L"bank group");
    FPrint(FileHandle, L"  sPPR Undo/Lock: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.BL32PostPackageRepair.bits.sPPRUndoLock ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  Burst length 32: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.BL32PostPackageRepair.bits.BL32 ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  MBIST/mPPR: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.BL32PostPackageRepair.bits.MBISTmPPR ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  mPPR/hPPR Abort: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.BL32PostPackageRepair.bits.mPPRhPPRAbort ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  PASR: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.DutyCycleAdjusterPartialArraySelfRefresh.bits.PASR ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  DCA Types Supported: %s\n", DDR5_DCA_PASR_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.DutyCycleAdjusterPartialArraySelfRefresh.bits.DCATypesSupported]);
    FPrint(FileHandle, L"  x4 RMW/ECS Writeback Suppression: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FaultHandling.bits.x4RMWECSWritebackSuppression ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  x4 RMW/ECS Writeback Suppression MR selector: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FaultHandling.bits.x4RMWECSWritebackSuppressionMRSelector ? L"MR15" : L"MR9");
    FPrint(FileHandle, L"  Bounded Fault: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.FaultHandling.bits.BoundedFault ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  SDRAM Nominal Voltage, VDDQ: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.NominalVoltageVDDQ.bits.nominal == 0 ? L"1.1V" : L"reserved");
    FPrint(FileHandle, L"  SDRAM Nominal Voltage, VPP: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.NominalVoltageVPP.bits.nominal == 0 ? L"1.8V" : L"reserved");

    // Solder Down
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0x0B)
    {
    }

    // Unbuffered Memory Module Types
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType == 2 ||
        g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType == 3)
    {
    }

    // Registered and Load Reduced Memory Module Types
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0x1 ||
        g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0x4)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 RDIMM");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Registering Clock Driver (RCD) present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.deviceType.bits.installed0 ? L"Yes" : L"No");

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  RCD Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecID);
        }

        FPrint(FileHandle, L"  RCD device type: %s\n", RCD_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.deviceType.bits.type]);
        FPrint(FileHandle, L"  Data Buffers (DB) present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.deviceType.bits.installed0 ? L"Yes" : L"No");

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  Data Buffers Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecID);
        }

        FPrint(FileHandle, L"  Data Buffers device type: %s\n", DB_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.deviceType.bits.type]);
        FPrint(FileHandle, L"  BCK_t/BCK_c: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.BCK_tBCK_c ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QDCK_t/QDCK_c: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QDCK_tQDCK_c ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QCCK_t/QCCK_c: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QCCK_tQCCK_c ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QBCK_t/QBCK_c: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QBCK_tQBCK_c ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QACK_t/QACK_c: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QACK_tQACK_c ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QBCS[1:0]_n output: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QBCS10_nOutput ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QACS[1:0]_n output: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QACS10_nOutput ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  Q[B:A]CA13 output driver: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QBACA13OutputDriver ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  BCS_n, BCOM[2:0] & BRST_n outputs: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.BCS_nBCOM20BRST_nOutputs ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  DCS1_n input buffer & QxCS1_n outputs: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.DCS1_nInputBufferQxCS1_nOutputs ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QBCA outputs: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QBCAOutputs ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QACA outputs: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QACAOutputs ? L"enabled" : L"disabled");
        FPrint(FileHandle, L"  QDCK_t/QDCK_c: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QDCK_tQDCK_c]);
        FPrint(FileHandle, L"  QCCK_t/QCCK_c: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QCCK_tQCCK_c]);
        FPrint(FileHandle, L"  QBCK_t/QBCK_c: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QBCK_tQBCK_c]);
        FPrint(FileHandle, L"  QACK_t/QACK_c: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QACK_tQACK_c]);

        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 RDIMM");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Driver Strength QxCS0_n, QxCS1_n, QCCK_t/QCCK_c: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0CQxCAQxCS_nDriverCharacteristics.bits.DriverStrengthQxCS0_nQxCS1_nQCCK_tQCCK_c]);
        FPrint(FileHandle, L"  Driver Strength Address/Command for both A&B outputs: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0CQxCAQxCS_nDriverCharacteristics.bits.DriverStrengthAddressCommandABOutputs]);
        FPrint(FileHandle, L"  Driver Strength BCK_t/BCK_c: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0DDataBufferInterfaceDriverCharacteristics.bits.DriverStrengthBCK_tBCK_c]);
        FPrint(FileHandle, L"  Driver Strength BCOM[2:0], BCS_n: %s\n", QCK_DRIVER_CHARACTERISTICS_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0DDataBufferInterfaceDriverCharacteristics.bits.DriverStrengthBCOM20BCS_n]);
        FPrint(FileHandle, L"  Q[B:A]CS[1:0]_n Single Ended Slew Rate: %s\n", SINGLE_ENDED_SLEW_RATE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0EQCKQCAQCOutputSlewRate.bits.QBACS10_nSingleEndedSlewRate]);
        FPrint(FileHandle, L"  Q[B:A]CA[13:0] Single Ended Slew Rate: %s\n", SINGLE_ENDED_SLEW_RATE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0EQCKQCAQCOutputSlewRate.bits.QBACA130SingleEndedSlewRate]);
        FPrint(FileHandle, L"  QCK[D:A]_t/QCK[D:A]_c Differential Slew Rate: %s\n", DIFFERENTIAL_SLEW_RATE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0EQCKQCAQCOutputSlewRate.bits.QCKDA_tQCKDA_cDifferentialSlewRate]);
        FPrint(FileHandle, L"  BCK_t/BCK_c Differential Slew Rate: %s\n", DIFFERENTIAL_SLEW_RATE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0FBCKBCOMBCSOutputSlewRate.bits.BCK_tBCK_cDifferentialSlewRate]);
        FPrint(FileHandle, L"  BCOM[2:0], BCS_n Single Ended slew rate: %s\n", RCDRW0F_SINGLE_ENDED_SLEW_RATE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0FBCKBCOMBCSOutputSlewRate.bits.BCOM20BCS_nSingleEndedSlewRate]);
        FPrint(FileHandle, L"  DQS RTT Park Termination: %s\n", DQS_RTT_PARK_TERMINATION_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.RDIMM.DBRW86DQSRTTParkTermination.bits.DQSRTTParkTermination]);
    }

    // Differential Memory Module Types
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0xA)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 DDIMM");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Differential Memory Buffer (DMB) present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.deviceType.bits.installed0 ? L"Yes" : L"No");

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  DMB Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecID);
        }

        FPrint(FileHandle, L"  DMB device type: %s\n", DMB_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.deviceType.bits.type]);
    }

    // NVDIMM-N Memory Module Types
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.hybrid &&
        g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.hybridMedia == 0x1)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 NVDIMM-N");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Registering Clock Driver (RCD) present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.deviceType.bits.installed0 ? L"Yes" : L"No");

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  RCD Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecID);
        }

        FPrint(FileHandle, L"  RCD device type: %s\n", RCD_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.deviceType.bits.type]);
        FPrint(FileHandle, L"  Data Buffers (DB) present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.deviceType.bits.installed0 ? L"Yes" : L"No");

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  Data Buffers Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecID);
        }

        FPrint(FileHandle, L"  Data Buffers device type: %s\n", DB_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.deviceType.bits.type]);
    }

    // NVDIMM-P Memory Module Types
    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.hybrid &&
        g_MemoryInfo[dimm].specific.DDR5SDRAM.keyByteModuleType.bits.hybridMedia == 0x2)
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"DDR5 NVDIMM-P");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Registering Clock Driver (RCD) present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.deviceType.bits.installed0 ? L"Yes" : L"No");

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  RCD Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecID);
        }

        FPrint(FileHandle, L"  RCD device type: %s\n", RCD_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.deviceType.bits.type]);
        FPrint(FileHandle, L"  Data Buffers (DB) present: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.deviceType.bits.installed0 ? L"Yes" : L"No");

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecID & 0x7F, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  Data Buffers Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecBank, g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecID);
        }

        FPrint(FileHandle, L"  Data Buffers device type: %s\n", DB_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.deviceType.bits.type]);
    }
}

// PopulateLPDDR5
//
// Output the LPDDR5 SPD details
void PopulateLPDDR5(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"LPDDR5");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"LPDDR5 Specific SPD Attributes\n");
    FPrint(FileHandle, L"  Maximum Clock Cycle time, tCKmax (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.tCKmax / 1000, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.tCKmax % 1000);
    FPrint(FileHandle, L"  Minimum Row Precharge Delay Time, Per Bank, tRP_pb (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.tRP_pb / 1000, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.tRP_pb % 1000);
    FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Command Period, Per Bank, tRFC_pb (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.tRFC_pb / 1000, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.tRFC_pb % 1000);

    FPrint(FileHandle, L"  Module information SPD revision: 0x%02X\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleSPDRev);
    FPrint(FileHandle, L"  SPD present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  SPD device type: %s\n", SPD_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecID & 0x7F, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  SPD Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecBank, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecID);
        }
    }

    FPrint(FileHandle, L"  PMIC 0 present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  PMIC 0 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID & 0x7F, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  PMIC 0 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID);
        }
    }
    FPrint(FileHandle, L"  PMIC 1 present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  PMIC 1 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID & 0x7F, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  PMIC 1 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID);
        }
    }
    FPrint(FileHandle, L"  PMIC 2 present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.installed0 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  PMIC 2 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID & 0x7F, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  PMIC 2 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID);
        }
    }
    FPrint(FileHandle, L"  Thermal Sensor 0 present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed0 ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Thermal Sensor 1 present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed1 ? L"Yes" : L"No");
    if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed0)
    {
        FPrint(FileHandle, L"  Thermal Sensor device type: %s\n", TS_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.type]);
        if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID & 0x7F, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
            FPrint(FileHandle, L"  Thermal Sensor Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID);
        }
    }

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"LPDDR5 Module");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  Module Type: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleType);
    FPrint(FileHandle, L"  Module Height (mm): %d - %d\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleHeight - 1, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleHeight);
    FPrint(FileHandle, L"  Module Thickness (mm): front %d-%d , back %d-%d \n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleThicknessFront - 1, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleThicknessFront, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleThicknessBack - 1, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleThicknessBack);

    FPrint(FileHandle, L"  Module reference card: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.moduleRefCard);
    FPrint(FileHandle, L"  # DRAM Rows: %d\n", 1 << (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.DIMMAttributes.bits.NumDRAMRows - 1));
    FPrint(FileHandle, L"  Heat spreader installed: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.DIMMAttributes.bits.HeatSpreader ? L"Yes" : L"No");
    FPrint(FileHandle, L"  Operating Temperature Range: %s\n", OPERATING_TEMP_RANGE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.DIMMAttributes.bits.OperatingTemperatureRange]);
    FPrint(FileHandle, L"  Rank Mix: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.ModuleOrganization.bits.RankMix ? L"Asymmetrical" : L"Symmetrical");
    FPrint(FileHandle, L"  Number of Package Ranks per Sub-Channel: %d\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.ModuleOrganization.bits.NumPackageRanksPerSubChannel + 1);
    FPrint(FileHandle, L"  Number of Sub-Channels per DIMM: %d\n", 1 << g_MemoryInfo[dimm].specific.LPDDR5SDRAM.MemoryChannelBusWidth.bits.NumSubChannelsPerDIMM);
    FPrint(FileHandle, L"  Bus width extension per Sub-Channel: %d bits\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.MemoryChannelBusWidth.bits.BusWidthExtensionPerSubChannel * 4);
    FPrint(FileHandle, L"  Primary bus width per Sub-Channel: %d bits\n", 1 << (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.MemoryChannelBusWidth.bits.PrimaryBusWidthPerSubChannel + 3));

    FPrint(FileHandle, L"  DRAM Stepping: %d\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.DRAMStepping);
    FPrint(FileHandle, L"  DRAM Manufacture: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.DRAMManuf);

    if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.SDRAMPackageType.bits.packageType == 0)
        FPrint(FileHandle, L"  SDRAM Package Type: Monolithic DRAM Device");
    else
        FPrint(FileHandle, L"  SDRAM Package Type: %d die Non-Monolithic Device", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.SDRAMPackageType.bits.diePerPackage + 1);

    const int SDRAM_DENSITY_PER_DIE_TABLE[16] = {0, 0, 1, 2, 4, 8, 16, 32, 12, 24, 3, 6};
    FPrint(FileHandle, L"  SDRAM Density Per Die: %dGb\n", 4 * SDRAM_DENSITY_PER_DIE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.SDRAMDensityBanks.bits.capacityPerDie]);
    const int COLUMN_ADDR_BITS[8] = {6, 6};
    FPrint(FileHandle, L"  SDRAM Column Address Bits: %d\n", COLUMN_ADDR_BITS[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.SDRAMAddressing.bits.bankColAddrBits]);
    FPrint(FileHandle, L"  SDRAM Row Address Bits: %d\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.SDRAMAddressing.bits.rowAddrBits + 12);
    FPrint(FileHandle, L"  SDRAM DRAM Die Data Width: %d bits\n", 1 << (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.LPDDR5ModuleOrganization.bits.DRAMDieDataWidth + 2));
    FPrint(FileHandle, L"  SDRAM Bank Groups: %d\n", 1 << (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.SDRAMDensityBanks.bits.bankGroupBits));
    FPrint(FileHandle, L"  SDRAM Banks Per Bank Group: %d\n", 1 << (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.SDRAMDensityBanks.bits.bankAddrBits + 2));

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"LPDDR5 Features");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"  Post Package Repair (PPR): %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.OptionalSDRAMFeatures.bits.PPR ? L"supported" : L"not supported");
    FPrint(FileHandle, L"  Soft PPR: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.OptionalSDRAMFeatures.bits.softPPR ? L"supported" : L"not supported");

    // CAMM2
    if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0x8) // CAMM2
    {
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"LPDDR5 CAMM2");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Clock Driver 0 present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.deviceType.bits.installed0 ? L"Yes" : L"No");
        if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.deviceType.bits.installed0)
        {
            FPrint(FileHandle, L"  Clock Driver 0 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.deviceType.bits.type]);
            if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecID & 0x7F, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
                FPrint(FileHandle, L"  Clock Driver 0 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecBank, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecID);
            }
        }
        FPrint(FileHandle, L"  Clock Driver 1 present: %s\n", g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.deviceType.bits.installed0 ? L"Yes" : L"No");
        if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.deviceType.bits.installed0)
        {
            FPrint(FileHandle, L"  Clock Driver 1 device type: %s\n", PMIC_DEVICE_TYPE_TABLE[g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.deviceType.bits.type]);
            if (g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecID & 0x7F, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecBank - 1, TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]));
                FPrint(FileHandle, L"  Clock Driver 1 Manufacturer: %s (Bank: %d, ID: 0x%02X)\n", TempBuf, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecBank, g_MemoryInfo[dimm].specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecID);
            }
        }
    }
}

// PopulateEPPAbbr
//
// Output the EPP abbreviated profile SPD details
void PopulateEPPAbbr(MT_HANDLE FileHandle, int dimm)
{
    int profIdx;
    CHAR16 TempBuf[128];

    // EPP abbreviated profile specific attributes
    for (profIdx = 0; profIdx < 4; profIdx++)
    {
        CHAR16 szwModType[32], szwXferSpeed[32], szwBandwidth[32];
        int tAA;
        int tRCD;
        int tRP;
        int tRAS;
        int tCK;

        if (g_MemoryInfo[dimm].specific.DDR2SDRAM.EPPSupported == FALSE ||
            g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileType != SPDINFO_DDR2_EPP_PROFILE_ABBR ||
            g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].enabled == FALSE)
            continue;

        if (FileHandle == NULL)
        {
            CHAR16 EPPProfile[16];
            UnicodeSPrint(EPPProfile, sizeof(EPPProfile), L"EPP Profile AP%d", profIdx);

            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), EPPProfile);
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        memoryType(g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tCK, &g_MemoryInfo[dimm], szwModType, szwXferSpeed, szwBandwidth, sizeof(szwModType));
        tAA = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tCK * g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].CASSupported;

        tRCD = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRCD;
        tRP = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRP;
        tRAS = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRAS;
        tCK = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tCK;

        // profile heading
        FPrint(FileHandle, L"EPP Profile AP%d\n", profIdx);
        FPrint(FileHandle, L"  Voltage level: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].voltageLevel);
        FPrint(FileHandle, L"  Clock speed (MHz): %d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].clkspeed);
        FPrint(FileHandle, L"  Transfer Speed (MT/s): %s\n", szwXferSpeed);
        FPrint(FileHandle, L"  Bandwidth (MB/s): %s\n", szwBandwidth);
        FPrint(FileHandle, L"  Minimum clock cycle time, tCK (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tCK / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tCK % 1000);
        FPrint(FileHandle, L"  Supported CAS latency: %d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].CASSupported);
        FPrint(FileHandle, L"  Minimum CAS latency time, tAA (ns): %d.%03d\n", tAA / 1000, tAA % 1000);
        FPrint(FileHandle, L"  Minimum RAS to CAS delay time, tRCD (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRCD / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRCD % 1000);
        FPrint(FileHandle, L"  Minimum row precharge time, tRP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRP / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRP % 1000);
        FPrint(FileHandle, L"  Minimum active to precharge time, tRAS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRAS / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].tRAS % 1000);
        FPrint(FileHandle, L"  Supported timing at highest clock speed: %s\n", getTimingsStr(g_MemoryInfo[dimm].type, tCK, tAA, tRCD, tRP, tRAS, TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  System CMD Rate Mode: %dT\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[profIdx].cmdRate);
    }
}

// PopulateEPPFull
//
// Output the EPP full profile SPD details
void PopulateEPPFull(MT_HANDLE FileHandle, int dimm)
{
    int profIdx;
    CHAR16 TempBuf[128];

    // EPP abbreviated profile specific attributes
    for (profIdx = 0; profIdx < 2; profIdx++)
    {
        CHAR16 szwModType[32], szwXferSpeed[32], szwBandwidth[32];
        int tAA;
        int tRCD;
        int tRP;
        int tRAS;
        int tCK;

        if (g_MemoryInfo[dimm].specific.DDR2SDRAM.EPPSupported == FALSE ||
            g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileType != SPDINFO_DDR2_EPP_PROFILE_FULL ||
            g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].enabled == FALSE)
            continue;

        if (FileHandle == NULL)
        {
            CHAR16 EPPProfile[16];
            UnicodeSPrint(EPPProfile, sizeof(EPPProfile), L"EPP Profile FP%d", profIdx);

            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), EPPProfile);

            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        memoryType(g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tCK, &g_MemoryInfo[dimm], szwModType, szwXferSpeed, szwBandwidth, sizeof(szwModType));
        tAA = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tCK * g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].CASSupported;

        tRCD = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRCD;
        tRP = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRP;
        tRAS = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRAS;
        tCK = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tCK;

        // profile heading
        FPrint(FileHandle, L"EPP Profile FP%d\n", profIdx);
        FPrint(FileHandle, L"  Voltage level: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].voltageLevel);
        FPrint(FileHandle, L"  Clock speed (MHz): %d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].clkspeed);
        FPrint(FileHandle, L"  Transfer Speed (MT/s): %s\n", szwXferSpeed);
        FPrint(FileHandle, L"  Bandwidth (MB/s): %s\n", szwBandwidth);
        FPrint(FileHandle, L"  Minimum clock cycle time, tCK (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tCK / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tCK % 1000);
        FPrint(FileHandle, L"  Supported CAS latency: %d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].CASSupported);
        FPrint(FileHandle, L"  Minimum CAS latency time, tAA (ns): %d.%03d\n", tAA / 1000, tAA % 1000);
        FPrint(FileHandle, L"  Minimum RAS to CAS delay time, tRCD (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRCD / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRCD % 1000);
        FPrint(FileHandle, L"  Minimum row precharge time, tRP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRP / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRP % 1000);
        FPrint(FileHandle, L"  Minimum active to precharge time, tRAS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRAS / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRAS % 1000);
        FPrint(FileHandle, L"  Supported timing at highest clock speed: %s\n", getTimingsStr(g_MemoryInfo[dimm].type, tCK, tAA, tRCD, tRP, tRAS, TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  System CMD Rate Mode: %dT\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].cmdRate);
        FPrint(FileHandle, L"  Minimum Write Recovery time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tWR / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tWR % 1000);
        FPrint(FileHandle, L"  Minimum Active to Auto-Refresh Delay, tRC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRC / 1000, g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].tRC % 1000);
        FPrint(FileHandle, L"  Address Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].addrDriveStrength);
        FPrint(FileHandle, L"  Chip Select Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].CSDriveStrength);
        FPrint(FileHandle, L"  Clock Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].clockDriveStrength);
        FPrint(FileHandle, L"  Data Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].dataDriveStrength);
        FPrint(FileHandle, L"  DQS Drive Strength: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].DQSDriveStrength);
        FPrint(FileHandle, L"  Address/Command Fine Delay: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].addrCmdFineDelay);
        FPrint(FileHandle, L"  Address/Command Setup Time: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].addrCmdSetupTime);
        FPrint(FileHandle, L"  Chip Select Fine Delay: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].CSFineDelay);
        FPrint(FileHandle, L"  Chip Select Setup Time: %s\n", g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[profIdx].CSSetupTime);
    }
}

// PopulateXMP
//
// Output the XMP SPD details
void PopulateXMP(MT_HANDLE FileHandle, int dimm)
{
    int profIdx;
    CHAR16 TempBuf[128];

    // EPP abbreviated profile specific attributes
    for (profIdx = 0; profIdx < 2; profIdx++)
    {
        CHAR16 szwModType[32], szwXferSpeed[32], szwBandwidth[32];
        int tAA;
        int tRCD;
        int tRP;
        int tRAS;
        int tCK;

        if (g_MemoryInfo[dimm].specific.DDR3SDRAM.XMPSupported == FALSE ||
            g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].enabled == FALSE)
            continue;

        if (FileHandle == NULL)
        {
            CHAR16 XMPProfile[32];

            if (profIdx == 0)
                UnicodeSPrint(XMPProfile, sizeof(XMPProfile), L"XMP Enthusiast / Certified Profile");
            else
                UnicodeSPrint(XMPProfile, sizeof(XMPProfile), L"XMP Extreme Profile");

            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), XMPProfile);

            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }
        memoryType(g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tCK, &g_MemoryInfo[dimm], szwModType, szwXferSpeed, szwBandwidth, sizeof(szwModType));
        tAA = g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tAA;

        tRCD = g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRCD;
        tRP = g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRP;
        tRAS = g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRAS;
        tCK = g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tCK;

        // profile heading
        if (profIdx == 0)
            FPrint(FileHandle, L"XMP Enthusiast / Certified Profile (Revision: %d.%d)\n", (g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.revision >> 4) & 0x0f, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.revision & 0x0f);
        else
            FPrint(FileHandle, L"XMP Extreme Profile (Revision: %d.%d)\n", (g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.revision >> 4) & 0x0f, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.revision & 0x0f);
        FPrint(FileHandle, L"  Module voltage: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].moduleVdd);
        FPrint(FileHandle, L"  Clock speed (MHz): %d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].clkspeed);
        FPrint(FileHandle, L"  Transfer Speed (MT/s): %s\n", szwXferSpeed);
        FPrint(FileHandle, L"  Bandwidth (MB/s): %s\n", szwBandwidth);
        FPrint(FileHandle, L"  Minimum clock cycle time, tCK (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tCK / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tCK % 1000);
        FPrint(FileHandle, L"  Supported CAS latencies: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].CASSupported);
        FPrint(FileHandle, L"  Minimum CAS latency time, tAA (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tAA / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tAA % 1000);
        FPrint(FileHandle, L"  Minimum RAS to CAS delay time, tRCD (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRCD / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRCD % 1000);
        FPrint(FileHandle, L"  Minimum row precharge time, tRP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRP / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRP % 1000);
        FPrint(FileHandle, L"  Minimum active to precharge time, tRAS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRAS / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRAS % 1000);

        FPrint(FileHandle, L"  Supported timing at highest clock speed: %s\n", getTimingsStr(g_MemoryInfo[dimm].type, tCK, tAA, tRCD, tRP, tRAS, TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  Minimum Row Active to Row Active Delay, tRRD (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRRD / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRRD % 1000);
        FPrint(FileHandle, L"  Minimum Active to Auto-Refresh Delay, tRC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRC / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRC % 1000);
        FPrint(FileHandle, L"  Minimum Recovery Delay time, tRFC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRFC / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRFC % 1000);
        FPrint(FileHandle, L"  Minimum Write Recovery time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tWR / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tWR % 1000);
        FPrint(FileHandle, L"  Minimum Write to Read CMD Delay, tWTR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tWTR / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tWTR % 1000);
        FPrint(FileHandle, L"  Minimum Read to Pre-charge CMD Delay, tRTP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRTP / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tRTP % 1000);
        FPrint(FileHandle, L"  Minimum Four Activate Window Delay, tFAW (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tFAW / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tFAW % 1000);
        FPrint(FileHandle, L"  Maximum Average Periodic Refresh Interval, tREFI (us): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tREFI / 1000, g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].tREFI % 1000);

        FPrint(FileHandle, L"  Write to Read CMD Turn-around Time Optimizations: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].WRtoRDTurnaround);
        FPrint(FileHandle, L"  Read to Write CMD Turn-around Time Optimizations: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].RDtoWRTurnaround);
        FPrint(FileHandle, L"  Back 2 Back CMD Turn-around Time Optimizations: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].back2BackTurnaround);
        FPrint(FileHandle, L"  System CMD Rate Mode: %dT\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].cmdRateMode);
        FPrint(FileHandle, L"  Memory Controller Voltage Level: %s\n", g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[profIdx].memCtrlVdd);
    }
}

// PopulateXMP2
//
// Output the XMP2 SPD details
void PopulateXMP2(MT_HANDLE FileHandle, int dimm)
{
    int profIdx;
    CHAR16 TempBuf[128];

    // EPP abbreviated profile specific attributes
    for (profIdx = 0; profIdx < 2; profIdx++)
    {
        CHAR16 szwModType[32], szwXferSpeed[32], szwBandwidth[32];
        int tAA;
        int tRCD;
        int tRP;
        int tRAS;
        int tCK;

        if (g_MemoryInfo[dimm].specific.DDR4SDRAM.XMPSupported == FALSE ||
            g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].enabled == FALSE)
            continue;

        if (FileHandle == NULL)
        {
            CHAR16 XMPProfile[32];

            if (profIdx == 0)
                UnicodeSPrint(XMPProfile, sizeof(XMPProfile), L"XMP Enthusiast / Certified Profile");
            else
                UnicodeSPrint(XMPProfile, sizeof(XMPProfile), L"XMP Extreme Profile");

            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), XMPProfile);

            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }
        memoryType(g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tCK, &g_MemoryInfo[dimm], szwModType, szwXferSpeed, szwBandwidth, sizeof(szwModType));
        tAA = g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tAA;

        tRCD = g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRCD;
        tRP = g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRP;
        tRAS = g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRAS;
        tCK = g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tCK;

        // profile heading
        if (profIdx == 0)
            FPrint(FileHandle, L"XMP Enthusiast / Certified Profile (Revision: %d.%d)\n", (g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.revision >> 4) & 0x0f, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.revision & 0x0f);
        else
            FPrint(FileHandle, L"XMP Extreme Profile (Revision: %d.%d)\n", (g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.revision >> 4) & 0x0f, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.revision & 0x0f);
        FPrint(FileHandle, L"  Module voltage: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].moduleVdd);
        FPrint(FileHandle, L"  Clock speed (MHz): %d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].clkspeed);
        FPrint(FileHandle, L"  Transfer Speed (MT/s): %s\n", szwXferSpeed);
        FPrint(FileHandle, L"  Bandwidth (MB/s): %s\n", szwBandwidth);
        FPrint(FileHandle, L"  Minimum clock cycle time, tCK (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tCK / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tCK % 1000);
        FPrint(FileHandle, L"  Supported CAS latencies: %s\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].CASSupported);
        FPrint(FileHandle, L"  Minimum CAS latency time, tAA (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tAA / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tAA % 1000);
        FPrint(FileHandle, L"  Minimum RAS to CAS delay time, tRCD (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRCD / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRCD % 1000);
        FPrint(FileHandle, L"  Minimum row precharge time, tRP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRP / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRP % 1000);
        FPrint(FileHandle, L"  Minimum active to precharge time, tRAS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRAS / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRAS % 1000);

        FPrint(FileHandle, L"  Supported timing at highest clock speed: %s\n", getTimingsStr(g_MemoryInfo[dimm].type, tCK, tAA, tRCD, tRP, tRAS, TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  Minimum Active to Auto-Refresh Delay, tRC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRC / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRC % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC1 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRFC1 / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRFC1 % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC2 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRFC2 / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRFC2 % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC4 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRFC4 / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRFC4 % 1000);
        FPrint(FileHandle, L"  Minimum Four Activate Window Delay, tFAW (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tFAW / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tFAW % 1000);
        FPrint(FileHandle, L"  Minimum Activate to Activate Delay diff. bank group, tRRD_Smin (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRRD_S / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRRD_S % 1000);
        FPrint(FileHandle, L"  Minimum Activate to Activate Delay same bank group, tRRD_Lmin (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRRD_L / 1000, g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[profIdx].tRRD_L % 1000);
    }
}

// PopulateXMP3
//
// Output the XMP3 SPD details
void PopulateXMP3(MT_HANDLE FileHandle, int dimm)
{
    int profIdx;
    CHAR16 TempBuf[128];

    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.XMPSupported == FALSE)
        return;

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"XMP 3.0");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"XMP version: %d.%d\n", BitExtract(g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.version, 7, 4), BitExtract(g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.version, 3, 0));
    FPrint(FileHandle, L"PMIC Vendor ID: %02X%02X\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.PMICVendorID[0], g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.PMICVendorID[1]);
    FPrint(FileHandle, L"Number of PMICs on DIMM: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.numPMIC);
    FPrint(FileHandle, L"PMIC capabilities:\n");
    FPrint(FileHandle, L" PMIC has capabilities for OC functions: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.PMICCapabilities.bits.OCCapable ? L"Yes" : L"No");
    FPrint(FileHandle, L" Current PMIC OC is enabled: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.PMICCapabilities.bits.OCEnable ? L"Yes" : L"No");
    FPrint(FileHandle, L" PMIC voltage default step size: %dmV\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.PMICCapabilities.bits.VoltageDefaultStepSize ? 10 : 5);
    FPrint(FileHandle, L" OC global reset functions: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.PMICCapabilities.bits.OCGlobalResetEnable ? L"Yes" : L"No");
    FPrint(FileHandle, L"Validation and Certification Capabilities:\n");
    FPrint(FileHandle, L" DIMM is self-certified by DIMM vendor: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.ValidationCertification.bits.SelfCertified ? L"Yes" : L"No");
    FPrint(FileHandle, L" PMIC Component is validated by Intel AVL level: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.ValidationCertification.bits.PMICIntelAVLValidated ? L"Yes" : L"No");
    FPrint(FileHandle, L"XMP revision: %d.%d\n", BitExtract(g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.revision, 7, 4), BitExtract(g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.revision, 3, 0));

    // EPP abbreviated profile specific attributes
    for (profIdx = 0; profIdx < MAX_XMP3_PROFILES; profIdx++)
    {
        CHAR16 szwModType[32], szwXferSpeed[32], szwBandwidth[32];
        int tAA;
        int tRCD;
        int tRP;
        int tRAS;
        int tCK;

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].enabled == FALSE)
            continue;

        if (FileHandle == NULL)
        {
            CHAR16 XMPProfile[SHORT_STRING_LEN];

            UnicodeSPrint(XMPProfile, sizeof(XMPProfile), L"XMP Profile %d \"%a\"", profIdx + 1, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].name);

            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), XMPProfile);

            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }
        memoryType(g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCK, &g_MemoryInfo[dimm], szwModType, szwXferSpeed, szwBandwidth, sizeof(szwModType));
        tAA = g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tAA;

        tRCD = g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRCD;
        tRP = g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRP;
        tRAS = g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRAS;
        tCK = g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCK;

        // profile heading
        FPrint(FileHandle, L"XMP Profile %d: \"%a\"\n", profIdx + 1, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].name);
        FPrint(FileHandle, L"  XMP Certified: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].certified ? L"Yes" : L"No");
        FPrint(FileHandle, L"  Recommended number of DIMMs per channel: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].dimmsPerChannel);
        FPrint(FileHandle, L"  Module VPP voltage: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].moduleVPP);
        FPrint(FileHandle, L"  Module VDD voltage: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].moduleVDD);
        FPrint(FileHandle, L"  Module VDDQ voltage: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].moduleVDDQ);
        FPrint(FileHandle, L"  Memory Controller voltage: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].memCtrlVoltage);
        FPrint(FileHandle, L"  Clock speed (MHz): %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].clkspeed);
        FPrint(FileHandle, L"  Transfer Speed (MT/s): %s\n", szwXferSpeed);
        FPrint(FileHandle, L"  Bandwidth (MB/s): %s\n", szwBandwidth);
        FPrint(FileHandle, L"  Minimum clock cycle time, tCK (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCK / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCK % 1000);
        FPrint(FileHandle, L"  Supported CAS latencies: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].CASSupported);
        FPrint(FileHandle, L"  Minimum CAS latency time, tAA (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tAA / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tAA % 1000);
        FPrint(FileHandle, L"  Minimum RAS to CAS delay time, tRCD (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRCD / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRCD % 1000);
        FPrint(FileHandle, L"  Minimum row precharge time, tRP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRP / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRP % 1000);
        FPrint(FileHandle, L"  Minimum active to precharge time, tRAS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRAS / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRAS % 1000);

        FPrint(FileHandle, L"  Supported timing at highest clock speed: %s\n", getTimingsStr(g_MemoryInfo[dimm].type, tCK, tAA, tRCD, tRP, tRAS, TempBuf, sizeof(TempBuf)));

        FPrint(FileHandle, L"  Minimum Active to Auto-Refresh Delay, tRC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRC / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRC % 1000);
        FPrint(FileHandle, L"  Minimum Write Recovery Time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tWR / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tWR % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC1 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRFC1 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRFC1 % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC2 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRFC2 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRFC2 % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFCsb (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRFCsb / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRFCsb % 1000);
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"additional XMP 3.0");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Minimum Read to Read Command Delay Time, Same Bank Group, tCCD_L (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_nCK);
        FPrint(FileHandle, L"  Minimum Write to Write Command Delay Time, Same Bank Group, tCCD_L_WR (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WR / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WR % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WR_nCK);
        FPrint(FileHandle, L"  Minimum Write to Write Command Delay Time, Second Write not RMW, Same Bank Group, tCCD_L_WR2 (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WR2 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WR2 % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WR2_nCK);
        FPrint(FileHandle, L"  Minimum Write to Read Command Delay Time, Same Bank Group, tCCD_L_WTR (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WTR / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WTR % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_L_WTR_nCK);
        FPrint(FileHandle, L"  Minimum Write to Read Command Delay Time, Different Bank Group, tCCD_S_WTR (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_S_WTR / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_S_WTR % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tCCD_S_WTR_nCK);
        FPrint(FileHandle, L"  Minimum Active to Active Command Delay Time, Same Bank Group, tRRD_L (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRRD_L / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRRD_L % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRRD_L_nCK);
        FPrint(FileHandle, L"  Minimum Read to Precharge Command Delay Time, tRTP (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRTP / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRTP % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tRTP_nCK);
        FPrint(FileHandle, L"  Minimum Four Activate Window, tFAW (ns): %d.%03d (%d nCK)\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tFAW / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tFAW % 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].tFAW_nCK);
        FPrint(FileHandle, L"  Advanced Memory Overclocking Features\n");
        FPrint(FileHandle, L"   Real-Time Memory Frequency Overclocking: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].AdvancedMemoryOverclockingFeatures.bits.RealTimeMemoryFrequencyOverclocking ? L"Supported" : L"Not supported");
        FPrint(FileHandle, L"   Intel Dynamic Memory Boost: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].AdvancedMemoryOverclockingFeatures.bits.IntelDynamicMemoryBoost ? L"Supported" : L"Not supported");
        FPrint(FileHandle, L"  System CMD Rate Mode: %dn\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].cmdRateMode);
        FPrint(FileHandle, L"  Vendor Personality Byte: 0x%02X\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[profIdx].VendorPersonalityByte);
    }
}

// PopulateEXPO
//
// Output the EXPO SPD details
void PopulateEXPO(MT_HANDLE FileHandle, int dimm)
{
    int profIdx;
    CHAR16 TempBuf[128];

    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPOSupported == FALSE)
        return;

    if (FileHandle == NULL)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"EXPO");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"EXPO version: %d.%d\n", BitExtract(g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.version, 7, 4), BitExtract(g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.version, 3, 0));
    FPrint(FileHandle, L"PMIC feature support:\n");
    FPrint(FileHandle, L" PMIC 10 mV step size support: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.PMICFeatureSupport.bits.stepSize10mV ? L"Yes" : L"No");

    // EXPO profile specific attributes
    for (profIdx = 0; profIdx < MAX_EXPO_PROFILES; profIdx++)
    {
        CHAR16 szwModType[32], szwXferSpeed[32], szwBandwidth[32];
        int tAA;
        int tRCD;
        int tRP;
        int tRAS;
        int tCK;

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].enabled == FALSE)
            continue;

        if (FileHandle == NULL)
        {
            CHAR16 EXPOProfile[SHORT_STRING_LEN];

            UnicodeSPrint(EXPOProfile, sizeof(EXPOProfile), L"EXPO Profile %d", profIdx + 1);

            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), EXPOProfile);

            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }
        memoryType(g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCK, &g_MemoryInfo[dimm], szwModType, szwXferSpeed, szwBandwidth, sizeof(szwModType));
        tAA = g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tAA;

        tRCD = g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRCD;
        tRP = g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRP;
        tRAS = g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRAS;
        tCK = g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCK;

        // profile heading
        FPrint(FileHandle, L"EXPO Profile %d:\n", profIdx + 1);
        FPrint(FileHandle, L"  DIMMs per channel supported: %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].dimmsPerChannel);
        FPrint(FileHandle, L"  EXPO Optional Block Support\n");
        FPrint(FileHandle, L"    Block 1 enabled: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].block1Enabled ? L"Yes" : L"No");
        FPrint(FileHandle, L"  SDRAM VDD: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].moduleVDD);
        FPrint(FileHandle, L"  SDRAM VDDQ: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].moduleVDDQ);
        FPrint(FileHandle, L"  SDRAM VPP: %s\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].moduleVPP);
        FPrint(FileHandle, L"  Clock speed (MHz): %d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].clkspeed);
        FPrint(FileHandle, L"  Transfer Speed (MT/s): %s\n", szwXferSpeed);
        FPrint(FileHandle, L"  Bandwidth (MB/s): %s\n", szwBandwidth);
        FPrint(FileHandle, L"  Minimum clock cycle time, tCK (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCK / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCK % 1000);
        FPrint(FileHandle, L"  Minimum CAS latency time, tAA (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tAA / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tAA % 1000);
        FPrint(FileHandle, L"  Minimum RAS to CAS delay time, tRCD (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRCD / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRCD % 1000);
        FPrint(FileHandle, L"  Minimum row precharge time, tRP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRP / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRP % 1000);
        FPrint(FileHandle, L"  Minimum active to precharge time, tRAS (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRAS / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRAS % 1000);

        FPrint(FileHandle, L"  Supported timing at highest clock speed: %s\n", getTimingsStr(g_MemoryInfo[dimm].type, tCK, tAA, tRCD, tRP, tRAS, TempBuf, sizeof(TempBuf)));

        FPrint(FileHandle, L"  Minimum Active to Auto-Refresh Delay, tRC (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRC / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRC % 1000);
        FPrint(FileHandle, L"  Minimum Write Recovery Time, tWR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tWR / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tWR % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC1 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRFC1 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRFC1 % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFC2 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRFC2 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRFC2 % 1000);
        FPrint(FileHandle, L"  Minimum Auto-Refresh to Active/Auto-Refresh Delay, tRFCsb (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRFCsb / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRFCsb % 1000);
        if (FileHandle == NULL)
        {
            Print(L"\n");
            Print(GetStringById(STRING_TOKEN(STR_SPD_ANY_KEY), TempBuf, sizeof(TempBuf)), L"additional EXPO");
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
        }

        FPrint(FileHandle, L"  Minimum Active to Active Command Delay Time, Same Bank Group, tRRD_L (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRRD_L / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRRD_L % 1000);
        FPrint(FileHandle, L"  Minimum Read to Read Command Delay Time, Same Bank Group, tCCD_L (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCCD_L / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCCD_L % 1000);
        FPrint(FileHandle, L"  Minimum Write to Write Command Delay Time, Same Bank Group, tCCD_L_WR (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCCD_L_WR / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCCD_L_WR % 1000);
        FPrint(FileHandle, L"  Minimum Write to Write Command Delay Time, Second Write not RMW, Same Bank Group, tCCD_L_WR2 (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCCD_L_WR2 / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tCCD_L_WR2 % 1000);
        FPrint(FileHandle, L"  Minimum Four Activate Window, tFAW (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tFAW / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tFAW % 1000);
        FPrint(FileHandle, L"  Minimum Write to Read Command Delay Time, Same Bank Group, tWTR_L (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tWTR_L / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tWTR_L % 1000);
        FPrint(FileHandle, L"  Minimum Write to Read Command Delay Time, Different Bank Group, tWTR_S (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tWTR_S / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tWTR_S % 1000);
        FPrint(FileHandle, L"  Minimum Read to Precharge Command Delay Time, tRTP (ns): %d.%03d\n", g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRTP / 1000, g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[profIdx].tRTP % 1000);
    }
}

// output_spd_info
//
// Output all SPD details
void output_spd_info(MT_HANDLE FileHandle, int dimm)
{
    CHAR16 szwRAMType[32];
    CHAR16 szwMaxXferSpeed[32];
    CHAR16 szwMaxBandwidth[32];

    CHAR16 szwMaxXferSpeed_jedec[32];
    CHAR16 szwMaxBandwidth_jedec[32];

    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[128];

    // Get Common Names etc...
    int minCK = g_MemoryInfo[dimm].tCK;
    BOOLEAN XMP = FALSE;
    BOOLEAN EXPO = FALSE;
    BOOLEAN EPP = FALSE;
    int j;

    if (g_MemoryInfo[dimm].type == SPDINFO_MEMTYPE_DDR5)
    {
        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.XMPSupported)
        {
            for (j = 0; j < MAX_XMP3_PROFILES; j++)
            {
                if (g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[j].enabled)
                {
                    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[j].tCK < minCK)
                    {
                        minCK = g_MemoryInfo[dimm].specific.DDR5SDRAM.XMP.profile[j].tCK;
                        XMP = true;
                    }
                }
            }
        }

        if (g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPOSupported)
        {
            for (j = 0; j < MAX_EXPO_PROFILES; j++)
            {
                if (g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[j].enabled)
                {
                    if (g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[j].tCK < minCK)
                    {
                        minCK = g_MemoryInfo[dimm].specific.DDR5SDRAM.EXPO.profile[j].tCK;
                        EXPO = true;
                    }
                }
            }
        }
    }
    else if (g_MemoryInfo[dimm].type == SPDINFO_MEMTYPE_DDR3)
    {
        if (g_MemoryInfo[dimm].specific.DDR3SDRAM.XMPSupported)
        {
            for (j = 0; j < 2; j++)
            {
                if (g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[j].enabled)
                {
                    if (g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[j].tCK < minCK)
                    {
                        minCK = g_MemoryInfo[dimm].specific.DDR3SDRAM.XMP.profile[j].tCK;
                        XMP = true;
                    }
                }
            }
        }
    }
    else if (g_MemoryInfo[dimm].type == SPDINFO_MEMTYPE_DDR4)
    {
        if (g_MemoryInfo[dimm].specific.DDR4SDRAM.XMPSupported)
        {
            for (j = 0; j < 2; j++)
            {
                if (g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[j].enabled)
                {
                    if (g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[j].tCK < minCK)
                    {
                        minCK = g_MemoryInfo[dimm].specific.DDR4SDRAM.XMP.profile[j].tCK;
                        XMP = true;
                    }
                }
            }
        }
    }
    else if (g_MemoryInfo[dimm].type == SPDINFO_MEMTYPE_DDR2)
    {
        if (g_MemoryInfo[dimm].specific.DDR2SDRAM.EPPSupported)
        {
            if (g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_ABBR)
            {
                for (j = 0; j < 4; j++)
                {
                    if (g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tCK < minCK)
                    {
                        minCK = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tCK;
                        EPP = true;
                    }
                }
            }
            else if (g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_FULL)
            {
                for (j = 0; j < 2; j++)
                {
                    if (g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tCK < minCK)
                    {
                        minCK = g_MemoryInfo[dimm].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tCK;
                        EPP = true;
                    }
                }
            }
        }
    }

    memoryType(minCK, &g_MemoryInfo[dimm], szwRAMType, szwMaxXferSpeed, szwMaxBandwidth, sizeof(szwRAMType));
    memoryType(g_MemoryInfo[dimm].tCK, &g_MemoryInfo[dimm], szwRAMType, szwMaxXferSpeed_jedec, szwMaxBandwidth_jedec, sizeof(szwRAMType));

    if (FileHandle == NULL)
    {
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    // Fix rounding errors
    int clkspeed = (int)1000000 / minCK;
    if ((clkspeed % 100) < 10)
        clkspeed = (clkspeed / 100) * 100;
    else if (clkspeed % 100 == 32)
        clkspeed += 1;

    FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_RAM_TYPE), TempBuf, sizeof(TempBuf)), szwRAMType);
    FPrint(FileHandle, L"  %s: %d (%s)\n", GetStringById(STRING_TOKEN(STR_SPD_MAX_CLOCK), TempBuf, sizeof(TempBuf)), clkspeed, XMP ? L"XMP" : (EXPO ? L"EXPO" : (EPP ? L"EMP" : L"JEDEC")));
    FPrint(FileHandle, L"  %s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MAX_XFER), TempBuf, sizeof(TempBuf)), szwMaxXferSpeed);
    FPrint(FileHandle, L"  %s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MAX_BANDWIDTH), TempBuf, sizeof(TempBuf)), szwMaxBandwidth);
    FPrint(FileHandle, L"%s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_MEM_CAPACITY), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].size);
    FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_JEDEC_MANUF), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].jedecManuf);
    FPrint(FileHandle, L"%s: %d.%d\n", GetStringById(STRING_TOKEN(STR_SPD_REVISION), TempBuf, sizeof(TempBuf)), (g_MemoryInfo[dimm].spdRev >> 4) & 0x0f, g_MemoryInfo[dimm].spdRev & 0x0f);
    FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_REGISTERED), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].registered ? GetStringById(STRING_TOKEN(STR_MENU_YES), TempBuf2, sizeof(TempBuf2)) : GetStringById(STRING_TOKEN(STR_MENU_NO), TempBuf2, sizeof(TempBuf2)));
    FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_ECC), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].ecc ? GetStringById(STRING_TOKEN(STR_MENU_YES), TempBuf2, sizeof(TempBuf2)) : GetStringById(STRING_TOKEN(STR_MENU_NO), TempBuf2, sizeof(TempBuf2)));
    FPrint(FileHandle, L"SPD #: %d\n", g_MemoryInfo[dimm].dimmNum);
    if (g_MemoryInfo[dimm].week)
    {
        CHAR16 ManufWeek[32];
        UnicodeSPrint(ManufWeek, sizeof(ManufWeek), GetStringById(STRING_TOKEN(STR_SPD_WEEK_YEAR), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].week, g_MemoryInfo[dimm].year);
        FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MANUF_DATE), TempBuf, sizeof(TempBuf)), ManufWeek);
    }
    else if (g_MemoryInfo[dimm].year)
    {
        CHAR16 ManufYear[32];
        UnicodeSPrint(ManufYear, sizeof(ManufYear), GetStringById(STRING_TOKEN(STR_SPD_YEAR), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].year);
        FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MANUF_DATE), TempBuf, sizeof(TempBuf)), ManufYear);
    }
    else
        FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MANUF_DATE), TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_MENU_UNKNOWN), TempBuf2, sizeof(TempBuf2)));

    FPrint(FileHandle, L"%s: %a\n", GetStringById(STRING_TOKEN(STR_SPD_MODULE_PART_NO), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].modulePartNo);
    FPrint(FileHandle, L"%s: 0x%04X\n", GetStringById(STRING_TOKEN(STR_SPD_MODULE_REV), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].moduleRev);
    if (gSPDReportExtSN)
        FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MODULE_SERIAL_NO), TempBuf, sizeof(TempBuf)), BytesToStrHex(g_MemoryInfo[dimm].moduleExtSerialNo, sizeof(g_MemoryInfo[dimm].moduleExtSerialNo), TempBuf2, sizeof(TempBuf2)));
    else
        FPrint(FileHandle, L"%s: %08X\n", GetStringById(STRING_TOKEN(STR_SPD_MODULE_SERIAL_NO), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].moduleSerialNo);
    FPrint(FileHandle, L"%s: 0x%02X\n", GetStringById(STRING_TOKEN(STR_SPD_MODULE_LOCATION), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].moduleManufLoc);
    FPrint(FileHandle, L"%s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_ROW_ADDR_BITS), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].rowAddrBits);
    FPrint(FileHandle, L"%s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_COL_ADDR_BITS), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].colAddrBits);
    FPrint(FileHandle, L"%s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_NUM_BANKS), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].numBanks);
    FPrint(FileHandle, L"%s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_NUM_RANKS), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].numRanks);
    FPrint(FileHandle, L"%s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_DEVICE_WIDTH), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].deviceWidth);
    FPrint(FileHandle, L"%s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_BUS_WIDTH), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].busWidth);
    FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MODULE_VOLTAGE), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].moduleVoltage);

    if (FileHandle == NULL)
    {
        FPrint(FileHandle, L"\n");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_SPD_TIMINGS_ANY_KEY), TempBuf, sizeof(TempBuf)));
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_CAS_LATENCIES), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].CASSupported);

    FPrint(FileHandle, L"%s: %s\n",
           GetStringById(STRING_TOKEN(STR_SPD_TIMINGS), TempBuf, sizeof(TempBuf)),
           getTimingsStr(g_MemoryInfo[dimm].type, g_MemoryInfo[dimm].tCK, g_MemoryInfo[dimm].tAA, g_MemoryInfo[dimm].tRCD, g_MemoryInfo[dimm].tRP, g_MemoryInfo[dimm].tRAS, TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"  %s: %d\n", GetStringById(STRING_TOKEN(STR_SPD_MAX_CLOCK), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].clkspeed);
    FPrint(FileHandle, L"  %s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MAX_XFER), TempBuf, sizeof(TempBuf)), szwMaxXferSpeed_jedec);
    FPrint(FileHandle, L"  %s: %s\n", GetStringById(STRING_TOKEN(STR_SPD_MAX_BANDWIDTH), TempBuf, sizeof(TempBuf)), szwMaxBandwidth_jedec);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_CK), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tCK / 1000, g_MemoryInfo[dimm].tCK % 1000);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_AA), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tAA / 1000, g_MemoryInfo[dimm].tAA % 1000);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_RCD), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tRCD / 1000, g_MemoryInfo[dimm].tRCD % 1000);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_RP), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tRP / 1000, g_MemoryInfo[dimm].tRP % 1000);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_RAS), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tRAS / 1000, g_MemoryInfo[dimm].tRAS % 1000);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_RRD), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tRRD / 1000, g_MemoryInfo[dimm].tRRD % 1000);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_RC), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tRC / 1000, g_MemoryInfo[dimm].tRC % 1000);
    FPrint(FileHandle, L"  %s: %d.%03d\n", GetStringById(STRING_TOKEN(STR_SPD_T_RFC), TempBuf, sizeof(TempBuf)), g_MemoryInfo[dimm].tRFC / 1000, g_MemoryInfo[dimm].tRFC % 1000);

#ifdef PRO_RELEASE
    // Specific info lables determined by module 1
    switch (g_MemoryInfo[dimm].type)
    {
    case SPDINFO_MEMTYPE_SDRAM:
        PopulateSDRAM(FileHandle, dimm);
        break;
    case SPDINFO_MEMTYPE_DDR:
        PopulateDDR1(FileHandle, dimm);
        break;
    case SPDINFO_MEMTYPE_DDR2:
        PopulateDDR2(FileHandle, dimm);
        PopulateEPPAbbr(FileHandle, dimm);
        PopulateEPPFull(FileHandle, dimm);
        break;
    case SPDINFO_MEMTYPE_DDR2FB:
        PopulateDDR2FB(FileHandle, dimm);
        break;
    case SPDINFO_MEMTYPE_DDR3:
        PopulateDDR3(FileHandle, dimm);
        PopulateXMP(FileHandle, dimm);
        break;
    case SPDINFO_MEMTYPE_DDR4:
    case SPDINFO_MEMTYPE_DDR4E:
        PopulateDDR4(FileHandle, dimm);
        PopulateXMP2(FileHandle, dimm);
        break;
    case SPDINFO_MEMTYPE_DDR5:
        PopulateDDR5(FileHandle, dimm);
        PopulateXMP3(FileHandle, dimm);
        PopulateEXPO(FileHandle, dimm);
        break;

    case SPDINFO_MEMTYPE_LPDDR5:
    case SPDINFO_MEMTYPE_LPDDR5X:
        PopulateLPDDR5(FileHandle, dimm);
        break;

    case SPDINFO_MEMTYPE_DDR2FBPROBE:
    default:
        break;
    }
#else
    if (FileHandle == NULL)
    {
        FPrint(FileHandle, L"\n");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_ERROR_SPD_PRO), TempBuf, sizeof(TempBuf)));
    }
#endif

    if (FileHandle == NULL)
    {
        FPrint(FileHandle, L"\n");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)));
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }
}

void output_smbios_info(MT_HANDLE FileHandle, int dimm)
{
    static const CHAR16 *FORM_FACTOR_STR[] = {
        L"Unknown",
        L"Other",
        L"Unknown",
        L"SIMM",
        L"SIP",
        L"Chip",
        L"DIP",
        L"ZIP",
        L"Proprietary Card",
        L"DIMM",
        L"TSOP",
        L"Row of chips",
        L"RIMM",
        L"SODIMM",
        L"SRIMM",
        L"FB-DIMM",
        L"Die"};

    static const CHAR16 *MEMORY_TYPE_STR[] = {
        L"Unknown",
        L"Other",
        L"Unknown",
        L"DRAM",
        L"EDRAM",
        L"VRAM",
        L"SRAM",
        L"RAM",
        L"ROM",
        L"FLASH",
        L"EEPROM",
        L"FEPROM",
        L"EPROM",
        L"CDRAM",
        L"3DRAM",
        L"SDRAM",
        L"SGRAM",
        L"RDRAM",
        L"DDR",
        L"DDR2",
        L"DDR2 FB-DIMM",
        L"Reserved",
        L"Reserved",
        L"Reserved",
        L"DDR3",
        L"FBD2",
        L"DDR4",
        L"LPDDR",
        L"LPDDR2",
        L"LPDDR3",
        L"LPDDR4",
        L"Logical non-volatile device",
        L"HBM (High Bandwidth Memory)",
        L"HBM2 (High Bandwidth Memory Generation 2)",
        L"DDR5",
        L"LPDDR5"};

    static const CHAR16 *MEMORY_TECH_STR[] = {
        L"Unknown",
        L"Other",
        L"Unknown",
        L"DRAM",
        L"NVDIMM-N",
        L"NVDIMM-F",
        L"NVDIMM-P",
        L"Intel Optane persistent memory"};

    static const CHAR16 *MEMORY_OP_MODE_CAP[] = {
        L"Unknown",
        L"Other",
        L"Unknown",
        L"Volatile memory",
        L"Byte-accessible persistent memory",
        L"Block-accessible persistent memory",
        L"Unknown",
        L"Unknown",
        L"Unknown",
        L"Unknown",
        L"Unknown",
        L"Unknown",
        L"Unknown",
        L"Unknown",
        L"Unknown",
        L""};

    UINT64 DimmSizeMB = 0;
    UINT64 SpeedMTS = 0;
    CHAR16 TempBuf[128];

    if (g_SMBIOSMemDevices[dimm].smb17.Size == 0)
        FPrint(FileHandle, L"<Empty slot>\n");

    if (g_SMBIOSMemDevices[dimm].smb17.TotalWidth != 0xffff)
        FPrint(FileHandle, L"%s: %d bits\n", L"Total Width", g_SMBIOSMemDevices[dimm].smb17.TotalWidth);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Total Width", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.DataWidth != 0xffff)
        FPrint(FileHandle, L"%s: %d bits\n", L"Data Width", g_SMBIOSMemDevices[dimm].smb17.DataWidth);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Data Width", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.Size == 0x7fff) // Need to use Extended Size field
    {
        DimmSizeMB = g_SMBIOSMemDevices[dimm].smb17.ExtendedSize;
    }
    else
    {
        if (BitExtract(g_SMBIOSMemDevices[dimm].smb17.Size, 15, 15) == 0)                           // MB - bit 15 is the unit MB or KB
            DimmSizeMB = g_SMBIOSMemDevices[dimm].smb17.Size;                                       // express in B //BIT601000.0012 overflow corrected
        else                                                                                        // KB
            DimmSizeMB = DivU64x32(MultU64x32(g_SMBIOSMemDevices[dimm].smb17.Size, 1024), 1048576); // express in B
    }

    FPrint(FileHandle, L"%s: %ld MB\n", L"Size", DimmSizeMB);
    FPrint(FileHandle, L"%s: %s\n", L"Form Factor", FORM_FACTOR_STR[g_SMBIOSMemDevices[dimm].smb17.FormFactor]);
    FPrint(FileHandle, L"%s: %d\n", L"Device Set", g_SMBIOSMemDevices[dimm].smb17.DeviceSet);
    FPrint(FileHandle, L"%s: %a\n", L"Device Locator", g_SMBIOSMemDevices[dimm].szDeviceLocator);
    FPrint(FileHandle, L"%s: %a\n", L"Bank Locator", g_SMBIOSMemDevices[dimm].szBankLocator);
    FPrint(FileHandle, L"%s: %s\n", L"Memory Type", MEMORY_TYPE_STR[g_SMBIOSMemDevices[dimm].smb17.MemoryType]);

    TempBuf[0] = L'\0';

    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.FastPaged)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Fast-paged, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.StaticColumn)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Static column, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.PseudoStatic)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Pseudo-static, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.Rambus)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"RAMBUS, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.Synchronous)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Synchronous, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.Cmos)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"CMOS, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.Edo)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"EDO, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.WindowDram)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Window DRAM, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.CacheDram)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Cache DRAM, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.Nonvolatile)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Non-volatile, ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.Registered)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Registered (Buffered), ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.Unbuffered)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"Unbuffered (Unregistered), ");
    if (g_SMBIOSMemDevices[dimm].smb17.TypeDetail.LrDimm)
        StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"LRDIMM, ");

    if (TempBuf[0] != L'\0')
        TempBuf[StrLen(TempBuf) - 2] = L'\0';

    FPrint(FileHandle, L"%s: %s\n", L"Type Detail", TempBuf);
    if (g_SMBIOSMemDevices[dimm].smb17.Speed == 0xffff) // Need to use Extended Speed field
    {
        SpeedMTS = g_SMBIOSMemDevices[dimm].smb17.ExtendedSpeed;
    }
    else
    {
        SpeedMTS = g_SMBIOSMemDevices[dimm].smb17.Speed;
    }
    if (SpeedMTS != 0)
        FPrint(FileHandle, L"%s: %ldMT/s\n", L"Speed", SpeedMTS);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Speed", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"%s: %a\n", L"Manufacturer", g_SMBIOSMemDevices[dimm].szManufacturer);
    FPrint(FileHandle, L"%s: %a\n", L"Serial Number", g_SMBIOSMemDevices[dimm].szSerialNumber);
    FPrint(FileHandle, L"%s: %a\n", L"Asset Tag", g_SMBIOSMemDevices[dimm].szAssetTag);
    FPrint(FileHandle, L"%s: %a\n", L"Part Number", g_SMBIOSMemDevices[dimm].szPartNumber);
    FPrint(FileHandle, L"%s: %08X\n", L"Attributes", g_SMBIOSMemDevices[dimm].smb17.Attributes);
    if (g_SMBIOSMemDevices[dimm].smb17.ConfiguredMemoryClockSpeed == 0xffff) // Need to use Extended Speed field
        SpeedMTS = g_SMBIOSMemDevices[dimm].smb17.ExtendedConfiguredMemorySpeed;
    else
        SpeedMTS = g_SMBIOSMemDevices[dimm].smb17.ConfiguredMemoryClockSpeed;

    if (SpeedMTS != 0)
        FPrint(FileHandle, L"%s: %ldMT/s\n", L"Configured Memory Speed", SpeedMTS);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Configured Memory Speed", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.MinimumVoltage != 0)
        FPrint(FileHandle, L"%s: %d mV\n", L"Minimum Voltage", g_SMBIOSMemDevices[dimm].smb17.MinimumVoltage);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Minimum Voltage", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.MaximumVoltage != 0)
        FPrint(FileHandle, L"%s: %d mV\n", L"Maximum Voltage", g_SMBIOSMemDevices[dimm].smb17.MaximumVoltage);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Maximum Voltage", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.ConfiguredVoltage != 0)
        FPrint(FileHandle, L"%s: %d mV\n", L"Configured Voltage", g_SMBIOSMemDevices[dimm].smb17.ConfiguredVoltage);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Configured Voltage", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (FileHandle == NULL)
    {
        FPrint(FileHandle, L"\n");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_SPD_MORE_ANY_KEY), TempBuf, sizeof(TempBuf)), L"SMBIOS");
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }

    FPrint(FileHandle, L"%s: %s\n", L"Memory Technology", MEMORY_TECH_STR[g_SMBIOSMemDevices[dimm].smb17.MemoryTechnology]);
    FPrint(FileHandle, L"%s: %s\n", L"Memory Operating Mode Capability", MEMORY_OP_MODE_CAP[g_SMBIOSMemDevices[dimm].smb17.MemoryOperatingModeCapability.Uint16]);
    FPrint(FileHandle, L"%s: %s\n", L"Firmware Version", g_SMBIOSMemDevices[dimm].szFirmwareVersion);

    if (g_SMBIOSMemDevices[dimm].smb17.ModuleManufacturerID != 0)
        FPrint(FileHandle, L"%s: %04X\n", L"Module Manufacturer ID", g_SMBIOSMemDevices[dimm].smb17.ModuleManufacturerID);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Module Manufacturer ID", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.ModuleProductID != 0)
        FPrint(FileHandle, L"%s: %04X\n", L"Module Product ID", g_SMBIOSMemDevices[dimm].smb17.ModuleProductID);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Module Product ID", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.MemorySubsystemControllerManufacturerID != 0)
        FPrint(FileHandle, L"%s: %04X\n", L"Memory Subsystem Controller Manufacturer ID", g_SMBIOSMemDevices[dimm].smb17.MemorySubsystemControllerManufacturerID);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Memory Subsystem Controller Manufacturer ID", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.MemorySubsystemControllerProductID != 0)
        FPrint(FileHandle, L"%s: %04X\n", L"Memory Subsystem Controller Product ID", g_SMBIOSMemDevices[dimm].smb17.MemorySubsystemControllerProductID);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Memory Subsystem Controller Product ID", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.NonVolatileSize != 0)
        FPrint(FileHandle, L"%s: %ld MB\n", L"Non Volatile Size", g_SMBIOSMemDevices[dimm].smb17.NonVolatileSize);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Non Volatile Size", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.VolatileSize != 0)
        FPrint(FileHandle, L"%s: %ld MB\n", L"Volatile Size", g_SMBIOSMemDevices[dimm].smb17.VolatileSize);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Volatile Size", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.CacheSize != 0)
        FPrint(FileHandle, L"%s: %ld MB\n", L"Cache Size", g_SMBIOSMemDevices[dimm].smb17.CacheSize);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Cache Size", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (g_SMBIOSMemDevices[dimm].smb17.LogicalSize != 0)
        FPrint(FileHandle, L"%s: %ld MB\n", L"Logical Size", g_SMBIOSMemDevices[dimm].smb17.LogicalSize);
    else
        FPrint(FileHandle, L"%s: %s\n", L"Logical Size", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));

    if (FileHandle == NULL)
    {
        FPrint(FileHandle, L"\n");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)));
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }
}

// display_memmap
//
// Output the EFI memory map
void display_memmap(MT_HANDLE FileHandle)
{
    EFI_STATUS Status;
    UINTN NumberOfEntries;
    UINTN Loop;
    UINTN Size;
    EFI_MEMORY_DESCRIPTOR *MemoryMap;
    EFI_MEMORY_DESCRIPTOR *MapEntry;
    UINTN MapKey;
    UINTN DescSize;
    UINT32 DescVersion;
    UINT64 Available;
    UINT64 Reserved;
    UINT64 SizeInBytes;
    CHAR16 TempBuf[128];

    if (FileHandle == NULL)
    {
        gST->ConOut->ClearScreen(gST->ConOut);
        gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);
    }

    Size = 0;
    Status = gBS->GetMemoryMap(&Size, NULL, NULL, NULL, NULL);

    if (Status != EFI_BUFFER_TOO_SMALL)
        return;

    Size = Size * 2; // Double the size just in case it changes

    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool(Size);
    if (MemoryMap == NULL)
        return;

    Status = gBS->GetMemoryMap(&Size, MemoryMap, &MapKey, &DescSize, &DescVersion);
    if (EFI_ERROR(Status))
    {
        FreePool(MemoryMap);
        return;
    }

    if (DescVersion != EFI_MEMORY_DESCRIPTOR_VERSION)
    {
        FreePool(MemoryMap);
        return;
    }

    NumberOfEntries = Size / DescSize;

    for (
        MapEntry = MemoryMap, Loop = 0, Available = 0, Reserved = 0;
        Loop < NumberOfEntries;
        MapEntry = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MapEntry + DescSize),
       Loop++)
    {
        EFI_STRING_ID MEMTYPE_STR_ID[] =
            {STRING_TOKEN(STR_MEMMAP_RSVD_MEM),
             STRING_TOKEN(STR_MEMMAP_LOADER_CODE),
             STRING_TOKEN(STR_MEMMAP_LOADER_DATA),
             STRING_TOKEN(STR_MEMMAP_BOOT_CODE),
             STRING_TOKEN(STR_MEMMAP_BOOT_DATA),
             STRING_TOKEN(STR_MEMMAP_RUNTIME_CODE),
             STRING_TOKEN(STR_MEMMAP_RUNTIME_DATA),
             STRING_TOKEN(STR_MEMMAP_FREE_MEM),
             STRING_TOKEN(STR_MEMMAP_UNUSABLE_MEM),
             STRING_TOKEN(STR_MEMMAP_ACPI_TABLES),
             STRING_TOKEN(STR_MEMMAP_ACPI_NV),
             STRING_TOKEN(STR_MEMMAP_OS_MAPPED_IO),
             STRING_TOKEN(STR_MEMMAP_SYS_MAPPED_IO),
             STRING_TOKEN(STR_MEMMAP_PAL_CODE),
             STRING_TOKEN(STR_MEMMAP_PERSISTENT_MEM)};

        UINT8 SocketId = 0xff;
        UINT8 MemoryControllerId = 0xff;
        UINT8 ChannelId = 0xff;
        UINT8 DimmSlot = 0xff;
        UINT8 DimmRank = 0xff;
        CHAR16 DimmAddrBuf[128];

        SizeInBytes = MultU64x32(MapEntry->NumberOfPages, EFI_PAGE_SIZE);

        DimmAddrBuf[0] = L'\0';
        if (FileHandle != NULL && iMSSystemAddressToDimmAddress(MapEntry->PhysicalStart, &SocketId, &MemoryControllerId, &ChannelId, &DimmSlot, &DimmRank) == EFI_SUCCESS)
            UnicodeSPrint(DimmAddrBuf, sizeof(DimmAddrBuf), L"{Sock: %d, Ctrl: %d, ChId: %d, DimmSl: %d, DimmRk: %d} ", SocketId, MemoryControllerId, ChannelId, DimmSlot, DimmRank);

        if (SizeInBytes >= SIZE_2MB)
        {
            FPrint(FileHandle, L"0x%012lx - 0x%012lx (%ldMB) %s{%s}\n",
                   MapEntry->PhysicalStart,
                   MapEntry->PhysicalStart + SizeInBytes - 1,
                   DivU64x32(SizeInBytes, SIZE_1MB),
                   DimmAddrBuf,
                   GetStringById(MEMTYPE_STR_ID[MapEntry->Type], TempBuf, sizeof(TempBuf)));
        }
        else
        {
            FPrint(FileHandle, L"0x%012lx - 0x%012lx (%ldKB) %s{%s}\n",
                   MapEntry->PhysicalStart,
                   MapEntry->PhysicalStart + SizeInBytes - 1,
                   DivU64x32(SizeInBytes, SIZE_1KB),
                   DimmAddrBuf,
                   GetStringById(MEMTYPE_STR_ID[MapEntry->Type], TempBuf, sizeof(TempBuf)));
        }

        if (FileHandle == NULL && Loop != 0 && Loop % (ConHeight - 3) == 0)
        {
            FPrint(FileHandle, L"\n");
            FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)));
            MtUiWaitForKey(0, NULL);
            gST->ConOut->ClearScreen(gST->ConOut);
            gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);
        }

        if (MapEntry->Type == EfiConventionalMemory)
            Available += MapEntry->NumberOfPages;
        else
            Reserved += MapEntry->NumberOfPages;
    }

    FPrint(FileHandle, L"\n");

    SizeInBytes = MultU64x32(Available, EFI_PAGE_SIZE);

    if (SizeInBytes >= SIZE_4GB)
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldGB)\n", GetStringById(STRING_TOKEN(STR_MENU_BENCH_AVAIL_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, DivU64x64Remainder(SizeInBytes, SIZE_1GB, NULL));
    }
    else if (SizeInBytes >= SIZE_2MB)
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldMB)\n", GetStringById(STRING_TOKEN(STR_MENU_BENCH_AVAIL_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, DivU64x32(SizeInBytes, SIZE_1MB));
    }
    else if (SizeInBytes >= SIZE_2KB)
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldKB)\n", GetStringById(STRING_TOKEN(STR_MENU_BENCH_AVAIL_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, DivU64x32(SizeInBytes, SIZE_1KB));
    }
    else
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldB)\n", GetStringById(STRING_TOKEN(STR_MENU_BENCH_AVAIL_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, SizeInBytes);
    }
    SizeInBytes = MultU64x32(Reserved, EFI_PAGE_SIZE);

    if (SizeInBytes >= SIZE_4GB)
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldGB)\n", GetStringById(STRING_TOKEN(STR_MEMMAP_RSVD_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, DivU64x64Remainder(SizeInBytes, SIZE_1GB, NULL));
    }
    else if (SizeInBytes >= SIZE_2MB)
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldMB)\n", GetStringById(STRING_TOKEN(STR_MEMMAP_RSVD_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, DivU64x32(SizeInBytes, SIZE_1MB));
    }
    else if (SizeInBytes >= SIZE_2KB)
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldKB)\n", GetStringById(STRING_TOKEN(STR_MEMMAP_RSVD_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, DivU64x32(SizeInBytes, SIZE_1KB));
    }
    else
    {
        FPrint(FileHandle, L"%s: 0x%lx (%ldB)\n", GetStringById(STRING_TOKEN(STR_MEMMAP_RSVD_MEM), TempBuf, sizeof(TempBuf)), SizeInBytes, SizeInBytes);
    }
    FreePool(MemoryMap);

    if (FileHandle == NULL)
    {
        FPrint(FileHandle, L"\n");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)));
        MtUiWaitForKey(0, NULL);
        gST->ConOut->ClearScreen(gST->ConOut);
    }
}

void change_save_location()
{
    UINTN i;
    EFI_FILE_PROTOCOL *NewSaveDir = NULL;

    for (i = 0; i < NumFSHandles; i++)
    {
        EFI_FILE_PROTOCOL *RootFs = EfiLibOpenRoot(FSHandleList[i]);

        if (RootFs)
        {
            EFI_FILE_SYSTEM_INFO *VolumeInfo = EfiLibFileSystemInfo(RootFs);
            if (VolumeInfo)
            {
                UINT64 FreeSpaceMB = DivU64x32(VolumeInfo->FreeSpace, 1048576);
                UINT64 VolumeMB = DivU64x32(VolumeInfo->VolumeSize, 1048576);
                UINT64 PctFree = VolumeInfo->VolumeSize > 0 ? DivU64x64Remainder(DivU64x32(VolumeInfo->FreeSpace, 100), VolumeInfo->VolumeSize, NULL) : 0;

                FPrint(NULL, L"%c[%x] FS Label: \"%s\", Mode: %s, Free space: %ld MB / %ld MB (%ld%%)\n", (INTN)i == SaveFSHandle ? L'*' : L' ', i, VolumeInfo->VolumeLabel, VolumeInfo->ReadOnly ? L"RO" : L"RW", FreeSpaceMB, VolumeMB, PctFree);
                FreePool(VolumeInfo);
            }
            RootFs->Close(RootFs);
        }
    }

    if (DataEfiDir)
        FPrint(NULL, L"%c[Esc] USB flash drive boot directory\n", SaveFSHandle < 0 ? L'*' : L' ');

    while (TRUE)
    {
        UINTN fs;
        Print(L"\nEnter the FS# (0-%d), or (Esc) for default: ", NumFSHandles - 1);

        if (getval(gST->ConOut->Mode->CursorColumn, gST->ConOut->Mode->CursorRow, 3, 1, &fs) == 0)
        {
            if (fs < NumFSHandles)
            {
                NewSaveDir = EfiLibOpenRoot(FSHandleList[fs]);
                if (NewSaveDir)
                {
                    EFI_FILE_SYSTEM_INFO *VolumeInfo = EfiLibFileSystemInfo(NewSaveDir);
                    if (VolumeInfo)
                    {
                        if (VolumeInfo->ReadOnly == FALSE)
                            AsciiFPrint(DEBUG_FILE_HANDLE, "Changing save location to: [%d] FS Label: \"%s\"", fs, VolumeInfo->VolumeLabel);
                        else
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to change to read-only save location: [%d] FS Label: \"%s\"", fs, VolumeInfo->VolumeLabel);
                            Print(L"\nFS#%d is read-only", fs);
                            NewSaveDir->Close(NewSaveDir);
                            NewSaveDir = NULL;
                            FreePool(VolumeInfo);
                            continue;
                        }

                        FreePool(VolumeInfo);
                    }
                }
                SaveFSHandle = fs;
                break;
            }
            else
                Print(L"\nPlease enter a valid FS#");
        }
        else
        {
            SaveFSHandle = -1;
            AsciiFPrint(DEBUG_FILE_HANDLE, "Changing save location to: Boot media directory");
            break;
        }
    }

    if (SaveDir != NULL)
        SaveDir->Close(SaveDir);
    SaveDir = NewSaveDir;

    AsciiFPrint(DEBUG_FILE_HANDLE, "=============================================");

    // Log MemTest86 version
#ifdef MDE_CPU_X64
    AsciiFPrint(DEBUG_FILE_HANDLE, "%s %s Build: %s (64-bit)", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
#elif defined(MDE_CPU_IA32)
    AsciiFPrint(DEBUG_FILE_HANDLE, "%s %s Build: %s (32-bit)", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
#endif
    AsciiFPrint(DEBUG_FILE_HANDLE, "=============================================");

    if (SaveFSHandle >= 0)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Changed to new save location to: FS%d", SaveFSHandle);
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Changed to new save location to: Boot media directory");
}

void getCPUSpeedLine(int khzspeed, int khzspeedturbo, CHAR16 *szwCPUSpeed, UINTN bufSize)
{
    if (khzspeed /* cpu_speed */ < 999499)
    {
        UINT32 speed = khzspeed + 50; /* for rounding */
        UnicodeSPrint(szwCPUSpeed, bufSize, L"%3d.%1d MHz", speed / 1000, (speed / 100) % 10);
    }
    else
    {
        UINT32 speed = khzspeed + 500; /* for rounding */
        UnicodeSPrint(szwCPUSpeed, bufSize, L"%d MHz", speed / 1000);
    }

    if (khzspeedturbo > 0 && khzspeedturbo > khzspeed)
    {
        CHAR16 TempBuf[32];
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L" [Turbo: %d.%1d MHz]", khzspeedturbo / 1000, (khzspeedturbo % 1000) / 100);
        StrCatS(szwCPUSpeed, bufSize / sizeof(CHAR16), TempBuf);
    }
}

#if 1 // test configuration in the console. No longer used

#define CINBUF 18
int getval(int x, int y, int len, int decimalonly, UINTN *val)
{
    UINTN tempval;
    int done;
    int i, n;
    int base;
    int shift;
    CHAR16 buf[CINBUF + 1];
    EFI_INPUT_KEY Key;

    gST->ConOut->EnableCursor(gST->ConOut, FALSE);

    for (i = 0; i < sizeof(buf) / sizeof(buf[0]); i++)
    {
        buf[i] = L' ';
    }
    buf[sizeof(buf) / sizeof(buf[0]) - 1] = L'\0';

    done = 0;
    n = 0;
    base = 10;
    while (!done)
    {

        /* Read a new character and process it */
        MtUiWaitForKey(0, &Key);

        switch (Key.UnicodeChar)
        {
        case 13: /* CR */
            /* If something has been entered we are done */
            if (n)
                done = 1;
            break;

        case 8: /* BS */
            if (n > 0)
            {
                n -= 1;
                buf[n] = L' ';
            }
            break;

        case 0x0:
        {
            switch (Key.ScanCode)
            {
            case 0x26: /* ^L/L - redraw the display */
                break;
            case 0x17: /* Esc */
                return 1;
            }
        }
        break;

        case L'p': /* p */
            if (decimalonly == 0 && n < len)
                buf[n] = L'p';
            break;
        case L'g': /* g */
            if (decimalonly == 0 && n < len)
                buf[n] = L'g';
            break;
        case L'm': /* m */
            if (decimalonly == 0 && n < len)
                buf[n] = L'm';
            break;
        case L'k': /* k */
            if (decimalonly == 0 && n < len)
                buf[n] = L'k';
            break;
        case L'x': /* x */
            /* Only allow 'x' after an initial 0 */
            if (decimalonly == 0 && n == 1 && (buf[0] == L'0'))
            {
                if (n < len)
                    buf[n] = L'x';
            }
            break;

            /* Don't allow entering a number not in our current base */
        case L'0':
            if (base >= 1)
                if (n < len)
                    buf[n] = L'0';
            break;
        case L'1':
            if (base >= 2)
                if (n < len)
                    buf[n] = L'1';
            break;
        case L'2':
            if (base >= 3)
                if (n < len)
                    buf[n] = L'2';
            break;
        case L'3':
            if (base >= 4)
                if (n < len)
                    buf[n] = L'3';
            break;
        case L'4':
            if (base >= 5)
                if (n < len)
                    buf[n] = L'4';
            break;
        case L'5':
            if (base >= 6)
                if (n < len)
                    buf[n] = L'5';
            break;
        case L'6':
            if (base >= 7)
                if (n < len)
                    buf[n] = L'6';
            break;
        case L'7':
            if (base >= 8)
                if (n < len)
                    buf[n] = L'7';
            break;
        case L'8':
            if (base >= 9)
                if (n < len)
                    buf[n] = L'8';
            break;
        case L'9':
            if (base >= 10)
                if (n < len)
                    buf[n] = L'9';
            break;
        case L'a':
            if (base >= 11)
                if (n < len)
                    buf[n] = L'a';
            break;
        case L'b':
            if (base >= 12)
                if (n < len)
                    buf[n] = L'b';
            break;
        case L'c':
            if (base >= 13)
                if (n < len)
                    buf[n] = L'c';
            break;
        case L'd':
            if (base >= 14)
                if (n < len)
                    buf[n] = L'd';
            break;
        case L'e':
            if (base >= 15)
                if (n < len)
                    buf[n] = L'e';
            break;
        case L'f':
            if (base >= 16)
                if (n < len)
                    buf[n] = L'f';
            break;
        default:
            break;
        }
        /* Don't allow anything to be entered after a suffix */
        if (n > 0 && ((buf[n - 1] == L'p') || (buf[n - 1] == L'g') ||
                      (buf[n - 1] == L'm') || (buf[n - 1] == L'k')))
        {
            buf[n] = L' ';
        }
        /* If we have entered a character increment n */
        if (buf[n] != L' ')
        {
            n++;
        }
        buf[n] = L' ';
        /* Print the current number */
        buf[len] = L'\0';
        ConsolePrintXY(x, y, buf);

        /* Find the base we are entering numbers in */
        base = 10;
        if ((buf[0] == L'0') && (buf[1] == L'x'))
        {
            base = 16;
        }
#if 0
        else if (buf[0] == '0') {
            base = 8;
        }
#endif
    }
    /* Compute our current shift */
    shift = 0;
    switch (buf[n - 1])
    {
    case L'g': /* gig */
        shift = 30;
        break;
    case L'm': /* meg */
        shift = 20;
        break;
    case L'p': /* page */
        shift = 12;
        break;
    case L'k': /* kilo */
        shift = 10;
        break;
    }

    /* Compute our current value */
    if (base == 16)
        tempval = StrHexToUintn(buf);
    else
        tempval = StrDecimalToUintn(buf);

    if (shift > 0)
    {
        /*
        if (shift >= 32) {
            val = 0xffffffff;
        } else {
            val <<= shift;
        }
        */
        tempval <<= shift;
    }
    else
    {
        /*
        if (-shift >= 32) {
            val = 0;
        }
        else {
            val >>= -shift;
        }
        */
        tempval >>= -shift;
    }

    gST->ConOut->EnableCursor(gST->ConOut, TRUE);
    *val = tempval;
    return 0;
}

/* Get a comma seperated list of numbers */
void get_list(int x, int y, int len, CHAR16 *buf)
{
    EFI_INPUT_KEY Key;
    int n = 0;

    gST->ConOut->EnableCursor(gST->ConOut, FALSE);

    len--;

    while (1)
    {
        /* Read a new character and process it */
        MtUiWaitForKey(0, &Key);

        switch (Key.UnicodeChar)
        {
        case 13: /* CR */
                 /* If something has been entered we are done */
            if (n)
            {
                buf[n] = 0;
                goto Done;
            }
            break;

        case 8: /* BS */
            if (n > 0)
            {
                n -= 1;
                buf[n] = L' ';
            }
            break;

        case L'0':
            if (n < len)
                buf[n++] = '0';
            break;
        case L'1':
            if (n < len)
                buf[n++] = '1';
            break;
        case L'2':
            if (n < len)
                buf[n++] = '2';
            break;
        case L'3':
            if (n < len)
                buf[n++] = '3';
            break;
        case L'4':
            if (n < len)
                buf[n++] = '4';
            break;
        case L'5':
            if (n < len)
                buf[n++] = '5';
            break;
        case L'6':
            if (n < len)
                buf[n++] = '6';
            break;
        case L'7':
            if (n < len)
                buf[n++] = '7';
            break;
        case L'8':
            if (n < len)
                buf[n++] = '8';
            break;
        case L'9':
            if (n < len)
                buf[n++] = '9';
            break;
        case L',':
            if (n < len)
                buf[n++] = ',';
            break;
        }
        ConsolePrintXY(x, y, buf);
    }

Done:
    gST->ConOut->EnableCursor(gST->ConOut, TRUE);
}

UINT16
EFIAPI
MtSupportTestConfig()
{
    while (TRUE)
    {
        if (gOknTestStart) {
            gOknTestStart = FALSE;
            gOknTestStatus = 1;
            return ID_BUTTON_START;
        }
        if (gOknTestStatus == 0) {

        }
        if (gOknTestReset == 0 || gOknTestReset == 1 || gOknTestReset == 2) {
            gRT->ResetSystem(gOknTestReset, 0, 0, NULL);
        }
        gBS->CheckEvent(gST->ConIn->WaitForKey);
        // gBS->Stall(50000);
    }
    return ID_BUTTON_START;

    EFI_INPUT_KEY key;

    gST->ConOut->ClearScreen(gST->ConOut);

    key.UnicodeChar = 0;

    while (key.UnicodeChar != L'0')
    {
        gST->ConOut->ClearScreen(gST->ConOut);
        MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                      L"PassMark"
                      L" " PROGRAM_NAME L" " PROGRAM_VERSION,
                      L"",
                      L"(i) Display System Info                      ",
                      L"(t) Test Selection                           ",
                      L"(a) Address Range                            ",
                      L"(c) CPU Selection                            ",
                      L"(r) RAM Benchmark                            ",
                      L"(s) Start Test                               ",
                      L"(x) Exit                                     ",
                      NULL);

        switch (key.UnicodeChar)
        {
        case L'i':
        {
            EFI_INPUT_KEY subkey;
            subkey.UnicodeChar = 0;
            while (subkey.UnicodeChar != L'0')
            {
                gST->ConOut->ClearScreen(gST->ConOut);
                MtCreatePopUp(gST->ConOut->Mode->Attribute, &subkey,
                              L"Display System Info",
                              L"",
                              L"(1) Display CPU Info                         ",
                              L"(2) Display Memory Info                      ",
                              L"(3) View Detailed RAM (SPD) Info             ",
                              L"(4) View Memory Usage                        ",
                              L"(5) Save System Info summary to file         ",
                              L"(0) Go back                                  ",
                              NULL);
                switch (subkey.UnicodeChar)
                {

                case L'1':
                {
                    MSR_TEMP CPUTemp;

                    CHAR16 TempBuf[128];
                    CHAR16 TextBuf[LONG_STRING_LEN];

                    gST->ConOut->ClearScreen(gST->ConOut);

                    cpu_type(TextBuf, sizeof(TextBuf));
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TYPE), TempBuf, sizeof(TempBuf)), TextBuf);

                    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_SPEED), TempBuf, sizeof(TempBuf)), TextBuf);

                    // CPU Temperature
                    SetMem(&CPUTemp, sizeof(CPUTemp), 0);

                    if (GetCPUTemp(&gCPUInfo, MPSupportGetBspId(), &CPUTemp) == EFI_SUCCESS)
                    {
                        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dC", CPUTemp.iTemperature);
                    }
                    else
                    {
                        GetStringById(STRING_TOKEN(STR_MENU_NA), TextBuf, sizeof(TextBuf));
                    }
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TEMP), TempBuf, sizeof(TempBuf)), TextBuf);

                    // # Logical Processors
                    if (MPSupportGetNumProcessors() == MPSupportGetNumEnabledProcessors())
                        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d", MPSupportGetNumProcessors());
                    else
                        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_AVAIL), TempBuf, sizeof(TempBuf)), MPSupportGetNumProcessors(), MPSupportGetNumEnabledProcessors());

                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_NUM_CPU), TempBuf, sizeof(TempBuf)), TextBuf);

                    // L1 Cache
                    if (gCPUInfo.L1_data_caches_per_package > 0)
                        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d x %dK (%d MB/s)", gCPUInfo.L1_data_caches_per_package, gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
                    else
                        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dK (%d MB/s)", gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L1_CACHE), TempBuf, sizeof(TempBuf)), TextBuf);

                    // L2 Cache
                    if (gCPUInfo.L2_caches_per_package > 0)
                        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d x %dK (%d MB/s)", gCPUInfo.L2_caches_per_package, gCPUInfo.L2_cache_size, l2_speed);
                    else
                        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dK (%d MB/s)", gCPUInfo.L2_cache_size, l2_speed);
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L2_CACHE), TempBuf, sizeof(TempBuf)), TextBuf);

                    // L3 Cache
                    if (gCPUInfo.L3_cache_size)
                    {
                        if (gCPUInfo.L3_caches_per_package > 0)
                            UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d x %dK (%d MB/s)", gCPUInfo.L3_caches_per_package, gCPUInfo.L3_cache_size, l3_speed);
                        else
                            UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dK (%d MB/s)", gCPUInfo.L3_cache_size, l3_speed);
                    }
                    else
                    {
                        GetStringById(STRING_TOKEN(STR_MENU_NA), TextBuf, sizeof(TextBuf));
                    }

                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)), TextBuf);

                    FPrint(NULL, L"\n");
                    FPrint(NULL, GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)));
                    MtUiWaitForKey(0, NULL);
                    gST->ConOut->ClearScreen(gST->ConOut);
                }
                break;

                case L'2':
                {
                    CHAR16 TempBuf[128];

                    CHAR16 TextBuf[LONG_STRING_LEN];

                    gST->ConOut->ClearScreen(gST->ConOut);

                    // Total Physical Memory
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%ldM (%d MB/s)", MtSupportMemSizeMB(), mem_speed);
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_TOTAL_MEM), TempBuf, sizeof(TempBuf)), TextBuf);

                    // Memory latency
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d.%03d ns", mem_latency / 1000, mem_latency % 1000);
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_MEM_LATENCY), TempBuf, sizeof(TempBuf)), TextBuf);

                    // ECC Enabled
                    MtSupportGetECCFeatures(TextBuf, sizeof(TextBuf));
                    FPrint(NULL, L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_ENABLED), TempBuf, sizeof(TempBuf)), TextBuf);

                    FPrint(NULL, L"\n");
                    FPrint(NULL, GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)));
                    MtUiWaitForKey(0, NULL);
                    gST->ConOut->ClearScreen(gST->ConOut);
                }
                break;

                case L'3':
                {
                    if (g_numMemModules == 0)
                    {
                        EFI_INPUT_KEY tempkey;
                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                      L" The RAM info could not be detected.            ",
                                      L" Please send the log file to help@passmark.com. ",
                                      L" Press any key to continue.                     ",
                                      NULL);
                    }
                    else
                    {
                        EFI_INPUT_KEY subkey2;

                        subkey2.UnicodeChar = 0;
                        subkey2.ScanCode = 0;

                        while (subkey2.UnicodeChar != L'0')
                        {
                            gST->ConOut->ClearScreen(gST->ConOut);

                            MtCreatePopUp(gST->ConOut->Mode->Attribute, &subkey2,
                                          L"View Detailed RAM (SPD) Info",
                                          L"",
                                          L"(1) View RAM (SPD) info                      ",
                                          L"(2) Save RAM (SPD) info to disk              ",
                                          L"(0) Go back                                  ",
                                          NULL);
                            switch (subkey2.UnicodeChar)
                            {
                            case L'1':
                            {
                                while (TRUE)
                                {
                                    int i;
                                    UINTN dimm;

                                    gST->ConOut->ClearScreen(gST->ConOut);

                                    for (i = 0; i < g_numMemModules; i++)
                                    {
                                        CHAR16 szwRAMType[32];
                                        CHAR16 szwMaxXferSpeed[32];
                                        CHAR16 szwMaxBandwidth[32];
                                        CHAR16 szwTiming[32];

                                        memoryType(g_MemoryInfo[i].tCK, &g_MemoryInfo[i], szwRAMType, szwMaxXferSpeed, szwMaxBandwidth, sizeof(szwRAMType));

#define CEIL(dividend, divisor) ((dividend + divisor - 1) / divisor)
                                        UnicodeSPrint(szwTiming, sizeof(szwTiming), L"%d-%d-%d-%d",
                                                      CEIL(g_MemoryInfo[i].tAA, g_MemoryInfo[i].tCK),
                                                      CEIL(g_MemoryInfo[i].tRCD, g_MemoryInfo[i].tCK),
                                                      CEIL(g_MemoryInfo[i].tRP, g_MemoryInfo[i].tCK),
                                                      CEIL(g_MemoryInfo[i].tRAS, g_MemoryInfo[i].tCK));

                                        Print(L"SPD #%d: %s %s (%d MHz) / %s / %s\n", i, szwMaxBandwidth, szwRAMType, g_MemoryInfo[i].clkspeed, g_MemoryInfo[i].jedecManuf, szwTiming);
                                    }

                                    Print(L"\nEnter the DIMM# (0-%d), or (Esc) to go back: ", g_numMemModules - 1);

                                    if (getval(gST->ConOut->Mode->CursorColumn, gST->ConOut->Mode->CursorRow, 3, 1, &dimm) == 0)
                                    {
                                        if ((int)dimm < g_numMemModules)
                                        {
                                            output_spd_info(NULL, (int)dimm);
                                        }
                                        else
                                        {
                                            EFI_INPUT_KEY tempkey;
                                            gST->ConOut->ClearScreen(gST->ConOut);
                                            MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                                          L"The DIMM number is invalid.",
                                                          NULL);
                                        }
                                    }
                                    else // Esc
                                        break;
                                }
                            }
                            break;

                            case L'2':
                            {
#ifdef PRO_RELEASE
                                MT_HANDLE FileHandle = NULL;
                                EFI_STATUS Status;

                                EFI_TIME Time;
                                CHAR16 Filename[128];
                                CHAR16 Prefix[128];

                                gRT->GetTime(&Time, NULL);

                                Prefix[0] = L'\0';
                                MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
                                UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.txt", Prefix, RAMINFO_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                                Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                                if (FileHandle == NULL)
                                {
                                    CHAR16 TempBuf[128];

                                    EFI_INPUT_KEY tempkey;

                                    gST->ConOut->ClearScreen(gST->ConOut);
                                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                                  GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), TempBuf, sizeof(TempBuf)),
                                                  NULL);
                                }
                                else
                                {
                                    CHAR16 TempBuf[128];
                                    CHAR16 TempBuf2[128];

                                    EFI_INPUT_KEY tempkey;

                                    UINTN BOMSize = 2;

                                    int i;

                                    // Write UTF16LE BOM
                                    MtSupportWriteFile(FileHandle, &BOMSize, (void *)BOM_UTF16LE);

                                    // Program Summary Header
                                    FPrint(FileHandle, L"%s %s Build: %s\r\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
                                    FPrint(FileHandle, L"PassMark Software\r\nwww.passmark.com\r\n\r\n");

                                    // Memory Summary
                                    FPrint(FileHandle, L"%s:\r\n", GetStringById(STRING_TOKEN(STR_REPORT_MEM_SUMMARY), TempBuf, sizeof(TempBuf)));
                                    FPrint(FileHandle, L"%s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM_SLOTS), TempBuf, sizeof(TempBuf)), g_numSMBIOSMem);
                                    FPrint(FileHandle, L"%s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM), TempBuf, sizeof(TempBuf)), g_numSMBIOSModules);
                                    FPrint(FileHandle, L"%s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_SPD), TempBuf, sizeof(TempBuf)), g_numMemModules);
                                    FPrint(FileHandle, L"%s: %4dM\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_TOTAL_MEM), TempBuf, sizeof(TempBuf)), MtSupportMemSizeMB());

                                    FPrint(FileHandle, L"%s:\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SPD_DETAILS), TempBuf, sizeof(TempBuf)));
                                    for (i = 0; i < g_numMemModules; i++)
                                    {
                                        FPrint(FileHandle, L"\r\nSPD #: %d\r\n", g_MemoryInfo[i].dimmNum);
                                        FPrint(FileHandle, L"==============\r\n");
                                        output_spd_info(FileHandle, i);
                                    }
                                    if (g_numMemModules == 0)
                                        FPrint(FileHandle, L"%s:\r\n", GetStringById(STRING_TOKEN(STR_ERROR_SPD), TempBuf, sizeof(TempBuf)));

                                    FPrint(FileHandle, L"\r\n%s:\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SMBIOS_DETAILS), TempBuf, sizeof(TempBuf)));
                                    for (i = 0; i < g_numSMBIOSMem; i++)
                                    {
                                        FPrint(FileHandle, L"\r\nMemory Device #: %d\r\n", i);
                                        FPrint(FileHandle, L"==============\r\n");
                                        if (g_SMBIOSMemDevices[i].smb17.Size != 0)
                                            output_smbios_info(FileHandle, i);
                                        else
                                            FPrint(FileHandle, L"<%s>\r\n", GetStringById(STRING_TOKEN(STR_REPORT_EMPTY), TempBuf, sizeof(TempBuf)));
                                    }
                                    MtSupportCloseFile(FileHandle);

                                    UnicodeSPrint(TempBuf2, sizeof(TempBuf2), GetStringById(STRING_TOKEN(STR_MENU_SPD_SAVED), TempBuf, sizeof(TempBuf)), Filename);

                                    gST->ConOut->ClearScreen(gST->ConOut);
                                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                                  TempBuf2,
                                                  NULL);
                                }
#else // Pro version only
                                CHAR16 ErrorMsg[128];

                                EFI_INPUT_KEY tempkey;

                                gST->ConOut->ClearScreen(gST->ConOut);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                              GetStringById(STRING_TOKEN(STR_ERROR_PRO_ONLY), ErrorMsg, sizeof(ErrorMsg)),
                                              NULL);
#endif
                            }
                            break;

                            case L'0':
                                subkey.UnicodeChar = L'0';
                                break;
                            }
                        }
                    }
                }
                break;

                case L'4':
                {
                    display_memmap(NULL);
                }
                break;

                case L'5':
                {
#ifdef PRO_RELEASE
                    MT_HANDLE FileHandle = NULL;
                    EFI_STATUS Status;

                    EFI_TIME Time;
                    CHAR16 Filename[128];
                    CHAR16 Prefix[128];

                    gRT->GetTime(&Time, NULL);

                    Prefix[0] = L'\0';
                    MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
                    UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.txt", Prefix, SYSINFO_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                    Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                    if (FileHandle == NULL)
                    {
                        CHAR16 TempBuf[128];
                        EFI_INPUT_KEY tempkey;

                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                      GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), TempBuf, sizeof(TempBuf)),
                                      NULL);
                    }
                    else
                    {
                        CHAR16 TempBuf[128];
                        CHAR16 TempBuf2[128];

                        EFI_INPUT_KEY tempkey;

                        UINTN BOMSize = 2;

                        // Write UTF16LE BOM
                        MtSupportWriteFile(FileHandle, &BOMSize, (void *)BOM_UTF16LE);

                        // Program Summary Header
                        FPrint(FileHandle, L"%s %s Build: %s\r\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
                        FPrint(FileHandle, L"PassMark Software\r\nwww.passmark.com\r\n\r\n");

                        // Hardware Info
                        FPrint(FileHandle, L"%s\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SYSINFO), TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"=============================================\r\n");
                        MtSupportOutputSysInfo(FileHandle);

                        MtSupportCloseFile(FileHandle);

                        UnicodeSPrint(TempBuf2, sizeof(TempBuf2), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_SAVED), TempBuf, sizeof(TempBuf)), Filename);

                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                      TempBuf2,
                                      NULL);
                    }
#else // Pro version only
                    CHAR16 ErrorMsg[128];

                    EFI_INPUT_KEY tempkey;

                    gST->ConOut->ClearScreen(gST->ConOut);
                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                  GetStringById(STRING_TOKEN(STR_ERROR_PRO_ONLY), ErrorMsg, sizeof(ErrorMsg)),
                                  NULL);
#endif
                }
                break;
                }
            }
        }
        break;

        case L't':
        {
            EFI_INPUT_KEY subkey;
            subkey.UnicodeChar = 0;
            while (subkey.UnicodeChar != L'0')
            {
                CHAR16 TempBuf[128];

                int i;

                gST->ConOut->ClearScreen(gST->ConOut);

                // Display all available processors that can be used for testing
                Print(L"Selected Tests: ");

                for (i = 0; i < gNumCustomTests; i++)
                {
                    if (gCustomTestList[i].Enabled)
                    {
                        Print(L"%d ", gCustomTestList[i].TestNo);
                    }
                }

                Print(L"\n\n");
                Print(GetStringById(STRING_TOKEN(STR_MENU_TEST_PASSES), TempBuf, sizeof(TempBuf)), gNumPasses);

                MtCreatePopUp(gST->ConOut->Mode->Attribute, &subkey,
                              L"Test Selection",
                              L"",
                              L"(1) All Tests                                ",
                              L"(2) Enter Test List                          ",
                              L"(3) Set # of Passes                          ",
                              L"(0) Go back                                  ",
                              NULL);

                switch (subkey.UnicodeChar)
                {
                case L'1':
                {
                    EFI_INPUT_KEY tempkey;

                    for (i = 0; i < gNumCustomTests; i++)
                        gCustomTestList[i].Enabled = TRUE;

                    gST->ConOut->ClearScreen(gST->ConOut);
                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                  L"Selected tests have been restored to default",
                                  NULL);
                }
                break;

                case L'2':
                {
                    int testno;
                    CHAR16 cp[64];

                    BOOLEAN TestSelected[MAXTESTS]; // TRUE if test is selected
                    int NumSel = 0;

                    EFI_INPUT_KEY tempkey;

                    SetMem(cp, 64 * sizeof(cp[0]), 0);

                    SetMem(TestSelected, sizeof(TestSelected), FALSE);

                    gST->ConOut->ClearScreen(gST->ConOut);

                    for (i = 0; i < gNumCustomTests; i++)
                    {
                        Print(L"%s\n", gCustomTestList[i].NameStr ? gCustomTestList[i].NameStr : GetStringById(gCustomTestList[i].NameStrId, TempBuf, sizeof(TempBuf)));
                    }

                    Print(L"\nEnter a comma separated list of tests to execute: ");

                    get_list(gST->ConOut->Mode->CursorColumn, gST->ConOut->Mode->CursorRow, 32, cp);

                    /* Now enable all of the tests in the
                     * list */

                    i = testno = 0;
                    while (1)
                    {
                        if (cp[i] >= L'0' && cp[i] <= L'9')
                        {
                            int n = cp[i] - L'0';
                            testno = testno * 10 + n;
                            i++;
                            if (cp[i] == ',' || cp[i] == 0)
                            {
                                int j;
                                for (j = 0; j < gNumCustomTests; j++)
                                {
                                    if (gCustomTestList[j].TestNo == (UINT32)testno)
                                    {
                                        if (gCustomTestList[j].NameStrId == STRING_TOKEN(STR_TEST_12_NAME) && !gSIMDSupported) // If user is selecting the 128-bit test and SSE is not supported, display error
                                        {
                                            gST->ConOut->ClearScreen(gST->ConOut);
                                            MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                                          GetStringById(STRING_TOKEN(STR_ERROR_TEST_NO_SIMD), TempBuf, sizeof(TempBuf)),
                                                          NULL);
                                        }
                                        else if (gCustomTestList[j].NameStrId == STRING_TOKEN(STR_TEST_14_NAME) && DMATestPart == NULL) // If user is selecting the DMA test and DMA Test partition is not found, display error
                                        {
                                            gST->ConOut->ClearScreen(gST->ConOut);
                                            MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                                          GetStringById(STRING_TOKEN(STR_ERROR_DMA_TEST), TempBuf, sizeof(TempBuf)),
                                                          NULL);
                                        }
                                        else
                                        {
                                            TestSelected[j] = TRUE;
                                            NumSel++;
                                        }
                                    }
                                }

                                if (cp[i] == 0)
                                    break;
                                testno = 0;
                                i++;
                            }
                        }
                    }

                    if (NumSel == 0)
                    {
                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                      GetStringById(STRING_TOKEN(STR_ERROR_SELECTED_TESTS), TempBuf, sizeof(TempBuf)),
                                      NULL);
                        break;
                    }

                    for (i = 0; i < gNumCustomTests; i++)
                        gCustomTestList[i].Enabled = TestSelected[i];

                    gST->ConOut->ClearScreen(gST->ConOut);
                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                  L"Tests have been selected accordingly",
                                  NULL);
                }
                break;

                case L'3':
                {
                    while (TRUE)
                    {
                        UINTN Columns;
                        UINTN Rows;
                        int x, y;
                        UINTN NumPasses = 0;

                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, NULL,
                                      L" Enter # of passes of the test sequence to perform, ",
                                      L" or (Esc) to go back:                               ",
                                      NULL);

                        //
                        // Retrieve the number of columns and rows in the current console mode
                        //
                        gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &Columns, &Rows);
                        y = (int)(Rows - 5) / 2 + 2;
                        x = (int)(Columns - 54) / 2 + 23;
                        if (getval(x, y, 3, 1, &NumPasses) == 0)
                        {
                            if (NumPasses > 0 && NumPasses < 1000)
                            {
#ifndef PRO_RELEASE // Free version
                                if (NumPasses > DEFAULT_NUM_PASSES)
                                {
                                    CHAR16 TempBuf2[128];

                                    EFI_INPUT_KEY tempkey;

                                    GetStringById(STRING_TOKEN(STR_ERROR_NUM_PASSES_FREE), TempBuf, sizeof(TempBuf));
                                    UnicodeSPrint(TempBuf2, sizeof(TempBuf2), TempBuf, DEFAULT_NUM_PASSES);

                                    gST->ConOut->ClearScreen(gST->ConOut);
                                    SetMem(&tempkey, sizeof(tempkey), 0);
                                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                                  TempBuf2,
                                                  NULL);
                                }
                                else
                                {
                                    gNumPasses = NumPasses;
                                    break;
                                }
#endif
                                gNumPasses = NumPasses;
                                break;
                            }
                            else
                            {
                                EFI_INPUT_KEY tempkey;
                                gST->ConOut->ClearScreen(gST->ConOut);

                                SetMem(&tempkey, sizeof(tempkey), 0);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                              L"The number of passes specified is invalid.",
                                              L"Please enter a valid number of passes     ",
                                              NULL);
                            }
                        }
                        else // Esc
                            break;
                    }
                }
                break;

                case L'0':
                    subkey.UnicodeChar = L'0';
                    break;
                }
            }
        }
        break;

        case L'a':
        {
            EFI_INPUT_KEY subkey;
            subkey.UnicodeChar = 0;
            while (subkey.UnicodeChar != L'0')
            {
                CHAR16 TempBuf[128];

                gST->ConOut->ClearScreen(gST->ConOut);

                // Display all available processors that can be used for testing
                Print(GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_LOWER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitLo);
                Print(L"\n");
                Print(GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_UPPER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitHi);
                Print(L"\n");
                Print(GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_MAXADDR), TempBuf, sizeof(TempBuf)), maxaddr);
                Print(L"\n");

                MtCreatePopUp(gST->ConOut->Mode->Attribute, &subkey,
                              L"Test Address Range:",
                              L"",
                              L"(1) Set Lower Limit                          ",
                              L"(2) Set Upper Limit                          ",
                              L"(3) Test All Memory                          ",
                              L"(0) Go back                                  ",
                              NULL);
                switch (subkey.UnicodeChar)
                {
                case L'1':
                {
                    while (TRUE)
                    {
                        UINTN page;
                        UINTN Columns;
                        UINTN Rows;
                        int x, y;
                        CHAR16 CurBuf[128];
                        CHAR16 MaxBuf[128];

                        if (sizeof(void *) > 4)
                        {
                            UnicodeSPrint(CurBuf, 128, L" Cur: % 16lx                 ", gAddrLimitLo);
                            UnicodeSPrint(MaxBuf, 128, L" Max: % 16lx                 ", maxaddr);
                        }
                        else
                        {
                            UnicodeSPrint(CurBuf, 128, L" Cur: % 16x                 ", gAddrLimitLo);
                            UnicodeSPrint(MaxBuf, 128, L" Max: % 16x                 ", maxaddr);
                        }

                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, NULL,
                                      L"Lower Limit (Page Aligned):",
                                      CurBuf,
                                      MaxBuf,
                                      L"                                       ",
                                      L" Enter an address or (Esc) to go back: ",
                                      L"                                       ",
                                      L"  Eg. Hex or decimal address:          ",
                                      L"          0x10000, 4096                ",
                                      L"  Eg. Suffixes (k,m,g):                ",
                                      L"          5m (5 megabytes)             ",
                                      NULL);

                        //
                        // Retrieve the number of columns and rows in the current console mode
                        //
                        gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &Columns, &Rows);
                        y = (int)(Rows - 12) / 2 + 6;
                        x = (int)(Columns - 41) / 2 + 9;
                        if (getval(x, y, CINBUF, 0, &page) == 0)
                        {
                            page &= ~0x0FFF;
                            if (page < gAddrLimitHi) // Check if lower limit is lower than upper limit
                            {
                                gAddrLimitLo = page;
                                break;
                            }
                            else
                            {
                                EFI_INPUT_KEY tempkey;
                                gST->ConOut->ClearScreen(gST->ConOut);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                              GetStringById(STRING_TOKEN(STR_ERROR_BAD_LOWER_ADDR), TempBuf, sizeof(TempBuf)),
                                              NULL);
                            }
                        }
                        else
                            break;
                    }
                }
                break;

                case L'2':
                {
                    while (TRUE)
                    {
                        UINTN page;
                        UINTN Columns;
                        UINTN Rows;
                        int x, y;
                        CHAR16 CurBuf[128];
                        CHAR16 MaxBuf[128];

                        if (sizeof(void *) > 4)
                        {
                            UnicodeSPrint(CurBuf, 128, L" Cur: % 16lx                 ", gAddrLimitHi);
                            UnicodeSPrint(MaxBuf, 128, L" Max: % 16lx                 ", maxaddr);
                        }
                        else
                        {
                            UnicodeSPrint(CurBuf, 128, L" Cur: % 16x                 ", gAddrLimitHi);
                            UnicodeSPrint(MaxBuf, 128, L" Max: % 16x                 ", maxaddr);
                        }
                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, NULL,
                                      L"Upper Limit: (Page Aligned)",
                                      CurBuf,
                                      MaxBuf,
                                      L"                                       ",
                                      L" Enter an address or (Esc) to go back: ",
                                      L"                                       ",
                                      L"  Eg. Hex or decimal address:          ",
                                      L"          0x10000, 4096                ",
                                      L"  Eg. Suffixes (k,m,g):                ",
                                      L"          5m (5 megabytes)             ",
                                      NULL);

                        //
                        // Retrieve the number of columns and rows in the current console mode
                        //
                        gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &Columns, &Rows);
                        y = (int)(Rows - 12) / 2 + 6;
                        x = (int)(Columns - 41) / 2 + 9;
                        if (getval(x, y, CINBUF, 0, &page) == 0)
                        {
                            page &= ~0x0FFF;
                            if (page > gAddrLimitLo && page <= maxaddr) // Check if upper limit is greater than lower limit, and less than the max system address
                            {
                                gAddrLimitHi = page;
                                break;
                            }
                            else if (page <= gAddrLimitLo)
                            {
                                EFI_INPUT_KEY tempkey;
                                gST->ConOut->ClearScreen(gST->ConOut);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                              GetStringById(STRING_TOKEN(STR_ERROR_BAD_UPPER_ADDR), TempBuf, sizeof(TempBuf)),
                                              NULL);
                            }
                            else if (page > maxaddr)
                            {
                                EFI_INPUT_KEY tempkey;
                                gST->ConOut->ClearScreen(gST->ConOut);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                              GetStringById(STRING_TOKEN(STR_ERROR_INVALID_ADDR), TempBuf, sizeof(TempBuf)),
                                              NULL);
                            }
                        }
                        else
                            break;
                    }
                }
                break;

                case L'3':
                {
                    gAddrLimitLo = minaddr;
                    gAddrLimitHi = (UINTN)maxaddr;
                }
                break;

                case L'0':
                    break;
                }
            }
        }
        break;

        case L'c':
        {
            EFI_INPUT_KEY subkey;
            subkey.UnicodeChar = 0;
            while (subkey.UnicodeChar != L'0')
            {
                int i;

                CHAR16 TempBuf[128];

                gST->ConOut->ClearScreen(gST->ConOut);

                // If multiprocessing is not support, display warning
                if (!gMPSupported)
                    Print(L"%s\n\n", GetStringById(STRING_TOKEN(STR_WARN_MULTIPROCESSOR), TempBuf, sizeof(TempBuf)));
                else if (gRestrictFlags & RESTRICT_FLAG_MP)
                    Print(L"%s\n\n", GetStringById(STRING_TOKEN(STR_WARN_MP_BLACKLIST), TempBuf, sizeof(TempBuf)));

                // Display all available processors that can be used for testing
                Print(L"%s", GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_AVAIL_CPUS), TempBuf, sizeof(TempBuf)));
                for (i = 0; i < (int)MPSupportGetMaxProcessors(); i++)
                {
                    if (MPSupportIsProcEnabled(i))
                    {
                        Print(L"%d ", i);
                    }
                }

                Print(L"\n\nCurrent CPU Mode: ");
                switch (gCPUSelMode)
                {
                case CPU_SINGLE:
                    Print(GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_SINGLE), TempBuf, sizeof(TempBuf)), gSelCPUNo);
                    break;
                case CPU_PARALLEL:
                    Print(GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_PARALLEL), TempBuf, sizeof(TempBuf)));
                    break;
                case CPU_RROBIN:
                    Print(GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_RROBIN), TempBuf, sizeof(TempBuf)));
                    break;
                case CPU_SEQUENTIAL:
                    Print(GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_SEQUENTIAL), TempBuf, sizeof(TempBuf)));
                    break;
                default:
                    break;
                }

                MtCreatePopUp(gST->ConOut->Mode->Attribute, &subkey,
                              L"CPU Selection",
                              L"",
                              L"(1) Single                                   ",
                              L"(2) Parallel (All CPUs)                      ",
                              L"(3) Round Robin (All CPUs)                   ",
                              L"(4) Sequential (All CPUs)                    ",
                              L"(0) Go back                                  ",
                              NULL);
                switch (subkey.UnicodeChar)
                {
                case L'1':
                {
                    while (TRUE)
                    {
                        UINTN Columns;
                        UINTN Rows;
                        int x, y;
                        UINTN ProcNum = 0;
                        CHAR16 Buffer[128];

                        gST->ConOut->ClearScreen(gST->ConOut);

                        Print(L"%s", GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_AVAIL_CPUS), TempBuf, sizeof(TempBuf)));
                        for (i = 0; i < (int)MPSupportGetMaxProcessors(); i++)
                        {
                            if (MPSupportIsProcEnabled(i))
                            {
                                Print(L"%d ", i);
                            }
                        }

                        UnicodeSPrint(Buffer, 128, L"Enter the CPU# (0-%2d), or (Esc) to go back:   ", (int)MPSupportGetMaxProcessors() - 1);

                        MtCreatePopUp(gST->ConOut->Mode->Attribute, NULL,
                                      Buffer,
                                      NULL);

                        //
                        // Retrieve the number of columns and rows in the current console mode
                        //
                        gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &Columns, &Rows);
                        y = (int)(Rows - 4) / 2 + 1;
                        x = (int)(Columns - 49) / 2 + 46;
                        if (getval(x, y, 2, 1, &ProcNum) == 0)
                        {
                            if (ProcNum < MPSupportGetMaxProcessors() && MPSupportIsProcEnabled((UINTN)ProcNum)) // Is CPU valid?
                            {
                                gCPUSelMode = CPU_SINGLE;
                                gSelCPUNo = ProcNum;
                                break;
                            }
                            else
                            {
                                EFI_INPUT_KEY tempkey;
                                gST->ConOut->ClearScreen(gST->ConOut);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                              GetStringById(STRING_TOKEN(STR_ERROR_INVALID_CPUS), TempBuf, sizeof(TempBuf)),
                                              NULL);
                            }
                        }
                        else // Esc
                            break;
                    }
                }
                break;

                case L'2':
                case L'3':
                case L'4':
                {
                    if (!gMPSupported) // If multiprocessing not supported, display error message
                    {
                        EFI_INPUT_KEY tempkey;
                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                      GetStringById(STRING_TOKEN(STR_ERROR_MULTI_CPU), TempBuf, sizeof(TempBuf)),
                                      NULL);
                    }
                    else
                    {
                        switch (subkey.UnicodeChar)
                        {
                        case L'2':
                            gCPUSelMode = CPU_PARALLEL;
                            break;
                        case L'3':
                            gCPUSelMode = CPU_RROBIN;
                            break;
                        case L'4':
                            gCPUSelMode = CPU_SEQUENTIAL;
                            break;
                        default:
                            break;
                        }
                    }
                }
                break;

                case L'0':
                    break;
                }
            }
        }
        break;

        case L's':
            return ID_BUTTON_START;

        case 'r':
        {
            EFI_INPUT_KEY subkey;
            subkey.UnicodeChar = 0;

            SetDataSize(8);
            SetTestMode(MEM_STEP);
            SetReadWrite(MEM_READ);

            while (subkey.UnicodeChar != L'0')
            {
                CHAR16 TempBuf[128];
                CHAR16 TempBuf2[128];

                gST->ConOut->ClearScreen(gST->ConOut);

                // Display all available processors that can be used for testing
                GetStringById(GetReadWrite() == MEM_WRITE ? STRING_TOKEN(STR_MENU_BENCH_WRITE) : STRING_TOKEN(STR_MENU_BENCH_READ), TempBuf2, sizeof(TempBuf2));
                Print(L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_BENCH_BLOCK_RW), TempBuf, sizeof(TempBuf)), TempBuf2);
                GetStringById(GetTestMode() == MEM_STEP ? STRING_TOKEN(STR_MENU_BENCH_TEST_MODE_STEP) : STRING_TOKEN(STR_MENU_BENCH_TEST_MODE_BLOCK), TempBuf2, sizeof(TempBuf2));
                Print(L"%s: %s\n", GetStringById(STRING_TOKEN(STR_MENU_BENCH_TEST_MODE), TempBuf, sizeof(TempBuf)), TempBuf2);
                if (GetTestMode() == MEM_BLOCK)
                    Print(L"%s: %d-bit\n", GetStringById(STRING_TOKEN(STR_MENU_BENCH_ACCESS_WIDTH), TempBuf, sizeof(TempBuf)), GetDataSize());

                Print(L"\nLast test result:\n");
                if (GetAveSpeed() > 0)
                    Print(L" Ave. Bandwidth: %ld.%03ld MB/s\n", DivU64x32(GetAveSpeed(), 1024), ModU64x32(DivU64x32(MultU64x32(GetAveSpeed(), 1000), 1024), 1000));
                else
                    Print(L" Ave. Bandwidth: N/A\n");

                MtCreatePopUp(gST->ConOut->Mode->Attribute, &subkey,
                              L"RAM Benchmark",
                              L"",
                              L"(1) Toggle read/write                        ",
                              L"(2) Toggle test mode (step size/block size)  ",
                              L"(3) Change data width                        ",
                              L"(4) Start benchmark tests                    ",
                              L"(0) Go back                                  ",
                              NULL);
                switch (subkey.UnicodeChar)
                {

                case L'1':
                    SetReadWrite(GetReadWrite() == MEM_READ ? MEM_WRITE : MEM_READ);
                    break;
                case L'2':
                    SetTestMode(GetTestMode() == MEM_STEP ? MEM_BLOCK : MEM_STEP);
                    break;

                case L'3':
                {
                    if (GetTestMode() == MEM_STEP)
                    {
                        EFI_INPUT_KEY tempkey;
                        gST->ConOut->ClearScreen(gST->ConOut);
                        MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                      L" This option only applies to Block Size test mode",
                                      L" Press any key to continue.                     ",
                                      NULL);
                    }
                    else
                    {
                        if (GetDataSize() < QWORD_BITS)
                            SetDataSize(GetDataSize() * 2);
                        else
                            SetDataSize(8);
                    }
                }
                break;

                case L'4':
                {
                    int i;
                    int nNumSteps = 0;

                    gST->ConOut->ClearScreen(gST->ConOut);

                    if (MEM_STEP == GetTestMode())
                    {
                        do
                        {
                            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Running MEM_STEP test (%s)", GetReadWrite() == MEM_READ ? L"Read" : L"Write");
                            AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                            Print(L"%s\n", TempBuf);

                            // Initialise the the test
                            nNumSteps = NewStepTest();

                            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Number of memory steps: %d (Block size: %ld Bytes)", nNumSteps, (UINT64)GetArraySize());
                            AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                            Print(L"%s\n", TempBuf);

                            // Memory allocation failed
                            if (nNumSteps == 0)
                            {
                                EFI_INPUT_KEY tempkey;
                                gST->ConOut->ClearScreen(gST->ConOut);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                              L" Memory allocation error",
                                              NULL);
                                break;
                            }

                            // Run the main test loop
                            for (i = 0; i < nNumSteps; i++)
                            {
                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Step %d of %d: Step size %ld bytes", i + 1, nNumSteps, (UINT64)GetCurrentStepSize());
                                AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                                Print(L"%s\n", TempBuf);

                                if (!Step())
                                {
                                    EFI_INPUT_KEY tempkey;
                                    gST->ConOut->ClearScreen(gST->ConOut);
                                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &tempkey,
                                                  L" Error during testing",
                                                  NULL);
                                    SetTestState(ERR_TEST);
                                    break;
                                }

                                if (gBS->CheckEvent(gST->ConIn->WaitForKey) == EFI_SUCCESS) // Check for abort
                                {
                                    EFI_INPUT_KEY tempkey;
                                    SetMem(&tempkey, sizeof(tempkey), 0);

                                    gST->ConIn->ReadKeyStroke(gST->ConIn, &tempkey);

                                    if (tempkey.ScanCode == 0x17 /* Esc */)
                                    {
                                        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"MEM_STEP test cancelled by user");
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                                        Print(L"%s\n", TempBuf);

                                        SetTestState(USER_STOPPED);
                                        break;
                                    }
                                }
                            }

                            SetAveSpeed(DivU64x32(GetTotalKBPerSec(), nNumSteps));
                            CleanupTest();

                        } while (0);

                        FPrint(NULL, L"\n");
                        FPrint(NULL, GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)));
                        MtUiWaitForKey(0, NULL);
                    }
                    else
                    {
                        do
                        {
                            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Running MEM_BLOCK test (%s, %d-bit)", GetReadWrite() == MEM_READ ? L"Read" : L"Write", GetDataSize());
                            AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                            Print(L"%s\n", TempBuf);

                            // Find out how many steps
                            nNumSteps = NewBlockTest();

                            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Number of blocks: %d", nNumSteps);
                            AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                            Print(L"%s\n", TempBuf);

                            for (i = 0; i < nNumSteps; i++)
                            {
                                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Step %d of %d: Block Size: %ld, Data Size %ld Bits", i + 1, nNumSteps, (UINT64)GetBlockSize(), (UINT64)GetDataSize());
                                AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                                Print(L"%s\n", TempBuf);

                                if (!StepBlock())
                                {
                                    SetTestState(ERR_TEST);
                                    break;
                                }

                                if (gBS->CheckEvent(gST->ConIn->WaitForKey) == EFI_SUCCESS)
                                {
                                    EFI_INPUT_KEY tempkey;
                                    SetMem(&tempkey, sizeof(tempkey), 0);

                                    gST->ConIn->ReadKeyStroke(gST->ConIn, &tempkey);

                                    if (tempkey.ScanCode == 0x17 /* Esc */)
                                    {
                                        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"MEM_BLOCK test cancelled by user");
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "%s", TempBuf);
                                        Print(L"%s\n", TempBuf);

                                        SetTestState(USER_STOPPED);
                                        break;
                                    }
                                }
                            }
                            SetAveSpeed(DivU64x32(GetTotalKBPerSec(), nNumSteps));

                            // Did the test finish successfully?
                            if (i == nNumSteps)
                            {
                                SetTestState(COMPLETE);
                                SetResultsAvailable(TRUE);
                            }
                        } while (0);

                        FPrint(NULL, L"\n");
                        FPrint(NULL, GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)));
                        MtUiWaitForKey(0, NULL);
                    }
                }
                break;

                case '0':
                    break;
                }
            }
        }
        break;

        case L'x':
            return ID_BUTTON_EXIT;

        default:
            break;
        }
    }
    return ID_BUTTON_START;
}
#endif

// MtSupportSetDefaultConfig
//
// Set the default test configuration
VOID
    EFIAPI
    MtSupportSetDefaultConfig()
{
    gAddrLimitLo = minaddr;
    gAddrLimitHi = maxaddr;
    gMemRemMB = 16;
    gMinMemRangeMB = 16;
    gSelCPUNo = 0;
    gMaxCPUs = DEF_CPUS;
    mSelProcNum = MPSupportGetBspId();
    gNumPasses = DEFAULT_NUM_PASSES;
    gECCPoll = TRUE;
    gECCInject = FALSE;
    gTSODPoll = TRUE;
    gCacheMode = CACHEMODE_DEFAULT;
    gTestCfgFile[0] = L'\0';
    gPass1Full = FALSE;
    gAddr2ChBits = 0;
    gAddr2SlBits = 0;
    gAddr2CSBits = 0;
    gNumChipMapCfg = 0;
    gChipMapCfg = NULL;
    gNumCheckMemSpeed = 0;
    gCheckMemSpeed = NULL;
    gCurLang = 0;
    gReportNumErr = DEFAULT_REPORT_NUM_ERRORS;
    gReportNumWarn = DEFAULT_REPORT_NUM_WARNINGS;
    gAutoMode = AUTOMODE_OFF;
    gAutoReport = TRUE;
    gAutoReportFmt = AUTOREPORTFMT_HTML;
    gAutoPromptFail = FALSE;
    gTCPServerIP[0] = L'\0';
    gTCPGatewayIP[0] = L'\0';
    gTCPServerPort = 80;
    gTCPRequestLocation[0] = L'\0';
    gTCPClientIP[0] = L'\0';
    gSkipSplash = FALSE;
    gSkipDecode = SKIPDECODE_NEVER;
    gExitMode = EXITMODE_PROMPT;
    gDisableSPD = FALSE;
    gMinSPDs = 0;
    for (int i = 0; i < MAX_NUM_SPDS; i++)
        gExactSPDs[i] = -1;
    gExactSPDSize = -1;
    gCheckMemSPDSize = FALSE;
    gCheckSPDSMBIOS = FALSE;
    gSPDManuf[0] = L'\0';
    gSPDPartNo[0] = L'\0';
    gSameSPDPartNo = FALSE;
    gSPDMatch = FALSE;
    gSPDReportByteLo = 0;
    gSPDReportByteHi = -1;
    gSPDReportExtSN = FALSE;
    gBgColour = EFI_BLACK;
    gHammerRandom = TRUE;
    gHammerPattern = 0;
    gHammerMode = HAMMER_DOUBLE;
    gHammerStep = ROW_INC;
    gDisableMP = FALSE;
    gEnableHT = FALSE;
    gConsoleMode = gST->ConOut->Mode->Mode;
    gConsoleOnly = FALSE;
    gBitFadeSecs = DEF_SLEEP_SECS;
    gMaxErrCount = DEFAULT_MAXERRCOUNT;
    gTriggerOnErr = FALSE;
    gReportPrefix = REPORT_PREFIX_DEFAULT;
    gReportPrepend[0] = L'\0';
    CopyMem(&gTFTPServerIp, &ServerIP, sizeof(gTFTPServerIp));
    gTFTPStatusSecs = DEFAULT_TFTP_STATUS_SECS;
    gPMPDisable = FALSE;
    gTCPDisable = TRUE;
    gRTCSync = FALSE;
    gVerbosity = 0;
    gTPL = TPL_APPLICATION;
}

EFI_STATUS
EFIAPI
FPrint(
    IN MT_HANDLE FileHandle,
    IN CONST CHAR16 *FormatString,
    ...)
{
    VA_LIST Marker;
    UINTN NumberOfPrinted;
    CHAR16 szBuffer[VLONG_STRING_LEN];

    VA_START(Marker, FormatString);
    NumberOfPrinted = UnicodeVSPrint(szBuffer, sizeof(szBuffer), FormatString, Marker);
    VA_END(Marker);

    if (FileHandle == DEBUG_FILE_HANDLE)
    {
        if (szBuffer[StrLen(szBuffer) - 1] == L'\n')
            szBuffer[StrLen(szBuffer) - 1] = L'\0';

        return MtSupportUCDebugWriteLine(szBuffer);
    }
    else if (FileHandle)
    {
        UINTN BufSize;

        BufSize = StrLen(szBuffer) * sizeof(CHAR16);

        return MtSupportWriteFile(FileHandle, &BufSize, szBuffer);
    }
    else
    {
        return gST->ConOut->OutputString(gST->ConOut, szBuffer);
    }
}

EFI_STATUS
EFIAPI
UnicodeCatf(
    IN VOID *FileHandleOrBuffer,
    IN INTN BufferSize,
    IN CONST CHAR16 *FormatString,
    ...)
{
    VA_LIST Marker;
    UINTN NumberOfPrinted;
    CHAR16 szBuffer[VLONG_STRING_LEN];

    VA_START(Marker, FormatString);
    NumberOfPrinted = UnicodeVSPrint(szBuffer, sizeof(szBuffer), FormatString, Marker);
    VA_END(Marker);

    if (BufferSize < 0)
    {
        if (FileHandleOrBuffer == DEBUG_FILE_HANDLE)
        {
            if (szBuffer[StrLen(szBuffer) - 1] == L'\n')
                szBuffer[StrLen(szBuffer) - 1] = L'\0';

            return MtSupportUCDebugWriteLine(szBuffer);
        }
        else if (FileHandleOrBuffer)
        {
            UINTN BufSize;

            BufSize = StrLen(szBuffer) * sizeof(CHAR16);

            return MtSupportWriteFile(FileHandleOrBuffer, &BufSize, szBuffer);
        }
        else
        {
            return gST->ConOut->OutputString(gST->ConOut, szBuffer);
        }
    }
    return StrCatS(FileHandleOrBuffer, BufferSize / sizeof(szBuffer[0]), szBuffer);
}

EFI_STATUS
EFIAPI
AsciiCatf(
    IN VOID *FileHandleOrBuffer,
    IN INTN BufferSize,
    IN CONST CHAR8 *FormatString,
    ...)
{
    VA_LIST Marker;
    UINTN NumberOfPrinted;
    CHAR8 szBuffer[VLONG_STRING_LEN];

    VA_START(Marker, FormatString);
    NumberOfPrinted = AsciiVSPrint(szBuffer, sizeof(szBuffer), FormatString, Marker);
    VA_END(Marker);

    if (BufferSize < 0)
    {
        if (FileHandleOrBuffer == DEBUG_FILE_HANDLE)
        {
            if (szBuffer[AsciiStrLen(szBuffer) - 1] == '\n')
                szBuffer[AsciiStrLen(szBuffer) - 1] = '\0';

            return MtSupportDebugWriteLine(szBuffer);
        }
        else if (FileHandleOrBuffer)
        {
            UINTN BufSize;

            BufSize = AsciiStrLen(szBuffer);

            return MtSupportWriteFile(FileHandleOrBuffer, &BufSize, szBuffer);
        }
        else
        {
            return AsciiPrint(szBuffer);
        }
    }
    return AsciiStrCatS(FileHandleOrBuffer, BufferSize, szBuffer);
}

EFI_STATUS
EFIAPI
AsciiFPrint(
    IN MT_HANDLE FileHandle,
    IN CONST CHAR8 *FormatString,
    ...)
{
    VA_LIST Marker;
    UINTN NumberOfPrinted;
    CHAR8 szBuffer[VLONG_STRING_LEN];

    VA_START(Marker, FormatString);
    NumberOfPrinted = AsciiVSPrint(szBuffer, sizeof(szBuffer), FormatString, Marker);
    VA_END(Marker);

    if (FileHandle == DEBUG_FILE_HANDLE)
    {
        if (szBuffer[AsciiStrLen(szBuffer) - 1] == '\n')
            szBuffer[AsciiStrLen(szBuffer) - 1] = '\0';

        return MtSupportDebugWriteLine(szBuffer);
    }
    else if (FileHandle)
    {
        UINTN BufSize;

        BufSize = AsciiStrLen(szBuffer);

        return MtSupportWriteFile(FileHandle, &BufSize, szBuffer);
    }
    else
    {
        return AsciiPrint(szBuffer);
    }
}

EFI_STATUS
EFIAPI
MtSupportDebugWriteLine(
    char *Line)
{
    static CHAR16 Filename[128];
    MT_HANDLE DebugFileHandle = NULL;
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_TIME Time;
    char TempBuf[64];
    UINTN BufSize;

    if (gDebugMode == FALSE)
        return EFI_SUCCESS;

    if (SelfDir == NULL)
        return EFI_SUCCESS;

    if (gRunFromBIOS)
        return EFI_SUCCESS;

    if (Filename[0] == L'\0')
        UnicodeSPrint(Filename, sizeof(Filename), L"%s-%s.log", DEBUG_BASE_FILENAME, MtSupportGetTimestampStr(&gBootTime));

    Status = MtSupportOpenFile(&DebugFileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    // ShellOpenFileByName(DEBUG_FILENAME, &DebugFileHandle, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

    if (DebugFileHandle == NULL)
        return Status;

    Status = MtSupportSetPosition(DebugFileHandle, (UINT64)-1);

    gRT->GetTime(&Time, NULL);

    AsciiSPrint(TempBuf, 64, "%d-%02d-%02d %02d:%02d:%02d - ", Time.Year, Time.Month, Time.Day, Time.Hour, Time.Minute, Time.Second);
    BufSize = AsciiStrLen(TempBuf);
    MtSupportWriteFile(DebugFileHandle, &BufSize, TempBuf);

    BufSize = AsciiStrLen(Line);
    MtSupportWriteFile(DebugFileHandle, &BufSize, Line);

    BufSize = (AsciiStrLen("\r\n"));
    MtSupportWriteFile(DebugFileHandle, &BufSize, "\r\n");

    MtSupportFlushFile(DebugFileHandle);

    MtSupportCloseFile(DebugFileHandle);

    DebugFileHandle = NULL;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MtSupportUCDebugWriteLine(
    CHAR16 *Line)
{
    UINTN Index;
    //
    // just buffer copy, not character copy
    //
    for (Index = 0; Index < StrLen(Line); Index++)
    {
        gBuffer[Index] = (CHAR8)Line[Index];
    }
    gBuffer[StrLen(Line)] = 0;

    return MtSupportDebugWriteLine(gBuffer);
}

EFI_STATUS
EFIAPI
MtSupportUCDebugPrint(
    IN CONST CHAR16 *FormatString,
    ...)
{
    VA_LIST Marker;
    UINTN NumberOfPrinted;
    CHAR16 szBuffer[VLONG_STRING_LEN];

    VA_START(Marker, FormatString);
    NumberOfPrinted = UnicodeVSPrint(szBuffer, sizeof(szBuffer), FormatString, Marker);
    VA_END(Marker);

    return MtSupportUCDebugWriteLine(szBuffer);
}

#if 0
VOID
EFIAPI
MtSupportFlushMPDebugLog()
{
    if (mDebugBufHead == mDebugBufTail)
        return;

    AcquireSpinLock(&mDebugLock);
    for (; mDebugBufHead != mDebugBufTail; mDebugBufHead = (mDebugBufHead + 1) % DEBUGBUF_SIZE)
    {
        MtSupportDebugWriteLine(mDebugBuf[mDebugBufHead]);
    }
    ReleaseSpinLock(&mDebugLock);
}

EFI_STATUS
EFIAPI
MtSupportMPDebugPrint(
    IN  UINTN ProcNum,
    IN  CONST CHAR8  *FormatString,
    ...
)
{
    VA_LIST Marker;
    UINTN   NumberOfPrinted;
    CHAR8 szBuffer[128];

    AcquireSpinLock(&mDebugLock);
    if (((mDebugBufTail + 1) % DEBUGBUF_SIZE) != mDebugBufHead)
    {
        VA_START(Marker, FormatString);
        NumberOfPrinted = AsciiVSPrint(szBuffer, sizeof(szBuffer), FormatString, Marker);
        VA_END(Marker);

        AsciiSPrint(mDebugBuf[mDebugBufTail], sizeof(mDebugBuf[mDebugBufTail]), "[CPU %d] - %a", ProcNum, szBuffer);
        mDebugBufTail = (mDebugBufTail + 1) % DEBUGBUF_SIZE;
    }
    else
        MtSupportDebugWriteLine("Warning - MP debug buffer overflow");
    ReleaseSpinLock(&mDebugLock);
    if (ProcNum == MPSupportGetBspId())
    {
        MtSupportFlushMPDebugLog();
    }

    return EFI_SUCCESS;
}
#endif

EFI_STATUS
MtSupportOpenFile(
    OUT MT_HANDLE *NewHandle,
    IN CHAR16 *FileName,
    IN UINT64 OpenMode,
    IN UINT64 Attributes)
{
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (NewHandle == NULL)
        return EFI_INVALID_PARAMETER;

    if ((SaveDir == NULL || ((OpenMode & EFI_FILE_MODE_WRITE) == 0) || (OpenMode & EFI_FILE_MODE_TFTP)) && PXEProtocol) // If booting from PXE
    {
        CHAR16 TftpPath[256];
        PXE_FILE_DATA PXEFile;
        UINT64 BufSize;

        SetMem(&PXEFile, sizeof(PXEFile), 0);
        SetMem(TftpPath, sizeof(TftpPath), 0);

        PXEFile.Header.FileHandleType = HANDLE_TYPE_PXE;

        // Append the path to the TFTP server, if available
        if (gTftpRoot[0])
        {
            StrCpyS(TftpPath, sizeof(TftpPath) / sizeof(TftpPath[0]), gTftpRoot);
            StrCatS(TftpPath, sizeof(TftpPath) / sizeof(TftpPath[0]), L"/");
        }
        StrCatS(TftpPath, sizeof(TftpPath) / sizeof(TftpPath[0]), FileName);

        UnicodeStrToAsciiStrS(TftpPath, PXEFile.Filename, ARRAY_SIZE(PXEFile.Filename));
        PXEFile.OpenMode = OpenMode;

        if ((OpenMode & EFI_FILE_MODE_CREATE) == 0)
        {
            // Get the file size
            Status = PXEProtocol->Mtftp(PXEProtocol, EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE, NULL, FALSE, &PXEFile.FileSize, NULL, &gTFTPServerIp, (UINT8 *)PXEFile.Filename, NULL, FALSE);

            if (Status == EFI_SUCCESS)
            {
                // Check if buffer is big enough to hold the file contents. Expand if necessary
                if (PXEFile.FileSize > PXEFile.BufferSize)
                {
                    UINTN NewSize = PXEFile.BufferSize;
                    VOID *NewBuf = NULL;
                    do
                    {
                        NewSize += 4096;
                    } while (PXEFile.FileSize > NewSize);

                    NewBuf = ReallocatePool(PXEFile.BufferSize, NewSize, PXEFile.Buffer);
                    if (NewBuf == NULL)
                        return EFI_OUT_OF_RESOURCES;
                    PXEFile.Buffer = (UINT8 *)NewBuf;
                    PXEFile.BufferSize = NewSize;
                }

                // Download the file from TFTP server and store in buffer
                if (PXEFile.FileSize > 0)
                {
                    UINTN BlockSize = 1468;
                    BufSize = PXEFile.FileSize;

                    Status = PXEProtocol->Mtftp(PXEProtocol, EFI_PXE_BASE_CODE_TFTP_READ_FILE, PXEFile.Buffer, FALSE, &BufSize, &BlockSize, &gTFTPServerIp, (UINT8 *)PXEFile.Filename, NULL, FALSE);

                    if (Status != EFI_SUCCESS)
                    {
                        if (PXEFile.Buffer)
                            FreePool(PXEFile.Buffer);
                        return Status;
                    }
                }
            }
            else // Get file size unsupported?
            {
                UINTN BlockSize = 1468;
                BufSize = 4096;
                VOID *Buf = AllocatePool((UINTN)BufSize);
                if (Buf == NULL)
                    return EFI_OUT_OF_RESOURCES;

                do
                {
                    Status = PXEProtocol->Mtftp(PXEProtocol, EFI_PXE_BASE_CODE_TFTP_READ_FILE, Buf, FALSE, &BufSize, &BlockSize, &gTFTPServerIp, (UINT8 *)PXEFile.Filename, NULL, FALSE);

                    if (Status == EFI_SUCCESS)
                    {
                        PXEFile.FileSize = BufSize;
                        PXEFile.BufferSize = (UINTN)BufSize;
                        PXEFile.Buffer = Buf;
                        break;
                    }
                    else if (Status == EFI_BUFFER_TOO_SMALL)
                    {
                        UINTN NewBufSize = (UINTN)BufSize * 2;
                        VOID *NewBuf = ReallocatePool((UINTN)BufSize, (UINTN)NewBufSize, Buf);
                        if (NewBuf == NULL)
                        {
                            FreePool(Buf);
                            return EFI_OUT_OF_RESOURCES;
                        }
                        BufSize = NewBufSize;
                        Buf = NewBuf;
                    }
                    else
                    {
                        FreePool(Buf);
                        return Status;
                    }
                } while (Status == EFI_BUFFER_TOO_SMALL);
            }
        }

        *NewHandle = AllocatePool(sizeof(PXE_FILE_DATA));
        CopyMem(*NewHandle, &PXEFile, sizeof(PXEFile));

        Status = EFI_SUCCESS;
    }
    else if (SaveDir || DataEfiDir || SelfDir)
    {
        DISK_FILE_DATA DiskFile;

        SetMem(&DiskFile, sizeof(DiskFile), 0);
        DiskFile.Header.FileHandleType = HANDLE_TYPE_DISK;
        if (SaveDir && ((OpenMode & EFI_FILE_MODE_WRITE) != 0))
            Status = SaveDir->Open(SaveDir, &DiskFile.EfiFileProtocol, FileName, OpenMode, Attributes);
        else if (DataEfiDir)
            Status = DataEfiDir->Open(DataEfiDir, &DiskFile.EfiFileProtocol, FileName, OpenMode, Attributes);
        else if (SelfDir)
            Status = SelfDir->Open(SelfDir, &DiskFile.EfiFileProtocol, FileName, OpenMode, Attributes);

        if (Status != EFI_SUCCESS)
            return Status;

        *NewHandle = AllocatePool(sizeof(DISK_FILE_DATA));
        CopyMem(*NewHandle, &DiskFile, sizeof(DiskFile));

        Status = EFI_SUCCESS;
    }

    return Status;
}

EFI_STATUS
MtSupportCloseFile(
    IN MT_HANDLE Handle)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        PXE_FILE_DATA *PXEFile = (PXE_FILE_DATA *)Handle;

        // If file is open for writing, upload to TFTP server
        if (PXEFile->OpenMode & EFI_FILE_MODE_WRITE)
        {
            UINT64 BufSize = PXEFile->FileSize;

            if (BufSize > 0)
                Status = PXEProtocol->Mtftp(PXEProtocol, EFI_PXE_BASE_CODE_TFTP_WRITE_FILE, PXEFile->Buffer, TRUE, &BufSize, NULL, &gTFTPServerIp, (UINT8 *)PXEFile->Filename, NULL, FALSE);
            else
                Status = EFI_BAD_BUFFER_SIZE;
        }
        if (PXEFile->Buffer)
            FreePool(PXEFile->Buffer);
        FreePool(Handle);
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        Status = DiskFile->EfiFileProtocol->Close(DiskFile->EfiFileProtocol);
        FreePool(Handle);
    }

    return Status;
}

// MtSupportDeleteFile
//
// Delete file
EFI_STATUS
MtSupportDeleteFile(
    IN MT_HANDLE Handle)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        return EFI_UNSUPPORTED;
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        Status = DiskFile->EfiFileProtocol->Delete(DiskFile->EfiFileProtocol);
        FreePool(Handle);
    }

    return Status;
}

EFI_STATUS
MtSupportReadFile(
    IN MT_HANDLE Handle,
    IN OUT UINTN *BufferSize,
    OUT VOID *Buffer)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        PXE_FILE_DATA *PXEFile = (PXE_FILE_DATA *)Handle;
        UINTN BytesToCopy = 0;

        // Check if file pointer is at EOF
        if (PXEFile->FilePointer >= PXEFile->FileSize)
            return EFI_DEVICE_ERROR;

        // Transfer file contents to supplied buffer and update file pointer
        BytesToCopy = PXEFile->FileSize - PXEFile->FilePointer > *BufferSize ? *BufferSize : (UINTN)(PXEFile->FileSize - PXEFile->FilePointer);
        CopyMem(Buffer, PXEFile->Buffer + PXEFile->FilePointer, BytesToCopy);
        PXEFile->FilePointer += BytesToCopy;
        *BufferSize = BytesToCopy;

        Status = EFI_SUCCESS;
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        Status = DiskFile->EfiFileProtocol->Read(DiskFile->EfiFileProtocol, BufferSize, Buffer);
    }

    return Status;
}

EFI_STATUS
MtSupportWriteFile(
    IN MT_HANDLE Handle,
    IN OUT UINTN *BufferSize,
    IN VOID *Buffer)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        PXE_FILE_DATA *PXEFile = (PXE_FILE_DATA *)Handle;

        if ((PXEFile->OpenMode & EFI_FILE_MODE_WRITE) == 0)
            return EFI_ACCESS_DENIED;

        // Check if buffer is large enough. Expand if necessary
        if (PXEFile->FilePointer + *BufferSize > PXEFile->BufferSize)
        {
            UINTN NewSize = PXEFile->BufferSize;
            VOID *NewBuf = NULL;
            do
            {
                NewSize += 4096;
            } while (PXEFile->FilePointer + *BufferSize > NewSize);

            NewBuf = ReallocatePool(PXEFile->BufferSize, NewSize, PXEFile->Buffer);
            if (NewBuf == NULL)
                return EFI_OUT_OF_RESOURCES;

            PXEFile->Buffer = (UINT8 *)NewBuf;
            PXEFile->BufferSize = NewSize;
        }

        // Copy contents to buffer and update file pointer and file size
        CopyMem(PXEFile->Buffer + PXEFile->FilePointer, Buffer, *BufferSize);
        PXEFile->FilePointer += *BufferSize;
        if (PXEFile->FilePointer > PXEFile->FileSize)
            PXEFile->FileSize = PXEFile->FilePointer;
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        Status = DiskFile->EfiFileProtocol->Write(DiskFile->EfiFileProtocol, BufferSize, Buffer);
    }

    return Status;
}

EFI_STATUS
MtSupportSetPosition(
    IN MT_HANDLE Handle,
    IN UINT64 Position)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        PXE_FILE_DATA *PXEFile = (PXE_FILE_DATA *)Handle;

        // Update file pointer
        if (Position == (UINT64)-1)
            PXEFile->FilePointer = PXEFile->FileSize;
        else
            PXEFile->FilePointer = Position;
        if (PXEFile->OpenMode & EFI_FILE_MODE_WRITE)
        {
            // Check if buffer is large enough. Expand if necessary.
            if (PXEFile->FilePointer > PXEFile->BufferSize)
            {
                UINTN NewSize = PXEFile->BufferSize;
                VOID *NewBuf = NULL;
                do
                {
                    NewSize += 4096;
                } while (PXEFile->FilePointer > NewSize);

                NewBuf = ReallocatePool(PXEFile->BufferSize, NewSize, PXEFile->Buffer);
                if (NewBuf == NULL)
                    return EFI_OUT_OF_RESOURCES;
                PXEFile->Buffer = (UINT8 *)NewBuf;
                PXEFile->BufferSize = NewSize;
                Status = EFI_SUCCESS;
            }
            // Check if we need to update the file size
            if (PXEFile->FilePointer > PXEFile->FileSize)
                PXEFile->FileSize = PXEFile->FilePointer;
        }
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        Status = DiskFile->EfiFileProtocol->SetPosition(DiskFile->EfiFileProtocol, Position);
    }

    return Status;
}

EFI_STATUS
MtSupportGetPosition(
    IN MT_HANDLE Handle,
    OUT UINT64 *Position)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        PXE_FILE_DATA *PXEFile = (PXE_FILE_DATA *)Handle;
        *Position = PXEFile->FilePointer;
        Status = EFI_SUCCESS;
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        Status = DiskFile->EfiFileProtocol->GetPosition(DiskFile->EfiFileProtocol, Position);
    }

    return Status;
}

EFI_STATUS
MtSupportGetFileSize(
    IN MT_HANDLE Handle,
    OUT UINT64 *FileSize)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        PXE_FILE_DATA *PXEFile = (PXE_FILE_DATA *)Handle;
        *FileSize = PXEFile->FileSize;
        Status = EFI_SUCCESS;
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        UINTN Size = 0;
        EFI_FILE_INFO *Info = NULL;

        Status = DiskFile->EfiFileProtocol->GetInfo(DiskFile->EfiFileProtocol, &gEfiFileInfoGuid, &Size, Info);
        if (Status == EFI_BUFFER_TOO_SMALL)
        {
            Info = (EFI_FILE_INFO *)AllocateZeroPool(Size);
            Status = DiskFile->EfiFileProtocol->GetInfo(DiskFile->EfiFileProtocol, &gEfiFileInfoGuid, &Size, Info);
            if (Status == EFI_SUCCESS)
            {
                *FileSize = Info->FileSize;
            }
            FreePool(Info);
        }
    }
    return Status;
}

EFI_STATUS
MtSupportFlushFile(
    IN MT_HANDLE Handle)
{
    FILE_HANDLE_HEADER *Header;
    EFI_STATUS Status = EFI_NO_MEDIA;

    if (Handle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)Handle;

    if (Header->FileHandleType == HANDLE_TYPE_PXE) // If booting from PXE
    {
        PXE_FILE_DATA *PXEFile = (PXE_FILE_DATA *)Handle;

        // Upload file to TFTP server
        if (PXEFile->OpenMode & EFI_FILE_MODE_WRITE)
        {
            UINT64 BufSize = PXEFile->FileSize;

            if (BufSize > 0)
                Status = PXEProtocol->Mtftp(PXEProtocol, EFI_PXE_BASE_CODE_TFTP_WRITE_FILE, PXEFile->Buffer, FALSE, &BufSize, NULL, &gTFTPServerIp, (UINT8 *)PXEFile->Filename, NULL, FALSE);
            else
                Status = EFI_BAD_BUFFER_SIZE;
        }
    }
    else if (Header->FileHandleType == HANDLE_TYPE_DISK)
    {
        DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)Handle;
        Status = DiskFile->EfiFileProtocol->Flush(DiskFile->EfiFileProtocol);
    }

    return Status;
}

/**
  Function to read a single line (up to but not including the \n) from a SHELL_FILE_HANDLE.

  If the position upon start is 0, then the Ascii Boolean will be set.  This should be
  maintained and not changed for all operations with the same file.

  @param[in]       Handle        SHELL_FILE_HANDLE to read from.
  @param[in, out]  Buffer        The pointer to buffer to read into.
  @param[in, out]  Size          The pointer to number of bytes in Buffer.
  @param[in]       Truncate      If the buffer is large enough, this has no effect.
                                 If the buffer is is too small and Truncate is TRUE,
                                 the line will be truncated.
                                 If the buffer is is too small and Truncate is FALSE,
                                 then no read will occur.

  @param[in, out]  Ascii         Boolean value for indicating whether the file is
                                 Ascii (TRUE) or UCS2 (FALSE).

  @retval EFI_SUCCESS           The operation was successful.  The line is stored in
                                Buffer.
  @retval EFI_INVALID_PARAMETER Handle was NULL.
  @retval EFI_INVALID_PARAMETER Size was NULL.
  @retval EFI_BUFFER_TOO_SMALL  Size was not large enough to store the line.
                                Size was updated to the minimum space required.
**/
EFI_STATUS
EFIAPI
MtSupportReadLine(
    IN MT_HANDLE Handle,
    IN OUT CHAR16 *Buffer,
    IN OUT UINTN *Size,
    IN BOOLEAN Truncate,
    IN OUT BOOLEAN *Ascii)
{
    EFI_STATUS Status;
    CHAR16 CharBuffer;
    UINTN CharSize;
    UINTN CountSoFar;
    UINT64 OriginalFilePosition;

    if (Handle == NULL || Size == NULL)
    {
        return (EFI_INVALID_PARAMETER);
    }
    if (Buffer == NULL)
    {
        ASSERT(*Size == 0);
    }
    else
    {
        *Buffer = CHAR_NULL;
    }

    MtSupportGetPosition(Handle, &OriginalFilePosition);
    if (OriginalFilePosition == 0)
    {
        CharSize = sizeof(CHAR16);
        Status = MtSupportReadFile(Handle, &CharSize, &CharBuffer);

        if (CharBuffer == EFI_UNICODE_BYTE_ORDER_MARK)
        {
            *Ascii = FALSE;
        }
        else
        {
            *Ascii = TRUE;
            MtSupportSetPosition(Handle, OriginalFilePosition);
        }
    }

    for (CountSoFar = 0;; CountSoFar++)
    {
        CharBuffer = 0;
        if (*Ascii)
        {
            CharSize = sizeof(CHAR8);
        }
        else
        {
            CharSize = sizeof(CHAR16);
        }
        Status = MtSupportReadFile(Handle, &CharSize, &CharBuffer);
        if (EFI_ERROR(Status) || CharSize == 0 || (CharBuffer == L'\n' && !(*Ascii)) || (CharBuffer == '\n' && *Ascii))
        {
            break;
        }
        //
        // if we have space save it...
        //
        if ((CountSoFar + 1) * sizeof(CHAR16) < *Size)
        {
            ((CHAR16 *)Buffer)[CountSoFar] = CharBuffer;
            ((CHAR16 *)Buffer)[CountSoFar + 1] = CHAR_NULL;
        }
    }

    //
    // if we ran out of space tell when...
    //
    if ((CountSoFar + 1) * sizeof(CHAR16) > *Size)
    {
        *Size = (CountSoFar + 1) * sizeof(CHAR16);
        if (!Truncate)
        {
            MtSupportSetPosition(Handle, OriginalFilePosition);
        }
        else
        {
            // DEBUG((DEBUG_WARN, "The line was truncated in ShellFileHandleReadLine"));
        }
        return (EFI_BUFFER_TOO_SMALL);
    }
    while (Buffer[StrLen(Buffer) - 1] == L'\r')
    {
        Buffer[StrLen(Buffer) - 1] = CHAR_NULL;
    }

    return EFI_SUCCESS;
}

/**
  Function to determine if a SHELL_FILE_HANDLE is at the end of the file.

  This will NOT work on directories.

  If Handle is NULL, then ASSERT.

  @param[in] Handle     the file handle

  @retval TRUE          the position is at the end of the file
  @retval FALSE         the position is not at the end of the file
**/
BOOLEAN
EFIAPI
MtSupportEof(
    MT_HANDLE Handle)
{
    EFI_STATUS Status;
    UINT64 FileSize = 0;
    UINT64 Pos;
    BOOLEAN RetVal;

    if (Handle == NULL)
        return TRUE;

    MtSupportGetPosition(Handle, &Pos);

    Status = MtSupportGetFileSize(Handle, &FileSize);

    MtSupportSetPosition(Handle, Pos);

    if (EFI_ERROR(Status))
    {
        return TRUE;
    }

    if (Pos == FileSize)
    {
        RetVal = TRUE;
    }
    else
    {
        RetVal = FALSE;
    }

    return (RetVal);
}

EFI_STATUS
EFIAPI
MtSupportEfiFileHandleToMtHandle(
    IN EFI_FILE_HANDLE EfiHandle,
    OUT MT_HANDLE *MtHandle)
{
    EFI_STATUS Status = EFI_NO_MEDIA;
    DISK_FILE_DATA DiskFile;

    if (EfiHandle == NULL || MtHandle == NULL)
        return EFI_INVALID_PARAMETER;

    SetMem(&DiskFile, sizeof(DiskFile), 0);
    DiskFile.Header.FileHandleType = HANDLE_TYPE_DISK;
    DiskFile.EfiFileProtocol = EfiHandle;

    *MtHandle = AllocatePool(sizeof(DISK_FILE_DATA));
    CopyMem(*MtHandle, &DiskFile, sizeof(DiskFile));

    Status = EFI_SUCCESS;

    return Status;
}

EFI_STATUS
EFIAPI
MtSupportGetEfiFileHandle(
    IN MT_HANDLE MtHandle,
    IN EFI_FILE_HANDLE *EfiHandle)
{
    FILE_HANDLE_HEADER *Header;

    if (MtHandle == NULL)
        return EFI_INVALID_PARAMETER;

    if (EfiHandle == NULL)
        return EFI_INVALID_PARAMETER;

    Header = (FILE_HANDLE_HEADER *)MtHandle;
    if (Header->FileHandleType != HANDLE_TYPE_DISK)
        return EFI_INVALID_PARAMETER;

    DISK_FILE_DATA *DiskFile = (DISK_FILE_DATA *)MtHandle;
    *EfiHandle = DiskFile->EfiFileProtocol;

    return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
MtSupportReadTimeOfDay(MT_HANDLE Handle, EFI_TIME *Time)
{
    EFI_STATUS Status = EFI_SUCCESS;
    CHAR16 ReadLine[64];
    CHAR16 *LinePtr = ReadLine;
    UINTN Size;
    BOOLEAN Ascii;

    UINT16 Year;
    UINT8 Month;
    UINT8 Day;
    UINT8 Hour;
    UINT8 Min;
    UINT8 Sec;

    if (Handle == NULL)
    {
        return FALSE;
    }

    Status = EFI_SUCCESS;
    Size = sizeof(ReadLine);

    SetMem(ReadLine, Size, 0);
    Status = MtSupportReadLine(Handle, ReadLine, &Size, TRUE, &Ascii);
    //
    // ignore too small of buffer...
    //
    if (Status == EFI_BUFFER_TOO_SMALL)
    {
        Status = EFI_SUCCESS;
    }
    if (EFI_ERROR(Status))
        return FALSE;

    Year = (UINT16)StrDecimalToUintn(LinePtr);
    if (Year < 1000 || Year > 3000)
        return FALSE;

    LinePtr = StrStr(LinePtr, L"-");
    if (LinePtr == NULL)
        return FALSE;
    LinePtr++;

    Month = (UINT8)StrDecimalToUintn(LinePtr);
    if (Month < 1 || Month > 12)
        return FALSE;

    LinePtr = StrStr(LinePtr, L"-");
    if (LinePtr == NULL)
        return FALSE;
    LinePtr++;

    Day = (UINT8)StrDecimalToUintn(LinePtr);
    if (Day < 1 || Day > 31)
        return FALSE;

    LinePtr = StrStr(LinePtr, L" ");
    if (LinePtr == NULL)
        return FALSE;
    LinePtr++;

    Hour = (UINT8)StrDecimalToUintn(LinePtr);
    if (Hour > 23)
        return FALSE;

    LinePtr = StrStr(LinePtr, L":");
    if (LinePtr == NULL)
        return FALSE;
    LinePtr++;

    Min = (UINT8)StrDecimalToUintn(LinePtr);
    if (Min > 59)
        return FALSE;

    LinePtr = StrStr(LinePtr, L":");
    if (LinePtr == NULL)
        return FALSE;
    LinePtr++;

    Sec = (UINT8)StrDecimalToUintn(LinePtr);
    if (Sec > 59)
        return FALSE;

    Time->Year = Year;
    Time->Month = Month;
    Time->Day = Day;
    Time->Hour = Hour;
    Time->Minute = Min;
    Time->Second = Sec;

    return TRUE;
}

EFI_STATUS
EFIAPI
MtSupportReadBlacklist(MT_HANDLE Handle, BLACKLIST *Blacklist)
{
    EFI_STATUS Status;
    CHAR16 *ReadLine;
    UINTN Size;
    BOOLEAN Found;
    BOOLEAN Ascii;
    const UINTN BUFSIZE = 4096;

    if (Handle == NULL)
    {
        return (EFI_INVALID_PARAMETER);
    }

    Status = EFI_SUCCESS;
    Size = BUFSIZE;
    Found = FALSE;

    ReadLine = (CHAR16 *)AllocateZeroPool(Size);
    if (ReadLine == NULL)
    {
        return (EFI_OUT_OF_RESOURCES);
    }

    for (; !MtSupportEof(Handle); Size = BUFSIZE)
    {
        CHAR16 *LinePtr = ReadLine;
        CHAR16 *BaseboardStr = NULL;
        CHAR16 *BIOSVerStr = NULL;
        CHAR16 *MatchStr = NULL;
        CHAR16 *RestrictStr = NULL;
        CHAR16 *StrPtr = NULL;
        BOOLEAN bStartQuotes, bEndQuotes;
        BLACKLISTENT BLBoard;
        int StrLen = 0;

        SetMem(ReadLine, Size, 0);
        Status = MtSupportReadLine(Handle, ReadLine, &Size, TRUE, &Ascii);
        //
        // ignore too small of buffer...
        //
        if (Status == EFI_BUFFER_TOO_SMALL)
        {
            Status = EFI_SUCCESS;
        }
        if (EFI_ERROR(Status))
        {
            break;
        }

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)LinePtr < (UINTN)ReadLine + Size) &&
               ((*LinePtr == L' ') || (*LinePtr == L'\t')))
        {
            LinePtr++;
        }

        if ((UINTN)LinePtr >= (UINTN)ReadLine + Size)
            continue;

        if (LinePtr[0] == L'#')
        {
            //
            // Skip comment lines
            //
            continue;
        }

        // Parse Baseboard string
        BaseboardStr = LinePtr;

        bStartQuotes = bEndQuotes = FALSE;
        for (StrPtr = BaseboardStr, StrLen = 0; (UINTN)StrPtr < (UINTN)ReadLine + Size; StrPtr++)
        {
            if (!bStartQuotes)
            {
                if (*StrPtr == L'\"')
                    bStartQuotes = TRUE;
            }
            else
            {
                if (*StrPtr == L'\"')
                {
                    bEndQuotes = TRUE;
                    break;
                }

                BLBoard.szBaseboardName[StrLen++] = (char)(*StrPtr);

                if (StrLen >= SMB_STRINGLEN)
                    break;
            }
        }
        if (!bEndQuotes)
            continue;

        BLBoard.szBaseboardName[StrLen] = '\0';

        StrPtr++;

        // Parse BIOS version string
        BIOSVerStr = StrStr(StrPtr, L",");

        if (BIOSVerStr == NULL)
            continue;

        BIOSVerStr++;

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)BIOSVerStr < (UINTN)ReadLine + Size) &&
               ((*BIOSVerStr == L' ') || (*BIOSVerStr == L'\t')))
        {
            BIOSVerStr++;
        }

        if (StrnCmp(BIOSVerStr, L"ALL", 3) == 0)
        {
            BLBoard.szBIOSVersion[0] = L'\0';
            StrPtr = BIOSVerStr + 3;
        }
        else
        {
            bStartQuotes = bEndQuotes = FALSE;
            for (StrPtr = BIOSVerStr, StrLen = 0; (UINTN)StrPtr < (UINTN)ReadLine + Size; StrPtr++)
            {
                if (!bStartQuotes)
                {
                    if (*StrPtr == L'\"')
                        bStartQuotes = TRUE;
                }
                else
                {
                    if (*StrPtr == L'\"')
                    {
                        bEndQuotes = TRUE;
                        break;
                    }

                    BLBoard.szBIOSVersion[StrLen++] = (char)(*StrPtr);

                    if (StrLen >= SMB_STRINGLEN)
                        break;
                }
            }
            if (!bEndQuotes)
                continue;

            BLBoard.szBIOSVersion[StrLen] = '\0';
        }

        // Parse Partial|Exact match
        MatchStr = StrStr(StrPtr, L",");

        if (MatchStr == NULL)
            continue;

        MatchStr++;

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)MatchStr < (UINTN)ReadLine + Size) &&
               ((*MatchStr == L' ') || (*MatchStr == L'\t')))
        {
            MatchStr++;
        }

        if (StrnCmp(MatchStr, L"EXACT", 5) == 0)
        {
            BLBoard.PartialMatch = FALSE;
        }
        else if (StrnCmp(MatchStr, L"PARTIAL", 7) == 0)
        {
            BLBoard.PartialMatch = TRUE;
        }
        else
            continue;

        // Parse Restrict flags string
        RestrictStr = StrStr(MatchStr + 1, L",");
        if (RestrictStr == NULL)
            continue;

        RestrictStr++;

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)RestrictStr < (UINTN)ReadLine + Size) &&
               ((*RestrictStr == L' ') || (*RestrictStr == L'\t')))
        {
            RestrictStr++;
        }

        if (StrnCmp(RestrictStr, L"RESTRICT_STARTUP", 16) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_STARTUP;
        }
        else if (StrnCmp(RestrictStr, L"RESTRICT_MP", 11) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_MP;
        }
        else if (StrnCmp(RestrictStr, L"DISABLE_MP", 10) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_MPDISABLE;
        }
        else if (StrnCmp(RestrictStr, L"DISABLE_CONCTRL", 15) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_CONCTRLDISABLE;
        }
        else if (StrnCmp(RestrictStr, L"FIXED_SCREENRES", 15) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_FIXED_SCREENRES;
        }
        else if (StrnCmp(RestrictStr, L"RESTRICT_ADDR", 13) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_RESTRICT_ADDR;
        }
        else if (StrnCmp(RestrictStr, L"TEST12_SINGLECPU", 16) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_TEST12_SINGLECPU;
        }
        else if (StrnCmp(RestrictStr, L"DISABLE_LANG", 12) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_LANGDISABLE;
        }
        else if (StrnCmp(RestrictStr, L"DISABLE_CPUINFO", 15) == 0)
        {
            BLBoard.RestrictFlags = RESTRICT_FLAG_CPUINFO;
        }
        else
            continue;

        // We have successfully parsed a blacklist entry. Add it to list

        if (Blacklist->iBlacklistLen >= Blacklist->iBlacklistSize)
        {
            int NewBlacklistSize = Blacklist->iBlacklistSize + 8;
            void *NewBlacklist = ReallocatePool(sizeof(BLACKLISTENT) * Blacklist->iBlacklistSize, sizeof(BLACKLISTENT) * NewBlacklistSize, Blacklist->pBlacklist);
            if (NewBlacklist == NULL)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "[BLACKLIST] Failed to re-allocate memory for blacklist (old size=%d, new size=%d)", Blacklist->iBlacklistSize, NewBlacklistSize);
                MtSupportDebugWriteLine(gBuffer);
                break;
            }
            Blacklist->pBlacklist = NewBlacklist;
            Blacklist->iBlacklistSize = NewBlacklistSize;
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "[BLACKLIST] Adding \"%a\" (BIOS: \"%a\", FLAGS: %x) to blacklist", BLBoard.szBaseboardName, BLBoard.szBIOSVersion, BLBoard.RestrictFlags);
        MtSupportDebugWriteLine(gBuffer);

        CopyMem(&Blacklist->pBlacklist[Blacklist->iBlacklistLen++], &BLBoard, sizeof(BLACKLISTENT));
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "[BLACKLIST] %d boards added to blacklist", Blacklist->iBlacklistLen);
    MtSupportDebugWriteLine(gBuffer);

    FreePool(ReadLine);

    return (Status);
}

// MtSupportSetDefaultTestConfig
//
// Set to default test configuration
VOID
    EFIAPI
    MtSupportSetDefaultTestConfig(MTCFG *Config, TESTCFG **pTestList, int *pNumTests)
{
    int i;
    TESTCFG *TestList = (TESTCFG *)AllocatePool(sizeof(TESTCFG) * NUMTEST);
    if (TestList == NULL)
        return;

    CopyMem(TestList, gTestList, sizeof(TESTCFG) * NUMTEST);

    for (i = 0; i < NUMTEST; i++)
    {
#ifndef TRIAL
        if (TestList[i].NameStrId == STRING_TOKEN(STR_TEST_13_NAME))
        {
            TestList[i].RunMemTest2 = Config->HammerMode == HAMMER_DOUBLE ? RunHammerTest_double_sided_hammer_fast : RunHammerTest_hammer_fast;
            TestList[i].RunMemTest3 = Config->HammerMode == HAMMER_DOUBLE ? RunHammerTest_double_sided_hammer_slow : RunHammerTest_hammer_slow;
            TestList[i].Iterations = Config->HammerStep;
            TestList[i].Pattern = Config->HammerRandom ? PAT_RAND : PAT_USER;
            TestList[i].PatternUser = Config->HammerPattern;
        }
        else if (TestList[i].NameStrId == STRING_TOKEN(STR_TEST_12_NAME))
        {
            if (gTest12SingleCPU)
                TestList[i].MaxCPUs = 1;
        }
#endif
    }

    if (*pTestList != NULL)
    {
        for (i = 0; i < gNumCustomTests; i++)
        {
            if ((*pTestList)[i].NameStr)
                FreePool((*pTestList)[i].NameStr);
        }
        FreePool(*pTestList);
    }

    *pTestList = TestList;
    *pNumTests = NUMTEST;
}

EFI_STATUS
EFIAPI
MtSupportReadTestConfig(MT_HANDLE Handle, TESTCFG **pTestList, int *pNumTests)
{
    EFI_STATUS Status;
    CHAR16 *ReadLine;
    UINTN Size;
    BOOLEAN Found;
    BOOLEAN Ascii;

    int TestListSize = 0;
    int TestListLen = 0;
    TESTCFG *TestList = NULL;
    const UINTN BUFSIZE = 4096;

    if (Handle == NULL)
    {
        return (EFI_INVALID_PARAMETER);
    }

    Status = EFI_SUCCESS;
    Size = BUFSIZE;
    Found = FALSE;

    ReadLine = (CHAR16 *)AllocateZeroPool(Size);
    if (ReadLine == NULL)
    {
        return (EFI_OUT_OF_RESOURCES);
    }

    for (; !MtSupportEof(Handle); Size = BUFSIZE)
    {
        CHAR16 *LinePtr = ReadLine;
        CHAR16 *TestStr = NULL;
        CHAR16 *PatternStr = NULL;
        CHAR16 *IterStr = NULL;
        CHAR16 *CacheStr = NULL;
        CHAR16 *NameStr = NULL;
        CHAR16 *StrPtr = NULL;
        BOOLEAN bStartQuotes, bEndQuotes;
        TESTCFG TestCfg;
        TESTTYPE TestType;
        UINT64 NumIter = 0;
        CHAR16 NameBuf[MAX_TEST_NAME + 1];
        int StrLen = 0;

        SetMem(ReadLine, Size, 0);
        Status = MtSupportReadLine(Handle, ReadLine, &Size, TRUE, &Ascii);
        //
        // ignore too small of buffer...
        //
        if (Status == EFI_BUFFER_TOO_SMALL)
        {
            Status = EFI_SUCCESS;
        }
        if (EFI_ERROR(Status))
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Error reading line: %r", Status);
            break;
        }

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)LinePtr < (UINTN)ReadLine + Size) &&
               ((*LinePtr == L' ') || (*LinePtr == L'\t')))
        {
            LinePtr++;
        }

        if ((UINTN)LinePtr >= (UINTN)ReadLine + Size)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Line too long (Size=%d)", Size);
            continue;
        }
        if (LinePtr[0] == L'#')
        {
            //
            // Skip comment lines
            //
            continue;
        }

        // Parse test type string
        TestStr = LinePtr;

        if (StrnCmp(TestStr, L"ADDRWALK1", 9) == 0)
        {
            TestType = TEST_ADDRWALK1;
        }
        else if (StrnCmp(TestStr, L"ADDROWN", 7) == 0)
        {
            TestType = TEST_ADDROWN;
        }
        else if (StrnCmp(TestStr, L"MOVINV8", 7) == 0)
        {
            TestType = TEST_MOVINV8;
        }
        else if (StrnCmp(TestStr, L"BLKMOV", 6) == 0)
        {
            TestType = TEST_BLKMOV;
        }
        else if (StrnCmp(TestStr, L"MOVINV32", 8) == 0)
        {
            TestType = TEST_MOVINV32;
        }
        else if (StrnCmp(TestStr, L"RNDNUM32", 8) == 0)
        {
            TestType = TEST_RNDNUM32;
        }
        else if (StrnCmp(TestStr, L"MOD20", 5) == 0)
        {
            TestType = TEST_MOD20;
        }
        else if (StrnCmp(TestStr, L"BITFADE", 7) == 0)
        {
            TestType = TEST_BITFADE;
        }
        else if (StrnCmp(TestStr, L"RNDNUM64", 8) == 0)
        {
            TestType = TEST_RNDNUM64;
        }
        else if (StrnCmp(TestStr, L"RNDNUM128", 9) == 0)
        {
            TestType = TEST_RNDNUM128;
        }
        else if (StrnCmp(TestStr, L"HAMMER", 6) == 0)
        {
            TestType = TEST_HAMMER;
        }
        else if (StrnCmp(TestStr, L"DMA", 3) == 0)
        {
            TestType = TEST_DMA;
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Unknown test: \"%s\"", TestStr);
            continue;
        }
        CopyMem(&TestCfg, &gTestList[TestType], sizeof(TestCfg));

        // Parse pattern string
        PatternStr = StrStr(TestStr, L",");

        if (PatternStr == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Missing pattern field");
            continue;
        }

        PatternStr++;

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)PatternStr < (UINTN)ReadLine + Size) &&
               ((*PatternStr == L' ') || (*PatternStr == L'\t')))
        {
            PatternStr++;
        }

        if (StrnCmp(PatternStr, L"PAT_RAND", 8) == 0)
        {
            TestCfg.Pattern = PAT_RAND;
        }
        else if (StrnCmp(PatternStr, L"PAT_WALK01", 10) == 0)
        {
            TestCfg.Pattern = PAT_WALK01;
        }
        else
        {
            TestCfg.Pattern = PAT_USER;

            if (PatternStr[0] == L'0' && PatternStr[1] == L'x')
                TestCfg.PatternUser = StrHexToUint64(PatternStr + 2);
            else
                TestCfg.PatternUser = StrHexToUint64(PatternStr);
        }

        // Parse iteration string
        IterStr = StrStr(PatternStr, L",");

        if (IterStr == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Missing iteration field");
            continue;
        }

        IterStr++;

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)IterStr < (UINTN)ReadLine + Size) &&
               ((*IterStr == L' ') || (*IterStr == L'\t')))
        {
            IterStr++;
        }

        if (StrnCmp(IterStr, L"ITER_DEF", 8) == 0)
        {
        }
        else
        {
            NumIter = StrDecimalToUint64(IterStr);
            if (NumIter > 0)
                TestCfg.Iterations = NumIter;
        }

        // Parse cache string
        CacheStr = StrStr(IterStr, L",");
        if (CacheStr == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Missing cache field");
            continue;
        }

        CacheStr++;

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)CacheStr < (UINTN)ReadLine + Size) &&
               ((*CacheStr == L' ') || (*CacheStr == L'\t')))
        {
            CacheStr++;
        }

        if (StrnCmp(CacheStr, L"CACHE_EN", 8) == 0)
        {
            TestCfg.CacheMode = CACHEMODE_ENABLE;
        }
        else if (StrnCmp(CacheStr, L"CACHE_DIS", 9) == 0)
        {
            TestCfg.CacheMode = CACHEMODE_DISABLE;
        }
        else if (StrnCmp(CacheStr, L"CACHE_DEF", 9) == 0)
        {
            TestCfg.CacheMode = CACHEMODE_DEFAULT;
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Unknown cache setting: \"%s\"", CacheStr);
            continue;
        }

        // Parse test name string
        NameStr = StrStr(CacheStr, L",");
        if (NameStr == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Missing name field");
            continue;
        }

        NameStr++;

        //
        // Ignore the pad spaces (space or tab)
        //
        while (((UINTN)NameStr < (UINTN)ReadLine + Size) &&
               ((*NameStr == L' ') || (*NameStr == L'\t')))
        {
            NameStr++;
        }

        bStartQuotes = bEndQuotes = FALSE;
        for (StrPtr = NameStr, StrLen = 0; (UINTN)StrPtr < (UINTN)ReadLine + Size; StrPtr++)
        {
            if (!bStartQuotes)
            {
                if (*StrPtr == L'\"')
                    bStartQuotes = TRUE;
            }
            else
            {
                if (*StrPtr == L'\"')
                {
                    bEndQuotes = TRUE;
                    break;
                }

                NameBuf[StrLen++] = *StrPtr;

                if (StrLen >= MAX_TEST_NAME)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Test name is too long - truncating");
                    bEndQuotes = TRUE;
                    break;
                }
            }
        }
        if (!bEndQuotes)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[TESTCFG] Missing end quote in name field");
            continue;
        }

        NameBuf[StrLen] = '\0';
        TestCfg.NameStr = EfiStrDuplicate(NameBuf);

        // We have successfully parsed a test entry. Add it to list

        if (TestListLen >= TestListSize)
        {
            int NewTestListSize = TestListSize + 8;
            void *NewTestList = ReallocatePool(sizeof(TESTCFG) * TestListSize, sizeof(TESTCFG) * NewTestListSize, TestList);
            if (NewTestList == NULL)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] Failed to re-allocate memory for test list (old size=%d, new size=%d)", TestListSize, NewTestListSize);
                MtSupportDebugWriteLine(gBuffer);
                break;
            }
            TestList = (TESTCFG *)NewTestList;
            TestListSize = NewTestListSize;
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] Adding \"%s\" (Type: %d, Pattern: %d (0x%016lX), iteration: %d, cache: %d) to test list", TestCfg.NameStr, TestType, TestCfg.Pattern, TestCfg.PatternUser, TestCfg.Iterations, TestCfg.CacheMode);
        MtSupportDebugWriteLine(gBuffer);

        TestCfg.Enabled = TRUE;
        if (TestCfg.NameStrId == STRING_TOKEN(STR_TEST_12_NAME))
        {
            if (!gSIMDSupported)
            {
                TestCfg.Enabled = FALSE;

                AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] Warning - SIMD instructions are not supported");
                MtSupportDebugWriteLine(gBuffer);
            }
        }
        else if (TestCfg.NameStrId == STRING_TOKEN(STR_TEST_14_NAME))
        {
            if (DMATestPart == NULL)
            {
                TestCfg.Enabled = FALSE;

                AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] Warning - DMA Test Partition is not found");
                MtSupportDebugWriteLine(gBuffer);
            }
        }
        TestCfg.TestNo = TestListLen;
        CopyMem(&TestList[TestListLen++], &TestCfg, sizeof(TESTCFG));

        if (TestListLen >= MAXTESTS)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] Max number of tests reached");
            MtSupportDebugWriteLine(gBuffer);
            break;
        }
    }

    if (TestListLen > 0)
    {
        if (*pTestList != NULL)
        {
            int i;
            for (i = 0; i < gNumCustomTests; i++)
            {
                if ((*pTestList)[i].NameStr)
                    FreePool((*pTestList)[i].NameStr);
            }
            FreePool(*pTestList);
        }
        *pTestList = (TESTCFG *)ReallocatePool(sizeof(TESTCFG) * TestListSize, sizeof(TESTCFG) * TestListLen, TestList);
        if (*pTestList == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] Failed to re-allocate memory for test list (old size=%d, new size=%d)", TestListSize, TestListLen);
            MtSupportDebugWriteLine(gBuffer);
            *pTestList = TestList;
        }

        *pNumTests = TestListLen;
        AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] %d tests added to test list", TestListLen);
        MtSupportDebugWriteLine(gBuffer);

        Status = EFI_SUCCESS;
    }
    else
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "[TESTCFG] No valid tests were found. Using default");
        MtSupportDebugWriteLine(gBuffer);

        Status = EFI_NOT_FOUND;
    }
    FreePool(ReadLine);

    return (Status);
}

EFI_STATUS
EFIAPI
MtSupportParseConfigParam(CHAR16 *Param, UINTN ParamSize, MTCFG *Config)
{
    if (StrnCmp(Param, L"TSTLIST=", 8) == 0)
    {
        CHAR16 *cp = Param + 8;
        int i, j = 0;
        BOOLEAN bValid = FALSE;

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Tests Selected: ");
        SetMem(Config->TestSelected, sizeof(Config->TestSelected), 0);
        while (((UINTN)cp < (UINTN)Param + ParamSize) && *cp && *cp >= L'0' && *cp <= L'9')
        {
            i = *cp - L'0';
            j = j * 10 + i;
            cp++;
            if (!(*cp >= L'0' && *cp <= L'9') ||
                ((UINTN)cp >= (UINTN)Param + ParamSize))
            {
                if (j < MAXTESTS)
                {
                    char TempBuf[8];

                    AsciiSPrint(TempBuf, sizeof(TempBuf), "%d ", j);
                    AsciiStrCatS(gBuffer, sizeof(gBuffer) / sizeof(gBuffer[0]), TempBuf);

                    Config->TestSelected[j] = TRUE;
                    bValid = TRUE;
                }
                if (*cp != L',' && *cp != L' ')
                    break;
                j = 0;
                cp++;
            }
        }

        if (!bValid)
        {
            MtSupportDebugWriteLine("[CONFIG] TSTLIST must include at least one test");
            SetMem(Config->TestSelected, sizeof(Config->TestSelected), TRUE);
        }
        else
        {
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"EXITMODE=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            char *EXITMODESTR = "prompt";

            Config->ExitMode = StrDecimalToUintn(Param + 9);

            if (Config->ExitMode == EXITMODE_REBOOT)
                EXITMODESTR = "reboot";
            else if (Config->ExitMode == EXITMODE_SHUTDOWN)
                EXITMODESTR = "shutdown";
            else if (Config->ExitMode == EXITMODE_EXIT)
                EXITMODESTR = "exit";

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Exit mode: %a", EXITMODESTR);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"AUTOMODE=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            Config->AutoMode = StrDecimalToUintn(Param + 9);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Auto mode: %a", Config->AutoMode ? (Config->AutoMode == AUTOMODE_ON ? "enabled" : "enabled - prompt only") : "disabled");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"AUTOREPORT=", 11) == 0)
    {
        if (StrLen(Param + 11) > 0)
        {
            Config->AutoReport = (BOOLEAN)StrDecimalToUintn(Param + 11);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Save report automatically: %a", Config->AutoReport ? "true" : "false");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"AUTOREPORTFMT=", 14) == 0)
    {
        if (StrnCmp(Param + 14, L"HTML", 4) == 0)
        {
            Config->AutoReportFmt = AUTOREPORTFMT_HTML;
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Auto report format: HTML");
        }
        else if (StrnCmp(Param + 14, L"BIN", 3) == 0)
        {
            Config->AutoReportFmt = AUTOREPORTFMT_BIN;
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Auto report format: binary");
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Invalid auto report format: %s", Param + 14);
        }

        MtSupportDebugWriteLine(gBuffer);
    }
    else if (StrnCmp(Param, L"AUTOPROMPTFAIL=", 15) == 0)
    {
        if (StrLen(Param + 15) > 0)
        {
            Config->AutoPromptFail = (BOOLEAN)StrDecimalToUintn(Param + 15);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Prompt on failure in auto mode: %a", Config->AutoPromptFail ? "true" : "false");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SKIPSPLASH=", 11) == 0)
    {
        if (StrLen(Param + 11) > 0)
        {
            Config->SkipSplash = (BOOLEAN)StrDecimalToUintn(Param + 11);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Skip splash screen: %a", Config->SkipSplash ? "true" : "false");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
#ifdef PRO_RELEASE
    else if (StrnCmp(Param, L"SKIPDECODE=", 11) == 0)
    {
        if (StrLen(Param + 11) > 0)
        {
            Config->SkipDecode = (BOOLEAN)StrDecimalToUintn(Param + 11);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Skip Decode results screen: %d", Config->SkipDecode);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TCPSERVERIP=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            StrCpyS(Config->TCPServerIP, sizeof(Config->TCPServerIP) / sizeof(Config->TCPServerIP[0]), Param + 12);
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Remote Address: %s", Config->TCPServerIP);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TCPGATEWAYIP=", 13) == 0)
    {
        if (StrLen(Param + 13) > 0)
        {
            StrCpyS(Config->TCPGatewayIP, sizeof(Config->TCPGatewayIP) / sizeof(Config->TCPGatewayIP[0]), Param + 13);
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Gateway Address: %s", Config->TCPGatewayIP);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TCPREQUESTLOCATION=", 19) == 0)
    {
        if (StrLen(Param + 19) > 0)
        {
            StrCpyS(Config->TCPRequestLocation, sizeof(Config->TCPRequestLocation) / sizeof(Config->TCPRequestLocation[0]), Param + 19);
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Request Location: %s", Config->TCPRequestLocation);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TCPSERVERPORT=", 14) == 0)
    {
        if (StrLen(Param + 14) > 0)
        {
            Config->TCPServerPort = (UINT32)StrDecimalToUintn(Param + 14);
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Remote Port: %s", Config->TCPServerPort);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TCPCLIENTIP=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            StrCpyS(Config->TCPClientIP, sizeof(Config->TCPClientIP) / sizeof(Config->TCPClientIP[0]), Param + 12);
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Client Address: %s", Config->TCPClientIP);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TESTCFGFILE=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            StrCpyS(Config->TestCfgFile, sizeof(Config->TestCfgFile) / sizeof(Config->TestCfgFile[0]), Param + 12);
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Test configuration file: %s", Config->TestCfgFile);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"NUMPASS=", 8) == 0)
    {
        if (StrLen(Param + 8) > 0)
        {
            Config->NumPasses = StrDecimalToUintn(Param + 8);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Number of passes: %d", Config->NumPasses);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"ADDRLIMLO=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            if (Param[10] == L'0' && Param[11] == L'x')
                Config->AddrLimitLo = StrHexToUint64(Param + 10);
            else
                Config->AddrLimitLo = StrDecimalToUint64(Param + 10);

            Config->AddrLimitLo &= ~0x0FFF;
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Lower address limit: 0x%p", Config->AddrLimitLo);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"ADDRLIMHI=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            if (Param[10] == L'0' && Param[11] == L'x')
                Config->AddrLimitHi = StrHexToUint64(Param + 10);
            else
                Config->AddrLimitHi = StrDecimalToUint64(Param + 10);

            Config->AddrLimitHi &= ~0x0FFF;

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Upper address limit: 0x%p", Config->AddrLimitHi);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"MEMREMMB=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            Config->MemRemMB = StrDecimalToUint64(Param + 9);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Amount of memory to leave unallocated: %ldMB", Config->MemRemMB);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"MINMEMRANGEMB=", 14) == 0)
    {
        if (StrLen(Param + 14) > 0)
        {
            Config->MinMemRangeMB = StrDecimalToUint64(Param + 14);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Minimum size of memory ranges to test: %ldMB", Config->MinMemRangeMB);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"CPUSEL=", 7) == 0)
    {
        if (StrnCmp(Param + 7, L"SINGLE", 6) == 0)
        {
            Config->CPUSelMode = CPU_SINGLE;
        }
        else if (StrnCmp(Param + 7, L"PARALLEL", 8) == 0)
        {
            Config->CPUSelMode = CPU_PARALLEL;
        }
        else if (StrnCmp(Param + 7, L"RROBIN", 6) == 0)
        {
            Config->CPUSelMode = CPU_RROBIN;
        }
        else if (StrnCmp(Param + 7, L"SEQ", 3) == 0)
        {
            Config->CPUSelMode = CPU_SEQUENTIAL;
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] CPU selection mode: %d", Config->CPUSelMode);
        MtSupportDebugWriteLine(gBuffer);
    }
    else if (StrnCmp(Param, L"CPUNUM=", 7) == 0)
    {
        if (StrLen(Param + 7) > 0)
        {
            Config->SelCPUNo = StrDecimalToUintn(Param + 7);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] CPU number: %d", Config->SelCPUNo);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"CPULIST=", 8) == 0)
    {
        CHAR16 *cp = Param + 8;
        int i, j = 0;
        BOOLEAN bValid = FALSE;

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] CPUs Selected: ");
        SetMem(Config->CPUList, sizeof(Config->CPUList), FALSE);
        while (((UINTN)cp < (UINTN)Param + ParamSize) && *cp && *cp >= L'0' && *cp <= L'9')
        {
            i = *cp - L'0';
            j = j * 10 + i;
            cp++;
            if (!(*cp >= L'0' && *cp <= L'9') ||
                ((UINTN)cp >= (UINTN)Param + ParamSize))
            {
                if (j < MAX_CPUS)
                {
                    char TempBuf[8];

                    AsciiSPrint(TempBuf, sizeof(TempBuf), "%d ", j);
                    AsciiStrCatS(gBuffer, sizeof(gBuffer) / sizeof(gBuffer[0]), TempBuf);

                    Config->CPUList[j] = TRUE;
                    bValid = TRUE;
                }
                if (*cp != L',' && *cp != L' ')
                    break;
                j = 0;
                cp++;
            }
        }

        if (!bValid)
        {
            MtSupportDebugWriteLine("[CONFIG] CPULIST must include at least one CPU");
            SetMem(Config->CPUList, sizeof(Config->CPUList), TRUE);
        }
        else
        {
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"MAXCPUS=", 8) == 0)
    {
        if (StrLen(Param + 8) > 0)
        {
            Config->MaxCPUs = StrDecimalToUintn(Param + 8);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Max # CPUs: %d", Config->MaxCPUs);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"ECCPOLL=", 8) == 0)
    {
        if (StrLen(Param + 8) > 0)
        {
            Config->ECCPoll = (BOOLEAN)StrDecimalToUintn(Param + 8);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] ECC polling: %a", Config->ECCPoll ? "enabled" : "disabled");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"ECCINJECT=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            Config->ECCInject = (BOOLEAN)StrDecimalToUintn(Param + 10);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] ECC injection: %a", Config->ECCInject ? "enabled" : "disabled");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TSODPOLL=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            Config->TSODPoll = (BOOLEAN)StrDecimalToUintn(Param + 9);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] TSOD polling: %a", Config->TSODPoll ? "enabled" : "disabled");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"MEMCACHE=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            UINTN CacheMode = StrDecimalToUintn(Param + 9);

            if (CacheMode >= CACHEMODE_MIN && CacheMode <= CACHEMODE_MAX)
            {
                Config->CacheMode = CacheMode;
                AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Memory cache mode: %d", Config->CacheMode);
                MtSupportDebugWriteLine(gBuffer);
            }
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Invalid memory cache mode: %d", CacheMode);
            }
        }
    }
    else if (StrnCmp(Param, L"PASS1FULL=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            Config->Pass1Full = (BOOLEAN)StrDecimalToUintn(Param + 10);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Pass 1 full test: %a", Config->Pass1Full ? "enabled" : "disabled");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"ADDR2CHBITS=", 12) == 0)
    {
        CHAR16 *cp = Param + 12;
        int i, j = 0;

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Ch Addr bits Selected: ");
        Config->Addr2ChBits = 0;
        while (((UINTN)cp < (UINTN)Param + ParamSize) && *cp && *cp >= L'0' && *cp <= L'9')
        {
            i = *cp - L'0';
            j = j * 10 + i;
            cp++;
            if (!(*cp >= L'0' && *cp <= L'9') ||
                ((UINTN)cp >= (UINTN)Param + ParamSize))
            {
                if (j < 64)
                {
                    char TempBuf[8];

                    AsciiSPrint(TempBuf, sizeof(TempBuf), "%d ", j);
                    AsciiStrCatS(gBuffer, sizeof(gBuffer) / sizeof(gBuffer[0]), TempBuf);

                    Config->Addr2ChBits |= LShiftU64(1, j);
                }
                if (*cp != L',' && *cp != L' ')
                    break;
                j = 0;
                cp++;
            }
        }
        MtSupportDebugWriteLine(gBuffer);
        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Ch Addr bits Mask: 0x%lx", Config->Addr2ChBits);
        MtSupportDebugWriteLine(gBuffer);
    }
    else if (StrnCmp(Param, L"ADDR2SLBITS=", 12) == 0)
    {
        CHAR16 *cp = Param + 12;
        int i, j = 0;

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Sl Addr bits Selected: ");
        Config->Addr2SlBits = 0;
        while (((UINTN)cp < (UINTN)Param + ParamSize) && *cp && *cp >= L'0' && *cp <= L'9')
        {
            i = *cp - L'0';
            j = j * 10 + i;
            cp++;
            if (!(*cp >= L'0' && *cp <= L'9') ||
                ((UINTN)cp >= (UINTN)Param + ParamSize))
            {
                if (j < 64)
                {
                    char TempBuf[8];

                    AsciiSPrint(TempBuf, sizeof(TempBuf), "%d ", j);
                    AsciiStrCatS(gBuffer, sizeof(gBuffer) / sizeof(gBuffer[0]), TempBuf);

                    Config->Addr2SlBits |= LShiftU64(1, j);
                }
                if (*cp != L',' && *cp != L' ')
                    break;
                j = 0;
                cp++;
            }
        }
        MtSupportDebugWriteLine(gBuffer);
        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Sl Addr bits Mask: 0x%lx", Config->Addr2SlBits);
        MtSupportDebugWriteLine(gBuffer);
    }
    else if (StrnCmp(Param, L"ADDR2CSBITS=", 12) == 0)
    {
        CHAR16 *cp = Param + 12;
        int i, j = 0;

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] CS Addr bits Selected: ");
        Config->Addr2CSBits = 0;
        while (((UINTN)cp < (UINTN)Param + ParamSize) && *cp && *cp >= L'0' && *cp <= L'9')
        {
            i = *cp - L'0';
            j = j * 10 + i;
            cp++;
            if (!(*cp >= L'0' && *cp <= L'9') ||
                ((UINTN)cp >= (UINTN)Param + ParamSize))
            {
                if (j < 64)
                {
                    char TempBuf[8];

                    AsciiSPrint(TempBuf, sizeof(TempBuf), "%d ", j);
                    AsciiStrCatS(gBuffer, sizeof(gBuffer) / sizeof(gBuffer[0]), TempBuf);

                    Config->Addr2CSBits |= LShiftU64(1, j);
                }
                if (*cp != L',' && *cp != L' ')
                    break;
                j = 0;
                cp++;
            }
        }
        MtSupportDebugWriteLine(gBuffer);
        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] CS Addr bits Mask: 0x%lx", Config->Addr2CSBits);
        MtSupportDebugWriteLine(gBuffer);
    }
    else if (StrnCmp(Param, L"CHIPMAP", 7) == 0 && StrStr(Param + 7, L"=") != NULL)
    {
        if (Config->NumChipMapCfg < MAX_CHIPMAPCFG)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parsing chip map configuration %s", Param);

            CHAR16 *EndPtr = StrStr(Param + 7, L"=");
            CHAR16 *CurPtr = StrStr(Param + 7, L".");
            if (CurPtr == NULL)
                CurPtr = EndPtr;
            CurPtr++;

            Config->ChipMapCfg[Config->NumChipMapCfg].ModuleType = -1;
            Config->ChipMapCfg[Config->NumChipMapCfg].FormFactor = -1;
            Config->ChipMapCfg[Config->NumChipMapCfg].Ranks = -1;
            Config->ChipMapCfg[Config->NumChipMapCfg].ChipWidth = -1;
            Config->ChipMapCfg[Config->NumChipMapCfg].DensityGB = -1;

            for (UINT8 j = 0; j < MAX_CHIPS; j++)
                AsciiSPrint(Config->ChipMapCfg[Config->NumChipMapCfg].ChipMap[j], sizeof(CHIPSTR), "U%d", j);

            int FieldIdx = 0;
            BOOLEAN Valid = TRUE;
            while (CurPtr < EndPtr)
            {
                CHAR16 Token[SHORT_STRING_LEN];
                CHAR16 *NextPtr = StrStr(CurPtr, L".");
                if (NextPtr == NULL)
                    NextPtr = EndPtr;

                int TokenLen = (int)(NextPtr - CurPtr);
                if (TokenLen <= 0)
                {
                    Valid = FALSE;
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Invalid field - zero length");
                    break;
                }

                StrnCpyS(Token, sizeof(Token) / sizeof(Token[0]), CurPtr, TokenLen);

                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parsing field \"%s\"", Token);

                switch (FieldIdx)
                {
                case 0:
                {
                    if (StrCmp(Token, L"DDR4") == 0)
                    {
                        Config->ChipMapCfg[Config->NumChipMapCfg].ModuleType = SPDINFO_MEMTYPE_DDR4;
                    }
                    else if (StrCmp(Token, L"DDR5") == 0)
                    {
                        Config->ChipMapCfg[Config->NumChipMapCfg].ModuleType = SPDINFO_MEMTYPE_DDR5;
                    }
                    else
                    {
                        Valid = FALSE;
                    }

                    if (Valid == FALSE)
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid module type \"%s\"", Token);
                }
                break;

                case 1:
                {
                    if (StrCmp(Token, L"DIMM") == 0)
                    {
                        Config->ChipMapCfg[Config->NumChipMapCfg].FormFactor = ChipMapFormFactorDimm;
                    }
                    else if (StrCmp(Token, L"SODIMM") == 0)
                    {
                        Config->ChipMapCfg[Config->NumChipMapCfg].FormFactor = ChipMapFormFactorSodimm;
                    }
                    else if (StrCmp(Token, L"CDIMM") == 0)
                    {
                        Config->ChipMapCfg[Config->NumChipMapCfg].FormFactor = ChipMapFormFactorCDimm;
                    }
                    else if (StrCmp(Token, L"CSODIMM") == 0)
                    {
                        Config->ChipMapCfg[Config->NumChipMapCfg].FormFactor = ChipMapFormFactorCSodimm;
                    }
                    else if (StrCmp(Token, L"CAMM2") == 0)
                    {
                        Config->ChipMapCfg[Config->NumChipMapCfg].FormFactor = ChipMapFormFactorCamm2;
                    }
                    else
                    {
                        Valid = FALSE;
                    }

                    if (Valid == FALSE)
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid form factor \"%s\"", Token);
                }
                break;

                case 2:
                {
                    if (TokenLen > 1 && Token[TokenLen - 1] == L'R')
                    {
                        UINTN Ranks = StrDecimalToUintn(Token);
                        if (Ranks > 0 && Ranks <= 32)
                            Config->ChipMapCfg[Config->NumChipMapCfg].Ranks = (INT8)Ranks;
                        else
                            Valid = FALSE;
                    }
                    else
                        Valid = FALSE;

                    if (Valid == FALSE)
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid rank \"%s\"", Token);
                }
                break;

                case 3:
                {
                    if (TokenLen > 1 && Token[0] == L'x')
                    {
                        UINTN ChipWidth = StrDecimalToUintn(Token + 1);
                        if (ChipWidth > 0 && ChipWidth <= 64)
                            Config->ChipMapCfg[Config->NumChipMapCfg].ChipWidth = (INT8)ChipWidth;
                        else
                            Valid = FALSE;
                    }
                    else
                        Valid = FALSE;

                    if (Valid == FALSE)
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid chip width \"%s\"", Token);
                }
                break;

                case 4:
                {
                    if (TokenLen > 2 && Token[TokenLen - 2] == L'G' && Token[TokenLen - 1] == L'B')
                    {
                        UINTN DensityGB = StrDecimalToUintn(Token);
                        if (DensityGB > 0)
                            Config->ChipMapCfg[Config->NumChipMapCfg].DensityGB = (INT16)DensityGB;
                        else
                            Valid = FALSE;
                    }
                    else
                        Valid = FALSE;

                    if (Valid == FALSE)
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid density \"%s\"", Token);
                }
                break;
                }
                if (Valid == FALSE)
                    break;

                FieldIdx++;
                CurPtr = NextPtr + 1;
            }

            if (Valid)
            {
                CurPtr = EndPtr + 1;
                EndPtr = Param + ParamSize;
                int MapInd = 0;

                while (CurPtr < EndPtr && *CurPtr != L'\0')
                {
                    CHAR16 Token[SHORT_STRING_LEN];
                    CHAR16 *NextPtr = StrStr(CurPtr, L",");
                    if (NextPtr == NULL)
                        NextPtr = CurPtr + StrLen(CurPtr);

                    int TokenLen = (int)(NextPtr - CurPtr);
                    if (TokenLen <= 0 || TokenLen > MAX_CHIP_NAME_LEN)
                    {
                        Valid = FALSE;
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Invalid chip name length (%d). Must be between 1-%d", TokenLen, MAX_CHIP_NAME_LEN);
                        break;
                    }

                    StrnCpyS(Token, sizeof(Token) / sizeof(Token[0]), CurPtr, TokenLen);

                    if (MapInd >= MAX_CHIPS)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Max chip names exceeded. Ignoring chip name \"%s\"", Token);
                        break;
                    }

                    // Valid number
                    BOOLEAN Number = TRUE;
                    for (CHAR16 *pCh = Token; *pCh != L'\0'; pCh++)
                    {
                        if (*pCh < L'0' || *pCh > L'9')
                        {
                            Number = FALSE;
                            break;
                        }
                    }

                    if (Valid)
                    {
                        UnicodeStrToAsciiStrS(Token, Config->ChipMapCfg[Config->NumChipMapCfg].ChipMap[MapInd], MAX_CHIP_NAME_LEN + 1);
                        // Special case for backwards compatibility. If non-negative integer, prepend with "U"
                        if (Number)
                        {
                            UINTN chip = StrDecimalToUintn(Token);
                            if (chip < 30)
                                AsciiSPrint(Config->ChipMapCfg[Config->NumChipMapCfg].ChipMap[MapInd], sizeof(CHIPSTR), "U%d", chip);
                        }
                    }

                    if (Valid == FALSE)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid chip name \"%s\"", Token);
                        break;
                    }

                    MapInd++;
                    CurPtr = NextPtr + 1;
                }

                if (Valid)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Chip map configuration #%d", Config->NumChipMapCfg);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           module type: %d", Config->ChipMapCfg[Config->NumChipMapCfg].ModuleType);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           form factor: %d", Config->ChipMapCfg[Config->NumChipMapCfg].FormFactor);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           ranks: %d", Config->ChipMapCfg[Config->NumChipMapCfg].Ranks);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           chip width: x%d", Config->ChipMapCfg[Config->NumChipMapCfg].ChipWidth);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           density: %dGB", Config->ChipMapCfg[Config->NumChipMapCfg].DensityGB);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           DRAM chip map: ");
                    int NumChips = MAX_CHIPS;
                    if (Config->ChipMapCfg[Config->NumChipMapCfg].Ranks > 0 && Config->ChipMapCfg[Config->NumChipMapCfg].ChipWidth > 0)
                        NumChips = Config->ChipMapCfg[Config->NumChipMapCfg].Ranks * 64 / Config->ChipMapCfg[Config->NumChipMapCfg].ChipWidth;

                    for (int i = 0; i < MAX_CHIPS; i++)
                    {
                        if (i == NumChips) // add marker for the expected # of chips
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "           ----------");
                            AsciiFPrint(DEBUG_FILE_HANDLE, "           - Unused -");
                        }
                        AsciiFPrint(DEBUG_FILE_HANDLE, "           %d -> %a", i, Config->ChipMapCfg[Config->NumChipMapCfg].ChipMap[i]);
                    }
                    Config->NumChipMapCfg++;
                }
            }
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Max chip map configuration exceeded. Ignoring.");
        }
    }
    else if (StrnCmp(Param, L"LANG=", 5) == 0)
    {
        int i = 0;
        CHAR8 Lang[16];
        SetMem(Lang, sizeof(Lang), 0);
        if (StrLen(Param + 5) < sizeof(Lang))
        {
            UnicodeStrToAsciiStrS(Param + 5, Lang, ARRAY_SIZE(Lang));
            while (mSupportedLangList[i] != NULL)
            {

                if (AsciiStrCmp(Lang, mSupportedLangList[i]) == 0)
                {
                    Config->CurLang = i;
                    break;
                }
                i++;
            }
        }

        if (mSupportedLangList[i] == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] \"%s\" is an invalid language", Param + 5);
            MtSupportDebugWriteLine(gBuffer);
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Language selection mode: %a", mSupportedLangList[Config->CurLang]);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"REPORTNUMERRS=", 14) == 0)
    {
        if (StrLen(Param + 14) > 0)
        {
            Config->ReportNumErr = StrDecimalToUintn(Param + 14);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Num Errors displayed in report: %d", Config->ReportNumErr);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"REPORTNUMWARN=", 14) == 0)
    {
        if (StrLen(Param + 14) > 0)
        {
            Config->ReportNumWarn = StrDecimalToUintn(Param + 14);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Num Warnings displayed in report: %d", Config->ReportNumWarn);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"DISABLESPD=", 11) == 0)
    {
        if (StrLen(Param + 11) > 0)
        {
            Config->DisableSPD = (BOOLEAN)StrDecimalToUintn(Param + 11);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Disable SPD collection: %a", Config->DisableSPD ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"MINSPDS=", 8) == 0)
    {
        if (StrLen(Param + 8) > 0)
        {
            Config->MinSPDs = StrDecimalToUintn(Param + 8);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Minimum # SPDs: %d", Config->MinSPDs);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"EXACTSPDS=", 10) == 0)
    {
        CHAR16 *CurPtr = Param + 10;
        CHAR16 *EndPtr = Param + ParamSize;
        BOOLEAN Valid = TRUE;
        int SPDIndex = 0;
        while (CurPtr < EndPtr)
        {
            CHAR16 Token[SHORT_STRING_LEN];
            CHAR16 *NextPtr = StrStr(CurPtr, L",");
            if (NextPtr == NULL)
                NextPtr = EndPtr;

            int TokenLen = (int)(NextPtr - CurPtr);
            if (TokenLen <= 0)
            {
                Valid = FALSE;
                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid EXACTSPDS - unexpected end");
                break;
            }

            StrnCpyS(Token, sizeof(Token) / sizeof(Token[0]), CurPtr, TokenLen);

            if (SPDIndex >= MAX_NUM_SPDS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Max EXACTSPDS exceeded. Ignoring value \"%s\"", Token);
                break;
            }

            // Valid number
            for (CHAR16 *pCh = Token; *pCh != L'\0'; pCh++)
            {
                if (*pCh < L'0' || *pCh > L'9')
                {
                    Valid = FALSE;
                    break;
                }
            }

            if (Valid)
            {
                UINTN val = StrDecimalToUintn(Token);
                if (val < 128)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] ExactSPD: %d", val);
                    Config->ExactSPDs[SPDIndex] = (INTN)val;
                }
                else
                    Valid = FALSE;
            }

            if (Valid == FALSE)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid EXACTSPDS value \"%s\"", Token);
                break;
            }

            SPDIndex++;
            CurPtr = NextPtr + 1;
        }

        if (SPDIndex == 0)
            MtSupportDebugWriteLine("[CONFIG] EXACTSPDS must include at least one value");
    }
    else if (StrnCmp(Param, L"EXACTSPDSIZE=", 13) == 0)
    {
        if (StrLen(Param + 13) > 0)
        {
            Config->ExactSPDSize = StrDecimalToUint64(Param + 13);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Exact SPD size: %d MB", Config->ExactSPDSize);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"CHECKMEMSPDSIZE=", 16) == 0)
    {
        if (StrLen(Param + 16) > 0)
        {
            Config->CheckMemSPDSize = (BOOLEAN)StrDecimalToUintn(Param + 16);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Check total memory capacity of detected SPDs: %a", Config->CheckMemSPDSize ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"CHECKMEMSPEED", 13) == 0 && StrStr(Param + 13, L"=") != NULL)
    {
        if (Config->NumCheckMemSpeed < MAX_CHECKMEMSPEED)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parsing memory speed check criteria: %s", Param);

            CHAR16 *EndPtr = StrStr(Param + 13, L"=");
            CHAR16 *CurPtr = StrStr(Param + 13, L".");
            if (CurPtr == NULL)
                CurPtr = EndPtr;
            CurPtr++;

            AsciiStrCpyS(Config->CheckMemSpeed[Config->NumCheckMemSpeed].PartNo, ARRAY_SIZE(Config->CheckMemSpeed[Config->NumCheckMemSpeed].PartNo), "*");

            BOOLEAN Valid = TRUE;
            while (CurPtr < EndPtr)
            {
                CHAR16 Token[SHORT_STRING_LEN];

                int TokenLen = (int)(EndPtr - CurPtr);
                if (TokenLen <= 0)
                {
                    Valid = FALSE;
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Invalid part number string - zero length");
                    break;
                }

                StrnCpyS(Token, sizeof(Token) / sizeof(Token[0]), CurPtr, TokenLen);

                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Memory speed check part number string \"%s\"", Token);

                UnicodeStrToAsciiStrS(Token, Config->CheckMemSpeed[Config->NumCheckMemSpeed].PartNo, MAX_PARTNO_STR + 1);
                break;
            }

            if (Valid)
            {
                CurPtr = EndPtr + 1;
                EndPtr = Param + ParamSize;

                if (CurPtr < EndPtr && *CurPtr != L'\0')
                {
                    if (StrCmp(CurPtr, L"OFF") == 0)
                    {
                        Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = MEMSPEED_OFF;
                    }
                    else if (StrCmp(CurPtr, L"MAX") == 0)
                    {
                        Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = MEMSPEED_MAX;
                    }
                    else if (StrCmp(CurPtr, L"JEDEC") == 0)
                    {
                        Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = MEMSPEED_JEDEC;
                    }
                    else if (StrCmp(CurPtr, L"XMP1") == 0)
                    {
                        Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = MEMSPEED_XMP1;
                    }
                    else if (StrCmp(CurPtr, L"XMP2") == 0)
                    {
                        Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = MEMSPEED_XMP2;
                    }
                    else if (StrCmp(CurPtr, L"EXPO1") == 0)
                    {
                        Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = MEMSPEED_EXPO1;
                    }
                    else if (StrCmp(CurPtr, L"EXPO2") == 0)
                    {
                        Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = MEMSPEED_EXPO2;
                    }
                    else
                    {

                        // Valid number
                        for (CHAR16 *pCh = CurPtr; *pCh != L'\0'; pCh++)
                        {
                            if (*pCh < L'0' || *pCh > L'9')
                            {
                                Valid = FALSE;
                                break;
                            }
                        }

                        if (Valid)
                        {
                            Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed = (int)StrDecimalToUintn(CurPtr);
                        }
                    }

                    if (Valid == FALSE)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] invalid memory speed check value \"%s\"", CurPtr);
                    }
                }

                if (Valid)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Memory speed check #%d", Config->NumCheckMemSpeed);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           part number string: %a", Config->CheckMemSpeed[Config->NumCheckMemSpeed].PartNo);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "           check value: %d", Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed);

                    if (Config->CheckMemSpeed[Config->NumCheckMemSpeed].CheckSpeed != MEMSPEED_OFF)
                        Config->NumCheckMemSpeed++;
                    else
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Memory speed check is off. Ignoring...");
                }
            }
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Max memory speed check entries exceeded. Ignoring.");
        }
    }
    else if (StrnCmp(Param, L"CHECKSPDSMBIOS=", 15) == 0)
    {
        if (StrLen(Param + 15) > 0)
        {
            Config->CheckSPDSMBIOS = (BOOLEAN)StrDecimalToUintn(Param + 15);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Check SPD data is consistent with SMBIOS: %a", Config->CheckSPDSMBIOS ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SPDMANUF=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            StrnCpyS(Config->SPDManuf, ARRAY_SIZE(Config->SPDManuf), Param + 9, SHORT_STRING_LEN - 1);
            Config->SPDManuf[SHORT_STRING_LEN - 1] = L'\0';
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] SPD JEDEC manufacture substring match: %s", Config->SPDManuf);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SPDPARTNO=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            StrnCpyS(Config->SPDPartNo, ARRAY_SIZE(Config->SPDPartNo), Param + 10, MODULE_PARTNO_LEN - 1);
            Config->SPDPartNo[MODULE_PARTNO_LEN - 1] = L'\0';
            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] SPD part number substring match: %s", Config->SPDPartNo);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SAMESPDPARTNO=", 14) == 0)
    {
        if (StrLen(Param + 14) > 0)
        {
            Config->SameSPDPartNo = (BOOLEAN)StrDecimalToUintn(Param + 14);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Match all SPD part number: %a", Config->SameSPDPartNo ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SPDMATCH=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            Config->SPDMatch = (BOOLEAN)StrDecimalToUintn(Param + 9);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Match SPD bytes with %s: %a", SPD_FILENAME, Config->SPDMatch ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SPDREPORTBYTELO=", 16) == 0)
    {
        if (StrLen(Param + 16) > 0)
        {
            Config->SPDReportByteLo = (INTN)StrDecimalToUintn(Param + 16);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] SPD report low byte: %d", Config->SPDReportByteLo);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SPDREPORTBYTEHI=", 16) == 0)
    {
        if (StrLen(Param + 16) > 0)
        {
            Config->SPDReportByteHi = (INTN)StrDecimalToUintn(Param + 16);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] SPD report high byte: %d", Config->SPDReportByteHi);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"SPDREPORTEXTSN=", 15) == 0)
    {
        if (StrLen(Param + 15) > 0)
        {
            Config->SPDReportExtSN = (BOOLEAN)StrDecimalToUintn(Param + 15);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] SPD report extended serial number: %a", Config->SPDReportExtSN ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"BGCOLOR=", 8) == 0)
    {
        int i = 0;
        for (i = 0; SUPPORTED_BGCOLORS[i] != NULL; i++)
        {
            if (StrnCmp(Param + 8, SUPPORTED_BGCOLORS[i], StrLen(SUPPORTED_BGCOLORS[i])) == 0)
            {
                Config->BgColour = SUPPORTED_BGCOLORS_MAP[i];
                break;
            }
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Bg colour: 0x%x", Config->BgColour);
        MtSupportDebugWriteLine(gBuffer);
    }
    else if (StrnCmp(Param, L"HAMMERPAT=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            if (Param[10] == L'0' && Param[11] == L'x')
                Config->HammerPattern = (UINT32)StrHexToUintn(Param + 10);
            else
                Config->HammerPattern = (UINT32)StrDecimalToUintn(Param + 10);

            Config->HammerRandom = FALSE;

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Row hammer pattern: 0x%08X", Config->HammerPattern);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"HAMMERMODE=", 11) == 0)
    {
        if (StrnCmp(Param + 11, L"SINGLE", 6) == 0)
        {
            Config->HammerMode = HAMMER_SINGLE;
        }
        else if (StrnCmp(Param + 11, L"DOUBLE", 6) == 0)
        {
            Config->HammerMode = HAMMER_DOUBLE;
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Row hammer mode: %d", Config->HammerMode);
        MtSupportDebugWriteLine(gBuffer);
    }
    else if (StrnCmp(Param, L"HAMMERSTEP=", 11) == 0)
    {
        if (StrLen(Param + 11) > 0)
        {
            if (Param[11] == L'0' && Param[12] == L'x')
                Config->HammerStep = (UINTN)StrHexToUint64(Param + 11);
            else
                Config->HammerStep = (UINTN)StrDecimalToUint64(Param + 11);

            Config->HammerStep = Config->HammerStep & ~0x3F; // Align to 64 bytes

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Row hammer step size: 0x%p", Config->HammerStep);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"DISABLEMP=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            Config->DisableMP = (BOOLEAN)StrDecimalToUintn(Param + 10);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Disable multiprocessor support: %a", Config->DisableMP ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"ENABLEHT=", 9) == 0)
    {
        if (StrLen(Param + 9) > 0)
        {
            Config->EnableHT = (BOOLEAN)StrDecimalToUintn(Param + 9);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Enable hyperthreading: %a", Config->EnableHT ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"CONSOLEMODE=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            Config->ConsoleMode = StrDecimalToUintn(Param + 12);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Console mode: %d", Config->ConsoleMode);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"CONSOLEONLY=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            Config->ConsoleOnly = (BOOLEAN)StrDecimalToUintn(Param + 12);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Console only: %a", Config->ConsoleOnly ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"BITFADESECS=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            Config->BitFadeSecs = StrDecimalToUintn(Param + 12);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Bit Fade Test sleep time: %d", Config->BitFadeSecs);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"MAXERRCOUNT=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            Config->MaxErrCount = StrDecimalToUintn(Param + 12);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Maximum error count: %d", Config->MaxErrCount);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TRIGGERONERR=", 13) == 0)
    {
        if (StrLen(Param + 13) > 0)
        {
            Config->TriggerOnErr = (BOOLEAN)StrDecimalToUintn(Param + 13);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Trigger on error: %a", Config->TriggerOnErr ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"REPORTPREFIX=", 13) == 0)
    {
        if (StrLen(Param + 13) > 0)
        {
            if (StrnCmp(Param + 13, L"BASEBOARDSN", StrLen(L"BASEBOARDSN")) == 0)
            {
                Config->ReportPrefix = REPORT_PREFIX_BASEBOARDSN;
            }
            else if (StrnCmp(Param + 13, L"SYSINFOSN", StrLen(L"SYSINFOSN")) == 0)
            {
                Config->ReportPrefix = REPORT_PREFIX_SYSINFOSN;
            }
            else if (StrnCmp(Param + 13, L"RAMSN", StrLen(L"RAMSN")) == 0)
            {
                Config->ReportPrefix = REPORT_PREFIX_RAMSN;
            }
            else if (StrnCmp(Param + 13, L"MACADDR", StrLen(L"MACADDR")) == 0)
            {
                Config->ReportPrefix = REPORT_PREFIX_MACADDR;
            }
            else if (StrnCmp(Param + 13, L"DEFAULT", StrLen(L"DEFAULT")) == 0)
            {
                Config->ReportPrefix = REPORT_PREFIX_DEFAULT;
            }
            else
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Invalid REPORTPREFIX value: %s", Param + 13);
                MtSupportDebugWriteLine(gBuffer);
            }

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Report prefix: %d", Config->ReportPrefix);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"REPORTPREPEND=", 14) == 0)
    {
        if (StrLen(Param + 14) > 0)
        {
            StrnCpyS(Config->ReportPrepend, ARRAY_SIZE(Config->ReportPrepend), Param + 14, ARRAY_SIZE(Config->ReportPrepend) - 1);
            Config->ReportPrepend[ARRAY_SIZE(Config->ReportPrepend) - 1] = L'\0';
            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Report prepend: %s", Config->ReportPrepend);
        }
    }
    else if (StrnCmp(Param, L"TFTPSERVERIP=", 13) == 0)
    {
        if (StrLen(Param + 13) > 0)
        {
            CHAR16 *cp = Param + 13;
            if (NetLibStrToIp4(cp, &Config->TFTPServerIp.v4) != EFI_SUCCESS)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] \"%s\" is an invalid IP address", cp);
                MtSupportDebugWriteLine(gBuffer);
            }
        }
    }
    else if (StrnCmp(Param, L"TFTPSTATUSSECS=", 15) == 0)
    {
        if (StrLen(Param + 15) > 0)
        {
            Config->TFTPStatusSecs = StrDecimalToUintn(Param + 15);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] TFTP status report time: %d", Config->BitFadeSecs);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"PMPDISABLE=", 11) == 0)
    {
        if (StrLen(Param + 11) > 0)
        {
            Config->PMPDisable = (BOOLEAN)StrDecimalToUintn(Param + 11);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Disable XML reporting to PXE server: %a", Config->PMPDisable ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TCPDISABLE=", 11) == 0)
    {
        if (StrLen(Param + 11) > 0)
        {
            Config->TCPDisable = (BOOLEAN)StrDecimalToUintn(Param + 11);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Disable TCP reporting to PXE server: %a", Config->TCPDisable ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"DHCPDISABLE=", 12) == 0)
    {
        if (StrLen(Param + 12) > 0)
        {
            Config->DHCPDisable = (BOOLEAN)StrDecimalToUintn(Param + 12);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Disable DHCP requests: %a", Config->TCPDisable ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"RTCSYNC=", 8) == 0)
    {
        if (StrLen(Param + 8) > 0)
        {
            Config->RTCSync = (BOOLEAN)StrDecimalToUintn(Param + 8);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Synchronize real-time clock with PXE server: %a", Config->RTCSync ? "yes" : "no");
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"VERBOSITY=", 10) == 0)
    {
        if (StrLen(Param + 10) > 0)
        {
            Config->Verbosity = (BOOLEAN)StrDecimalToUintn(Param + 10);

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Verbosity: %d", Config->Verbosity);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else if (StrnCmp(Param, L"TPL=", 4) == 0)
    {
        if (StrLen(Param + 4) > 0)
        {
            if (StrnCmp(Param + 4, L"APPLICATION", StrLen(L"APPLICATION")) == 0)
            {
                Config->TPL = TPL_APPLICATION;
            }
            else if (StrnCmp(Param + 4, L"CALLBACK", StrLen(L"CALLBACK")) == 0)
            {
                Config->TPL = TPL_CALLBACK;
            }
            else if (StrnCmp(Param + 4, L"NOTIFY", StrLen(L"NOTIFY")) == 0)
            {
                Config->TPL = TPL_NOTIFY;
            }
            else if (StrnCmp(Param + 4, L"HIGH_LEVEL", StrLen(L"HIGH_LEVEL")) == 0)
            {
                Config->TPL = TPL_HIGH_LEVEL;
            }
            else
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Invalid TPL value: %s", Param + 4);
                MtSupportDebugWriteLine(gBuffer);
            }

            AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] TPL: %d", Config->TPL);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
#endif
    else
    { // UNKNOWN
        AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Unknown line: \"%s\"", Param);
        MtSupportDebugWriteLine(gBuffer);
    }
    return EFI_SUCCESS;
}

void
    EFIAPI
    MtSupportInitConfig(MTCFG *Config)
{
    // Save the current configuration into the struct
    Config->AddrLimitLo = gAddrLimitLo;
    Config->AddrLimitHi = gAddrLimitHi;
    Config->MemRemMB = gMemRemMB;
    Config->MinMemRangeMB = gMinMemRangeMB;
    Config->CPUSelMode = gCPUSelMode;
    Config->SelCPUNo = gSelCPUNo;
    SetMem(Config->CPUList, sizeof(Config->CPUList), TRUE);
    Config->MaxCPUs = gMaxCPUs;
    Config->NumPasses = gNumPasses;
    Config->ECCPoll = gECCPoll;
    Config->ECCInject = gECCInject;
    Config->TSODPoll = gTSODPoll;
    Config->CacheMode = gCacheMode;

    SetMem(Config->TestSelected, sizeof(Config->TestSelected), FALSE);
    for (int i = 0; i < NUMTEST; i++)
        Config->TestSelected[i] = gTestList[i].Enabled;

    StrCpyS(Config->TestCfgFile, sizeof(Config->TestCfgFile) / sizeof(Config->TestCfgFile[0]), gTestCfgFile);
    Config->Pass1Full = gPass1Full;
    Config->Addr2ChBits = gAddr2ChBits;
    Config->Addr2SlBits = gAddr2SlBits;
    Config->Addr2CSBits = gAddr2CSBits;
    SetMem(Config->ChipMapCfg, sizeof(Config->ChipMapCfg), 0);
    Config->NumChipMapCfg = gNumChipMapCfg;
    Config->CurLang = gCurLang;
    Config->ReportNumErr = gReportNumErr;
    Config->ReportNumWarn = gReportNumWarn;
    Config->AutoMode = gAutoMode;
    Config->AutoReport = gAutoReport;
    Config->AutoReportFmt = gAutoReportFmt;
    Config->AutoPromptFail = gAutoPromptFail;
    CopyMem(&Config->TCPServerIP, &gTCPServerIP, sizeof(Config->TCPServerIP));
    CopyMem(&Config->TCPRequestLocation, &gTCPRequestLocation, sizeof(Config->TCPRequestLocation));
    CopyMem(&Config->TCPGatewayIP, &gTCPGatewayIP, sizeof(Config->TCPGatewayIP));
    Config->TCPServerPort = gTCPServerPort;
    CopyMem(&Config->TCPClientIP, &gTCPClientIP, sizeof(Config->TCPClientIP));
    Config->SkipSplash = gSkipSplash;
    Config->SkipDecode = gSkipDecode;
    Config->ExitMode = gExitMode;
    Config->DisableSPD = gDisableSPD;
    Config->MinSPDs = gMinSPDs;
    CopyMem(Config->ExactSPDs, gExactSPDs, sizeof(Config->ExactSPDs));
    Config->ExactSPDSize = gExactSPDSize;
    Config->CheckMemSPDSize = gCheckMemSPDSize;
    SetMem(Config->SPDManuf, sizeof(Config->SPDManuf), 0);
    SetMem(Config->SPDPartNo, sizeof(Config->SPDPartNo), 0);
    Config->SameSPDPartNo = gSameSPDPartNo;
    Config->SPDMatch = gSPDMatch;
    SetMem(Config->CheckMemSpeed, sizeof(Config->CheckMemSpeed), 0);
    Config->NumCheckMemSpeed = gNumCheckMemSpeed;
    Config->CheckSPDSMBIOS = gCheckSPDSMBIOS;
    Config->SPDReportByteLo = gSPDReportByteLo;
    Config->SPDReportByteHi = gSPDReportByteHi;
    Config->SPDReportExtSN = gSPDReportExtSN;
    Config->BgColour = gBgColour;
    Config->HammerRandom = gHammerRandom;
    Config->HammerPattern = gHammerPattern;
    Config->HammerMode = gHammerMode;
    Config->HammerStep = gHammerStep;
    Config->DisableMP = gDisableMP;
    Config->EnableHT = gEnableHT;
    Config->ConsoleMode = gConsoleMode;
    Config->ConsoleOnly = gConsoleOnly;
    Config->BitFadeSecs = gBitFadeSecs;
    Config->MaxErrCount = gMaxErrCount;
    Config->TriggerOnErr = gTriggerOnErr;
    Config->ReportPrefix = gReportPrefix;
    CopyMem(&Config->ReportPrepend, &gReportPrepend, sizeof(Config->ReportPrepend));
    CopyMem(&Config->TFTPServerIp, &gTFTPServerIp, sizeof(Config->TFTPServerIp));
    Config->TFTPStatusSecs = gTFTPStatusSecs;
    Config->PMPDisable = gPMPDisable;
    Config->TCPDisable = gTCPDisable;
    Config->DHCPDisable = gDHCPDisable;
    Config->RTCSync = gRTCSync;
    Config->Verbosity = gVerbosity;
    Config->TPL = gTPL;
}

EFI_STATUS
EFIAPI
MtSupportReadConfig(MT_HANDLE Handle, MTCFG *Config, UINTN *ConfigSize, INTN *DefaultIdx, UINTN *Timeout)
{
    EFI_STATUS Status;

    if (Handle == NULL)
    {
        return EFI_INVALID_PARAMETER;
    }

    const UINTN BUFSIZE = 4096;
    CHAR16 *ReadLine = (CHAR16 *)AllocateZeroPool(BUFSIZE);
    if (ReadLine == NULL)
    {
        return (EFI_OUT_OF_RESOURCES);
    }

    UINTN MaxCfg = *ConfigSize;
    UINTN NumCfg = 0;

    BOOL ConfigTagFound = FALSE;
    enum
    {
        NO_CONFIG,
        SINGLE_CONFIG,
        MULTI_CONFIG
    } MultipleCfg = NO_CONFIG;

    INTN CfgDefault = 1;
    while (!MtSupportEof(Handle))
    {
        SetMem(ReadLine, BUFSIZE, 0);
        BOOLEAN Ascii;
        UINTN Size = BUFSIZE;
        Status = MtSupportReadLine(Handle, ReadLine, &Size, TRUE, &Ascii);
        //
        // ignore too small of buffer...
        //
        if (Status == EFI_BUFFER_TOO_SMALL)
        {
            Status = EFI_SUCCESS;
        }
        if (EFI_ERROR(Status))
        {
            break;
        }

        //
        // Ignore the pad spaces (space or tab)
        //
        CHAR16 *LinePtr = ReadLine;
        while (((UINTN)LinePtr < (UINTN)ReadLine + Size) &&
               ((*LinePtr == L' ') || (*LinePtr == L'\t')))
        {
            LinePtr++;
        }

        if (LinePtr[0] == L'\0')
            continue;

        if ((UINTN)LinePtr >= (UINTN)ReadLine + Size)
            continue;

        if (LinePtr[0] == L'#')
        {
            //
            // Skip comment lines
            //
            continue;
        }

        if (MultipleCfg == NO_CONFIG || MultipleCfg == MULTI_CONFIG)
        {
            if (ConfigTagFound)
            {
                if (StrnCmp(LinePtr, L"</CONFIG>", 9) == 0)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] End of config \"%s\"", Config[NumCfg].Name);
                    NumCfg++;
                    ConfigTagFound = FALSE;
                }
                else
                    MtSupportParseConfigParam(LinePtr, (UINTN)ReadLine + Size - (UINTN)LinePtr, &Config[NumCfg]);
            }
            else
            {
                CHAR16 *TagStart = StrStr(LinePtr, L"<");
                CHAR16 *TagEnd = StrStr(LinePtr, L">");

                BOOLEAN IsTag = TagStart != NULL && TagEnd != NULL && TagStart < TagEnd;

                if (MultipleCfg == NO_CONFIG && IsTag)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Detected multi config file ...");
                    MultipleCfg = MULTI_CONFIG;
                }

                if (MultipleCfg == MULTI_CONFIG)
                {
                    if (IsTag == FALSE)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parse error - Tag not found: \"%s\"", LinePtr);
                        continue;
                    }

                    if (StrnCmp(LinePtr, L"<CONFIG=", 8) == 0)
                    {
                        if (NumCfg >= MaxCfg)
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parse error - Max number of configs exceeded ...");
                            break;
                        }

                        MtSupportInitConfig(&Config[NumCfg]);

                        // Parse Baseboard string
                        BOOL bStartQuotes = FALSE;
                        BOOL bEndQuotes = FALSE;
                        UINTN StrLen = 0;
                        for (CHAR16 *StrPtr = LinePtr + 8; (UINTN)StrPtr < (UINTN)ReadLine + Size; StrPtr++)
                        {
                            if (!bStartQuotes)
                            {
                                if (*StrPtr == L'\"')
                                    bStartQuotes = TRUE;
                            }
                            else
                            {
                                if (*StrPtr == L'\"')
                                {
                                    bEndQuotes = TRUE;
                                    break;
                                }

                                Config[NumCfg].Name[StrLen++] = *StrPtr;

                                if (StrLen >= MAX_CONFIG_NAME_LEN)
                                    break;
                            }
                        }
                        if (!bEndQuotes)
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parse error - Unable to find closing double quotations for config name. Ignoring ...");
                            continue;
                        }

                        Config[NumCfg].Name[StrLen] = L'\0';

                        ConfigTagFound = TRUE;
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Found new config \"%s\"", Config[NumCfg].Name);
                    }
                    else if (StrnCmp(LinePtr, L"<CFGTIMEOUT=", 12) == 0)
                    {
                        UINTN CfgTimeout = StrDecimalToUintn(LinePtr + 12);
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Found CFGTIMEOUT value (%d)", CfgTimeout);
                        if (Timeout)
                            *Timeout = CfgTimeout;
                    }
                    else if (StrnCmp(LinePtr, L"<CFGDEFAULT=", 12) == 0)
                    {
                        CHAR16 Value[32];

                        Value[0] = L'\0';
                        StrnCpyS(Value, sizeof(Value) / sizeof(Value[0]), LinePtr + 12, TagEnd - (LinePtr + 12));

                        if (StrCmp(Value, L"LAST") == 0)
                        {
                            CHAR16 Filename[128];
                            UnicodeSPrint(Filename, sizeof(Filename), L"%02x-%02x-%02x-%02x-%02x-%02x.lastcfg",
                                          ClientMAC.Addr[0], ClientMAC.Addr[1], ClientMAC.Addr[2], ClientMAC.Addr[3], ClientMAC.Addr[4], ClientMAC.Addr[5]);

                            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Attempting to open last configuration record file: %s", Filename);
                            MT_HANDLE LastCfgHandle = NULL;

                            EFI_STATUS ErrStatus = MtSupportOpenFile(&LastCfgHandle, Filename, EFI_FILE_MODE_READ, 0);
                            if (LastCfgHandle)
                            {
                                CHAR16 Buffer[32];
                                UINTN BufSize = sizeof(Buffer);

                                SetMem(Buffer, sizeof(Buffer), 0);
                                MtSupportReadLine(LastCfgHandle, Buffer, &BufSize, TRUE, &Ascii);

                                CfgDefault = StrDecimalToUintn(Buffer);
                                MtSupportCloseFile(LastCfgHandle);

                                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Successfully read: \"%s\"", Buffer);
                                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Last configuration selected: %d", CfgDefault);
                            }
                            else
                            {
                                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Unable to open file: %r", ErrStatus);
                                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Resetting default configuration selected to %d", CfgDefault);
                            }
                        }
                        else
                        {
                            CfgDefault = StrDecimalToUintn(Value);
                        }

                        if (CfgDefault > 0)
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Found CFGDEFAULT value (%d)", CfgDefault);
                        }
                        else
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parse error - CFGDEFAULT must be greater than 0");
                        }
                    }
                    else
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parse error - Unexpected tag found: \"%s\"", LinePtr);
                        continue;
                    }
                }
            }
        }

        if (MultipleCfg == NO_CONFIG || MultipleCfg == SINGLE_CONFIG)
        {
            if (MultipleCfg == NO_CONFIG)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Detected single config file ...");
                MultipleCfg = SINGLE_CONFIG;
                NumCfg = 1;
                MtSupportInitConfig(&Config[0]);
            }
            MtSupportParseConfigParam(LinePtr, (UINTN)ReadLine + Size - (UINTN)LinePtr, &Config[0]);
        }
    }
    if (ConfigTagFound)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Parse error - EOF without closing </CONFIG> tag");
    }

    if (NumCfg == 0)
    {
        MtSupportInitConfig(&Config[0]);
        NumCfg = 1;
    }

    if (CfgDefault >= 1 && CfgDefault <= (INTN)NumCfg)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Default configuration set to %d", CfgDefault);
    }
    else
    {
        CfgDefault = 1;
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Default configuration must be between 1 and %d. Resetting to %d.", NumCfg, CfgDefault);
    }
    if (DefaultIdx)
        *DefaultIdx = CfgDefault - 1;

    FreePool(ReadLine);

    *ConfigSize = NumCfg;
    return (Status);
}

EFI_STATUS
EFIAPI
MtSupportApplyConfig(MTCFG *Config)
{
    int i;
    int NumSel = 0;
    if (Config == NULL)
    {
        return (EFI_INVALID_PARAMETER);
    }

    // Apply the configuration in the struct into the global configuration variables
    for (i = 0; i < gNumCustomTests; i++)
    {
        if (gCustomTestList[i].NameStrId == STRING_TOKEN(STR_TEST_12_NAME))
        {
            if (!gSIMDSupported)
            {
                Config->TestSelected[i] = FALSE;

                AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Warning - SIMD instructions are not supported");
                MtSupportDebugWriteLine(gBuffer);
            }
        }
        else if (gCustomTestList[i].NameStrId == STRING_TOKEN(STR_TEST_14_NAME))
        {
            if (DMATestPart == NULL)
            {
                Config->TestSelected[i] = FALSE;

                AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] Warning - DMA test partition not found");
                MtSupportDebugWriteLine(gBuffer);
            }
        }
        if (Config->TestSelected[i])
            NumSel++;
    }
    if (NumSel > 0)
    {
        for (i = 0; i < gNumCustomTests; i++)
            gCustomTestList[i].Enabled = Config->TestSelected[i];
    }
    else
    {
        MtSupportDebugWriteLine("[CONFIG] TSTLIST must enable at least one test");
    }

    StrCpyS(gTestCfgFile, sizeof(gTestCfgFile) / sizeof(gTestCfgFile[0]), Config->TestCfgFile);

    if (Config->NumPasses > 0)
        gNumPasses = Config->NumPasses;
    else
        MtSupportDebugWriteLine("[CONFIG] NUMPASS must be greater than 0");

    if (Config->AddrLimitLo < gAddrLimitHi)
        gAddrLimitLo = Config->AddrLimitLo;
    else
        MtSupportDebugWriteLine("[CONFIG] ADDRLIMLO must be less than ADDRLIMHI");

    if (Config->AddrLimitHi > gAddrLimitLo && Config->AddrLimitHi <= maxaddr)
        gAddrLimitHi = Config->AddrLimitHi;
    else if (Config->AddrLimitHi <= gAddrLimitLo)
        MtSupportDebugWriteLine("[CONFIG] ADDRLIMHI must be greater than ADDRLIMLO");
    else if (Config->AddrLimitHi > maxaddr)
        MtSupportDebugWriteLine("[CONFIG] ADDRLIMHI must be less than the maximum address");

    if (!gMPSupported && Config->CPUSelMode != CPU_SINGLE)
    {
        MtSupportDebugWriteLine("[CONFIG] CPUSEL cannot be set to multi-CPU mode due to UEFI firmware limitations");
        gCPUSelMode = CPU_SINGLE;
    }
    else
        gCPUSelMode = Config->CPUSelMode;

    for (i = 0; i < (int)MPSupportGetMaxProcessors(); i++)
    {
        if ((UINTN)i != MPSupportGetBspId())
        {
            if (Config->EnableHT)
            {
                EFI_PROCESSOR_INFORMATION ProcessorInfoBuffer;
                SetMem(&ProcessorInfoBuffer, sizeof(ProcessorInfoBuffer), 0);
                MPSupportGetProcessorInfo(i, &ProcessorInfoBuffer);

                // Check if we are enabling hyperthreads
                if (MPSupportIsProcEnabled(i) == FALSE && ProcessorInfoBuffer.Location.Thread != 0)
                {
                    MPSupportEnableProc(i, TRUE);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Enabling hyperthread CPU %d", i);
                }
            }
            if (Config->CPUList[i] == FALSE)
            {
                MPSupportEnableProc(i, FALSE);
                AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Disabling CPU %d from CPULIST", i);
            }
        }
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "[CONFIG] This platform has %d logical processors of which %d are enabled.",
                (UINT32)MPSupportGetNumProcessors(), (UINT32)MPSupportGetNumEnabledProcessors());
    MtSupportDebugWriteLine(gBuffer);

    if (Config->SelCPUNo < MPSupportGetMaxProcessors() && MPSupportIsProcEnabled(Config->SelCPUNo))
        gSelCPUNo = Config->SelCPUNo;
    else
        MtSupportDebugWriteLine("[CONFIG] CPUNUM is not valid");

    if (Config->MaxCPUs > 0 && Config->MaxCPUs <= MAX_CPUS)
        gMaxCPUs = Config->MaxCPUs;
    else
        MtSupportDebugWriteLine("[CONFIG] MAXCPUS is not valid");

    if (mLangFontSupported[Config->CurLang])
        gCurLang = Config->CurLang;
    else
        MtSupportDebugWriteLine("[CONFIG] LANG is not supported due to UEFI firmware limitations");

    if (Config->ReportNumErr <= MAX_REPORT_NUM_ERRORS)
        gReportNumErr = Config->ReportNumErr;
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] REPORTNUMERRS must be no more than %d", MAX_REPORT_NUM_ERRORS);

    if (Config->ReportNumWarn <= MAX_REPORT_NUM_WARNINGS)
        gReportNumWarn = Config->ReportNumWarn;
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] REPORTNUMWARN must be no more than %d", MAX_REPORT_NUM_WARNINGS);

    if (Config->AutoMode < AUTOMODE_MAX)
        gAutoMode = Config->AutoMode;
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] AUTOMODE is not valid");

    if (Config->ExitMode < EXITMODE_MAX)
        gExitMode = Config->ExitMode;
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] EXITMODE is not valid");

    if (Config->HammerStep >= 0x40)
        gHammerStep = Config->HammerStep;
    else
        MtSupportDebugWriteLine("[CONFIG] HAMMERSTEP must be greater than or equal to 64");

    if ((INT32)Config->ConsoleMode <= gST->ConOut->Mode->MaxMode)
    {
        if (Config->ConsoleMode != gConsoleMode)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] Setting ConsoleMode to %d (MaxMode = %d)", Config->ConsoleMode, gST->ConOut->Mode->MaxMode);

            gConsoleMode = Config->ConsoleMode;

            gST->ConOut->SetMode(gST->ConOut, gConsoleMode);
            gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &ConWidth, &ConHeight);
            MaxUIWidth = ConWidth > 80 || ConWidth == 0 ? 80 : ConWidth;
        }
    }
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] CONSOLEMODE must not be greater than the max mode (%d)", gST->ConOut->Mode->MaxMode);

    if (Config->BitFadeSecs >= MIN_SLEEP_SECS && Config->BitFadeSecs <= MAX_SLEEP_SECS)
        gBitFadeSecs = Config->BitFadeSecs;
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] BITFADESECS must be at least %d and no more than %d", MIN_SLEEP_SECS, MAX_SLEEP_SECS);

    if (Config->MaxErrCount > 0 && Config->MaxErrCount <= (UINT32)(-1))
        gMaxErrCount = Config->MaxErrCount;
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] MAXERRCOUNT must be greater than 0 and no more than %d", (UINT32)(-1));

    if (Config->TFTPStatusSecs >= MIN_TFTP_STATUS_SECS && Config->TFTPStatusSecs <= MAX_TFTP_STATUS_SECS)
        gTFTPStatusSecs = Config->TFTPStatusSecs;
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] TFTPSTATUSSECS must be at least %d and no more than %d", MIN_TFTP_STATUS_SECS, MAX_TFTP_STATUS_SECS);

    if (Config->ECCPoll && !MtSupportECCEnabled())
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] ECCPOLL cannot be enabled as ECC is not available");
        gECCPoll = FALSE;
    }
    else
        gECCPoll = Config->ECCPoll;

    if (Config->SPDReportByteLo <= Config->SPDReportByteHi)
    {
        if (Config->SPDReportByteLo >= 0 && Config->SPDReportByteHi < MAX_SPD_LENGTH)
        {
            gSPDReportByteLo = Config->SPDReportByteLo;
            gSPDReportByteHi = Config->SPDReportByteHi;
        }
        else
            AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] SPDREPORTBYTELO/SPDREPORTBYTEHI must be greater than or equal to 0 and less than %d", MAX_SPD_LENGTH);
    }
    else if (Config->SPDReportByteHi >= 0)
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] SPDREPORTBYTELO must be less than or equal to SPDREPORTBYTEHI");

    gECCInject = Config->ECCInject;
    gTSODPoll = Config->TSODPoll;
    gCacheMode = Config->CacheMode;
    gPass1Full = Config->Pass1Full;
    gMemRemMB = Config->MemRemMB;
    gMinMemRangeMB = Config->MinMemRangeMB;
    gAddr2ChBits = Config->Addr2ChBits;
    gAddr2SlBits = Config->Addr2SlBits;
    gAddr2CSBits = Config->Addr2CSBits;
    gChipMapCfg = (CHIPMAPCFG *)AllocateCopyPool(sizeof(Config->ChipMapCfg[0]) * Config->NumChipMapCfg, Config->ChipMapCfg);
    if (gChipMapCfg)
        gNumChipMapCfg = Config->NumChipMapCfg;
    gAutoReport = Config->AutoReport;
    gAutoReportFmt = Config->AutoReportFmt;
    gAutoPromptFail = Config->AutoPromptFail;
    StrCpyS(gTCPServerIP, sizeof(gTCPServerIP) / sizeof(gTCPServerIP[0]), Config->TCPServerIP);
    StrCpyS(gTCPRequestLocation, sizeof(gTCPRequestLocation) / sizeof(gTCPRequestLocation[0]), Config->TCPRequestLocation);
    StrCpyS(gTCPGatewayIP, sizeof(gTCPGatewayIP) / sizeof(gTCPGatewayIP[0]), Config->TCPGatewayIP);
    gTCPServerPort = Config->TCPServerPort;
    StrCpyS(gTCPClientIP, sizeof(gTCPClientIP) / sizeof(gTCPClientIP[0]), Config->TCPClientIP);
    gSkipSplash = Config->SkipSplash;
    gSkipDecode = Config->SkipDecode;
    gDisableSPD = Config->DisableSPD;
    gMinSPDs = Config->MinSPDs;
    CopyMem(gExactSPDs, Config->ExactSPDs, sizeof(gExactSPDs));
    gExactSPDSize = Config->ExactSPDSize;
    gCheckMemSPDSize = Config->CheckMemSPDSize;
    StrCpyS(gSPDManuf, sizeof(gSPDManuf) / sizeof(gSPDManuf[0]), Config->SPDManuf);
    StrCpyS(gSPDPartNo, sizeof(gSPDPartNo) / sizeof(gSPDPartNo[0]), Config->SPDPartNo);
    gSameSPDPartNo = Config->SameSPDPartNo;
    gSPDMatch = Config->SPDMatch;
    gCheckMemSpeed = (CHECKMEMSPEED *)AllocateCopyPool(sizeof(Config->CheckMemSpeed[0]) * Config->NumCheckMemSpeed, Config->CheckMemSpeed);
    if (gCheckMemSpeed)
        gNumCheckMemSpeed = Config->NumCheckMemSpeed;
    gCheckSPDSMBIOS = Config->CheckSPDSMBIOS;
    gSPDReportExtSN = Config->SPDReportExtSN;
    gBgColour = Config->BgColour;
    gHammerRandom = Config->HammerRandom;
    gHammerPattern = Config->HammerPattern;
    gHammerMode = Config->HammerMode;
    gDisableMP = Config->DisableMP;
    gEnableHT = Config->EnableHT;
    gConsoleOnly = Config->ConsoleOnly;
    gTriggerOnErr = Config->TriggerOnErr;
    gReportPrefix = Config->ReportPrefix;
    CopyMem(&gReportPrepend, &Config->ReportPrepend, sizeof(gReportPrepend));
    CopyMem(&gTFTPServerIp, &Config->TFTPServerIp, sizeof(gTFTPServerIp));
    gPMPDisable = Config->PMPDisable;
    gTCPDisable = Config->TCPDisable;
    gDHCPDisable = Config->DHCPDisable;
    gRTCSync = Config->RTCSync;
    gVerbosity = Config->Verbosity;
    gTPL = Config->TPL;

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MtSupportSaveLastConfigSelected(UINTN LastCfg)
{
    CHAR16 Filename[128];
    UnicodeSPrint(Filename, sizeof(Filename), L"%02x-%02x-%02x-%02x-%02x-%02x.lastcfg",
                  ClientMAC.Addr[0], ClientMAC.Addr[1], ClientMAC.Addr[2], ClientMAC.Addr[3], ClientMAC.Addr[4], ClientMAC.Addr[5]);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Saving last configuration record file: %s", Filename);
    MT_HANDLE FileHandle = NULL;

    EFI_STATUS Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (FileHandle)
    {
        AsciiFPrint(FileHandle, "%d", LastCfg);
        MtSupportCloseFile(FileHandle);

        AsciiFPrint(DEBUG_FILE_HANDLE, "Wrote last configuration record file: \"%d\"", LastCfg);
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to open file for writing: %r", Status);
    }
    return Status;
}

#if 0 // not used
CHAR16* StrTok(CHAR16* str, const CHAR16* delim) {
    static CHAR16* _buffer;
    if (str != NULL) _buffer = str;
    if (_buffer[0] == L'\0') return NULL;

    CHAR16* ret = _buffer, * b;
    const CHAR16* d;

    for (b = _buffer; *b != L'\0'; b++) {
        for (d = delim; *d != L'\0'; d++) {
            if (*b == *d) {
                *b = L'\0';
                _buffer = b + 1;

                // skip the beginning delimiters
                if (b == ret) {
                    ret++;
                    continue;
                }
                return ret;
            }
        }
    }
    _buffer = b;
    return ret;
}
#endif

EFI_STATUS
EFIAPI
MtSupportParseCommandArgs(CHAR16 *CommandLineArgs, UINTN CommandLineArgsSize, MTCFG *Config)
{
    CHAR16 TempBuf[128];

    TempBuf[0] = L'\0';
    StrnCpyS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), CommandLineArgs, CommandLineArgsSize / sizeof(CommandLineArgs[0]));

    AsciiFPrint(DEBUG_FILE_HANDLE, "Parsing command line: \"%s\"", TempBuf);

    CHAR16 *CurPtr = TempBuf;
    CHAR16 *EndPtr = TempBuf + StrLen(TempBuf);

    while (CurPtr < EndPtr)
    {
        CHAR16 Token[SHORT_STRING_LEN];
        CHAR16 *NextPtr = StrStr(CurPtr, L" ");

        if (NextPtr == NULL)
            NextPtr = EndPtr;

        int TokenLen = (int)(NextPtr - CurPtr);
        if (TokenLen > 0)
        {
            StrnCpyS(Token, sizeof(Token) / sizeof(Token[0]), CurPtr, TokenLen);

            AsciiFPrint(DEBUG_FILE_HANDLE, "Parsing argument: \"%s\"", Token);

            MtSupportParseConfigParam(Token, TokenLen, Config);
        }
        CurPtr = NextPtr + 1;
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
MtSupportWriteConfig(MT_HANDLE Handle)
{
    BOOLEAN IsFirst = TRUE;
    char TempBuf[64];
    int i;

    TempBuf[0] = '\0';
    IsFirst = TRUE;
    for (i = 0; i < gNumCustomTests; i++)
    {
        if (gCustomTestList[i].Enabled)
        {
            char NumBuf[16];
            AsciiSPrint(NumBuf, sizeof(NumBuf), IsFirst ? "%d" : ",%d", i);
            AsciiStrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), NumBuf);
            IsFirst = FALSE;
        }
    }
    AsciiFPrint(Handle, "TSTLIST=%a\n", TempBuf);
    AsciiFPrint(Handle, "EXITMODE=%d\n", gExitMode);
    AsciiFPrint(Handle, "AUTOMODE=%d\n", gAutoMode);
    AsciiFPrint(Handle, "AUTOREPORT=%d\n", gAutoReport);
    AsciiFPrint(Handle, "AUTOREPORTFMT=%s\n", gAutoReportFmt == AUTOREPORTFMT_HTML ? L"HTML" : L"BIN");
    AsciiFPrint(Handle, "AUTOPROMPTFAIL=%d\n", gAutoPromptFail);
    AsciiFPrint(Handle, "SKIPSPLASH=%d\n", gSkipSplash);
    AsciiFPrint(Handle, "SKIPDECODE=%d\n", gSkipDecode);
    if (gTCPServerIP[0] != '\0')
        AsciiFPrint(Handle, "TCPSERVERIP=%s\n", gTCPServerIP);
    if (gTCPGatewayIP[0] != '\0')
        AsciiFPrint(Handle, "TCPGATEWAYIP=%s\n", gTCPGatewayIP);
    if (gTCPRequestLocation[0] != '\0')
        AsciiFPrint(Handle, "TCPREQUESTLOCATION=%s\n", gTCPRequestLocation);
    AsciiFPrint(Handle, "TCPSERVERPORT=%s\n", gTCPServerPort);
    if (gTCPClientIP[0] != '\0')
        AsciiFPrint(Handle, "TCPCLIENTIP=%s\n", gTCPClientIP);
    if (gTestCfgFile[0] != L'\0')
        AsciiFPrint(Handle, "TESTCFGFILE=%s\n", gTestCfgFile);
    AsciiFPrint(Handle, "NUMPASS=%d\n", gNumPasses);

    if (gAddrLimitLo != 0)
        AsciiFPrint(Handle, "ADDRLIMLO=0x%lx\n", gAddrLimitLo);

    if (gAddrLimitHi != maxaddr)
        AsciiFPrint(Handle, "ADDRLIMHI=0x%lx\n", gAddrLimitHi);

    AsciiFPrint(Handle, "MEMREMMB=%ld\n", gMemRemMB);
    AsciiFPrint(Handle, "MINMEMRANGEMB=%ld\n", gMinMemRangeMB);

    switch (gCPUSelMode)
    {
    case CPU_SINGLE:
        AsciiFPrint(Handle, "CPUSEL=SINGLE\n");
        break;
    case CPU_PARALLEL:
        AsciiFPrint(Handle, "CPUSEL=PARALLEL\n");
        break;
    case CPU_RROBIN:
        AsciiFPrint(Handle, "CPUSEL=RROBIN\n");
        break;
    case CPU_SEQUENTIAL:
        AsciiFPrint(Handle, "CPUSEL=SEQ\n");
        break;
    }
    AsciiFPrint(Handle, "CPUNUM=%d\n", gSelCPUNo);
    AsciiFPrint(Handle, "MAXCPUS=%d\n", gMaxCPUs);
    AsciiFPrint(Handle, "ECCPOLL=%d\n", gECCPoll);
    AsciiFPrint(Handle, "ECCINJECT=%d\n", gECCInject);
    AsciiFPrint(Handle, "TSODPOLL=%d\n", gTSODPoll);
    AsciiFPrint(Handle, "MEMCACHE=%d\n", gCacheMode);
    AsciiFPrint(Handle, "PASS1FULL=%d\n", gPass1Full);
    if (gAddr2ChBits != 0)
    {
        IsFirst = TRUE;
        TempBuf[0] = '\0';

        for (i = 0; i < 64; i++)
        {
            if ((RShiftU64(gAddr2ChBits, i) & 0x1ULL) != 0)
            {
                char NumBuf[16];
                AsciiSPrint(NumBuf, sizeof(NumBuf), IsFirst ? "%d" : ",%d", i);
                AsciiStrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), NumBuf);
                IsFirst = FALSE;
            }
        }
        AsciiFPrint(Handle, "ADDR2CHBITS=%a\n", TempBuf);
    }
    if (gAddr2SlBits != 0)
    {
        IsFirst = TRUE;
        TempBuf[0] = '\0';

        for (i = 0; i < 64; i++)
        {
            if ((RShiftU64(gAddr2SlBits, i) & 0x1ULL) != 0)
            {
                char NumBuf[16];
                AsciiSPrint(NumBuf, sizeof(NumBuf), IsFirst ? "%d" : ",%d", i);
                AsciiStrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), NumBuf);
                IsFirst = FALSE;
            }
        }
        AsciiFPrint(Handle, "ADDR2SLBITS=%a\n", TempBuf);
    }
    if (gAddr2CSBits != 0)
    {
        IsFirst = TRUE;
        TempBuf[0] = '\0';

        for (i = 0; i < 64; i++)
        {
            if ((RShiftU64(gAddr2CSBits, i) & 0x1ULL) != 0)
            {
                char NumBuf[16];
                AsciiSPrint(NumBuf, sizeof(NumBuf), IsFirst ? "%d" : ",%d", i);
                AsciiStrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), NumBuf);
                IsFirst = FALSE;
            }
        }
        AsciiFPrint(Handle, "ADDR2CSBITS=%a\n", TempBuf);
    }
    for (i = 0; i < gNumChipMapCfg && gChipMapCfg != NULL; i++)
    {
        AsciiFPrint(Handle, "CHIPMAP");
        if (gChipMapCfg[i].ModuleType == SPDINFO_MEMTYPE_DDR4)
        {
            AsciiFPrint(Handle, ".DDR4");
        }
        else if (gChipMapCfg[i].ModuleType == SPDINFO_MEMTYPE_DDR5)
        {
            AsciiFPrint(Handle, ".DDR5");
        }

        if (gChipMapCfg[i].FormFactor == ChipMapFormFactorDimm)
        {
            AsciiFPrint(Handle, ".DIMM");
        }
        else if (gChipMapCfg[i].FormFactor == ChipMapFormFactorSodimm)
        {
            AsciiFPrint(Handle, ".SODIMM");
        }
        else if (gChipMapCfg[i].FormFactor == ChipMapFormFactorCDimm)
        {
            AsciiFPrint(Handle, ".CDIMM");
        }
        else if (gChipMapCfg[i].FormFactor == ChipMapFormFactorCSodimm)
        {
            AsciiFPrint(Handle, ".CSODIMM");
        }
        else if (gChipMapCfg[i].FormFactor == ChipMapFormFactorCamm2)
        {
            AsciiFPrint(Handle, ".CAMM2");
        }

        if (gChipMapCfg[i].Ranks > 0 && gChipMapCfg[i].Ranks <= 32)
        {
            AsciiFPrint(Handle, ".%dR", gChipMapCfg[i].Ranks);
        }

        if (gChipMapCfg[i].ChipWidth > 0 && gChipMapCfg[i].ChipWidth <= 64)
        {
            AsciiFPrint(Handle, ".x%d", gChipMapCfg[i].ChipWidth);
        }

        if (gChipMapCfg[i].DensityGB > 0)
        {
            AsciiFPrint(Handle, ".%dGB", gChipMapCfg[i].DensityGB);
        }

        AsciiFPrint(Handle, "=");
        for (int chip = 0; chip < ARRAY_SIZE(gChipMapCfg[i].ChipMap); chip++)
        {
            if (chip > 0)
                AsciiFPrint(Handle, ",");
            AsciiFPrint(Handle, "%a", gChipMapCfg[i].ChipMap[chip]);
        }
        AsciiFPrint(Handle, "\n");
    }

    AsciiFPrint(Handle, "LANG=%a\n", mSupportedLangList[gCurLang]);

    AsciiFPrint(Handle, "REPORTNUMERRS=%d\n", gReportNumErr);
    AsciiFPrint(Handle, "REPORTNUMWARN=%d\n", gReportNumWarn);

    AsciiFPrint(Handle, "DISABLESPD=%d\n", gDisableSPD);
    if (gMinSPDs > 0)
        AsciiFPrint(Handle, "MINSPDS=%d\n", gMinSPDs);
    if (gExactSPDs[0] >= 0)
    {
        AsciiSPrint(TempBuf, sizeof(TempBuf), "%d", gExactSPDs[0]);

        for (i = 1; i < MAX_NUM_SPDS; i++)
        {
            if (gExactSPDs[i] < 0)
                break;

            char NumBuf[16];
            AsciiSPrint(NumBuf, sizeof(NumBuf), ",%d", gExactSPDs[i]);
            AsciiStrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), NumBuf);
        }

        AsciiFPrint(Handle, "EXACTSPDS=%a\n", TempBuf);
    }
    if (gExactSPDSize >= 0)
        AsciiFPrint(Handle, "EXACTSPDSIZE=%ld\n", gExactSPDSize);
    AsciiFPrint(Handle, "CHECKMEMSPDSIZE=%d\n", gCheckMemSPDSize);

    for (i = 0; i < gNumCheckMemSpeed && gCheckMemSpeed != NULL; i++)
    {
        AsciiFPrint(Handle, "CHECKMEMSPEED");

        if (gCheckMemSpeed[i].PartNo[0] != '\0')
        {
            AsciiFPrint(Handle, ".%a", gCheckMemSpeed[i].PartNo);
        }

        AsciiFPrint(Handle, "=");
        if (gCheckMemSpeed[i].CheckSpeed == MEMSPEED_OFF)
        {
            AsciiFPrint(Handle, "OFF");
        }
        else if (gCheckMemSpeed[i].CheckSpeed == MEMSPEED_MAX)
        {
            AsciiFPrint(Handle, "MAX");
        }
        else if (gCheckMemSpeed[i].CheckSpeed == MEMSPEED_JEDEC)
        {
            AsciiFPrint(Handle, "JEDEC");
        }
        else if (gCheckMemSpeed[i].CheckSpeed == MEMSPEED_XMP1)
        {
            AsciiFPrint(Handle, "XMP1");
        }
        else if (gCheckMemSpeed[i].CheckSpeed == MEMSPEED_XMP2)
        {
            AsciiFPrint(Handle, "XMP2");
        }
        else if (gCheckMemSpeed[i].CheckSpeed == MEMSPEED_EXPO1)
        {
            AsciiFPrint(Handle, "EXPO1");
        }
        else if (gCheckMemSpeed[i].CheckSpeed == MEMSPEED_EXPO2)
        {
            AsciiFPrint(Handle, "EXPO2");
        }
        else
        {
            AsciiFPrint(Handle, "%d", gCheckMemSpeed[i].CheckSpeed);
        }
        AsciiFPrint(Handle, "\n");
    }
    AsciiFPrint(Handle, "CHECKSPDSMBIOS=%d\n", gCheckSPDSMBIOS);

    if (gSPDManuf[0] != L'\0')
        AsciiFPrint(Handle, "SPDMANUF=%s\n", gSPDManuf);
    if (gSPDPartNo[0] != L'\0')
        AsciiFPrint(Handle, "SPDPARTNO=%s\n", gSPDPartNo);
    AsciiFPrint(Handle, "SAMESPDPARTNO=%d\n", gSameSPDPartNo);
    AsciiFPrint(Handle, "SPDMATCH=%d\n", gSPDMatch);
    if (gSPDReportByteLo <= gSPDReportByteHi)
    {
        AsciiFPrint(Handle, "SPDREPORTBYTELO=%d\n", gSPDReportByteLo);
        AsciiFPrint(Handle, "SPDREPORTBYTEHI=%d\n", gSPDReportByteHi);
    }
    AsciiFPrint(Handle, "SPDREPORTEXTSN=%d\n", gSPDReportExtSN);

    if (gBgColour != EFI_BLACK)
        AsciiFPrint(Handle, "BGCOLOR=%s\n", SUPPORTED_BGCOLORS[gBgColour]);
    if (gHammerRandom == FALSE)
        AsciiFPrint(Handle, "HAMMERPAT=0x%08x\n", gHammerPattern);

    if (gHammerMode == HAMMER_SINGLE)
        AsciiFPrint(Handle, "HAMMERMODE=SINGLE\n");
    else
        AsciiFPrint(Handle, "HAMMERMODE=DOUBLE\n");

    AsciiFPrint(Handle, "HAMMERSTEP=0x%lx\n", gHammerStep);
    AsciiFPrint(Handle, "DISABLEMP=%d\n", gDisableMP);
    AsciiFPrint(Handle, "ENABLEHT=%d\n", gEnableHT);
    AsciiFPrint(Handle, "CONSOLEMODE=%d\n", gConsoleMode);
    AsciiFPrint(Handle, "CONSOLEONLY=%d\n", gConsoleOnly);
    AsciiFPrint(Handle, "BITFADESECS=%d\n", gBitFadeSecs);
    AsciiFPrint(Handle, "MAXERRCOUNT=%d\n", gMaxErrCount);
    AsciiFPrint(Handle, "TRIGGERONERR=%d\n", gTriggerOnErr);
    switch (gReportPrefix)
    {
    case REPORT_PREFIX_BASEBOARDSN:
        AsciiFPrint(Handle, "REPORTPREFIX=BASEBOARDSN\n");
        break;
    case REPORT_PREFIX_SYSINFOSN:
        AsciiFPrint(Handle, "REPORTPREFIX=SYSINFOSN\n");
        break;
    case REPORT_PREFIX_RAMSN:
        AsciiFPrint(Handle, "REPORTPREFIX=RAMSN\n");
        break;
    case REPORT_PREFIX_MACADDR:
        AsciiFPrint(Handle, "REPORTPREFIX=MACADDR\n");
        break;
    default:
        AsciiFPrint(Handle, "REPORTPREFIX=DEFAULT\n");
        break;
    }
    if (gReportPrepend != L'\0')
        AsciiFPrint(Handle, "REPORTPREPEND=%s\n", gReportPrepend);

    if (gTFTPServerIp.Addr[0] != 0)
        AsciiFPrint(Handle, "TFTPSERVERIP=%d.%d.%d.%d\n", gTFTPServerIp.v4.Addr[0], gTFTPServerIp.v4.Addr[1], gTFTPServerIp.v4.Addr[2], gTFTPServerIp.v4.Addr[3]);
    AsciiFPrint(Handle, "TFTPSTATUSSECS=%d\n", gTFTPStatusSecs);
    AsciiFPrint(Handle, "PMPDISABLE=%d\n", gPMPDisable);
    AsciiFPrint(Handle, "TCPDISABLE=%d\n", gTCPDisable);
    AsciiFPrint(Handle, "DHCPDISABLE=%d\n", gDHCPDisable);
    AsciiFPrint(Handle, "RTCSYNC=%d\n", gRTCSync);
    AsciiFPrint(Handle, "VERBOSITY=%d\n", gVerbosity);
    switch (gTPL)
    {
    case TPL_APPLICATION:
        AsciiFPrint(Handle, "TPL=APPLICATION\n");
        break;
    case TPL_CALLBACK:
        AsciiFPrint(Handle, "TPL=CALLBACK\n");
        break;
    case TPL_NOTIFY:
        AsciiFPrint(Handle, "TPL=NOTIFY\n");
        break;
    case TPL_HIGH_LEVEL:
        AsciiFPrint(Handle, "TPL=HIGH_LEVEL\n");
        break;
    }

    return EFI_SUCCESS;
}

static __inline BOOLEAN IsHexChar(CHAR16 Char)
{
    return (
        (Char >= L'0' && Char <= L'9') ||
        (Char >= L'A' && Char <= L'F') ||
        (Char >= L'a' && Char <= L'f'));
}

static __inline UINT8 HexCharToUint8(CHAR16 Char)
{
    if (Char >= L'0' && Char <= L'9')
    {
        return (UINT8)(Char - L'0');
    }

    return (UINT8)(10 + CharToUpper(Char) - L'A');
}

// MtSupportReadSPDFile
//
// Read the SPD values from file
EFI_STATUS
EFIAPI
MtSupportReadSPDFile(MT_HANDLE FileHandle, UINT8 *SPDBytes, UINT8 *BitMask, UINTN *NumSPDBytes)
{
    EFI_STATUS Status;
    CHAR16 *ReadLine;
    UINTN Size;
    UINTN LineNo = 1;
    UINT8 SPDData[MAX_SPD_LENGTH];
    UINT8 SPDMask[MAX_SPD_LENGTH];
    UINTN ByteIdx = 0;
    BOOLEAN Found;
    BOOLEAN Ascii;
    CHAR8 TempBuf[64];
    UINTN i;
    const UINTN BUFSIZE = 4096;

    if (FileHandle == NULL || SPDBytes == NULL || BitMask == NULL || NumSPDBytes == NULL)
    {
        return (EFI_INVALID_PARAMETER);
    }

    *NumSPDBytes = 0;

    Status = EFI_SUCCESS;
    Size = BUFSIZE;
    Found = FALSE;

    ReadLine = (CHAR16 *)AllocateZeroPool(Size);
    if (ReadLine == NULL)
    {
        return (EFI_OUT_OF_RESOURCES);
    }

    for (; !MtSupportEof(FileHandle); Size = BUFSIZE)
    {
        CHAR16 *LinePtr = ReadLine;
        SetMem(ReadLine, Size, 0);
        Status = MtSupportReadLine(FileHandle, ReadLine, &Size, TRUE, &Ascii);
        //
        // ignore too small of buffer...
        //
        if (Status == EFI_BUFFER_TOO_SMALL)
        {
            Status = EFI_SUCCESS;
        }
        if (EFI_ERROR(Status))
        {
            break;
        }

        while (((UINTN)LinePtr < (UINTN)ReadLine + Size) && *LinePtr)
        {
            while (((UINTN)LinePtr < (UINTN)ReadLine + Size) && *LinePtr &&
                   ((*LinePtr == L' ') || (*LinePtr == L'\t')))
                LinePtr++;

            if (((UINTN)LinePtr >= (UINTN)ReadLine + Size) || *LinePtr == L'\0')
                break;

            if (*LinePtr == L'?')
            {
                SPDMask[ByteIdx] = 0x00;
                SPDData[ByteIdx] = 0x00;
            }
            else if (IsHexChar(*LinePtr))
            {
                UINT8 SPDByte;
                SPDMask[ByteIdx] = 0xF0;
                SPDByte = HexCharToUint8(*LinePtr);
                SPDData[ByteIdx] = (SPDByte & 0x0F) << 4;
            }
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[Upper nibble] Unexpected character '%c' (\\x%02X) found in line %d, col %d", *LinePtr, *LinePtr, LineNo, (UINTN)(LinePtr - ReadLine) + 1);
                Status = EFI_LOAD_ERROR;
                break;
            }
            LinePtr++;

            if (((UINTN)LinePtr >= (UINTN)ReadLine + Size) || *LinePtr == L'\0')
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "EOL found expecting hex char in line %d, col %d", LineNo, (UINTN)(LinePtr - ReadLine) + 1);
                Status = EFI_LOAD_ERROR;
                break;
            }

            if (*LinePtr == L'?')
            {
                SPDMask[ByteIdx] &= 0xF0;
                SPDData[ByteIdx] &= 0xF0;
            }
            else if (IsHexChar(*LinePtr))
            {
                UINT8 SPDByte;
                SPDMask[ByteIdx] |= 0x0F;
                SPDByte = HexCharToUint8(*LinePtr);
                SPDData[ByteIdx] |= SPDByte & 0x0F;
            }
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[Lower nibble] Unexpected character '%c' (\\x%02X) found in line %d, col %d", *LinePtr, *LinePtr, LineNo, (UINTN)(LinePtr - ReadLine) + 1);
                Status = EFI_LOAD_ERROR;
                break;
            }
            LinePtr++;

            ByteIdx++;

            if (ByteIdx >= MAX_SPD_LENGTH)
                break;
        }

        if (EFI_ERROR(Status))
            break;

        if (ByteIdx >= MAX_SPD_LENGTH)
            break;

        LineNo++;
    }

    FreePool(ReadLine);

    if (EFI_ERROR(Status))
        return Status;

    *NumSPDBytes = ByteIdx;
    CopyMem(SPDBytes, SPDData, ByteIdx);
    CopyMem(BitMask, SPDMask, ByteIdx);

    AsciiFPrint(DEBUG_FILE_HANDLE, "SPDMATCH SPD bytes (%d bytes):", *NumSPDBytes);

    TempBuf[0] = '\0';
    for (i = 0; i < *NumSPDBytes; i++)
    {
        CHAR8 SPDHex[8];
        AsciiSPrint(SPDHex, sizeof(SPDHex), "%02X ", SPDBytes[i]);
        AsciiStrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), SPDHex);

        if (((i % 16) == 15) || (i == *NumSPDBytes - 1))
        {
            MtSupportDebugWriteLine(TempBuf);
            TempBuf[0] = '\0';
        }
    }

    AsciiFPrint(DEBUG_FILE_HANDLE, "SPDMATCH mask bytes (%d bytes):", *NumSPDBytes);

    TempBuf[0] = '\0';
    for (i = 0; i < *NumSPDBytes; i++)
    {
        CHAR8 SPDHex[8];
        AsciiSPrint(SPDHex, sizeof(SPDHex), "%02X ", BitMask[i]);
        AsciiStrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), SPDHex);

        if (((i % 16) == 15) || (i == *NumSPDBytes - 1))
        {
            MtSupportDebugWriteLine(TempBuf);
            TempBuf[0] = '\0';
        }
    }

    return (Status);
}

BOOLEAN
EFIAPI
MtSupportIsValidSPDFile(CHAR16 *FileName)
{
    BOOLEAN Valid = TRUE;
    EFI_STATUS Status = EFI_SUCCESS;
    MT_HANDLE SPDFileHandle = NULL;
    UINT8 SPDMatchData[MAX_SPD_LENGTH];
    UINT8 SPDMatchMask[MAX_SPD_LENGTH];
    UINTN SPDMatchNumBytes = 0;

    SetMem(SPDMatchData, sizeof(SPDMatchData), 0);
    SetMem(SPDMatchMask, sizeof(SPDMatchMask), 0);

    // Read SPD data from file for matching
    Status = MtSupportOpenFile(&SPDFileHandle, FileName, EFI_FILE_MODE_READ, 0);

    if (SPDFileHandle)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Found %s file", FileName);
        Status = MtSupportReadSPDFile(SPDFileHandle, SPDMatchData, SPDMatchMask, &SPDMatchNumBytes);
        if (EFI_ERROR(Status))
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to read %s (%r). File data may be invalid.", SPD_FILENAME, Status);
            Valid = FALSE;
        }
        MtSupportCloseFile(SPDFileHandle);
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to open %s: %r", SPD_FILENAME, Status);
        Valid = FALSE;
    }
    return Valid;
}

BOOLEAN
EFIAPI
MtSupportCheckSPD(CHAR16 *FileName, int *pFailedSPDMatchIdx, int *pFailedSPDMatchByte, UINT8 *pActualValue, UINT8 *pExpectedValue)
{
    EFI_STATUS Status = EFI_SUCCESS;
    MT_HANDLE SPDFileHandle = NULL;
    UINT8 SPDMatchData[MAX_SPD_LENGTH];
    UINT8 SPDMatchMask[MAX_SPD_LENGTH];
    UINTN SPDMatchNumBytes = 0;
    int i;
    BOOLEAN Match = TRUE;

    SetMem(SPDMatchData, sizeof(SPDMatchData), 0);
    SetMem(SPDMatchMask, sizeof(SPDMatchMask), 0);

    // Read SPD data from file for matching
    Status = MtSupportOpenFile(&SPDFileHandle, FileName, EFI_FILE_MODE_READ, 0);

    if (SPDFileHandle)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Found %s file", FileName);
        Status = MtSupportReadSPDFile(SPDFileHandle, SPDMatchData, SPDMatchMask, &SPDMatchNumBytes);
        if (EFI_ERROR(Status))
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to read %s (%r). File data may be invalid.", SPD_FILENAME, Status);
        }
        MtSupportCloseFile(SPDFileHandle);
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to open %s: %r", SPD_FILENAME, Status);
    }

    for (i = 0; i < g_numMemModules; i++)
    {
        int j;
        for (j = 0; j < (int)SPDMatchNumBytes; j++)
        {
            if ((g_MemoryInfo[i].rawSPDData[j] & SPDMatchMask[j]) != (SPDMatchData[j] & SPDMatchMask[j]))
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Module %d, byte %d (0x%02X) does not match the %s file, byte %d (0x%02X)", i, j, g_MemoryInfo[i].rawSPDData[j], SPD_FILENAME, j, SPDMatchData[j]);

                if (*pFailedSPDMatchIdx < 0)
                {
                    *pFailedSPDMatchIdx = i;
                    *pFailedSPDMatchByte = j;
                    *pActualValue = g_MemoryInfo[i].rawSPDData[j];
                    *pExpectedValue = SPDMatchData[j];
                    Match = FALSE;
                }
            }
        }
    }
    return Match;
}

VOID
    EFIAPI
    MtSupportOutputSysInfo(MT_HANDLE FileHandle)
{
    CHAR16 TextBuf[64];
    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[128];

    cpu_type(TextBuf, sizeof(TextBuf));

    FPrint(FileHandle, L"%s: \r\n", GetStringById(STRING_TOKEN(STR_REPORT_SYSTEM), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_MANUFACTURER), TempBuf, sizeof(TempBuf)), gSystemInfo.szManufacturer);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_PRODUCT), TempBuf, sizeof(TempBuf)), gSystemInfo.szProductName);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_VERSION), TempBuf, sizeof(TempBuf)), gSystemInfo.szVersion);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SERIAL_NUM), TempBuf, sizeof(TempBuf)), gSystemInfo.szSerialNumber);

    FPrint(FileHandle, L"%s: \r\n", GetStringById(STRING_TOKEN(STR_REPORT_BIOS), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_VENDOR), TempBuf, sizeof(TempBuf)), gBIOSInfo.szVendor);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_VERSION), TempBuf, sizeof(TempBuf)), gBIOSInfo.szBIOSVersion);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_RELEASE_DATE), TempBuf, sizeof(TempBuf)), gBIOSInfo.szBIOSReleaseDate);

    FPrint(FileHandle, L"%s: \r\n", GetStringById(STRING_TOKEN(STR_REPORT_BASEBOARD), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_MANUFACTURER), TempBuf, sizeof(TempBuf)), gBaseBoardInfo.szManufacturer);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_PRODUCT), TempBuf, sizeof(TempBuf)), gBaseBoardInfo.szProductName);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_VERSION), TempBuf, sizeof(TempBuf)), gBaseBoardInfo.szVersion);
    FPrint(FileHandle, L"  %s: %a\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SERIAL_NUM), TempBuf, sizeof(TempBuf)), gBaseBoardInfo.szSerialNumber);

    FPrint(FileHandle, L"%s: %s\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TYPE), TempBuf, sizeof(TempBuf)), TextBuf);

    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));
    FPrint(FileHandle, L"  %s: %s\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_SPEED), TempBuf, sizeof(TempBuf)), TextBuf);

    if (gCPUInfo.L1_data_caches_per_package > 0)
        FPrint(FileHandle, L"  %s: %d x %dK (%d MB/s)\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L1_CACHE), TempBuf, sizeof(TempBuf)), gCPUInfo.L1_data_caches_per_package, gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
    else
        FPrint(FileHandle, L"  %s: %dK (%d MB/s)\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L1_CACHE), TempBuf, sizeof(TempBuf)), gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);

    if (gCPUInfo.L2_caches_per_package > 0)
        FPrint(FileHandle, L"  %s: %d x %dK (%d MB/s)\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L2_CACHE), TempBuf, sizeof(TempBuf)), gCPUInfo.L2_caches_per_package, gCPUInfo.L2_cache_size, l2_speed);
    else
        FPrint(FileHandle, L"  %s: %dK (%d MB/s)\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L2_CACHE), TempBuf, sizeof(TempBuf)), gCPUInfo.L2_cache_size, l2_speed);

    if (gCPUInfo.L3_cache_size)
    {
        if (gCPUInfo.L3_caches_per_package > 0)
            FPrint(FileHandle, L"  %s: %d x %dK (%d MB/s)\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)), gCPUInfo.L3_caches_per_package, gCPUInfo.L3_cache_size, l3_speed);
        else
            FPrint(FileHandle, L"  %s: %dK (%d MB/s)\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)), gCPUInfo.L3_cache_size, l3_speed);
    }
    else
    {
        FPrint(FileHandle, L"  %s: %s\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_MENU_NA), TextBuf, sizeof(TextBuf)));
    }

    FPrint(FileHandle, L"%s: \r\n", GetStringById(STRING_TOKEN(STR_MEMORY), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"  %s: %ldM (%d MB/s)\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_TOTAL_MEM), TempBuf, sizeof(TempBuf)), MtSupportMemSizeMB(), mem_speed);

    FPrint(FileHandle, L"  %s: %s\r\n", GetStringById(STRING_TOKEN(STR_REPORT_RAM_CONFIG), TempBuf, sizeof(TempBuf)), getMemConfigStr(TempBuf2, sizeof(TempBuf2)));
    FPrint(FileHandle, L"  %s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_SPD), TempBuf, sizeof(TempBuf)), g_numMemModules);

    for (int i = 0; i < g_numMemModules; i++)
    {
        CHAR16 szwRAMSummary[128];
        getSPDSummaryLine(&g_MemoryInfo[i], szwRAMSummary, sizeof(szwRAMSummary));

        FPrint(FileHandle, L"    SPD #%d: %s\r\n", i, szwRAMSummary);
        FPrint(FileHandle, L"      %s: ", GetStringById(STRING_TOKEN(STR_REPORT_VENDOR_PART_INFO), TempBuf, sizeof(TempBuf)));
        if (g_MemoryInfo[i].jedecManuf[0])
            FPrint(FileHandle, L"%s ", g_MemoryInfo[i].jedecManuf);

        if (g_MemoryInfo[i].modulePartNo[0])
            FPrint(FileHandle, L"%a ", g_MemoryInfo[i].modulePartNo);

        if (g_MemoryInfo[i].moduleSerialNo)
        {
            if (gSPDReportExtSN)
                FPrint(FileHandle, L"%s", BytesToStrHex(g_MemoryInfo[i].moduleExtSerialNo, sizeof(g_MemoryInfo[i].moduleExtSerialNo), TempBuf, sizeof(TempBuf)));
            else
                FPrint(FileHandle, L"%08X", g_MemoryInfo[i].moduleSerialNo);
        }
        FPrint(FileHandle, L"\r\n");

        int clkspeed = g_MemoryInfo[i].clkspeed;
        int tCK = g_MemoryInfo[i].tCK;
        int tAA = g_MemoryInfo[i].tAA;
        int tRCD = g_MemoryInfo[i].tRCD;
        int tRP = g_MemoryInfo[i].tRP;
        int tRAS = g_MemoryInfo[i].tRAS;
        CHAR16 *pszVoltage = g_MemoryInfo[i].moduleVoltage;

        FPrint(FileHandle, L"      JEDEC Profile: %s\r\n",
               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));

        if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR5)
        {
            if (g_MemoryInfo[i].specific.DDR5SDRAM.XMPSupported)
            {
                for (int j = 0; j < MAX_XMP3_PROFILES; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].enabled)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tAA;
                        tRCD = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].moduleVDD;

                        FPrint(FileHandle, L"      XMP Profile %d: %s\r\n", j + 1,
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                    }
                }
            }

            if (g_MemoryInfo[i].specific.DDR5SDRAM.EXPOSupported)
            {
                for (int j = 0; j < MAX_EXPO_PROFILES; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].enabled)
                    {
                        if (g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tCK <= tCK)
                        {
                            clkspeed = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].clkspeed;
                            tCK = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tCK;
                            tAA = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tAA;
                            tRCD = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRCD;
                            tRP = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRP;
                            tRAS = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRAS;
                            pszVoltage = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].moduleVDD;

                            FPrint(FileHandle, L"      EXPO Profile %d: %s\r\n", j + 1,
                                   getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                        }
                    }
                }
            }
        }
        else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR3)
        {
            if (g_MemoryInfo[i].specific.DDR3SDRAM.XMPSupported)
            {
                for (int j = 0; j < 2; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].enabled)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tAA;
                        tRCD = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].moduleVdd;

                        FPrint(FileHandle, L"      XMP Profile %d: %s\r\n", j + 1,
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                    }
                }
            }
        }
        else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR4)
        {
            if (g_MemoryInfo[i].specific.DDR4SDRAM.XMPSupported)
            {
                for (int j = 0; j < 2; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].enabled)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tAA;
                        tRCD = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].moduleVdd;

                        FPrint(FileHandle, L"      XMP Profile %d: %s\r\n", j + 1,
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                    }
                }
            }
        }
        else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR2)
        {
            if (g_MemoryInfo[i].specific.DDR2SDRAM.EPPSupported)
            {
                if (g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_ABBR)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].CASSupported * tCK;
                        tRCD = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].voltageLevel;

                        FPrint(FileHandle, L"      EPP Abbreviated Profile %d: %s\r\n", j + 1,
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                    }
                }
                else if (g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_FULL)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].CASSupported * tCK;
                        tRCD = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].voltageLevel;

                        FPrint(FileHandle, L"      EPP Full Profile %d: %s\r\n", j + 1,
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                    }
                }
            }
        }
    }

    FPrint(FileHandle, L"  %s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM_SLOTS), TempBuf, sizeof(TempBuf)), g_numSMBIOSMem);
    FPrint(FileHandle, L"  %s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM), TempBuf, sizeof(TempBuf)), g_numSMBIOSModules);
    for (int i = 0; i < g_numSMBIOSMem; i++)
    {
        CHAR16 szwRAMSummary[128];

        if (g_SMBIOSMemDevices[i].smb17.Size != 0)
        {
            getSMBIOSRAMSummaryLine(&g_SMBIOSMemDevices[i], szwRAMSummary, sizeof(szwRAMSummary));
            FPrint(FileHandle, L"    %s: %s\r\n", MtSupportGetSlotLabel(i, TRUE, TempBuf, sizeof(TempBuf)), szwRAMSummary);
            FPrint(FileHandle, L"      %s: ", GetStringById(STRING_TOKEN(STR_REPORT_VENDOR_PART_INFO), TempBuf, sizeof(TempBuf)));
            if (g_SMBIOSMemDevices[i].szManufacturer[0])
                FPrint(FileHandle, L"%a ", g_SMBIOSMemDevices[i].szManufacturer);

            if (g_SMBIOSMemDevices[i].szPartNumber[0])
                FPrint(FileHandle, L"%a ", g_SMBIOSMemDevices[i].szPartNumber);

            if (g_SMBIOSMemDevices[i].szSerialNumber[0])
                FPrint(FileHandle, L"%a", g_SMBIOSMemDevices[i].szSerialNumber);
            FPrint(FileHandle, L"\r\n");

            FPrint(FileHandle, L"      SMBIOS Profile: %s\r\n", getSMBIOSRAMProfileStr(&g_SMBIOSMemDevices[i], TempBuf, sizeof(TempBuf)));
        }
        else
        {
            FPrint(FileHandle, L"    %s: %s\r\n", MtSupportGetSlotLabel(i, TRUE, TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_REPORT_EMPTY), TempBuf2, sizeof(TempBuf2)));
        }
    }
}

VOID
    EFIAPI
    MtSupportDisplayErrorSummary()
{
    int i, n;
    UINTN addr;
    UINTN mb;

    UINT32 hours, mins, secs;
    UINT32 t;
    UINTN Percent;

    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[128];

    gST->ConOut->ClearScreen(gST->ConOut);

    t = mElapsedTime;

    secs = t % 60;
    t /= 60;
    mins = t % 60;
    t /= 60;
    hours = t;

    Print(L"%s %s (Build: %s) %s\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD, GetStringById(STRING_TOKEN(STR_REPORT_RESULT_SUMMARY), TempBuf, sizeof(TempBuf)));
    Print(L"PassMark Software\nwww.passmark.com\n\n");

    const int VALUE_OFFSET = 35;
    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_TEST_TIME), TempBuf, sizeof(TempBuf)));
    ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d-%02d-%02d %02d:%02d:%02d\n", mTestStartTime.Year, mTestStartTime.Month, mTestStartTime.Day, mTestStartTime.Hour, mTestStartTime.Minute, mTestStartTime.Second);
    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_ELAPSED_TIME), TempBuf, sizeof(TempBuf)));
    ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d:%02d:%02d\n", hours, mins, secs);
    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_CPUS_ACTIVE), TempBuf, sizeof(TempBuf)));
    ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d\n", MtSupportCPUSelMode() == CPU_PARALLEL ? MPSupportGetNumEnabledProcessors() : 1);

    StrCpyS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TEMP), TempBuf2, sizeof(TempBuf2)));
    UnicodeCatf(TempBuf, sizeof(TempBuf), L" (%s)", GetStringById(STRING_TOKEN(STR_MIN_MAX_AVE), TempBuf2, sizeof(TempBuf2)));
    ConsolePrintXY(-1, -1, L"%s", TempBuf);
    if (mNumCPUTempSamples > 0)
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %dC/%dC/%dC\n", mMinCPUTemp, mMaxCPUTemp, DivS64x64Remainder(mSumCPUTemp, mNumCPUTempSamples, NULL));
    }
    else
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  -/-/-\n");
    }

    StrCpyS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_RAM_TEMP), TempBuf2, sizeof(TempBuf2)));
    UnicodeCatf(TempBuf, sizeof(TempBuf), L" (%s)", GetStringById(STRING_TOKEN(STR_MIN_MAX_AVE), TempBuf2, sizeof(TempBuf2)));
    ConsolePrintXY(-1, -1, L"%s", TempBuf);
    if (mNumRAMTempSamples[0] > 0)
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %dC/%dC/%dC\n", DivS64x64Remainder(mMinRAMTemp[0], 1000, NULL), DivS64x64Remainder(mMaxRAMTemp[0], 1000, NULL), DivS64x64Remainder(mSumRAMTemp[0], MultS64x64(mNumRAMTempSamples[0], 1000), NULL));
    }
    else
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  -/-/-\n", TempBuf);
    }

    ConsolePrintXY(-1, -1, GetStringById(STRING_TOKEN(STR_REPORT_LOW_MEM_SPEED), TempBuf, sizeof(TempBuf)));
    if (mCurMemTimings.SpeedMTs > 0)
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d MT/s (%d-%d-%d-%d)\n", mMinMemTimings.SpeedMTs, mMinMemTimings.tCAS, mMinMemTimings.tRCD, mMinMemTimings.tRP, mMinMemTimings.tRAS);
    }
    else
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %s\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf2, sizeof(TempBuf2)));
    }

    ConsolePrintXY(-1, -1, GetStringById(STRING_TOKEN(STR_REPORT_HI_MEM_SPEED), TempBuf, sizeof(TempBuf)));
    if (mCurMemTimings.SpeedMTs > 0)
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d MT/s (%d-%d-%d-%d)\n", mMaxMemTimings.SpeedMTs, mMaxMemTimings.tCAS, mMaxMemTimings.tRCD, mMaxMemTimings.tRP, mMaxMemTimings.tRAS);
    }
    else
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %s\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf2, sizeof(TempBuf2)));
    }

    UINTN TotalTests = gNumPasses * MtSupportNumTests();

    Percent = (UINTN)DivU64x64Remainder(
        MultU64x32(MtSupportTestsCompleted(), 100),
        TotalTests,
        NULL);

    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_NUM_TESTS_COMPLETED), TempBuf, sizeof(TempBuf)));
    ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d/%d (%d%%)\n", MtSupportTestsCompleted(), TotalTests, Percent);

    Percent = MtSupportTestsCompleted() == 0 ? 0 : (UINTN)DivU64x64Remainder(MultU64x32(mPassCount, 100), MtSupportTestsCompleted(), NULL);

    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_NUM_PASSED), TempBuf, sizeof(TempBuf)));
    ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d/%d (%d%%)\n", mPassCount, MtSupportTestsCompleted(), Percent);
    Print(L"\n");

    addr = erri.low_addr;
    mb = erri.low_addr >> 20;
    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_LOW_ERROR_ADDR), TempBuf, sizeof(TempBuf)));
    if (erri.low_addr <= erri.high_addr)
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  0x%p (%ldMB)\n", addr, DivU64x32(addr, SIZE_1MB));
    }
    else
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %s\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf2, sizeof(TempBuf2)));
    }

    addr = erri.high_addr;
    mb = erri.high_addr >> 20;
    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_HI_ERROR_ADDR), TempBuf, sizeof(TempBuf)));
    if (erri.low_addr <= erri.high_addr)
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  0x%p (%ldMB)\n", addr, DivU64x32(addr, SIZE_1MB));
    }
    else
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %s\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf2, sizeof(TempBuf2)));
    }

    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_BIT_MASK), TempBuf, sizeof(TempBuf)));
    if (sizeof(void *) > 4)
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %016lX\n", erri.ebits);
    }
    else
    {
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %08X\n", erri.ebits);
    }

    /* Calc bits in error */
    for (i = 0, n = 0; i < sizeof(void *) * 8; i++)
    {
        if (erri.ebits >> i & 1)
        {
            n++;
        }
    }

    UnicodeSPrint(TempBuf2, sizeof(TempBuf2), GetStringById(STRING_TOKEN(STR_REPORT_ERROR_BIT_SUMMARY), TempBuf, sizeof(TempBuf)), n, erri.min_bits, erri.max_bits, erri.data_err > 0 ? erri.tbits / erri.data_err : 0);
    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_BITS), TempBuf, sizeof(TempBuf)));
    ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %s\n", TempBuf2);

    ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_CONTIGUOUS), TempBuf, sizeof(TempBuf)));
    ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d\n", erri.maxl);

    TempBuf2[0] = L'\0';
    for (i = 0; i < sizeof(erri.ecpus) / sizeof(erri.ecpus[0]); i++)
    {
        for (n = 0; n < sizeof(erri.ecpus[0]) * 8; n++)
        {
            int cpu = i * sizeof(erri.ecpus[0]) * 8 + n;
            if (erri.ecpus[i] & (LShiftU64(1, cpu)))
            {
                CHAR16 CPUBuf[8];
                UnicodeSPrint(CPUBuf, sizeof(CPUBuf), L"%d", cpu);
                if (TempBuf2[0] != L'\0')
                    StrCatS(TempBuf2, sizeof(TempBuf2) / sizeof(TempBuf2[0]), L", ");
                StrCatS(TempBuf2, sizeof(TempBuf2) / sizeof(TempBuf2[0]), CPUBuf);
            }
        }
    }
    if (TempBuf2[0] != L'\0')
    {
        ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_CPUS), TempBuf, sizeof(TempBuf)));
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  { %s }\n", TempBuf2);
    }

    if (MtSupportGetNumCorrECCErrors() > 0 || MtSupportGetNumUncorrECCErrors() > 0)
    {
        ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_ECC_CORRECTABLE), TempBuf, sizeof(TempBuf)));
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d\n", MtSupportGetNumCorrECCErrors());
        ConsolePrintXY(-1, -1, L"%s", GetStringById(STRING_TOKEN(STR_ECC_UNCORRECTABLE), TempBuf2, sizeof(TempBuf2)));
        ConsolePrintXY(VALUE_OFFSET, -1, L"  :  %d\n", MtSupportGetNumUncorrECCErrors());
    }
    else
    {
        Print(L"\n");
    }
    Print(L"\n");

    if (MtSupportCPUSelMode() != CPU_PARALLEL)
    {
        Print(L"%s\n", GetStringById(STRING_TOKEN(STR_NOTE_PARALLEL_1), TempBuf, sizeof(TempBuf)));
        Print(L"%s\n", GetStringById(STRING_TOKEN(STR_NOTE_PARALLEL_2), TempBuf, sizeof(TempBuf)));
    }

#if 0
    cprint(LINE_HEADER + 1, 36, " -      . MB");
    dprint(LINE_HEADER + 1, 39, mb, 5, 0);
    dprint(LINE_HEADER + 1, 45, ((page & 0xF) * 10) / 16, 1, 0);
#endif
#if 0
    cprint(LINE_HEADER + 2, 36, " -      . MB");
    dprint(LINE_HEADER + 2, 39, mb, 5, 0);
    dprint(LINE_HEADER + 2, 45, ((page & 0xF) * 10) / 16, 1, 0);
#endif

    Print(L"\n");
    Print(GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)));
    MtUiWaitForKey(0, NULL);
    gST->ConOut->ClearScreen(gST->ConOut);

    Print(L"\n%s  %s\n", GetStringById(STRING_TOKEN(STR_TEST), TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_ERRORS), TempBuf2, sizeof(TempBuf2)));
    for (i = 0; i < (int)mTestCount; i++)
    {
        Print(L"%2d  %8d\n", mTests[i].TestNo, mTests[i].NumErrors);
    }
}

static BOOL EncodeCharToEntity(CHAR8 srcChar, CHAR8 *destStr, UINTN *destStrLen)
{
    CHAR8 tmpEntity[10];
    BOOL IsEntity = FALSE;
    UINTN destStrLenAvail;

    if (destStr == NULL && *destStrLen > 0)
        return FALSE;

    destStrLenAvail = *destStrLen;

    switch (srcChar)
    {
    case '&':
        AsciiStrCpyS(tmpEntity, sizeof(tmpEntity) / sizeof(tmpEntity[0]), "&amp;");
        IsEntity = TRUE;
        break;
    case '\"':
        AsciiStrCpyS(tmpEntity, sizeof(tmpEntity) / sizeof(tmpEntity[0]), "&quot;");
        IsEntity = TRUE;
        break;
    case '\'':
        AsciiStrCpyS(tmpEntity, sizeof(tmpEntity) / sizeof(tmpEntity[0]), "&apos;");
        IsEntity = TRUE;
        break;
    case '<':
        AsciiStrCpyS(tmpEntity, sizeof(tmpEntity) / sizeof(tmpEntity[0]), "&lt;");
        IsEntity = TRUE;
        break;
    case '>':
        AsciiStrCpyS(tmpEntity, sizeof(tmpEntity) / sizeof(tmpEntity[0]), "&gt;");
        IsEntity = TRUE;
        break;
    default:
        // Non-printable ASCII chars
        if (srcChar > 0x7E)
        {
            AsciiSPrint(tmpEntity, sizeof(tmpEntity), "&#x%02X;", srcChar);
            IsEntity = TRUE;
        }
        else if (srcChar < 0x20) // No control characters
        {
            AsciiStrCpyS(tmpEntity, sizeof(tmpEntity) / sizeof(tmpEntity[0]), " ");
            IsEntity = TRUE;
        }
        break;
    }

    if (IsEntity)
    {
        *destStrLen = AsciiStrLen(tmpEntity);
        if (*destStrLen > destStrLenAvail)
        {
            AsciiStrCpyS(destStr, destStrLenAvail, "."); // no room, so we can only give them a dot
            *destStrLen = 1;
        }
        else
            AsciiStrCpyS(destStr, destStrLenAvail, tmpEntity);
    }

    return IsEntity;
}

static UINTN ConvertStringToEntities(CHAR8 *srcStr, CHAR8 *destStr, UINTN destStrLen)
{
    UINTN srcStrLen;
    UINTN srcPos = 0, destPos = 0;
    UINTN entLen;

    if (srcStr == NULL || destStr == NULL || destStrLen == 0)
        return 0;

    srcStrLen = AsciiStrLen(srcStr);

    destStr[0] = '\0';

    for (srcPos = 0; srcPos < srcStrLen && destPos < destStrLen; srcPos++)
    {
        entLen = destStrLen - destPos;
        if (EncodeCharToEntity(srcStr[srcPos], destStr + destPos, &entLen))
        {
            // converted to entity, entLen now contains the length of the entity string
            destPos += entLen;
        }
        else
        {
            // not an entity character, we simply copy it over
            destStr[destPos] = srcStr[srcPos];
            destPos++;
        }
    }

    destStr[destPos] = '\0';
    return destPos;
}

static void RemoveInvalidFilenameChars(CHAR16 *srcStr, UINTN srcStrSize)
{
    UINTN srcStrLen;
    UINTN srcPos = 0;

    if (srcStr == NULL)
        return;

    srcStrLen = StrnLenS(srcStr, srcStrSize);

    for (srcPos = 0; srcPos < srcStrLen; srcPos++)
    {
        switch (srcStr[srcPos])
        {
        case L'\\':
        case L'<':
        case L'>':
        case L'*':
        case L'?':
        case L'/':
        case L'\'':
        case L'\"':
        case L':':
        case L'`':
        case L'|':
            srcStr[srcPos] = L' ';
            break;
        default:
            break;
        }
    }
}

VOID
    EFIAPI
    MtSupportWriteReportHeader(MT_HANDLE FileHandle)
{
#ifdef PRO_RELEASE
    EFI_STATUS Status;
    MT_HANDLE CSSHandle = NULL, HeaderHandle = NULL;
#endif
    UINTN BOMSize = 2;

    // Write UTF16LE BOM
    MtSupportWriteFile(FileHandle, &BOMSize, (void *)BOM_UTF16LE);

    FPrint(FileHandle, L"<html>\n");
    FPrint(FileHandle, L"<head>\n");
    FPrint(FileHandle, L"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\">\n");

#ifdef PRO_RELEASE
    MtSupportOpenFile(&CSSHandle, CSS_FILENAME, EFI_FILE_MODE_READ, 0);
    if (CSSHandle != NULL)
    {
        UINT8 ReadBuf[512];
        UINTN ReadBufSize = sizeof(ReadBuf);

        FPrint(FileHandle, L"<style>\n");

        ReadBufSize = 2;
        // Look for UTF16 BOM
        if (MtSupportReadFile(CSSHandle, &ReadBufSize, ReadBuf) == EFI_SUCCESS && ReadBufSize > 0)
        {
            // If not found, reset pointer to beginning of file
            if (CompareMem(ReadBuf, (void *)BOM_UTF16LE, 2) != 0)
                MtSupportSetPosition(CSSHandle, 0);
        }

        ReadBufSize = sizeof(ReadBuf);
        while ((Status = MtSupportReadFile(CSSHandle, &ReadBufSize, ReadBuf)) == EFI_SUCCESS && ReadBufSize > 0)
        {
            Status = MtSupportWriteFile(FileHandle, &ReadBufSize, ReadBuf);
        }

        MtSupportCloseFile(CSSHandle);

        FPrint(FileHandle, L"</style>\n");
    }
    else
    {
#endif
        FPrint(FileHandle, L"<style>\n");
        FPrint(FileHandle, L"body, p { font-family: Arial, sans-serif; color:black; width: 600px; font-size: 10px}\n");
        FPrint(FileHandle, L"h1 {  width: 640px;    margin-bottom:20px; }\n");
        FPrint(FileHandle, L"h2 {  width: 640px;    margin-bottom:15px; font-size: 16px; font-weight: normal}\n");
        FPrint(FileHandle, L".failstamp { position: absolute; top: 16px; left: 500px; width: 150px; padding: 5px 0; border: 5px solid red; font-family: Arial; font-size: 50px; font-weight: bold; color:red; text-align: center; text-transform: uppercase; transform: rotate(-10deg);}\n");
        FPrint(FileHandle, L".passstamp { position: absolute; top: 16px; left: 500px; width: 150px; padding: 5px 0; border: 5px solid green; font-family: Arial; font-size: 50px; font-weight: bold; color:green; text-align: center; text-transform: uppercase; transform: rotate(-10deg);}\n");

        FPrint(FileHandle, L".header table {  width: 640px; margin:0px; text-align: left; }\n");

        FPrint(FileHandle, L".summary table, tr {  width: 600px; margin-left:20px; font-size: 10px; margin-bottom: 20px; border-collapse: collapse; }\n");
        FPrint(FileHandle, L".summary td { padding-left:5px; }\n");
        FPrint(FileHandle, L".summary td.PASS {  }\n");
        FPrint(FileHandle, L".summary td.INCOMPLETE {  }\n");
        FPrint(FileHandle, L".summary td.FAIL {  }\n");

        FPrint(FileHandle, L".sysinfo table, tr {  width: 600px; margin-left:20px; font-size: 10px; margin-bottom: 20px; border-collapse: collapse; }\n");
        FPrint(FileHandle, L".sysinfo td { border: 1px solid black; padding-left:5px; text-align: left; }\n");
        FPrint(FileHandle, L".sysinfo td.value {  }\n");
        FPrint(FileHandle, L".sysinfo td.subvalue { padding-left:25px; }\n");
        FPrint(FileHandle, L".sysinfo td.subvalue-2 { padding-left:30px; }\n");
        FPrint(FileHandle, L".sysinfo td.altvalue { }\n");

        FPrint(FileHandle, L"div.results { width: 600px; margin-left:20px; font-size: 10px; margin-bottom: 20px; }\n");
        FPrint(FileHandle, L"div.results table { width: 100%%; border-collapse: collapse; margin-bottom: 10px; }\n");

        FPrint(FileHandle, L"div.results table.errormask { border: 0; border-collapse: collapse; }\n");
        FPrint(FileHandle, L"div.results table.errormask tr { height: 10px; }\n");
        FPrint(FileHandle, L"div.results table.errormask td { border: 1px solid black; }\n");
        FPrint(FileHandle, L"div.results table.errormask tr.noborder td { padding : 0; border : 0; font-size: 8px; }\n");
        FPrint(FileHandle, L"div.results table.errormask td.bitok { background-color: green; color: green; padding : 0; font-family: Courier New, Courier, monospace;}\n");
        FPrint(FileHandle, L"div.results table.errormask td.biterr { background-color: red; color: red; padding : 0; font-family: Courier New, Courier, monospace;}\n");

        FPrint(FileHandle, L".results tr.fail {  }\n");
        FPrint(FileHandle, L".results td { border: 1px solid black; padding-left:5px; text-align: left; }\n");
        FPrint(FileHandle, L".results td.value {  }\n");
        FPrint(FileHandle, L".results td.subvalue { padding-left:25px; }\n");
        FPrint(FileHandle, L".results td.altvalue {  }\n");
        FPrint(FileHandle, L".results td.PASS { }\n");
        FPrint(FileHandle, L".results td.INCOMPLETE {  }\n");
        FPrint(FileHandle, L".results td.FAIL { }\n");
        FPrint(FileHandle, L".results td.header { background-color:#222222;     color:white; }\n");

        FPrint(FileHandle, L".footer { font-size: 10px; margin-top: 10px; text-align: left}\n");
        FPrint(FileHandle, L".footer p { font-size: 9px; border-top: solid #000000 1px; margin-top: 10px; }\n");
        FPrint(FileHandle, L".footer table, tr {  width: 600px; margin-left:20px; font-size: 10px; margin-bottom: 20px; border-collapse: collapse; }\n");
        FPrint(FileHandle, L".footer td { padding-left:5px; border: none}\n");

        FPrint(FileHandle, L"</style>");

#ifdef PRO_RELEASE
    }
#endif

    FPrint(FileHandle, L"</head>\n");
    FPrint(FileHandle, L"<body>\n");

#ifdef PRO_RELEASE
    // Open the Certificate report template
    MtSupportOpenFile(&HeaderHandle, CERT_HEADER_FILENAME, EFI_FILE_MODE_READ, 0);
    if (HeaderHandle != NULL)
    {
        UINT8 ReadBuf[512];
        UINTN ReadBufSize = sizeof(ReadBuf);

        ReadBufSize = 2;
        // Look for UTF16LE BOM
        if (MtSupportReadFile(HeaderHandle, &ReadBufSize, ReadBuf) == EFI_SUCCESS && ReadBufSize > 0)
        {
            // If not found, reset pointer to beginning of file
            if (CompareMem(ReadBuf, (void *)BOM_UTF16LE, 2) != 0)
                MtSupportSetPosition(HeaderHandle, 0);
        }

        ReadBufSize = sizeof(ReadBuf);
        while ((Status = MtSupportReadFile(HeaderHandle, &ReadBufSize, ReadBuf)) == EFI_SUCCESS && ReadBufSize > 0)
        {
            Status = MtSupportWriteFile(FileHandle, &ReadBufSize, ReadBuf);
        }

        MtSupportCloseFile(HeaderHandle);
    }
    else
    {
#endif
        FPrint(FileHandle, L"<div class=\"header\">\n");
        FPrint(FileHandle, L"<table>\n");
        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td>\n");
        FPrint(FileHandle, L"<img src=\"https://www.memtest86.com/img/mt86.png\">\n");
        FPrint(FileHandle, L"</td>\n");
        FPrint(FileHandle, L"</tr>\n");
        FPrint(FileHandle, L"</table>\n");
        FPrint(FileHandle, L"</div>\n");
#ifdef PRO_RELEASE
    }
#endif
}

VOID
    EFIAPI
    MtSupportWriteReportSysInfo(MT_HANDLE FileHandle)
{
    CHAR16 TextBuf[128];
    CHAR16 TempBuf[128];

    // Write out the heading
    FPrint(FileHandle, L"<h2>%s</h2>\n", GetStringById(STRING_TOKEN(STR_REPORT_SYSINFO), TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"<div class=\"sysinfo\">\n");

    // Customer summary table
    FPrint(FileHandle, L"<table>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_EFI_VERSION), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d.%02d</td>\n", (gRT->Hdr.Revision >> 16) & 0x0000FFFF, gRT->Hdr.Revision & 0x0000FFFF);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_SYSTEM), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\"></td>\n");
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_MANUFACTURER), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gSystemInfo.szManufacturer);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_PRODUCT), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gSystemInfo.szProductName);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_VERSION), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gSystemInfo.szVersion);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_SERIAL_NUM), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gSystemInfo.szSerialNumber);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_BIOS), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\"></td>\n");
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_VENDOR), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gBIOSInfo.szVendor);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_VERSION), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gBIOSInfo.szBIOSVersion);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_RELEASE_DATE), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gBIOSInfo.szBIOSReleaseDate);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_BASEBOARD), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\"></td>\n");
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_MANUFACTURER), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gBaseBoardInfo.szManufacturer);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_PRODUCT), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gBaseBoardInfo.szProductName);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_VERSION), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gBaseBoardInfo.szVersion);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_SERIAL_NUM), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%a</td>\n", gBaseBoardInfo.szSerialNumber);
    FPrint(FileHandle, L"</tr>\n");

    cpu_type(TextBuf, sizeof(TextBuf));

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TYPE), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", TextBuf);
    FPrint(FileHandle, L"</tr>\n");

    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_SPEED), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", TextBuf);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_NUM_CPU), TempBuf, sizeof(TempBuf)));
    if (MPSupportGetNumProcessors() == MPSupportGetNumEnabledProcessors())
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d</td>\n", MPSupportGetNumProcessors());
    else
    {
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_AVAIL), TempBuf, sizeof(TempBuf)), MPSupportGetNumProcessors(), MPSupportGetNumEnabledProcessors());
        FPrint(FileHandle, L"</td>\n");
    }
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L1_CACHE), TempBuf, sizeof(TempBuf)));
    if (gCPUInfo.L1_data_caches_per_package > 0)
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d x %dK (%d MB/s)</td>\n", gCPUInfo.L1_data_caches_per_package, gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
    else
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%dK (%d MB/s)</td>\n", gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L2_CACHE), TempBuf, sizeof(TempBuf)));
    if (gCPUInfo.L2_caches_per_package > 0)
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d x %dK (%d MB/s)</td>\n", gCPUInfo.L2_caches_per_package, gCPUInfo.L2_cache_size, l2_speed);
    else
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%dK (%d MB/s)</td>\n", gCPUInfo.L2_cache_size, l2_speed);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)));
    if (gCPUInfo.L3_cache_size)
    {
        if (gCPUInfo.L3_caches_per_package > 0)
            FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d x %dK (%d MB/s)</td>\n", gCPUInfo.L3_caches_per_package, gCPUInfo.L3_cache_size, l3_speed);
        else
            FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%dK (%d MB/s)</td>\n", gCPUInfo.L3_cache_size, l3_speed);
    }
    else
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"</tr>\n");

    // Ram
    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MEMORY), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%ldM (%d MB/s)</td>\n", MtSupportMemSizeMB(), mem_speed);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_RAM_CONFIG), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", getMemConfigStr(TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_SPD), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d</td>\n", g_numMemModules);
    FPrint(FileHandle, L"</tr>\n");

    for (int i = 0; i < g_numMemModules; i++)
    {
        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">  SPD #%d</td>\n", i);
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", getSPDSummaryLine(&g_MemoryInfo[i], TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</tr>\n");

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_VENDOR_PART_INFO), TempBuf, sizeof(TempBuf)));

        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), g_MemoryInfo[i].jedecManuf);

        if (g_MemoryInfo[i].modulePartNo[0])
        {
            if (TextBuf[0])
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" / ");

            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%a", g_MemoryInfo[i].modulePartNo);
            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
        }

        if (g_MemoryInfo[i].moduleSerialNo)
        {
            if (TextBuf[0])
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" / ");

            if (gSPDReportExtSN)
                UnicodeCatf(TextBuf, sizeof(TextBuf), L"%s", BytesToStrHex(g_MemoryInfo[i].moduleExtSerialNo, sizeof(g_MemoryInfo[i].moduleExtSerialNo), TempBuf, sizeof(TempBuf)));
            else
                UnicodeCatf(TextBuf, sizeof(TextBuf), L"%08X", g_MemoryInfo[i].moduleSerialNo);
        }

        if (gSPDReportByteLo <= gSPDReportByteHi)
        {
            if (TextBuf[0])
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" / ");

            TempBuf[0] = L'\0';
            for (INTN j = gSPDReportByteLo; j <= gSPDReportByteHi; j++)
                UnicodeCatf(TempBuf, sizeof(TempBuf), L"%02X", g_MemoryInfo[i].rawSPDData[j]);

            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
        }

#ifdef PRO_RELEASE
        // Display the channel/slot number as obtained via SMBus
        // Custom request by Virtium
        if (g_MemoryInfo[i].channel >= 0 && g_MemoryInfo[i].slot >= 0)
        {
            if (TextBuf[0])
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" / ");

            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Channel: %d Slot: %d", g_MemoryInfo[i].channel, g_MemoryInfo[i].slot);
            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
        }
#endif

        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", TextBuf);
        FPrint(FileHandle, L"</tr>\n");

        int clkspeed = g_MemoryInfo[i].clkspeed;
        int tCK = g_MemoryInfo[i].tCK;
        int tAA = g_MemoryInfo[i].tAA;
        int tRCD = g_MemoryInfo[i].tRCD;
        int tRP = g_MemoryInfo[i].tRP;
        int tRAS = g_MemoryInfo[i].tRAS;
        CHAR16 *pszVoltage = g_MemoryInfo[i].moduleVoltage;

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">JEDEC Profile</td>\n");
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</tr>\n");

        if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR5)
        {
            if (g_MemoryInfo[i].specific.DDR5SDRAM.XMPSupported)
            {
                for (int j = 0; j < MAX_XMP3_PROFILES; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].enabled)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tAA;
                        tRCD = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].moduleVDD;

                        FPrint(FileHandle, L"<tr>\n");
                        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">XMP Profile %d</td>\n", j + 1);
                        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"</tr>\n");
                    }
                }
            }

            if (g_MemoryInfo[i].specific.DDR5SDRAM.EXPOSupported)
            {
                for (int j = 0; j < MAX_EXPO_PROFILES; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].enabled)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tAA;
                        tRCD = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].moduleVDD;

                        FPrint(FileHandle, L"<tr>\n");
                        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">EXPO Profile %d</td>\n", j + 1);
                        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"</tr>\n");
                    }
                }
            }
        }
        else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR3)
        {
            if (g_MemoryInfo[i].specific.DDR3SDRAM.XMPSupported)
            {
                for (int j = 0; j < 2; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].enabled)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tAA;
                        tRCD = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].moduleVdd;

                        FPrint(FileHandle, L"<tr>\n");
                        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">XMP Profile %d</td>\n", j + 1);
                        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"</tr>\n");
                    }
                }
            }
        }
        else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR4)
        {
            if (g_MemoryInfo[i].specific.DDR4SDRAM.XMPSupported)
            {
                for (int j = 0; j < 2; j++)
                {
                    if (g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].enabled)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tAA;
                        tRCD = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].moduleVdd;

                        FPrint(FileHandle, L"<tr>\n");
                        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">XMP Profile %d</td>\n", j + 1);
                        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"</tr>\n");
                    }
                }
            }
        }
        else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR2)
        {
            if (g_MemoryInfo[i].specific.DDR2SDRAM.EPPSupported)
            {
                if (g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_ABBR)
                {
                    for (int j = 0; j < 4; j++)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].CASSupported * tCK;
                        tRCD = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].voltageLevel;

                        FPrint(FileHandle, L"<tr>\n");
                        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">EPP Abbreviated Profile %d</td>\n", j + 1);
                        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"</tr>\n");
                    }
                }
                else if (g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_FULL)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        clkspeed = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].clkspeed;
                        tCK = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tCK;
                        tAA = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].CASSupported * tCK;
                        tRCD = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRCD;
                        tRP = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRP;
                        tRAS = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRAS;
                        pszVoltage = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].voltageLevel;

                        FPrint(FileHandle, L"<tr>\n");
                        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">EPP Full Profile %d</td>\n", j + 1);
                        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
                               getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"</tr>\n");
                    }
                }
            }
        }
    }

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM_SLOTS), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d</td>\n", g_numSMBIOSMem);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d</td>\n", g_numSMBIOSModules);
    FPrint(FileHandle, L"</tr>\n");

    for (int i = 0; i < g_numSMBIOSMem; i++)
    {
        if (g_SMBIOSMemDevices[i].smb17.Size == 0)
        {
            FPrint(FileHandle, L"<tr>\n");
            FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">  %s</td>\n", MtSupportGetSlotLabel(i, TRUE, TempBuf, sizeof(TempBuf)));
            FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">Empty slot</td>\n");
            FPrint(FileHandle, L"</tr>\n");
            continue;
        }

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">  %s</td>\n", MtSupportGetSlotLabel(i, TRUE, TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", getSMBIOSRAMSummaryLine(&g_SMBIOSMemDevices[i], TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</tr>\n");

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_VENDOR_PART_INFO), TempBuf, sizeof(TempBuf)));
        AsciiStrToUnicodeStrS(g_SMBIOSMemDevices[i].szManufacturer, TextBuf, ARRAY_SIZE(TextBuf));

        if (g_SMBIOSMemDevices[i].szPartNumber[0])
        {
            if (TextBuf[0])
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" / ");

            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%a", g_SMBIOSMemDevices[i].szPartNumber);
            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
        }

        if (g_SMBIOSMemDevices[i].szSerialNumber[0])
        {
            if (TextBuf[0])
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L" / ");

            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%a", g_SMBIOSMemDevices[i].szSerialNumber);
            StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
        }

        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", TextBuf);
        FPrint(FileHandle, L"</tr>\n");

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue-2\">SMBIOS Profile</td>\n");
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n",
               getSMBIOSRAMProfileStr(&g_SMBIOSMemDevices[i], TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</tr>\n");
    }

    // SPDMATCH
    if (gSPDMatch)
    {
        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">SPDMATCH %s</td>\n", GetStringById(STRING_TOKEN(STR_RESULT), TempBuf, sizeof(TempBuf)));
        if (gFailedSPDMatchIdx < 0)
        {
            if (g_numMemModules > 0)
                FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue PASS\">%s</td>\n", GetStringById(STRING_TOKEN(STR_PASS), TempBuf, sizeof(TempBuf)));
            else
                FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
        }
        else
            FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue FAIL\">%s (DIMM %d, SPD byte %d)</td>\n", GetStringById(STRING_TOKEN(STR_FAIL), TempBuf, sizeof(TempBuf)), gFailedSPDMatchIdx, gFailedSPDMatchByte);
        FPrint(FileHandle, L"</tr>\n");
    }

    // Terminate the table
    FPrint(FileHandle, L"</table>\n");

    FPrint(FileHandle, L"</div>\n");
}

VOID
    EFIAPI
    MtSupportWriteReportFooter(MT_HANDLE FileHandle)
{
#ifdef PRO_RELEASE
    EFI_STATUS Status;
    MT_HANDLE FooterHandle = NULL;

    // Copy the remainder of the template to the Certificate file after the restult substitution
    Status = MtSupportOpenFile(&FooterHandle, CERT_FOOTER_FILENAME, EFI_FILE_MODE_READ, 0);
    if (FooterHandle != NULL)
    {
        UINT8 ReadBuf[512];
        UINTN ReadBufSize = sizeof(ReadBuf);

        ReadBufSize = 2;
        // Look for UTF16 BOM
        if (MtSupportReadFile(FooterHandle, &ReadBufSize, ReadBuf) == EFI_SUCCESS && ReadBufSize > 0)
        {
            // If not found, reset pointer to beginning of file
            if (CompareMem(ReadBuf, (void *)BOM_UTF16LE, 2) != 0)
                MtSupportSetPosition(FooterHandle, 0);
        }

        ReadBufSize = sizeof(ReadBuf);
        while ((Status = MtSupportReadFile(FooterHandle, &ReadBufSize, ReadBuf)) == EFI_SUCCESS && ReadBufSize > 0)
        {
            Status = MtSupportWriteFile(FileHandle, &ReadBufSize, ReadBuf);
        }

        MtSupportCloseFile(FooterHandle);
    }
#else
    FPrint(FileHandle, L"<h2>Certification</h2>\n");
    FPrint(FileHandle, L"<div class=\"footer\">\n");
    FPrint(FileHandle, L"<table width=\"640\">\n");
    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td>\n");
    FPrint(FileHandle, L"This document certifies that the Tests described above have been carried out by a suitably qualified technician on the System described above.\n");
    FPrint(FileHandle, L"</td>\n");
    FPrint(FileHandle, L"</tr>\n");
    FPrint(FileHandle, L"</table>\n");

    FPrint(FileHandle, L"Signed<br><br>\n");
    FPrint(FileHandle, L"___________________________<br>\n");
    FPrint(FileHandle, L"<p>\n");
    FPrint(FileHandle, L"Your company name here<br>\n");
    FPrint(FileHandle, L"123 Your Address, City, Country<br>\n");
    FPrint(FileHandle, L"Phone 123-456-7890<br>\n");
    FPrint(FileHandle, L"E-Mail: info@your-company-name.com<br>\n");
    FPrint(FileHandle, L"</p>\n");
    FPrint(FileHandle, L"</div>\n");
#endif

    FPrint(FileHandle, L"</body>\n");
    FPrint(FileHandle, L"</html>\n");
}

VOID
    EFIAPI
    MtSupportSaveHTMLCertificate(MT_HANDLE FileHandle)
{
    EFI_TIME ReportTime;
    CHAR16 TextBuf[256];
    CHAR16 TempBuf[512];
    CHAR16 TempBuf2[64];
    CHAR16 TempBuf3[64];

    int i, n;
    UINTN addr;
    UINTN mb;
    UINT32 hours, mins, secs;
    UINT32 t;
    UINTN Percent;
    UINTN TotalErrCount = MtSupportGetNumErrors() + MtSupportGetNumCorrECCErrors() + MtSupportGetNumUncorrECCErrors();
    UINTN NumRowHammerErr = 0;
    int TestResult = MtSupportGetTestResult();
    int datawidth = sizeof(void *) * 8;

    MtSupportWriteReportHeader(FileHandle);

    // Print result "stamp" on top-right corner
    if (TestResult == RESULT_PASS)
        FPrint(FileHandle, L"<div class=\"passstamp\">PASS</div>\n");
    else if (TestResult == RESULT_FAIL)
        FPrint(FileHandle, L"<div class=\"failstamp\">FAIL</div>\n");

    // Write out the heading
    FPrint(FileHandle, L"<h2>%s</h2>\n", GetStringById(STRING_TOKEN(STR_REPORT_SUMMARY), TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"<div class=\"summary\">\n");

    // Customer summary table
    FPrint(FileHandle, L"<table>\n");

    gRT->GetTime(&ReportTime, NULL);

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_DATE), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\">%d-%02d-%02d %02d:%02d:%02d</td>\n",
           ReportTime.Year, ReportTime.Month, ReportTime.Day, ReportTime.Hour, ReportTime.Minute, ReportTime.Second);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_GENERATED_BY), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\">\n");
    FPrint(FileHandle, L"%s %s (%d-bit)<br />\n", PROGRAM_NAME, PROGRAM_VERSION, sizeof(void *) > 4 ? 64 : 32);
#ifndef PRO_RELEASE // Free version
    FPrint(FileHandle, L"<small><i>%s</i></small>\n", GetStringById(STRING_TOKEN(STR_UPGRADE_PRO), TempBuf, sizeof(TempBuf)));
#endif

    FPrint(FileHandle, L"</td>\n");
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\">%s</td>\n", GetStringById(STRING_TOKEN(STR_RESULT), TempBuf, sizeof(TempBuf)));

    if (TestResult == RESULT_PASS)
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"PASS");
    else if (TestResult == RESULT_INCOMPLETE_PASS || TestResult == RESULT_INCOMPLETE_FAIL)
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"INCOMPLETE");
    else if (TestResult == RESULT_FAIL)
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"FAIL");
    else
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"");
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"%s\">%s</td>\n", TextBuf, MtSupportGetTestResultStr(MtSupportGetTestResult(), gCurLang, TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"</table>\n");

    FPrint(FileHandle, L"</div>\n");

    // Write Sys Info
    MtSupportWriteReportSysInfo(FileHandle);

    // Start a new table
    FPrint(FileHandle, L"<h2>%s</h2>\n", GetStringById(STRING_TOKEN(STR_REPORT_RESULT_SUMMARY), TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"<div class=\"results\">\n");

    FPrint(FileHandle, L"<table>\n");
    FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%d-%02d-%02d %02d:%02d:%02d</td></tr>\n",
           GetStringById(STRING_TOKEN(STR_REPORT_TEST_TIME), TempBuf, sizeof(TempBuf)),
           mTestStartTime.Year, mTestStartTime.Month, mTestStartTime.Day, mTestStartTime.Hour, mTestStartTime.Minute, mTestStartTime.Second);

    t = mElapsedTime;

    secs = t % 60;
    t /= 60;
    mins = t % 60;
    t /= 60;
    hours = t;

    FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%d:%02d:%02d</td></tr>\n",
           GetStringById(STRING_TOKEN(STR_REPORT_ELAPSED_TIME), TempBuf, sizeof(TempBuf)),
           hours, mins, secs);

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_MEM_RANGE), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">0x%lx - %lx (%ldMB)</td>\n", gAddrLimitLo, gAddrLimitHi, DivU64x32(gAddrLimitHi - gAddrLimitLo, SIZE_1MB));
    FPrint(FileHandle, L"</tr>\n");

    if (gCPUSelMode == CPU_SINGLE)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_SINGLE), TempBuf, sizeof(TempBuf)), gSelCPUNo);
    else if (gCPUSelMode == CPU_PARALLEL)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_PARALLEL), TempBuf, sizeof(TempBuf)));
    else if (gCPUSelMode == CPU_RROBIN)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_RROBIN), TempBuf, sizeof(TempBuf)));
    else if (gCPUSelMode == CPU_SEQUENTIAL)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_SEQUENTIAL), TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_CPU_MODE), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", TextBuf);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s %s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TEMP), TempBuf2, sizeof(TempBuf2)), GetStringById(STRING_TOKEN(STR_MIN_MAX_AVE), TempBuf, sizeof(TempBuf)));
    if (mNumCPUTempSamples > 0)
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%dC/%dC/%dC</td>\n", mMinCPUTemp, mMaxCPUTemp, DivS64x64Remainder(mSumCPUTemp, mNumCPUTempSamples, NULL));
    else
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">-/-/-</td>\n");
    FPrint(FileHandle, L"</tr>\n");

    for (i = 0; i < MAX_RAM_TEMP_DIMMS; i++)
    {
        if (mNumRAMTempSamples[i] == 0)
            break;

        if (i == 0)
        {
            FPrint(FileHandle, L"<tr>\n");
            FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s %s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_RAM_TEMP), TempBuf2, sizeof(TempBuf2)), GetStringById(STRING_TOKEN(STR_MIN_MAX_AVE), TempBuf, sizeof(TempBuf)));
            FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\"></td>\n");
            FPrint(FileHandle, L"</tr>\n");
        }

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"subvalue\">  TSOD%d</td>\n", i);
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%dC/%dC/%dC</td>\n", DivS64x64Remainder(mMinRAMTemp[i], 1000, NULL), DivS64x64Remainder(mMaxRAMTemp[i], 1000, NULL), DivS64x64Remainder(mSumRAMTemp[i], MultS64x64(mNumRAMTempSamples[i], 1000), NULL));
        FPrint(FileHandle, L"</tr>\n");
    }

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_LOW_MEM_SPEED), TempBuf, sizeof(TempBuf)));
    if (mCurMemTimings.SpeedMTs > 0)
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d MT/s (%d-%d-%d-%d)</td>\n", mMinMemTimings.SpeedMTs, mMinMemTimings.tCAS, mMinMemTimings.tRCD, mMinMemTimings.tRP, mMinMemTimings.tRAS);
    else
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_HI_MEM_SPEED), TempBuf, sizeof(TempBuf)));
    if (mCurMemTimings.SpeedMTs > 0)
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d MT/s (%d-%d-%d-%d)</td>\n", mMaxMemTimings.SpeedMTs, mMaxMemTimings.tCAS, mMaxMemTimings.tRCD, mMaxMemTimings.tRP, mMaxMemTimings.tRAS);
    else
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"</tr>\n");

    if (gECCPoll)
    {
        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_ECC_POLLING), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_ENABLED), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</tr>\n");
    }

    UINTN TotalTests = gNumPasses * MtSupportNumTests();

    Percent = (UINTN)DivU64x64Remainder(
        MultU64x32(MtSupportTestsCompleted(), 100),
        TotalTests,
        NULL);

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_TESTS_COMPLETED), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\" class=\"altvalue\">%d/%d (%d%%)</td>\n", MtSupportTestsCompleted(), TotalTests, Percent);
    FPrint(FileHandle, L"</tr>\n");

    Percent = MtSupportTestsCompleted() == 0 ? 0 : (UINTN)DivU64x64Remainder(MultU64x32(mPassCount, 100), MtSupportTestsCompleted(), NULL);

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\" class=\"value\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_PASSED), TempBuf, sizeof(TempBuf)));

    if (TestResult == RESULT_PASS)
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"PASS");
    else if (TestResult == RESULT_INCOMPLETE_PASS || TestResult == RESULT_INCOMPLETE_FAIL)
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"INCOMPLETE");
    else if (TestResult == RESULT_FAIL)
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"FAIL");
    else
        StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L"altvalue");

    FPrint(FileHandle, L"<td width=\"65%%\" class=\"%s\">%d/%d (%d%%)</td>\n", TextBuf, mPassCount, MtSupportTestsCompleted(), Percent);

    FPrint(FileHandle, L"</tr>\n");

    // Terminate the 1st table and start the 2nd
    FPrint(FileHandle, L"</table>\n");

    if (MtSupportGetNumErrors() > 0)
    {
        FPrint(FileHandle, L"<table>\n");

        addr = erri.low_addr;
        mb = erri.low_addr >> 20;
        if (erri.low_addr <= erri.high_addr)
            FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">0x%p (%ldMB)</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_LOW_ERROR_ADDR), TempBuf, sizeof(TempBuf)), addr, DivU64x32(addr, SIZE_1MB));
        else
            FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%s</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_LOW_ERROR_ADDR), TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf2, sizeof(TempBuf2)));

        addr = erri.high_addr;
        mb = erri.high_addr >> 20;
        if (erri.low_addr <= erri.high_addr)
            FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">0x%p (%ldMB)</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_HI_ERROR_ADDR), TempBuf, sizeof(TempBuf)), addr, DivU64x32(addr, SIZE_1MB));
        else
            FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%s</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_HI_ERROR_ADDR), TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf2, sizeof(TempBuf2)));

        if (sizeof(void *) > 4)
            FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%016lX</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_BIT_MASK), TempBuf, sizeof(TempBuf)), erri.ebits);
        else
            FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%08X</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_BIT_MASK), TempBuf, sizeof(TempBuf)), erri.ebits);

        /* Calc bits in error */
        for (i = 0, n = 0; i < datawidth; i++)
        {
            if (erri.ebits >> i & 1)
            {
                n++;
            }
        }

        FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%d</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_BITS), TempBuf, sizeof(TempBuf)), n);
        FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%d</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_CONTIGUOUS), TempBuf, sizeof(TempBuf)), erri.maxl);

        TempBuf2[0] = L'\0';
        for (i = 0; i < sizeof(erri.ecpus) / sizeof(erri.ecpus[0]); i++)
        {
            for (n = 0; n < sizeof(erri.ecpus[0]) * 8; n++)
            {
                int cpu = i * sizeof(erri.ecpus[0]) * 8 + n;
                if (erri.ecpus[i] & (LShiftU64(1, cpu)))
                {
                    CHAR16 CPUBuf[8];
                    UnicodeSPrint(CPUBuf, sizeof(CPUBuf), L"%d", cpu);
                    if (TempBuf2[0] != L'\0')
                        StrCatS(TempBuf2, sizeof(TempBuf2) / sizeof(TempBuf2[0]), L", ");
                    StrCatS(TempBuf2, sizeof(TempBuf2) / sizeof(TempBuf2[0]), CPUBuf);
                }
            }
        }
        FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">{ %s }</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_CPUS), TempBuf, sizeof(TempBuf)), TempBuf2);

        // Terminate the table
        FPrint(FileHandle, L"</table>\n");

        FPrint(FileHandle, L"<div>%s</div>\n", GetStringById(STRING_TOKEN(STR_REPORT_ERROR_BITS), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"<table class=\"errormask\">\n");
        FPrint(FileHandle, L"<tr>");
        for (i = datawidth - 1; i >= 0; i--)
        {
            if ((erri.ebits >> i) & 1)
                FPrint(FileHandle, L"<td class=\"biterr\">x</td>");
            else
                FPrint(FileHandle, L"<td class=\"bitok\">.</td>");
        }
        FPrint(FileHandle, L"</tr>\n");
        FPrint(FileHandle, L"<tr class=\"noborder\">");
        for (i = datawidth - 1; i >= 0; i--)
        {
            if (i == datawidth - 1)
                FPrint(FileHandle, L"<td>%d</td>", i);
            else if (i == 0)
                FPrint(FileHandle, L"<td style=\"text-align:right\">%d</td>", i);
            else
                FPrint(FileHandle, L"<td></td>");
        }
        FPrint(FileHandle, L"</tr>\n");
        FPrint(FileHandle, L"</table>\n");

#ifdef PRO_RELEASE
        FPrint(FileHandle, L"<table>\n");

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"40%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_DIMM_CHIP), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"<td width=\"20%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_SERIAL_NUM), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"<td width=\"10%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_SPD_NUM_RANKS), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"<td width=\"10%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_ERRORS), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"<td width=\"20%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_RESULT), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</tr>\n");

        for (i = 0; i < g_numSMBIOSMem; i++)
        {
            if (g_SMBIOSMemDevices[i].smb17.Size != 0)
            {
                int DimmResult = MtSupportGetDimmTestResult(i);
                int SlotErrs = MtSupportGetNumSlotErrors(i);
                int ChipWidth = MtSupportGetChipWidth(i);
                int NumRanks = MtSupportGetNumRanks(i);

                if (SlotErrs > 0)
                    FPrint(FileHandle, L"<tr class=\"fail\">\n");
                else
                    FPrint(FileHandle, L"<tr>\n");

                FPrint(FileHandle, L"<td class=\"value\">%s</td>\n", MtSupportGetSlotLabel(i, TRUE, TempBuf, sizeof(TempBuf)));
                FPrint(FileHandle, L"<td>%a</td>\n", g_SMBIOSMemDevices[i].szSerialNumber);
                FPrint(FileHandle, L"<td>%dRx%d</td>\n", NumRanks, ChipWidth);
                FPrint(FileHandle, L"<td>%d</td>\n", SlotErrs);
                FPrint(FileHandle, L"<td>%s</td>\n", MtSupportGetTestResultStr(DimmResult, gCurLang, TempBuf, sizeof(TempBuf)));
                FPrint(FileHandle, L"</tr>\n");

#ifdef SITE_EDITION
                int NumChips = NumRanks * 64 / ChipWidth;

                UINT32 TotalChipErrs = 0;
                for (int chip = 0; chip < NumChips; chip++)
                {
                    int ChipErrs = MtSupportGetNumSlotChipErrors(i, chip);
                    if (ChipErrs > 0)
                    {
                        CHAR16 ChipStr[8];
                        FPrint(FileHandle, L"<tr><td colspan=3 style=\"padding-left:15px;\">%s</td><td colspan=2>%d</td></tr>\n", MtSupportGetChipLabel(chip, ChipStr, sizeof(ChipStr)), ChipErrs);
                        TotalChipErrs += ChipErrs;
                    }
                }
                INT32 UnknownErrs = SlotErrs - TotalChipErrs;
                if (UnknownErrs > 0)
                {
                    FPrint(FileHandle, L"<tr><td colspan=3 style=\"padding-left:15px; font-style: italic;\">%s</td><td colspan=2>%d</td></tr>\n", GetStringById(STRING_TOKEN(STR_MENU_UNKNOWN), TempBuf, sizeof(TempBuf)), UnknownErrs);
                }
#endif
            }
        }

        if (MtSupportGetNumUnknownSlotErrors() > 0)
        {
            FPrint(FileHandle, L"<tr>\n");
            FPrint(FileHandle, L"<td class=\"value\" style=\"font-style: italic;\">%s</td>\n", GetStringById(STRING_TOKEN(STR_MENU_UNKNOWN), TempBuf, sizeof(TempBuf)));
            FPrint(FileHandle, L"<td></td>\n");
            FPrint(FileHandle, L"<td></td>\n");
            FPrint(FileHandle, L"<td>%d</td>\n", MtSupportGetNumUnknownSlotErrors());
            FPrint(FileHandle, L"<td></td>\n");
            FPrint(FileHandle, L"</tr>\n");
        }
        // Terminate the table
        FPrint(FileHandle, L"</table>\n");
#endif
    }

    if (MtSupportGetNumCorrECCErrors() > 0 || MtSupportGetNumUncorrECCErrors() > 0)
    {
        FPrint(FileHandle, L"<table>\n");

        FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%d</td></tr>\n", GetStringById(STRING_TOKEN(STR_ECC_CORRECTABLE), TempBuf, sizeof(TempBuf)), MtSupportGetNumCorrECCErrors());
        FPrint(FileHandle, L"<tr><td width=\"35%%\" class=\"value\">%s</td><td width=\"65%%\" class=\"altvalue\">%d</td></tr>\n", GetStringById(STRING_TOKEN(STR_ECC_UNCORRECTABLE), TempBuf, sizeof(TempBuf)), MtSupportGetNumUncorrECCErrors());

        // Terminate the table
        FPrint(FileHandle, L"</table>\n");
    }

    // Start results table
    FPrint(FileHandle, L"<table>\n");

    // Table 2 headings
    FPrint(FileHandle, L"<tr>\n");

    FPrint(FileHandle, L"<td width=\"60%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_TEST), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"20%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_PASSED), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"20%%\" class=\"header\">%s</td>\n", GetStringById(STRING_TOKEN(STR_ERRORS), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"</tr>\n");

    if (gUsingCustomTests)
        FPrint(FileHandle, L"<i>%s</i>\n", GetStringById(STRING_TOKEN(STR_REPORT_CUSTOM_TESTS), TempBuf, sizeof(TempBuf)));

    for (i = 0; i < (int)mTestCount; i++)
    {
        Percent = mTests[i].NumCompleted == 0 ? 0 : (UINTN)DivU64x64Remainder(MultU64x32(mTests[i].NumPassed, 100), mTests[i].NumCompleted, NULL);

        if (mTests[i].NumPassed < mTests[i].NumCompleted)
            FPrint(FileHandle, L"<tr class=\"fail\">\n");
        else
            FPrint(FileHandle, L"<tr>\n");

        FPrint(FileHandle, L"<td width=\"60%%\" class=\"value\">");
        if (gUsingCustomTests)
            FPrint(FileHandle, L"<i>");
        FPrint(FileHandle, L"%s", mTests[i].NameStr ? mTests[i].NameStr : GetStringById(mTests[i].NameStrId, TempBuf, sizeof(TempBuf)));
        if (gUsingCustomTests)
            FPrint(FileHandle, L"</i>");
        FPrint(FileHandle, L"</td>\n");

        FPrint(FileHandle, L"<td width=\"20%%\" class=\"altvalue\">%d/%d (%d%%)</td>\n", mTests[i].NumPassed, mTests[i].NumCompleted, Percent);

        FPrint(FileHandle, L"<td width=\"20%%\" class=\"altvalue\">%d</td>\n", mTests[i].NumErrors);
        FPrint(FileHandle, L"</tr>\n");

        // Save the number of row hammer errors to determine whether or not we show the warning
        if (mTests[i].NameStrId == STRING_TOKEN(STR_TEST_13_NAME))
            NumRowHammerErr = mTests[i].NumErrors;
    }

    // End the table
    FPrint(FileHandle, L"</table>\n");

    if (TotalErrCount > 0)
    {
        UINT32 BufIdx = mErrorBufTail > 0 ? mErrorBufTail - 1 : mErrorBufSize - 1;
        UINTN logCount = 0;

        // Start errors table
        FPrint(FileHandle, L"<table>\n");
        FPrint(FileHandle, L"<tr><td class=\"header\">");
        FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_REPORT_PREVIOUS_ERRORS), TempBuf, sizeof(TempBuf)), gReportNumErr);
        FPrint(FileHandle, L"</td></tr>\n");

        while (mErrorBuf[BufIdx].Valid && logCount < gReportNumErr)
        {
            int ch = -1, sl = -1, chip = -1;
            CHAR16 ChBuf[32];
            CHAR16 SNBuf[64];
            SNBuf[0] = L'\0';

            GetErrorSlotStr(&mErrorBuf[BufIdx], &ch, &sl, &chip, ChBuf, sizeof(ChBuf));

            if (mErrorBuf[BufIdx].Type == 2)
            {
                CHAR16 SynBuf[16];

                if (mErrorBuf[BufIdx].ErrInfo.ECCErr.chan >= 0 && mErrorBuf[BufIdx].ErrInfo.ECCErr.dimm >= 0)
                {
#ifdef PRO_RELEASE
                    for (i = 0; i < g_numMemModules; i++)
                    {
                        if (g_MemoryInfo[i].channel == mErrorBuf[BufIdx].ErrInfo.ECCErr.chan &&
                            g_MemoryInfo[i].slot == mErrorBuf[BufIdx].ErrInfo.ECCErr.dimm)
                        {
                            UnicodeSPrint(SNBuf, sizeof(SNBuf), L" (S/N: %08x)", g_MemoryInfo[i].moduleSerialNo);
                            break;
                        }
                    }
#endif
                }

                GetECCSyndStr(&mErrorBuf[BufIdx], SynBuf, sizeof(SynBuf));

                /* ECC error */
                if (mErrorBuf[BufIdx].AddrType == 0)
                {
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_ECC_ERRORS_INFO_PHYSADDR), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  GetECCTypeString(mErrorBuf[BufIdx].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)),
                                  mErrorBuf[BufIdx].TestNo, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, GetStringById(mErrorBuf[BufIdx].ErrInfo.ECCErr.corrected ? STRING_TOKEN(STR_MENU_YES) : STRING_TOKEN(STR_MENU_NO), TempBuf3, sizeof(TempBuf3)), SynBuf, ChBuf);
                    StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), SNBuf);
                }
                else
                {
                    CHAR16 AddrBuf[32];
                    GetDimmAddrStr(&mErrorBuf[BufIdx], AddrBuf, sizeof(AddrBuf));

                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_ECC_ERRORS_INFO_DIMM), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  GetECCTypeString(mErrorBuf[BufIdx].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)),
                                  mErrorBuf[BufIdx].TestNo, AddrBuf, GetStringById(mErrorBuf[BufIdx].ErrInfo.ECCErr.corrected ? STRING_TOKEN(STR_MENU_YES) : STRING_TOKEN(STR_MENU_NO), TempBuf3, sizeof(TempBuf3)), SynBuf, ChBuf);
                    StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), SNBuf);
                }
            }
            else if (mErrorBuf[BufIdx].Type == 1)
            {
                UnicodeSPrint(TextBuf, sizeof(TextBuf), L"[Parity Error] Test: %d, CPU: %d, Address: %lx", mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr);
            }
            else
            {

                switch (mErrorBuf[BufIdx].ErrDataSize)
                {
                case 4:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_32BIT), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT32 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT32 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 8:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_64BIT), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT64 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT32 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 16:
                {
                    UINT64 Actual[2];
                    UINT64 Expected[2];
                    m128i_to_u64_array(&mErrorBuf[BufIdx].ErrInfo.DataErr.Actual, Actual);
                    m128i_to_u64_array(&mErrorBuf[BufIdx].ErrInfo.DataErr.Expected, Expected);

                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_128BIT), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, Expected[0], Expected[1], Actual[0], Actual[1]);
                    break;
                }
                }
#ifdef PRO_RELEASE
                if (ch >= 0 && sl >= 0)
                {
                    int smbind = MC2SMBIOSSlot(ch, sl);
                    if (smbind < g_numSMBIOSMem)
                    {
                        if (g_SMBIOSMemDevices[smbind].smb17.Size != 0)
                            UnicodeSPrint(SNBuf, sizeof(SNBuf), L" (S/N: %a)", g_SMBIOSMemDevices[smbind].szSerialNumber);
                    }
                }
#endif
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), SNBuf);
            }
            FPrint(FileHandle, L"<tr><td class=\"value\">%s</td></tr>\n", TextBuf);

            BufIdx = BufIdx > 0 ? BufIdx - 1 : mErrorBufSize - 1;
            logCount++;
        }
        // End the table
        FPrint(FileHandle, L"</table>\n");
    }

    // If errors were detected in the "fast" pass but not the "slow" pass of Test 13, display warning
    if (erri.data_warn > 0 && NumRowHammerErr == 0)
    {
        // Start warning table
        FPrint(FileHandle, L"<table>\n");
        FPrint(FileHandle, L"<tr><td class=\"header\">");
        FPrint(FileHandle, L"%s %s", GetStringById(STRING_TOKEN(STR_TEST_13_NAME), TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_REPORT_WARNING), TempBuf2, sizeof(TempBuf2)));
        FPrint(FileHandle, L"</td></tr>\n");
        FPrint(FileHandle, L"<tr><td class=\"value\">%s</td></tr>\n", GetStringById(STRING_TOKEN(STR_REPORT_ROW_HAMMER_WARNING), TempBuf, sizeof(TempBuf)));
        // End the table
        FPrint(FileHandle, L"</table>\n");

        // If the REPORTNUMWARN config file parameter was specified, displayed the details of the errors detected in the "fast" pass
        if (gReportNumWarn > 0)
        {
            UINT32 BufIdx = mWarningBufTail > 0 ? mWarningBufTail - 1 : mWarningBufSize - 1;
            UINTN logCount = 0;

            // Start errors table
            FPrint(FileHandle, L"<table>\n");
            FPrint(FileHandle, L"<tr><td class=\"header\">");
            FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_TEST_13_NAME), TempBuf, sizeof(TempBuf)));
            FPrint(FileHandle, L" - ");
            FPrint(FileHandle, GetStringById(STRING_TOKEN(STR_REPORT_PREVIOUS_WARNINGS), TempBuf, sizeof(TempBuf)), gReportNumWarn);
            FPrint(FileHandle, L"</td></tr>\n");

            while (mWarningBuf[BufIdx].Valid && logCount < gReportNumWarn)
            {
                CHAR16 ChBuf[32];
                SetMem(ChBuf, sizeof(ChBuf), 0);

                GetErrorSlotStr(&mWarningBuf[BufIdx], NULL, NULL, NULL, ChBuf, sizeof(ChBuf));

                switch (mWarningBuf[BufIdx].ErrDataSize)
                {
                case 4:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_32BIT), TempBuf, sizeof(TempBuf)),
                                  mWarningBuf[BufIdx].Time.Year, mWarningBuf[BufIdx].Time.Month, mWarningBuf[BufIdx].Time.Day, mWarningBuf[BufIdx].Time.Hour, mWarningBuf[BufIdx].Time.Minute, mWarningBuf[BufIdx].Time.Second,
                                  mWarningBuf[BufIdx].TestNo, mWarningBuf[BufIdx].ProcNum, (UINT64)mWarningBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT32 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT32 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 8:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_64BIT), TempBuf, sizeof(TempBuf)),
                                  mWarningBuf[BufIdx].Time.Year, mWarningBuf[BufIdx].Time.Month, mWarningBuf[BufIdx].Time.Day, mWarningBuf[BufIdx].Time.Hour, mWarningBuf[BufIdx].Time.Minute, mWarningBuf[BufIdx].Time.Second,
                                  mWarningBuf[BufIdx].TestNo, mWarningBuf[BufIdx].ProcNum, (UINT64)mWarningBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT64 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT64 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 16:
                {
                    UINT64 Actual[2];
                    UINT64 Expected[2];
                    m128i_to_u64_array(&mWarningBuf[BufIdx].ErrInfo.DataErr.Actual, Actual);
                    m128i_to_u64_array(&mWarningBuf[BufIdx].ErrInfo.DataErr.Expected, Expected);

                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_128BIT), TempBuf, sizeof(TempBuf)),
                                  mWarningBuf[BufIdx].Time.Year, mWarningBuf[BufIdx].Time.Month, mWarningBuf[BufIdx].Time.Day, mWarningBuf[BufIdx].Time.Hour, mWarningBuf[BufIdx].Time.Minute, mWarningBuf[BufIdx].Time.Second,
                                  mWarningBuf[BufIdx].TestNo, mWarningBuf[BufIdx].ProcNum, (UINT64)mWarningBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, Expected[0], Expected[1], Actual[0], Actual[1]);
                    break;
                }
                }
                FPrint(FileHandle, L"<tr><td class=\"value\">%s</td></tr>\n", TextBuf);

                BufIdx = BufIdx > 0 ? BufIdx - 1 : mWarningBufSize - 1;
                logCount++;
            }
            // End the table
            FPrint(FileHandle, L"</table>\n");
        }
    }

#ifdef PRO_RELEASE
    if (MtSupportGetNumErrors() > 0)
    {
        FPrint(FileHandle, L"<table>\n");
        FPrint(FileHandle, L"<tr><td class=\"header\">");
        FPrint(FileHandle, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_BADRAM), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</td></tr>\n");
        FPrint(FileHandle, L"<tr><td class=\"value\">\n");
        FPrint(FileHandle, L" <p>%s</p>\n", GetStringById(STRING_TOKEN(STR_REPORT_BADRAM_INSTR), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L" <ol>\n");
        FPrint(FileHandle, L"  <li>%s</li>\n", GetStringById(STRING_TOKEN(STR_REPORT_BADRAM_STEP1), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  <code style=\"white-space: pre-wrap; word-break: break-all;\">GRUB_BADRAM=\"%s\"</code>\n", get_badram_string(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0])));
        FPrint(FileHandle, L"  <li>%s</li>\n", GetStringById(STRING_TOKEN(STR_REPORT_BADRAM_STEP2), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  <code>sudo update-grub</code>\n");
        FPrint(FileHandle, L" </ol>\n");
        FPrint(FileHandle, L"<i>%s</i>\n", GetStringById(STRING_TOKEN(STR_REPORT_DISCLAIMER), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</td></tr>\n");
        FPrint(FileHandle, L"</table>\n");

        FPrint(FileHandle, L"<table>\n");
        FPrint(FileHandle, L"<tr><td class=\"header\">");
        FPrint(FileHandle, L"%s", GetStringById(STRING_TOKEN(STR_REPORT_BADMEMORYLIST), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</td></tr>\n");
        FPrint(FileHandle, L"<tr><td class=\"value\">\n");
        FPrint(FileHandle, L" <p>%s</p>\n", GetStringById(STRING_TOKEN(STR_REPORT_BADMEMORYLIST_INSTR), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L" <ol>\n");
        FPrint(FileHandle, L"  <li>%s</li>\n", GetStringById(STRING_TOKEN(STR_REPORT_BADMEMORYLIST_STEP1), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  <li>%s</li>\n", GetStringById(STRING_TOKEN(STR_REPORT_BADMEMORYLIST_STEP2), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  <code>bcdedit /set {badmemory} badmemoryaccess no</code>\n");
        FPrint(FileHandle, L"  <li>%s</li>\n", GetStringById(STRING_TOKEN(STR_REPORT_BADMEMORYLIST_STEP3), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"  <code style=\"white-space: pre-wrap; word-break: break-all;\">bcdedit /set {badmemory} badmemorylist %s</code>\n", get_badaddresslist_string(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0])));
        FPrint(FileHandle, L" </ol>\n");
        FPrint(FileHandle, L"<i>%s</i>\n", GetStringById(STRING_TOKEN(STR_REPORT_DISCLAIMER), TempBuf, sizeof(TempBuf)));
        FPrint(FileHandle, L"</td></tr>\n");
        FPrint(FileHandle, L"</table>\n");
    }
#endif
    FPrint(FileHandle, L"</div>\n");

    MtSupportWriteReportFooter(FileHandle);
}

EFI_STATUS
EFIAPI
MtSupportSaveBinaryReport(MT_HANDLE FileHandle)
{
    EFI_STATUS Ret = EFI_SUCCESS;
    TESTRESULT *tr;
    int i;

    UINTN ReportSize = OFFSET_OF(TESTRESULT, AllTests) +
                       FIELD_SIZEOF(TESTRESULT, AllTests) *
                           MtSupportNumTests() +
                       FIELD_SIZEOF(TESTRESULT, SlotChipErrInfo.SlotChipErrsArrCount) +
                       FIELD_SIZEOF(TESTRESULT, SlotChipErrInfo.SlotChipErrs) *
                           (MAX_SLOTS * (MAX_CHIPS + 1) + 1);
    tr = (TESTRESULT *)AllocateZeroPool(ReportSize);
    if (tr == NULL)
        return EFI_BAD_BUFFER_SIZE;

    CopyMem(tr->Signature, REPORT_BIN_SIG, sizeof(tr->Signature));
    tr->Revision = (REPORT_BIN_MAJOR_REV << 8) | REPORT_BIN_MINOR_REV;
    tr->StartTime = efi_time(&mTestStartTime);
    tr->ElapsedTime = mElapsedTime;
    tr->RangeMin = gAddrLimitLo;
    tr->RangeMax = gAddrLimitHi;
    tr->CPUSelMode = (UINT16)MtSupportCPUSelMode();
    tr->CPUTempMin = mNumCPUTempSamples > 0 ? (INT16)mMinCPUTemp : -1;
    tr->CPUTempMax = mNumCPUTempSamples > 0 ? (INT16)mMaxCPUTemp : -1;
    tr->CPUTempAve = mNumCPUTempSamples > 0 ? (INT16)DivS64x64Remainder(mSumCPUTemp, mNumCPUTempSamples, NULL) : -1;
    tr->RAMTempMin = mNumRAMTempSamples[0] > 0 ? (INT16)DivS64x64Remainder(mMinRAMTemp[0], 1000, NULL) : -1;
    tr->RAMTempMax = mNumRAMTempSamples[0] > 0 ? (INT16)DivS64x64Remainder(mMaxRAMTemp[0], 1000, NULL) : -1;
    tr->RAMTempAve = mNumRAMTempSamples[0] > 0 ? (INT16)DivS64x64Remainder(mSumRAMTemp[0], MultS64x64(mNumRAMTempSamples[0], 1000), NULL) : -1;

    tr->ECCSupport = MtSupportECCEnabled();
    tr->TestResult = (INT8)MtSupportGetTestResult();
    tr->ErrorCode = EFI_SUCCESS; // todo
    tr->NumErrors = (UINT32)MtSupportGetNumErrors();
    tr->MinErrorAddr = erri.low_addr;
    tr->MaxErrorAddr = erri.high_addr;
    tr->ErrorBits = erri.ebits;
    tr->NumCorrECCErrors = MtSupportGetNumCorrECCErrors();
    tr->NumUncorrECCErrors = MtSupportGetNumUncorrECCErrors();
    tr->NumTestsEnabled = (UINT16)MtSupportNumTests();
    for (i = 0; i < tr->NumTestsEnabled; i++)
    {
        tr->AllTests[i].TestNo = (UINT8)mTests[i].TestNo;
        tr->AllTests[i].NumTestsPassed = (UINT16)mTests[i].NumPassed;
        tr->AllTests[i].NumTestsCompleted = (UINT16)mTests[i].NumCompleted;
        tr->AllTests[i].NumErrors = (UINT32)mTests[i].NumErrors;
    }

    struct SLOTCHIP_ERRINFO *scei = (struct SLOTCHIP_ERRINFO *)((UINT8 *)tr->AllTests + sizeof(tr->AllTests[0]) * tr->NumTestsEnabled);

    for (i = 0; i < g_numSMBIOSMem; i++)
    {
        UINT32 SlotErrs = MtSupportGetNumSlotErrors(i);
        if (SlotErrs > 0)
        {
            UINT32 TotalChipErrs = 0;
#ifdef SITE_EDITION
            int ChipWidth = MtSupportGetChipWidth(i);
            int NumRanks = MtSupportGetNumRanks(i);
            int NumChips = NumRanks * 64 / ChipWidth;

            for (int chip = 0; chip < NumChips; chip++)
            {
                int ChipErrs = MtSupportGetNumSlotChipErrors(i, chip);
                if (ChipErrs > 0)
                {
                    scei->SlotChipErrs[scei->SlotChipErrsArrCount].Slot = (INT8)i;
                    scei->SlotChipErrs[scei->SlotChipErrsArrCount].Chip = (INT8)chip;
                    scei->SlotChipErrs[scei->SlotChipErrsArrCount].NumErrors = ChipErrs;

                    scei->SlotChipErrsArrCount++;

                    TotalChipErrs += ChipErrs;
                }
            }
#endif
            INT32 UnknownErrs = SlotErrs - TotalChipErrs;
            if (UnknownErrs > 0)
            {
                scei->SlotChipErrs[scei->SlotChipErrsArrCount].Slot = (INT8)i;
                scei->SlotChipErrs[scei->SlotChipErrsArrCount].Chip = -1;
                scei->SlotChipErrs[scei->SlotChipErrsArrCount].NumErrors = UnknownErrs;
                scei->SlotChipErrsArrCount++;
            }
        }
    }
    if (MtSupportGetNumUnknownSlotErrors() > 0)
    {
        scei->SlotChipErrs[scei->SlotChipErrsArrCount].Slot = -1;
        scei->SlotChipErrs[scei->SlotChipErrsArrCount].Chip = -1;
        scei->SlotChipErrs[scei->SlotChipErrsArrCount].NumErrors = MtSupportGetNumUnknownSlotErrors();
        scei->SlotChipErrsArrCount++;
    }

    ReportSize = OFFSET_OF(TESTRESULT, AllTests) +
                 FIELD_SIZEOF(TESTRESULT, AllTests) *
                     MtSupportNumTests() +
                 FIELD_SIZEOF(TESTRESULT, SlotChipErrInfo.SlotChipErrsArrCount) +
                 FIELD_SIZEOF(TESTRESULT, SlotChipErrInfo.SlotChipErrs) *
                     scei->SlotChipErrsArrCount;

    Ret = MtSupportWriteFile(FileHandle, &ReportSize, (void *)tr);

    FreePool(tr);
    return Ret;
}

VOID
    EFIAPI
    MtSupportPMPHeader(CHAR8 *Buffer, UINTN BufferSize, CHAR16 *MessageId)
{
    AsciiCatf(Buffer, BufferSize, "<Header>\n");
    AsciiCatf(Buffer, BufferSize, "<MessageID>%s</MessageID>\n", MessageId);
    AsciiCatf(Buffer, BufferSize, "<MachineID>%02x-%02x-%02x-%02x-%02x-%02x</MachineID>\n", ClientMAC.Addr[0], ClientMAC.Addr[1], ClientMAC.Addr[2], ClientMAC.Addr[3], ClientMAC.Addr[4], ClientMAC.Addr[5]);
    AsciiCatf(Buffer, BufferSize, "<AppName>%s</AppName>\n", PROGRAM_NAME);
    AsciiCatf(Buffer, BufferSize, "<AppVer>%s Build: %s (%s)</AppVer>\n", PROGRAM_VERSION, PROGRAM_BUILD, sizeof(void *) > 4 ? L"64-bit" : L"32-bit");
    AsciiCatf(Buffer, BufferSize, "</Header>\n");
}

VOID
    EFIAPI
    MtSupportPMPSysInfo(CHAR8 *Buffer, UINTN BufferSize)
{
    CHAR16 TextBuf[128];
    CHAR8 TempBuf[128];
    CHAR8 TempBuf2[128];

    AsciiCatf(Buffer, BufferSize, "<SysInfoSum name=\"System summary\">\n");
    AsciiCatf(Buffer, BufferSize, "<OS name=\"Operating System\">UEFI %d.%02d (%s)</OS>\n", (gRT->Hdr.Revision >> 16) & 0x0000FFFF, gRT->Hdr.Revision & 0x0000FFFF, sizeof(void *) > 4 ? L"64-bit" : L"32-bit");
    cpu_type(TextBuf, sizeof(TextBuf));
    UnicodeStrToAsciiStrS(TextBuf, TempBuf, ARRAY_SIZE(TempBuf));
    ConvertStringToEntities(TempBuf, TempBuf2, sizeof(TempBuf2));
    AsciiCatf(Buffer, BufferSize, "<CPU name=\"CPU\">%a</CPU>\n", TempBuf2);
    AsciiCatf(Buffer, BufferSize, "<Memory name=\"Memory\">%ldMB RAM</Memory>\n", MtSupportMemSizeMB());
    AsciiCatf(Buffer, BufferSize, "</SysInfoSum>\n");

    AsciiCatf(Buffer, BufferSize, "<General name=\"General\">\n");
    ConvertStringToEntities(gSystemInfo.szProductName, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<SysModel name=\"System Model\">%a</SysModel>\n", TempBuf);
    ConvertStringToEntities(gBaseBoardInfo.szManufacturer, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<MBManuf name=\"Motherboard Manufacturer\">%a</MBManuf>\n", TempBuf);
    ConvertStringToEntities(gBaseBoardInfo.szProductName, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<MBName name=\"Motherboard Name\">%a</MBName>\n", TempBuf);
    ConvertStringToEntities(gBaseBoardInfo.szVersion, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<MBVersion name=\"Motherboard Version\">%a</MBVersion>\n", TempBuf);
    ConvertStringToEntities(gBIOSInfo.szVendor, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<BIOSManuf name=\"BIOS Manufacturer\">%a</BIOSManuf>\n", TempBuf);
    ConvertStringToEntities(gBIOSInfo.szBIOSVersion, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<BIOSVer name=\"BIOS Version\">%a</BIOSVer>\n", TempBuf);
    ConvertStringToEntities(gBIOSInfo.szBIOSReleaseDate, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<BIOSDate name=\"BIOS Release Date\">%a</BIOSDate>\n", TempBuf);
    AsciiCatf(Buffer, BufferSize, "</General>\n");

    AsciiCatf(Buffer, BufferSize, "<Details>\n");
    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>Motherboard Serial Number</name>\n");
    ConvertStringToEntities(gBaseBoardInfo.szSerialNumber, TempBuf, sizeof(TempBuf));
    AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>CPU Clock</name>\n");
    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));
    AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n", TextBuf);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name># Logical Processors</name>\n");
    if (MPSupportGetNumProcessors() == MPSupportGetNumEnabledProcessors())
        AsciiCatf(Buffer, BufferSize, "<value>%d</value>\n", MPSupportGetNumProcessors());
    else
    {
        UnicodeStrToAsciiStrS(GetStringFromPkg(0, STRING_TOKEN(STR_MENU_SYSINFO_CPU_AVAIL), TextBuf, sizeof(TextBuf)), TempBuf, ARRAY_SIZE(TempBuf));
        AsciiSPrint(TempBuf2, sizeof(TempBuf2), TempBuf, MPSupportGetNumProcessors(), MPSupportGetNumEnabledProcessors());
        AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf2);
    }
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>L1 Cache</name>\n");
    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));
    if (gCPUInfo.L1_data_caches_per_package > 0)
        AsciiCatf(Buffer, BufferSize, "<value>%d x %dK (%d MB/s)</value>\n", gCPUInfo.L1_data_caches_per_package, gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
    else
        AsciiCatf(Buffer, BufferSize, "<value>%dK (%d MB/s)</value>\n", gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>L2 Cache</name>\n");
    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));
    if (gCPUInfo.L1_data_caches_per_package > 0)
        AsciiCatf(Buffer, BufferSize, "<value>%d x %dK (%d MB/s)</value>\n", gCPUInfo.L2_caches_per_package, gCPUInfo.L2_cache_size, l2_speed);
    else
        AsciiCatf(Buffer, BufferSize, "<value>%dK (%d MB/s)</value>\n", gCPUInfo.L2_cache_size, l2_speed);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>L3 Cache</name>\n");
    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));
    if (gCPUInfo.L3_cache_size > 0)
    {
        if (gCPUInfo.L3_caches_per_package > 0)
            AsciiCatf(Buffer, BufferSize, "<value>%d x %dK (%d MB/s)</value>\n", gCPUInfo.L3_caches_per_package, gCPUInfo.L3_cache_size, l3_speed);
        else
            AsciiCatf(Buffer, BufferSize, "<value>%dK (%d MB/s)</value>\n", gCPUInfo.L3_cache_size, l3_speed);
    }
    else
        AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n", GetStringFromPkg(0, STRING_TOKEN(STR_MENU_NA), TextBuf, sizeof(TextBuf)));
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>Memory</name>\n");
    AsciiCatf(Buffer, BufferSize, "<value>%ldM (%d MB/s)</value>\n", MtSupportMemSizeMB(), mem_speed);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>RAM Configuration</name>\n");
    AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n", getMemConfigStr(TextBuf, sizeof(TextBuf)));
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>Number of RAM SPDs detected</name>\n");
    AsciiCatf(Buffer, BufferSize, "<value>%d</value>\n", g_numMemModules);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    if (g_numMemModules > 0)
    {
        for (int i = 0; i < g_numMemModules; i++)
        {
            CHAR16 szwRAMSummary[128];
            getSPDSummaryLine(&g_MemoryInfo[i], szwRAMSummary, sizeof(szwRAMSummary));
            UnicodeStrToAsciiStrS(szwRAMSummary, TempBuf, ARRAY_SIZE(TempBuf));
            ConvertStringToEntities(TempBuf, TempBuf2, sizeof(TempBuf2));

            AsciiCatf(Buffer, BufferSize, "<detail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>SPD #%d</name>\n", i);
            AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf2);

            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>JEDEC Manufacturer</name>\n");
            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n", g_MemoryInfo[i].jedecManuf);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Module Serial #</name>\n");
            if (gSPDReportExtSN)
                AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n", BytesToStrHex(g_MemoryInfo[i].moduleExtSerialNo, sizeof(g_MemoryInfo[i].moduleExtSerialNo), TextBuf, sizeof(TextBuf)));
            else
                AsciiCatf(Buffer, BufferSize, "<value>%08X</value>\n", g_MemoryInfo[i].moduleSerialNo);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Module Part #</name>\n");
            ConvertStringToEntities(g_MemoryInfo[i].modulePartNo, TempBuf, sizeof(TempBuf));
            AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Manufacture Date</name>\n");
            if (g_MemoryInfo[i].week)
                AsciiCatf(Buffer, BufferSize, "<value>Week %d of Year %d</value>\n", g_MemoryInfo[i].week, g_MemoryInfo[i].year);
            else if (g_MemoryInfo[i].year)
                AsciiCatf(Buffer, BufferSize, "<value>Year %d</value>\n", g_MemoryInfo[i].year);
            else
                AsciiCatf(Buffer, BufferSize, "<value>Unknown</value>\n", g_MemoryInfo[i].week, g_MemoryInfo[i].year);

            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Memory Capacity (MB)</name>\n");
            AsciiCatf(Buffer, BufferSize, "<value>%d</value>\n", g_MemoryInfo[i].size);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Module Manufacturer's Specific Data</name>\n");
            AsciiCatf(Buffer, BufferSize, "<value>");
            for (int j = 0; j < g_MemoryInfo[i].manufDataLen; j++)
            {
                int ind = g_MemoryInfo[i].manufDataOff + j;
                AsciiCatf(Buffer, BufferSize, "%02X", g_MemoryInfo[i].rawSPDData[ind]);
            }
            AsciiCatf(Buffer, BufferSize, "</value>\n");
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");

            int clkspeed = g_MemoryInfo[i].clkspeed;
            int tCK = g_MemoryInfo[i].tCK;
            int tAA = g_MemoryInfo[i].tAA;
            int tRCD = g_MemoryInfo[i].tRCD;
            int tRP = g_MemoryInfo[i].tRP;
            int tRAS = g_MemoryInfo[i].tRAS;
            CHAR16 *pszVoltage = g_MemoryInfo[i].moduleVoltage;

            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Profiles</name>\n");

            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>JEDEC Profile</name>\n");
            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                      getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TextBuf, sizeof(TextBuf)));
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");

            if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR5)
            {
                if (g_MemoryInfo[i].specific.DDR5SDRAM.XMPSupported)
                {
                    for (int j = 0; j < MAX_XMP3_PROFILES; j++)
                    {
                        if (g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].enabled)
                        {
                            clkspeed = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].clkspeed;
                            tCK = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tCK;
                            tAA = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tAA;
                            tRCD = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRCD;
                            tRP = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRP;
                            tRAS = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].tRAS;
                            pszVoltage = g_MemoryInfo[i].specific.DDR5SDRAM.XMP.profile[j].moduleVDD;

                            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
                            AsciiCatf(Buffer, BufferSize, "<name>XMP Profile %d</name>\n", j + 1);
                            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                                      getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TextBuf, sizeof(TextBuf)));
                            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
                        }
                    }
                }

                if (g_MemoryInfo[i].specific.DDR5SDRAM.EXPOSupported)
                {
                    for (int j = 0; j < MAX_EXPO_PROFILES; j++)
                    {
                        if (g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].enabled)
                        {
                            clkspeed = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].clkspeed;
                            tCK = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tCK;
                            tAA = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tAA;
                            tRCD = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRCD;
                            tRP = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRP;
                            tRAS = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].tRAS;
                            pszVoltage = g_MemoryInfo[i].specific.DDR5SDRAM.EXPO.profile[j].moduleVDD;

                            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
                            AsciiCatf(Buffer, BufferSize, "<name>EXPO Profile %d</name>\n", j + 1);
                            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                                      getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TextBuf, sizeof(TextBuf)));
                            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
                        }
                    }
                }
            }
            else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR3)
            {
                if (g_MemoryInfo[i].specific.DDR3SDRAM.XMPSupported)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        if (g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].enabled)
                        {
                            clkspeed = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].clkspeed;
                            tCK = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tCK;
                            tAA = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tAA;
                            tRCD = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRCD;
                            tRP = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRP;
                            tRAS = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].tRAS;
                            pszVoltage = g_MemoryInfo[i].specific.DDR3SDRAM.XMP.profile[j].moduleVdd;

                            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
                            AsciiCatf(Buffer, BufferSize, "<name>XMP Profile %d</name>\n", j + 1);
                            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                                      getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TextBuf, sizeof(TextBuf)));
                            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
                        }
                    }
                }
            }
            else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR4)
            {
                if (g_MemoryInfo[i].specific.DDR4SDRAM.XMPSupported)
                {
                    for (int j = 0; j < 2; j++)
                    {
                        if (g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].enabled)
                        {
                            clkspeed = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].clkspeed;
                            tCK = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tCK;
                            tAA = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tAA;
                            tRCD = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRCD;
                            tRP = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRP;
                            tRAS = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].tRAS;
                            pszVoltage = g_MemoryInfo[i].specific.DDR4SDRAM.XMP.profile[j].moduleVdd;

                            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
                            AsciiCatf(Buffer, BufferSize, "<name>XMP Profile %d</name>\n", j + 1);
                            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                                      getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TextBuf, sizeof(TextBuf)));
                            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
                        }
                    }
                }
            }
            else if (g_MemoryInfo[i].type == SPDINFO_MEMTYPE_DDR2)
            {
                if (g_MemoryInfo[i].specific.DDR2SDRAM.EPPSupported)
                {
                    if (g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_ABBR)
                    {
                        for (int j = 0; j < 4; j++)
                        {
                            clkspeed = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].clkspeed;
                            tCK = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tCK;
                            tAA = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].CASSupported * tCK;
                            tRCD = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRCD;
                            tRP = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRP;
                            tRAS = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].tRAS;
                            pszVoltage = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.abbrProfile[j].voltageLevel;

                            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
                            AsciiCatf(Buffer, BufferSize, "<name>EPP Abbreviated Profile %d</name>\n", j + 1);
                            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                                      getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TextBuf, sizeof(TextBuf)));
                            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
                        }
                    }
                    else if (g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_FULL)
                    {
                        for (int j = 0; j < 2; j++)
                        {
                            clkspeed = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].clkspeed;
                            tCK = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tCK;
                            tAA = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].CASSupported * tCK;
                            tRCD = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRCD;
                            tRP = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRP;
                            tRAS = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].tRAS;
                            pszVoltage = g_MemoryInfo[i].specific.DDR2SDRAM.EPP.profileData.fullProfile[j].voltageLevel;

                            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
                            AsciiCatf(Buffer, BufferSize, "<name>EPP Full Profile %d</name>\n", j + 1);
                            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                                      getSPDProfileStr(g_MemoryInfo[i].type, clkspeed, tCK, tAA, tRCD, tRP, tRAS, pszVoltage, TextBuf, sizeof(TextBuf)));
                            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
                        }
                    }
                }
            }
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "</detail>\n");
        }
    }

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>Number of RAM slots</name>\n");
    AsciiCatf(Buffer, BufferSize, "<value>%d</value>\n", g_numSMBIOSMem);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    AsciiCatf(Buffer, BufferSize, "<detail>\n");
    AsciiCatf(Buffer, BufferSize, "<name>Number of RAM modules</name>\n");
    AsciiCatf(Buffer, BufferSize, "<value>%d</value>\n", g_numSMBIOSModules);
    AsciiCatf(Buffer, BufferSize, "</detail>\n");

    if (g_numSMBIOSMem > 0)
    {
        int i;
        for (i = 0; i < g_numSMBIOSMem; i++)
        {
            UINT64 RAMSizeMB = 0;
            CHAR16 szwRAMSummary[128];
            getSMBIOSRAMSummaryLine(&g_SMBIOSMemDevices[i], szwRAMSummary, sizeof(szwRAMSummary));
            UnicodeStrToAsciiStrS(szwRAMSummary, TempBuf, ARRAY_SIZE(TempBuf));
            ConvertStringToEntities(TempBuf, TempBuf2, sizeof(TempBuf2));

            AsciiCatf(Buffer, BufferSize, "<detail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>%s</name>\n", MtSupportGetSlotLabel(i, TRUE, TextBuf, sizeof(TextBuf)));
            AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf2);
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>JEDEC Manufacturer</name>\n");
            ConvertStringToEntities(g_SMBIOSMemDevices[i].szManufacturer, TempBuf, sizeof(TempBuf));
            AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Module Serial #</name>\n");
            ConvertStringToEntities(g_SMBIOSMemDevices[i].szSerialNumber, TempBuf, sizeof(TempBuf));
            AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Module Part #</name>\n");
            ConvertStringToEntities(g_SMBIOSMemDevices[i].szPartNumber, TempBuf, sizeof(TempBuf));
            AsciiCatf(Buffer, BufferSize, "<value>%a</value>\n", TempBuf);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Manufacture Date</name>\n");
            AsciiCatf(Buffer, BufferSize, "<value>Unknown</value>\n");
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Memory Capacity (MB)</name>\n");
            if (g_SMBIOSMemDevices[i].smb17.Size == 0x7fff) // Need to use Extended Size field
            {
                RAMSizeMB = g_SMBIOSMemDevices[i].smb17.ExtendedSize;
            }
            else
            {
                if (BitExtract(g_SMBIOSMemDevices[i].smb17.Size, 15, 15) == 0) // MB - bit 15 is the unit MB or KB
                    RAMSizeMB = g_SMBIOSMemDevices[i].smb17.Size;
                else // KB
                    RAMSizeMB = (g_SMBIOSMemDevices[i].smb17.Size * 1024) / 1048576;
            }

            AsciiCatf(Buffer, BufferSize, "<value>%d</value>\n", RAMSizeMB);
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>Module Manufacturer's Specific Data</name>\n");
            AsciiCatf(Buffer, BufferSize, "<value>");
            AsciiCatf(Buffer, BufferSize, "</value>\n");
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");

            AsciiCatf(Buffer, BufferSize, "<subdetail>\n");
            AsciiCatf(Buffer, BufferSize, "<name>SMBIOS Profile</name>\n");
            AsciiCatf(Buffer, BufferSize, "<value>%s</value>\n",
                      getSMBIOSRAMProfileStr(&g_SMBIOSMemDevices[i], TextBuf, sizeof(TextBuf)));
            AsciiCatf(Buffer, BufferSize, "</subdetail>\n");

            AsciiCatf(Buffer, BufferSize, "</detail>\n");
        }
    }

    AsciiCatf(Buffer, BufferSize, "</Details>\n");
}

VOID
    EFIAPI
    MtSupportPMPTestInfo(CHAR8 *Buffer, UINTN BufferSize)
{
    UINT32 ElapsedTime;

    CHAR16 TextBuf[128];
    CHAR16 TempBuf[128];
    CHAR8 TempBufUTF8[128];
    CHAR8 TempBuf2UTF8[128];

    int i;
    UINTN Percent;

    UINTN TotalErrCount = MtSupportGetNumErrors() + MtSupportGetNumCorrECCErrors() + MtSupportGetNumUncorrECCErrors();

    AsciiCatf(Buffer, BufferSize, "<StartTime name=\"Test Start Time\" type=\"timestamp\">%d</StartTime>\n", efi_time(&mTestStartTime));
    if (mElapsedTime > 0)
    {
        AsciiCatf(Buffer, BufferSize, "<StopTime name=\"Test Stop Time\" type=\"timestamp\">%d</StopTime>\n", efi_time(&mTestStartTime) + mElapsedTime);
        ElapsedTime = mElapsedTime;
    }
    else
    {
        EFI_TIME Time;
        gRT->GetTime(&Time, NULL);

        ElapsedTime = efi_time(&Time) - efi_time(&mTestStartTime);
    }

    AsciiCatf(Buffer, BufferSize, "<Duration name=\"Test Duration\" type=\"secs\">%d</Duration>\n", ElapsedTime);
    AsciiCatf(Buffer, BufferSize, "<MemRange name=\"Memory Range Tested\">0x%lx - %lx (%ldMB)</MemRange>\n", gAddrLimitLo, gAddrLimitHi, DivU64x32(gAddrLimitHi - gAddrLimitLo, SIZE_1MB));

    if (gCPUSelMode == CPU_SINGLE)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_MENU_CPUSEL_SINGLE), TempBuf, sizeof(TempBuf)), gSelCPUNo);
    else if (gCPUSelMode == CPU_PARALLEL)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_MENU_CPUSEL_PARALLEL), TempBuf, sizeof(TempBuf)));
    else if (gCPUSelMode == CPU_RROBIN)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_MENU_CPUSEL_RROBIN), TempBuf, sizeof(TempBuf)));
    else if (gCPUSelMode == CPU_SEQUENTIAL)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_MENU_CPUSEL_SEQUENTIAL), TempBuf, sizeof(TempBuf)));

    UnicodeStrToAsciiStrS(TextBuf, TempBufUTF8, ARRAY_SIZE(TempBufUTF8));
    ConvertStringToEntities(TempBufUTF8, TempBuf2UTF8, sizeof(TempBuf2UTF8));
    AsciiCatf(Buffer, BufferSize, "<CPUSelMode name=\"CPU Selection Mode\">%a</CPUSelMode>\n", TempBuf2UTF8);

    if (mNumCPUTempSamples > 0)
        AsciiCatf(Buffer, BufferSize, "<CPUTemp min=\"%d\" max=\"%d\" ave=\"%d\">%dC/%dC/%dC</CPUTemp>\n", mMinCPUTemp, mMaxCPUTemp, DivS64x64Remainder(mSumCPUTemp, mNumCPUTempSamples, NULL), mMinCPUTemp, mMaxCPUTemp, DivS64x64Remainder(mSumCPUTemp, mNumCPUTempSamples, NULL));
    else
        AsciiCatf(Buffer, BufferSize, "<CPUTemp min=\"-1\" max=\"-1\" ave=\"-1\">-/-/-</CPUTemp>\n");

    for (i = 0; i < MAX_RAM_TEMP_DIMMS; i++)
    {
        if (mNumRAMTempSamples[i] == 0)
            break;

        int MinTempC = (int)DivS64x64Remainder(mMinRAMTemp[i], 1000, NULL);
        int MaxTempC = (int)DivS64x64Remainder(mMaxRAMTemp[i], 1000, NULL);
        int AveTempC = (int)DivS64x64Remainder(mSumRAMTemp[i], MultS64x64(mNumRAMTempSamples[i], 1000), NULL);

        AsciiCatf(Buffer, BufferSize, "<RAMTemp name=\"TSOD%d\" min=\"%d\" max=\"%d\" ave=\"%d\">%dC/%dC/%dC</RAMTemp>\n", i, MinTempC, MaxTempC, AveTempC, MinTempC, MaxTempC, AveTempC);
    }

    if (mCurMemTimings.SpeedMTs > 0)
        AsciiCatf(Buffer, BufferSize, "<MemSpeedLo speed=\"%d\" cas=\"%d\" rcd=\"%d\" rp=\"%d\" ras=\"%d\">%d MT/s (%d-%d-%d-%d)</MemSpeedLo>\n",
                  mMinMemTimings.SpeedMTs, mMinMemTimings.tCAS, mMinMemTimings.tRCD, mMinMemTimings.tRP, mMinMemTimings.tRAS,
                  mMinMemTimings.SpeedMTs, mMinMemTimings.tCAS, mMinMemTimings.tRCD, mMinMemTimings.tRP, mMinMemTimings.tRAS);
    else
        AsciiCatf(Buffer, BufferSize, "<MemSpeedLo speed=\"-1\" cas=\"-1\" rcd=\"-1\" rp=\"-1\" ras=\"-1\">N/A</MemSpeedLo>\n");

    if (mCurMemTimings.SpeedMTs > 0)
        AsciiCatf(Buffer, BufferSize, "<MemSpeedHi speed=\"%d\" cas=\"%d\" rcd=\"%d\" rp=\"%d\" ras=\"%d\">%d MT/s (%d-%d-%d-%d)</MemSpeedHi>\n",
                  mMaxMemTimings.SpeedMTs, mMaxMemTimings.tCAS, mMaxMemTimings.tRCD, mMaxMemTimings.tRP, mMaxMemTimings.tRAS,
                  mMaxMemTimings.SpeedMTs, mMaxMemTimings.tCAS, mMaxMemTimings.tRCD, mMaxMemTimings.tRP, mMaxMemTimings.tRAS);
    else
        AsciiCatf(Buffer, BufferSize, "<MemSpeedHi speed=\"-1\" cas=\"-1\" rcd=\"-1\" rp=\"-1\" ras=\"-1\">N/A</MemSpeedHi>\n");

    if (gECCPoll)
    {
        AsciiCatf(Buffer, BufferSize, "<ECCPoll name=\"ECC Polling\">Enabled</ECCPoll>\n");
    }

    UINTN TotalTests = gNumPasses * MtSupportNumTests();

    Percent = (UINTN)DivU64x64Remainder(
        MultU64x32(MtSupportTestsCompleted(), 100),
        TotalTests,
        NULL);

    AsciiCatf(Buffer, BufferSize, "<TestsCompleted name=\"# Tests Completed\">%d/%d (%d%%)</TestsCompleted>\n", MtSupportTestsCompleted(), TotalTests, Percent);

    Percent = MtSupportTestsCompleted() == 0 ? 0 : (UINTN)DivU64x64Remainder(MultU64x32(mPassCount, 100), MtSupportTestsCompleted(), NULL);

    AsciiCatf(Buffer, BufferSize, "<PassCount name=\"# Tests Passed\">%d/%d (%d%%)</PassCount>\n", mPassCount, MtSupportTestsCompleted(), Percent);

    if (TotalErrCount > 0)
    {
        UINTN addr;
        UINTN mb;
        int n;

        AsciiCatf(Buffer, BufferSize, "<ErrorSummary>\n");

        if (MtSupportGetNumErrors() > 0)
        {
            addr = erri.low_addr;
            mb = erri.low_addr >> 20;
            if (erri.low_addr <= erri.high_addr)
                AsciiCatf(Buffer, BufferSize, "<LoAddr name=\"Lowest Error Address\">0x%p (%ldMB)</LoAddr>\n", addr, DivU64x32(addr, SIZE_1MB));
            else
                AsciiCatf(Buffer, BufferSize, "<LoAddr name=\"Lowest Error Address\">N/A</LoAddr>\n");

            addr = erri.high_addr;
            mb = erri.high_addr >> 20;
            if (erri.low_addr <= erri.high_addr)
                AsciiCatf(Buffer, BufferSize, "<HiAddr name=\"Highest Error Address\">0x%p (%ldMB)</HiAddr>\n", addr, DivU64x32(addr, SIZE_1MB));
            else
                AsciiCatf(Buffer, BufferSize, "<HiAddr name=\"Highest Error Address\">N/A</HiAddr>\n");

            if (sizeof(void *) > 4)
                AsciiCatf(Buffer, BufferSize, "<ErrBitMask name=\"Bits in Error Mask\">%016lX</ErrBitMask>\n", erri.ebits);
            else
                AsciiCatf(Buffer, BufferSize, "<ErrBitMask name=\"Bits in Error Mask\">%08X</ErrBitMask>\n", erri.ebits);

            /* Calc bits in error */
            for (i = 0, n = 0; i < sizeof(void *) * 8; i++)
            {
                if (erri.ebits >> i & 1)
                {
                    n++;
                }
            }

            AsciiCatf(Buffer, BufferSize, "<NumErrBits name=\"Bits in Error\">%d</NumErrBits>\n", n);
            AsciiCatf(Buffer, BufferSize, "<MaxContErrs name=\"Max Contiguous Errors\">%d</MaxContErrs>\n", erri.maxl);
        }

        TextBuf[0] = L'\0';
        for (i = 0; i < sizeof(erri.ecpus) / sizeof(erri.ecpus[0]); i++)
        {
            for (n = 0; n < sizeof(erri.ecpus[0]) * 8; n++)
            {
                int cpu = i * sizeof(erri.ecpus[0]) * 8 + n;
                if (erri.ecpus[i] & (LShiftU64(1, cpu)))
                {
                    CHAR16 CPUBuf[8];
                    UnicodeSPrint(CPUBuf, sizeof(CPUBuf), L"%d", cpu);
                    if (TextBuf[0] != L'\0')
                        StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), L", ");
                    StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), CPUBuf);
                }
            }
        }
        AsciiCatf(Buffer, BufferSize, "<CPUMemErr name=\"CPUs that detected memory errors\">{ %s }</CPUMemErr>\n", TextBuf);

        for (i = 0; i < g_numSMBIOSMem; i++)
        {
            if (g_SMBIOSMemDevices[i].smb17.Size != 0)
            {
                int DimmResult = MtSupportGetDimmTestResult(i);
                int SlotErrs = MtSupportGetNumSlotErrors(i);

                UINT32 TotalChipErrs = 0;
                int ChipWidth = MtSupportGetChipWidth(i);
                int NumRanks = MtSupportGetNumRanks(i);

                AsciiCatf(Buffer, BufferSize, "<DimmErr name=\"%s\" index=\"%d\" sn=\"%a\" ranks=\"%d\" width=\"x%d\" count=\"%d\" result=\"%s\">\n",
                          MtSupportGetSlotLabel(i, TRUE, TempBuf, sizeof(TempBuf)),
                          i,
                          g_SMBIOSMemDevices[i].szSerialNumber,
                          NumRanks,
                          ChipWidth,
                          SlotErrs,
                          MtSupportGetTestResultStr(DimmResult, 0, TextBuf, sizeof(TextBuf)));

#ifdef SITE_EDITION
                int NumChips = NumRanks * 64 / ChipWidth;

                for (int chip = 0; chip < NumChips; chip++)
                {
                    int ChipErrs = MtSupportGetNumSlotChipErrors(i, chip);
                    if (ChipErrs > 0)
                    {
                        CHAR16 ChipStr[8];
                        AsciiCatf(Buffer, BufferSize, "<ChipErr name=\"%s\" index=\"%d\" count=\"%d\" />\n",
                                  MtSupportGetChipLabel(chip, ChipStr, sizeof(ChipStr)),
                                  chip,
                                  ChipErrs);
                        TotalChipErrs += ChipErrs;
                    }
                }
#endif
                INT32 UnknownErrs = SlotErrs - TotalChipErrs;
                if (UnknownErrs > 0)
                {
                    AsciiCatf(Buffer, BufferSize, "<ChipErr name=\"Unknown\" index=\"-1\" count=\"%d\" />\n", UnknownErrs);
                }

                AsciiCatf(Buffer, BufferSize, "</DimmErr>\n");
            }
        }

        if (MtSupportGetNumUnknownSlotErrors() > 0)
        {
            AsciiCatf(Buffer, BufferSize, "<DimmErr name=\"Unknown\" index=\"-1\" count=\"%d\" />\n", MtSupportGetNumUnknownSlotErrors());
        }

        if (MtSupportGetNumCorrECCErrors() > 0 || MtSupportGetNumUncorrECCErrors() > 0)
        {
            AsciiCatf(Buffer, BufferSize, "<NumEccCorr name=\"ECC Correctable Errors\">%d</NumEccCorr>\n", MtSupportGetNumCorrECCErrors());
            AsciiCatf(Buffer, BufferSize, "<NumEccUncorr name=\"ECC Uncorrectable Errors\">%d</NumEccUncorr>\n", MtSupportGetNumUncorrECCErrors());
        }

        AsciiCatf(Buffer, BufferSize, "</ErrorSummary>\n");
    }
}

VOID
    EFIAPI
    MtSupportPMPTestResultDetails(CHAR8 *Buffer, UINTN BufferSize)
{
    UINTN NumRowHammerErr = 0;

    CHAR16 TextBuf[256];
    CHAR16 TempBuf[256];
    CHAR16 TempBuf2[64];
    CHAR16 TempBuf3[64];
    CHAR8 TempBufUTF8[256];
    CHAR8 TempBuf2UTF8[256];
    int i;

    UINTN TotalErrCount = MtSupportGetNumErrors() + MtSupportGetNumCorrECCErrors() + MtSupportGetNumUncorrECCErrors();

    AsciiCatf(Buffer, BufferSize, "<TestList>\n");
    for (i = 0; i < (int)mTestCount; i++)
    {
        UINTN Percent = mTests[i].NumCompleted == 0 ? 0 : (UINTN)DivU64x64Remainder(MultU64x32(mTests[i].NumPassed, 100), mTests[i].NumCompleted, NULL);

        AsciiCatf(Buffer, BufferSize, "<Test>\n");
        if (mTests[i].NameStr)
            StrCpyS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), mTests[i].NameStr);
        else
            GetStringFromPkg(0, mTests[i].NameStrId, TempBuf, sizeof(TempBuf));
        UnicodeStrToAsciiStrS(TempBuf, TempBufUTF8, ARRAY_SIZE(TempBufUTF8));
        ConvertStringToEntities(TempBufUTF8, TempBuf2UTF8, sizeof(TempBuf2UTF8));
        AsciiCatf(Buffer, BufferSize, "<Name name=\"Test\">%a</Name>\n", TempBuf2UTF8);
        AsciiCatf(Buffer, BufferSize, "<NumPassed name=\"# Tests Passed\">%d/%d (%d%%)</NumPassed>\n", mTests[i].NumPassed, mTests[i].NumCompleted, Percent);
        AsciiCatf(Buffer, BufferSize, "<Errors name=\"Errors\">%d</Errors>\n", mTests[i].NumErrors);
        AsciiCatf(Buffer, BufferSize, "</Test>\n");

        // Save the number of row hammer errors to determine whether or not we show the warning
        if (mTests[i].TestNo == 13)
            NumRowHammerErr = mTests[i].NumErrors;
    }
    AsciiCatf(Buffer, BufferSize, "</TestList>\n");

    if (TotalErrCount > 0)
    {
        UINT32 BufIdx = mErrorBufTail > 0 ? mErrorBufTail - 1 : mErrorBufSize - 1;
        UINTN logCount = 0;

        // Start errors table
        AsciiCatf(Buffer, BufferSize, "<ErrorLog name=\"Last %d Errors\">\n", gReportNumErr);

        while (mErrorBuf[BufIdx].Valid && logCount < gReportNumErr)
        {
            int ch = -1, sl = -1, chip = -1;
            CHAR16 ChBuf[16];
            CHAR16 SNBuf[64];
            SNBuf[0] = L'\0';

            GetErrorSlotStr(&mErrorBuf[BufIdx], &ch, &sl, &chip, ChBuf, sizeof(ChBuf));

            if (mErrorBuf[BufIdx].Type == 2)
            {
                CHAR16 SynBuf[16];

                if (mErrorBuf[BufIdx].ErrInfo.ECCErr.chan >= 0 && mErrorBuf[BufIdx].ErrInfo.ECCErr.dimm >= 0)
                {
                    for (i = 0; i < g_numMemModules; i++)
                    {
                        if (g_MemoryInfo[i].channel == mErrorBuf[BufIdx].ErrInfo.ECCErr.chan &&
                            g_MemoryInfo[i].slot == mErrorBuf[BufIdx].ErrInfo.ECCErr.dimm)
                        {
                            UnicodeSPrint(SNBuf, sizeof(SNBuf), L" (S/N: %08x)", g_MemoryInfo[i].moduleSerialNo);
                            break;
                        }
                    }
                }

                GetECCSyndStr(&mErrorBuf[BufIdx], SynBuf, sizeof(SynBuf));

                /* ECC error */
                if (mErrorBuf[BufIdx].AddrType == 0)
                {
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_ECC_ERRORS_INFO_PHYSADDR), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  GetECCTypeStringEN(mErrorBuf[BufIdx].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)),
                                  mErrorBuf[BufIdx].TestNo, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, GetStringFromPkg(0, mErrorBuf[BufIdx].ErrInfo.ECCErr.corrected ? STRING_TOKEN(STR_MENU_YES) : STRING_TOKEN(STR_MENU_NO), TempBuf3, sizeof(TempBuf3)), SynBuf, ChBuf);
                    StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), SNBuf);
                }
                else
                {
                    CHAR16 AddrBuf[32];
                    GetDimmAddrStr(&mErrorBuf[BufIdx], AddrBuf, sizeof(AddrBuf));

                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_ECC_ERRORS_INFO_DIMM), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  GetECCTypeStringEN(mErrorBuf[BufIdx].ErrInfo.ECCErr.type, TempBuf2, sizeof(TempBuf2)),
                                  mErrorBuf[BufIdx].TestNo, AddrBuf, GetStringFromPkg(0, mErrorBuf[BufIdx].ErrInfo.ECCErr.corrected ? STRING_TOKEN(STR_MENU_YES) : STRING_TOKEN(STR_MENU_NO), TempBuf3, sizeof(TempBuf3)), SynBuf, ChBuf);
                    StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), SNBuf);
                }
            }
            else if (mErrorBuf[BufIdx].Type == 1)
            {
                UnicodeSPrint(TextBuf, sizeof(TextBuf), L"[Parity Error] Test: %d, CPU: %d, Address: %lx", mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr);
            }
            else
            {

                switch (mErrorBuf[BufIdx].ErrDataSize)
                {
                case 4:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_32BIT), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT32 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT32 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 8:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_64BIT), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT64 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT32 *)(&mErrorBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 16:
                {
                    UINT64 Actual[2];
                    UINT64 Expected[2];
                    m128i_to_u64_array(&mErrorBuf[BufIdx].ErrInfo.DataErr.Actual, Actual);
                    m128i_to_u64_array(&mErrorBuf[BufIdx].ErrInfo.DataErr.Expected, Expected);

                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_128BIT), TempBuf, sizeof(TempBuf)),
                                  mErrorBuf[BufIdx].Time.Year, mErrorBuf[BufIdx].Time.Month, mErrorBuf[BufIdx].Time.Day, mErrorBuf[BufIdx].Time.Hour, mErrorBuf[BufIdx].Time.Minute, mErrorBuf[BufIdx].Time.Second,
                                  mErrorBuf[BufIdx].TestNo, mErrorBuf[BufIdx].ProcNum, (UINT64)mErrorBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, Expected[0], Expected[1], Actual[0], Actual[1]);
                    break;
                }
                }

                if (ch >= 0 && sl >= 0)
                {
                    int smbind = MC2SMBIOSSlot(ch, sl);
                    if (smbind < g_numSMBIOSMem)
                    {
                        if (g_SMBIOSMemDevices[smbind].smb17.Size != 0)
                            UnicodeSPrint(SNBuf, sizeof(SNBuf), L" (S/N: %a)", g_SMBIOSMemDevices[smbind].szSerialNumber);
                    }
                }
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), SNBuf);
            }
            UnicodeStrToAsciiStrS(TextBuf, TempBufUTF8, ARRAY_SIZE(TempBufUTF8));
            ConvertStringToEntities(TempBufUTF8, TempBuf2UTF8, sizeof(TempBuf2UTF8));
            AsciiCatf(Buffer, BufferSize, "<LogEntry>%a</LogEntry>\n", TempBuf2UTF8);

            BufIdx = BufIdx > 0 ? BufIdx - 1 : mErrorBufSize - 1;
            logCount++;
        }
        // End the table
        AsciiCatf(Buffer, BufferSize, "</ErrorLog>\n");
    }

    // If errors were detected in the "fast" pass but not the "slow" pass of Test 13, display warning
    if (erri.data_warn > 0 && NumRowHammerErr == 0)
    {
        // Start warning table
        GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_ROW_HAMMER_WARNING), TempBuf, sizeof(TempBuf));
        UnicodeStrToAsciiStrS(TempBuf, TempBufUTF8, ARRAY_SIZE(TempBufUTF8));
        ConvertStringToEntities(TempBufUTF8, TempBuf2UTF8, sizeof(TempBuf2UTF8));
        AsciiCatf(Buffer, BufferSize, "<HammerTestWarn>%a</HammerTestWarn>\n", TempBuf2UTF8);

        // If the REPORTNUMWARN config file parameter was specified, displayed the details of the errors detected in the "fast" pass
        if (gReportNumWarn > 0)
        {
            UINT32 BufIdx = mWarningBufTail > 0 ? mWarningBufTail - 1 : mWarningBufSize - 1;
            UINTN logCount = 0;

            // Start errors table
            AsciiCatf(Buffer, BufferSize, "<WarnLog name=\"Last %d Warnings\">\n", gReportNumWarn);

            while (mWarningBuf[BufIdx].Valid && logCount < gReportNumWarn)
            {
                CHAR16 ChBuf[16];
                SetMem(ChBuf, sizeof(ChBuf), 0);

                GetErrorSlotStr(&mWarningBuf[BufIdx], NULL, NULL, NULL, ChBuf, sizeof(ChBuf));

                switch (mWarningBuf[BufIdx].ErrDataSize)
                {
                case 4:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_32BIT), TempBuf, sizeof(TempBuf)),
                                  mWarningBuf[BufIdx].Time.Year, mWarningBuf[BufIdx].Time.Month, mWarningBuf[BufIdx].Time.Day, mWarningBuf[BufIdx].Time.Hour, mWarningBuf[BufIdx].Time.Minute, mWarningBuf[BufIdx].Time.Second,
                                  mWarningBuf[BufIdx].TestNo, mWarningBuf[BufIdx].ProcNum, (UINT64)mWarningBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT32 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT32 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 8:
                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_64BIT), TempBuf, sizeof(TempBuf)),
                                  mWarningBuf[BufIdx].Time.Year, mWarningBuf[BufIdx].Time.Month, mWarningBuf[BufIdx].Time.Day, mWarningBuf[BufIdx].Time.Hour, mWarningBuf[BufIdx].Time.Minute, mWarningBuf[BufIdx].Time.Second,
                                  mWarningBuf[BufIdx].TestNo, mWarningBuf[BufIdx].ProcNum, (UINT64)mWarningBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, *((UINT64 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Expected)), *((UINT64 *)(&mWarningBuf[BufIdx].ErrInfo.DataErr.Actual)));
                    break;
                case 16:
                {
                    UINT64 Actual[2];
                    UINT64 Expected[2];
                    m128i_to_u64_array(&mWarningBuf[BufIdx].ErrInfo.DataErr.Actual, Actual);
                    m128i_to_u64_array(&mWarningBuf[BufIdx].ErrInfo.DataErr.Expected, Expected);

                    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringFromPkg(0, STRING_TOKEN(STR_REPORT_MEM_ERRORS_INFO_128BIT), TempBuf, sizeof(TempBuf)),
                                  mWarningBuf[BufIdx].Time.Year, mWarningBuf[BufIdx].Time.Month, mWarningBuf[BufIdx].Time.Day, mWarningBuf[BufIdx].Time.Hour, mWarningBuf[BufIdx].Time.Minute, mWarningBuf[BufIdx].Time.Second,
                                  mWarningBuf[BufIdx].TestNo, mWarningBuf[BufIdx].ProcNum, (UINT64)mWarningBuf[BufIdx].AddrInfo.PhysAddr, ChBuf, Expected[0], Expected[1], Actual[0], Actual[1]);
                    break;
                }
                }
                UnicodeStrToAsciiStrS(TextBuf, TempBufUTF8, ARRAY_SIZE(TempBufUTF8));
                ConvertStringToEntities(TempBufUTF8, TempBuf2UTF8, sizeof(TempBuf2UTF8));
                AsciiCatf(Buffer, BufferSize, "<LogEntry>%a</LogEntry>\n", TempBuf2UTF8);

                BufIdx = BufIdx > 0 ? BufIdx - 1 : mWarningBufSize - 1;
                logCount++;
            }
            // End the table
            AsciiCatf(Buffer, BufferSize, "</WarnLog>\n");
        }
    }
}

EFI_STATUS
EFIAPI DHCPCallback(
    EFI_DHCP4_PROTOCOL *This,
    VOID *Context,
    EFI_DHCP4_STATE CurrentState,
    EFI_DHCP4_EVENT Dhcp4Event,
    EFI_DHCP4_PACKET *Packet,
    EFI_DHCP4_PACKET **NewPacket)
{
    switch (Dhcp4Event)
    {
    case Dhcp4SendRequest:
    {
        EFI_IPv4_ADDRESS ServerAddr = Packet->Dhcp4.Header.ServerAddr;
        AsciiFPrint(DEBUG_FILE_HANDLE, "DHCP Request sent to %d.%d.%d.%d", ServerAddr.Addr[0], ServerAddr.Addr[1], ServerAddr.Addr[2], ServerAddr.Addr[3]);
        break;
    }
    case Dhcp4RcvdAck:
    {
        if (Packet == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "ERROR DHCP response could not be found");
        }
        else
        {
            EFI_IPv4_ADDRESS ServerAddr = Packet->Dhcp4.Header.ServerAddr;
            AsciiFPrint(DEBUG_FILE_HANDLE, "DHCP Resoponse from %d.%d.%d.%d", ServerAddr.Addr[0], ServerAddr.Addr[1], ServerAddr.Addr[2], ServerAddr.Addr[3]);

            DHCPAddr = Packet->Dhcp4.Header.YourAddr;
            AsciiFPrint(DEBUG_FILE_HANDLE, "DHCP address received: %d.%d.%d.%d", DHCPAddr.Addr[0], DHCPAddr.Addr[1], DHCPAddr.Addr[2], DHCPAddr.Addr[3]);
        }
        break;
    }
    case Dhcp4BoundCompleted:
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "DHCP configuration process complete.");
        break;
    }
    default:
        AsciiFPrint(DEBUG_FILE_HANDLE, "DHCP Configuation could not be established");
        break;
    }

    return EFI_SUCCESS;
}

// Converts a string to an EFI_IPv4 address. Returns an empty structure if the address is invalid
EFI_IPv4_ADDRESS
MTSupportParseIPStrToAddr(
    CHAR16 *IPStr)
{
#ifdef DNS_ENABLED
    if (gPMPConnectionConfig.DNSAvaliable)
    {
        EFI_DNS4_CONFIG_DATA DNSConfigureData;

        DNSConfigureData.DnsServerListCount = 0;
        DNSConfigureData.EnableDnsCache = TRUE;
        DNSConfigureData.Protocol = 6; // TCP defined here: (https://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml)
        DNSConfigureData.LocalPort = 0;
        DNSConfigureData.RetryCount = 3;
        DNSConfigureData.RetryInterval = 2;

        EFI_STATUS Status = DNS->Configure(DNS, &DNSConfigureData);

        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Successfully configured DNS server. Found %d DNS servers", DNSConfigureData.DnsServerListCount);
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Could not configure DNS server");
            gPMPConnectionConfig.DNSAvaliable = FALSE;
        }

        if (gPMPConnectionConfig.DNSAvaliable)
        {
            EFI_DNS4_COMPLETION_TOKEN dnsCompletionToken;
            EFI_DNS4_MODE_DATA dnsModeData;
            gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &dnsCompletionToken.Event);
            Status = DNS->HostNameToIp(DNS, IPStr, &dnsCompletionToken);

            UINTN EventIndex;
            Status = gBS->WaitForEvent(1, &dnsCompletionToken.Event, &EventIndex);

            Status = DNS->GetModeData(DNS, &dnsModeData);

            if (dnsModeData.DnsServerCount > 0)
            {
                return dnsModeData.DnsServerList[0];
            }
        }
    }
#endif

    EFI_IPv4_ADDRESS V4Address;
    if (NetLibStrToIp4(IPStr, &V4Address) != EFI_SUCCESS)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[CONFIG] \"%s\" is an invalid IP address", IPStr);
    }
    return V4Address;
}

// Get the current state of the connection between the host and client specified by gPMPConnectionConfig
EFI_STATUS
MTSupportPMPTCPGetConnectionStatus()
{
    EFI_STATUS Status;
    EFI_TCP4_CONNECTION_STATE TCPConnectionState;
    Status = TCP4->GetModeData(TCP4, &TCPConnectionState, NULL, NULL, NULL, NULL);
    gPMPConnectionConfig.TCPConnectionState = TCPConnectionState;
    return Status;
}

// Send TCP Connection request to the host specified in the gPMPConnectionConfig structure
EFI_STATUS
MTSupportPMPTCPEstablishConnection()
{
    EFI_STATUS Status = 0;
    EFI_TCP4_CONNECTION_TOKEN connectiontoken;
    gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &connectiontoken.CompletionToken.Event);

    EFI_IPv4_ADDRESS subnetAdr;
    subnetAdr.Addr[0] = 0;
    subnetAdr.Addr[1] = 0;
    subnetAdr.Addr[2] = 0;
    subnetAdr.Addr[3] = 0;

#if 1
    EFI_IPv4_ADDRESS defaultGateway = MTSupportParseIPStrToAddr(gTCPGatewayIP);

    if (defaultGateway.Addr[0] == 0)
        defaultGateway = MTSupportParseIPStrToAddr(L"192.168.0.1");

    Status = TCP4->Routes(TCP4, FALSE, &subnetAdr, &subnetAdr, &defaultGateway);

    if (Status == EFI_SUCCESS)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Default gateway set to %d.%d.%d.%d", defaultGateway.Addr[0], defaultGateway.Addr[1], defaultGateway.Addr[2], defaultGateway.Addr[3]);
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Could not configure default gateway");
    }
#endif

    Status = TCP4->Connect(TCP4, &connectiontoken);

    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Connecting to remote server");
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Could not connect to remote server ERR %d", Status);
        return Status;
    }

    UINTN EventIndex;
    Status = gBS->WaitForEvent(1, &connectiontoken.CompletionToken.Event, &EventIndex);
    if (Status == EFI_SUCCESS)
    {
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Failed to create connection event", Status);
        return Status;
    }

    MTSupportPMPTCPGetConnectionStatus();

    if (gPMPConnectionConfig.TCPConnectionState != Tcp4StateEstablished)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Connection to server could not be esablished. Error: %d", Status);
        return EFI_NO_RESPONSE;
    }

    return Status;
}

EFI_STATUS
MTSupportPMPTCPCloseConnection()
{
    EFI_STATUS Status = 0;
    EFI_TCP4_CLOSE_TOKEN closeToken;
    UINTN EventIndex;

    gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &closeToken.CompletionToken.Event);

    Status = TCP4->Close(TCP4, &closeToken);

    if (Status != EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Failed to close connection with server ERR: %d", Status);

    Status = gBS->WaitForEvent(1, &closeToken.CompletionToken.Event, &EventIndex);

    return Status;
}

EFI_STATUS
MtSupportPMPTCPReconnectIfNeeded()
{

    EFI_STATUS Status = 0;

    MTSupportPMPTCPGetConnectionStatus();
    // If the connection to the server has been lost attempt to restablish connection

    AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Connection State %d", gPMPConnectionConfig.TCPConnectionState);
    if (gPMPConnectionConfig.TCPConnectionState == Tcp4StateCloseWait)
    {
        MTSupportPMPTCPCloseConnection();
    }

    if (gPMPConnectionConfig.TCPConnectionState != Tcp4StateEstablished)
    {
        Status = MTSupportPMPTCPEstablishConnection();
        if (Status != EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Could not re-establish connection to console server error: %d", Status);
            gPMPConnectionConfig.TCPAvaliable = FALSE;
        }
    }

    return Status;
}

EFI_STATUS
MTSupportPMPTCPTransmitData(
    const CHAR8 *RequestData,
    UINTN RequestSize)
{
    EFI_STATUS Status = 0;
    UINTN EventIndex = 0;
    EFI_TCP4_TRANSMIT_DATA TCPTransmitData;
    EFI_TCP4_IO_TOKEN TCPTransmitToken;
    CHAR8 PostRequest[MAX_TCP_BUFFER_SIZE];

    CHAR8 RequestHeader[VLONG_STRING_LEN];
    AsciiSPrint(RequestHeader, VLONG_STRING_LEN, "POST %ls HTTP/1.1\nAccept: */*\nAccept-Encoding: identity\nConnection: keep-alive\nUser-Agent: Memtest86 - passmark.com [MT]\nHost: 127.0.0.1\nContent-Length: %d\n\n", gTCPRequestLocation, RequestSize);

    SetMem(PostRequest, MAX_TCP_BUFFER_SIZE, 0);
    AsciiStrCatS(PostRequest, MAX_TCP_BUFFER_SIZE, RequestHeader);
    AsciiStrCatS(PostRequest, MAX_TCP_BUFFER_SIZE, RequestData);
    AsciiFPrint(DEBUG_FILE_HANDLE, "Sending HTTP message");
    AsciiFPrint(DEBUG_FILE_HANDLE, PostRequest);

    TCPTransmitData.Push = FALSE;
    TCPTransmitData.Urgent = FALSE;
    TCPTransmitData.DataLength = (UINT32)AsciiStrLen(PostRequest);
    TCPTransmitData.FragmentCount = 1;
    TCPTransmitData.FragmentTable[0].FragmentLength = (UINT32)AsciiStrLen(PostRequest);
    TCPTransmitData.FragmentTable[0].FragmentBuffer = PostRequest;
    TCPTransmitToken.Packet.TxData = &TCPTransmitData;

    gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &TCPTransmitToken.CompletionToken.Event);
    Status = TCP4->Transmit(TCP4, &TCPTransmitToken);
    if (Status != EFI_SUCCESS)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Transmit failed with error: %d", Status);
        return Status;
    }
    Status = gBS->WaitForEvent(1, &TCPTransmitToken.CompletionToken.Event, &EventIndex);

    if (Status == EFI_SUCCESS)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Successfully transmitted %d bytes to server", AsciiStrLen(PostRequest));
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Failed to transmit packet to remote server ERR: %d", Status);
    }

    return Status;
}

EFI_STATUS
EFIAPI
MtSupportPMPConnect()
{
#ifdef SITE_EDITION

    if (gPMPDisable)
        return EFI_SUCCESS;

    EFI_STATUS Status = EFI_NOT_STARTED;
    EFI_SERVICE_BINDING_PROTOCOL *TCPService = NULL;
    EFI_SERVICE_BINDING_PROTOCOL *DHCPService = NULL;
#ifdef DNS_ENABLED
    EFI_SERVICE_BINDING_PROTOCOL *DNSService = NULL;
#endif
    EFI_SERVICE_BINDING_PROTOCOL *TLSService = NULL;
    EFI_HANDLE TCPChildHandle = NULL;
    EFI_HANDLE DHCPChildHanle = NULL;
#ifdef DNS_ENABLED
    EFI_HANDLE DNSChildHandle = NULL;
#endif
    EFI_HANDLE TLSChildHandle = NULL;

    if (gTCPDisable == FALSE)
    {

        if (gDHCPDisable == FALSE)
        {
            // Connect to local DHCP server to automatically obtain network configuration parameters
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Connecting DHCP Service");
            Status = gBS->LocateProtocol(&dhcpserviceguid, NULL, (VOID **)&DHCPService);
            if (Status != EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DHCP Service not supported, using manually configured TCP configuration");
                gPMPConnectionConfig.DHCPAvaliable = FALSE;
            }
            else
            {
                Print(L"Configuring DHCP Address...\n");
                gPMPConnectionConfig.DHCPAvaliable = TRUE;
                Status = DHCPService->CreateChild(DHCPService, &DHCPChildHanle);
                Status = gBS->OpenProtocol(DHCPChildHanle, &dhcpprotguid, (VOID **)&DHCP, gImageHandle, DHCPChildHanle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

                if (Status != EFI_SUCCESS)
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DHCP could not open protocol ERR: %d", Status);

                // Setting client address to non zero value indicates re-initialization.
                // Zero client address signals new connection
                UINT8 ClientAddress[4] = {0, 0, 0, 0};

                EFI_DHCP4_CONFIG_DATA DHCPConfigData;
                EFI_DHCP4_MODE_DATA DHCPModeData;

                DHCPConfigData.DiscoverTryCount = 0;
                DHCPConfigData.RequestTryCount = 0;
                DHCPConfigData.Dhcp4Callback = &DHCPCallback;
                DHCPConfigData.CallbackContext = NULL;
                gBS->CopyMem(&DHCPConfigData.ClientAddress, ClientAddress, sizeof(ClientAddress));

                Status = DHCP->Configure(DHCP, &DHCPConfigData);

                if (Status == EFI_SUCCESS)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DHCP SUCCUSSFULLY CONFIGURED WITH ADDRESS: %d.%d.%d.%d", DHCPAddr.Addr[0], DHCPAddr.Addr[1], DHCPAddr.Addr[2], DHCPAddr.Addr[3]);
                }
                else
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DHCP FAILED TO CONFIGURE ERR %d", Status);
                    gPMPConnectionConfig.DHCPAvaliable = FALSE;
                }

                Status = DHCP->GetModeData(DHCP, &DHCPModeData);

                if (DHCPModeData.State == Dhcp4Init)
                {
                    Status = DHCP->Start(DHCP, NULL);

                    if (Status != EFI_SUCCESS)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DHCP FAILED TO START ERR %d", Status);
                        gPMPConnectionConfig.DHCPAvaliable = FALSE;
                    }
                }
                else
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DHCP CANNOT START AS NOT IN INIT STATE", Status);
                    gPMPConnectionConfig.DHCPAvaliable = FALSE;
                }
            }
        }

#ifdef DNS_ENABLED
        Status = gBS->LocateProtocol(&gEfiDns4ServiceBindingProtocolGuid, NULL, (VOID **)&DNSService);
        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DNS protocol successfully located");
            Status = DNSService->CreateChild(DNSService, &DNSChildHandle);
            Status = gBS->OpenProtocol(DNSChildHandle, &gEfiDns4ProtocolGuid, (VOID **)&DNS, gImageHandle, DNSChildHandle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
            gPMPConnectionConfig.DNSAvaliable = TRUE;

            if (Status == EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DNS protocol opened");
            }
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DNS protocol Failed to open with ERROR %d", Status);
                gPMPConnectionConfig.DNSAvaliable = FALSE;
            }
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] DNS protocol not supported on this device, direct IP address should be used in TCPSERVERIP parameter");
        }
#endif

        Status = gBS->LocateProtocol(&gEfiTlsServiceBindingProtocolGuid, NULL, (VOID **)&TLSService);
        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TLS protocol successfully located");
            Status = TLSService->CreateChild(TLSService, &TLSChildHandle);
            Status = gBS->OpenProtocol(TLSChildHandle, &gEfiTlsProtocolGuid, (VOID **)&TLS, gImageHandle, TLSChildHandle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

            if (Status == EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TLS protocol opened");
            }
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Could not open TLS protocol");
            }
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TLS protocol could not be loaded");
        }

        // Determine if TCP protocol exists on this hardware
        Status = gBS->LocateProtocol(&gEfiTcp4ServiceBindingProtocolGuid, NULL, (VOID **)&TCPService);

        if (Status == EFI_SUCCESS)
        {
            Print(L"Connecting to Management Console...\n");

            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TCP protocol successfully located");
            Status = TCPService->CreateChild(TCPService, &TCPChildHandle);
            Status = gBS->OpenProtocol(TCPChildHandle, &gEfiTcp4ProtocolGuid, (VOID **)&TCP4, gImageHandle, TCPChildHandle, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
            gPMPConnectionConfig.TCPAvaliable = TRUE;

            if (Status == EFI_SUCCESS)
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TCP protocol open");
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TCP protocol failed to open with err %d", Status);
                gPMPConnectionConfig.TCPAvaliable = FALSE;
            }

            gPMPConnectionConfig.TCPConfigData.TypeOfService = 0;
            gPMPConnectionConfig.TCPConfigData.TimeToLive = 16;
            gPMPConnectionConfig.TCPConfigData.AccessPoint.UseDefaultAddress = FALSE;
            gPMPConnectionConfig.TCPConfigData.AccessPoint.StationPort = 0;
            gPMPConnectionConfig.TCPConfigData.AccessPoint.RemotePort = (UINT16)gTCPServerPort;
            gPMPConnectionConfig.TCPConfigData.AccessPoint.ActiveFlag = TRUE;

            gPMPConnectionConfig.TCPConfigData.AccessPoint.RemoteAddress = MTSupportParseIPStrToAddr(gTCPServerIP);

            if (gPMPConnectionConfig.DHCPAvaliable)
                gPMPConnectionConfig.TCPConfigData.AccessPoint.StationAddress = DHCPAddr;
            else
            {
                EFI_IPv4_ADDRESS stationAddr = MTSupportParseIPStrToAddr(gTCPClientIP);
                gBS->CopyMem(&gPMPConnectionConfig.TCPConfigData.AccessPoint.StationAddress, &stationAddr, sizeof(stationAddr));
            }

#if 1
            UINT8 subnetMask[4] = {255, 255, 255, 0};
            gBS->CopyMem(&gPMPConnectionConfig.TCPConfigData.AccessPoint.SubnetMask, &subnetMask, sizeof(subnetMask));
#endif

            Status = TCP4->Configure(TCP4, &gPMPConnectionConfig.TCPConfigData);

            if (Status == EFI_SUCCESS)
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TCP protocol successfully configured");
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] EFI TCP failed to confgure with err %d", Status);
                gPMPConnectionConfig.TCPAvaliable = FALSE;
            }

            EFI_IPv4_ADDRESS remoteaddr = gPMPConnectionConfig.TCPConfigData.AccessPoint.RemoteAddress;
            EFI_IPv4_ADDRESS stationaddr = gPMPConnectionConfig.TCPConfigData.AccessPoint.StationAddress;

            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Connecting to %d.%d.%d.%d:%d from %d.%d.%d.%d:%d", remoteaddr.Addr[0], remoteaddr.Addr[1], remoteaddr.Addr[2], remoteaddr.Addr[3], gPMPConnectionConfig.TCPConfigData.AccessPoint.RemotePort,
                        stationaddr.Addr[0], stationaddr.Addr[1], stationaddr.Addr[2], stationaddr.Addr[3], gPMPConnectionConfig.TCPConfigData.AccessPoint.StationPort);
            Status = MTSupportPMPTCPEstablishConnection();

            if (Status != EFI_SUCCESS)
            {
                return Status;
            }

            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Successfully established connection to server");

            // TLS Handshake
            EFI_TLS_VERSION EFITlsVersion = {1, 3};
            TLS->SetSessionData(TLS, EfiTlsVersion, &EFITlsVersion, sizeof(EFI_TLS_VERSION));

            UINTN BufSize = 128 * 1024;
            CHAR8 *messageBuffer = AllocateZeroPool(BufSize);
            if (messageBuffer == NULL)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Failed to allocate message buffer");
                return EFI_OUT_OF_RESOURCES;
            }

            AsciiCatf(messageBuffer, BufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
            AsciiCatf(messageBuffer, BufSize, "<PMP>\n");
            MtSupportPMPHeader(messageBuffer, BufSize, L"Connect");
            AsciiCatf(messageBuffer, BufSize, "<Connect>\n");

            MtSupportPMPSysInfo(messageBuffer, BufSize);

            AsciiCatf(messageBuffer, BufSize, "</Connect>\n");
            AsciiCatf(messageBuffer, BufSize, "</PMP>\n");
            Status = MTSupportPMPTCPTransmitData(messageBuffer, AsciiStrLen(messageBuffer));

            FreePool(messageBuffer);
            return Status;
        }
        else
        {
            gPMPConnectionConfig.TCPAvaliable = FALSE;
        }
    }
#if 0
    if (gPMPConnectionConfig.TCPAvaliable == FALSE || gTCPDisable)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] TCP protocol not supported on this device, defaulting to TFTP mode");
        MT_HANDLE FileHandle = NULL;
        EFI_TIME Time;
        CHAR16 Filename[128];

        gRT->GetTime(&Time, NULL);

        UnicodeSPrint(Filename, sizeof(Filename), L"%s\\%02x-%02x-%02x-%02x-%02x-%02x.%s.Connect.xml", CLIENT_DIR,
                      ClientMAC.Addr[0], ClientMAC.Addr[1], ClientMAC.Addr[2], ClientMAC.Addr[3], ClientMAC.Addr[4], ClientMAC.Addr[5],
                      MtSupportGetTimestampStr(&Time));

#ifdef PXE_DEBUG
        // Create Clients directory, if not exist
        Status = MtSupportOpenFile(&FileHandle, CLIENT_DIR, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Created directory \"%s\"", CLIENT_DIR);
        }
        if (FileHandle != NULL)
        {
            MtSupportCloseFile(FileHandle);
            FileHandle = NULL;
        }
        Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
#else
        if (PXEProtocol == NULL)
            return EFI_SUCCESS;

        Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE | EFI_FILE_MODE_TFTP, 0);
#endif
        if (FileHandle == NULL)
            return Status;

        UINTN BufSize = 128 * 1024;
        CHAR8 *messageBuffer = AllocateZeroPool(BufSize);
        if (messageBuffer == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to allocate message buffer");
            return EFI_OUT_OF_RESOURCES;
        }

        // Write out the heading
        AsciiCatf(messageBuffer, BufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        AsciiCatf(messageBuffer, BufSize, "<PMP>\n");
        MtSupportPMPHeader(messageBuffer, BufSize, L"Connect");
        AsciiCatf(messageBuffer, BufSize, "<Connect>\n");
        MtSupportPMPSysInfo(messageBuffer, BufSize);
        AsciiCatf(messageBuffer, BufSize, "</Connect>\n");
        AsciiCatf(messageBuffer, BufSize, "</PMP>\n");

        UINTN MsgLen = AsciiStrLen(messageBuffer);
        MtSupportWriteFile(FileHandle, &MsgLen, messageBuffer);

        FreePool(messageBuffer);
        return MtSupportCloseFile(FileHandle);
    }
#endif
    return Status;
#else
    return EFI_SUCCESS;
#endif
}

EFI_STATUS
EFIAPI
MtSupportPMPStatus()
{
#ifdef SITE_EDITION
    if (gPMPDisable)
        return EFI_SUCCESS;

    EFI_STATUS Status = EFI_NOT_FOUND;
    UINTN TotalErrCount = MtSupportGetNumErrors() + MtSupportGetNumCorrECCErrors() + MtSupportGetNumUncorrECCErrors();

    // If Memtest has established a valid TCP connection
    if (gPMPConnectionConfig.TCPAvaliable && gTCPDisable == FALSE)
    {
        Status = MtSupportPMPTCPReconnectIfNeeded();

        if (Status != EFI_SUCCESS)
            return Status;

        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Sending Status to console");
        UINTN BufSize = 128 * 1024;
        CHAR8 *messageBuffer = AllocateZeroPool(BufSize);
        if (messageBuffer == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Failed to allocate message buffer");
            return EFI_OUT_OF_RESOURCES;
        }

        AsciiCatf(messageBuffer, BufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        AsciiCatf(messageBuffer, BufSize, "<PMP>\n");
        MtSupportPMPHeader(messageBuffer, BufSize, L"Status");
        AsciiCatf(messageBuffer, BufSize, "<Status>\n");
        AsciiCatf(messageBuffer, BufSize, "<MT86Status>\n");
        AsciiCatf(messageBuffer, BufSize, "<Status>RUNNING - %s</Status>\n", TotalErrCount == 0 ? L"OK" : L"Errors");
        MtSupportPMPTestInfo(messageBuffer, BufSize);
        MtSupportPMPTestResultDetails(messageBuffer, BufSize);
        AsciiCatf(messageBuffer, BufSize, "</MT86Status>\n");
        AsciiCatf(messageBuffer, BufSize, "</Status>\n");
        AsciiCatf(messageBuffer, BufSize, "</PMP>\n");

        if (gPMPConnectionConfig.TCPAvaliable)
            Status = MTSupportPMPTCPTransmitData(messageBuffer, AsciiStrLen(messageBuffer));

        FreePool(messageBuffer);
        return Status;
    }

    if (gPMPConnectionConfig.TCPAvaliable == FALSE || gTCPDisable)
    {
        MT_HANDLE FileHandle = NULL;
        CHAR16 Filename[128];
        EFI_TIME Time;

        gRT->GetTime(&Time, NULL);

        UnicodeSPrint(Filename, sizeof(Filename), L"%s\\%02x-%02x-%02x-%02x-%02x-%02x.%s.Status.xml", CLIENT_DIR,
                      ClientMAC.Addr[0], ClientMAC.Addr[1], ClientMAC.Addr[2], ClientMAC.Addr[3], ClientMAC.Addr[4], ClientMAC.Addr[5],
                      MtSupportGetTimestampStr(&Time));

#ifdef PXE_DEBUG
        // Create Clients directory, if not exist
        Status = MtSupportOpenFile(&FileHandle, CLIENT_DIR, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Created directory \"%s\"", CLIENT_DIR);
        }
        if (FileHandle != NULL)
        {
            MtSupportCloseFile(FileHandle);
            FileHandle = NULL;
        }
        Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
#else
        if (PXEProtocol == NULL)
            return EFI_SUCCESS;

        Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE | EFI_FILE_MODE_TFTP, 0);
#endif

        if (FileHandle == NULL)
            return Status;

        UINTN BufSize = 128 * 1024;
        CHAR8 *messageBuffer = AllocateZeroPool(BufSize);
        if (messageBuffer == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to allocate message buffer");
            return EFI_OUT_OF_RESOURCES;
        }

        // Write out the heading
        AsciiCatf(messageBuffer, BufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        AsciiCatf(messageBuffer, BufSize, "<PMP>\n");
        MtSupportPMPHeader(messageBuffer, BufSize, L"Status");
        AsciiCatf(messageBuffer, BufSize, "<Status>\n");
        AsciiCatf(messageBuffer, BufSize, "<MT86Status>\n");
        AsciiCatf(messageBuffer, BufSize, "<Status>RUNNING - %s</Status>\n", TotalErrCount == 0 ? L"OK" : L"Errors");
        MtSupportPMPTestInfo(messageBuffer, BufSize);
        MtSupportPMPTestResultDetails(messageBuffer, BufSize);
        AsciiCatf(messageBuffer, BufSize, "</MT86Status>\n");
        AsciiCatf(messageBuffer, BufSize, "</Status>\n");
        AsciiCatf(messageBuffer, BufSize, "</PMP>\n");

        UINTN MsgLen = AsciiStrLen(messageBuffer);
        MtSupportWriteFile(FileHandle, &MsgLen, messageBuffer);

        FreePool(messageBuffer);
        return MtSupportCloseFile(FileHandle);
    }
    return Status;
#else
    return EFI_SUCCESS;
#endif
}

EFI_STATUS
EFIAPI
MtSupportPMPTestResult()
{
#ifdef SITE_EDITION
    CHAR16 TempBuf[32];

    if (gPMPDisable)
        return EFI_SUCCESS;

    EFI_STATUS Status = EFI_NOT_FOUND;

    if (gPMPConnectionConfig.TCPAvaliable && gTCPDisable == FALSE)
    {
        Status = MtSupportPMPTCPReconnectIfNeeded();

        if (Status != EFI_SUCCESS)
            return Status;

        AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Sending Results to console");
        UINTN BufSize = 128 * 1024;
        CHAR8 *messageBuffer = AllocateZeroPool(BufSize);
        if (messageBuffer == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[PMP] Failed to allocate message buffer");
            return EFI_OUT_OF_RESOURCES;
        }

        // Write out the heading
        AsciiCatf(messageBuffer, BufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        AsciiCatf(messageBuffer, BufSize, "<PMP>\n");
        MtSupportPMPHeader(messageBuffer, BufSize, L"TestResult");
        AsciiCatf(messageBuffer, BufSize, "<TestResult>\n");
        MtSupportPMPSysInfo(messageBuffer, BufSize);
        AsciiCatf(messageBuffer, BufSize, "<MT86Result>\n");
        AsciiCatf(messageBuffer, BufSize, "<Result>%s</Result>\n", MtSupportGetTestResultStr(MtSupportGetTestResult(), 0, TempBuf, sizeof(TempBuf)));
        MtSupportPMPTestInfo(messageBuffer, BufSize);
        MtSupportPMPTestResultDetails(messageBuffer, BufSize);

        AsciiCatf(messageBuffer, BufSize, "</MT86Result>\n");
        AsciiCatf(messageBuffer, BufSize, "</TestResult>\n");
        AsciiCatf(messageBuffer, BufSize, "</PMP>\n");

        if (gPMPConnectionConfig.TCPAvaliable)
            Status = MTSupportPMPTCPTransmitData(messageBuffer, AsciiStrLen(messageBuffer));

        FreePool(messageBuffer);
        return Status;
    }

    if (gPMPConnectionConfig.TCPAvaliable == FALSE || gTCPDisable)
    {
        MT_HANDLE FileHandle = NULL;
        EFI_TIME Time;
        CHAR16 Filename[128];

        gRT->GetTime(&Time, NULL);

        UnicodeSPrint(Filename, sizeof(Filename), L"%s\\%02x-%02x-%02x-%02x-%02x-%02x.%s.TestResult.xml", CLIENT_DIR,
                      ClientMAC.Addr[0], ClientMAC.Addr[1], ClientMAC.Addr[2], ClientMAC.Addr[3], ClientMAC.Addr[4], ClientMAC.Addr[5],
                      MtSupportGetTimestampStr(&Time));

#ifdef PXE_DEBUG
        // Create Clients directory, if not exist
        Status = MtSupportOpenFile(&FileHandle, CLIENT_DIR, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Created directory \"%s\"", CLIENT_DIR);
        }
        if (FileHandle != NULL)
        {
            MtSupportCloseFile(FileHandle);
            FileHandle = NULL;
        }
        Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
#else
        if (PXEProtocol == NULL)
            return EFI_SUCCESS;

        Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE | EFI_FILE_MODE_TFTP, 0);
#endif

        if (FileHandle == NULL)
            return Status;

        UINTN BufSize = 128 * 1024;
        CHAR8 *messageBuffer = AllocateZeroPool(BufSize);
        if (messageBuffer == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to allocate message buffer");
            return EFI_OUT_OF_RESOURCES;
        }

        // Write out the heading
        AsciiCatf(messageBuffer, BufSize, "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
        AsciiCatf(messageBuffer, BufSize, "<PMP>\n");
        MtSupportPMPHeader(messageBuffer, BufSize, L"TestResult");
        AsciiCatf(messageBuffer, BufSize, "<TestResult>\n");
        MtSupportPMPSysInfo(messageBuffer, BufSize);
        AsciiCatf(messageBuffer, BufSize, "<MT86Result>\n");
        AsciiCatf(messageBuffer, BufSize, "<Result>%s</Result>\n", MtSupportGetTestResultStr(MtSupportGetTestResult(), 0, TempBuf, sizeof(TempBuf)));
        MtSupportPMPTestInfo(messageBuffer, BufSize);
        MtSupportPMPTestResultDetails(messageBuffer, BufSize);

        AsciiCatf(messageBuffer, BufSize, "</MT86Result>\n");
        AsciiCatf(messageBuffer, BufSize, "</TestResult>\n");
        AsciiCatf(messageBuffer, BufSize, "</PMP>\n");

        UINTN MsgLen = AsciiStrLen(messageBuffer);
        MtSupportWriteFile(FileHandle, &MsgLen, messageBuffer);

        FreePool(messageBuffer);
        return MtSupportCloseFile(FileHandle);
    }
    return Status;
#else
    return EFI_SUCCESS;
#endif
}

VOID SyncEnableCacheAllAPs(BOOLEAN Enable)
{
    EFI_EVENT APEvent[MAX_CPUS];
    UINTN TotalProcs = MPSupportGetMaxProcessors();
    UINTN TotalEnabledProcs = 0;
    BOOLEAN bAPFinished = TRUE;
    UINTN ProcNum;
    UINT64 WaitStartTime = 0;

    SetMem(APEvent, sizeof(APEvent), 0);
    for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
    {
        if (MPSupportIsProcEnabled(ProcNum))
            TotalEnabledProcs++;
    }
    barrier_init(&mBarr, (int)TotalProcs);

    if (1 /*gCPUSelMode == CPU_PARALLEL && NumProcsEn > 1*/)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "SyncEnableCacheAllAPs - %s memory cache for %d processors", Enable ? L"Enabling" : L"Disabling", TotalEnabledProcs);
        MtSupportDebugWriteLine(gBuffer);

        for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
        {
            if (ProcNum != MPSupportGetBspId() &&
                MPSupportIsProcEnabled(ProcNum))
            {
                EFI_STATUS Status;
                Status = MPSupportEnableCacheAP(ProcNum, &APEvent[ProcNum], Enable, &mBarr);
                if (EFI_ERROR(Status))
                {
                    AsciiSPrint(gBuffer, BUF_SIZE, "SyncEnableCacheAllAPs - Could not %s cache on AP#%d (%r).", Enable ? L"enable" : L"disable", ProcNum, Status);
                    MtSupportDebugWriteLine(gBuffer);
                }
            }
        }
    }

    // barrier(&mBarr); // Sync all CPUs

    AsciiSPrint(gBuffer, BUF_SIZE, "SyncEnableCacheAllAPs - %s memory cache on BSP", Enable ? L"Enabling" : L"Disabling");
    MtSupportDebugWriteLine(gBuffer);

    MtSupportEnableCache(Enable);

    // barrier(&mBarr); // Sync all CPUS once more

    AsciiSPrint(gBuffer, BUF_SIZE, "SyncEnableCacheAllAPs - Waiting for APs to finish");
    MtSupportDebugWriteLine(gBuffer);

    WaitStartTime = MtSupportGetTimeInMilliseconds();
    UINT64 WaitedMs = 0;
    do
    {
        bAPFinished = TRUE;

        WaitedMs = MtSupportGetTimeInMilliseconds() - WaitStartTime;
        for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
        {
            if (ProcNum != MPSupportGetBspId())
            {
                if (APEvent[ProcNum])
                {
                    EFI_STATUS Status;
                    Status = gBS->CheckEvent(APEvent[ProcNum]);

                    if (Status == EFI_NOT_READY) // CPU finished event has not been signalled
                    {
                        bAPFinished = FALSE;

                        if (WaitedMs > 1000)
                        {
                            AsciiSPrint(gBuffer, BUF_SIZE, "WARNING - waited for %lds for CPU #%d to finish", DivU64x32(WaitedMs, 1000), ProcNum);
                            MtSupportDebugWriteLine(gBuffer);
                        }
                    }
                    else if (Status == EFI_SUCCESS) // CPU finished event has been signalled
                    {
                        gBS->CloseEvent(APEvent[ProcNum]);
                        APEvent[ProcNum] = NULL;
                    }
                    else
                    {
                        AsciiSPrint(gBuffer, BUF_SIZE, "WARNING - Unexpected error waiting for CPU #%d to finish: %r", ProcNum, Status);
                        MtSupportDebugWriteLine(gBuffer);
                    }
                }
            }
        }
    } while (bAPFinished == FALSE && ((WaitedMs) < 5000));
}

VOID SyncSetAllMtrrsAllProcs(MTRR_SETTINGS *AllMtrrs)
{
    EFI_EVENT APEvent[MAX_CPUS];
    UINTN TotalProcs = MPSupportGetMaxProcessors();
    UINTN TotalEnabledProcs = 0;
    BOOLEAN bAPFinished = TRUE;
    UINTN ProcNum;
    UINT64 WaitStartTime = 0;

    SetMem(APEvent, sizeof(APEvent), 0);
    for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
    {
        if (MPSupportIsProcEnabled(ProcNum))
            TotalEnabledProcs++;
    }
    barrier_init(&mBarr, (int)TotalProcs);

    if (1 /*gCPUSelMode == CPU_PARALLEL && NumProcsEn > 1*/)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "SyncSetAllMtrrsAllProcs - Set all MTRRs for %d processors", TotalEnabledProcs);

        for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
        {
            if (ProcNum != MPSupportGetBspId() &&
                MPSupportIsProcEnabled(ProcNum))
            {
                EFI_STATUS Status;
                Status = MPSupportSetAllMtrrsAP(ProcNum, &APEvent[ProcNum], AllMtrrs, &mBarr);
                if (EFI_ERROR(Status))
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "SyncSetAllMtrrsAllProcs - Could not set all MTRRs on AP#%d (%r).", ProcNum, Status);
                }
            }
        }
    }

    // barrier(&mBarr); // Sync all CPUs

    AsciiFPrint(DEBUG_FILE_HANDLE, "SyncSetAllMtrrsAllProcs - Set all MTRRs on BSP");

    MTRR_SETTINGS CurMtrrs;
    SetMem(&CurMtrrs, sizeof(CurMtrrs), 0);

    MtrrGetAllMtrrs(&CurMtrrs);
    if (CompareMem(&CurMtrrs, AllMtrrs, sizeof(CurMtrrs)) == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "MtSupportRunAllTests - MTRR registers have not changed. Setting is not required.");
    }
    else
    {
        MtrrSetAllMtrrs(AllMtrrs);
    }

    // barrier(&mBarr); // Sync all CPUS once more

    AsciiFPrint(DEBUG_FILE_HANDLE, "SyncSetAllMtrrsAllProcs - Waiting for APs to finish");

    WaitStartTime = MtSupportGetTimeInMilliseconds();
    UINT64 WaitedMs = 0;
    do
    {
        bAPFinished = TRUE;

        WaitedMs = MtSupportGetTimeInMilliseconds() - WaitStartTime;
        for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
        {
            if (ProcNum != MPSupportGetBspId())
            {
                if (APEvent[ProcNum])
                {
                    EFI_STATUS Status;
                    Status = gBS->CheckEvent(APEvent[ProcNum]);

                    if (Status == EFI_NOT_READY) // CPU finished event has not been signalled
                    {
                        bAPFinished = FALSE;

                        if (WaitedMs > 1000)
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "WARNING - waited for %lds for CPU #%d to finish", DivU64x32(WaitedMs, 1000), ProcNum);
                        }
                    }
                    else if (Status == EFI_SUCCESS) // CPU finished event has been signalled
                    {
                        gBS->CloseEvent(APEvent[ProcNum]);
                        APEvent[ProcNum] = NULL;
                    }
                    else
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "WARNING - Unexpected error waiting for CPU #%d to finish: %r", ProcNum, Status);
                    }
                }
            }
        }
    } while (bAPFinished == FALSE && ((WaitedMs) < 5000));
    AsciiFPrint(DEBUG_FILE_HANDLE, "SyncSetAllMtrrsAllProcs - Finished setting all MTRRs on BSP");
}

VOID
    EFIAPI
    MtSupportEnableCache(BOOLEAN Enable)
{
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    if (Enable)
    {
        AsmEnableCache();
    }
    else
    {
        AsmDisableCache();
    }
#elif defined(MDE_CPU_AARCH64)
    Enable ? ArmEnableDataCache() : ArmDisableDataCache();
#endif
}

int
    EFIAPI
    MtSupportGetTSODInfo(int *DimmTemp, int DimmTempSize)
{
    BOOLEAN bSMBusMUX = FALSE;
    int i;

    if (AsciiStrCmp(gBaseBoardInfo.szProductName, "X8DTU-6+") == 0 ||
        AsciiStrCmp(gBaseBoardInfo.szProductName, "X8DTU-LN4+") == 0 ||
        AsciiStrCmp(gBaseBoardInfo.szProductName, "X8DTN") == 0 ||
        AsciiStrCmp(gBaseBoardInfo.szProductName, "Z8P(N)E-D12(X)") == 0)
        bSMBusMUX = TRUE;

    g_numTSODModules = 0;

    // If we have a valid SMBus address in the 64KB I/O space then search for all slave devices on the SMBus
    for (i = 0; i < g_numTSODCtrl; i++)
    {
        if (g_TSODController[i].tsodDriver != TSOD_NONE)
        {
            int numModules = 0;
#if 0
            AsciiSPrint(gBuffer, BUF_SIZE, "Probing TSOD modules (VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X MMIO Add:%p Rev:%.2X [%s %s])",
                g_TSODController[i].dwVendor_ID, g_TSODController[i].dwDevice_ID,
                g_TSODController[i].byBus, g_TSODController[i].byDev, g_TSODController[i].byFun,
                g_TSODController[i].dwSMB_Address,
                g_TSODController[i].MMCFGBase,
                g_TSODController[i].byRev,
                g_TSODController[i].tszVendor_Name, g_TSODController[i].tszDevice_Name);
            MtSupportDebugWriteLine(gBuffer);
#endif
            switch (g_TSODController[i].tsodDriver)
            {

            case TSOD_INTEL_801:
                numModules = smbGetTSODIntel801((WORD)g_TSODController[i].dwSMB_Address);
                break;

            case TSOD_INTEL_801_SPD5:
                numModules = smbGetTSODIntel801((WORD)g_TSODController[i].dwSMB_Address);
                if (numModules == 0)
                    numModules = smbGetSPD5TSODIntel801((WORD)g_TSODController[i].dwSMB_Address);
                break;

            case TSOD_PIIX4:
                numModules = smbGetTSODPIIX4((WORD)g_TSODController[i].dwSMB_Address);
                if (numModules == 0)
                    numModules = smbGetSPD5TSODPIIX4((WORD)g_TSODController[i].dwSMB_Address);
                break;

            case TSOD_INTEL_E3: // Intel
                numModules = Get_TSOD_Intel_E3_MMIO((g_TSODController[i].dwDevice_ID << 4) | g_TSODController[i].dwVendor_ID, (BYTE)g_TSODController[i].SMB_Address_PCI.dwBus, (PBYTE)g_TSODController[i].MMCFGBase);
                break;

            case TSOD_INTEL_E5: // Intel
                numModules = Get_TSOD_Intel_E5_MMIO((g_TSODController[i].dwDevice_ID << 4) | g_TSODController[i].dwVendor_ID, (BYTE)g_TSODController[i].SMB_Address_PCI.dwBus, (BYTE)g_TSODController[i].SMB_Address_PCI.dwDev, (BYTE)g_TSODController[i].SMB_Address_PCI.dwFun, (PBYTE)g_TSODController[i].MMCFGBase);
                break;

            case TSOD_INTEL_PCU:
                numModules = smbGetTSODIntelPCU(g_TSODController[i].SMB_Address_PCI.dwBus, g_TSODController[i].SMB_Address_PCI.dwDev, g_TSODController[i].SMB_Address_PCI.dwFun);
                break;

            case TSOD_INTEL_PCU_IL:
                numModules = smbGetTSODIntelPCU_IL(g_TSODController[i].SMB_Address_PCI.dwBus, g_TSODController[i].SMB_Address_PCI.dwDev, g_TSODController[i].SMB_Address_PCI.dwFun);
                break;

            default:
                break;
            }
#if 0
            AsciiSPrint(gBuffer, BUF_SIZE, "Probing TSOD modules completed (%d modules found)",
                numModules);
            MtSupportDebugWriteLine(gBuffer);
#endif
        }
    }

    for (i = 0; i < g_numTSODModules; i++)
    {
        if (i < DimmTempSize)
            DimmTemp[i] = g_MemTSODInfo[i].temperature;
    }
    return g_numTSODModules;
}

UINT64 MtSupportMemSizeGB()
{
    const UINT32 BYTES_PER_GB = 1073741824;
    UINT32 Rem;
    UINT64 SizeGB = DivU64x32Remainder(memsize, BYTES_PER_GB, &Rem);

    // Rounding
    if (Rem * 2 >= BYTES_PER_GB)
        SizeGB += 1;
    return SizeGB;
}

UINT64 MtSupportMemSizeMB()
{
    const UINT32 BYTES_PER_MB = 1048576;
    UINT32 Rem;
    UINT64 SizeMB = DivU64x32Remainder(memsize, BYTES_PER_MB, &Rem);

    // Rounding
    if (Rem * 2 >= BYTES_PER_MB)
        SizeMB += 1;
    return SizeMB;
}

static const CHAR16 *SIZE_UNITS_SUFFIX[] =
    {
        L"B",
        L"KB",
        L"MB",
        L"GB",
        L"TB",
        NULL};

CHAR16 *MtSupportHumanReadableSize(UINT64 SizeBytes, CHAR16 *Buffer, UINTN BufferSize)
{
    for (int UnitIdx = 0; SIZE_UNITS_SUFFIX[UnitIdx] != NULL; UnitIdx++)
    {
        UINT64 SizeRem = 0;
        UINT64 UnitBytes = LShiftU64(1, UnitIdx * 10);
        UINT64 SizeUnits = DivU64x64Remainder(SizeBytes, UnitBytes, &SizeRem);
        if (SizeUnits < 1024)
        {
            if (UnitIdx < 2) // No decimal point for B, KB
            {
                UnicodeSPrint(Buffer, BufferSize, L"%ld %s", SizeUnits, SIZE_UNITS_SUFFIX[UnitIdx]);
            }
            else
            {
                UINT64 Decimal = DivU64x64Remainder(MultU64x32(SizeRem, 10), UnitBytes, NULL);
                if (SizeUnits >= 1000 || Decimal == 0)
                    UnicodeSPrint(Buffer, BufferSize, L"%ld %s", SizeUnits, SIZE_UNITS_SUFFIX[UnitIdx]);
                else
                {
                    UnicodeSPrint(Buffer, BufferSize, L"%ld.%ld %s", SizeUnits, Decimal, SIZE_UNITS_SUFFIX[UnitIdx]);
                }
            }

            return Buffer;
        }
    }
    return Buffer;
}

CHAR16 *MtSupportGetSlotLabel(int slot, BOOLEAN LongName, CHAR16 *Buffer, UINTN BufferSize)
{
    BOOLEAN DecodeSupported = get_mem_ctrl_decode_supported() != 0;
    int DimmsPerChannel = MtSupportGetDimmsPerChannel();

    if (DecodeSupported)
    {
        UINT32 family = CPUID_FAMILY((&cpu_id));
        UINT32 model = CPUID_MODEL((&cpu_id));
        CHAR16 MC;

        // ASUS AM4 motherboards physical slot layout is swapped (B1, B2, A1, A2)
        // See https://dlcdnets.asus.com/pub/ASUS/mb/SocketAM4/ROG_STRIX_B550-F_GAMING_WIFI_II/E18592_ROG_STRIX_B550-F_GAMING_WI-FI_II_UM_WEB.pdf?model=ROG%20STRIX%20B550-F%20GAMING%20WIFI%20II
        if ((((family == 0x17 && model >= 0x71 && model <= 0x7f) ||
              (family == 0x19 && model >= 0x21 && model <= 0x2f)) && // AMD Ryzen Zen 2
             AsciiStrStr(gBaseBoardInfo.szManufacturer, "ASUSTeK") != NULL) ||
            // Gigabyte/MSI/BIOSTAR Z890 motherboards physical slot layout is swapped (B1, B2, A1, A2)
            (SysInfo_IsArrowLake(&gCPUInfo) &&
             (AsciiStrStr(gBaseBoardInfo.szManufacturer, "Gigabyte") != NULL ||
              AsciiStrStr(gBaseBoardInfo.szManufacturer, "Micro-Star International") != NULL ||
              AsciiStrStr(gBaseBoardInfo.szManufacturer, "BIOSTAR") != NULL)))
        {
            MC = slot >> 1 == 0 ? L'B' : L'A';
        }
        else
        {
            MC = L'A' + (CHAR16)(slot / DimmsPerChannel);
        }
        const int Dimm = slot % DimmsPerChannel + 1;

        if (LongName)
            UnicodeSPrint(Buffer, BufferSize, L"DIMM %c%d", MC, Dimm);
        else
            UnicodeSPrint(Buffer, BufferSize, L"%c%d", MC, Dimm);
    }
    else if (g_SMBIOSMemDevices[slot].szDeviceLocator[0] != '\0')
        UnicodeSPrint(Buffer, BufferSize, L"%a", g_SMBIOSMemDevices[slot].szDeviceLocator);
    else
        UnicodeSPrint(Buffer, BufferSize, L"%d", slot);
    return Buffer;
}

CHAR16 *MtSupportGetChipLabel(int chip, CHAR16 *Buf, UINTN BufSize)
{
    int BestIdx = 0;
    int BestLevel = 0;
    if (gNumChipMapCfg > 0)
    {
        INT8 ModuleType = -1;
        INT8 FormFactor = -1;
        INT8 NumRanks = -1;
        INT8 ChipWidth = -1;
        INT16 DensityGB = -1;

        for (int i = 0; i < g_numSMBIOSMem; i++)
        {
            if (g_SMBIOSMemDevices[i].smb17.Size == 0)
                continue;

            if (g_SMBIOSMemDevices[i].smb17.Size == 0x7fff) // Need to use Extended Size field
            {
                DensityGB = (INT16)DivU64x32(g_SMBIOSMemDevices[i].smb17.ExtendedSize, 1024);
            }
            else
            {
                if (BitExtract(g_SMBIOSMemDevices[i].smb17.Size, 15, 15) == 0) // MB - bit 15 is the unit MB or KB
                    DensityGB = (INT16)DivU64x32(g_SMBIOSMemDevices[i].smb17.Size, 1024);
                else // KB
                    DensityGB = (INT16)DivU64x32(g_SMBIOSMemDevices[i].smb17.Size, 1048576);
            }

            FormFactor = g_SMBIOSMemDevices[i].smb17.FormFactor;
            if (g_SMBIOSMemDevices[i].smb17.MemoryType == 0x1a)
            {
                ModuleType = SPDINFO_MEMTYPE_DDR4;
            }
            else if (g_SMBIOSMemDevices[i].smb17.MemoryType == 0x22)
            {
                ModuleType = SPDINFO_MEMTYPE_DDR5;
                if (g_numMemModules > 0 && g_MemoryInfo[0].type == ModuleType)
                {
                    switch (g_MemoryInfo[0].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType)
                    {
                    case DDR5_MODULE_TYPE_CUDIMM:
                        FormFactor = ChipMapFormFactorCDimm;
                        break;
                    case DDR5_MODULE_TYPE_CSODIMM:
                        FormFactor = ChipMapFormFactorCSodimm;
                        break;
                    case DDR5_MODULE_TYPE_CAMM2:
                        FormFactor = ChipMapFormFactorCamm2;
                        break;
                    }
                }
            }

#ifdef MEM_DECODE_DEBUG
            DensityGB = 8;
            ModuleType = SPDINFO_MEMTYPE_DDR5;
            FormFactor = ChipMapFormFactorCDimm;
#endif
            NumRanks = (INT8)MtSupportGetNumRanks(i);
            ChipWidth = (INT8)MtSupportGetChipWidth(i);
            break;
        }

        for (int i = 0; i < gNumChipMapCfg; i++)
        {
            int Level = 0;
            if (gChipMapCfg[i].ModuleType >= 0)
            {
                Level++;

                if (gChipMapCfg[i].ModuleType != ModuleType)
                    continue;
            }

            if (gChipMapCfg[i].FormFactor >= 0)
            {
                Level++;

                if (gChipMapCfg[i].FormFactor != FormFactor)
                    continue;
            }

            if (gChipMapCfg[i].Ranks >= 0)
            {
                Level++;

                if (gChipMapCfg[i].Ranks != NumRanks)
                    continue;
            }

            if (gChipMapCfg[i].ChipWidth >= 0)
            {
                Level++;

                if (gChipMapCfg[i].ChipWidth != ChipWidth)
                    continue;
            }

            if (gChipMapCfg[i].DensityGB >= 0)
            {
                Level++;

                if (gChipMapCfg[i].DensityGB != DensityGB)
                    continue;
            }

            if (Level >= BestLevel)
            {
                BestIdx = i;
                BestLevel = Level;
            }
        }
    }
    if (BestIdx >= 0 && BestIdx < gNumChipMapCfg)
    {
        if (chip < MAX_CHIPS)
        {
            AsciiStrToUnicodeStrS(gChipMapCfg[BestIdx].ChipMap[chip], Buf, BufSize / sizeof(Buf[0]));
            return Buf;
        }
    }
    UnicodeSPrint(Buf, BufSize, L"U%d", chip);
    return Buf;
}

BOOLEAN MtSupportECCEnabled()
{
    return get_mem_ctrl_mode() & ECC_ANY;
}

void MtSupportGetECCFeatures(CHAR16 *Buffer, UINTN BufferSize)
{
    Buffer[0] = L'\0';

    UINT32 ECCMode = get_mem_ctrl_mode();
    if (ECCMode & ECC_ANY)
    {
        CHAR16 TempBuf[64];
        CHAR16 ECCFeat[128];

        TempBuf[0] = L'\0';
        ECCFeat[0] = L'\0';
        if (ECCMode & __ECC_CHIPKILL)
            GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_MULTIBIT), TempBuf, sizeof(TempBuf));
        else if (ECCMode & __ECC_CORRECT)
            GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_CORRECT), TempBuf, sizeof(TempBuf));
        else if (ECCMode & __ECC_DETECT)
            GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_DETECT), TempBuf, sizeof(TempBuf));
        StrCatS(ECCFeat, sizeof(ECCFeat) / sizeof(ECCFeat[0]), TempBuf);

        if (ECCMode & __ECC_SCRUB)
        {
            if (ECCFeat[0] != L'\0')
                StrCatS(ECCFeat, sizeof(ECCFeat) / sizeof(ECCFeat[0]), L", ");

            GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_SCRUB), TempBuf, sizeof(TempBuf));
            StrCatS(ECCFeat, sizeof(ECCFeat) / sizeof(ECCFeat[0]), TempBuf);
        }

        if (ECCMode & __ECC_INBAND)
        {
            if (ECCFeat[0] != L'\0')
                StrCatS(ECCFeat, sizeof(ECCFeat) / sizeof(ECCFeat[0]), L", ");

            GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_IBECC), TempBuf, sizeof(TempBuf));
            StrCatS(ECCFeat, sizeof(ECCFeat) / sizeof(ECCFeat[0]), TempBuf);
        }

        UnicodeSPrint(Buffer, BufferSize, L"%s (%s)", GetStringById(STRING_TOKEN(STR_MENU_YES), TempBuf, sizeof(TempBuf)), ECCFeat);
    }
    else if (ECCMode == ECC_NONE)
        GetStringById(STRING_TOKEN(STR_MENU_NO), Buffer, BufferSize);
    else
        GetStringById(STRING_TOKEN(STR_MENU_NA), Buffer, BufferSize);
}

int MtSupportGetNumRanks(int dimm)
{
    if (dimm >= g_numSMBIOSMem)
        return 0;

    if (g_SMBIOSMemDevices[dimm].smb17.Size == 0)
        return 0;

#ifdef MEM_DECODE_DEBUG
    return 2;
#else
    // Check SMBIOS first
    int NumRanks = g_SMBIOSMemDevices[dimm].smb17.Attributes & 0xF; // bits 3-0: rank Value=0 for unknown rank information
    if (NumRanks > 0)
        return NumRanks;

    // Check memory controller
    int mcidx = (g_numSMBIOSMem << 1) == get_mem_ctrl_num_slots() ? (dimm << 1) : dimm; // workaround for unused MC slots (eg. Q670EI-IM-A)
    NumRanks = get_mem_ctrl_num_ranks(mcidx);
    if (NumRanks > 0)
        return NumRanks;

    // Check SPD
    for (int i = 0; i < g_numMemModules; i++)
    {
        if (g_MemoryInfo[i].smbiosInd == dimm)
        {
            NumRanks = g_MemoryInfo[i].numRanks;
            break;
        }
    }
    if (NumRanks > 0 && NumRanks <= 8)
        return NumRanks;

    // Default to 1
    return 1;
#endif
}

int MtSupportGetChipWidth(int dimm)
{
    if (dimm >= g_numSMBIOSMem)
        return 0;

    if (g_SMBIOSMemDevices[dimm].smb17.Size == 0)
        return 0;

#ifdef MEM_DECODE_DEBUG
    return 8;
#else
    // Check memory controller
    int mcidx = (g_numSMBIOSMem << 1) == get_mem_ctrl_num_slots() ? (dimm << 1) : dimm; // workaround for unused MC slots (eg. Q670EI-IM-A)
    int ChipWidth = get_mem_ctrl_chip_width(mcidx);
    if (ChipWidth > 0)
        return ChipWidth;

    // Check SPD
    for (int i = 0; i < g_numMemModules; i++)
    {
        if (g_MemoryInfo[i].smbiosInd == dimm)
        {
            ChipWidth = g_MemoryInfo[i].deviceWidth;
            break;
        }
    }

    if (numsetbits32(ChipWidth) == 1 && ChipWidth >= 4 && ChipWidth <= 64) // Should be power of 2
        return ChipWidth;

    // Default to 8
    return 8;
#endif
}

UINT8 MtSupportGetFormFactor(int dimm)
{
    if (dimm >= g_numSMBIOSMem)
        return ChipMapFormFactorUnknown;

    if (g_SMBIOSMemDevices[dimm].smb17.Size == 0)
        return ChipMapFormFactorUnknown;

    if (g_SMBIOSMemDevices[dimm].smb17.MemoryType == 0x22)
    {
        for (int i = 0; i < g_numMemModules; i++)
        {
            if (g_MemoryInfo[i].smbiosInd == dimm)
            {
                switch (g_MemoryInfo[i].specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType)
                {
                case DDR5_MODULE_TYPE_CUDIMM:
                    return ChipMapFormFactorCDimm;
                    break;
                case DDR5_MODULE_TYPE_CSODIMM:
                    return ChipMapFormFactorCSodimm;
                    break;
                case DDR5_MODULE_TYPE_CAMM2:
                    return ChipMapFormFactorCamm2;
                    break;
                case DDR5_MODULE_TYPE_RDIMM:
                case DDR5_MODULE_TYPE_UDIMM:
                case DDR5_MODULE_TYPE_LRDIMM:
                case DDR5_MODULE_TYPE_MRDIMM:
                case DDR5_MODULE_TYPE_DDIMM:
                    return ChipMapFormFactorDimm;
                    break;

                case DDR5_MODULE_TYPE_SODIMM:
                    return ChipMapFormFactorSodimm;

                case DDR5_MODULE_TYPE_SOLDER_DOWN:
                    return ChipMapFormFactorRowOfChips;
                    break;

                default:
                    return g_SMBIOSMemDevices[dimm].smb17.FormFactor;
                    break;
                }
            }
        }
    }
    return g_SMBIOSMemDevices[dimm].smb17.FormFactor;
}

UINT8 MtSupportGetMemoryFormFactor()
{
    for (int i = 0; i < g_numSMBIOSMem; i++)
    {
        if (g_SMBIOSMemDevices[i].smb17.Size > 0)
            return MtSupportGetFormFactor(i);
    }
    return ChipMapFormFactorUnknown;
}

UINT8 MtSupportGetDimmsPerChannel()
{
    return get_mem_ctrl_num_dimms_per_channel() > 1 && (g_numSMBIOSMem << 1) <= get_mem_ctrl_num_slots() ? (UINT8)(get_mem_ctrl_num_dimms_per_channel() >> 1) : (UINT8)get_mem_ctrl_num_dimms_per_channel(); // workaround for unused MC slots (eg. Q670EI-IM-A)
}

CHAR16 *MtSupportGetReportPrefix(CHAR16 *Buffer, UINTN BufferSize)
{
    StrCpyS(Buffer, BufferSize / sizeof(Buffer[0]), gReportPrepend);
    if (Buffer[0] != L'\0')
        StrCatS(Buffer, BufferSize / sizeof(Buffer[0]), L"-");

    switch (gReportPrefix)
    {
    case REPORT_PREFIX_BASEBOARDSN:
        UnicodeCatf(Buffer, BufferSize, L"%a-", gBaseBoardInfo.szSerialNumber);
        break;
    case REPORT_PREFIX_SYSINFOSN:
        UnicodeCatf(Buffer, BufferSize, L"%a-", gSystemInfo.szSerialNumber);
        break;
    case REPORT_PREFIX_RAMSN:
    {
        if (g_numMemModules > 0)
        {
            UnicodeCatf(Buffer, BufferSize, L"%08X-", g_MemoryInfo[0].moduleSerialNo);
        }
        else
        {
            for (int i = 0; i < g_numSMBIOSMem; i++)
            {
                if (g_SMBIOSMemDevices[i].smb17.Size != 0)
                {
                    UnicodeCatf(Buffer, BufferSize, L"%a-", g_SMBIOSMemDevices[i].szSerialNumber);
                    break;
                }
            }
        }
    }
    break;

    case REPORT_PREFIX_MACADDR:
        UnicodeCatf(Buffer, BufferSize, L"%02x-%02x-%02x-%02x-%02x-%02x-",
                    ClientMAC.Addr[0], ClientMAC.Addr[1], ClientMAC.Addr[2], ClientMAC.Addr[3], ClientMAC.Addr[4], ClientMAC.Addr[5]);
        break;
    default:
#ifdef SITE_EDITION
        if (gBaseBoardInfo.szSerialNumber[0] != '\0' &&
            AsciiStrStr(gBaseBoardInfo.szSerialNumber, "Default") == NULL &&
            AsciiStrStr(gBaseBoardInfo.szSerialNumber, "String") == NULL &&
            AsciiStrStr(gBaseBoardInfo.szSerialNumber, "string") == NULL &&
            AsciiStrStr(gBaseBoardInfo.szSerialNumber, "Empty") == NULL &&
            AsciiStrStr(gBaseBoardInfo.szSerialNumber, "None") == NULL)
            UnicodeCatf(Buffer, BufferSize, L"%a-", gBaseBoardInfo.szSerialNumber);
#endif
        break;
    }
    RemoveInvalidFilenameChars(Buffer, BufferSize);

    return Buffer;
}

CHAR16 *MtSupportGetTimestampStr(EFI_TIME *Time)
{
    static CHAR16 Timestamp[64];

    UnicodeSPrint(Timestamp, sizeof(Timestamp), L"%d%02d%02d-%02d%02d%02d_%06d", Time->Year, Time->Month, Time->Day, Time->Hour, Time->Minute, Time->Second, ModU64x32(MtSupportReadTSC(), 1000000));

    return Timestamp;
}

BOOLEAN WildCardCompare(CHAR8 *pTameText,   // A string without wildcards
                        CHAR8 *pWildText,   // A (potentially) corresponding string with wildcards
                        bool bCaseSensitive // By default, match on 'X' vs 'x'
)
{
    BOOLEAN bMatch = true;
    CHAR8 *pAfterLastWild = NULL; // The location after the last '*', if we've encountered one
    CHAR8 *pAfterLastTame = NULL; // The location in the tame string, from which we started after last wildcard
    CHAR8 t, w;

    // Walk the text strings one character at a time.
    while (1)
    {
        t = *pTameText;
        w = *pWildText;

        // How do you match a unique text string?
        if (!t)
        {
            // Easy: unique up on it!
            if (!w)
            {
                break; // "x" matches "x"
            }
            else if (w == '*')
            {
                pWildText++;
                continue; // "x*" matches "x" or "xy"
            }
            else if (pAfterLastTame)
            {
                if (!(*pAfterLastTame))
                {
                    bMatch = false;
                    break;
                }
                pTameText = pAfterLastTame++;
                pWildText = pAfterLastWild;
                continue;
            }

            bMatch = false;
            break; // "x" doesn't match "xy"
        }
        else
        {
            if (!bCaseSensitive)
            {
                // Lowercase the characters to be compared.
                if (t >= 'A' && t <= 'Z')
                {
                    t += 'a' - 'A';
                }

                if (w >= 'A' && w <= 'Z')
                {
                    w += 'a' - 'A';
                }
            }

            // How do you match a tame text string?
            if (t != w)
            {
                // The tame way: unique up on it!
                if (w == '*')
                {
                    pAfterLastWild = ++pWildText;
                    pAfterLastTame = pTameText;
                    w = *pWildText;

                    if (!w)
                    {
                        break; // "*" matches "x"
                    }
                    continue; // "*y" matches "xy"
                }
                else if (pAfterLastWild)
                {
                    if (pAfterLastWild != pWildText)
                    {
                        pWildText = pAfterLastWild;
                        w = *pWildText;

                        if (!bCaseSensitive && w >= 'A' && w <= 'Z')
                        {
                            w += 'a' - 'A';
                        }

                        if (t == w)
                        {
                            pWildText++;
                        }
                    }
                    pTameText++;
                    continue; // "*sip*" matches "mississippi"
                }
                else
                {
                    bMatch = false;
                    break; // "x" doesn't match "y"
                }
            }
        }
        pTameText++;
        pWildText++;
    }
    return bMatch;
}

BOOLEAN
EFIAPI
MtSupportCheckMemSpeed(CHAR16 *CfgSpeedBuf, UINTN CfgSpeedBufSize, CHAR16 *CheckSpeedBuf, UINTN CheckSpeedBufSize)
{
    if (gNumCheckMemSpeed == 0)
        return TRUE;

    if (g_numMemModules == 0)
        return FALSE;

    for (int iCheckInd = 0; iCheckInd < gNumCheckMemSpeed; iCheckInd++)
    {
        if (gCheckMemSpeed[iCheckInd].CheckSpeed == MEMSPEED_OFF)
            continue;

        for (int iSPD = 0; iSPD < g_numMemModules; iSPD++)
        {
            if (WildCardCompare(g_MemoryInfo[iSPD].modulePartNo, gCheckMemSpeed[iCheckInd].PartNo, TRUE))
            {
                FPrint(DEBUG_FILE_HANDLE, L"mem speed check - found SPD P/N (%a) matching match string (%a)", g_MemoryInfo[iSPD].modulePartNo, gCheckMemSpeed[iCheckInd].PartNo);

                // Query the configured memory speed/timings
                int CfgSpeed = 0;
                unsigned int memclk;
                unsigned char cas = 0, rcd = 0, rp = 0, ras = 0;
                if (get_mem_ctrl_timings(&memclk, NULL, &cas, &rcd, &rp, &ras) == 0) // check memory controller first
                {
                    CfgSpeed = memclk * 2;
                    FPrint(DEBUG_FILE_HANDLE, L"mem speed check - detected speed (ctrl): %d MT/s (%d-%d-%d-%d)", CfgSpeed, cas, rcd, rp, ras);
                }
                else // otherwise, check SMBIOS
                {
                    for (int iSMB = 0; iSMB < g_numSMBIOSMem; iSMB++)
                    {
                        if (g_SMBIOSMemDevices[iSMB].smb17.Size != 0)
                        {
                            if (g_SMBIOSMemDevices[iSMB].smb17.ConfiguredMemoryClockSpeed == 0xffff) // Need to use Extended Speed field
                                CfgSpeed = g_SMBIOSMemDevices[iSMB].smb17.ExtendedConfiguredMemorySpeed;
                            else
                                CfgSpeed = g_SMBIOSMemDevices[iSMB].smb17.ConfiguredMemoryClockSpeed;

                            FPrint(DEBUG_FILE_HANDLE, L"mem speed check - detected speed (smbios): %d MT/s", CfgSpeed);
                            break;
                        }
                    }
                }

                UnicodeSPrint(CfgSpeedBuf, CfgSpeedBufSize, L"%d MT/s", CfgSpeed);
                if (cas != 0)
                {
                    UnicodeCatf(CfgSpeedBuf, CfgSpeedBufSize, L" %d-%d-%d-%d", cas, rcd, rp, ras);
                }

                if (gCheckMemSpeed[iCheckInd].CheckSpeed > 0)
                {
                    FPrint(DEBUG_FILE_HANDLE, L"mem speed check - checking with min speed: %d MT/s", gCheckMemSpeed[iCheckInd].CheckSpeed);

                    UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"MIN %d MT/s", gCheckMemSpeed[iCheckInd].CheckSpeed);
                    return CfgSpeed >= (gCheckMemSpeed[iCheckInd].CheckSpeed - 50);
                }
                else
                {
                    int CheckSpeed = 0;
                    int tCK = 0;
                    int CheckCAS = 0, CheckRCD = 0, CheckRP = 0, CheckRAS = 0;

                    switch (gCheckMemSpeed[iCheckInd].CheckSpeed)
                    {
                    case MEMSPEED_MAX:
                    {
                        int clkspeed;
                        getMaxSPDInfo(&g_MemoryInfo[iSPD], &tCK, &clkspeed, &CheckCAS, &CheckRCD, &CheckRP, &CheckRAS, NULL);
                        CheckSpeed = clkspeed * 2;

                        UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"MAX profile");
                    }
                    break;

                    case MEMSPEED_JEDEC:
                        CheckSpeed = g_MemoryInfo[iSPD].clkspeed * 2;
                        tCK = g_MemoryInfo[iSPD].tCK;
                        CheckCAS = g_MemoryInfo[iSPD].tAA;
                        CheckRCD = g_MemoryInfo[iSPD].tRCD;
                        CheckRP = g_MemoryInfo[iSPD].tRP;
                        CheckRAS = g_MemoryInfo[iSPD].tRAS;

                        UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"JEDEC profile");
                        break;

                    case MEMSPEED_XMP1:
                        if (!g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[0].enabled)
                        {
                            FPrint(DEBUG_FILE_HANDLE, L"mem speed check - XMP profile 1 is unavailable");
                            UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"XMP profile 1 (N/A)");
                            return FALSE;
                        }
                        CheckSpeed = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[0].clkspeed * 2;
                        tCK = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[0].tCK;
                        CheckCAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[0].tAA;
                        CheckRCD = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[0].tRCD;
                        CheckRP = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[0].tRP;
                        CheckRAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[0].tRAS;

                        UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"XMP profile 1");
                        break;

                    case MEMSPEED_XMP2:
                        if (!g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[1].enabled)
                        {
                            FPrint(DEBUG_FILE_HANDLE, L"mem speed check - XMP profile 2 is unavailable");
                            UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"XMP profile 2 (N/A)");
                            return FALSE;
                        }
                        CheckSpeed = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[1].clkspeed * 2;
                        tCK = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[1].tCK;
                        CheckCAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[1].tAA;
                        CheckRCD = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[1].tRCD;
                        CheckRP = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[1].tRP;
                        CheckRAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.XMP.profile[1].tRAS;

                        UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"XMP profile 2");
                        break;

                    case MEMSPEED_EXPO1:
                        if (!g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[0].enabled)
                        {
                            FPrint(DEBUG_FILE_HANDLE, L"mem speed check - EXPO profile 1 is unavailable");
                            UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"EXPO profile 1 (N/A)");
                            return FALSE;
                        }
                        CheckSpeed = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[0].clkspeed * 2;
                        tCK = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[0].tCK;
                        CheckCAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[0].tAA;
                        CheckRCD = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[0].tRCD;
                        CheckRP = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[0].tRP;
                        CheckRAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[0].tRAS;

                        UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"EXPO profile 1");
                        break;

                    case MEMSPEED_EXPO2:
                        if (!g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[1].enabled)
                        {
                            FPrint(DEBUG_FILE_HANDLE, L"mem speed check - EXPO profile 2 is unavailable");
                            UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"EXPO profile 2 (N/A)");
                            return FALSE;
                        }
                        CheckSpeed = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[1].clkspeed * 2;
                        tCK = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[1].tCK;
                        CheckCAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[1].tAA;
                        CheckRCD = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[1].tRCD;
                        CheckRP = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[1].tRP;
                        CheckRAS = g_MemoryInfo[iSPD].specific.DDR5SDRAM.EXPO.profile[1].tRAS;

                        UnicodeSPrint(CheckSpeedBuf, CheckSpeedBufSize, L"EXPO profile 2");
                        break;

                    default:
                        break;
                    }

                    CHAR16 szTimings[32];
                    getTimingsStr(g_MemoryInfo[iSPD].type, tCK, CheckCAS, CheckRCD, CheckRP, CheckRAS, szTimings, sizeof(szTimings));
                    UnicodeCatf(CheckSpeedBuf, CheckSpeedBufSize, L" (%d MT/s %s)", CheckSpeed, szTimings);
                    FPrint(DEBUG_FILE_HANDLE, L"mem speed check - checking with %s", CheckSpeedBuf);

                    INT64 ErrorMargin100 = MultS64x64(CheckSpeed, MEM_SPEED_MARGIN_OF_ERROR);
                    BOOLEAN SpeedOK = MultS64x64(ABS(CfgSpeed - CheckSpeed), 100) < ErrorMargin100;

                    convertMemTimings(g_MemoryInfo[iSPD].type, tCK, &CheckCAS, &CheckRCD, &CheckRP, &CheckRAS);
                    BOOLEAN CASOK = cas == 0 || CheckCAS == 0 || cas == CheckCAS;

                    return SpeedOK && CASOK;
                }
            }
        }
    }
    return TRUE;
}

BOOLEAN
EFIAPI
MtSupportSkipDIMMResults()
{
    if (gConsoleOnly != FALSE)
        return TRUE;

    switch (gSkipDecode)
    {
    case SKIPDECODE_NEVER:
        return FALSE;
    case SKIPDECODE_ALL:
        return TRUE;
    case SKIPDECODE_DDR4:
    case SKIPDECODE_DDR5:
    {
        for (int i = 0; i < g_numSMBIOSMem; i++)
        {
            if (g_SMBIOSMemDevices[i].smb17.MemoryType == 0x1a || // DDR4
                g_SMBIOSMemDevices[i].smb17.MemoryType == 0x1e)   // LPDDR4
                return gSkipDecode == SKIPDECODE_DDR4;
            else if (g_SMBIOSMemDevices[i].smb17.MemoryType == 0x22 || // DDR5
                     g_SMBIOSMemDevices[i].smb17.MemoryType == 0x23)   // LPDDR5
                return gSkipDecode == SKIPDECODE_DDR5;
        }
        return FALSE;
    }
    break;
    }
    return FALSE;
}

CHAR16 *BytesToStrHex(const unsigned char *Bytes, UINTN BytesLen, CHAR16 *Buf, UINTN BufLen)
{
    if (Bytes == NULL || BytesLen == 0 || Buf == NULL || BufLen == 0)
        return L"";

    Buf[0] = L'\0';

    for (UINTN i = 0; i < BytesLen; i++)
    {
        UnicodeCatf(Buf, BufLen, L"%02X", Bytes[i]);
    }
    return Buf;
}

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
/**
  Worker function prints all MTRRs for debugging.

  If MtrrSetting is not NULL, print MTRR settings from input MTRR
  settings buffer.
  If MtrrSetting is NULL, print MTRR settings from MTRRs.

  @param  MtrrSetting    A buffer holding all MTRRs content.
**/
VOID LogAllMtrrs(
    IN MTRR_SETTINGS *MtrrSetting)
{
    UINT32 Index;
    MTRR_SETTINGS LocalMtrrs;
    MTRR_SETTINGS *Mtrrs;
    RETURN_STATUS Status;
    UINTN RangeCount;
    BOOLEAN ContainVariableMtrr;
    MTRR_MEMORY_RANGE Ranges[MTRR_NUMBER_OF_FIXED_MTRR * sizeof(UINT64) + 2 * ARRAY_SIZE(Mtrrs->Variables.Mtrr) + 1];

    if (MtrrSetting != NULL)
    {
        Mtrrs = MtrrSetting;
    }
    else
    {
        MtrrGetAllMtrrs(&LocalMtrrs);
        Mtrrs = &LocalMtrrs;
    }

    RangeCount = ARRAY_SIZE(Ranges);
    Status = MtrrGetMemoryAttributesInMtrrSettings(Mtrrs, Ranges, &RangeCount);
    if (RETURN_ERROR(Status))
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "MTRR is not enabled.\n");
        return;
    }

    //
    // Dump RAW MTRR contents
    //
    AsciiFPrint(DEBUG_FILE_HANDLE, "MTRR Settings:\n");
    AsciiFPrint(DEBUG_FILE_HANDLE, "=============\n");
    AsciiFPrint(DEBUG_FILE_HANDLE, "MTRR Default Type: %016lx\n", Mtrrs->MtrrDefType);
    for (Index = 0; Index < MTRR_NUMBER_OF_FIXED_MTRR; Index++)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Fixed MTRR[%02d]   : %016lx\n", Index, Mtrrs->Fixed.Mtrr[Index]);
    }

    ContainVariableMtrr = FALSE;
    for (Index = 0; Index < ARRAY_SIZE(Mtrrs->Variables.Mtrr); Index++)
    {
        if ((Mtrrs->Variables.Mtrr[Index].Mask & BIT11) == 0)
        {
            //
            // If mask is not valid, then do not display range
            //
            continue;
        }

        ContainVariableMtrr = TRUE;
        AsciiFPrint(DEBUG_FILE_HANDLE,
                    "Variable MTRR[%02d]: Base=%016lx Mask=%016lx\n",
                    Index,
                    Mtrrs->Variables.Mtrr[Index].Base,
                    Mtrrs->Variables.Mtrr[Index].Mask);
    }

    if (!ContainVariableMtrr)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Variable MTRR    : None.\n");
    }

    AsciiFPrint(DEBUG_FILE_HANDLE, "\n");

    AsciiFPrint(DEBUG_FILE_HANDLE, "Available Variable MTRR    : %d\n", GetFirmwareVariableMtrrCount());

    AsciiFPrint(DEBUG_FILE_HANDLE, "\n");

    //
    // Dump MTRR setting in ranges
    //
    AsciiFPrint(DEBUG_FILE_HANDLE, "Memory Ranges:\n");
    AsciiFPrint(DEBUG_FILE_HANDLE, "====================================\n");
    for (Index = 0; Index < RangeCount; Index++)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE,
                    "%a:%016lx-%016lx\n",
                    mMtrrMemoryCacheTypeShortName[Ranges[Index].Type],
                    Ranges[Index].BaseAddress,
                    Ranges[Index].BaseAddress + Ranges[Index].Length - 1);
    }
}

#endif
