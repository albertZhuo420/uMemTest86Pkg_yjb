// PassMark MemTest86
//
// Copyright (c) 2013-2024
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
//	uMemTest86.h
//
// Author(s):
//	Keith Mah
//
// Description:
//	Defines and declarations for uMemTest86.c
//
// References:
//   https://github.com/jljusten/MemTestPkg
//
// History
//	27 Feb 2013: Initial version

/** @file
  MemTest EFI Shell Application.

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

#ifndef _MEM_TEST_H_INCLUDED_
#define _MEM_TEST_H_INCLUDED_

#ifndef OKN_MT86
#define OKN_MT86
#endif

// #define CUSTOM_HP_BUILD

// Application
#define PROGRAM_NAME L"MemTest86" // Name of the application

#define STRINGIFY(x) L## #x
#define TOSTR(x) STRINGIFY(x)

#ifdef CUSTOM_HP_BUILD // Custom build for HP
#define PROGRAM_VERSION_MAJOR 6
#define PROGRAM_VERSION_MINOR 1
#define PROGRAM_VERSION_BUILD 1000
#define PROGRAM_VERSION L"V6.1.hp Pro" // Version of application
#define PROGRAM_BUILD L"hp.1000"       // Build number within this version
#else
#define PROGRAM_VERSION_MAJOR 11   // Update for new major version
#define PROGRAM_VERSION_MINOR 5    // Update for new minor version
#define PROGRAM_VERSION_BUILD 1000 // Update for new build
#define PROGRAM_VERSION_SPECIAL L""
#ifdef PRO_RELEASE
#define PROGRAM_EDITION L"Pro"
#elif defined(SITE_EDITION)
#define PROGRAM_EDITION L"Site"
#define PXEBOOT
#define PRO_RELEASE
// #define TRIAL
#else
#define PROGRAM_EDITION L"Free"
#endif
#define PROGRAM_VERSION L"V" TOSTR(PROGRAM_VERSION_MAJOR) L"." TOSTR(PROGRAM_VERSION_MINOR) L" " PROGRAM_EDITION // Version string
#define PROGRAM_BUILD TOSTR(PROGRAM_VERSION_BUILD) PROGRAM_VERSION_SPECIAL                                       // Build number string
// #define DDR5_DEBUG
// #define MEM_DECODE_DEBUG
// #define TCPIP_DEBUG
// #define TSOD_DEBUG
// #define PXE_DEBUG
#endif

#define CONFIG_FILENAME L"mt86.cfg"           // configuration filename
#define BLACKLIST_FILENAME L"blacklist.cfg"   // blacklist filename
#define SPD_FILENAME L"SPD.spd"               // SPD filename
#define TIMEOFDAY_FILENAME L"CurrentTime.txt" // time-of-day filename
#define DEBUG_BASE_FILENAME L"MemTest86"      // log filename
#define SYSINFO_BASE_FILENAME L"mt86SysInfo"  // base filename for system information summary (eg. mt86SysInfo-20160819-082007.txt)
#define RAMINFO_FILENAME L"MemTest86-RAMInfo.txt"
#define RAMINFO_BASE_FILENAME L"mt86RAMInfo"      // base filename for RAM information (eg. mt86RAMInfo-20160819-082007.txt)
#define REPORT_BASE_FILENAME L"MemTest86-Report"  // base filename for test report (eg. MemTest86-Report-20160819-082007.html)
#define DIMM_RESULTS_BASE_FILENAME L"DIMMResults" // base filename for DIMM results (eg. DIMMResults-20160819-082007.html)
#define BENCH_DIR L"Benchmark"                    // RAM benchmrk results directory
#define CLIENT_DIR L"Clients"                     // Clients directory
#define BENCH_BASE_FILENAME L"mt86Bench"          // base filename for RAM benchmark report (eg. mt86Bench-20160819-082007.html)
#define SCREENSHOT_BASE_FILENAME L"mt86Screen"    // base filename for screenshot

#define CSS_FILENAME L"report.css"           // Customizable report .css file
#define CERT_HEADER_FILENAME L"mt86head.htm" // Customizable report header file
#define CERT_FOOTER_FILENAME L"mt86foot.htm" // Customizable report footer file

#define UNICODE_FONT_FILENAME L"unifont.bin" // Unifont font file to load if one or more languages are unsupported by the system font

#ifndef OKN_MT86
#define DEFAULT_NUM_PASSES 4 // Default number of passes to run
#else
#define DEFAULT_NUM_PASSES 1
#endif

#define DEFAULT_REPORT_NUM_ERRORS 10 // Default number of errors to include in the report
#define MAX_REPORT_NUM_ERRORS 5000   // Max number of errors to include in the report

#define DEFAULT_REPORT_NUM_WARNINGS 0 // Default number of warnings to include in the report
#define MAX_REPORT_NUM_WARNINGS 5000  // Max number of warnings to include in the report

#define DEFAULT_MAXERRCOUNT 10000 // Max number of errors before the tests are aborted

#define DEFAULT_TFTP_STATUS_SECS 60 // Default management console status report period
#define MIN_TFTP_STATUS_SECS 10     // Minimum management console status report period
#define MAX_TFTP_STATUS_SECS 600    // Maximum management console status report period

// Hard limit defined in MemTest86. Can increase but needs testing first
#ifdef PRO_RELEASE
#define MAX_CPUS 512
#define DEF_CPUS 128
#else
#define MAX_CPUS 16
#define DEF_CPUS 16
#endif
#define MEM_SPD_MARGIN_OF_ERROR 5   // Percent margin of error between system memory and total capacity of detected SPDs
#define MEM_SPEED_MARGIN_OF_ERROR 1 // Percent margin of error between configured memory speed and check speed

#define MAX_SLOTS 8
#define MAX_CHIPS 16

#define MAX_NUM_SPDS 4

#if defined(_MSC_VER)
#define DISABLE_OPTIMISATIONS() __pragma(optimize("", off))
#define ENABLE_OPTIMISATIONS() __pragma(optimize("", on))
#elif defined(__GNUC__)
#define DISABLE_OPTIMISATIONS() \
  _Pragma("GCC push_options")   \
      _Pragma("GCC optimize (\"O0\")")
#define ENABLE_OPTIMISATIONS() _Pragma("GCC pop_options")
#elif defined(__clang__)
#define DISABLE_OPTIMISATIONS() _Pragma("clang optimize off")
#define ENABLE_OPTIMISATIONS() _Pragma("clang optimize on")
#else
#error new compiler
#endif

#define BitExtract(dwReg, wIndexHigh, dwIndexLow) ((dwReg) >> (dwIndexLow) & ((1 << ((wIndexHigh) - (dwIndexLow) + 1)) - 1))
#define BitExtractULL(dwReg, wIndexHigh, dwIndexLow) (RShiftU64((UINT64)(dwReg), dwIndexLow) & (LShiftU64(1, ((wIndexHigh) - (dwIndexLow) + 1)) - 1))

// For debug logging
#define BUF_SIZE 1024
extern char gBuffer[BUF_SIZE];
extern CHAR16 g_wszBuffer[BUF_SIZE];

#include <Uefi.h>

/* Test details */
typedef enum
{
  TEST_ADDRWALK1 = 0,
  TEST_ADDROWN = 2,
  TEST_MOVINV8 = 3,
  TEST_BLKMOV = 6,
  TEST_MOVINV32 = 7,
  TEST_RNDNUM32 = 8,
  TEST_MOD20 = 9,
  TEST_BITFADE = 10,
  TEST_RNDNUM64 = 11,
  TEST_RNDNUM128 = 12,
  TEST_HAMMER = 13,
  TEST_DMA = 14
} TESTTYPE;

typedef enum
{
  PAT_DEF,
  PAT_RAND,
  PAT_WALK01,
  PAT_USER
} TESTPAT;

typedef enum
{
  CACHEMODE_DISABLE = 0,
  CACHEMODE_ENABLE = 1,
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
  CACHEMODE_WC,
  CACHEMODE_WT,
  CACHEMODE_WP,
  CACHEMODE_WB,
  CACHEMODE_UC = CACHEMODE_DISABLE,
  CACHEMODE_DEFAULT = CACHEMODE_ENABLE,
  CACHEMODE_MIN = CACHEMODE_DISABLE,
  CACHEMODE_MAX = CACHEMODE_WB
#else
  CACHEMODE_DEFAULT = CACHEMODE_ENABLE,
  CACHEMODE_MIN = CACHEMODE_DISABLE,
  CACHEMODE_MAX = CACHEMODE_ENABLE
#endif
} CACHEMODE;

// Define memory test function typedef
typedef EFI_STATUS(EFIAPI *TEST_MEM_RANGE)(
    IN UINT32 TestNum,
    IN UINTN ProcNum,
    IN EFI_PHYSICAL_ADDRESS Start,
    IN UINT64 Length,
    IN UINT64 IterCount,
    IN TESTPAT Pattern,
    IN UINT64 PatternUser,
    IN VOID *Context);

#define MAX_TEST_NAME 64

typedef struct _TESTCFG
{
  BOOLEAN Enabled;
  UINT32 TestNo;
  CHAR16 *NameStr;
  EFI_STRING_ID NameStrId;
  TEST_MEM_RANGE RunMemTest1;
  TEST_MEM_RANGE RunMemTest2;
  TEST_MEM_RANGE RunMemTest3;
  TEST_MEM_RANGE RunMemTest4;
  UINT32 MaxCPUs;
  UINT64 Iterations;
  UINT32 Align;
  TESTPAT Pattern;
  UINT64 PatternUser;
  CACHEMODE CacheMode;
  VOID *Context;
} TESTCFG;

#ifdef TRIAL
#define NUMTEST 5
#elif defined(PRO_RELEASE)
#define NUMTEST 15
#else
#define NUMTEST 13
#endif
#define MAXTESTS 20
extern TESTCFG gTestList[NUMTEST];

enum
{
  RESTRICT_FLAG_STARTUP = (1 << 0),
  RESTRICT_FLAG_MP = (1 << 1),
  RESTRICT_FLAG_MPDISABLE = (1 << 2),
  RESTRICT_FLAG_CONCTRLDISABLE = (1 << 3),
  RESTRICT_FLAG_FIXED_SCREENRES = (1 << 4),
  RESTRICT_FLAG_RESTRICT_ADDR = (1 << 5),
  RESTRICT_FLAG_TEST12_SINGLECPU = (1 << 6),
  RESTRICT_FLAG_LANGDISABLE = (1 << 7),
  RESTRICT_FLAG_CPUINFO = (1 << 8),
};
typedef struct _BLACKLISTENT
{
  char szBaseboardName[65];
  char szBIOSVersion[65];
  BOOLEAN PartialMatch;
  UINT32 RestrictFlags;
} BLACKLISTENT;

typedef struct _BLACKLIST
{
  int iBlacklistLen;        // Number of entries in blacklist
  int iBlacklistSize;       // Size of gBlacklist structure
  BLACKLISTENT *pBlacklist; // gBlacklist read from blacklist.cfg
} BLACKLIST;

#define MAX_CHIPMAPCFG 32

#define MAX_CHIP_NAME_LEN 6
typedef CHAR8 CHIPSTR[MAX_CHIP_NAME_LEN + 1];

typedef enum
{
  ChipMapFormFactorOther = 0x01,
  ChipMapFormFactorUnknown = 0x02,
  ChipMapFormFactorSimm = 0x03,
  ChipMapFormFactorSip = 0x04,
  ChipMapFormFactorChip = 0x05,
  ChipMapFormFactorDip = 0x06,
  ChipMapFormFactorZip = 0x07,
  ChipMapFormFactorProprietaryCard = 0x08,
  ChipMapFormFactorDimm = 0x09,
  ChipMapFormFactorTsop = 0x0A,
  ChipMapFormFactorRowOfChips = 0x0B,
  ChipMapFormFactorRimm = 0x0C,
  ChipMapFormFactorSodimm = 0x0D,
  ChipMapFormFactorSrimm = 0x0E,
  ChipMapFormFactorFbDimm = 0x0F,
  ChipMapFormFactorDie = 0x10,
  ChipMapFormFactorCDimm = 0x11,
  ChipMapFormFactorCSodimm = 0x12,
  ChipMapFormFactorCamm2 = 0x13,
} CHIPMAP_FORM_FACTOR;

typedef struct _CHIPMAPCFG
{
  INT8 ModuleType;
  INT8 FormFactor;
  INT8 Ranks;
  INT8 ChipWidth;
  INT16 DensityGB;
  CHIPSTR ChipMap[MAX_CHIPS];
} CHIPMAPCFG;

#define MAX_CHECKMEMSPEED 32

#define MAX_PARTNO_STR 32
typedef CHAR8 PARTNOSTR[MAX_PARTNO_STR + 1];

#define MAX_PREPEND_LEN 32
enum
{
  MEMSPEED_OFF = 0,
  MEMSPEED_MAX = -1,
  MEMSPEED_JEDEC = -2,
  MEMSPEED_XMP1 = -3,
  MEMSPEED_XMP2 = -4,
  MEMSPEED_EXPO1 = -5,
  MEMSPEED_EXPO2 = -6
};

typedef struct _CHECKMEMSPEED
{
  PARTNOSTR PartNo;
  int CheckSpeed;
} CHECKMEMSPEED;

#define MAX_NUM_CONFIG 10
#define MAX_CONFIG_NAME_LEN 80
typedef struct _MTCFG
{
  CHAR16 Name[MAX_CONFIG_NAME_LEN + 1];
  UINT64 AddrLimitLo;
  UINT64 AddrLimitHi;
  UINT64 MemRemMB;
  UINT64 MinMemRangeMB;
  UINTN CPUSelMode;
  UINTN SelCPUNo;
  BOOLEAN CPUList[MAX_CPUS];
  UINTN MaxCPUs;
  UINTN NumPasses;
  BOOLEAN ECCPoll;
  BOOLEAN ECCInject;
  BOOLEAN TSODPoll;
  UINTN CacheMode;
  BOOLEAN TestSelected[MAXTESTS];
  CHAR16 TestCfgFile[32];
  BOOLEAN Pass1Full;
  UINT64 Addr2ChBits;
  UINT64 Addr2SlBits;
  UINT64 Addr2CSBits;
  CHIPMAPCFG ChipMapCfg[MAX_CHIPMAPCFG];
  int NumChipMapCfg;
  int CurLang;
  UINTN ReportNumErr;
  UINTN ReportNumWarn;
  UINTN AutoMode;
  BOOLEAN AutoReport;
  UINTN AutoReportFmt;
  BOOLEAN AutoPromptFail;
  CHAR16 TCPServerIP[32];
  CHAR16 TCPRequestLocation[2048];
  CHAR16 TCPGatewayIP[32];
  UINT32 TCPServerPort;
  CHAR16 TCPClientIP[32];
  BOOLEAN TCPDisable;
  BOOLEAN DHCPDisable;
  BOOLEAN SkipSplash;
  UINTN SkipDecode;
  UINTN ExitMode;
  BOOLEAN DisableSPD;
  UINTN MinSPDs;
  INTN ExactSPDs[MAX_NUM_SPDS];
  INT64 ExactSPDSize;
  BOOLEAN CheckMemSPDSize;
  CHECKMEMSPEED CheckMemSpeed[MAX_CHECKMEMSPEED];
  int NumCheckMemSpeed;
  BOOLEAN CheckSPDSMBIOS;
  CHAR16 SPDManuf[64];
  CHAR16 SPDPartNo[32];
  BOOLEAN SameSPDPartNo;
  BOOLEAN SPDMatch;
  INTN SPDReportByteLo;
  INTN SPDReportByteHi;
  BOOLEAN SPDReportExtSN;
  UINTN BgColour;
  BOOLEAN HammerRandom;
  UINT32 HammerPattern;
  UINTN HammerMode;
  UINT64 HammerStep;
  BOOLEAN DisableMP;
  BOOLEAN EnableHT;
  UINTN ConsoleMode;
  BOOLEAN ConsoleOnly;
  UINTN BitFadeSecs;
  UINTN MaxErrCount;
  BOOLEAN TriggerOnErr;
  UINTN ReportPrefix;
  CHAR16 ReportPrepend[MAX_PREPEND_LEN];
  EFI_IP_ADDRESS TFTPServerIp;
  UINTN TFTPStatusSecs;
  BOOLEAN PMPDisable;
  BOOLEAN RTCSync;
  UINTN Verbosity;
  EFI_TPL TPL;
} MTCFG;

enum
{
  AUTOMODE_OFF = 0,
  AUTOMODE_ON,
  AUTOMODE_PROMPT,
  AUTOMODE_MAX
};

enum
{
  AUTOREPORTFMT_HTML = 0,
  AUTOREPORTFMT_BIN
};

enum
{
  EXITMODE_REBOOT = 0,
  EXITMODE_SHUTDOWN,
  EXITMODE_EXIT,
  EXITMODE_PROMPT,
  EXITMODE_MAX
};

enum
{
  SKIPDECODE_NEVER = 0,
  SKIPDECODE_ALL,
  SKIPDECODE_DDR4,
  SKIPDECODE_DDR5,
  SKIPDECODE_MAX
};

enum
{
  REPORT_PREFIX_DEFAULT = 0,
  REPORT_PREFIX_BASEBOARDSN,
  REPORT_PREFIX_SYSINFOSN,
  REPORT_PREFIX_RAMSN,
  REPORT_PREFIX_MACADDR,
};

extern const CHAR16 *SUPPORTED_BGCOLORS[];

// Retrieve localization strings
extern CHAR16 *
GetStringById(
    IN EFI_STRING_ID Id,
    OUT CHAR16 *Str,
    OUT UINTN StrSize);

extern CHAR16 *
GetStringFromPkg(
    IN int LangId,
    IN EFI_STRING_ID StringId,
    OUT CHAR16 *Str,
    OUT UINTN StrSize);

// Sidebar buttons
enum
{
  ID_BUTTON_SYSINFO,
  ID_BUTTON_TESTSELECT,
  ID_BUTTON_MEMADDR,
  ID_BUTTON_CPUSELECT,
  ID_BUTTON_START,
  ID_BUTTON_BENCHMARK,
  ID_BUTTON_SETTINGS,
#ifndef PRO_RELEASE
  ID_BUTTON_UPGRADE,
#endif
  ID_BUTTON_EXIT,
  NUM_SIDEBAR_BUTTONS,
  ID_BUTTON_SPDINFO,
  ID_BUTTON_BENCHRESULTS,
  ID_BUTTON_PREVBENCHRESULTS
};

#endif
