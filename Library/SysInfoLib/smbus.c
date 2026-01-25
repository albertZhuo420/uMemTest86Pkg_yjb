// PassMark SysInfo
//
// Copyright (c) 2005 - 2016
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
// Program:
//	SysInfo
//
// Module:
//	SMBus.cpp
//
// Author(s):
//	Ian Robinson, Keith Mah
//
// Description:
//	Functions for retrieving and decoding SPD information via SMBUS
//
// References:
//   Based on SIDACT code - \\holly\Software Dev\SIDACT
//   SPD structure definition - references\SPD\documents\standards
//                            - (perl script) references\SPD\source code\decode-dimms.pl
//   Intel 801 chipset - (linux driver) drivers/i2c/busses/i2c-i801.c
//                     - references\SPD\documents\datasheets\[Any Intel southbridge datasheet]
//   nVidia chipset    - (linux driver) drivers/i2c/busses/i2c-force2.c
//                     - references\SPD\documents\datasheets\AMD8111 and nForce chipsets.pdf
//                     ** nVidia chipset datasheets aren't publically available but is shown to be similar to AMD 8111 **
//   ATI(PIIX4) chipset- (linux driver) drivers/i2c/busses/i2c-piix4.c
//                     - references\SPD\documents\datasheets\PIIX4.pdf
//   VIA chipset       - (linux driver) drivers/i2c/busses/i2c-viapro.c
//                     - references\SPD\documents\datasheets\VIA VT8231 Southbridge datasheet.pdf
//   AMD756 chipset    - (linux driver) drivers/i2c/busses/i2c-amd756.c
//                     - references\SPD\documents\datasheets\AMD-756 Datasheet.pdf
//   SiS96x chipset    - (linux driver) drivers/i2c/busses/i2c-sis96x.c
//                     ** datasheet isn't publically available **
//   Ali1563 chipset   - (linux driver) drivers/i2c/busses/i2c-ali1563.c
//                     ** datasheet isn't publically available **
//
// History
//	1 June 2005: Initial version

#ifdef WIN32
#include <Windows.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <intrin.h>
#include "SysInfoPrivate.h"
#include "SysInfo.h"
#include "PCI.h"
#include "SMBus.h"
#include "JedecIDList.h"
#include "DirectPort_Port32.h"
#include "DirectPort_DirectIo.h" //"DirectIo.h"

#if _MSC_VER
#define DISABLE_OPTIMISATIONS() __pragma(optimize("", off))
#define ENABLE_OPTIMISATIONS() __pragma(optimize("", on))
#elif __GNUC__
#define DISABLE_OPTIMISATIONS() \
    _Pragma("GCC push_options") \
        _Pragma("GCC optimize (\"O0\")")
#define ENABLE_OPTIMISATIONS() _Pragma("GCC pop_options")
#else
#error new compiler
#endif

// For MemTest86. SysInfo uses fixed size array so don't expand the array
#define expand_spd_info_array() \
    {                           \
    }
#define expand_tsod_info_array() \
    {                            \
    }
#define MtSupportUCDebugWriteLine(x) SysInfo_DebugLogWriteLine(x)

#else // For MemTest86
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemTestSupportLib.h>
#include <Library/IoLib.h>
#include <uMemTest86.h>
#include "pci.h"
#include "smbus.h"

#define SysInfo_IsLogging() FALSE
#define SysInfo_DebugLogWriteLine(x) \
    if (SysInfo_IsLogging())         \
    MtSupportUCDebugWriteLine(x)
#define SysInfo_DebugLogF(...) FPrint(DEBUG_FILE_HANDLE, __VA_ARGS__)
#define DebugMessageBoxes(x)
#define g_szSITempDebug1 g_wszBuffer
#define _T(x) L##x
#define TCHAR CHAR16
#define wchar_t CHAR16
#define _tcscpy(dest, src) StrCpyS(dest, sizeof(dest) / sizeof(dest[0]), src)
#define _tcscpy_s(dest, src) StrCpyS(dest, sizeof(dest) / sizeof(dest[0]), src)
#define _tcscat_s(dest, src) StrCatS(dest, sizeof(dest) / sizeof(dest[0]), src)
#define _tcscat(dest, src) StrCatS(dest, sizeof(dest) / sizeof(dest[0]), src)
#define strlen AsciiStrLen
#define wcslen StrLen
#define wcscpy(dest, src) StrCpyS(dest, sizeof(dest) / sizeof(dest[0]), src)
#define _stprintf_s(buf, ...) UnicodeSPrint(buf, sizeof(buf), __VA_ARGS__)
#define sprintf_s(buf, ...) AsciiSPrint(buf, sizeof(buf), __VA_ARGS__)
#define memset(buf, val, size) SetMem(buf, size, val)
#define memcpy CopyMem
#define memcmp CompareMem

#define Sleep(x) gBS->Stall(x * 1000)

#define BitExtract(dwReg, wIndexHigh, dwIndexLow) ((dwReg) >> (dwIndexLow) & ((1 << ((wIndexHigh) - (dwIndexLow) + 1)) - 1))
#define BitExtractULL(dwReg, wIndexHigh, dwIndexLow) (RShiftU64((UINT64)(dwReg), dwIndexLow) & (LShiftU64(1, ((wIndexHigh) - (dwIndexLow) + 1)) - 1))

// SPDINFO				g_MemoryInfo[MAX_MEMORY_SLOTS];		//Var to hold info from smbus on memory
extern SPDINFO *g_MemoryInfo; // Var to hold info from smbus on memory. Allocate dynamically to keep binary smaller
extern int g_numMemModules;   // Number of memory modules

extern TSODINFO *g_MemTSODInfo;
extern int g_numTSODModules;

extern void expand_spd_info_array();
extern void expand_tsod_info_array();

static bool GetPortVal(WORD wPortAddr, PDWORD pdwPortVal, BYTE bSize)
{
    switch (bSize)
    {
    case 1:
        *pdwPortVal = IoRead8(wPortAddr);
        return true;

    case 2:
        *pdwPortVal = IoRead16(wPortAddr);
        return true;

    case 4:
        *pdwPortVal = IoRead32(wPortAddr);
        return true;

    default:
        break;
    }
    return false;
}

static bool SetPortVal(WORD wPortAddr, DWORD dwPortVal, BYTE bSize)
{
    switch (bSize)
    {
    case 1:
        IoWrite8(wPortAddr, (UINT8)dwPortVal);
        return true;

    case 2:
        IoWrite16(wPortAddr, (UINT16)dwPortVal);
        return true;

    case 4:
        IoWrite32(wPortAddr, (UINT32)dwPortVal);
        return true;

    default:
        break;
    }
    return false;
}

extern bool GetManNameFromJedecID(UINT64 jedecID, wchar_t *buffer, int maxBufSize);
extern bool GetManNameFromJedecIDContCode(unsigned char jedecID, unsigned char numContCode, wchar_t *buffer, int maxBufSize);
#endif

static __inline int numsetbits32(unsigned int i)
{
    // Java: use >>> instead of >>
    // C or C++: use uint32_t
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

static void trim(char *str)
{
    if (!str)
        return;

    char *ptr = str;
    int len = (int)strlen(ptr);

    while (len - 1 > 0 && ptr[len - 1] == ' ')
        ptr[--len] = 0;

    while (*ptr && *ptr == ' ')
        ++ptr, --len;

    memcpy(str, ptr, len + 1);
}

static bool CheckSPDBytes(BYTE *spdData, int spdDataLen);
static bool DecodeSPD(BYTE *spdData, int spdDataLen, SPDINFO *MemoryInfo);
static bool IsDDR4SPD(BYTE *spdData, int spdDataLen);
static bool IsDDR5SPD(BYTE *spdData, int spdDataLen);

static bool CheckCLTTPolling(WORD tsodData[MAX_TSOD_LENGTH]);
static bool CheckTSOD(TSODINFO *tsodInfo);
static int Crc16(BYTE *ptr, int count);

#define SWAP_16(Value) (unsigned short)(((Value) << 8) | ((Value) >> 8))

/* SPD constants/macros */
#define SPD_SDR_MODULE_PARTNO_LEN 18

#define SPD_SDR_MODULE_ATTR_BUFFERED (1 << 0)
#define SPD_SDR_MODULE_ATTR_REGISTERED (1 << 1)
#define SPD_SDR_MODULE_ATTR_ONCARD_PLL (1 << 2)
#define SPD_SDR_MODULE_ATTR_BUFFERED_DQMB (1 << 3)
#define SPD_SDR_MODULE_ATTR_REGISTERED_DQMB (1 << 4)
#define SPD_SDR_MODULE_ATTR_DIFF_CLOCK (1 << 5)

#define SPD_SDR_CHIP_ATTR_EARLY_RAS_PRECH (1 << 0)
#define SPD_SDR_CHIP_ATTR_AUTO_PRECH (1 << 1)
#define SPD_SDR_CHIP_ATTR_PRECH_ALL (1 << 2)
#define SPD_SDR_CHIP_ATTR_WRITE_READ_BURST (1 << 3)
#define SPD_SDR_CHIP_ATTR_LOWER_VCC_TOL (1 << 4)
#define SPD_SDR_CHIP_ATTR_UPPER_VCC_TOL (1 << 5)

#define SPD_DDR_MODULE_PARTNO_LEN 18

#define SPD_DDR_MODULE_ATTR_BUFFERED (1 << 0)
#define SPD_DDR_MODULE_ATTR_REGISTERED (1 << 1)
#define SPD_DDR_MODULE_ATTR_ONCARD_PLL (1 << 2)
#define SPD_DDR_MODULE_ATTR_FET_BOARD_EN (1 << 3)
#define SPD_DDR_MODULE_ATTR_FET_EXT_EN (1 << 4)
#define SPD_DDR_MODULE_ATTR_DIFF_CLOCK (1 << 5)

#define SPD_DDR_CHIP_ATTR_WEAK_DRIVER (1 << 0)
#define SPD_DDR_CHIP_ATTR_LOWER_VCC_TOL (1 << 4)
#define SPD_DDR_CHIP_ATTR_UPPER_VCC_TOL (1 << 5)
#define SPD_DDR_CHIP_ATTR_CONC_AUTO_PRECH (1 << 6)
#define SPD_DDR_CHIP_ATTR_FAST_AP (1 << 7)

#define SPD_DDR2_MODULE_PARTNO_LEN 18

#define SPD_DDR2_MODULE_ATTR_NUM_REG(x) (x & 0x03)
#define SPD_DDR2_MODULE_ATTR_NUM_PLLS(x) ((x & 0xC0) >> 2)
#define SPD_DDR2_MODULE_ATTR_FET_EXT_EN (1 << 4)
#define SPD_DDR2_MODULE_ATTR_ANALYSIS_PROBE (1 << 6)

#define SPD_DDR2_CHIP_ATTR_WEAK_DRIVER (1 << 0)
#define SPD_DDR2_CHIP_ATTR_50_OHM_ODT (1 << 1)
#define SPD_DDR2_CHIP_ATTR_PASR (1 << 2)

#define SPD_DDR2_EPP_ID_1 0x4E
#define SPD_DDR2_EPP_ID_2 0x56
#define SPD_DDR2_EPP_ID_3 0x6D

#define SPD_DDR3_MODULE_PARTNO_LEN 18

#define SPD_DDR3_XMP_ID_1 0x0C
#define SPD_DDR3_XMP_ID_2 0x4A

#define SPD_DDR4_MODULE_PARTNO_LEN 20

#define SPD_DDR4_XMP2_ID_1 0x0C
#define SPD_DDR4_XMP2_ID_2 0x4A

#define SPD_DDR5_MODULE_PARTNO_LEN 30

#define SPD_DDR5_XMP3_ID_1 0x0C
#define SPD_DDR5_XMP3_ID_2 0x4A

#define SPD_DDR5_EXPO_ID_0 0x45
#define SPD_DDR5_EXPO_ID_1 0x58
#define SPD_DDR5_EXPO_ID_2 0x50
#define SPD_DDR5_EXPO_ID_3 0x4F

static const TCHAR *VOLTAGE_LEVEL_TABLE[] = {_T("TTL (5V tolerant)"), _T("LVTTL (not 5V tolerant)"), _T("HSTL 1.5V"), _T("SSTL 3.3V"), _T("SSTL 2.5V"), _T("SSTL 1.8V")};
static const TCHAR *REFRESH_RATE_TABLE[] = {_T("Normal (15.625us)"), _T("Reduced (3.9us)"), _T("Reduced (7.8us)"), _T("Extended (31.3us)"), _T("Extended (62.5us)"), _T("Extended (125us)")};
static const TCHAR *BANK_DENSITY_TABLE[] = {_T("1 GB/4 MB"), _T("2 GB/8 MB"), _T("4 GB/16 MB"), _T("32 MB"), _T("64 MB"), _T("128 MB"), _T("256 MB"), _T("512 MB")};

static const TCHAR *DDR2_MODULE_HEIGHT_TABLE[] = {_T("< 25.4"), _T("25.4"), _T("25.4 - 30.0"), _T("30.0"), _T("30.5"), _T("> 30.5")};
static const TCHAR *DDR2_MODULE_TYPES_TABLE[] = {_T("RDIMM"), _T("UDIMM"), _T("SO-DIMM"), _T("Micro-DIMM"), _T("Mini-RDIMM"), _T("Mini-UDIMM")};

static const TCHAR *DDR2FB_MODULE_HEIGHT_TABLE[] = {_T("Undefined"), _T("15 - 20"), _T("20 - 25"), _T("25 - 30"), _T("30 - 35"), _T("35 - 40")};
static const TCHAR *DDR2FB_MODULE_TYPES_TABLE[] = {_T("Undefined"), _T("RDIMM"), _T("UDIMM"), _T("SO-DIMM"), _T("Micro-DIMM"), _T("Mini-RDIMM"), _T("Mini-UDIMM"), _T("FB-DIMM")};

static const TCHAR *MODULE_THICKNESS_TABLE[] = {_T("Undefined"), _T("< 6.0"), _T("6.0 - 7.0"), _T("7.0 - 8.0"), _T("8.0 - 9.0"), _T("> 9.0")};

static const TCHAR *DDR3_MODULE_TYPES_TABLE[] = {_T("Undefined"), _T("RDIMM"), _T("UDIMM"), _T("SO-DIMM"), _T("Micro-DIMM"), _T("Mini-RDIMM"), _T("Mini-UDIMM"), _T("Mini-CDIMM"),
                                                 _T("72b-SO-UDIMM"), _T("72b-SO-RDIMM"), _T("72b-SO-CDIMM"), _T("Reserved"), _T("Reserved"), _T("Reserved"), _T("Reserved"), _T("Reserved")};

static const TCHAR *DDR4_MODULE_TYPES_TABLE[] = {_T("Extended"), _T("RDIMM"), _T("UDIMM"), _T("SO-DIMM"), _T("LRDIMM"), _T("Mini-RDIMM"), _T("Mini-UDIMM"), _T("Reserved"),
                                                 _T("72b-SO-RDIMM"), _T("72b-SO-UDIMM"), _T("Reserved"), _T("Reserved"), _T("16b-SO-DIMM"), _T("32b-SO-DIMM"), _T("Reserved"), _T("Placeholder")};

static const TCHAR *DDR5_MODULE_TYPES_TABLE[] = {_T("Reserved"), _T("RDIMM"), _T("UDIMM"), _T("SODIMM"), _T("LRDIMM"), _T("CUDIMM"), _T("CSODIMM"), _T("MRDIMM"),
                                                 _T("CAMM2"), _T("Reserved"), _T("DDIMM"), _T("Solder down"), _T("Reserved"), _T("Reserved"), _T("Reserved"), _T("Reserved")};

static const TCHAR *MAX_ACTIVATE_COUNT_TABLE[] = {_T("Untested MAC"), _T("700 K"), _T("600 K"), _T("500 K"), _T("400 K"), _T("300 K"), _T("200 K"), _T("Reserved"), _T("Unlimited MAC"), _T("Reserved"), _T("Reserved"), _T("Reserved"), _T("Reserved"), _T("Reserved"), _T("Reserved"), _T("Reserved")};

static const TCHAR *DRIVE_STRENGTH_TABLE[] = {_T("Light Drive"), _T("Moderate Drive"), _T("Strong Drive"), _T("Very Strong Drive")};

static const TCHAR *MDQ_DRIVE_STRENGTH_TABLE[] = {_T("RZQ/6 (40ohm)"), _T("RZQ/7 (34ohm)"), _T("RZQ/5 (48ohm)"), _T("Reserved"), _T("Reserved"), _T("RZQ/4 (60ohm)"), _T("Reserved"), _T("Reserved")};
static const TCHAR *MDQ_READ_TERM_STRENGTH_TABLE[] = {_T("Disabled"), _T("RZQ/6 (60ohm)"), _T("RZQ/2 (120ohm)"), _T("RZQ/4 (40ohm)"), _T("RZQ (240ohm"), _T("RZQ/5 (48ohm)"), _T("RZQ/3 (80ohm)"), _T("RZQ/7 (34ohm)")};
static const TCHAR *DRAM_DRIVE_STRENGTH_TABLE[] = {_T("RZQ/7 (34ohm)"), _T("RZQ/5 (48ohm)"), _T("Reserved"), _T("Reserved")};
static const TCHAR *DYN_ODT_DRIVE_TABLE[] = {_T("Dynamic ODT Off"), _T("RZQ/2 (120ohm)"), _T("RZQ (240ohm)"), _T("Hi-Impedance"), _T("RZQ/3 (80ohm)"), _T("Reserved"), _T("Reserved"), _T("Reserved")};
static const TCHAR *NOM_ODT_DRIVE_TABLE[] = {_T("Disabled"), _T("RZQ/4 (60ohm)"), _T("RZQ/2 (120ohm)"), _T("RZQ/6 (40ohm)"), _T("RZQ (240ohm)"), _T("RZQ/5 (48ohm)"), _T("RZQ/3 (80ohm)"), _T("RZQ/7 (34ohm)")};
static const TCHAR *PARK_ODT_DRIVE_TABLE[] = {_T("Disabled"), _T("RZQ/4 (60ohm)"), _T("RZQ/2 (120ohm)"), _T("RZQ/6 (40ohm)"), _T("RZQ (240ohm)"), _T("RZQ/5 (48ohm)"), _T("RZQ/3 (80ohm)"), _T("RZQ/7 (34ohm)")};

const TCHAR *SPD_DEVICE_TYPE_TABLE[] = {
    _T("SPD5118"),
    _T("ESPD5216"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved")};

const TCHAR *PMIC_DEVICE_TYPE_TABLE[] = {
    _T("PMIC5000"),
    _T("PMIC5010"),
    _T("PMIC5100"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved")};

const TCHAR *TS_DEVICE_TYPE_TABLE[] = {
    _T("TS5111"),
    _T("TS5110"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved")};

const TCHAR *RCD_DEVICE_TYPE_TABLE[] = {
    _T("DDR5RCD01"),
    _T("DDR5RCD02"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved")};

const TCHAR *DB_DEVICE_TYPE_TABLE[] = {
    _T("DDR5DB01"),
    _T("DDR5DB02"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved")};

const TCHAR *DMB_DEVICE_TYPE_TABLE[] = {
    _T("DMB501"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved")};

const TCHAR *CK_DEVICE_TYPE_TABLE[] = {
    _T("DDR5CK01"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved")};

const TCHAR *OPERATING_TEMP_RANGE[] = {
    _T("A1T (-40 to + 125 C)"),
    _T("A2T (-40 to + 105 C)"),
    _T("A3T (-40 to + 85 C)"),
    _T("IT (-40 to + 95 C)"),
    _T("ST (-25 to + 85 C)"),
    _T("ET (-25 to + 105 C)"),
    _T("RT (0 to + 45 C)"),
    _T("NT (0 to + 85 C)"),
    _T("XT (0 to + 95 C)"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
    _T("Reserved"),
};

const TCHAR *DDR5_DCA_PASR_TABLE[] = {
    _T("Device does not support DCA"),
    _T("Device supports DCA for single / 2-phase internal clock(s)"),
    _T("Device supports DCA for 4-phase internal clock(s)"),
    _T("Reserved")};

const TCHAR *QCK_DRIVER_CHARACTERISTICS_TABLE[] = {
    _T("RZQ/12 (20 Ohms)"),
    _T("RZQ/17 (14 Ohms)"),
    _T("RZQ/24 (10 Ohms)"),
    _T("Reserved"),
};

const TCHAR *SINGLE_ENDED_SLEW_RATE_TABLE[] = {
    _T("Moderate (4-7 V/ns)"),
    _T("Fast (6-10 V/ns)"),
    _T("Slow (2.7-4.5 V/ns)"),
    _T("Reserved"),
};

const TCHAR *DIFFERENTIAL_SLEW_RATE_TABLE[] = {
    _T("Moderate (12-20 V/ns)"),
    _T("Fast (14-27 V/ns)"),
    _T("Reserved"),
    _T("Reserved"),
};

const TCHAR *RCDRW0F_SINGLE_ENDED_SLEW_RATE_TABLE[] = {
    _T("Moderate (3.7-6.7 V/ns)"),
    _T("Fast (5.2-9.2 V/ns)"),
    _T("Slow (2.5-4.3 V/ns)"),
    _T("Reserved"),
};

const TCHAR *DQS_RTT_PARK_TERMINATION_TABLE[] = {
    _T("RTT_OFF (default)"),
    _T("RZQ/1 (240 Ohms)"),
    _T("RZQ/2 (120 Ohms)"),
    _T("RZQ/3 (80 Ohms)"),
    _T("RZQ/4 (60 Ohms)"),
    _T("RZQ/5 (48 Ohms)"),
    _T("RZQ/6 (40 Ohms)"),
    _T("RZQ/7 (34 Ohms)")};

#ifdef WIN32
static const float FRACTIONAL_NS_TABLE[6] = {0.0f, .25f, .33f, .50f, .66f, .75f};
#else
static const int FRACTIONAL_NS_TABLE[6] = {0, 250, 330, 500, 660, 750};
#endif

/* SPD5 constants */

#define SPD5_MR11_I2C_LEGACY_MODE_ADDR_1BYTE (0 << 3)
#define SPD5_MR11_I2C_LEGACY_MODE_ADDR_2BYTE (1 << 3)

#define SPD5_CMD_MEMREG (1 << 7)
#define SPD5_VOLATILE_REG_MR(addr) (addr & 0x3F)

/* I801 SMBus address offsets */
#define INTEL801_SMB_STS 0x00
#define INTEL801_SMB_CNT 0x02
#define INTEL801_SMB_CMD 0x03
#define INTEL801_SMB_ADDR 0x04
#define INTEL801_SMB_DATA 0x05
#define INTEL801_SMB_DATA2 0x06
#define INTEL801_SMB_HBD 0x07
#define INTEL801_SMB_PEC 0x08
#define INTEL801_SMB_AUXSTS 0x0C
#define INTEL801_SMB_AUXCTL 0x0D

/* I801 command constants */
#define INTEL801_SMB_CMD_QUICK 0x00
#define INTEL801_SMB_CMD_BYTE 0x04
#define INTEL801_SMB_CMD_BYTE_DATA 0x08
#define INTEL801_SMB_CMD_WORD_DATA 0x0C
#define INTEL801_SMB_CMD_PROC_CALL 0x10 /* unimplemented */
#define INTEL801_SMB_CMD_BLOCK_DATA 0x14
#define INTEL801_SMB_CMD_I2C_BLOCK_DATA 0x18 /* ICH5 and later */
#define INTEL801_SMB_CMD_BLOCK_LAST 0x34
#define INTEL801_SMB_CMD_I2C_BLOCK_LAST 0x38 /* ICH5 and later */
#define INTEL801_SMB_CMD_START 0x40
#define INTEL801_SMB_CMD_PEC_EN 0x80 /* ICH3 and later */
#define INTEL801_SMBHCTL_KILL (1 << 1)

/* I801 Hosts Status register bits */
#define INTEL801_SMBHSTSTS_BYTE_DONE 0x80
#define INTEL801_SMBHSTSTS_INUSE_STS 0x40
#define INTEL801_SMBHSTSTS_SMBALERT_STS 0x20
#define INTEL801_SMBHSTSTS_FAILED 0x10
#define INTEL801_SMBHSTSTS_BUS_ERR 0x08
#define INTEL801_SMBHSTSTS_DEV_ERR 0x04
#define INTEL801_SMBHSTSTS_INTR 0x02
#define INTEL801_SMBHSTSTS_HOST_BUSY 0x01

#define INTEL801_STATUS_FLAGS (INTEL801_SMBHSTSTS_BYTE_DONE | INTEL801_SMBHSTSTS_FAILED | \
                               INTEL801_SMBHSTSTS_BUS_ERR | INTEL801_SMBHSTSTS_DEV_ERR |  \
                               INTEL801_SMBHSTSTS_INTR)

#ifdef WIN32
#define INIT_TIMEOUT(StartTickCount) StartTickCount = GetTickCount();
#define CHECK_TIMEOUT(StartTickCount, Timeout) ((GetTickCount() - StartTickCount) >= Timeout)
#define INTEL801_MAX_TIMEOUT 250
#define INTEL801_SEM_TIMEOUT 100
#else
#define INIT_TIMEOUT(StartTickCount) StartTickCount = MtSupportReadTSC();

static __inline UINT64
GetTimeInMilliseconds(UINT64 StartTime)
{
    extern UINTN clks_msec; // CPU speed in KHz or # clock cycles / msec
    UINT64 t;

    /* We can't do the elapsed time unless the rdtsc instruction
     * is supported
     */

    if (MtSupportReadTSC() >= StartTime)
        t = MtSupportReadTSC() - StartTime;
    else // overflow?
        t = ((UINT64)-1) - (StartTime - MtSupportReadTSC() + 1);

    t = DivU64x64Remainder(t, clks_msec, NULL);

    return t;
}

static __inline CHAR16 *getVoltageStr(int millivolts, CHAR16 *buf, UINTN bufsize)
{
    if (millivolts % 100 == 0)
        UnicodeSPrint(buf, bufsize, _T("%d.%dV"), millivolts / 1000, (millivolts % 1000) / 100);
    else if (millivolts % 10 == 0)
        UnicodeSPrint(buf, bufsize, _T("%d.%02dV"), millivolts / 1000, (millivolts % 1000) / 10);
    else
        UnicodeSPrint(buf, bufsize, _T("%d.%03dV"), millivolts / 1000, millivolts % 1000);
    return buf;
}

#define CHECK_TIMEOUT(StartTickCount, Timeout) (GetTimeInMilliseconds(StartTickCount) >= Timeout)
#define INTEL801_MAX_TIMEOUT 250
#define INTEL801_SEM_TIMEOUT INTEL801_MAX_TIMEOUT
#endif

#define ICH_GPIOBASE_ADDR 0x500

#define DIMM_SMBUS_MUX_LSB_ADR 0x3A // Address of GPO Level for LSB
#define DIMM_SMBUS_MUX_MSB_ADR 0x3A // Address of GPO Level for MSB
#define GPIO_USE_SEL2 0x30
#define GP_IO_SEL2 0x34
#define DIMM_SMBUS_MUX_LSB_GPO 0x10 // Bit map of LSB GPO
#define DIMM_SMBUS_MUX_MSB_GPO 0x20 // Bit map of MSB GPO

//-----------------------------------------------------------------------------
DISABLE_OPTIMISATIONS() // Optimisation must be turned off
void Intel801SelectSMBusSeg(WORD BaseAddr, WORD seg)
{
    WORD cnt;
    WORD tmp;
    WORD bitValue;
    DWORD gpioUseSelect2 = 0;
    DWORD gpioIoSelect2 = 0;
    DWORD gpioData = 0;

    _stprintf_s(g_szSITempDebug1, _T("Intel801SelectSMBusSeg: GPIO base address=%04X, seg=%d"), BaseAddr, seg);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    DebugMessageBoxes(g_szSITempDebug1);

    // Read GPIO select registers
    GetPortVal(BaseAddr + GPIO_USE_SEL2, &gpioUseSelect2, 4);
    GetPortVal(BaseAddr + GP_IO_SEL2, &gpioIoSelect2, 4);

    _stprintf_s(g_szSITempDebug1, _T("Intel801SelectSMBusSeg: GPIO_USE_SEL2=%08X, GP_IO_SEL2=%08X"), gpioUseSelect2, gpioIoSelect2);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    cnt = 0;
    tmp = DIMM_SMBUS_MUX_LSB_GPO;
    while (!(tmp & 1))
    {
        tmp >>= 1;
        cnt++;
    }
    // GPIO bit value to set LSB
    bitValue = (seg & 1) << cnt;

    // Read GPIO bit value
    GetPortVal(BaseAddr + DIMM_SMBUS_MUX_LSB_ADR, &gpioData, 2);
    _stprintf_s(g_szSITempDebug1, _T("Intel801SelectSMBusSeg: GP_LVL[48:63]=%04X"), gpioData);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Check to see if it matches. If not, change the value.
    if (bitValue != (gpioData & DIMM_SMBUS_MUX_LSB_GPO))
    {
        gpioData &= ~(DIMM_SMBUS_MUX_LSB_GPO | 0x02);
        gpioData |= bitValue;
        SetPortVal(BaseAddr + DIMM_SMBUS_MUX_LSB_ADR, gpioData, 2);
    }

    cnt = 0;
    tmp = DIMM_SMBUS_MUX_MSB_GPO;
    while (!(tmp & 1))
    {
        tmp >>= 1;
        cnt++;
    }

    // GPIO bit value to set MSB
    bitValue = ((seg >> 1) & 1) << cnt;

    // Read GPIO bit value
    GetPortVal(BaseAddr + DIMM_SMBUS_MUX_MSB_ADR, &gpioData, 2);

    // Check to see if it matches. If not, change the value.
    if (bitValue != (gpioData & DIMM_SMBUS_MUX_MSB_GPO))
    {
        gpioData &= ~(DIMM_SMBUS_MUX_MSB_GPO);
        gpioData |= bitValue;
        SetPortVal(BaseAddr + DIMM_SMBUS_MUX_MSB_ADR, gpioData, 2);
    }

    GetPortVal(BaseAddr + DIMM_SMBUS_MUX_MSB_ADR, &gpioData, 2);
    _stprintf_s(g_szSITempDebug1, _T("Intel801SelectSMBusSeg: New GP_LVL[48:63]=%04X"), gpioData);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    DebugMessageBoxes(g_szSITempDebug1);
}
ENABLE_OPTIMISATIONS()

int smbGetSPDIntel801(WORD BaseAddr)
{
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    // for (idx = 0x0; idx <=  0x7F; idx++)
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            if (smbCallBusIntel801(BaseAddr, 0, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Intel801)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);

                DebugMessageBoxes(tmpMsg);

                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

#if 0
                    BYTE spdData[512] = { 0x23, 0x07, 0x0C, 0x01, 0x84, 0x19, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x01, 0x0B, 0x80, 0x00,
                        0x00, 0x00, 0x08, 0x0C, 0xF4, 0x03, 0x00, 0x00, 0x6C, 0x6C, 0x6C, 0x11, 0x08, 0x74, 0x20, 0x08,
                        0x00, 0x05, 0x70, 0x03, 0x00, 0xA8, 0x1E, 0x2B, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x2E, 0x16, 0x36,
                        0x16, 0x36, 0x16, 0x36, 0x0E, 0x2E, 0x23, 0x04, 0x2B, 0x0C, 0x2B, 0x0C, 0x23, 0x04, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0xB5, 0xCE, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0xEB, 0x3C,
                        0x11, 0x11, 0x03, 0x05, 0x00, 0x86, 0x32, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF, 0x9E,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x80, 0xAD, 0x01, 0x14, 0x05, 0x3C, 0x80, 0x65, 0x7E, 0x48, 0x4D, 0x41, 0x34, 0x35, 0x31, 0x52,
                        0x37, 0x4D, 0x46, 0x52, 0x38, 0x4E, 0x2D, 0x54, 0x46, 0x54, 0x31, 0x20, 0x20, 0x00, 0x80, 0xAD,
                        0x03, 0x54, 0x49, 0x34, 0x31, 0x51, 0x32, 0x35, 0x34, 0x37, 0x32, 0x30, 0x31, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x0C, 0x4A, 0x0D, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00,
                        0x00, 0x5A, 0x5A, 0x5A, 0x10, 0xD2, 0x2C, 0x20, 0x08, 0x00, 0x05, 0x70, 0x03, 0x00, 0xA8, 0x1E,
                        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif
                    iNumberOfSPDBytes = 256;

                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    SysInfo_DebugLogWriteLine(_T("A - Setting bank address to 0"));
                    smbSetBankAddrIntel801(BaseAddr, 0);
                    SysInfo_DebugLogWriteLine(_T("A - Finished setting bank address to 0"));

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        if (ioffset == 16 && IsDDR5SPD(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes appears to be DDR5 SPD - not valid"));
                            break;
                        }

                        ret = smbCallBusIntel801(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }

                    if (IsDDR4SPD(spdData, iNumberOfSPDBytes))
                    {
                        SysInfo_DebugLogWriteLine(_T("B - Setting bank address to 1"));
                        smbSetBankAddrIntel801(BaseAddr, 1);
                        SysInfo_DebugLogWriteLine(_T("B - Finished setting bank address to 1"));

                        iNumberOfSPDBytes = 512;

                        ioffset = 256;
                        _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                        SysInfo_DebugLogWriteLine(tmpMsg);
                        for (ioffset = 256; ioffset < 512; ioffset += 2)
                        {
                            ret = smbCallBusIntel801(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data);
                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                            spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                        }

                        SysInfo_DebugLogWriteLine(_T("C - Setting bank address back to 0"));
                        smbSetBankAddrIntel801(BaseAddr, 0);
                        SysInfo_DebugLogWriteLine(_T("C - Finished setting bank address to 0"));
                    }
                    else if (spdData[0] == 0x00 || spdData[1] == 0x00)
                    {
                        SysInfo_DebugLogWriteLine(_T("Failed to retrieve valid SPD bytes. Attempting byte access."));
                        for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                        {
                            if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                            {
                                SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                                break;
                            }

                            if (ioffset == 16 && IsDDR5SPD(spdData, 16))
                            {
                                SysInfo_DebugLogWriteLine(_T("First 16 bytes appears to be DDR5 SPD - not valid"));
                                break;
                            }

                            ret = smbCallBusIntel801(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                        }
                    }

                    /* Block read
                    DWORD AuxCtrl = 0;
                    ret = GetPortVal(BaseAddr + INTEL801_SMB_AUXCTL, &AuxCtrl, 1);
                    ret = SetPortVal(BaseAddr + INTEL801_SMB_AUXCTL, AuxCtrl | INTEL801_SMBAUXCTL_E32B, 1);

                    ioffset = 0;
                    while (ioffset < iNumberOfSPDBytes && ioffset < 256 )
                    {
                    TCHAR strSPDBytes[4];
                    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, 0, 1);
                    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, 0, 1);
                    ret = GetPortVal(BaseAddr + INTEL801_SMB_CNT, &AuxCtrl, 1);
                    ret = smbCallBusIntel801(BaseAddr, ioffset, idx, RW_READ, INTEL801_SMB_CMD_BLOCK_DATA, &Data);

                    spdData[ioffset] = (BYTE)Data;
                    _stprintf_s(strSPDBytes,_T("%02X "), spdData[ioffset]);
                    _tcscat_s(tmpMsg,strSPDBytes);

                    ioffset++;
                    for (int i = 0; i < 32; i++)
                    {
                    DWORD blkData;
                    ret = GetPortVal(BaseAddr + INTEL801_SMB_HBD, (DWORD *)&blkData, 1);
                    spdData[ioffset] = (BYTE)blkData;
                    _stprintf_s(strSPDBytes,_T("%02X "), spdData[ioffset]);
                    _tcscat_s(tmpMsg,strSPDBytes);

                    ioffset++;
                    }
                    }
                    */

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

int smbGetSPD5Intel801(DWORD Bus, DWORD Dev, DWORD Fun, WORD BaseAddr)
{
    int idx, iNumberOfSPDBytes;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    DWORD dwHCFG = 0;
    if (!NeedBSODWorkaround())
    {
        dwHCFG = Get_PCI_Reg(Bus, Dev, Fun, 0x40);

        _stprintf_s(tmpMsg, _T("HCFG=%02X (Intel801_SPD5)"), dwHCFG & 0xFF); // SI101005
        MtSupportUCDebugWriteLine(tmpMsg);
    }

    // for (idx = 0x0; idx <=  0x7F; idx++)
    for (idx = 0x50; idx <= 0x57; idx++)
    {
#ifndef DDR5_DEBUG
        DWORD Data;
        if (smbCallBusIntel801(BaseAddr, 0, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data))
#endif
        {
#ifndef DDR5_DEBUG
            DWORD OldMR11 = 0;
            BYTE spdData[MAX_SPD_LENGTH];
            memset(spdData, 0, sizeof(spdData));
            int ioffset;
#else
            /*
            BYTE spdData[MAX_SPD_LENGTH] = {
                0x30, 0x09, 0x12, 0x02, 0x04, 0x00, 0x40, 0x42, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0xA0, 0x01, 0xE8, 0x03, 0x72, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41,
                0x00, 0x41, 0x00, 0x41, 0x00, 0x7D, 0x00, 0xBE, 0x00, 0x75, 0x27, 0x01, 0xA0, 0x00, 0x82, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB8, 0xAE,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x09, 0x00, 0x80, 0xB3, 0x80, 0x20, 0x80, 0xB3, 0x82, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x02, 0x81, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xCC,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x80, 0x2C, 0x0F, 0x20, 0x33, 0x29, 0xE2, 0x96, 0x0E, 0x4D, 0x54, 0x43, 0x34, 0x43, 0x31, 0x30,
                0x31, 0x36, 0x33, 0x53, 0x31, 0x55, 0x43, 0x34, 0x38, 0x42, 0x41, 0x59, 0x20, 0x20, 0x20, 0x20,
                0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x59, 0x80, 0x2C, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            BYTE spdData[MAX_SPD_LENGTH] = {
                0x30, 0x10, 0x12, 0x02, 0x05, 0x01, 0x20, 0x62, 0x00, 0x00, 0x00, 0x00, 0xA2, 0x12, 0x0F, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x65, 0x01, 0xF2, 0x03, 0x7A, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3E,
                0x80, 0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x9A, 0x01, 0xDC, 0x00, 0xBE, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD4, 0x00, 0x00, 0x00, 0xD4, 0x00, 0x00, 0x00, 0xD4, 0x00,
                0x00, 0x00, 0xD4, 0x00, 0x00, 0x00, 0x88, 0x13, 0x08, 0x88, 0x13, 0x08, 0x20, 0x4E, 0x20, 0x10,
                0x27, 0x10, 0xA4, 0x2C, 0x20, 0x10, 0x27, 0x10, 0xC4, 0x09, 0x04, 0x4C, 0x1D, 0x0C, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x10, 0x00, 0x86, 0x32, 0x80, 0x15, 0x8A, 0x8C, 0x82, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x01, 0x00, 0x81, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x18,
                0x85, 0x9B, 0x00, 0x18, 0x51, 0xFF, 0x01, 0x14, 0xD7, 0x44, 0x58, 0x33, 0x47, 0x36, 0x34, 0x30,
                0x38, 0x59, 0x34, 0x43, 0x41, 0x44, 0x32, 0x4A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x55, 0x80, 0x2C, 0x47, 0x42, 0x5A, 0x41, 0x4C, 0x4C,
                0x43, 0x37, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x0C, 0x4A, 0x30, 0x03, 0x05, 0x8A, 0x8C, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x36,
                0x30, 0x30, 0x20, 0x43, 0x4C, 0x34, 0x36, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x35, 0x32,
                0x30, 0x30, 0x20, 0x43, 0x4C, 0x34, 0x32, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB, 0xA9,
                0x30, 0x22, 0x22, 0x00, 0x22, 0x65, 0x01, 0x7A, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x26, 0x40, 0x80,
                0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x9A, 0x01, 0xDC, 0x00, 0xBE, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x3C, 0xF9,
                0x30, 0x22, 0x22, 0x00, 0x22, 0x80, 0x01, 0x7A, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3E, 0x80,
                0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x9A, 0x01, 0xDC, 0x00, 0xBE, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x9F, 0x65,
                0x45, 0x58, 0x50, 0x4F, 0x10, 0x55, 0x11, 0x00, 0x00, 0x00, 0x22, 0x22, 0x30, 0x00, 0x65, 0x01,
                0x26, 0x40, 0x80, 0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x9A, 0x01, 0xDC, 0x00,
                0xBE, 0x00, 0x88, 0x13, 0x88, 0x13, 0x20, 0x4E, 0x10, 0x27, 0xA4, 0x2C, 0x10, 0x27, 0xC4, 0x09,
                0x4C, 0x1D, 0x22, 0x22, 0x30, 0x00, 0x80, 0x01, 0x80, 0x3E, 0x80, 0x3E, 0x80, 0x3E, 0x00, 0x7D,
                0x80, 0xBB, 0x30, 0x75, 0x9A, 0x01, 0xDC, 0x00, 0xBE, 0x00, 0x88, 0x13, 0x88, 0x13, 0x20, 0x4E,
                0x10, 0x27, 0x13, 0x30, 0x10, 0x27, 0xC4, 0x09, 0x4C, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE4, 0x2E,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            */

            BYTE spdData[MAX_SPD_LENGTH] = {// LPDDR5 CAMM2
                                            0x30, 0x09, 0x15, 0x08, 0x86, 0x21, 0xB5, 0x00, 0x00, 0x40, 0x00, 0x00, 0x0A, 0x01, 0x00, 0x00,
                                            0x48, 0x00, 0x09, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x90, 0xA8, 0x90, 0xC0, 0x08, 0x60,
                                            0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xC6, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x10, 0x00, 0x86, 0x32, 0x80, 0x15, 0x0B, 0x2A, 0x82, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x01, 0x04, 0x71, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE5, 0x78,
                                            0x85, 0x9B, 0x00, 0x18, 0x51, 0xFF, 0x02, 0x11, 0xF2, 0x44, 0x58, 0x32, 0x47, 0x31, 0x32, 0x38,
                                            0x31, 0x36, 0x59, 0x35, 0x32, 0x50, 0x49, 0x34, 0x4A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                                            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x59, 0x80, 0x2C, 0x58, 0x42, 0x5A, 0x41, 0x4C, 0x51,
                                            0x45, 0x59, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#endif
            _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Intel801_SPD5)"), idx); // SI101005
            MtSupportUCDebugWriteLine(tmpMsg);

            DebugMessageBoxes(tmpMsg);

            iNumberOfSPDBytes = 1024;

            // if (Data > 64)
            //	iNumberOfSPDBytes = Data;
            if (g_numMemModules >= MAX_MEMORY_SLOTS)
                expand_spd_info_array();

            if (g_numMemModules < MAX_MEMORY_SLOTS)
                g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
            g_MemoryInfo[g_numMemModules].channel = -1;
            g_MemoryInfo[g_numMemModules].slot = -1;

#ifndef DDR5_DEBUG
            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(0), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR0 - Device Type MSB: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(1), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR1 - Device Type LSB: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(2), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR2 - Device Revision: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(3), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR3 - Device Revision: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(4), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR4 - Device Revision: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(5), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR5 - Device Capability: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(11), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &OldMR11);
            _stprintf_s(tmpMsg, _T("MR11 - I2C Legacy Mode Device Configuration: %02X"), OldMR11 & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(12), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR12 - NVM Protection Configuration: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(13), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR13 - NVM Protection Configuration: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(18), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR18 - Device Configuration: %02X (INF_SEL=%d)"), Data & 0xFF, (Data >> 5) & 1);
            MtSupportUCDebugWriteLine(tmpMsg);

            smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(52), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
            _stprintf_s(tmpMsg, _T("MR52 - Hub and Thermal Sensor Error Status: %02X"), Data & 0xFF);
            MtSupportUCDebugWriteLine(tmpMsg);

            BOOL bI2CBlock = TRUE;

            if (bI2CBlock)
            {
                int page = OldMR11 & 0x7;
                if (page != 0)
                {
                    MtSupportUCDebugWriteLine(_T("Current SPD page is not zero. Resetting to 0..."));
                    if (smbSetSPD5PageAddrIntel801(BaseAddr, (BYTE)idx, 0))
                        MtSupportUCDebugWriteLine(_T("Successfully reset page address to 0"));
                    else
                        MtSupportUCDebugWriteLine(_T("Failed to reset page address to 0"));
                }

                MtSupportUCDebugWriteLine(_T("I2C block read start"));
                if (smbBlockDataIntel801(BaseAddr, (BYTE)(SPD5_CMD_MEMREG | 0), (BYTE)idx, RW_READ, spdData, iNumberOfSPDBytes))
                {
                    MtSupportUCDebugWriteLine(_T("I2C block read completed successfully"));
                    ioffset = iNumberOfSPDBytes;
                    if (!CheckSPDBytes(spdData, 16))
                    {
                        MtSupportUCDebugWriteLine(_T("First 16 bytes 00 or FF - not valid, using random access"));
                        bI2CBlock = FALSE;
                    }
                }
                else
                {
                    MtSupportUCDebugWriteLine(_T("I2C block read failed - using random access"));
                    bI2CBlock = FALSE;
                }
            }

            if (bI2CBlock == FALSE)
            {
                if (dwHCFG & (1 << 4))
                {
                    _stprintf_s(tmpMsg, _T("** WARNING ** SPD bytes may not be read because SPD Writes are disabled (Intel801_SPD5)")); // SI101005
                    MtSupportUCDebugWriteLine(tmpMsg);
                }

#if 0 // <km> Using 2-bit addressing
                smbSetSPD5AddrModeIntel801(BaseAddr, (BYTE)idx, (BYTE)SPD5_MR11_I2C_LEGACY_MODE_ADDR_2BYTE);
#endif
                for (ioffset = 0; ioffset < iNumberOfSPDBytes; ioffset += 2)
                {
#if 0 // <km> Using 2-byte addressing
                    Data = SPD5_CMD_MEMREG | (ioffset & (0x80 - 1));
                    Data |= ((ioffset >> 7) & 0x0F) << 8;
                    if (smbCallBusIntel801(BaseAddr, 0, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_PROC_CALL, &Data))
                    {
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }
                    else
                    {
                        MtSupportUCDebugWriteLine(_T("2-byte addressing failed - switching to 1-byte addressing"));
                        b2ByteAddr = FALSE;
                    }
#endif
                    const int BYTES_PER_PAGE = 128;
                    int page = ioffset / BYTES_PER_PAGE;

                    if (ioffset % BYTES_PER_PAGE == 0)
                    {
                        // Check whether we were able to successfully change pages by comparing the bytes from page 0 and page 1
                        if (page == 2)
                        {
                            if (memcmp(spdData, spdData + BYTES_PER_PAGE, BYTES_PER_PAGE) == 0)
                            {
                                int i;
                                MtSupportUCDebugWriteLine(_T("Failed to set page - bytes from page 0 and 1 have the same values"));

                                tmpMsg[0] = L'\0';
                                for (i = 0; i < ioffset; i++)
                                {
                                    TCHAR strSPDBytes[8];
                                    _stprintf_s(strSPDBytes, _T("%02X "), spdData[i]);
                                    _tcscat(tmpMsg, strSPDBytes);

                                    if ((i % 16 == 15) || (i == ioffset - 1))
                                    {
                                        MtSupportUCDebugWriteLine(tmpMsg);
                                        _stprintf_s(tmpMsg, _T(""));
                                    }
                                }
                                break;
                            }
                        }
                        _stprintf_s(tmpMsg, _T("A - Setting page address to %d"), page);
                        MtSupportUCDebugWriteLine(tmpMsg);
                        if (smbSetSPD5PageAddrIntel801(BaseAddr, (BYTE)idx, (BYTE)page))
                            _stprintf_s(tmpMsg, _T("A - Successfully set page address to %d"), page);
                        else
                            _stprintf_s(tmpMsg, _T("A - Failed to set page address to %d"), page);
                        MtSupportUCDebugWriteLine(tmpMsg);
                    }

#ifndef DDR5_DEBUG
                    smbCallBusIntel801(BaseAddr, (BYTE)(SPD5_CMD_MEMREG | (ioffset & (0x80 - 1))), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data);
                    spdData[ioffset] = (BYTE)(Data & 0xFF);
                    spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
#endif
                    if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                    {

                        MtSupportUCDebugWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                        break;
                    }

                    if (ioffset == 2 && !IsDDR5SPD(spdData, ioffset))
                    {
                        MtSupportUCDebugWriteLine(_T("Error - not DDR5 SPD data"));
                        break;
                    }
                }

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(11), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                _stprintf_s(tmpMsg, _T("MR11 - I2C Legacy Mode Device Configuration: %02X"), Data & 0xFF);
                MtSupportUCDebugWriteLine(tmpMsg);

                if ((Data & 0xFF) != (OldMR11 & 0xFF))
                {
                    _stprintf_s(tmpMsg, _T("Restoring MR11 back to %02X"), OldMR11 & 0xFF);
                    MtSupportUCDebugWriteLine(tmpMsg);

                    smbSetSPD5MRIntel801(BaseAddr, (BYTE)idx, 11, (BYTE)OldMR11);

                    smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(0), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                    _stprintf_s(tmpMsg, _T("MR0 - Device Type MSB: %02X"), Data & 0xFF);
                    MtSupportUCDebugWriteLine(tmpMsg);

                    smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(1), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                    _stprintf_s(tmpMsg, _T("MR1 - Device Type LSB: %02X"), Data & 0xFF);
                    MtSupportUCDebugWriteLine(tmpMsg);
                }
            }

            if (ioffset < iNumberOfSPDBytes)
                continue;
#endif

            if (g_numMemModules < MAX_MEMORY_SLOTS)
            {
                if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                {
                    g_numMemModules++;
                    numMemSticks++;
#ifdef DDR5_DEBUG
                    break;
#endif
                }
            }
            else
            {
                _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
        }
    }

    return numMemSticks;
}

int smbGetTSODIntel801(WORD BaseAddr)
{
    DWORD Data;
    int idx, ioffset;
    int numTSOD = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    // for (idx = 0x0; idx <=  0x7F; idx++)
    for (idx = 0x18; idx <= 0x1F; idx++)
    {
        if (smbCallBusIntel801(BaseAddr, 0, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data))
        {
            WORD tsodData[MAX_TSOD_LENGTH];
            memset(tsodData, 0, sizeof(tsodData));

            _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Intel801)"), idx); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            DebugMessageBoxes(tmpMsg);

            // if (Data > 64)
            //	iNumberOfSPDBytes = Data;
            if (g_numTSODModules >= MAX_TSOD_SLOTS)
                expand_tsod_info_array();

            g_MemTSODInfo[g_numTSODModules].channel = -1;
            g_MemTSODInfo[g_numTSODModules].slot = -1;

            ioffset = 0;
            _stprintf_s(tmpMsg, _T("Retrieving TSOD words %d-%d"), ioffset, MAX_TSOD_LENGTH - 1);
            SysInfo_DebugLogWriteLine(tmpMsg);

            // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
            //  and are not defined in the spec.
            for (ioffset = 0; ioffset < MAX_TSOD_LENGTH; ioffset++)
            {
                if (smbCallBusIntel801(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data))
                    tsodData[ioffset] = SWAP_16(Data & 0xFFFF);
            }

            if (!CheckSPDBytes((BYTE *)tsodData, MAX_TSOD_LENGTH * sizeof(tsodData[0])))
                continue;

            if (g_numTSODModules < MAX_TSOD_SLOTS)
            {
                int temp;
#ifdef WIN32
                float fTemp;
#endif
                int i;

                _stprintf_s(tmpMsg, _T("Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                SysInfo_DebugLogWriteLine(tmpMsg);

                tmpMsg[0] = L'\0';
                for (i = 0; i < MAX_TSOD_LENGTH; i++)
                {
                    TCHAR strSPDBytes[8];
                    _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                    _tcscat(tmpMsg, strSPDBytes);
                }

                SysInfo_DebugLogWriteLine(tmpMsg);

                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCap = tsodData[0];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCfg = tsodData[1];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wHiLim = tsodData[2];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wLoLim = tsodData[3];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCritLim = tsodData[4];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wAmbTemp = tsodData[5];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wManufID = tsodData[6];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wDevRev = tsodData[7];
                memcpy(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor, tsodData + 8, sizeof(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor));

                if (!CheckTSOD(&g_MemTSODInfo[g_numTSODModules]))
                {
                    _stprintf_s(tmpMsg, _T("**WARNING** TSOD may be invalid. Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                    MtSupportUCDebugWriteLine(tmpMsg);

                    tmpMsg[0] = L'\0';
                    for (i = 0; i < MAX_TSOD_LENGTH; i++)
                    {
                        TCHAR strSPDBytes[8];
                        _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                        _tcscat(tmpMsg, strSPDBytes);
                    }
                    MtSupportUCDebugWriteLine(tmpMsg);

                    if (!CheckCLTTPolling(tsodData))
                        continue;

                    SysInfo_DebugLogF(L"Found repeated TSOD words likely from CLTT. Continue parsing TSOD data");
                }

                // High limit
                temp = tsodData[2] & 0xFFC;
                if (BitExtract(tsodData[2], 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Low limit
                temp = tsodData[3] & 0xFFC;
                if (BitExtract(tsodData[3], 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // TCRIT limit
                temp = tsodData[4] & 0xFFC;
                if (BitExtract(tsodData[4], 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Current temperature
                temp = tsodData[5] & 0xFFF;
                if (tsodData[5] & (1 << 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %.3fC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, fTemp, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));

                if (abs((int)fTemp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = fTemp;
                    g_numTSODModules++;
                    numTSOD++;
                }
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %d.%03dC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, temp / 1000, temp % 1000, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));

                if (ABS(temp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = temp;
                    g_numTSODModules++;
                    numTSOD++;
                }
                else
                {
                    _stprintf_s(tmpMsg, _T("Invalid DIMM temperature: %d.%03dC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, temp / 1000, temp % 1000, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));
                    MtSupportUCDebugWriteLine(tmpMsg);
                }
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
            else
            {
                _stprintf_s(tmpMsg, _T("Maximum number of TSOD modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
        }
    }
    return numTSOD;
}

int smbGetSPD5TSODIntel801(WORD BaseAddr)
{
    DWORD Data;
    int idx;
    int numTSOD = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    // for (idx = 0x0; idx <=  0x7F; idx++)
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (smbCallBusIntel801(BaseAddr, 0, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data))
        {
            _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Intel801_SPD5_TSOD)"), idx); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            DebugMessageBoxes(tmpMsg);

            // if (Data > 64)
            //	iNumberOfSPDBytes = Data;
            if (g_numTSODModules >= MAX_TSOD_SLOTS)
                expand_tsod_info_array();

            g_MemTSODInfo[g_numTSODModules].channel = -1;
            g_MemTSODInfo[g_numTSODModules].slot = -1;

            if (g_numTSODModules < MAX_TSOD_SLOTS)
            {
                int temp;
#ifdef WIN32
                float fTemp;
#endif
                // Device type - MSB
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(0), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                if (Data != 0x51)
                    continue;

                // Device type - LSB
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(1), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                if (Data != 0x18)
                    continue;

                // Device Capability
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(5), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                if (Data != 0x03)
                    continue;

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(26), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                _stprintf_s(tmpMsg, _T("MR26 - Thermal Sensor Configuration: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                if (BitExtract(Data, 0, 0))
                {
                    MtSupportUCDebugWriteLine(_T("Temperature sensor disabled. Enabling ..."));

                    Data &= ~1;
                    smbSetSPD5MRIntel801(BaseAddr, (BYTE)idx, 26, (BYTE)(Data & 0xFF));
                    Sleep(1);

                    smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(26), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                    _stprintf_s(tmpMsg, _T("MR26 - Thermal Sensor Configuration: %02X"), Data & 0xFF);
                    MtSupportUCDebugWriteLine(tmpMsg);
                }

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(52), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                _stprintf_s(tmpMsg, _T("MR52 - Hub and Thermal Sensor Error Status: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(26), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bTSConfig = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR26 - TS Configuration: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(27), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bIntConfig = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR27 - Interrupt Configuration: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(28), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim = Data & 0xFF;
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(29), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR28-29 - TS Temp High Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(30), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim = Data & 0xFF;
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(31), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR30-31 - TS Temp Low Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(32), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim = Data & 0xFF;
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(33), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR32-33 - TS Critical Temp High Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(34), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim = Data & 0xFF;
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(35), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR34-35 - TS Critical Temp Low Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(49), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp = Data & 0xFF;
                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(50), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR49-50 - TS Current Sensed Temperature: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(51), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bTSStatus = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR51 - TS Temperature Status: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(52), (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bError = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR51 - Hub, Thermal & NVM Error Status: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                // High limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Low limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Critical High limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical high  limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Critical Low limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical high  limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Current temperature
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp & 0xFFC;
                if (g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp & (1 << 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %.3fC"), g_numTSODModules, fTemp);

                if (abs((int)fTemp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = fTemp;
                    g_numTSODModules++;
                    numTSOD++;
                }
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);

                if (ABS(temp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = temp;
                    g_numTSODModules++;
                    numTSOD++;
                }
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
            else
            {
                _stprintf_s(tmpMsg, _T("Maximum number of TSOD modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
        }
    }
    return numTSOD;
}

DISABLE_OPTIMISATIONS()
BOOL smbCallBusIntel801(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Result = 0, Dump1;
    int ret;

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801(BaseAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbCallBusIntel801: Unable to acquire SMBus resource"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    // Wait until the device is free
    if (smbWaitForFreeIntel801(BaseAddr) == FALSE)
    {
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    if (Prot == INTEL801_SMB_CMD_PROC_CALL)
    {
        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, *Data & 0xFF, 1);
        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, (*Data >> 8) & 0xFF, 1);
    }
    else
    {
        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, 0, 1);
        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, 0, 1);
        ret = SetPortVal(BaseAddr + INTEL801_SMB_CMD, CMD, 1);
    }

    ret = SetPortVal(BaseAddr + INTEL801_SMB_ADDR, (Slave << 1) | RW, 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | Prot, 1); // 0x48 write command to get a byte. 0x4C a word, 0x54 a block
    // Sleep(1);

    // Wait until the command has finished
    if (smbWaitForEndIntel801(BaseAddr) == FALSE)
    {
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    // Retrieve the data
    Dump1 = 0;
    ret = GetPortVal(BaseAddr + INTEL801_SMB_DATA2, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Dump1 << 8;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + INTEL801_SMB_DATA, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Result | Dump1;

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801(BaseAddr);
    *Data = Result;
    return TRUE;
}

BOOL smbBlockDataIntel801(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE *Data, DWORD Len)
{
    DWORD Dump1;
    int ret;
    int iRetry = 0;

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801(BaseAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbBlockDataIntel801: Unable to acquire SMBus resource"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    // Wait until the device is free
    if (smbWaitForFreeIntel801(BaseAddr) == FALSE)
    {
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    while (iRetry < 5)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbBlockDataIntel801: [Attempt %d] Reading %d bytes"), iRetry, Len);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, 0, 1);
        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, CMD, 1);
        ret = SetPortVal(BaseAddr + INTEL801_SMB_HBD, 0, 1);
        ret = SetPortVal(BaseAddr + INTEL801_SMB_CMD, 0, 1);

        ret = SetPortVal(BaseAddr + INTEL801_SMB_ADDR, (Slave << 1) | RW, 1);
        ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | INTEL801_SMB_CMD_I2C_BLOCK_DATA, 1);
        // Sleep(1);

        BOOL bErr = FALSE;
        for (DWORD i = 0; i < Len; i++)
        {
            // Wait until the command has finished
            if (smbWaitForByteDoneIntel801(BaseAddr) == FALSE)
            {
                bErr = TRUE;
                break;
            }

            // Retrieve the data
            Dump1 = 0;
            ret = GetPortVal(BaseAddr + INTEL801_SMB_HBD, (DWORD *)&Dump1, 1); // 1 Bytes
            Data[i] = (BYTE)(Dump1 & 0xFF);

            // When the next-to-last byte is transferred during in I2C read command cycle type, the programmer sets the
            // LAST_BYTE bit (HCTL[05]). This lets the SMBus controller assert a NACK after the last byte is transferred.
            if (i == Len - 2)
                ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_I2C_BLOCK_LAST, 1);

            // Clear status
            ret = SetPortVal(BaseAddr + INTEL801_SMB_STS, INTEL801_SMBHSTSTS_BYTE_DONE, 1);
        }
        if (bErr == FALSE)
            break;

        iRetry++;
    }

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801(BaseAddr);
    return iRetry < 5;
}

BOOL smbWaitForFreeIntel801(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    while (((Status & 1) != 0) && !CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    }

    if (CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForFreeIntel801: Timeout exceeded (SMB_STS=0x%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        SysInfo_DebugLogWriteLine(_T("smbWaitForFreeIntel801: Setting KILL bit"));
        ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMBHCTL_KILL, 1);
        SysInfo_DebugLogWriteLine(_T("smbWaitForFreeIntel801: Resetting KILL bit"));
        ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, 0, 1);

        return FALSE;
    }

    if ((Status & 0x1e) != 0)
        ret = SetPortVal(BaseAddr, 0x1e, 1);

    return TRUE;
}

BOOL smbWaitForEndIntel801(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    while ((Status & INTEL801_SMBHSTSTS_HOST_BUSY) != 0 && !CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    }

    if (CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel801: Timeout exceeded (SMB_STS=0x%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTEL801_SMBHSTSTS_FAILED)
        return FALSE;

    if (Status & INTEL801_SMBHSTSTS_DEV_ERR)
        return FALSE;

    if (Status & INTEL801_SMBHSTSTS_BUS_ERR)
        return FALSE;

    return TRUE;
}

BOOL smbWaitForByteDoneIntel801(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    while ((Status & INTEL801_SMBHSTSTS_BYTE_DONE) == 0 &&
           !CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        if ((Status & INTEL801_SMBHSTSTS_HOST_BUSY) == 0)
        {
            _stprintf_s(g_szSITempDebug1, _T("smbWaitForByteDoneIntel801: I2C read aborted (SMB_STS=0x%08X)"), Status);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
            return FALSE;
        }

        if ((Status & (INTEL801_SMBHSTSTS_SMBALERT_STS | INTEL801_SMBHSTSTS_FAILED | INTEL801_SMBHSTSTS_BUS_ERR | INTEL801_SMBHSTSTS_DEV_ERR)) != 0)
        {
            _stprintf_s(g_szSITempDebug1, _T("smbWaitForByteDoneIntel801: SMBus command failed (SMB_STS=0x%08X)"), Status);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
            return FALSE;
        }

        // Sleep(1);
        ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    }

    if (CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForByteDoneIntel801: Timeout exceeded (SMB_STS=0x%08X)"), Status);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

BOOL smbReleaseSMBusIntel801(WORD BaseAddr)
{
    SetPortVal(BaseAddr + INTEL801_SMB_STS, INTEL801_SMBHSTSTS_INUSE_STS, 1);

    return TRUE;
}

BOOL smbAcquireSMBusIntel801(WORD BaseAddr, DWORD dwTimeout)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    while ((Status & INTEL801_SMBHSTSTS_INUSE_STS) != 0 && !CHECK_TIMEOUT(timeout, dwTimeout))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
    }

    if ((Status & INTEL801_SMBHSTSTS_INUSE_STS) != 0)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbAcquireSMBusIntel801: Forcing release of semaphore (SMB_STS=0x%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        smbReleaseSMBusIntel801(BaseAddr);

        ret = GetPortVal(BaseAddr + INTEL801_SMB_STS, (DWORD *)&Status, 1); // 1 Bytes
        if ((Status & INTEL801_SMBHSTSTS_INUSE_STS) != 0)
        {
            _stprintf_s(g_szSITempDebug1, _T("smbAcquireSMBusIntel801: Semaphore could not be released. Ignoring semaphore bit."));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
    }

    return TRUE;
}

BOOL smbSetBankAddrIntel801(WORD BaseAddr, BYTE bBankNo)
{
    BYTE Slave = 0;
    int ret;

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801(BaseAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntel801: Unable to acquire SMBus resource"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (smbWaitForFreeIntel801(BaseAddr) == FALSE)
    {
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    if (bBankNo)
        Slave = 0x6E;
    else
        Slave = 0x6C;

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntel801: Setting SMB_ADDR=%08X"), Slave);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_ADDR, Slave, 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | INTEL801_SMB_CMD_QUICK, 1);
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntel801: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndIntel801(BaseAddr) == FALSE)
    {
        // SysInfo_DebugLogWriteLine(_T("smbSetBankAddrIntel801 failed"));
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801(BaseAddr);
    return TRUE;
}

BOOL smbSetSPD5PageAddrIntel801(WORD BaseAddr, BYTE Slave, BYTE Page)
{
    int ret;
    DWORD Data = 0;
    BYTE Cmd;

    smbCallBusIntel801(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(11), Slave, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data);
    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrIntel801: MR11 - I2C Legacy Mode Device Configuration: %02X"), Data & 0xFF);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801(BaseAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrIntel801: Unable to acquire SMBus resource"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (smbWaitForFreeIntel801(BaseAddr) == FALSE)
    {
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    Data &= ~0x0f;
    Data |= SPD5_MR11_I2C_LEGACY_MODE_ADDR_1BYTE | (Page & 0x7);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrIntel801: Setting SMB_DATA=%02X"), Data);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, Data & 0xFF, 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, 0, 1);

    Cmd = SPD5_VOLATILE_REG_MR(11);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrIntel801: Setting SMB_CMD=%02X"), Cmd);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_CMD, Cmd, 1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_ADDR, (Slave << 1), 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | INTEL801_SMB_CMD_BYTE_DATA, 1);
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrIntel801: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndIntel801(BaseAddr) == FALSE)
    {
        SysInfo_DebugLogWriteLine(_T("smbSetSPD5PageAddrIntel801 failed"));
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801(BaseAddr);

    return TRUE;
}

BOOL smbSetSPD5AddrModeIntel801(WORD BaseAddr, BYTE Slave, BYTE AddrMode)
{
    int ret;
    DWORD Data;
    BYTE Cmd;

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801(BaseAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5AddrModeIntel801: Unable to acquire SMBus resource"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (smbWaitForFreeIntel801(BaseAddr) == FALSE)
    {
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    Data = AddrMode;

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5AddrModeIntel801: Setting SMB_DATA=%08X"), Data);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, Data, 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, 0, 1);

    Cmd = SPD5_VOLATILE_REG_MR(11);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5AddrModeIntel801: Setting SMB_CMD=%08X"), Cmd);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_CMD, Cmd, 1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_ADDR, (Slave << 1), 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | INTEL801_SMB_CMD_BYTE_DATA, 1);
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5AddrModeIntel801: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndIntel801(BaseAddr) == FALSE)
    {
        SysInfo_DebugLogWriteLine(_T("smbSetSPD5AddrModeIntel801 failed"));
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801(BaseAddr);

    return TRUE;
}

BOOL smbSetSPD5MRIntel801(WORD BaseAddr, BYTE Slave, BYTE MR, BYTE Data)
{
    int ret;
    BYTE Cmd;

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801(BaseAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5MRIntel801: Unable to acquire SMBus resource"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (smbWaitForFreeIntel801(BaseAddr) == FALSE)
    {
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5MRIntel801: Setting SMB_DATA=%02X"), Data);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, Data, 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, 0, 1);

    Cmd = SPD5_VOLATILE_REG_MR(MR);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5MRIntel801: Setting SMB_CMD=%02X"), Cmd);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_CMD, Cmd, 1);

    ret = SetPortVal(BaseAddr + INTEL801_SMB_ADDR, (Slave << 1), 1);
    ret = SetPortVal(BaseAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | INTEL801_SMB_CMD_BYTE_DATA, 1);
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5MRIntel801: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndIntel801(BaseAddr) == FALSE)
    {
        SysInfo_DebugLogWriteLine(_T("smbSetSPD5MRIntel801 failed"));
        smbReleaseSMBusIntel801(BaseAddr);
        return FALSE;
    }

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801(BaseAddr);

    return TRUE;
}
ENABLE_OPTIMISATIONS()

int smbGetSPDIntel801_MMIO(BYTE *BaseAddr)
{
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

#ifdef WIN32
    BYTE *pbLinBaseAddr = NULL;
    // HANDLE		hPhysicalMemory	= NULL;
    tagPhys32Struct Phys32Struct = {0};

    // Ask driver to map MMIO base address to pointer we can use
    pbLinBaseAddr = MapPhysToLin(BaseAddr, 32, &Phys32Struct, TRUE);

    if (pbLinBaseAddr)
    {
#else
    PBYTE pbLinBaseAddr = BaseAddr;
#endif

        // for (idx = 0x0; idx <=  0x7F; idx++)
        for (idx = 0x50; idx <= 0x57; idx++)
        {
            if (idx != 0x69) // Don't probe the Clock chip
            {
                if (smbCallBusIntel801_MMIO(pbLinBaseAddr, 0, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_BYTE_DATA, &Data))
                {
                    _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Intel80_MMIO1)"), idx); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    DebugMessageBoxes(tmpMsg);

                    // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                    if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                    {
                        BYTE spdData[MAX_SPD_LENGTH];
                        memset(spdData, 0, sizeof(spdData));
#if 0
                    BYTE spdData[512] = { 0x23, 0x07, 0x0C, 0x01, 0x84, 0x19, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x01, 0x0B, 0x80, 0x00,
                        0x00, 0x00, 0x08, 0x0C, 0xF4, 0x03, 0x00, 0x00, 0x6C, 0x6C, 0x6C, 0x11, 0x08, 0x74, 0x20, 0x08,
                        0x00, 0x05, 0x70, 0x03, 0x00, 0xA8, 0x1E, 0x2B, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x2E, 0x16, 0x36,
                        0x16, 0x36, 0x16, 0x36, 0x0E, 0x2E, 0x23, 0x04, 0x2B, 0x0C, 0x2B, 0x0C, 0x23, 0x04, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0xB5, 0xCE, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0xEB, 0x3C,
                        0x11, 0x11, 0x03, 0x05, 0x00, 0x86, 0x32, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF, 0x9E,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x80, 0xAD, 0x01, 0x14, 0x05, 0x3C, 0x80, 0x65, 0x7E, 0x48, 0x4D, 0x41, 0x34, 0x35, 0x31, 0x52,
                        0x37, 0x4D, 0x46, 0x52, 0x38, 0x4E, 0x2D, 0x54, 0x46, 0x54, 0x31, 0x20, 0x20, 0x00, 0x80, 0xAD,
                        0x03, 0x54, 0x49, 0x34, 0x31, 0x51, 0x32, 0x35, 0x34, 0x37, 0x32, 0x30, 0x31, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x0C, 0x4A, 0x0D, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00,
                        0x00, 0x5A, 0x5A, 0x5A, 0x10, 0xD2, 0x2C, 0x20, 0x08, 0x00, 0x05, 0x70, 0x03, 0x00, 0xA8, 0x1E,
                        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif
                        iNumberOfSPDBytes = 256;

                        // if (Data > 64)
                        //	iNumberOfSPDBytes = Data;
                        if (g_numMemModules >= MAX_MEMORY_SLOTS)
                            expand_spd_info_array();

                        if (g_numMemModules < MAX_MEMORY_SLOTS)
                            g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                        g_MemoryInfo[g_numMemModules].channel = -1;
                        g_MemoryInfo[g_numMemModules].slot = -1;

                        SysInfo_DebugLogWriteLine(_T("A - Setting bank address to 0"));
                        smbSetBankAddrIntel801_MMIO(pbLinBaseAddr, 0);
                        SysInfo_DebugLogWriteLine(_T("A - Finished setting bank address to 0"));

                        ioffset = 0;
                        _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                        SysInfo_DebugLogWriteLine(tmpMsg);

                        // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                        //  and are not defined in the spec.
                        for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                        {
                            if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                            {
                                SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                                break;
                            }

                            ret = smbCallBusIntel801_MMIO(pbLinBaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data);
                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                            spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                        }

                        if (IsDDR4SPD(spdData, iNumberOfSPDBytes))
                        {
                            SysInfo_DebugLogWriteLine(_T("B - Setting bank address to 1"));
                            smbSetBankAddrIntel801_MMIO(pbLinBaseAddr, 1);
                            SysInfo_DebugLogWriteLine(_T("B - Finished setting bank address to 1"));

                            iNumberOfSPDBytes = 512;

                            ioffset = 256;
                            _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                            SysInfo_DebugLogWriteLine(tmpMsg);
                            for (ioffset = 256; ioffset < 512; ioffset += 2)
                            {
                                ret = smbCallBusIntel801_MMIO(pbLinBaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data);
                                spdData[ioffset] = (BYTE)(Data & 0xFF);
                                spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                            }

                            SysInfo_DebugLogWriteLine(_T("C - Setting bank address back to 0"));
                            smbSetBankAddrIntel801_MMIO(pbLinBaseAddr, 0);
                            SysInfo_DebugLogWriteLine(_T("C - Finished setting bank address to 0"));
                        }

                        /* Block read
                        DWORD AuxCtrl = 0;
                        ret = GetPortVal(BaseAddr + INTEL801_SMB_AUXCTL, &AuxCtrl, 1);
                        ret = SetPortVal(BaseAddr + INTEL801_SMB_AUXCTL, AuxCtrl | INTEL801_SMBAUXCTL_E32B, 1);

                        ioffset = 0;
                        while (ioffset < iNumberOfSPDBytes && ioffset < 256 )
                        {
                        TCHAR strSPDBytes[4];
                        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA, 0, 1);
                        ret = SetPortVal(BaseAddr + INTEL801_SMB_DATA2, 0, 1);
                        ret = GetPortVal(BaseAddr + INTEL801_SMB_CNT, &AuxCtrl, 1);
                        ret = smbCallBusIntel801(BaseAddr, ioffset, idx, RW_READ, INTEL801_SMB_CMD_BLOCK_DATA, &Data);

                        spdData[ioffset] = (BYTE)Data;
                        _stprintf_s(strSPDBytes,_T("%02X "), spdData[ioffset]);
                        _tcscat_s(tmpMsg,strSPDBytes);

                        ioffset++;
                        for (int i = 0; i < 32; i++)
                        {
                        DWORD blkData;
                        ret = GetPortVal(BaseAddr + INTEL801_SMB_HBD, (DWORD *)&blkData, 1);
                        spdData[ioffset] = (BYTE)blkData;
                        _stprintf_s(strSPDBytes,_T("%02X "), spdData[ioffset]);
                        _tcscat_s(tmpMsg,strSPDBytes);

                        ioffset++;
                        }
                        }
                        */

                        if (g_numMemModules < MAX_MEMORY_SLOTS)
                        {
                            if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                            {
                                g_numMemModules++;
                                numMemSticks++;
                            }
                        }
                        else
                        {
                            _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                            SysInfo_DebugLogWriteLine(tmpMsg);
                        }
                    }
                }
            }
        }
#ifdef WIN32
        UnmapPhysicalMemory(&Phys32Struct, pbLinBaseAddr, 32);
    }
#endif

    return numMemSticks;
}

BOOL smbCallBusIntel801_MMIO(BYTE *pbLinAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Result = 0, Dump1;

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801_MMIO(pbLinAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbCallBusIntel801_MMIO: Unable to acquire SMBus resource"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    // Wait until the device is free
    if (smbWaitForFreeIntel801_MMIO(pbLinAddr) == FALSE)
    {
        smbReleaseSMBusIntel801_MMIO(pbLinAddr);
        return FALSE;
    }

#ifdef _WIN32
    *((BYTE *)(pbLinAddr + INTEL801_SMB_DATA)) = 0;
    *((BYTE *)(pbLinAddr + INTEL801_SMB_DATA2)) = 0;
    *((BYTE *)(pbLinAddr + INTEL801_SMB_CMD)) = CMD;
    *((BYTE *)(pbLinAddr + INTEL801_SMB_ADDR)) = (Slave << 1) | RW;
    *((BYTE *)(pbLinAddr + INTEL801_SMB_CNT)) = INTEL801_SMB_CMD_START | Prot; // 0x48 write command to get a byte. 0x4C a word, 0x54 a block
#else
    MmioWrite8((UINTN)pbLinAddr + INTEL801_SMB_DATA, 0);
    MmioWrite8((UINTN)pbLinAddr + INTEL801_SMB_DATA2, 0);
    MmioWrite8((UINTN)pbLinAddr + INTEL801_SMB_CMD, CMD);
    MmioWrite8((UINTN)pbLinAddr + INTEL801_SMB_ADDR, (Slave << 1) | RW);
    MmioWrite8((UINTN)pbLinAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | Prot); // 0x48 write command to get a byte. 0x4C a word, 0x54 a block
#endif

    // Wait until the command has finished
    if (smbWaitForEndIntel801_MMIO(pbLinAddr) == FALSE)
    {
        smbReleaseSMBusIntel801_MMIO(pbLinAddr);
        return FALSE;
    }

    // Retrieve the data
#ifdef _WIN32
    Dump1 = *((BYTE *)(pbLinAddr + INTEL801_SMB_DATA2));
    Result = Dump1 << 8;

    Dump1 = *((BYTE *)(pbLinAddr + INTEL801_SMB_DATA));
    Result = Result | Dump1;
#else
    Dump1 = MmioRead8((UINTN)pbLinAddr + INTEL801_SMB_DATA2);
    Result = Dump1 << 8;

    Dump1 = MmioRead8((UINTN)pbLinAddr + INTEL801_SMB_DATA);
    Result = Result | Dump1;
#endif

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801_MMIO(pbLinAddr);
    *Data = Result;
    return TRUE;
}

BOOL smbSetBankAddrIntel801_MMIO(BYTE *pbLinAddr, BYTE bBankNo)
{
    BYTE Slave = 0;

    // Acquire SMBus semaphore
    if (smbAcquireSMBusIntel801_MMIO(pbLinAddr, INTEL801_SEM_TIMEOUT) == FALSE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntel801_MMIO: Unable to acquire SMBus resource"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (smbWaitForFreeIntel801_MMIO(pbLinAddr) == FALSE)
    {
        smbReleaseSMBusIntel801_MMIO(pbLinAddr);
        return FALSE;
    }

    if (bBankNo)
        Slave = 0x6E;
    else
        Slave = 0x6C;

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntel801_MMIO: Setting SMB_ADDR=%08X"), Slave);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef _WIN32
    *((BYTE *)(pbLinAddr + INTEL801_SMB_ADDR)) = Slave;
    *((BYTE *)(pbLinAddr + INTEL801_SMB_CNT)) = INTEL801_SMB_CMD_START | INTEL801_SMB_CMD_QUICK;
#else
    MmioWrite8((UINTN)pbLinAddr + INTEL801_SMB_ADDR, Slave);
    MmioWrite8((UINTN)pbLinAddr + INTEL801_SMB_CNT, INTEL801_SMB_CMD_START | INTEL801_SMB_CMD_QUICK);
#endif

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntel801_MMIO: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndIntel801_MMIO(pbLinAddr) == FALSE)
    {
        // SysInfo_DebugLogWriteLine(_T("smbSetBankAddrIntel801_MMIO failed"));
        smbReleaseSMBusIntel801_MMIO(pbLinAddr);
        return FALSE;
    }

    // Release the SMBus semaphore
    smbReleaseSMBusIntel801_MMIO(pbLinAddr);
    return TRUE;
}

BOOL smbWaitForFreeIntel801_MMIO(BYTE *pbLinAddr)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)); // 1 Byte
    while (((Status & 1) != 0) && !CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        // Sleep(1);
        Status = *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)); // 1 Byte`
    }

    if (CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForFreeIntel801_MMIO: Timeout exceeded (SMB_STS=0x%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        SysInfo_DebugLogWriteLine(_T("smbWaitForFreeIntel801_MMIO: Setting KILL bit"));
        *((BYTE *)(pbLinAddr + INTEL801_SMB_CNT)) = INTEL801_SMBHCTL_KILL;
        SysInfo_DebugLogWriteLine(_T("smbWaitForFreeIntel801_MMIO: Resetting KILL bit"));
        *((BYTE *)(pbLinAddr + INTEL801_SMB_CNT)) = 0;

        return FALSE;
    }

    if ((Status & 0x1e) != 0)
        *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)) = 0x1e;

    return TRUE;
}

BOOL smbWaitForEndIntel801_MMIO(BYTE *pbLinAddr)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)); // 1 Byte
    while ((Status & 1) == 1 && !CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        // Sleep(1);
        Status = *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)); // 1 Byte
    }

    if (CHECK_TIMEOUT(timeout, INTEL801_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel801_MMIO: Timeout exceeded (SMB_STS=0x%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTEL801_SMBHSTSTS_FAILED)
        return FALSE;

    if (Status & INTEL801_SMBHSTSTS_DEV_ERR)
        return FALSE;

    if (Status & INTEL801_SMBHSTSTS_BUS_ERR)
        return FALSE;

    return TRUE;
}

BOOL smbAcquireSMBusIntel801_MMIO(BYTE *pbLinAddr, DWORD dwTimeout)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)); // 1 Byte
    while ((Status & INTEL801_SMBHSTSTS_INUSE_STS) != 0 && !CHECK_TIMEOUT(timeout, dwTimeout))
    {
        // Sleep(1);
        Status = *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)); // 1 Byte
    }

    if ((Status & INTEL801_SMBHSTSTS_INUSE_STS) != 0)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbAcquireSMBusIntel801_MMIO: Forcing release of semaphore (SMB_STS=0x%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        smbReleaseSMBusIntel801_MMIO(pbLinAddr);

        Status = *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)); // 1 Byte
        if ((Status & INTEL801_SMBHSTSTS_INUSE_STS) != 0)
        {
            _stprintf_s(g_szSITempDebug1, _T("smbAcquireSMBusIntel801_MMIO: Semaphore could not be released. Ignoring semaphore bit."));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
    }

    return TRUE;
}

BOOL smbReleaseSMBusIntel801_MMIO(BYTE *pbLinAddr)
{
    *((BYTE *)(pbLinAddr + INTEL801_SMB_STS)) = INTEL801_SMBHSTSTS_INUSE_STS;

    return TRUE;
}

/* IX79 SMBus address offsets */
#define INTELX79_SMB_BASE(ch) (0x180 + ch * 0x10)
#define INTELX79_SMB_STAT(ch) (INTELX79_SMB_BASE(ch) + 0x00)
#define INTELX79_SMBCMD(ch) (INTELX79_SMB_BASE(ch) + 0x04)
#define INTELX79_SMBCNTL(ch) (INTELX79_SMB_BASE(ch) + 0x08)
#define INTELX79_SMB_TSOD_POLL_RATE_CNTR(ch) (INTELX79_SMB_BASE(ch) + 0x0C)

#define INTELX79_DIMMTEMPSTAT_0 0x150
#define INTELX79_DIMMTEMPSTAT_1 0x154
#define INTELX79_DIMMTEMPSTAT_2 0x158

/* IX79 Hosts Status register bits */
#define INTELX79_SMB_RDO 0x80000000
#define INTELX79_SMB_WOD 0x40000000
#define INTELX79_SMB_SBE 0x20000000
#define INTELX79_SMB_BUSY 0x10000000

#define INTELX79_SMB_LAST_SLAVE_ADDR(x) ((x >> 24) & 0x07)
#define INTELX79_SMB_RDATA(x) (x & 0xFFFF)

#define INTELX79_SMB_CMD_TRIGGER 0x80000000
#define INTELX79_SMB_PNTR_SEL 0x40000000
#define INTELX79_SMB_WORD_ACCESS 0x20000000
#define INTELX79_SMB_WRT_PNTR 0x10000000
#define INTELX79_SMB_WRT_CMD 0x08000000
#define INTELX79_SMB_SA(x) ((x & 0x07) << 24)
#define INTELX79_SMB_BA(x) ((x & 0xFF) << 16)
#define INTELX79_SMB_WDATA(x) (x & 0xFFFF)

#define INTELX79_SMB_DTI(x) ((x & 0x0F) << 28)
#define INTELX79_SMB_CKOVRD 0x08000000
#define INTELX79_SMB_DIS_WRT 0x04000000
#define INTELX79_SMB_SOFT_RST 0x00000400
#define INTELX79_SMB_START_TSOD_POLL 0x00000200
#define INTELX79_SMB_TSOD_POLL_EN 0x00000100
#define INTELX79_TSOD_PRESENT 0x000000FF

#define INTELX79_MAX_TIMEOUT 250

int smbGetSPDIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
    int idx, ioffset, iNumberOfSPDBytes;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    for (BYTE ch = 0; ch < 2; ch++)
    {
        BOOL bErr = FALSE;
        DWORD Data = 0;

        bErr = Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMB_STAT(ch), &Data);
        if (bErr == FALSE)
        {
            _stprintf_s(tmpMsg, _T("smbGetSPDIntel801_PCI: Could not read PCI register {%02X:%02X:%02X:%04X}"), Bus, Dev, Fun, INTELX79_SMB_BASE(ch)); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);
            continue;
        }
        _stprintf_s(tmpMsg, _T("SMB_STAT[%d]=%08X"), ch, Data); // SI101005
        SysInfo_DebugLogWriteLine(tmpMsg);

        bErr = Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMBCMD(ch), &Data);
        if (bErr == FALSE)
        {
            _stprintf_s(tmpMsg, _T("smbGetSPDIntel801_PCI: Could not read PCI register {%02X:%02X:%02X:%04X}"), Bus, Dev, Fun, INTELX79_SMBCMD(ch)); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);
            continue;
        }

        _stprintf_s(tmpMsg, _T("SMBCMD[%d]=%08X"), ch, Data); // SI101005
        SysInfo_DebugLogWriteLine(tmpMsg);

        bErr = Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMBCNTL(ch), &Data);
        if (bErr == FALSE)
        {
            _stprintf_s(tmpMsg, _T("smbGetSPDIntel801_PCI: Could not read PCI register {%02X:%02X:%02X:%04X}"), Bus, Dev, Fun, INTELX79_SMBCNTL(ch)); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);
            continue;
        }

        _stprintf_s(tmpMsg, _T("SMBCNTL[%d]=%08X"), ch, Data); // SI101005
        SysInfo_DebugLogWriteLine(tmpMsg);

        for (idx = 0; idx <= 7; idx++)
        {
#ifdef WIN32
            // <km> We ask the driver to perform the SMBus access because sometimes we get corrupt data when the SMBus is being used at the same time.
            if (DirectIo_Read_SandyBridgeE_SMBus(Bus, Dev, Fun, INTELX79_SMB_BASE(ch), idx, 0, &Data) == ERROR_SUCCESS)
#else
            if (smbCallBusIntel801_PCI(Bus, Dev, Fun, ch, 0, (BYTE)idx, FALSE, &Data))
#endif
            {
                BOOL bUseByteAccess = TRUE;
                BYTE spdData[MAX_SPD_LENGTH];
                memset(spdData, 0, sizeof(spdData));

                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at channel %d, slot %d (Intel801_PCI)"), ch, idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);

                iNumberOfSPDBytes = 256;
                // if (Data > 64)
                //	iNumberOfSPDBytes = Data;

                // g_MemoryInfo[numMemSticks].dimmNum = (ch * 8) + idx;
                if (g_numMemModules >= MAX_MEMORY_SLOTS)
                    expand_spd_info_array();

                if (g_numMemModules < MAX_MEMORY_SLOTS)
                    g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                g_MemoryInfo[g_numMemModules].channel = -1;
                g_MemoryInfo[g_numMemModules].slot = -1;

                ioffset = 0;
                _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                SysInfo_DebugLogWriteLine(tmpMsg);

                // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                //  and are not defined in the spec.

#ifdef WIN32
                // <km> We ask the driver to perform the SMBus access because sometimes we get corrupt data when the SMBus is being used at the same time.
                if (DirectIo_Read_SandyBridgeE_SMBus(Bus, Dev, Fun, INTELX79_SMB_BASE(ch), idx, 0, &Data) == ERROR_SUCCESS)
#else
                if (smbCallBusIntel801_PCI(Bus, Dev, Fun, ch, 0, (BYTE)idx, TRUE, &Data)) // See if we can read word-by-word
#endif
                {
                    bUseByteAccess = FALSE;
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        int iErrCount = 0;

                        Data = 0;

                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }
#ifdef WIN32
                        // <km> We ask the driver to perform the SMBus access because sometimes we get corrupt data when the SMBus is being used at the same time.
                        while (DirectIo_Read_SandyBridgeE_SMBus(Bus, Dev, Fun, INTELX79_SMB_BASE(ch), idx, ioffset, &Data) != ERROR_SUCCESS && iErrCount < 10)
#else
                        while (!smbCallBusIntel801_PCI(Bus, Dev, Fun, ch, (BYTE)ioffset, (BYTE)idx, TRUE, &Data) && iErrCount < 10)
#endif
                        {
                            iErrCount++;
                        }

                        if (iErrCount >= 10)
                        {
                            _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                            SysInfo_DebugLogWriteLine(tmpMsg);

                            bUseByteAccess = TRUE;
                            break;
                        }

                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }
                }

                if (bUseByteAccess) // Read byte-by-byte
                {
                    SysInfo_DebugLogWriteLine(_T("Word access failed, attempting byte access"));
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                    {
                        int iErrCount = 0;

                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        Data = 0;

                        while (!smbCallBusIntel801_PCI(Bus, Dev, Fun, ch, (BYTE)ioffset, (BYTE)idx, FALSE, &Data) && iErrCount < 10)
                        {
                            iErrCount++;
                        }

                        if (iErrCount >= 10)
                        {
                            _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                            SysInfo_DebugLogWriteLine(tmpMsg);
                            break;
                        }

                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                    }
                }

                if (g_numMemModules < MAX_MEMORY_SLOTS)
                {
                    if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                    {
                        g_numMemModules++;
                        numMemSticks++;
                    }
                }
                else
                {
                    _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }
            }
        }
    }

    return numMemSticks;
}

BOOL smbCallBusIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, BYTE Ch, BYTE bOffset, BYTE bSlot, BOOL bWord, DWORD *Data)
{
    DWORD Result = 0;
    DWORD Command = 0;
    DWORD Control = 0;

    if (smbWaitForFreeIntel801_PCI(Bus, Dev, Fun, Ch) == FALSE)
        return FALSE;

    // Make sure DTI is set to 0xA and TSOD polling is disabled in the SMBCNTL register
    if (Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMBCNTL(Ch), &Control) == FALSE)
        return FALSE;

    Control &= 0x07FFFCFF;
    Control |= 0xA8000000;

    if (Set_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMBCNTL(Ch), Control) == FALSE)
        return FALSE;

    Command = INTELX79_SMB_BA(bOffset) | INTELX79_SMB_SA(bSlot);
    if (bWord)
        Command |= INTELX79_SMB_WORD_ACCESS;

    if (Set_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMBCMD(Ch), Command | INTELX79_SMB_CMD_TRIGGER) == FALSE)
        return FALSE;

    // Sleep(1);
    if (smbWaitForEndIntel801_PCI(Bus, Dev, Fun, Ch) == FALSE)
        return FALSE;

    if (Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMB_STAT(Ch), &Result) == FALSE)
        return FALSE;

    Result = INTELX79_SMB_RDATA(Result);

    if (bWord)
        *Data = ((Result << 8) & 0x0000FF00) | ((Result >> 8) & 0x000000FF);
    else
        *Data = Result & 0x000000FF;

    return TRUE;
}

BOOL smbWaitForFreeIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, BYTE Ch)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    if (Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMB_STAT(Ch), &Status) == FALSE)
        return FALSE;

    while (((Status & INTELX79_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
    {
        // Sleep(1);

        if (Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMB_STAT(Ch), &Status) == FALSE)
            return FALSE;
    }

    if (CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, BYTE Ch)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    if (Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMB_STAT(Ch), &Status) == FALSE)
        return FALSE;

    while (((Status & INTELX79_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
    {
        // Sleep(1);

        if (Get_PCIEx_Reg(Bus, Dev, Fun, INTELX79_SMB_STAT(Ch), &Status) == FALSE)
            return FALSE;
    }

    if (CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel801_PCI: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTELX79_SMB_SBE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel801_PCI: Device Error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

int smbGetSPDIntelX79_MMIO(DWORD Bus, DWORD Dev, DWORD Fun, PBYTE MMCFGBase, PBYTE BaseAddr)
{
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    DWORD dwTSOD_CONTROL = 0;
    DWORD dwVIDDID = 0;
    unsigned long long tick = 0;
    int iSocket = 0;

    memset(tmpMsg, 0, sizeof(tmpMsg));
#ifdef WIN32
    BYTE *pbLinAddr = NULL;
    // HANDLE		hPhysicalMemory	= NULL;
    tagPhys32Struct Phys32Struct = {0};

    // Ask driver to map MMIO base address to pointer we can use
    pbLinAddr = MapPhysToLin(BaseAddr, 4096, &Phys32Struct, TRUE);

    if (pbLinAddr)
    {
#endif
        dwVIDDID = Get_PCI_Reg(Bus, 30, 1, 0);

        // Reference: Intel Xeon Processor E5/E7 v3 Product Family External Design Specification (EDS), Volume Two: Registers (Doc. No.: 507849)
        // When the SPD bus needs to be used when CLTT is enabled, write tsod_control.tsod_polling_interval = 0, wait 10ms, then the SPD bus will be idle.
        // After using the SPD bus, restore tsod_polling_interval to previous value(should be 8) to allow CLTT TSOD polling to resume
        if (dwVIDDID == 0x2f998086 || dwVIDDID == 0x6F998086) // Xeon E7 v3/Xeon E5 v3/Core i7 Power Control Unit
        {
            dwTSOD_CONTROL = Get_PCI_Reg(Bus, 30, 1, 0xE0);

            _stprintf_s(tmpMsg, _T("TSOD_CONTROL=%08X"), dwTSOD_CONTROL); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            if ((dwTSOD_CONTROL & 0x0000001F) != 0)
            {
                _stprintf_s(tmpMsg, _T("Setting TSOD_CONTROL to 0x%08X"), dwTSOD_CONTROL & 0xFFFFFFE0); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                Set_PCI_Reg(Bus, 30, 1, 0xE0, dwTSOD_CONTROL & 0xFFFFFFE0);

                // Wait 10ms
                INIT_TIMEOUT(tick);
                while (!CHECK_TIMEOUT(tick, 10))
                    ;
            }
        }

        dwVIDDID = Get_PCI_Reg(Bus, Dev, Fun, 0);

#ifndef WIN32
        if (dwVIDDID == 0x2fa88086 || dwVIDDID == 0x2f688086 || dwVIDDID == 0x6FA88086 || dwVIDDID == 0x6F688086 || dwVIDDID == 0x3CA88086)
        {
            PBYTE pbRegAddr = MMCFGBase + (0 * 0x100000) + (5 * 0x8000) + (0 * 0x1000) + 0x108;
            DWORD dwCPUBUSNO = 0;
            BYTE bCPUBus1;

            MtSupportUCDebugWriteLine(L"Reading cpubusno register");

            dwCPUBUSNO = *((DWORD *)pbRegAddr);
            _stprintf_s(tmpMsg, _T("cpubusno=%08x"), dwCPUBUSNO);

            MtSupportUCDebugWriteLine(tmpMsg);
            bCPUBus1 = (dwCPUBUSNO >> 8) & 0xFF;
            iSocket = Bus == bCPUBus1 ? 0 : 1;
        }
#endif
        for (BYTE ch = 0; ch < 2; ch++)
        {

            PBYTE pbLinBaseAddr = NULL;

            DWORD dwSMB_STAT_Old = 0;
            DWORD dwSMBCMD_Old = 0;
            DWORD dwSMBCNTL_Old = 0;
#ifdef WIN32
            pbLinBaseAddr = pbLinAddr;
#else
        pbLinBaseAddr = BaseAddr;
#endif

            // Get the current value of the registers
            dwSMB_STAT_Old = *((DWORD *)(pbLinBaseAddr + INTELX79_SMB_STAT(ch)));
            dwSMBCMD_Old = *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCMD(ch)));
            dwSMBCNTL_Old = *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCNTL(ch)));

            _stprintf_s(tmpMsg, _T("SMB_STAT[%d]=%08X"), ch, *((DWORD *)(pbLinBaseAddr + INTELX79_SMB_STAT(ch)))); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            _stprintf_s(tmpMsg, _T("SMBCMD[%d]=%08X"), ch, *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCMD(ch)))); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            _stprintf_s(tmpMsg, _T("SMBCNTL[%d]=%08X"), ch, *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCNTL(ch)))); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            DebugMessageBoxes(tmpMsg);

            if (*((DWORD *)(pbLinBaseAddr + INTELX79_SMBCNTL(ch))) == 0)
            {
                SysInfo_DebugLogWriteLine(_T("Warning: SMBus may not be present"));
                break;
            }

            for (idx = 0; idx <= 7; idx++)
            {
                int iChannel = -1, iSlot = -1;

                if (dwVIDDID == 0x2fa88086)
                {
                    const int CH_PER_SKT = 4;
                    DWORD dwPCHID = Get_PCI_Reg(0, 0x1f, 3, 0);
                    if (dwPCHID == 0x8D228086) // Xeon E5 v3 has 4 memory channels
                    {
                        const int SL_PER_CH = 2;
                        iChannel = idx < 4 ? iSocket * CH_PER_SKT + ch * SL_PER_CH : iSocket * CH_PER_SKT + ch * SL_PER_CH + 1;
                        iSlot = idx % 4;
                    }
                    else // 2 memory channels
                    {
                        iChannel = idx < 4 ? iSocket * CH_PER_SKT : iSocket * CH_PER_SKT + 1;
                        iSlot = idx % 4;
                    }
                }
                else if (dwVIDDID == 0x2f688086)
                {
                    const int CH_PER_SKT = 4;
                    iChannel = idx < 4 ? iSocket * CH_PER_SKT + 2 : iSocket * CH_PER_SKT + 3;
                    iSlot = idx % 4;
                }
                else if (dwVIDDID == 0x6FA88086)
                {
                    const int CH_PER_SKT = 4;
                    DWORD dwPCHID = Get_PCI_Reg(0, 0x1f, 3, 0);
                    if (dwPCHID == 0x8C228086) // Xeon D-1500 has 2 memory channels
                    {
                        // idx=0 -> ch=0, ch=0
                        // idx=1 -> ch=0, ch=1
                        // idx=2 -> ch=1, sl=0
                        // idx=3 -> ch=1, sl=1
                        iChannel = iSocket * CH_PER_SKT + idx / 2;
                        iSlot = idx % 2;
                    }
                    else // Xeon E5 v4 has 4 memory channels
                    {
                        iChannel = idx < 4 ? iSocket * CH_PER_SKT : iSocket * CH_PER_SKT + 1;
                        iSlot = idx % 4;
                    }
                }
                else if (dwVIDDID == 0x6F688086)
                {
                    const int CH_PER_SKT = 4;
                    iChannel = idx < 4 ? iSocket * CH_PER_SKT + 2 : iSocket * CH_PER_SKT + 3;
                    iSlot = idx % 4;
                }
                else if (dwVIDDID == 0x3CA88086)
                {
                    const int CH_PER_SKT = 4;
                    const int SL_PER_CH = 2;
                    iChannel = idx < 4 ? iSocket * CH_PER_SKT + ch * SL_PER_CH : iSocket * CH_PER_SKT + ch * SL_PER_CH + 1;
                    iSlot = idx % 4;
                }
                else
                {
                    iChannel = idx < 4 ? 0 : 1;
                    iSlot = idx % 4;
                }

                if ((dwSMBCNTL_Old & (1 << idx)))
                {
                    _stprintf_s(tmpMsg, _T("[%d] TSOD Present at Channel %d, Slot %d"), idx, iChannel, iSlot);
#ifdef WIN32
                    SysInfo_DebugLogWriteLine(tmpMsg);
#else
                MtSupportUCDebugWriteLine(tmpMsg);
#endif
                }

                if (smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0xA, 0, (BYTE)idx, FALSE, FALSE, &Data))
                {
                    BOOL bUseByteAccess = TRUE;
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    _stprintf_s(tmpMsg, _T("Device detected on SMBUS at channel %d, slot %d (Intel801_MMIO)"), ch, idx); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    DebugMessageBoxes(tmpMsg);

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = iChannel;
                    g_MemoryInfo[g_numMemModules].slot = iSlot;

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    SysInfo_DebugLogWriteLine(_T("D - Setting bank address to 0"));
                    smbSetBankAddrIntelX79_MMIO(pbLinBaseAddr, ch, 0);
                    SysInfo_DebugLogWriteLine(_T("D - Finished setting bank address to 0"));

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);
                    if (smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0xA, 0, (BYTE)idx, TRUE, FALSE, &Data)) // See if we can read word-by-word
                    {
                        bUseByteAccess = FALSE;
                        for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                        {
                            int iErrCount = 0;

                            if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                            {
                                SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                                break;
                            }

                            Data = 0;

                            while (!smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0xA, (BYTE)ioffset, (BYTE)idx, TRUE, FALSE, &Data) && iErrCount < 10)
                            {
                                iErrCount++;
                            }

                            if (iErrCount >= 10)
                            {
                                _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                SysInfo_DebugLogWriteLine(tmpMsg);

                                bUseByteAccess = TRUE;
                                break;
                            }

                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                            spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                        }
                    }

                    if (bUseByteAccess) // Read byte-by-byte
                    {
                        SysInfo_DebugLogWriteLine(_T("Word access failed, attempting byte access"));
                        for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                        {
                            int iErrCount = 0;

                            if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                            {
                                SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                                break;
                            }

                            Data = 0;

                            while (!smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0xA, (BYTE)ioffset, (BYTE)idx, FALSE, FALSE, &Data) && iErrCount < 10)
                            {
                                iErrCount++;
                            }

                            if (iErrCount >= 10)
                            {
                                _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                SysInfo_DebugLogWriteLine(tmpMsg);
                                break;
                            }

                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                        }
                    }

                    if (IsDDR4SPD(spdData, iNumberOfSPDBytes))
                    {
                        SysInfo_DebugLogWriteLine(_T("E - Setting bank address to 1"));
                        smbSetBankAddrIntelX79_MMIO(pbLinBaseAddr, ch, 1);
                        SysInfo_DebugLogWriteLine(_T("E - Finished setting bank address to 1"));

                        iNumberOfSPDBytes = 512;

                        ioffset = 256;
                        _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                        SysInfo_DebugLogWriteLine(tmpMsg);
                        if (bUseByteAccess) // Read byte-by-byte
                        {
                            for (ioffset = 256; ioffset < 512; ioffset++)
                            {
                                int iErrCount = 0;

                                Data = 0;

                                while (!smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0xA, (BYTE)(ioffset & 0xFF), (BYTE)idx, FALSE, FALSE, &Data) && iErrCount < 10)
                                {
                                    iErrCount++;
                                }

                                if (iErrCount >= 10)
                                {
                                    _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                    SysInfo_DebugLogWriteLine(tmpMsg);
                                    break;
                                }

                                spdData[ioffset] = (BYTE)(Data & 0xFF);
                            }
                        }
                        else
                        {
                            for (ioffset = 256; ioffset < 512; ioffset += 2)
                            {
                                int iErrCount = 0;

                                Data = 0;

                                while (!smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0xA, (BYTE)(ioffset & 0xFF), (BYTE)idx, TRUE, FALSE, &Data) && iErrCount < 10)
                                {
                                    iErrCount++;
                                }

                                if (iErrCount >= 10)
                                {
                                    _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                    SysInfo_DebugLogWriteLine(tmpMsg);
                                    break;
                                }

                                spdData[ioffset] = (BYTE)(Data & 0xFF);
                                spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                            }
                        }

                        SysInfo_DebugLogWriteLine(_T("F - Setting bank address back to 0"));
                        smbSetBankAddrIntelX79_MMIO(pbLinBaseAddr, ch, 0);
                        SysInfo_DebugLogWriteLine(_T("F - Finished setting bank address to 0"));
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }

                // TSOD
                if (smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0x3, 0, (BYTE)idx, FALSE, FALSE, &Data))
                {
                    _stprintf_s(tmpMsg, _T("[1] TSOD value=%08X"), Data);
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }

                if (smbCallBusIntelX79_MMIO(pbLinBaseAddr, ch, 0x3, 0, (BYTE)idx, FALSE, TRUE, &Data))
                {
                    _stprintf_s(tmpMsg, _T("[2] TSOD value=%08X"), Data);
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }

                _stprintf_s(tmpMsg, _T("SMB_STAT[%d]=%08X"), ch, *((DWORD *)(pbLinBaseAddr + INTELX79_SMB_STAT(ch)))); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);

                _stprintf_s(tmpMsg, _T("SMBCMD[%d]=%08X"), ch, *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCMD(ch)))); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);

                _stprintf_s(tmpMsg, _T("SMBCNTL[%d]=%08X"), ch, *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCNTL(ch)))); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
            }

            _stprintf_s(tmpMsg, _T("Restoring old SMBus register values for channel %i"), ch); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            *((DWORD *)(pbLinBaseAddr + INTELX79_SMB_STAT(ch))) = dwSMB_STAT_Old;
            *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCMD(ch))) = dwSMBCMD_Old;
            *((DWORD *)(pbLinBaseAddr + INTELX79_SMBCNTL(ch))) = dwSMBCNTL_Old;
        }

        // Resume TSOD polling
        if ((dwTSOD_CONTROL & 0x0000001F) != 0)
        {
            _stprintf_s(tmpMsg, _T("Restoring TSOD_CONTROL back to 0x%08X"), dwTSOD_CONTROL); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);
            Set_PCI_Reg(Bus, 30, 1, 0xE0, dwTSOD_CONTROL);
        }

#ifdef WIN32
        UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 32);
    }
#endif

    return numMemSticks;
}

BOOL smbCallBusIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh, BYTE bDTI, BYTE bOffset, BYTE bSlot, BOOL bWord, BOOL bTSOD, DWORD *Data)
{
    DWORD Result = 0;
    DWORD Command = 0;
    DWORD Control = 0;

    if (smbWaitForFreeIntelX79_MMIO(pbLinAddr, bCh) == FALSE)
        return FALSE;

    // Make sure DTI is set to 0xA and TSOD polling is disabled in the SMBCNTL register
#ifdef WIN32
    Control = *((DWORD *)(pbLinAddr + INTELX79_SMBCNTL(bCh)));
    Control &= 0x07FFFCFF;
    Control |= (bDTI << 28) | 0x08000000;
    if (bTSOD)
        Control |= INTELX79_SMB_TSOD_POLL_EN | INTELX79_SMB_START_TSOD_POLL;

    *((DWORD *)(pbLinAddr + INTELX79_SMBCNTL(bCh))) = Control;
#else
    Control = MmioRead32((UINTN)pbLinAddr + INTELX79_SMBCNTL(bCh));
    Control &= 0x07FFFCFF;
    Control |= (bDTI << 28) | 0x08000000;

    MmioWrite32((UINTN)pbLinAddr + INTELX79_SMBCNTL(bCh), Control);
#endif

    Command = INTELX79_SMB_BA(bOffset) | INTELX79_SMB_SA(bSlot);
    if (bWord)
        Command |= INTELX79_SMB_WORD_ACCESS;

#ifdef WIN32
    *((DWORD *)(pbLinAddr + INTELX79_SMBCMD(bCh))) = Command;
    *((DWORD *)(pbLinAddr + INTELX79_SMBCMD(bCh))) |= INTELX79_SMB_CMD_TRIGGER;
#else
    MmioWrite32((UINTN)pbLinAddr + INTELX79_SMBCMD(bCh), Command);
    MmioOr32((UINTN)pbLinAddr + INTELX79_SMBCMD(bCh), INTELX79_SMB_CMD_TRIGGER);
#endif
    // Sleep(1);
    if (smbWaitForEndIntelX79_MMIO(pbLinAddr, bCh) == FALSE)
        return FALSE;
#ifdef WIN32
    Result = INTELX79_SMB_RDATA(*((DWORD *)(pbLinAddr + INTELX79_SMB_STAT(bCh))));
#else
    Result = INTELX79_SMB_RDATA(MmioRead32((UINTN)pbLinAddr + INTELX79_SMB_STAT(bCh)));
#endif
    *Data = ((Result << 8) & 0x0000FF00) | ((Result >> 8) & 0x000000FF);

    return TRUE;
}

BOOL smbSetBankAddrIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh, BYTE bBankNo)
{
    DWORD Command = 0;
    DWORD Control = 0;

    if (smbWaitForFreeIntelX79_MMIO(pbLinAddr, bCh) == FALSE)
        return FALSE;

    // Make sure DTI is set to 0x6 and TSOD polling is disabled in the SMBCNTL register
#ifdef WIN32
    Control = *((DWORD *)(pbLinAddr + INTELX79_SMBCNTL(bCh)));
    Control &= 0x07FFFCFF;
    Control |= 0x68000000;

    *((DWORD *)(pbLinAddr + INTELX79_SMBCNTL(bCh))) = Control;
#else
    Control = MmioRead32((UINTN)pbLinAddr + INTELX79_SMBCNTL(bCh));
    Control &= 0x07FFFCFF;
    Control |= 0x68000000;

    MmioWrite32((UINTN)pbLinAddr + INTELX79_SMBCNTL(bCh), Control);
#endif

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntelX79_MMIO: Setting SMBCNTL=%08X"), Control);
    // SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    if (bBankNo)
        Command = INTELX79_SMB_SA(0x7) | INTELX79_SMB_WRT_CMD;
    else
        Command = INTELX79_SMB_SA(0x6) | INTELX79_SMB_WRT_CMD;

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntelX79_MMIO: Setting SMBCMD=%08X"), Command | INTELX79_SMB_CMD_TRIGGER);
    // SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
    *((DWORD *)(pbLinAddr + INTELX79_SMBCMD(bCh))) = Command;
    *((DWORD *)(pbLinAddr + INTELX79_SMBCMD(bCh))) |= INTELX79_SMB_CMD_TRIGGER;
#else
    MmioWrite32((UINTN)pbLinAddr + INTELX79_SMBCMD(bCh), Command);
    MmioOr32((UINTN)pbLinAddr + INTELX79_SMBCMD(bCh), INTELX79_SMB_CMD_TRIGGER);
#endif

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrIntelX79_MMIO: Waiting for end"));
    // SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Sleep(1);
    if (smbWaitForEndIntelX79_MMIO(pbLinAddr, bCh) == FALSE)
    {
        // SysInfo_DebugLogWriteLine(_T("smbSetBankAddrIntelX79_MMIO failed"));
        return FALSE;
    }

    return TRUE;
}

BOOL smbWaitForFreeIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh)
{
    volatile DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

#ifdef WIN32
    Status = *((DWORD *)(pbLinAddr + INTELX79_SMB_STAT(bCh)));
#else
    Status = MmioRead32((UINTN)pbLinAddr + INTELX79_SMB_STAT(bCh));
#endif

    while (((Status & INTELX79_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
    {
        // Sleep(1);

#ifdef WIN32
        Status = *((DWORD *)(pbLinAddr + INTELX79_SMB_STAT(bCh)));
#else
        Status = MmioRead32((UINTN)pbLinAddr + INTELX79_SMB_STAT(bCh));
#endif
    }

    if (CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh)
{
    volatile DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

#ifdef WIN32
    Status = *((DWORD *)(pbLinAddr + INTELX79_SMB_STAT(bCh)));
#else
    Status = MmioRead32((UINTN)pbLinAddr + INTELX79_SMB_STAT(bCh));
#endif

    while (((Status & INTELX79_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
    {
        // Sleep(1);

#ifdef WIN32
        Status = *((DWORD *)(pbLinAddr + INTELX79_SMB_STAT(bCh)));
#else
        Status = MmioRead32((UINTN)pbLinAddr + INTELX79_SMB_STAT(bCh));
#endif
    }

    if (CHECK_TIMEOUT(timeout, INTELX79_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntelX79_MMIO: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTELX79_SMB_SBE)
    {
        return FALSE;
    }

    return TRUE;
}

/* Intel PCU SMBus PCI offsets */
#define INTELPCU_SMB_PERIOD_CFG_0 0x70
#define INTELPCU_SMB_PERIOD_CFG_1 0x74
#define INTELPCU_SMB_PERIOD_CNTR_CFG_0 0x78
#define INTELPCU_SMB_PERIOD_CNTR_CFG_1 0x7C
#define INTELPCU_SMB_PERIOD_CNTR_CFG_2 0x80
#define INTELPCU_SMB_TSOD_POLL_RATE_CFG_0 0x84
#define INTELPCU_SMB_TSOD_POLL_RATE_CFG_1 0x88
#define INTELPCU_SMB_MUX_CONFIG_CFG_0 0x8C
#define INTELPCU_SMB_MUX_CONFIG_CFG_1 0x90
#define INTELPCU_SMB_TSOD_CONFIG_CFG_0 0x94
#define INTELPCU_SMB_TSOD_CONFIG_CFG_1 0x98
#define INTELPCU_SMB_CMD_CFG_0 0x9C
#define INTELPCU_SMB_CMD_CFG_1 0xA0
#define INTELPCU_SMB_CMD_CFG_2 0xA4
#define INTELPCU_SMB_STATUS_CFG_0 0xA8
#define INTELPCU_SMB_STATUS_CFG_1 0xAC
#define INTELPCU_SMB_STATUS_CFG_2 0xB0
#define INTELPCU_SMB_DATA_CFG_0 0xB4
#define INTELPCU_SMB_DATA_CFG_1 0xB8
#define INTELPCU_SMB_DATA_CFG_2 0xBC
#define INTELPCU_SMB_TSOD_POLL_RATE_CNTR_CFG_0 0xC0
#define INTELPCU_SMB_TSOD_POLL_RATE_CNTR_CFG_1 0xC4
#define INTELPCU_SMB_TSOD_POLL_RATE_CNTR_CFG_2 0xC8
#define INTELPCU_SMB_TLOW_TIMEOUT_CNTR_CFG_0 0xCC
#define INTELPCU_SMB_TLOW_TIMEOUT_CNTR_CFG_1 0xD0
#define INTELPCU_SMB_TLOW_TIMEOUT_CNTR_CFG_2 0xD4

static const DWORD INTELPCU_SMB_STATUS_REG_LIST[] = {INTELPCU_SMB_STATUS_CFG_0, INTELPCU_SMB_STATUS_CFG_1 /*, INTELPCU_SMB_STATUS_CFG_2 */};
static const DWORD INTELPCU_SMB_CMD_REG_LIST[] = {INTELPCU_SMB_CMD_CFG_0, INTELPCU_SMB_CMD_CFG_1 /*, INTELPCU_SMB_CMD_CFG_2 */};
static const DWORD INTELPCU_SMB_DATA_REG_LIST[] = {INTELPCU_SMB_DATA_CFG_0, INTELPCU_SMB_DATA_CFG_1 /*, INTELPCU_SMB_DATA_CFG_2 */};
static const DWORD INTELPCU_SMB_TSOD_CONFIG_REG_LIST[] = {INTELPCU_SMB_TSOD_CONFIG_CFG_0, INTELPCU_SMB_TSOD_CONFIG_CFG_1};
static const DWORD INTELPCU_SMB_MUX_CONFIG_REG_LIST[] = {INTELPCU_SMB_MUX_CONFIG_CFG_0, INTELPCU_SMB_MUX_CONFIG_CFG_1};
static const DWORD INTELPCU_SMB_TSOD_POLL_RATE_REG_LIST[] = {INTELPCU_SMB_TSOD_POLL_RATE_CFG_0, INTELPCU_SMB_TSOD_POLL_RATE_CFG_1};
static const DWORD INTELPCU_SMB_PERIOD_CNTR_REG_LIST[] = {INTELPCU_SMB_PERIOD_CNTR_CFG_0, INTELPCU_SMB_PERIOD_CNTR_CFG_1 /*, INTELPCU_SMB_PERIOD_CNTR_CFG_2 */};
static const DWORD INTELPCU_SMB_PERIOD_CFG_REG_LIST[] = {INTELPCU_SMB_PERIOD_CFG_0, INTELPCU_SMB_PERIOD_CFG_1};

/* Intel PCU SMBus Status register bits */

#define INTELPCU_SMB_WOD (1 << 3)
#define INTELPCU_SMB_RDO (1 << 2)
#define INTELPCU_SMB_SBE (1 << 1)
#define INTELPCU_SMB_BUSY (1 << 0)

#define INTELPCU_SMB_TSOD_SA(x) ((x >> 8) & 0x07)
#define INTELPCU_SMB_LAST_DTI(x) ((x >> 11) & 0x0F)
#define INTELPCU_SMB_LAST_BRANCH_CFG(x) ((x >> 16) & 0x07)

#define INTELPCU_SMB_CKOVRD (1 << 29)
#define INTELPCU_SMB_DIS_WRT (1 << 28)
#define INTELPCU_SMB_SBE_EN (1 << 27)
#define INTELPCU_SMB_SBE_SMI_EN (1 << 26)
#define INTELPCU_SMB_SBE_ERR0_EN (1 << 25)
#define INTELPCU_SMB_SOFT_RST (1 << 24)
#define INTELPCU_SMB_TSOD_POLL_EN (1 << 20)
#define INTELPCU_SMB_CMD_TRIGGER (1 << 19)
#define INTELPCU_SMB_PNTR_SEL (1 << 18)
#define INTELPCU_SMB_WORD_ACCESS (1 << 17)
#define INTELPCU_SMB_WRT(x) ((x & 0x03) << 15)
#define INTELPCU_SMB_DTI(x) ((x & 0x0F) << 11)
#define INTELPCU_SMB_SA(x) ((x & 0x07) << 8)
#define INTELPCU_SMB_BA(x) (x & 0xFF)

#define INTELPCU_SMB_WDATA(x) ((x << 16) & 0xFFFF)
#define INTELPCU_SMB_RDATA(x) (x & 0xFFFF)

#define INTELPCU_SMB_GROUP0_DIMM_PRESENT 0x0F

#define INTELPCU_SMB_MUX_EXISTS (1 << 0)
#define INTELPCU_SMB_BRANCH_EXISTS(x) ((x << 1) & 0x0F)
#define INTELPCU_SMB_MUX_ADDRESS(x) ((x << 5) & 0x07)

#define INTELPCU_SMB_MUX_BRANCH0_CONFIG(x) ((x << 16) & 0x07)
#define INTELPCU_SMB_MUX_BRANCH1_CONFIG(x) ((x << 20) & 0x07)
#define INTELPCU_SMB_MUX_BRANCH2_CONFIG(x) ((x << 24) & 0x07)
#define INTELPCU_SMB_MUX_BRANCH3_CONFIG(x) ((x << 28) & 0x07)

#define INTELPCU_SMB_TSOD_POLL_RATE(x) (x & 0x0003FFFF)

#define INTELPCU_SMB_TSOD_POLL_RATE_CNTR 0x0003FFFF

#define INTELPCU_SMB_CLK_PRD_CNTR 0xFFFF

#define INTELPCU_SMB_CLK_PRD(x) (x & 0xFFFF)
#define INTELPCU_SMB_CLK_OFFSET(x) ((x << 16) & 0xFFFF)

#define INTELPCU_MAX_TIMEOUT 250

int smbGetSPDIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun)
{
    int numreg = sizeof(INTELPCU_SMB_STATUS_REG_LIST) / sizeof(INTELPCU_SMB_STATUS_REG_LIST[0]);
    int reg, idx, ioffset, iNumberOfSPDBytes;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    for (reg = 0; reg < numreg; reg++)
    {
        DWORD Data = 0;

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_STATUS_REG_LIST[reg]);
        _stprintf_s(tmpMsg, _T("SMB_STATUS_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_CMD_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_CMD_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_DATA_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_DATA_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_TSOD_CONFIG_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_TSOD_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_MUX_CONFIG_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_MUX_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        for (idx = 0; idx <= 7; idx++)
        {
            if (smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0xA, 0, (BYTE)idx, FALSE, FALSE, &Data))
            {
                BOOL bUseByteAccess = TRUE;
                BYTE spdData[MAX_SPD_LENGTH];
                memset(spdData, 0, sizeof(spdData));

                _stprintf_s(tmpMsg, _T("Device detected on SMBUS on port %d, slot %d (IntelPCU)"), reg, idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);

                DebugMessageBoxes(tmpMsg);

                iNumberOfSPDBytes = 256;
                // if (Data > 64)
                //	iNumberOfSPDBytes = Data;
                if (g_numMemModules >= MAX_MEMORY_SLOTS)
                    expand_spd_info_array();

                if (g_numMemModules < MAX_MEMORY_SLOTS)
                    g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                g_MemoryInfo[g_numMemModules].channel = -1;
                g_MemoryInfo[g_numMemModules].slot = -1;

                // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                //  and are not defined in the spec.

                SysInfo_DebugLogWriteLine(_T("Setting bank address to 0"));
                smbSetBankAddrIntelPCU(Bus, Dev, Fun, reg, 0);
                SysInfo_DebugLogWriteLine(_T("Finished setting bank address to 0"));

                ioffset = 0;
                _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                SysInfo_DebugLogWriteLine(tmpMsg);
                if (smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0xA, 0, (BYTE)idx, TRUE, FALSE, &Data)) // See if we can read word-by-word
                {
                    bUseByteAccess = FALSE;
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        int iErrCount = 0;

                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        Data = 0;

                        while (!smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0xA, (BYTE)ioffset, (BYTE)idx, TRUE, FALSE, &Data) && iErrCount < 10)
                        {
                            iErrCount++;
                        }

                        if (iErrCount >= 10)
                        {
                            _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                            SysInfo_DebugLogWriteLine(tmpMsg);

                            bUseByteAccess = TRUE;
                            break;
                        }

                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }
                }

                if (bUseByteAccess) // Read byte-by-byte
                {
                    SysInfo_DebugLogWriteLine(_T("Word access failed, attempting byte access"));
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                    {
                        int iErrCount = 0;

                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        Data = 0;

                        while (!smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0xA, (BYTE)ioffset, (BYTE)idx, FALSE, FALSE, &Data) && iErrCount < 10)
                        {
                            iErrCount++;
                        }

                        if (iErrCount >= 10)
                        {
                            _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                            SysInfo_DebugLogWriteLine(tmpMsg);
                            break;
                        }

                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                    }
                }

                if (IsDDR4SPD(spdData, iNumberOfSPDBytes))
                {
                    SysInfo_DebugLogWriteLine(_T("Setting bank address to 1"));
                    smbSetBankAddrIntelPCU(Bus, Dev, Fun, reg, 1);
                    SysInfo_DebugLogWriteLine(_T("Finished setting bank address to 1"));

                    iNumberOfSPDBytes = 512;

                    ioffset = 256;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);
                    if (bUseByteAccess) // Read byte-by-byte
                    {
                        for (ioffset = 256; ioffset < 512; ioffset++)
                        {
                            int iErrCount = 0;

                            Data = 0;

                            while (!smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0xA, (BYTE)(ioffset & 0xFF), (BYTE)idx, FALSE, FALSE, &Data) && iErrCount < 10)
                            {
                                iErrCount++;
                            }

                            if (iErrCount >= 10)
                            {
                                _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                SysInfo_DebugLogWriteLine(tmpMsg);
                                break;
                            }

                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                        }
                    }
                    else
                    {
                        for (ioffset = 256; ioffset < 512; ioffset += 2)
                        {
                            int iErrCount = 0;

                            Data = 0;

                            while (!smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0xA, (BYTE)(ioffset & 0xFF), (BYTE)idx, TRUE, FALSE, &Data) && iErrCount < 10)
                            {
                                iErrCount++;
                            }

                            if (iErrCount >= 10)
                            {
                                _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                SysInfo_DebugLogWriteLine(tmpMsg);
                                break;
                            }

                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                            spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                        }
                    }

                    SysInfo_DebugLogWriteLine(_T("Setting bank address back to 0"));
                    smbSetBankAddrIntelPCU(Bus, Dev, Fun, reg, 0);
                    SysInfo_DebugLogWriteLine(_T("Finished setting bank address to 0"));
                }

                if (g_numMemModules < MAX_MEMORY_SLOTS)
                {
                    if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                    {
                        g_numMemModules++;
                        numMemSticks++;
                    }
                }
                else
                {
                    _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }
            }
        }
    }

    return numMemSticks;
}

int smbGetTSODIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun)
{
    int numreg = sizeof(INTELPCU_SMB_STATUS_REG_LIST) / sizeof(INTELPCU_SMB_STATUS_REG_LIST[0]);
    int reg, idx, ioffset;
    int numTSOD = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    for (reg = 0; reg < numreg; reg++)
    {
        DWORD Data = 0;

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_STATUS_REG_LIST[reg]);
        _stprintf_s(tmpMsg, _T("SMB_STATUS_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_CMD_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_CMD_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_DATA_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_DATA_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_TSOD_CONFIG_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_TSOD_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_MUX_CONFIG_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_MUX_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        for (idx = 0; idx <= 7; idx++)
        {
            if (smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0x3, 0, (BYTE)idx, FALSE, FALSE, &Data))
            {
                WORD tsodData[MAX_TSOD_LENGTH];
                memset(tsodData, 0, sizeof(tsodData));

                _stprintf_s(tmpMsg, _T("Device detected on SMBUS on port %d, slot %d (IntelPCU)"), reg, idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);

                DebugMessageBoxes(tmpMsg);

                // if (Data > 64)
                //	iNumberOfSPDBytes = Data;
                if (g_numTSODModules >= MAX_TSOD_SLOTS)
                    expand_tsod_info_array();

                g_MemTSODInfo[g_numTSODModules].channel = -1;
                g_MemTSODInfo[g_numTSODModules].slot = -1;

                ioffset = 0;
                _stprintf_s(tmpMsg, _T("Retrieving TSOD words %d-%d"), ioffset, MAX_TSOD_LENGTH - 1);
                SysInfo_DebugLogWriteLine(tmpMsg);

                // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                //  and are not defined in the spec.
                for (ioffset = 0; ioffset < MAX_TSOD_LENGTH; ioffset++)
                {
                    if (smbCallBusIntelPCU(Bus, Dev, Fun, reg, 0x3, 0, (BYTE)idx, TRUE, FALSE, &Data))
                        tsodData[ioffset] = SWAP_16(Data & 0xFFFF);
                }

                if (!CheckSPDBytes((BYTE *)tsodData, MAX_TSOD_LENGTH * sizeof(tsodData[0])))
                    continue;

                if (g_numTSODModules < MAX_TSOD_SLOTS)
                {
                    int temp;
#ifdef WIN32
                    float fTemp;
#endif
                    int i;

                    _stprintf_s(tmpMsg, _T("Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    tmpMsg[0] = L'\0';
                    for (i = 0; i < MAX_TSOD_LENGTH; i++)
                    {
                        TCHAR strSPDBytes[8];
                        _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                        _tcscat(tmpMsg, strSPDBytes);
                    }

                    SysInfo_DebugLogWriteLine(tmpMsg);

                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCap = tsodData[0];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCfg = tsodData[1];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wHiLim = tsodData[2];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wLoLim = tsodData[3];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCritLim = tsodData[4];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wAmbTemp = tsodData[5];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wManufID = tsodData[6];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wDevRev = tsodData[7];
                    memcpy(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor, tsodData + 8, sizeof(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor));

                    if (!CheckTSOD(&g_MemTSODInfo[g_numTSODModules]))
                    {
                        _stprintf_s(tmpMsg, _T("**WARNING** TSOD may be invalid. Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        tmpMsg[0] = L'\0';
                        for (i = 0; i < MAX_TSOD_LENGTH; i++)
                        {
                            TCHAR strSPDBytes[8];
                            _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                            _tcscat(tmpMsg, strSPDBytes);
                        }
                        MtSupportUCDebugWriteLine(tmpMsg);

                        if (!CheckCLTTPolling(tsodData))
                            continue;

                        SysInfo_DebugLogF(L"Found repeated TSOD words likely from CLTT. Continue parsing TSOD data");
                    }

                    // High limit
                    temp = tsodData[2] & 0xFFC;
                    if (BitExtract(tsodData[2], 12, 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d high limit: %.3fC"), g_numTSODModules, fTemp);
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // Low limit
                    temp = tsodData[3] & 0xFFC;
                    if (BitExtract(tsodData[3], 12, 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d low limit: %.3fC"), g_numTSODModules, fTemp);
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // TCRIT limit
                    temp = tsodData[4] & 0xFFC;
                    if (BitExtract(tsodData[4], 12, 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %.3fC"), g_numTSODModules, fTemp);
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // Current temperature
                    temp = tsodData[5] & 0xFFF;
                    if (tsodData[5] & (1 << 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d temperature: %.3fC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, fTemp, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));

                    if (abs((int)fTemp) < MAX_TSOD_TEMP)
                    {
                        g_MemTSODInfo[g_numTSODModules].temperature = fTemp;
                        g_numTSODModules++;
                        numTSOD++;
                    }
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d temperature: %d.%03dC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, temp / 1000, temp % 1000, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));

                    if (ABS(temp) < MAX_TSOD_TEMP)
                    {
                        g_MemTSODInfo[g_numTSODModules].temperature = temp;
                        g_numTSODModules++;
                        numTSOD++;
                    }
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }
                else
                {
                    _stprintf_s(tmpMsg, _T("Maximum number of TSOD modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }
            }
        }
    }

    return numTSOD;
}

DISABLE_OPTIMISATIONS() // Optimisation must be turned off
BOOL smbCallBusIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bDTI, BYTE bOffset, BYTE bSlot, BOOL bWord, BOOL bTSOD, DWORD *Data)
{
    DWORD Result = 0;
    DWORD Command = 0;
    const int NUM_RETRIES = 5;
    int i;

    for (i = 0; i < NUM_RETRIES; i++)
    {
        if (smbWaitForFreeIntelPCU(Bus, Dev, Fun, SMBPort) == FALSE)
            return FALSE;

        // Make sure DTI is set to 0xA and TSOD polling is disabled in the SMBCNTL register
        Command = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_CMD_REG_LIST[SMBPort]);

        Command &= 0xFFE00000;
        Command |= INTELPCU_SMB_BA(bOffset);
        Command |= INTELPCU_SMB_SA(bSlot);
        Command |= INTELPCU_SMB_DTI(bDTI);
        Command |= INTELPCU_SMB_WRT(0);
        if (bWord)
            Command |= INTELPCU_SMB_WORD_ACCESS;
        if (bTSOD)
            Command |= INTELPCU_SMB_TSOD_POLL_EN;
        Set_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_CMD_REG_LIST[SMBPort], Command | INTELPCU_SMB_CMD_TRIGGER);

        Sleep(5);
        if (smbWaitForEndIntelPCU(Bus, Dev, Fun, SMBPort))
        {

            Result = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_DATA_REG_LIST[SMBPort]);

            Result = INTELPCU_SMB_RDATA(Result);

            if (bWord)
                *Data = ((Result << 8) & 0x0000FF00) | ((Result >> 8) & 0x000000FF);
            else
                *Data = Result & 0x000000FF;

            return TRUE;
        }
        Sleep(5);
    }
    return FALSE;
}

BOOL smbSetBankAddrIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bBankNo)
{
    DWORD Command = 0;
    const int NUM_RETRIES = 5;
    int i;

    for (i = 0; i < NUM_RETRIES; i++)
    {
        if (smbWaitForFreeIntelPCU(Bus, Dev, Fun, SMBPort) == FALSE)
            return FALSE;

        // Make sure DTI is set to 0x6 and TSOD polling is disabled in the SMBCNTL register
        Command = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_CMD_REG_LIST[SMBPort]);
        Command &= 0xFFE00000;
        Command |= INTELPCU_SMB_DTI(0x6);
        Command |= INTELPCU_SMB_WRT(0x1);

        if (bBankNo)
            Command |= INTELPCU_SMB_SA(0x7);
        else
            Command |= INTELPCU_SMB_SA(0x6);

        Set_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_CMD_REG_LIST[SMBPort], Command | INTELPCU_SMB_CMD_TRIGGER);

        Sleep(5);
        if (smbWaitForEndIntelPCU(Bus, Dev, Fun, SMBPort))
            return TRUE;
        Sleep(5);
    }
    return FALSE;
}

BOOL smbWaitForFreeIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_STATUS_REG_LIST[SMBPort]);

    while (((Status & INTELPCU_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELPCU_MAX_TIMEOUT))
    {
        Sleep(1);

        Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_STATUS_REG_LIST[SMBPort]);
    }

    if (CHECK_TIMEOUT(timeout, INTELPCU_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_STATUS_REG_LIST[SMBPort]);

    while (((Status & INTELPCU_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELPCU_MAX_TIMEOUT))
    {
        Sleep(1);

        Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_SMB_STATUS_REG_LIST[SMBPort]);
    }

    if (CHECK_TIMEOUT(timeout, INTELPCU_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntelPCU: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTELPCU_SMB_SBE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntelPCU: Device Error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}
ENABLE_OPTIMISATIONS()

/* Intel PCU SMBus PCI offsets */
#define INTELPCU_IL_SMB_CMD_CFG_0 0x80
#define INTELPCU_IL_SMB_STATUS_CFG_0 0x84
#define INTELPCU_IL_SMB_DATA_CFG_0 0x88
#define INTELPCU_IL_SMB_500NS_CONFIG_CFG_0 0x8C
#define INTELPCU_IL_SMB_PERIOD_CFG_0 0x90
#define INTELPCU_IL_SMB_TLOW_TIMEOUT_CNTR_CFG_0 0x94
#define INTELPCU_IL_SMB_TSOD_POLL_RATE_CFG_0 0x98
#define INTELPCU_IL_SMB_TSOD_CONFIG_CFG_0 0x9C

static const DWORD INTELPCU_IL_SMB_STATUS_REG_LIST[] = {INTELPCU_IL_SMB_STATUS_CFG_0};
static const DWORD INTELPCU_IL_SMB_CMD_REG_LIST[] = {INTELPCU_IL_SMB_CMD_CFG_0};
static const DWORD INTELPCU_IL_SMB_DATA_REG_LIST[] = {INTELPCU_IL_SMB_DATA_CFG_0};
static const DWORD INTELPCU_IL_SMB_TSOD_CONFIG_REG_LIST[] = {INTELPCU_IL_SMB_TSOD_CONFIG_CFG_0};
static const DWORD INTELPCU_IL_SMB_TSOD_POLL_RATE_REG_LIST[] = {INTELPCU_IL_SMB_TSOD_POLL_RATE_CFG_0};
static const DWORD INTELPCU_IL_SMB_500NS_CONFIG_REG_LIST[] = {INTELPCU_IL_SMB_500NS_CONFIG_CFG_0};
static const DWORD INTELPCU_IL_SMB_PERIOD_CFG_REG_LIST[] = {INTELPCU_IL_SMB_PERIOD_CFG_0};

/* Intel PCU SMBus Status register bits */

#define INTELPCU_IL_SMB_WOD (1 << 3)
#define INTELPCU_IL_SMB_RDO (1 << 2)
#define INTELPCU_IL_SMB_SBE (1 << 1)
#define INTELPCU_IL_SMB_BUSY (1 << 0)

#define INTELPCU_IL_SMB_TSOD_SA(x) ((x >> 8) & 0x07)
#define INTELPCU_IL_SMB_LAST_DTI(x) ((x >> 11) & 0x0F)
#define INTELPCU_IL_SMB_LAST_BRANCH_CFG(x) ((x >> 16) & 0x07)

#define INTELPCU_IL_SMB_CKOVRD (1 << 29)
#define INTELPCU_IL_SMB_DIS_WRT (1 << 28)
#define INTELPCU_IL_SMB_SBE_EN (1 << 27)
#define INTELPCU_IL_SMB_SBE_SMI_EN (1 << 26)
#define INTELPCU_IL_SMB_SBE_ERR0_EN (1 << 25)
#define INTELPCU_IL_SMB_SOFT_RST (1 << 24)
#define INTELPCU_IL_SMB_TSOD_POLL_EN (1 << 20)
#define INTELPCU_IL_SMB_CMD_TRIGGER (1 << 19)
#define INTELPCU_IL_SMB_PNTR_SEL (1 << 18)
#define INTELPCU_IL_SMB_WORD_ACCESS (1 << 17)
#define INTELPCU_IL_SMB_WRT(x) ((x & 0x03) << 15)
#define INTELPCU_IL_SMB_DTI(x) ((x & 0x0F) << 11)
#define INTELPCU_IL_SMB_SA(x) ((x & 0x07) << 8)
#define INTELPCU_IL_SMB_BA(x) (x & 0xFF)

#define INTELPCU_IL_SMB_WDATA(x) ((x << 16) & 0xFFFF)
#define INTELPCU_IL_SMB_RDATA(x) (x & 0xFFFF)

#define INTELPCU_IL_SMB_GROUP0_DIMM_PRESENT 0x0F

#define INTELPCU_IL_SMB_MUX_EXISTS (1 << 0)
#define INTELPCU_IL_SMB_BRANCH_EXISTS(x) ((x << 1) & 0x0F)
#define INTELPCU_IL_SMB_MUX_ADDRESS(x) ((x << 5) & 0x07)

#define INTELPCU_IL_SMB_MUX_BRANCH0_CONFIG(x) ((x << 16) & 0x07)
#define INTELPCU_IL_SMB_MUX_BRANCH1_CONFIG(x) ((x << 20) & 0x07)
#define INTELPCU_IL_SMB_MUX_BRANCH2_CONFIG(x) ((x << 24) & 0x07)
#define INTELPCU_IL_SMB_MUX_BRANCH3_CONFIG(x) ((x << 28) & 0x07)

#define INTELPCU_IL_SMB_TSOD_POLL_RATE(x) (x & 0x0003FFFF)

#define INTELPCU_IL_SMB_TSOD_POLL_RATE_CNTR 0x0003FFFF

#define INTELPCU_IL_SMB_CLK_PRD_CNTR 0xFFFF

#define INTELPCU_IL_SMB_CLK_PRD(x) (x & 0xFFFF)
#define INTELPCU_IL_SMB_CLK_OFFSET(x) ((x << 16) & 0xFFFF)

#define INTELPCU_IL_MAX_TIMEOUT 250

int smbGetSPDIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun)
{
    int numreg = sizeof(INTELPCU_IL_SMB_STATUS_REG_LIST) / sizeof(INTELPCU_IL_SMB_STATUS_REG_LIST[0]);
    int reg, idx, ioffset, iNumberOfSPDBytes;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    for (reg = 0; reg < numreg; reg++)
    {
        DWORD Data = 0;

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_STATUS_REG_LIST[reg]);
        _stprintf_s(tmpMsg, _T("SMB_STATUS_CFG[%d]=%08X"), reg, Data);
        MtSupportUCDebugWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_CMD_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_CMD_CFG[%d]=%08X"), reg, Data);
        MtSupportUCDebugWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_DATA_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_DATA_CFG[%d]=%08X"), reg, Data);
        MtSupportUCDebugWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_TSOD_CONFIG_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_TSOD_CFG[%d]=%08X"), reg, Data);
        MtSupportUCDebugWriteLine(tmpMsg);

        for (idx = 0; idx <= 7; idx++)
        {
            if (smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0xA, 0, (BYTE)idx, FALSE, FALSE, &Data))
            {
                BOOL bUseByteAccess = TRUE;
                BYTE spdData[MAX_SPD_LENGTH];
                memset(spdData, 0, sizeof(spdData));

                _stprintf_s(tmpMsg, _T("Device detected on SMBUS on port %d, slot %d (IntelPCU_IL)"), reg, idx); // SI101005
                MtSupportUCDebugWriteLine(tmpMsg);

                DebugMessageBoxes(tmpMsg);

                iNumberOfSPDBytes = 256;
                // if (Data > 64)
                //	iNumberOfSPDBytes = Data;
                if (g_numMemModules >= MAX_MEMORY_SLOTS)
                    expand_spd_info_array();

                if (g_numMemModules < MAX_MEMORY_SLOTS)
                    g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                g_MemoryInfo[g_numMemModules].channel = -1;
                g_MemoryInfo[g_numMemModules].slot = -1;

                // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                //  and are not defined in the spec.

                MtSupportUCDebugWriteLine(_T("Setting bank address to 0"));
                smbSetBankAddrIntelPCU_IL(Bus, Dev, Fun, reg, 0);
                MtSupportUCDebugWriteLine(_T("Finished setting bank address to 0"));

                ioffset = 0;
                _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                MtSupportUCDebugWriteLine(tmpMsg);
                if (smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0xA, 0, (BYTE)idx, TRUE, FALSE, &Data)) // See if we can read word-by-word
                {
                    bUseByteAccess = FALSE;
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        int iErrCount = 0;

                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            MtSupportUCDebugWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        Data = 0;

                        while (!smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0xA, (BYTE)ioffset, (BYTE)idx, TRUE, FALSE, &Data) && iErrCount < 10)
                        {
                            iErrCount++;
                        }

                        if (iErrCount >= 10)
                        {
                            _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                            MtSupportUCDebugWriteLine(tmpMsg);

                            bUseByteAccess = TRUE;
                            break;
                        }

                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }
                }

                if (bUseByteAccess) // Read byte-by-byte
                {
                    MtSupportUCDebugWriteLine(_T("Word access failed, attempting byte access"));
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                    {
                        int iErrCount = 0;

                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            MtSupportUCDebugWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        Data = 0;

                        while (!smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0xA, (BYTE)ioffset, (BYTE)idx, FALSE, FALSE, &Data) && iErrCount < 10)
                        {
                            iErrCount++;
                        }

                        if (iErrCount >= 10)
                        {
                            _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                            MtSupportUCDebugWriteLine(tmpMsg);
                            break;
                        }

                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                    }
                }

                if (IsDDR4SPD(spdData, iNumberOfSPDBytes))
                {
                    MtSupportUCDebugWriteLine(_T("Setting bank address to 1"));
                    smbSetBankAddrIntelPCU_IL(Bus, Dev, Fun, reg, 1);
                    MtSupportUCDebugWriteLine(_T("Finished setting bank address to 1"));

                    iNumberOfSPDBytes = 512;

                    ioffset = 256;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    MtSupportUCDebugWriteLine(tmpMsg);
                    if (bUseByteAccess) // Read byte-by-byte
                    {
                        for (ioffset = 256; ioffset < 512; ioffset++)
                        {
                            int iErrCount = 0;

                            Data = 0;

                            while (!smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0xA, (BYTE)(ioffset & 0xFF), (BYTE)idx, FALSE, FALSE, &Data) && iErrCount < 10)
                            {
                                iErrCount++;
                            }

                            if (iErrCount >= 10)
                            {
                                _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                MtSupportUCDebugWriteLine(tmpMsg);
                                break;
                            }

                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                        }
                    }
                    else
                    {
                        for (ioffset = 256; ioffset < 512; ioffset += 2)
                        {
                            int iErrCount = 0;

                            Data = 0;

                            while (!smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0xA, (BYTE)(ioffset & 0xFF), (BYTE)idx, TRUE, FALSE, &Data) && iErrCount < 10)
                            {
                                iErrCount++;
                            }

                            if (iErrCount >= 10)
                            {
                                _stprintf_s(tmpMsg, _T("Unable to retrieve data at offset %d after %d tries"), ioffset, iErrCount); // SI101005
                                MtSupportUCDebugWriteLine(tmpMsg);
                                break;
                            }

                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                            spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                        }
                    }

                    MtSupportUCDebugWriteLine(_T("Setting bank address back to 0"));
                    smbSetBankAddrIntelPCU_IL(Bus, Dev, Fun, reg, 0);
                    MtSupportUCDebugWriteLine(_T("Finished setting bank address to 0"));
                }

                if (g_numMemModules < MAX_MEMORY_SLOTS)
                {
                    if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                    {
                        g_numMemModules++;
                        numMemSticks++;
                    }
                }
                else
                {
                    _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                    MtSupportUCDebugWriteLine(tmpMsg);
                }
            }
        }
    }

    return numMemSticks;
}

int smbGetTSODIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun)
{
    int numreg = sizeof(INTELPCU_IL_SMB_STATUS_REG_LIST) / sizeof(INTELPCU_IL_SMB_STATUS_REG_LIST[0]);
    int reg, idx, ioffset;
    int numTSOD = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    for (reg = 0; reg < numreg; reg++)
    {
        DWORD Data = 0;

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_STATUS_REG_LIST[reg]);
        _stprintf_s(tmpMsg, _T("SMB_STATUS_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_CMD_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_CMD_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_DATA_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_DATA_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        Data = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_TSOD_CONFIG_REG_LIST[reg]);

        _stprintf_s(tmpMsg, _T("SMB_TSOD_CFG[%d]=%08X"), reg, Data);
        SysInfo_DebugLogWriteLine(tmpMsg);

        for (idx = 0; idx <= 7; idx++)
        {
            if (smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0x3, 0, (BYTE)idx, FALSE, FALSE, &Data))
            {
                WORD tsodData[MAX_TSOD_LENGTH];
                memset(tsodData, 0, sizeof(tsodData));

                _stprintf_s(tmpMsg, _T("Device detected on SMBUS on port %d, slot %d (IntelPCU_IL)"), reg, idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);

                DebugMessageBoxes(tmpMsg);

                // if (Data > 64)
                //	iNumberOfSPDBytes = Data;
                if (g_numTSODModules >= MAX_TSOD_SLOTS)
                    expand_tsod_info_array();

                g_MemTSODInfo[g_numTSODModules].channel = -1;
                g_MemTSODInfo[g_numTSODModules].slot = -1;

                ioffset = 0;
                _stprintf_s(tmpMsg, _T("Retrieving TSOD words %d-%d"), ioffset, MAX_TSOD_LENGTH - 1);
                SysInfo_DebugLogWriteLine(tmpMsg);

                // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                //  and are not defined in the spec.
                for (ioffset = 0; ioffset < MAX_TSOD_LENGTH; ioffset++)
                {
                    if (smbCallBusIntelPCU_IL(Bus, Dev, Fun, reg, 0x3, 0, (BYTE)idx, TRUE, FALSE, &Data))
                        tsodData[ioffset] = SWAP_16(Data & 0xFFFF);
                }

                if (!CheckSPDBytes((BYTE *)tsodData, MAX_TSOD_LENGTH * sizeof(tsodData[0])))
                    continue;

                if (g_numTSODModules < MAX_TSOD_SLOTS)
                {
                    int temp;
#ifdef WIN32
                    float fTemp;
#endif
                    int i;

                    _stprintf_s(tmpMsg, _T("Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    tmpMsg[0] = L'\0';
                    for (i = 0; i < MAX_TSOD_LENGTH; i++)
                    {
                        TCHAR strSPDBytes[8];
                        _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                        _tcscat(tmpMsg, strSPDBytes);
                    }

                    SysInfo_DebugLogWriteLine(tmpMsg);

                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCap = tsodData[0];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCfg = tsodData[1];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wHiLim = tsodData[2];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wLoLim = tsodData[3];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCritLim = tsodData[4];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wAmbTemp = tsodData[5];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wManufID = tsodData[6];
                    g_MemTSODInfo[g_numTSODModules].raw.DDR4.wDevRev = tsodData[7];
                    memcpy(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor, tsodData + 8, sizeof(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor));

                    if (!CheckTSOD(&g_MemTSODInfo[g_numTSODModules]))
                    {
                        _stprintf_s(tmpMsg, _T("**WARNING** TSOD may be invalid. Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        tmpMsg[0] = L'\0';
                        for (i = 0; i < MAX_TSOD_LENGTH; i++)
                        {
                            TCHAR strSPDBytes[8];
                            _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                            _tcscat(tmpMsg, strSPDBytes);
                        }
                        MtSupportUCDebugWriteLine(tmpMsg);

                        if (!CheckCLTTPolling(tsodData))
                            continue;

                        SysInfo_DebugLogF(L"Found repeated TSOD words likely from CLTT. Continue parsing TSOD data");
                    }

                    // High limit
                    temp = tsodData[2] & 0xFFC;
                    if (BitExtract(tsodData[2], 12, 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d high limit: %.3fC"), g_numTSODModules, fTemp);
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // Low limit
                    temp = tsodData[3] & 0xFFC;
                    if (BitExtract(tsodData[3], 12, 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d low limit: %.3fC"), g_numTSODModules, fTemp);
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // TCRIT limit
                    temp = tsodData[4] & 0xFFC;
                    if (BitExtract(tsodData[4], 12, 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %.3fC"), g_numTSODModules, fTemp);
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // Current temperature
                    temp = tsodData[5] & 0xFFF;
                    if (tsodData[5] & (1 << 12)) // two's complement
                        temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                    fTemp = (float)temp * 0.0625f;
                    _stprintf_s(tmpMsg, _T("DIMM%d temperature: %.3fC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, fTemp, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));

                    if (abs((int)fTemp) < MAX_TSOD_TEMP)
                    {
                        g_MemTSODInfo[g_numTSODModules].temperature = fTemp;
                        g_numTSODModules++;
                        numTSOD++;
                    }
#else
                    temp = temp * 625 / 10;
                    _stprintf_s(tmpMsg, _T("DIMM%d temperature: %d.%03dC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, temp / 1000, temp % 1000, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));

                    if (ABS(temp) < MAX_TSOD_TEMP)
                    {
                        g_MemTSODInfo[g_numTSODModules].temperature = temp;
                        g_numTSODModules++;
                        numTSOD++;
                    }
#endif
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }
                else
                {
                    _stprintf_s(tmpMsg, _T("Maximum number of TSOD modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);
                }
            }
        }
    }

    return numTSOD;
}

DISABLE_OPTIMISATIONS() // Optimisation must be turned off
BOOL smbCallBusIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bDTI, BYTE bOffset, BYTE bSlot, BOOL bWord, BOOL bTSOD, DWORD *Data)
{
    DWORD Result = 0;
    DWORD Command = 0;
    const int NUM_RETRIES = 5;
    int i;

    for (i = 0; i < NUM_RETRIES; i++)
    {
        if (smbWaitForFreeIntelPCU_IL(Bus, Dev, Fun, SMBPort) == FALSE)
            return FALSE;

        // Make sure DTI is set to 0xA and TSOD polling is disabled in the SMBCNTL register
        Command = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_CMD_REG_LIST[SMBPort]);

        Command &= 0xFFE00000;
        Command |= INTELPCU_IL_SMB_BA(bOffset);
        Command |= INTELPCU_IL_SMB_SA(bSlot);
        Command |= INTELPCU_IL_SMB_DTI(bDTI);
        Command |= INTELPCU_IL_SMB_WRT(0);
        if (bWord)
            Command |= INTELPCU_IL_SMB_WORD_ACCESS;
        if (bTSOD)
            Command |= INTELPCU_IL_SMB_TSOD_POLL_EN;
        Set_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_CMD_REG_LIST[SMBPort], Command | INTELPCU_IL_SMB_CMD_TRIGGER);

        Sleep(5);
        if (smbWaitForEndIntelPCU_IL(Bus, Dev, Fun, SMBPort))
        {

            Result = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_DATA_REG_LIST[SMBPort]);

            Result = INTELPCU_IL_SMB_RDATA(Result);

            if (bWord)
                *Data = ((Result << 8) & 0x0000FF00) | ((Result >> 8) & 0x000000FF);
            else
                *Data = Result & 0x000000FF;

            return TRUE;
        }
        Sleep(5);
    }
    return FALSE;
}

BOOL smbSetBankAddrIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bBankNo)
{
    DWORD Command = 0;
    const int NUM_RETRIES = 5;
    int i;

    for (i = 0; i < NUM_RETRIES; i++)
    {
        if (smbWaitForFreeIntelPCU_IL(Bus, Dev, Fun, SMBPort) == FALSE)
            return FALSE;

        // Make sure DTI is set to 0x6 and TSOD polling is disabled in the SMBCNTL register
        Command = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_CMD_REG_LIST[SMBPort]);
        Command &= 0xFFE00000;
        Command |= INTELPCU_IL_SMB_DTI(0x6);
        Command |= INTELPCU_IL_SMB_WRT(0x1);

        if (bBankNo)
            Command |= INTELPCU_IL_SMB_SA(0x7);
        else
            Command |= INTELPCU_IL_SMB_SA(0x6);

        Set_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_CMD_REG_LIST[SMBPort], Command | INTELPCU_IL_SMB_CMD_TRIGGER);

        Sleep(5);
        if (smbWaitForEndIntelPCU_IL(Bus, Dev, Fun, SMBPort))
            return TRUE;
        Sleep(5);
    }
    return FALSE;
}

BOOL smbWaitForFreeIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_STATUS_REG_LIST[SMBPort]);

    while (((Status & INTELPCU_IL_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELPCU_IL_MAX_TIMEOUT))
    {
        Sleep(1);

        Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_STATUS_REG_LIST[SMBPort]);
    }

    if (CHECK_TIMEOUT(timeout, INTELPCU_IL_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_STATUS_REG_LIST[SMBPort]);

    while (((Status & INTELPCU_IL_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTELPCU_IL_MAX_TIMEOUT))
    {
        Sleep(1);

        Status = Get_PCI_Reg(Bus, Dev, Fun, INTELPCU_IL_SMB_STATUS_REG_LIST[SMBPort]);
    }

    if (CHECK_TIMEOUT(timeout, INTELPCU_IL_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntelPCU_IL: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTELPCU_IL_SMB_SBE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntelPCU_IL: Device Error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}
ENABLE_OPTIMISATIONS()

/* Intel5000 SMBus address offsets */
#define INTEL5000_SMB_STAT 0x74
#define INTEL5000_SMBCMD_0 0x78
#define INTEL5000_SMBCMD_1 0x7C
#define INTEL5000_SMB_STAT_0(x) (x & 0x0000FFFF)
#define INTEL5000_SMB_STAT_1(x) ((x >> 16) & 0x0000FFFF)

/* I5000 Hosts Status register bits */
#define INTEL5000_SMB_RDO 0x00008000
#define INTEL5000_SMB_WOD 0x00004000
#define INTEL5000_SMB_SBE 0x00002000
#define INTEL5000_SMB_BUSY 0x00001000
#define INTEL5000_SMB_RDATA(x) (x & 0x000000FF)

#define INTEL5000_SMB_DTI(x) ((x & 0x0F) << 28)
#define INTEL5000_SMB_CKOVRD 0x08000000
#define INTEL5000_SMB_SA(x) ((x & 0x07) << 24)
#define INTEL5000_SMB_BA(x) ((x & 0xFF) << 16)
#define INTEL5000_SMB_WDATA(x) (x & 0xFFFF)

#define INTEL5000_MAX_RETRY 5
#define INTEL5000_MAX_TIMEOUT 250

int smbGetSPDIntel5000_PCI()
{
    DWORD Data;
    int br, ch, sa, ioffset;
    int iNumberOfSPDBytes, ret;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    for (br = 0; br <= 1; br++)
    {
        for (ch = 0; ch <= 1; ch++)
        {
            for (sa = 0; sa <= 1; sa++)
            {
                if (smbCallBusIntel5000_PCI(0, (BYTE)br, (BYTE)ch, (BYTE)sa, FALSE, &Data))
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    _stprintf_s(tmpMsg, _T("Device detected on SMBUS at branch %d, slot %d, slave addr %d (smbCallBusIntel5000_PCI)"), br, ch, sa); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data;

                    // g_MemoryInfo[numMemSticks].dimmNum = (br * 2 * 2) + (ch * 2) + sa;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }
                        ret = smbCallBusIntel5000_PCI((BYTE)ioffset, (BYTE)br, (BYTE)ch, (BYTE)sa, FALSE, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }

    return numMemSticks;
}

BOOL smbCallBusIntel5000_PCI(BYTE bOffset, BYTE bBranch, BYTE bChannel, BYTE bSA, BOOL bWord, DWORD *Data)
{
    DWORD Result = 0;
    DWORD Command = 0;
    int retry = 0;

    while (retry++ < INTEL5000_MAX_RETRY)
    {
        if (smbWaitForFreeIntel5000_PCI(bBranch, bChannel) == FALSE)
        {
            _stprintf_s(g_szSITempDebug1, _T("smbCallBusIntel5000_PCI: smbWaitForFreeIntel5000_PCI failed"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
            return FALSE;
        }

        // Command = Get_PCI_Reg(0, 0x15 + bBranch, 0, bChannel ? INTEL5000_SMBCMD_1 : INTEL5000_SMBCMD_0);
        Command = 0xA8000000 | (INTEL5000_SMB_BA(bOffset) | INTEL5000_SMB_SA(bSA));

        // Command &= 0xF8000000;
        // Command |= (INTEL5000_SMB_BA(bOffset) | INTEL5000_SMB_SA(bSA));

#if 0
        if (bWord)
            Command |= INTEL5000_SMB_WORD_ACCESS;
#endif

        Set_PCI_Reg(0, 0x15 + bBranch, 0, bChannel ? INTEL5000_SMBCMD_1 : INTEL5000_SMBCMD_0, Command);

        // Sleep(1);
        if (smbWaitForEndIntel5000_PCI(bBranch, bChannel) == TRUE)
            break;
    }

    if (retry >= INTEL5000_MAX_RETRY)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbCallBusIntel5000_PCI: maximum retry exceeded"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (bChannel)
        Result = INTEL5000_SMB_RDATA(INTEL5000_SMB_STAT_1(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT)));
    else
        Result = INTEL5000_SMB_RDATA(INTEL5000_SMB_STAT_0(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT)));

    *Data = Result;

    return TRUE;
}

BOOL smbWaitForFreeIntel5000_PCI(BYTE bBranch, BYTE bChannel)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    if (bChannel)
        Status = INTEL5000_SMB_STAT_1(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));
    else
        Status = INTEL5000_SMB_STAT_0(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));

    while (((Status & INTEL5000_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTEL5000_MAX_TIMEOUT))
    {
        // Sleep(1);
        if (bChannel)
            Status = INTEL5000_SMB_STAT_1(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));
        else
            Status = INTEL5000_SMB_STAT_0(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));
    }

    if (CHECK_TIMEOUT(timeout, INTEL5000_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndIntel5000_PCI(BYTE bBranch, BYTE bChannel)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    if (bChannel)
        Status = INTEL5000_SMB_STAT_1(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));
    else
        Status = INTEL5000_SMB_STAT_0(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));

    while (((Status & INTEL5000_SMB_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTEL5000_MAX_TIMEOUT))
    {
        // Sleep(1);
        if (bChannel)
            Status = INTEL5000_SMB_STAT_1(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));
        else
            Status = INTEL5000_SMB_STAT_0(Get_PCI_Reg(0, 0x15 + bBranch, 0, INTEL5000_SMB_STAT));
    }

    if (CHECK_TIMEOUT(timeout, INTEL5000_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel5000_PCI: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTEL5000_SMB_SBE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel5000_PCI: Bus Error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

/* Intel5100 SMBus address offsets */
#define INTEL5100_SPDDATA 0x48
#define INTEL5100_SPDCMD 0x4C

/* I5000 SPDDATA register bits */
#define INTEL5100_SPDDATA_RDO 0x00008000
#define INTEL5100_SPDDATA_WOD 0x00004000
#define INTEL5100_SPDDATA_SBE 0x00002000
#define INTEL5100_SPDDATA_BUSY 0x00001000
#define INTEL5100_SPDDATA_RDATA(x) (x & 0x000000FF)

#define INTEL5100_SPDCMD_DTI(x) ((x & 0x0F) << 28)
#define INTEL5100_SPDCMD_CKOVRD 0x08000000
#define INTEL5100_SPDCMD_SA(x) ((x & 0x07) << 24)
#define INTEL5100_SPDCMD_BA(x) ((x & 0xFF) << 16)
#define INTEL5100_SPDCMD_WDATA(x) ((x & 0xFF) << 8)
#define INTEL5100_SPDCMD_CMD(write) (write ? 0x00000001 : 0x00000000)

#define INTEL5100_MAX_RETRY 5
#define INTEL5100_MAX_TIMEOUT 250

int smbGetSPDIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
    DWORD Data;
    int sa, ioffset;
    int iNumberOfSPDBytes, ret;
    int numMemSticks = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    for (sa = 0; sa <= 7; sa++)
    {
        if (smbCallBusIntel5100_PCI(Bus, Dev, Fun, Reg, 0, (BYTE)sa, &Data))
        {
            BYTE spdData[MAX_SPD_LENGTH];
            memset(spdData, 0, sizeof(spdData));

            _stprintf_s(tmpMsg, _T("Device detected on SMBUS at slave addr %d (smbCallBusIntel5100_PCI)"), sa); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            iNumberOfSPDBytes = 256;
            // if (Data > 64)
            //	iNumberOfSPDBytes = Data;

            // g_MemoryInfo[numMemSticks].dimmNum = (br * 2 * 2) + (ch * 2) + sa;
            if (g_numMemModules >= MAX_MEMORY_SLOTS)
                expand_spd_info_array();

            if (g_numMemModules < MAX_MEMORY_SLOTS)
                g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
            g_MemoryInfo[g_numMemModules].channel = -1;
            g_MemoryInfo[g_numMemModules].slot = -1;

            ioffset = 0;
            _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
            SysInfo_DebugLogWriteLine(tmpMsg);

            // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
            //  and are not defined in the spec.

            for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
            {
                if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                {
                    SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                    break;
                }
                ret = smbCallBusIntel5100_PCI(Bus, Dev, Fun, Reg, (BYTE)ioffset, (BYTE)sa, &Data);
                spdData[ioffset] = (BYTE)(Data & 0xFF);
            }

            if (g_numMemModules < MAX_MEMORY_SLOTS)
            {
                if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                {
                    g_numMemModules++;
                    numMemSticks++;
                }
            }
            else
            {
                _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
        }
    }

    return numMemSticks;
}

BOOL smbCallBusIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, BYTE bOffset, BYTE bSlot, DWORD *Data)
{
    DWORD Result = 0;
    DWORD Command = 0;
    int retry = 0;

    while (retry++ < INTEL5100_MAX_RETRY)
    {
        if (smbWaitForFreeIntel5100_PCI(Bus, Dev, Fun, Reg) == FALSE)
        {
            _stprintf_s(g_szSITempDebug1, _T("smbCallBusIntel5100_PCI: smbWaitForFreeIntel5100_PCI failed"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
            return FALSE;
        }

        // Command = Get_PCI_Reg(0, 0x15 + bBranch, 0, bChannel ? INTEL5100_SMBCMD_1 : INTEL5100_SMBCMD_0);
        Command = 0xA8000000 | (INTEL5100_SPDCMD_BA(bOffset) | INTEL5100_SPDCMD_SA(bSlot));

        // Command &= 0xF8000000;
        // Command |= (INTEL5100_SMB_BA(bOffset) | INTEL5100_SMB_SA(bSA));

#if 0
        if (bWord)
            Command |= INTEL5100_SMB_WORD_ACCESS;
#endif

        Set_PCI_Reg(Bus, Dev, Fun, INTEL5100_SPDCMD, Command);

        // Sleep(1);
        if (smbWaitForEndIntel5100_PCI(Bus, Dev, Fun, Reg) == TRUE)
            break;
    }

    if (retry >= INTEL5100_MAX_RETRY)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbCallBusIntel5100_PCI: maximum retry exceeded"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    Result = INTEL5100_SPDDATA_RDATA(Get_PCI_Reg(Bus, Dev, Fun, INTEL5100_SPDDATA));

    *Data = Result;

    return TRUE;
}

BOOL smbWaitForFreeIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = Get_PCI_Reg(Bus, Dev, Fun, INTEL5100_SPDDATA) & 0xFFFF;

    while (((Status & INTEL5100_SPDDATA_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTEL5100_MAX_TIMEOUT))
    {
        // Sleep(1);
        Status = Get_PCI_Reg(Bus, Dev, Fun, INTEL5100_SPDDATA) & 0xFFFF;
    }

    if (CHECK_TIMEOUT(timeout, INTEL5100_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
    DWORD Status = 0;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    Status = Get_PCI_Reg(Bus, Dev, Fun, INTEL5100_SPDDATA) & 0xFFFF;

    while (((Status & INTEL5100_SPDDATA_BUSY) != 0) && !CHECK_TIMEOUT(timeout, INTEL5100_MAX_TIMEOUT))
    {
        // Sleep(1);
        Status = Get_PCI_Reg(Bus, Dev, Fun, INTEL5100_SPDDATA) & 0xFFFF;
    }

    if (CHECK_TIMEOUT(timeout, INTEL5100_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel5100_PCI: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & INTEL5100_SPDDATA_SBE)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndIntel5100_PCI: Bus Error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

// nVidia nForce4 definitions
/*
 * ACPI 2.0 chapter 13 SMBus 2.0 EC register model
 */
// IO address mapped offsets from SMBASE
#define NVIDIA_SMB_PRTCL 0x00 /* protocol, PEC */
#define NVIDIA_SMB_STS 0x01   /* status */
#define NVIDIA_SMB_ADDR 0x02  /* address */
#define NVIDIA_SMB_CMD 0x03   /* command */
#define NVIDIA_SMB_DATA 0x04  /* 32 data registers */
#define NVIDIA_SMB_DATA2 0x05
#define NVIDIA_SMB_BCNT 0x24   /* number of data bytes */
#define NVIDIA_SMB_ALRM_A 0x25 /* alarm address */
#define NVIDIA_SMB_ALRM_D 0x26 /* 2 bytes alarm data */

#define NVIDIA_SMB_STS_DONE 0x80
#define NVIDIA_SMB_STS_ALRM 0x40
#define NVIDIA_SMB_STS_RES 0x20
#define NVIDIA_SMB_STS_STATUS 0x1f

#define NVIDIA_SMB_PRTCL_WRITE 0x00
#define NVIDIA_SMB_PRTCL_READ 0x01
#define NVIDIA_SMB_PRTCL_QUICK 0x02
#define NVIDIA_SMB_PRTCL_BYTE 0x04
#define NVIDIA_SMB_PRTCL_BYTE_DATA 0x06
#define NVIDIA_SMB_PRTCL_WORD_DATA 0x08
#define NVIDIA_SMB_PRTCL_BLOCK_DATA 0x0a
#define NVIDIA_SMB_PRTCL_PROC_CALL 0x0c
#define NVIDIA_SMB_PRTCL_BLOCK_PROC_CALL 0x0d
#define NVIDIA_SMB_PRTCL_I2C_BLOCK_DATA 0x4a
#define NVIDIA_SMB_PRTCL_PEC 0x80

/* Other settings */
#define NVIDIA_MAX_TIMEOUT 250

int smbGetSPDNvidia(WORD BaseAddr)
{
    int numMemSticks = 0;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    // for (idx = 0x20; idx <=  0x68; idx++)
    // only scan the sub set of the memory chip locations
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            ret = SetPortVal(BaseAddr + NVIDIA_SMB_DATA, 0, 1);  // Clear the first data byte - Host data 1
            ret = SetPortVal(BaseAddr + NVIDIA_SMB_DATA2, 0, 1); // Clear the first data byte - Host data 2

            if (smbCallBusNvidia(BaseAddr, 0, (BYTE)idx, RW_READ, NVIDIA_SMB_PRTCL_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Nvidia)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.
                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        ret = SetPortVal(BaseAddr + NVIDIA_SMB_DATA, 0, 1);
                        ret = SetPortVal(BaseAddr + NVIDIA_SMB_DATA2, 0, 1);
                        ret = smbCallBusNvidia(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, NVIDIA_SMB_PRTCL_WORD_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

BOOL smbCallBusNvidia(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Result = 0, Dump1;
    int ret;

    ret = SetPortVal(BaseAddr + NVIDIA_SMB_CMD, CMD, 1);                    // Host command nVidia nForce4
    ret = SetPortVal(BaseAddr + NVIDIA_SMB_ADDR, (Slave << 1) /*| RW*/, 1); // Transmit slave address nForce4. Note: no "| RW"in Linux
    ret = SetPortVal(BaseAddr + NVIDIA_SMB_PRTCL, NVIDIA_SMB_PRTCL_READ | Prot, 1);
    // Sleep(1);
    if (smbWaitForEndNvidia(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + NVIDIA_SMB_DATA2, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Dump1 << 8;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + NVIDIA_SMB_DATA, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Result | Dump1;

    *Data = Result;
    return TRUE;
}

BOOL smbWaitForEndNvidia(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + NVIDIA_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while (!(Status & NVIDIA_SMB_STS_DONE) && !CHECK_TIMEOUT(timeout, NVIDIA_MAX_TIMEOUT))
    {
        if (Status & NVIDIA_SMB_STS_STATUS)
            return FALSE;

        // Sleep(1);
        ret = GetPortVal(BaseAddr + NVIDIA_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, NVIDIA_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

// IO address mapped offsets from SMBASE
#define PIIX4_SMB_STS 0x00
#define PIIX4_SMB_CTRL 0x02
#define PIIX4_SMB_CMD 0x03
#define PIIX4_SMB_ADDR 0x04
#define PIIX4_SMB_DATA 0x05
#define PIIX4_SMB_DATA2 0x06
#define PIIX4_SMB_BLOCK_DATA 0x07

/* PIIX4 constants */
#define PIIX4_SMB_PRTCL_QUICK 0x00
#define PIIX4_SMB_PRTCL_BYTE 0x04
#define PIIX4_SMB_PRTCL_BYTE_DATA 0x08
#define PIIX4_SMB_PRTCL_WORD_DATA 0x0C
#define PIIX4_SMB_PRTCL_PROC_CALL 0x10
#define PIIX4_SMB_PRTCL_BLOCK_DATA 0x14
#define PIIX4_SMB_PRTCL_START 0x40
/* PIIX4 Hosts Status register bits */

#define PIIX4_SMBHSTSTS_FAILED 0x10
#define PIIX4_SMBHSTSTS_BUS_ERR 0x08
#define PIIX4_SMBHSTSTS_DEV_ERR 0x04
#define PIIX4_SMBHSTSTS_INTR 0x02
#define PIIX4_SMBHSTSTS_HOST_BUSY 0x01

/* Other settings */
#define PIIX4_MAX_TIMEOUT 250

#define AMD_ACPI_FCH_PM_DECODEEN 0xFED80300

int smbGetSPDPIIX4(DWORD Bus, DWORD Dev, DWORD Fun, WORD BaseAddr, BYTE byRev)
{
    int numMemSticks = 0;
    int iNumSMBus = 1;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

#if 0
    if (byRev >= 0x49)
        iNumSMBus = 4;
#endif
    for (int smbus0sel = 0; smbus0sel < iNumSMBus; smbus0sel++)
    {
#if 0
        DWORD PrevDecodeEn = 0;
        if (byRev >= 0x49)
        {
#ifdef WIN32
            // Ask driver to map MMIO base address to pointer we can use
            BYTE* pbLinAddr = NULL;
            tagPhys32Struct Phys32Struct = { 0 };
            // HANDLE		hPhysicalMemory	= NULL;
            pbLinAddr = MapPhysToLin((PBYTE)(UINT_PTR)AMD_ACPI_FCH_PM_DECODEEN, 4, &Phys32Struct, TRUE);
            if (pbLinAddr)
            {
                PrevDecodeEn = *((DWORD*)pbLinAddr);
#else
            PrevDecodeEn = MmioRead32(AMD_ACPI_FCH_PM_DECODEEN);
#endif
            _stprintf_s(tmpMsg, _T("Previous MMIO PM DECODEEN: %08X (smbus0sel=%d)"), PrevDecodeEn, BitExtract(PrevDecodeEn, 20, 19));
            MtSupportUCDebugWriteLine(tmpMsg);

            if ((int)BitExtract(PrevDecodeEn, 20, 19) != smbus0sel)
            {
                DWORD NewDecodeEn = PrevDecodeEn;
                NewDecodeEn &= ~(0x3 << 19);
                NewDecodeEn |= (smbus0sel & 0x3) << 19;

                _stprintf_s(tmpMsg, _T("Writing MMIO PM DECODEEN: %08X (smbus0sel=%d)"), NewDecodeEn, BitExtract(NewDecodeEn, 20, 19));
                MtSupportUCDebugWriteLine(tmpMsg);

#ifdef WIN32
                *((DWORD*)pbLinAddr) = NewDecodeEn;
#else
                MmioWrite32(AMD_ACPI_FCH_PM_DECODEEN, NewDecodeEn);
#endif
            }
#ifdef WIN32
            UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
            }
#endif
        }
#endif
        // for (idx = 0x0; idx <=  0x7F; idx++)
        // for (idx = 0x20; idx <=  0x68; idx++)
        // only scan the sub set of the memory chip locations
        for (idx = 0x50; idx <= 0x57; idx++)
        {
            if (idx != 0x69) // Don't probe the Clock chip
            {
                if (smbCallBusPIIX4(BaseAddr, 0, (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data))
                {
                    _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (PIIX4)"), idx); // SI101005
                    MtSupportUCDebugWriteLine(tmpMsg);

                    // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                    if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                    {
                        BYTE spdData[MAX_SPD_LENGTH];
                        memset(spdData, 0, sizeof(spdData));

                        // BYTE spdData[512] = { 0x92, 0x10, 0x0B, 0x02, 0x03, 0x19, 0x00, 0x09, 0x03, 0x52, 0x01, 0x08, 0x0F, 0x00, 0x1C, 0x00, 0x69, 0x78, 0x69, 0x3C, 0x69, 0x11, 0x2C, 0x95, 0x00, 0x05, 0x3C, 0x3C, 0x01, 0x2C, 0x83, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x11, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB, 0xF3, 0x46, 0x33, 0x2D, 0x38, 0x35, 0x30, 0x30, 0x43, 0x4C, 0x37, 0x2D, 0x34, 0x47, 0x42, 0x52, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x04, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                        // BYTE spdData[512] = { 0x92, 0x11, 0x0B, 0x02, 0x04, 0x21, 0x00, 0x09, 0x03, 0x11, 0x01, 0x08, 0x0C, 0x00, 0x7E, 0x00, 0x69, 0x78, 0x69, 0x30, 0x69, 0x11, 0x20, 0x89, 0x20, 0x08, 0x3C, 0x3C, 0x00, 0xF0, 0x83, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x11, 0x21, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA9, 0xE1, 0x46, 0x33, 0x2D, 0x31, 0x39, 0x32, 0x30, 0x30, 0x43, 0x31, 0x30, 0x2D, 0x38, 0x47, 0x42, 0x5A, 0x48, 0x44, 0x00, 0x00, 0x04, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x4A, 0x3F, 0x13, 0x01, 0x08, 0x0A, 0x60, 0x11, 0x2D, 0x07, 0x3F, 0x40, 0x00, 0x2E, 0x4C, 0x4C, 0x78, 0x10, 0xCB, 0x1B, 0x3F, 0x00, 0x20, 0x08, 0x3C, 0x30, 0x00, 0xB8, 0x3C, 0x00, 0x00, 0x10, 0x00, 0x25, 0xD6, 0xAF, 0x87, 0x87, 0xBB, 0x00, 0x00, 0x00, 0x00, 0x2D, 0x08, 0x4D, 0x40, 0x00, 0x38, 0x5D, 0x5D, 0x90, 0x10, 0xF5, 0x55, 0x4B, 0x00, 0xC4, 0x09, 0x48, 0x39, 0x00, 0xDD, 0x48, 0x00, 0x00, 0x10, 0x00, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                        // BYTE spdData[512] = { 0x92, 0x11, 0x09, 0x12, 0x48, 0x23, 0x07, 0x10, 0x00, 0x01, 0x04, 0x0C, 0x20, 0x33, 0x3C, 0x42, 0x3C, 0x72, 0x50, 0x3C, 0x1E, 0x3C, 0x00, 0xB4, 0xF0, 0xA4, 0x01, 0x1E, 0x1E, 0x03, 0x07, 0x01, 0xC2, 0x50, 0x7A, 0x48, 0x2E, 0x36, 0x27, 0x4C, 0x20, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x02, 0x00, 0x10, 0x54, 0x50, 0x44, 0x26, 0x3F, 0x50, 0x54, 0x57, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0xCA, 0x00, 0xD5, 0x60, 0x08, 0x02, 0x00, 0x00, 0x4C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0x51, 0x85, 0x51, 0x45, 0x07, 0x37, 0x03, 0x01, 0xB7, 0x14, 0x6F, 0x24, 0x37, 0x32, 0x54, 0x32, 0x35, 0x36, 0x39, 0x32, 0x30, 0x48, 0x46, 0x41, 0x33, 0x53, 0x42, 0x20, 0x20, 0x20, 0x04, 0x9D, 0x85, 0x51, 0x43, 0x10, 0x44, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
                        // BYTE spdData[512] = { 0x80, 0x08, 0x08, 0x0E, 0x0A, 0x61, 0x40, 0x00, 0x05, 0x25, 0x40, 0x00, 0x82, 0x08, 0x00, 0x00, 0x0C, 0x08, 0x70, 0x01, 0x04, 0x00, 0x03, 0x30, 0x45, 0x3D, 0x50, 0x3C, 0x1E, 0x3C, 0x2D, 0x01, 0x17, 0x25, 0x05, 0x12, 0x3C, 0x1E, 0x1E, 0x00, 0x06, 0x3C, 0x7F, 0x80, 0x14, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0xDF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x55, 0x33, 0x33, 0x32, 0x47, 0x30, 0x41, 0x4C, 0x45, 0x50, 0x52, 0x38, 0x48, 0x32, 0x4C, 0x36, 0x43, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                        // BYTE spdData[512] = { 0x80, 0x08, 0x07, 0x0D, 0x0B, 0x02, 0x40, 0x00, 0x04, 0x50, 0x70, 0x00, 0x82, 0x08, 0x00, 0x01, 0x0E, 0x04, 0x18, 0x01, 0x02, 0x20, 0xC0, 0x60, 0x70, 0x00, 0x00, 0x48, 0x28, 0x48, 0x28, 0x80, 0x60, 0x60, 0x40, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x46, 0x20, 0x28, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCA, 0x7F, 0x7F, 0x9E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x56, 0x53, 0x31, 0x47, 0x42, 0x34, 0x30, 0x30, 0x43, 0x33, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#if 0
                    BYTE spdData[512] = { 0x23, 0x07, 0x0C, 0x01, 0x84, 0x19, 0x00, 0x08, 0x00, 0x00, 0x00, 0x03, 0x01, 0x0B, 0x80, 0x00,
                        0x00, 0x00, 0x08, 0x0C, 0xF4, 0x03, 0x00, 0x00, 0x6C, 0x6C, 0x6C, 0x11, 0x08, 0x74, 0x20, 0x08,
                        0x00, 0x05, 0x70, 0x03, 0x00, 0xA8, 0x1E, 0x2B, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0E, 0x2E, 0x16, 0x36,
                        0x16, 0x36, 0x16, 0x36, 0x0E, 0x2E, 0x23, 0x04, 0x2B, 0x0C, 0x2B, 0x0C, 0x23, 0x04, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0xB5, 0xCE, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0xEB, 0x3C,
                        0x11, 0x11, 0x03, 0x05, 0x00, 0x86, 0x32, 0xB1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF, 0x9E,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x80, 0xAD, 0x01, 0x14, 0x05, 0x3C, 0x80, 0x65, 0x7E, 0x48, 0x4D, 0x41, 0x34, 0x35, 0x31, 0x52,
                        0x37, 0x4D, 0x46, 0x52, 0x38, 0x4E, 0x2D, 0x54, 0x46, 0x54, 0x31, 0x20, 0x20, 0x00, 0x80, 0xAD,
                        0x03, 0x54, 0x49, 0x34, 0x31, 0x51, 0x32, 0x35, 0x34, 0x37, 0x32, 0x30, 0x31, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x0C, 0x4A, 0x0D, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00,
                        0x00, 0x5A, 0x5A, 0x5A, 0x10, 0xD2, 0x2C, 0x20, 0x08, 0x00, 0x05, 0x70, 0x03, 0x00, 0xA8, 0x1E,
                        0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; ioffset = 0;
#endif
                        iNumberOfSPDBytes = 256;
                        // if (Data > 64)
                        //	iNumberOfSPDBytes = Data;
                        if (g_numMemModules >= MAX_MEMORY_SLOTS)
                            expand_spd_info_array();

                        if (g_numMemModules < MAX_MEMORY_SLOTS)
                            g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                        g_MemoryInfo[g_numMemModules].channel = -1;
                        g_MemoryInfo[g_numMemModules].slot = -1;

                        SysInfo_DebugLogWriteLine(_T("A - Setting bank address to 0"));
                        smbSetBankAddrPIIX4(BaseAddr, 0);
                        SysInfo_DebugLogWriteLine(_T("A - Finished setting bank address to 0"));

                        ioffset = 0;
                        _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                        SysInfo_DebugLogWriteLine(tmpMsg);

                        // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                        //  and are not defined in the spec.
                        for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                        {
                            if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                            {
                                MtSupportUCDebugWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                                break;
                            }
                            ret = smbCallBusPIIX4(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_WORD_DATA, &Data);

                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                            spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                        }

                        if (IsDDR4SPD(spdData, iNumberOfSPDBytes))
                        {
                            SysInfo_DebugLogWriteLine(_T("B - Setting bank address to 1"));
                            smbSetBankAddrPIIX4(BaseAddr, 1);
                            SysInfo_DebugLogWriteLine(_T("B - Finished setting bank address to 1"));

                            iNumberOfSPDBytes = 512;

                            ioffset = 256;
                            _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                            SysInfo_DebugLogWriteLine(tmpMsg);
                            for (ioffset = 256; ioffset < 512; ioffset += 2)
                            {
                                ret = smbCallBusIntel801(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data);
                                spdData[ioffset] = (BYTE)(Data & 0xFF);
                                spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                            }

                            SysInfo_DebugLogWriteLine(_T("C - Setting bank address back to 0"));
                            smbSetBankAddrPIIX4(BaseAddr, 0);
                            SysInfo_DebugLogWriteLine(_T("C - Finished setting bank address to 0"));
                        }

                        if (g_numMemModules < MAX_MEMORY_SLOTS)
                        {
                            if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                            {
                                g_numMemModules++;
                                numMemSticks++;
                            }
                        }
                        else
                        {
                            _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                            SysInfo_DebugLogWriteLine(tmpMsg);
                        }
                    }
                }
            }
        }
#if 0
    if (byRev >= 0x49)
    {
        DWORD DecodeEn;
#ifdef WIN32
        // Ask driver to map MMIO base address to pointer we can use
        BYTE* pbLinAddr = NULL;
        tagPhys32Struct Phys32Struct = { 0 };
        // HANDLE		hPhysicalMemory	= NULL;
        pbLinAddr = MapPhysToLin((PBYTE)(UINT_PTR)AMD_ACPI_FCH_PM_DECODEEN, 4, &Phys32Struct, TRUE);
        if (pbLinAddr)
        {
            DecodeEn = *((DWORD*)pbLinAddr);
#else
        DecodeEn = MmioRead32(AMD_ACPI_FCH_PM_DECODEEN);
#endif
        if (DecodeEn != PrevDecodeEn)
        {
            _stprintf_s(tmpMsg, _T("Restoring MMIO PM DECODEEN: %08X (smbus0sel=%d)"), PrevDecodeEn, BitExtract(PrevDecodeEn, 20, 19));
            MtSupportUCDebugWriteLine(tmpMsg);

#ifdef WIN32
            * ((DWORD*)pbLinAddr) = PrevDecodeEn;
#else
            MmioWrite32(AMD_ACPI_FCH_PM_DECODEEN, PrevDecodeEn);
#endif
        }
#ifdef WIN32
        UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
        }
#endif
    }
#endif
    }
    return numMemSticks;
}

int smbGetTSODPIIX4(WORD BaseAddr)
{
    int numTSOD = 0;
    DWORD Data;
    int idx, ioffset;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    // for (idx = 0x20; idx <=  0x68; idx++)
    // only scan the sub set of the memory chip locations
    for (idx = 0x18; idx <= 0x1F; idx++)
    {
        if (smbCallBusPIIX4(BaseAddr, 0, (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_WORD_DATA, &Data))
        {
            WORD tsodData[MAX_TSOD_LENGTH];
            memset(tsodData, 0, sizeof(tsodData));

            _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (PIIX4)"), idx); // SI101005
            SysInfo_DebugLogWriteLine(tmpMsg);

            // if (Data > 64)
            //	iNumberOfSPDBytes = Data;
            if (g_numTSODModules >= MAX_TSOD_SLOTS)
                expand_tsod_info_array();

            g_MemTSODInfo[g_numTSODModules].channel = -1;
            g_MemTSODInfo[g_numTSODModules].slot = -1;

            ioffset = 0;
            _stprintf_s(tmpMsg, _T("Retrieving TSOD words %d-%d"), ioffset, MAX_TSOD_LENGTH - 1);
            SysInfo_DebugLogWriteLine(tmpMsg);

            // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
            //  and are not defined in the spec.
            for (ioffset = 0; ioffset < MAX_TSOD_LENGTH; ioffset++)
            {
                if (smbCallBusPIIX4(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, INTEL801_SMB_CMD_WORD_DATA, &Data))
                    tsodData[ioffset] = SWAP_16(Data & 0xFFFF);
            }

            if (!CheckSPDBytes((BYTE *)tsodData, MAX_TSOD_LENGTH * sizeof(tsodData[0])))
                continue;

            if (g_numTSODModules < MAX_TSOD_SLOTS)
            {
                int temp;
#ifdef WIN32
                float fTemp;
#endif
                int i;

                _stprintf_s(tmpMsg, _T("Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                SysInfo_DebugLogWriteLine(tmpMsg);

                tmpMsg[0] = L'\0';
                for (i = 0; i < MAX_TSOD_LENGTH; i++)
                {
                    TCHAR strSPDBytes[8];
                    _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                    _tcscat(tmpMsg, strSPDBytes);
                }

                SysInfo_DebugLogWriteLine(tmpMsg);

                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCap = tsodData[0];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCfg = tsodData[1];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wHiLim = tsodData[2];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wLoLim = tsodData[3];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wCritLim = tsodData[4];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wAmbTemp = tsodData[5];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wManufID = tsodData[6];
                g_MemTSODInfo[g_numTSODModules].raw.DDR4.wDevRev = tsodData[7];
                memcpy(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor, tsodData + 8, sizeof(g_MemTSODInfo[g_numTSODModules].raw.DDR4.wVendor));

                if (!CheckTSOD(&g_MemTSODInfo[g_numTSODModules]))
                {
                    _stprintf_s(tmpMsg, _T("**WARNING** TSOD may be invalid. Raw TSOD bytes for DIMM#%d:"), g_numTSODModules);
                    MtSupportUCDebugWriteLine(tmpMsg);

                    tmpMsg[0] = L'\0';
                    for (i = 0; i < MAX_TSOD_LENGTH; i++)
                    {
                        TCHAR strSPDBytes[8];
                        _stprintf_s(strSPDBytes, _T("%04X "), tsodData[i]);
                        _tcscat(tmpMsg, strSPDBytes);
                    }
                    MtSupportUCDebugWriteLine(tmpMsg);

                    if (!CheckCLTTPolling(tsodData))
                        continue;

                    SysInfo_DebugLogF(L"Found repeated TSOD words likely from CLTT. Continue parsing TSOD data");
                }

                // High limit
                temp = tsodData[2] & 0xFFC;
                if (BitExtract(tsodData[2], 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Low limit
                temp = tsodData[3] & 0xFFC;
                if (BitExtract(tsodData[3], 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // TCRIT limit
                temp = tsodData[4] & 0xFFC;
                if (BitExtract(tsodData[4], 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d TCRIT limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Current temperature
                temp = tsodData[5] & 0xFFF;
                if (tsodData[5] & (1 << 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %.3fC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, fTemp, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));
                SysInfo_DebugLogWriteLine(tmpMsg);

                if (abs((int)fTemp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = fTemp;
                    g_numTSODModules++;
                    numTSOD++;
                }
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %d.%03dC (Low=%d High=%d TCRIT=%d)"), g_numTSODModules, temp / 1000, temp % 1000, BitExtract(tsodData[5], 13, 13), BitExtract(tsodData[5], 14, 14), BitExtract(tsodData[5], 15, 15));
                SysInfo_DebugLogWriteLine(tmpMsg);

                if (ABS(temp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = temp;
                    g_numTSODModules++;
                    numTSOD++;
                }
#endif
            }
            else
            {
                _stprintf_s(tmpMsg, _T("Maximum number of TSOD modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
        }
    }
    return numTSOD;
}

int smbGetSPD5PIIX4(DWORD Bus, DWORD Dev, DWORD Fun, WORD BaseAddr, BYTE byRev)
{
    int numMemSticks = 0;
    int iNumSMBus = 1;
    DWORD Data;
    int idx, iNumberOfSPDBytes;
    TCHAR tmpMsg[VLONG_STRING_LEN];

#if 0
    if (byRev >= 0x49)
        iNumSMBus = 4;
#endif

    for (int smbus0sel = 0; smbus0sel < iNumSMBus; smbus0sel++)
    {
#if 0
        DWORD PrevDecodeEn = 0;
        if (byRev >= 0x49)
        {
#ifdef WIN32
            // Ask driver to map MMIO base address to pointer we can use
            BYTE* pbLinAddr = NULL;
            tagPhys32Struct Phys32Struct = { 0 };
            // HANDLE		hPhysicalMemory	= NULL;
            pbLinAddr = MapPhysToLin((PBYTE)(UINT_PTR)AMD_ACPI_FCH_PM_DECODEEN, 4, &Phys32Struct, TRUE);
            if (pbLinAddr)
            {
                PrevDecodeEn = *((DWORD*)pbLinAddr);
#else
            PrevDecodeEn = MmioRead32(AMD_ACPI_FCH_PM_DECODEEN);
#endif
            _stprintf_s(tmpMsg, _T("Previous MMIO PM DECODEEN: %08X (smbus0sel=%d)"), PrevDecodeEn, BitExtract(PrevDecodeEn, 20, 19));
            MtSupportUCDebugWriteLine(tmpMsg);

            if ((int)BitExtract(PrevDecodeEn, 20, 19) != smbus0sel)
            {
                DWORD NewDecodeEn = PrevDecodeEn;
                NewDecodeEn &= ~(0x3 << 19);
                NewDecodeEn |= (smbus0sel & 0x3) << 19;

                _stprintf_s(tmpMsg, _T("Writing MMIO PM DECODEEN: %08X (smbus0sel=%d)"), NewDecodeEn, BitExtract(NewDecodeEn, 20, 19));
                MtSupportUCDebugWriteLine(tmpMsg);

#ifdef WIN32
                * ((DWORD*)pbLinAddr) = NewDecodeEn;
#else
                MmioWrite32(AMD_ACPI_FCH_PM_DECODEEN, NewDecodeEn);
#endif
            }
#ifdef WIN32
            UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
            }
#endif
        }
#endif
        // for (idx = 0x0; idx <=  0x7F; idx++)
        // for (idx = 0x20; idx <=  0x68; idx++)
        // only scan the sub set of the memory chip locations
        for (idx = 0x50; idx <= 0x57; idx++)
        {
            if (idx != 0x69) // Don't probe the Clock chip
            {
#ifndef DDR5_DEBUG
                if (smbCallBusPIIX4(BaseAddr, 0, (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data))
#endif
                {
                    _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (PIIX4 SPD5)"), idx); // SI101005
                    SysInfo_DebugLogWriteLine(tmpMsg);
                    // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                    if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                    {
                        DWORD OldMR11 = 0;
#ifndef DDR5_DEBUG
                        int ioffset;
                        BYTE spdData[MAX_SPD_LENGTH];
                        memset(spdData, 0, sizeof(spdData));
#else
                        /*BYTE spdData[MAX_SPD_LENGTH] = {
                            0x30, 0x10, 0x12, 0x02, 0x04, 0x00, 0x20, 0x62, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x01, 0x0F, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x65, 0x01, 0xF2, 0x03, 0x7A, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3E,
                            0x80, 0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x27, 0x01, 0xA0, 0x00, 0x82, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD4, 0x00, 0x00, 0x00, 0xD4, 0x00, 0x00, 0x00, 0xD4, 0x00,
                            0x00, 0x00, 0xD4, 0x00, 0x00, 0x00, 0x88, 0x13, 0x08, 0x88, 0x13, 0x08, 0x20, 0x4E, 0x20, 0x10,
                            0x27, 0x10, 0xA4, 0x2C, 0x20, 0x10, 0x27, 0x10, 0xC4, 0x09, 0x04, 0x4C, 0x1D, 0x0C, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x10, 0x00, 0x86, 0x32, 0x80, 0x15, 0x8A, 0x8C, 0x82, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x01, 0x00, 0x81, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x56, 0x03,
                            0x85, 0x9B, 0x00, 0x18, 0x51, 0xFF, 0x01, 0x51, 0xD1, 0x44, 0x58, 0x32, 0x47, 0x36, 0x34, 0x30,
                            0x38, 0x59, 0x35, 0x32, 0x4B, 0x44, 0x32, 0x4A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x55, 0x80, 0x2C, 0x47, 0x42, 0x5A, 0x41, 0x4C, 0x4E,
                            0x36, 0x53, 0x58, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x0C, 0x4A, 0x30, 0x03, 0x05, 0x8A, 0x8C, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35, 0x36,
                            0x30, 0x30, 0x20, 0x43, 0x4C, 0x34, 0x36, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x35, 0x32,
                            0x30, 0x30, 0x20, 0x43, 0x4C, 0x34, 0x32, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB, 0xA9,
                            0x30, 0x22, 0x22, 0x00, 0x22, 0x65, 0x01, 0x7A, 0xAD, 0x00, 0x00, 0x00, 0x00, 0x26, 0x40, 0x80,
                            0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x27, 0x01, 0xA0, 0x00, 0x82, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0xC8, 0x28,
                            0x30, 0x22, 0x22, 0x00, 0x22, 0x80, 0x01, 0x7A, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3E, 0x80,
                            0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x27, 0x01, 0xA0, 0x00, 0x82, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x00, 0x6B, 0xB4,
                            0x45, 0x58, 0x50, 0x4F, 0x10, 0x55, 0x11, 0x00, 0x00, 0x00, 0x22, 0x22, 0x30, 0x00, 0x65, 0x01,
                            0x26, 0x40, 0x80, 0x3E, 0x80, 0x3E, 0x00, 0x7D, 0x80, 0xBB, 0x30, 0x75, 0x27, 0x01, 0xA0, 0x00,
                            0x82, 0x00, 0x88, 0x13, 0x88, 0x13, 0x20, 0x4E, 0x10, 0x27, 0xA4, 0x2C, 0x10, 0x27, 0xC4, 0x09,
                            0x4C, 0x1D, 0x22, 0x22, 0x30, 0x00, 0x80, 0x01, 0x80, 0x3E, 0x80, 0x3E, 0x80, 0x3E, 0x00, 0x7D,
                            0x80, 0xBB, 0x30, 0x75, 0x27, 0x01, 0xA0, 0x00, 0x82, 0x00, 0x88, 0x13, 0x88, 0x13, 0x20, 0x4E,
                            0x10, 0x27, 0x13, 0x30, 0x10, 0x27, 0xC4, 0x09, 0x4C, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0xAC,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        };

                        BYTE spdData[MAX_SPD_LENGTH] = {
                            0x30, 0x11, 0x12, 0x02, 0x04, 0x00, 0x20, 0x62, 0x00, 0x00, 0x00, 0x00, 0x90, 0x02, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0xA0, 0x01, 0xF2, 0x03, 0x72, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x41,
                            0x1A, 0x41, 0x1A, 0x41, 0x00, 0x7D, 0x1A, 0xBE, 0x30, 0x75, 0x27, 0x01, 0xA0, 0x00, 0x82, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x88, 0x13, 0x08, 0x88, 0x13, 0x08, 0x20, 0x4E, 0x20, 0x10,
                            0x27, 0x10, 0x15, 0x34, 0x20, 0x10, 0x27, 0x10, 0xC4, 0x09, 0x04, 0x4C, 0x1D, 0x0C, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x10, 0x00, 0x80, 0xB3, 0x80, 0x21, 0x0B, 0x10, 0x82, 0x44, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x01, 0x00, 0x81, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF2, 0xF6,
                            0x04, 0xCD, 0x00, 0x24, 0x02, 0xC7, 0x55, 0x2E, 0xF9, 0x46, 0x35, 0x2D, 0x36, 0x30, 0x30, 0x30,
                            0x4A, 0x33, 0x36, 0x33, 0x36, 0x46, 0x31, 0x36, 0x47, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x80, 0xCE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x45, 0x58, 0x50, 0x4F, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x27, 0x27, 0x30, 0x00, 0x4D, 0x01,
                            0xD8, 0x2E, 0xD8, 0x2E, 0xD8, 0x2E, 0xEC, 0x7C, 0xC5, 0xAB, 0x1D, 0x75, 0x27, 0x01, 0xA0, 0x00,
                            0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0xA5,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        }; */

                        BYTE spdData[MAX_SPD_LENGTH] = {// LPDDR5 CAMM2
                                                        0x30, 0x09, 0x15, 0x08, 0x86, 0x21, 0xB5, 0x00, 0x00, 0x40, 0x00, 0x00, 0x0A, 0x01, 0x00, 0x00,
                                                        0x48, 0x00, 0x09, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x90, 0xA8, 0x90, 0xC0, 0x08, 0x60,
                                                        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xC6, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x10, 0x00, 0x86, 0x32, 0x80, 0x15, 0x0B, 0x2A, 0x82, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x01, 0x04, 0x71, 0x00, 0x61, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE5, 0x78,
                                                        0x85, 0x9B, 0x00, 0x18, 0x51, 0xFF, 0x02, 0x11, 0xF2, 0x44, 0x58, 0x32, 0x47, 0x31, 0x32, 0x38,
                                                        0x31, 0x36, 0x59, 0x35, 0x32, 0x50, 0x49, 0x34, 0x4A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
                                                        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x59, 0x80, 0x2C, 0x58, 0x42, 0x5A, 0x41, 0x4C, 0x51,
                                                        0x45, 0x59, 0x30, 0x30, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif
                        iNumberOfSPDBytes = 1024;
                        // if (Data > 64)
                        //	iNumberOfSPDBytes = Data;
                        if (g_numMemModules >= MAX_MEMORY_SLOTS)
                            expand_spd_info_array();

                        if (g_numMemModules < MAX_MEMORY_SLOTS)
                            g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                        g_MemoryInfo[g_numMemModules].channel = -1;
                        g_MemoryInfo[g_numMemModules].slot = -1;
#ifndef DDR5_DEBUG
                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(0), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR0 - Device Type MSB: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(1), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR1 - Device Type LSB: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(2), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR2 - Device Revision: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(3), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR3 - Device Revision: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(4), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR4 - Device Revision: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(5), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR5 - Device Capability: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(11), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &OldMR11);
                        _stprintf_s(tmpMsg, _T("MR11 - I2C Legacy Mode Device Configuration: %02X"), OldMR11 & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(12), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR12 - NVM Protection Configuration: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(13), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR13 - NVM Protection Configuration: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(18), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR18 - Device Configuration: %02X (INF_SEL=%d)"), Data & 0xFF, (Data >> 5) & 1);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(52), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR52 - Hub and Thermal Sensor Error Status: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        for (ioffset = 0; ioffset < iNumberOfSPDBytes; ioffset += 2)
                        {
                            const int BYTES_PER_PAGE = 128;
                            int page = ioffset / BYTES_PER_PAGE;

                            if (ioffset % BYTES_PER_PAGE == 0)
                            {
                                // Check whether we were able to successfully change pages by comparing the bytes from page 0 and page 1
                                if (page == 2)
                                {
                                    if (memcmp(spdData, spdData + BYTES_PER_PAGE, BYTES_PER_PAGE) == 0)
                                    {
                                        int i;
                                        MtSupportUCDebugWriteLine(_T("Failed to set page - bytes from page 0 and 1 have the same values"));

                                        tmpMsg[0] = L'\0';
                                        for (i = 0; i < ioffset; i++)
                                        {
                                            TCHAR strSPDBytes[8];
                                            _stprintf_s(strSPDBytes, _T("%02X "), spdData[i]);
                                            _tcscat(tmpMsg, strSPDBytes);

                                            if ((i % 16 == 15) || (i == ioffset - 1))
                                            {
                                                MtSupportUCDebugWriteLine(tmpMsg);
                                                _stprintf_s(tmpMsg, _T(""));
                                            }
                                        }
                                        break;
                                    }
                                }
                                _stprintf_s(tmpMsg, _T("A - Setting page address to %d"), page);
                                MtSupportUCDebugWriteLine(tmpMsg);
                                smbSetSPD5PageAddrPIIX4(BaseAddr, (BYTE)idx, (BYTE)page);
                                _stprintf_s(tmpMsg, _T("A - Finished setting page address to %d"), page);
                                MtSupportUCDebugWriteLine(tmpMsg);
                            }

                            if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                            {

                                MtSupportUCDebugWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                                break;
                            }

                            smbCallBusPIIX4(BaseAddr, (BYTE)(SPD5_CMD_MEMREG | (ioffset & (0x80 - 1))), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_WORD_DATA, &Data);
#if 0 // <km> Using 2-bit addressing
                        Data = SPD5_CMD_MEMREG | (ioffset & (0x80 - 1));
                        Data |= ((ioffset >> 7) & 0x0F) << 8;
                        smbCallBusPIIX4(BaseAddr, 0, (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_PROC_CALL, &Data);
#endif
                            spdData[ioffset] = (BYTE)(Data & 0xFF);
                            spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);

                            if (ioffset == 2 && !IsDDR5SPD(spdData, ioffset))
                            {
                                MtSupportUCDebugWriteLine(_T("Error - not DDR5 SPD data"));
                                break;
                            }
                        }

                        if (ioffset < iNumberOfSPDBytes)
                            continue;
#endif
                        if (g_numMemModules < MAX_MEMORY_SLOTS)
                        {
                            if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                            {
                                g_numMemModules++;
                                numMemSticks++;
#ifdef DDR5_DEBUG
                                break;
#endif
                            }
                        }
                        else
                        {
                            _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                            SysInfo_DebugLogWriteLine(tmpMsg);
                        }

                        smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(11), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                        _stprintf_s(tmpMsg, _T("MR11 - I2C Legacy Mode Device Configuration: %02X"), Data & 0xFF);
                        MtSupportUCDebugWriteLine(tmpMsg);

                        if ((Data & 0xFF) != (OldMR11 & 0xFF))
                        {
                            _stprintf_s(tmpMsg, _T("Restoring MR11 back to %02X"), OldMR11 & 0xFF);
                            MtSupportUCDebugWriteLine(tmpMsg);

                            smbSetSPD5MRPIIX4(BaseAddr, (BYTE)idx, 11, (BYTE)OldMR11);

                            smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(0), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                            _stprintf_s(tmpMsg, _T("MR0 - Device Type MSB: %02X"), Data & 0xFF);
                            MtSupportUCDebugWriteLine(tmpMsg);

                            smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(1), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                            _stprintf_s(tmpMsg, _T("MR1 - Device Type LSB: %02X"), Data & 0xFF);
                            MtSupportUCDebugWriteLine(tmpMsg);
                        }
                    }
                }
            }
        }
#if 0
    if (byRev >= 0x49)
    {
        DWORD DecodeEn;
#ifdef WIN32
        // Ask driver to map MMIO base address to pointer we can use
        BYTE* pbLinAddr = NULL;
        tagPhys32Struct Phys32Struct = { 0 };
        // HANDLE		hPhysicalMemory	= NULL;
        pbLinAddr = MapPhysToLin((PBYTE)(UINT_PTR)AMD_ACPI_FCH_PM_DECODEEN, 4, &Phys32Struct, TRUE);
        if (pbLinAddr)
        {
            DecodeEn = *((DWORD*)pbLinAddr);
#else
        DecodeEn = MmioRead32(AMD_ACPI_FCH_PM_DECODEEN);
#endif
        if (DecodeEn != PrevDecodeEn)
        {
            _stprintf_s(tmpMsg, _T("Restoring MMIO PM DECODEEN: %08X (smbus0sel=%d)"), PrevDecodeEn, BitExtract(PrevDecodeEn, 20, 19));
            MtSupportUCDebugWriteLine(tmpMsg);

#ifdef WIN32
            * ((DWORD*)pbLinAddr) = PrevDecodeEn;
#else
            MmioWrite32(AMD_ACPI_FCH_PM_DECODEEN, PrevDecodeEn);
#endif
        }
#ifdef WIN32
        UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
        }
#endif
    }
#endif
    }
    return numMemSticks;
}

int smbGetSPD5TSODPIIX4(WORD BaseAddr)
{
    DWORD Data;
    int idx;
    int numTSOD = 0;
    TCHAR tmpMsg[VLONG_STRING_LEN];
    memset(tmpMsg, 0, sizeof(tmpMsg));

    // for (idx = 0x0; idx <=  0x7F; idx++)
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (smbCallBusPIIX4(BaseAddr, 0, (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data))
        {
            SysInfo_DebugLogF(L"Device detected on SMBUS at address %02X (smbGetSPD5TSODPIIX4)", idx);

            DebugMessageBoxes(tmpMsg);

            // if (Data > 64)
            //	iNumberOfSPDBytes = Data;
            if (g_numTSODModules >= MAX_TSOD_SLOTS)
                expand_tsod_info_array();

            g_MemTSODInfo[g_numTSODModules].channel = -1;
            g_MemTSODInfo[g_numTSODModules].slot = -1;

            if (g_numTSODModules < MAX_TSOD_SLOTS)
            {
                int temp;
#ifdef WIN32
                float fTemp;
#endif
                // Device type - MSB
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(0), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                if ((Data & 0xFF) != 0x51)
                {
                    SysInfo_DebugLogF(L"Unexpected MR0 - Device Type MSB: %02X", Data & 0xFF);
                    continue;
                }

                // Device type - LSB
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(1), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                if ((Data & 0xFF) != 0x18)
                {
                    SysInfo_DebugLogF(L"Unexpected MR1 - Device Type LSB: %02X", Data & 0xFF);
                    continue;
                }

                // Device Capability
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(5), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                if ((Data & 0xFF) != 0x03)
                {
                    SysInfo_DebugLogF(L"Unexpected MR5 - Device Capability: %02X", Data & 0xFF);
                    continue;
                }

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(26), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                _stprintf_s(tmpMsg, _T("MR26 - Thermal Sensor Configuration: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                if (BitExtract(Data, 0, 0))
                {
                    MtSupportUCDebugWriteLine(_T("Temperature sensor disabled. Enabling ..."));

                    Data &= ~1;
                    smbSetSPD5MRPIIX4(BaseAddr, (BYTE)idx, 26, (BYTE)(Data & 0xFF));
                    Sleep(1);

                    smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(26), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                    _stprintf_s(tmpMsg, _T("MR26 - Thermal Sensor Configuration: %02X"), Data & 0xFF);
                    MtSupportUCDebugWriteLine(tmpMsg);
                }

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(52), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                _stprintf_s(tmpMsg, _T("MR52 - Hub and Thermal Sensor Error Status: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(26), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bTSConfig = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR26 - TS Configuration: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(27), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bIntConfig = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR27 - Interrupt Configuration: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(28), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim = Data & 0xFF;
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(29), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR28-29 - TS Temp High Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(30), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim = Data & 0xFF;
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(31), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR30-31 - TS Temp Low Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(32), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim = Data & 0xFF;
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(33), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR32-33 - TS Critical Temp High Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(34), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim = Data & 0xFF;
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(35), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim |= (Data & 0xFF) << 8;
                _stprintf_s(tmpMsg, _T("MR34-35 - TS Critical Temp Low Limit Configuration: %04X"), g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(49), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp = Data & 0xFF;
                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(50), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp |= (Data & 0xFF) << 8;
                SysInfo_DebugLogF(L"MR49-50 - TS Current Sensed Temperature: %04X", g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(51), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bTSStatus = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR51 - TS Temperature Status: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(52), (BYTE)idx, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
                g_MemTSODInfo[g_numTSODModules].raw.DDR5.bError = Data & 0xFF;
                _stprintf_s(tmpMsg, _T("MR51 - Hub, Thermal & NVM Error Status: %02X"), Data & 0xFF);
                SysInfo_DebugLogWriteLine(tmpMsg);

                // High limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wHiLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Low limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wLoLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Critical High limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritHiLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical high  limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical high limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Critical Low limit
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim & 0xFFC;
                if (BitExtract(g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCritLoLim, 12, 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical high  limit: %.3fC"), g_numTSODModules, fTemp);
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d Critical low limit: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);

                // Current temperature
                temp = g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp & 0xFFC;
                if (g_MemTSODInfo[g_numTSODModules].raw.DDR5.wCurTemp & (1 << 12)) // two's complement
                    temp = -((temp ^ 0xFFF) + 1);
#ifdef WIN32
                fTemp = (float)temp * 0.0625f;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %.3fC"), g_numTSODModules, fTemp);

                if (abs((int)fTemp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = fTemp;
                    g_numTSODModules++;
                    numTSOD++;
                }
#else
                temp = temp * 625 / 10;
                _stprintf_s(tmpMsg, _T("DIMM%d temperature: %d.%03dC"), g_numTSODModules, temp / 1000, temp % 1000);

                if (ABS(temp) < MAX_TSOD_TEMP)
                {
                    g_MemTSODInfo[g_numTSODModules].temperature = temp;
                    g_numTSODModules++;
                    numTSOD++;
                }
#endif
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
            else
            {
                _stprintf_s(tmpMsg, _T("Maximum number of TSOD modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
            }
        }
    }
    return numTSOD;
}

DISABLE_OPTIMISATIONS() // Optimisation must be turned off
BOOL smbCallBusPIIX4(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Result = 0, Dump1;
    int ret;

    if (smbWaitForFreePIIX4(BaseAddr) == FALSE)
        return FALSE;

    ret = SetPortVal(BaseAddr + PIIX4_SMB_CMD, CMD, 1);                // Host command
    ret = SetPortVal(BaseAddr + PIIX4_SMB_ADDR, (Slave << 1) | RW, 1); // Slave address
    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA, 0, 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA2, 0, 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_CTRL, PIIX4_SMB_PRTCL_START | Prot, 1); // Initiate operation
    // Sleep(1);

    if (smbWaitForEndPIIX4(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + PIIX4_SMB_DATA2, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Dump1 << 8;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + PIIX4_SMB_DATA, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Result | Dump1;

    *Data = Result;

    return TRUE;
}
ENABLE_OPTIMISATIONS()

BOOL smbWaitForFreePIIX4(WORD BaseAddr)
{

    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + PIIX4_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while ((Status) && !CHECK_TIMEOUT(timeout, PIIX4_MAX_TIMEOUT))
    {
        SetPortVal(BaseAddr + PIIX4_SMB_STS, Status, 1);
        // Sleep(1);
        ret = GetPortVal(BaseAddr + PIIX4_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, PIIX4_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForFreePIIX4: Timeout exceeded (SMB_STS=0x%08X)"), Status);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        return FALSE;
    }

    return TRUE;
}

BOOL smbWaitForEndPIIX4(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + PIIX4_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while ((Status & PIIX4_SMBHSTSTS_HOST_BUSY) && !CHECK_TIMEOUT(timeout, PIIX4_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + PIIX4_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (Status & (PIIX4_SMBHSTSTS_DEV_ERR | PIIX4_SMBHSTSTS_BUS_ERR | PIIX4_SMBHSTSTS_FAILED))
        return FALSE;

    if (CHECK_TIMEOUT(timeout, PIIX4_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndPIIX4: Timeout exceeded (SMB_STS=0x%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        return FALSE;
    }

    if (Status & PIIX4_SMBHSTSTS_DEV_ERR)
        return FALSE;

    if (Status & PIIX4_SMBHSTSTS_BUS_ERR)
        return FALSE;

    if (Status & PIIX4_SMBHSTSTS_FAILED)
        return FALSE;

    return TRUE;
}

BOOL smbSetBankAddrPIIX4(WORD BaseAddr, BYTE bBankNo)
{
    BYTE Slave = 0;
    int ret;

    if (smbWaitForFreePIIX4(BaseAddr) == FALSE)
        return FALSE;

    if (bBankNo)
        Slave = 0x6E;
    else
        Slave = 0x6C;

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrPIIX4: Setting SMB_ADDR=%08X"), Slave);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_ADDR, Slave, 1);                                         // Slave address
    ret = SetPortVal(BaseAddr + PIIX4_SMB_CTRL, PIIX4_SMB_PRTCL_START | PIIX4_SMB_PRTCL_QUICK, 1); // Initiate operation
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetBankAddrPIIX4: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    if (smbWaitForEndPIIX4(BaseAddr) == FALSE)
        return FALSE;

    return TRUE;
}

BOOL smbSetSPD5PageAddrPIIX4(WORD BaseAddr, BYTE Slave, BYTE Page)
{
    int ret;
    DWORD Data;
    BYTE Cmd;

    if (smbWaitForFreePIIX4(BaseAddr) == FALSE)
        return FALSE;

    smbCallBusPIIX4(BaseAddr, (BYTE)SPD5_VOLATILE_REG_MR(11), Slave, RW_READ, PIIX4_SMB_PRTCL_BYTE_DATA, &Data);
    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrPIIX4: MR11 - I2C Legacy Mode Device Configuration: %02X"), Data & 0xFF);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    Data &= ~0x0f;
    Data |= SPD5_MR11_I2C_LEGACY_MODE_ADDR_1BYTE | (Page & 0x7);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrPIIX4: Setting SMB_DATA=%08X"), Data);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA, Data, 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA2, 0, 1);

    Cmd = SPD5_VOLATILE_REG_MR(11);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrPIIX4: Setting SMB_CMD=%08X"), Cmd);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_CMD, Cmd, 1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_ADDR, (Slave << 1), 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_CTRL, PIIX4_SMB_PRTCL_START | PIIX4_SMB_PRTCL_BYTE_DATA, 1);
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5PageAddrPIIX4: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndPIIX4(BaseAddr) == FALSE)
    {
        SysInfo_DebugLogWriteLine(_T("smbSetSPD5PageAddrPIIX4 failed"));
        return FALSE;
    }

    return TRUE;
}

BOOL smbSetSPD5AddrModePIIX4(WORD BaseAddr, BYTE Slave, BYTE AddrMode)
{
    int ret;
    DWORD Data;
    BYTE Cmd;

    if (smbWaitForFreePIIX4(BaseAddr) == FALSE)
        return FALSE;

    Data = AddrMode;

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5AddrModePIIX4: Setting SMB_DATA=%08X"), Data);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA, Data, 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA2, 0, 1);

    Cmd = SPD5_VOLATILE_REG_MR(11);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5AddrModePIIX4: Setting SMB_CMD=%08X"), Cmd);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_CMD, Cmd, 1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_ADDR, (Slave << 1), 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_CTRL, PIIX4_SMB_PRTCL_START | PIIX4_SMB_PRTCL_BYTE_DATA, 1);
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5AddrModePIIX4: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndPIIX4(BaseAddr) == FALSE)
    {
        // SysInfo_DebugLogWriteLine(_T("smbSetBankAddrPIIX4 failed"));
        return FALSE;
    }

    return TRUE;
}

BOOL smbSetSPD5MRPIIX4(WORD BaseAddr, BYTE Slave, BYTE MR, BYTE Data)
{
    int ret;
    BYTE Cmd;

    if (smbWaitForFreePIIX4(BaseAddr) == FALSE)
        return FALSE;

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5MRPIIX4: Setting SMB_DATA=%02X"), Data);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA, Data, 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_DATA2, 0, 1);

    Cmd = SPD5_VOLATILE_REG_MR(MR);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5MRPIIX4: Setting SMB_CMD=%08X"), Cmd);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_CMD, Cmd, 1);

    ret = SetPortVal(BaseAddr + PIIX4_SMB_ADDR, (Slave << 1), 1);
    ret = SetPortVal(BaseAddr + PIIX4_SMB_CTRL, PIIX4_SMB_PRTCL_START | PIIX4_SMB_PRTCL_BYTE_DATA, 1);
    // Sleep(1);

    _stprintf_s(g_szSITempDebug1, _T("smbSetSPD5MRPIIX4: Waiting for end"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Wait until the command has finished
    if (smbWaitForEndPIIX4(BaseAddr) == FALSE)
    {
        // SysInfo_DebugLogWriteLine(_T("smbSetBankAddrPIIX4 failed"));
        return FALSE;
    }

    return TRUE;
}

// IO address mapped offsets from SMBASE
#define VIA_SMB_STS 0x00
#define VIA_SMB_CTRL 0x02
#define VIA_SMB_CMD 0x03
#define VIA_SMB_ADDR 0x04
#define VIA_SMB_DATA 0x05
#define VIA_SMB_DATA2 0x06
#define VIA_SMB_BLOCK_DATA 0x07

/* VIA constants */
#define VIA_SMB_PRTCL_QUICK 0x00
#define VIA_SMB_PRTCL_BYTE 0x04
#define VIA_SMB_PRTCL_BYTE_DATA 0x08
#define VIA_SMB_PRTCL_WORD_DATA 0x0C
#define VIA_SMB_PRTCL_PROC_CALL 0x10
#define VIA_SMB_PRTCL_BLOCK_DATA 0x14
#define VIA_SMB_PRTCL_I2C_BLOCK_DATA 0x34
#define VIA_SMB_PRTCL_START 0x40

/* VIA Hosts Status register bits */

#define VIA_SMBHSTSTS_FAILED 0x10
#define VIA_SMBHSTSTS_BUS_ERR 0x08
#define VIA_SMBHSTSTS_DEV_ERR 0x04
#define VIA_SMBHSTSTS_INTR 0x02
#define VIA_SMBHSTSTS_HOST_BUSY 0x01

/* Other settings */
#define VIA_MAX_TIMEOUT 250

int smbGetSPDVia(WORD BaseAddr)
{
    int numMemSticks = 0;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    // for (idx = 0x20; idx <=  0x68; idx++)
    // only scan the sub set of the memory chip locations
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            ret = SetPortVal(BaseAddr + VIA_SMB_DATA, 0, 1); // Clear the first data byte - Host data 1

            if (smbCallBusVia(BaseAddr, 0, (BYTE)idx, RW_READ, VIA_SMB_PRTCL_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Via)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }
                        ret = SetPortVal(BaseAddr + VIA_SMB_DATA, 0, 1);
                        ret = SetPortVal(BaseAddr + VIA_SMB_DATA2, 0, 1);
                        ret = smbCallBusVia(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, VIA_SMB_PRTCL_WORD_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

BOOL smbCallBusVia(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Result = 0, Dump1;
    int ret;

    if (smbWaitForFreeVia(BaseAddr) == FALSE)
        return FALSE;

    ret = SetPortVal(BaseAddr + VIA_SMB_CMD, CMD, 1);                         // Command
    ret = SetPortVal(BaseAddr + VIA_SMB_ADDR, (Slave << 1) | RW, 1);          // Slave address
    ret = SetPortVal(BaseAddr + VIA_SMB_CTRL, VIA_SMB_PRTCL_START | Prot, 1); // Initiate operation
    // Sleep(1);
    if (smbWaitForEndVia(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + VIA_SMB_DATA2, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Dump1 << 8;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + VIA_SMB_DATA, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Result | Dump1;

    *Data = Result;
    return TRUE;
}

BOOL smbWaitForFreeVia(WORD BaseAddr)
{

    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + VIA_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while ((Status) && !CHECK_TIMEOUT(timeout, VIA_MAX_TIMEOUT))
    {
        SetPortVal(BaseAddr + VIA_SMB_STS, Status, 1);
        // Sleep(1);
        ret = GetPortVal(BaseAddr + VIA_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, VIA_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndVia(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + VIA_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while ((Status & PIIX4_SMBHSTSTS_HOST_BUSY) && !CHECK_TIMEOUT(timeout, VIA_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + VIA_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, VIA_MAX_TIMEOUT))
        return FALSE;

    if (Status & VIA_SMBHSTSTS_DEV_ERR)
        return FALSE;

    if (Status & VIA_SMBHSTSTS_BUS_ERR)
        return FALSE;

    if (Status & VIA_SMBHSTSTS_FAILED)
        return FALSE;

    return TRUE;
}

// IO address mapped offsets from SMBASE
#define AMD756_SMB_STS 0x00
#define AMD756_SMB_CTRL 0x02
#define AMD756_SMB_ADDR 0x04
#define AMD756_SMB_DATA 0x06
#define AMD756_SMB_CMD 0x08
#define AMD756_SMB_BLOCK_DATA 0x09

/* AMD756 constants */
#define AMD756_SMB_PRTCL_QUICK 0x00
#define AMD756_SMB_PRTCL_BYTE 0x01
#define AMD756_SMB_PRTCL_BYTE_DATA 0x02
#define AMD756_SMB_PRTCL_WORD_DATA 0x03
#define AMD756_SMB_PRTCL_PROCESS_CALL 0x04
#define AMD756_SMB_PRTCL_BLOCK_DATA 0x05

#define AMD756_SMBGS_ABRT_STS (1 << 0)
#define AMD756_SMBGS_COL_STS (1 << 1)
#define AMD756_SMBGS_PRERR_STS (1 << 2)
#define AMD756_SMBGS_HST_STS (1 << 3)
#define AMD756_SMBGS_HCYC_STS (1 << 4)
#define AMD756_SMBGS_TO_STS (1 << 5)
#define AMD756_SMBGS_SMB_STS (1 << 11)

#define AMD756_SMBGS_CLEAR_STS (AMD756_SMBGS_ABRT_STS | AMD756_SMBGS_COL_STS | AMD756_SMBGS_PRERR_STS | \
                                AMD756_SMBGS_HCYC_STS | AMD756_SMBGS_TO_STS)

#define AMD756_SMBGE_CYC_TYPE_MASK (7)
#define AMD756_SMBGE_HOST_STC (1 << 3)
#define AMD756_SMBGE_ABORT (1 << 5)

/* Other settings */
#define AMD756_MAX_TIMEOUT 250

int smbGetSPDAmd756(WORD BaseAddr)
{
    int numMemSticks = 0;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    //
    // only scan the sub set of the memory chip locations
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            ret = SetPortVal(BaseAddr + AMD756_SMB_DATA, 0, 2); // Clear the first data byte - Host data 1

            if (smbCallBusAmd756(BaseAddr, 0, (BYTE)idx, RW_READ, AMD756_SMB_PRTCL_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (AMD756)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data & 0xFF;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        ret = SetPortVal(BaseAddr + AMD756_SMB_DATA, 0, 2);
                        ret = smbCallBusAmd756(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, AMD756_SMB_PRTCL_WORD_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

BOOL smbCallBusAmd756(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Dump1;
    DWORD GlobalEn = 0;
    int ret;

    if (smbWaitForFreeAmd756(BaseAddr) == FALSE)
        return FALSE;

    ret = SetPortVal(BaseAddr + AMD756_SMB_CMD, CMD, 1);                // Command
    ret = SetPortVal(BaseAddr + AMD756_SMB_ADDR, (Slave << 1) | RW, 2); // Slave address

    ret = GetPortVal(BaseAddr + AMD756_SMB_CTRL, &GlobalEn, 2);
    ret = SetPortVal(BaseAddr + AMD756_SMB_CTRL, GlobalEn | (Prot & AMD756_SMBGE_CYC_TYPE_MASK) | AMD756_SMBGE_HOST_STC, 2);
    // Sleep(1);
    if (smbWaitForEndAmd756(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + AMD756_SMB_DATA, (DWORD *)&Dump1, 2); // Get 1 byte of data (nVidia nForce4)

    *Data = Dump1;
    return TRUE;
}

BOOL smbWaitForFreeAmd756(WORD BaseAddr)
{

    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + AMD756_SMB_STS, (DWORD *)&Status, 2); // Get SMBus status
    while ((Status & (AMD756_SMBGS_HST_STS | AMD756_SMBGS_SMB_STS)) && !CHECK_TIMEOUT(timeout, AMD756_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + AMD756_SMB_STS, (DWORD *)&Status, 2); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, AMD756_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndAmd756(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + AMD756_SMB_STS, (DWORD *)&Status, 2); // Get SMBus status
    while ((Status & AMD756_SMBGS_HST_STS) && !CHECK_TIMEOUT(timeout, AMD756_MAX_TIMEOUT))
    {
        // Sleep(1); //500
        ret = GetPortVal(BaseAddr + AMD756_SMB_STS, (DWORD *)&Status, 2); // Get SMBus status
    }
    SetPortVal(BaseAddr + AMD756_SMB_STS, AMD756_SMBGS_CLEAR_STS, 2);

    if (CHECK_TIMEOUT(timeout, AMD756_MAX_TIMEOUT))
        return FALSE;

    if (Status & AMD756_SMBGS_PRERR_STS)
        return FALSE;

    if (Status & AMD756_SMBGS_COL_STS)
        return FALSE;

    if (Status & AMD756_SMBGS_TO_STS)
        return FALSE;

    return TRUE;
}

// IO address mapped offsets from SMBASE
#define SIS96X_SMB_STS 0x00 /* status */
#define SIS96X_SMB_EN 0x01  /* status enable */
#define SIS96X_SMB_CNT 0x02
#define SIS96X_SMBHOST_CNT 0x03
#define SIS96X_SMB_ADDR 0x04
#define SIS96X_SMB_CMD 0x05
#define SIS96X_SMB_PCOUNT 0x06 /* processed count */
#define SIS96X_SMB_COUNT 0x07
#define SIS96X_SMB_BYTE 0x08 /* ~0x8F data byte field */
#define SIS96X_SMBDEV_ADDR 0x10
#define SIS96X_SMB_DB0 0x11
#define SIS96X_SMB_DB1 0x12
#define SIS96X_SMB_SAA 0x13

/* SIS96X constants */
#define SIS96X_SMB_PRTCL_QUICK 0x00
#define SIS96X_SMB_PRTCL_BYTE 0x01
#define SIS96X_SMB_PRTCL_BYTE_DATA 0x02
#define SIS96X_SMB_PRTCL_WORD_DATA 0x03
#define SIS96X_SMB_PRTCL_PROC_CALL 0x04
#define SIS96X_SMB_PRTCL_BLOCK_DATA 0x05
#define SIS96X_SMB_PRTCL_START 0x10

#define SIS96X_SMBSTS_BUS_ERR 0x04
#define SIS96X_SMBSTS_DEV_ERR 0x02

/* Other settings */
#define SIS96X_MAX_TIMEOUT 250

int smbGetSPDSiS96x(WORD BaseAddr)
{
    int numMemSticks = 0;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    //
    // only scan the sub set of the memory chip locations
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            ret = SetPortVal(BaseAddr + SIS96X_SMB_BYTE, 0, 1); // Clear the first data byte - Host data 1

            if (smbCallBusSiS96x(BaseAddr, 0, (BYTE)idx, RW_READ, SIS96X_SMB_PRTCL_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (SiS96x)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data & 0xFF;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }
                        ret = SetPortVal(BaseAddr + SIS96X_SMB_BYTE, 0, 2);
                        ret = smbCallBusSiS96x(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, SIS96X_SMB_PRTCL_BYTE_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

BOOL smbCallBusSiS96x(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Status = 0;
    DWORD Dump1;
    int ret;

    if (smbWaitForFreeSiS96x(BaseAddr) == FALSE)
        return FALSE;

    /* clear all (sticky) status flags */
    ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1);
    ret = SetPortVal(BaseAddr + SIS96X_SMB_STS, Status & 0x1E, 1);

    ret = SetPortVal(BaseAddr + SIS96X_SMB_CMD, CMD, 1);                // Command
    ret = SetPortVal(BaseAddr + SIS96X_SMB_ADDR, (Slave << 1) | RW, 1); // Slave address
    ret = SetPortVal(BaseAddr + SIS96X_SMBHOST_CNT, (Prot & 0x07) | SIS96X_SMB_PRTCL_START, 1);
    // Sleep(1);
    if (smbWaitForEndSiS96x(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + SIS96X_SMB_BYTE, (DWORD *)&Dump1, 2);

    *Data = Dump1;
    return TRUE;
}

BOOL smbWaitForFreeSiS96x(WORD BaseAddr)
{

    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + SIS96X_SMB_CNT, (DWORD *)&Status, 1); // Get SMBus status
    while (((Status & 0x03) != 0x00) && !CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + SIS96X_SMB_CNT, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndSiS96x(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while (((Status & 0x0E) != 0) && !CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
    {
        // Sleep(1); //500
        ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }
    SetPortVal(BaseAddr + SIS96X_SMB_STS, Status, 1);

    if (CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
        return FALSE;

    if (Status & SIS96X_SMBSTS_DEV_ERR)
        return FALSE;

    if (Status & SIS96X_SMBSTS_BUS_ERR)
        return FALSE;

    return TRUE;
}

#define SIS968_SMB_BYTE 0x10 /* ~0x8F data byte field */

int smbGetSPDSiS968(WORD BaseAddr)
{
    int numMemSticks = 0;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    //
    // only scan the sub set of the memory chip locations
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            ret = SetPortVal(BaseAddr + SIS968_SMB_BYTE, 0, 1); // Clear the first data byte - Host data 1

            if (smbCallBusSiS968(BaseAddr, 0, (BYTE)idx, RW_READ, SIS96X_SMB_PRTCL_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (SiS968)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data & 0xFF;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        ret = SetPortVal(BaseAddr + SIS968_SMB_BYTE, 0, 1);
                        ret = smbCallBusSiS968(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, SIS96X_SMB_PRTCL_BYTE_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

BOOL smbCallBusSiS968(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Status = 0;
    DWORD Dump1;
    int ret;

    if (smbWaitForFreeSiS968(BaseAddr) == FALSE)
        return FALSE;

    /* clear all (sticky) status flags */
    ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1);
    ret = SetPortVal(BaseAddr + SIS96X_SMB_STS, Status & 0x1E, 1);

    ret = SetPortVal(BaseAddr + SIS96X_SMB_CMD, CMD, 1);                // Command
    ret = SetPortVal(BaseAddr + SIS96X_SMB_ADDR, (Slave << 1) | RW, 1); // Slave address
    ret = SetPortVal(BaseAddr + SIS96X_SMBHOST_CNT, (Prot & 0x07) | SIS96X_SMB_PRTCL_START, 1);
    // Sleep(1);
    if (smbWaitForEndSiS968(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + SIS968_SMB_BYTE, (DWORD *)&Dump1, 1);

    *Data = Dump1;
    return TRUE;
}

BOOL smbWaitForFreeSiS968(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    SetPortVal(BaseAddr + SIS96X_SMB_STS, 0xFF, 1);

    ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while (((Status & 0x0E) != 0) && !CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
    {
        // Sleep(1); //500
        ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForFreeSiS968: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

BOOL smbWaitForEndSiS968(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while (((Status & 0x0E) == 0) && !CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
    {
        // Sleep(1); //500
        ret = GetPortVal(BaseAddr + SIS96X_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }
    SetPortVal(BaseAddr + SIS96X_SMB_STS, Status, 1);

    if (CHECK_TIMEOUT(timeout, SIS96X_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndSiS968: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & SIS96X_SMBSTS_DEV_ERR)
        return FALSE;

    if (Status & SIS96X_SMBSTS_BUS_ERR)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndSiS968: bus error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

/* SIS630/730/964 SMBus registers */
#define SIS630_SMB_STS 0x00     /* status */
#define SIS630_SMB_CNT 0x02     /* control */
#define SIS630_SMBHOST_CNT 0x03 /* host control */
#define SIS630_SMB_ADDR 0x04    /* address */
#define SIS630_SMB_CMD 0x05     /* command */
#define SIS630_SMB_COUNT 0x07   /* byte count */
#define SIS630_SMB_BYTE 0x08    /* ~0x8F data byte field */

/* SIS96X constants */
#define SIS630_SMB_PRTCL_QUICK 0x00
#define SIS630_SMB_PRTCL_BYTE 0x01
#define SIS630_SMB_PRTCL_BYTE_DATA 0x02
#define SIS630_SMB_PRTCL_WORD_DATA 0x03
#define SIS630_SMB_PRTCL_PROC_CALL 0x04
#define SIS630_SMB_PRTCL_BLOCK_DATA 0x05
#define SIS630_SMB_PRTCL_START 0x10

#define SIS630_SMBSTS_BUS_ERR 0x04
#define SIS630_SMBSTS_DEV_ERR 0x02

/* Other settings */
#define SIS630_MAX_TIMEOUT 250

int smbGetSPDSiS630(WORD BaseAddr)
{
    int numMemSticks = 0;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    //
    // only scan the sub set of the memory chip locations
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            ret = SetPortVal(BaseAddr + SIS630_SMB_BYTE, 0, 1); // Clear the first data byte - Host data 1

            if (smbCallBusSiS630(BaseAddr, 0, (BYTE)idx, RW_READ, SIS630_SMB_PRTCL_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (SiS630)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data & 0xFF;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset++)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }

                        ret = SetPortVal(BaseAddr + SIS630_SMB_BYTE, 0, 1);
                        ret = smbCallBusSiS630(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, SIS630_SMB_PRTCL_BYTE_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

BOOL smbCallBusSiS630(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Status = 0;
    DWORD Dump1;
    int ret;

    if (smbWaitForFreeSiS630(BaseAddr) == FALSE)
        return FALSE;

    /* clear all (sticky) status flags */
    ret = GetPortVal(BaseAddr + SIS630_SMB_STS, (DWORD *)&Status, 1);
    ret = SetPortVal(BaseAddr + SIS630_SMB_STS, Status & 0x1E, 1);

    ret = SetPortVal(BaseAddr + SIS630_SMB_CMD, CMD, 1);                // Command
    ret = SetPortVal(BaseAddr + SIS630_SMB_ADDR, (Slave << 1) | RW, 1); // Slave address
    ret = SetPortVal(BaseAddr + SIS630_SMBHOST_CNT, (Prot & 0x07) | SIS630_SMB_PRTCL_START, 1);
    // Sleep(1);
    if (smbWaitForEndSiS630(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + SIS630_SMB_BYTE, (DWORD *)&Dump1, 1);

    *Data = Dump1;
    return TRUE;
}

BOOL smbWaitForFreeSiS630(WORD BaseAddr)
{

    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + SIS630_SMB_CNT, (DWORD *)&Status, 1); // Get SMBus status
    while (((Status & 0x03) != 0x00) && !CHECK_TIMEOUT(timeout, SIS630_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + SIS630_SMB_CNT, (DWORD *)&Status, 1); // Get SMBus status
    }

    if (CHECK_TIMEOUT(timeout, SIS630_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForFreeSiS630: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

BOOL smbWaitForEndSiS630(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + SIS630_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    while (((Status & 0x0E) != 0) && !CHECK_TIMEOUT(timeout, SIS630_MAX_TIMEOUT))
    {
        // Sleep(1); //500
        ret = GetPortVal(BaseAddr + SIS630_SMB_STS, (DWORD *)&Status, 1); // Get SMBus status
    }
    SetPortVal(BaseAddr + SIS630_SMB_STS, Status, 1);

    if (CHECK_TIMEOUT(timeout, SIS630_MAX_TIMEOUT))
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndSiS630: Timeout (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & SIS630_SMBSTS_DEV_ERR)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndSiS630: Device Error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    if (Status & SIS630_SMBSTS_BUS_ERR)
    {
        _stprintf_s(g_szSITempDebug1, _T("smbWaitForEndSiS630: Bus Error (Status=%08X)"), Status);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return FALSE;
    }

    return TRUE;
}

// IO address mapped offsets from SMBASE
#define ALI1563_SMB_HST_STS 0x00
#define ALI1563_SMB_HST_CNTL1 0x01
#define ALI1563_SMB_HST_CNTL2 0x02
#define ALI1563_SMB_HST_CMD 0x03
#define ALI1563_SMB_HST_ADDR 0x04
#define ALI1563_SMB_HST_DATA 0x05
#define ALI1563_SMB_HST_DATA2 0x06
#define ALI1563_SMB_BLOCK_DATA 0x07

/* ALI1563 constants */
#define ALI1563_SMB_HST_CNTL2_KILL 0x04
#define ALI1563_SMB_HST_CNTL2_START 0x40
#define ALI1563_SMB_HST_CNTL2_QUICK 0x00
#define ALI1563_SMB_HST_CNTL2_BYTE 0x01
#define ALI1563_SMB_HST_CNTL2_BYTE_DATA 0x02
#define ALI1563_SMB_HST_CNTL2_WORD_DATA 0x03
#define ALI1563_SMB_HST_CNTL2_BLOCK 0x05

#define ALI1563_SMB_HST_STS_BUSY 0x01
#define ALI1563_SMB_HST_STS_INTR 0x02
#define ALI1563_SMB_HST_STS_DEVERR 0x04
#define ALI1563_SMB_HST_STS_BUSERR 0x08
#define ALI1563_SMB_HST_STS_FAIL 0x10
#define ALI1563_SMB_HST_STS_DONE 0x80
#define ALI1563_SMB_HST_STS_BAD 0x1c

#define ALI1563_SMB_HST_CNTL1_TIMEOUT 0x80
#define ALI1563_SMB_HST_CNTL1_LAST 0x40

#define ALI1563_SMB_HST_CNTL2_SIZEMASK 0x38

/* Other settings */
#define ALI1563_MAX_TIMEOUT 250

int smbGetSPDAli1563(WORD BaseAddr)
{
    int numMemSticks = 0;
    DWORD Data;
    int idx, ioffset, iNumberOfSPDBytes, ret;
    TCHAR tmpMsg[VLONG_STRING_LEN];

    // for (idx = 0x0; idx <=  0x7F; idx++)
    //
    // only scan the sub set of the memory chip locations
    for (idx = 0x50; idx <= 0x57; idx++)
    {
        if (idx != 0x69) // Don't probe the Clock chip
        {
            ret = SetPortVal(BaseAddr + ALI1563_SMB_HST_DATA, 0, 1); // Clear the first data byte - Host data 1

            if (smbCallBusAli1563(BaseAddr, 0, (BYTE)idx, RW_READ, ALI1563_SMB_HST_CNTL2_BYTE_DATA, &Data))
            {
                _stprintf_s(tmpMsg, _T("Device detected on SMBUS at address %02X (Ali1563)"), idx); // SI101005
                SysInfo_DebugLogWriteLine(tmpMsg);
                // Memory chips should be hardcoded to 0x50 to 0x57 (but what happens if there are more than 8 sticks of RAM?)
                if (idx >= 0x50 && idx <= 0x57) // Should be a memory chip (should be at 0x50 - 0x57)
                {
                    BYTE spdData[MAX_SPD_LENGTH];
                    memset(spdData, 0, sizeof(spdData));

                    iNumberOfSPDBytes = 256;
                    // if (Data > 64)
                    //	iNumberOfSPDBytes = Data & 0xFF;
                    if (g_numMemModules >= MAX_MEMORY_SLOTS)
                        expand_spd_info_array();

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                        g_MemoryInfo[g_numMemModules].dimmNum = g_numMemModules;
                    g_MemoryInfo[g_numMemModules].channel = -1;
                    g_MemoryInfo[g_numMemModules].slot = -1;

                    ioffset = 0;
                    _stprintf_s(tmpMsg, _T("Retrieving SPD bytes %d-%d"), ioffset, iNumberOfSPDBytes - 1);
                    SysInfo_DebugLogWriteLine(tmpMsg);

                    // For a memory chip only the first 98 bytes are standard - the rest are manufacturer specific
                    //  and are not defined in the spec.

                    for (ioffset = 0; ioffset < iNumberOfSPDBytes && ioffset < 256; ioffset += 2)
                    {
                        if (ioffset == 16 && !CheckSPDBytes(spdData, 16))
                        {
                            SysInfo_DebugLogWriteLine(_T("First 16 bytes 00 or FF - not valid"));
                            break;
                        }
                        ret = SetPortVal(BaseAddr + ALI1563_SMB_HST_DATA, 0, 1);
                        ret = SetPortVal(BaseAddr + ALI1563_SMB_HST_DATA2, 0, 1);
                        ret = smbCallBusAli1563(BaseAddr, (BYTE)ioffset, (BYTE)idx, RW_READ, ALI1563_SMB_HST_CNTL2_WORD_DATA, &Data);
                        spdData[ioffset] = (BYTE)(Data & 0xFF);
                        spdData[ioffset + 1] = (BYTE)((Data >> 8) & 0xFF);
                    }

                    if (g_numMemModules < MAX_MEMORY_SLOTS)
                    {
                        if (DecodeSPD(spdData, iNumberOfSPDBytes, &g_MemoryInfo[g_numMemModules])) // SI101005
                        {
                            g_numMemModules++;
                            numMemSticks++;
                        }
                    }
                    else
                    {
                        _stprintf_s(tmpMsg, _T("Maximum number of memory modules exceeded (%u)"), MAX_MEMORY_SLOTS); // SI101005
                        SysInfo_DebugLogWriteLine(tmpMsg);
                    }
                }
            }
        }
    }
    return numMemSticks;
}

BOOL smbCallBusAli1563(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data)
{
    DWORD Result = 0, Dump1;
    DWORD Ctrl2 = 0;
    int ret;

    if (smbWaitForFreeAli1563(BaseAddr) == FALSE)
        return FALSE;

    ret = SetPortVal(BaseAddr + ALI1563_SMB_HST_CMD, CMD, 1);                // Command
    ret = SetPortVal(BaseAddr + ALI1563_SMB_HST_ADDR, (Slave << 1) | RW, 1); // Slave address
    ret = GetPortVal(BaseAddr + ALI1563_SMB_HST_CNTL2, &Ctrl2, 1);
    ret = SetPortVal(BaseAddr + ALI1563_SMB_HST_CNTL2, (Ctrl2 & ~ALI1563_SMB_HST_CNTL2_SIZEMASK) | (Prot << 3) | ALI1563_SMB_HST_CNTL2_START, 1);

    // Sleep(1);
    if (smbWaitForEndAli1563(BaseAddr) == FALSE)
        return FALSE;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + ALI1563_SMB_HST_DATA2, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Dump1 << 8;

    Dump1 = 0;
    ret = GetPortVal(BaseAddr + ALI1563_SMB_HST_DATA, (DWORD *)&Dump1, 1); // 1 Bytes
    Result = Result | Dump1;

    *Data = Result;
    return TRUE;
}

BOOL smbWaitForFreeAli1563(WORD BaseAddr)
{

    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + ALI1563_SMB_HST_STS, (DWORD *)&Status, 1); // Get SMBus status
    while ((Status & ALI1563_SMB_HST_STS_BUSY) && !CHECK_TIMEOUT(timeout, ALI1563_MAX_TIMEOUT))
    {
        // Sleep(1);
        ret = GetPortVal(BaseAddr + ALI1563_SMB_HST_STS, (DWORD *)&Status, 1); // Get SMBus status
    }
    // Clear status bits
    ret = SetPortVal(BaseAddr + ALI1563_SMB_HST_STS, 0xFF, 1);

    if (CHECK_TIMEOUT(timeout, ALI1563_MAX_TIMEOUT))
        return FALSE;

    return TRUE;
}

BOOL smbWaitForEndAli1563(WORD BaseAddr)
{
    DWORD Status = 0;
    int ret;
    unsigned long long timeout = 0;

    INIT_TIMEOUT(timeout);

    ret = GetPortVal(BaseAddr + ALI1563_SMB_HST_STS, (DWORD *)&Status, 1); // Get SMBus status
    while ((Status & ALI1563_SMB_HST_STS_BUSY) && !CHECK_TIMEOUT(timeout, ALI1563_MAX_TIMEOUT))
    {
        // Sleep(1); //500
        ret = GetPortVal(BaseAddr + ALI1563_SMB_HST_STS, (DWORD *)&Status, 1); // Get SMBus status
    }
    SetPortVal(BaseAddr + ALI1563_SMB_HST_STS, 0xFF, 1);

    if (CHECK_TIMEOUT(timeout, ALI1563_MAX_TIMEOUT))
        return FALSE;

    if (Status & ALI1563_SMB_HST_STS_BAD)
        return FALSE;

    if (Status & ALI1563_SMB_HST_STS_DEVERR)
        return FALSE;

    if (Status & ALI1563_SMB_HST_STS_BUSERR)
        return FALSE;

    if (Status & ALI1563_SMB_HST_STS_FAIL)
        return FALSE;

    return TRUE;
}

#ifdef WIN32
#define TO_TIMING_VAR(x) ((float)x)
#define SPRINTF_FLOAT(str, format, floatval) _stprintf_s(str, format _T("%.3f"), floatval)
#define CEIL(dividend, divisor) ceil((float)dividend / (float)divisor)
#else
#define TO_TIMING_VAR(x) (1000 * x)
#define SPRINTF_FLOAT(str, format, floatval) _stprintf_s(str, format _T("%d.%03d"), floatval / 1000, floatval % 1000)
#define CEIL(dividend, divisor) ((dividend + divisor - 40) / divisor)
#endif

bool DecodeSPD(BYTE *spdData, int spdDataLen, SPDINFO *MemoryInfo)
{
    typedef enum
    {
        SDRAMREV123,
        DDRVER10,
        DDR2FB10,
        DDR2FB11,
        DDR210,
        DDR212,
        DDR310,
        DDR311,
        DDR312,
        DDR313,
        DDR410,
        LPDDR411,
        DDR510,
        LPDDR510
    } SPDVER;

    SPDVER SPDVer = SDRAMREV123;
    int i = 0;

    TCHAR tmpMsg[VLONG_STRING_LEN];
    _stprintf_s(tmpMsg, _T("Raw SPD bytes for DIMM#%d (Channel %d, Slot %d):"), MemoryInfo->dimmNum, MemoryInfo->channel, MemoryInfo->slot);
#ifdef WIN32
    SysInfo_DebugLogWriteLine(tmpMsg);
#else
    MtSupportUCDebugWriteLine(tmpMsg);
#endif
    _stprintf_s(tmpMsg, _T(""));

    for (i = 0; i < spdDataLen; i++)
    {
        TCHAR strSPDBytes[8];
        _stprintf_s(strSPDBytes, _T("%02X "), spdData[i]);
        _tcscat(tmpMsg, strSPDBytes);

        if ((i % 16 == 15) || (i == spdDataLen - 1))
        {
#ifdef WIN32
            SysInfo_DebugLogWriteLine(tmpMsg);
#else
            MtSupportUCDebugWriteLine(tmpMsg);
#endif
            _stprintf_s(tmpMsg, _T(""));
        }
    }

    // Just check if all bytes are 0xFF or 0x00 - if so, don't decode the SPD data as it is clearly junk //SI101005
    if (!CheckSPDBytes(spdData, spdDataLen))
    {
        SysInfo_DebugLogWriteLine(_T("All bytes 00 or FF - not valid"));
        return false;
    }

    // Check if byte 0 is 0x00 (Number of Bytes Used / Number of Bytes in SPD Device)
    if (spdData[0] == 0x00)
    {
        SysInfo_DebugLogWriteLine(_T("Byte 0 is not valid"));
        return false;
    }

    // Check if byte 1 is 0x00 (SPD Revision)
    if (spdData[1] == 0x00)
    {
        SysInfo_DebugLogWriteLine(_T("Byte 1 is not valid"));
        return false;
    }

    // Get the type of the memory, byte 2
    MemoryInfo->type = spdData[2];

    if (spdData[2] == SPDINFO_MEMTYPE_DDR2) // DDR2
    {
        MemoryInfo->spdRev = spdData[62];
        SysInfo_DebugLogWriteLine(_T("RAM type: DDR2"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[62]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (spdData[62] == 0x10)
            SPDVer = DDR210;
        else if (spdData[62] == 0x12 || spdData[62] == 0x13)
            SPDVer = DDR212;
        else
        {
            SysInfo_DebugLogWriteLine(_T("Byte 62: SPD revision is not valid"));
            return false;
        }
    }
    else if (spdData[2] == SPDINFO_MEMTYPE_DDR2FB || spdData[2] == SPDINFO_MEMTYPE_DDR2FBPROBE) // FB ram
    {
        MemoryInfo->spdRev = spdData[1];
        SysInfo_DebugLogWriteLine(_T("RAM type: DDR2 FB"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[1]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        // if it is FB ram, use byte 1 to determine SPD revision
        if (spdData[1] == 0x10)
            SPDVer = DDR2FB10;
        else if (spdData[1] == 0x11)
            SPDVer = DDR2FB11;
        else
        {
            SysInfo_DebugLogWriteLine(_T("Byte 1: SPD revision is not valid"));
            return false;
        }
    }
    else if (spdData[2] == SPDINFO_MEMTYPE_DDR3) // DDR3
    {
        MemoryInfo->spdRev = spdData[1];
        SysInfo_DebugLogWriteLine(_T("RAM type: DDR3"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[1]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // if it is DDR3 ram, use byte 1 to determine SPD revision
        if (spdData[1] == 0x10)
            SPDVer = DDR310;
        else if (spdData[1] == 0x11)
            SPDVer = DDR311;
        else if (spdData[1] == 0x12)
            SPDVer = DDR312;
        else if (spdData[1] >= 0x13 && spdData[1] <= 0x15)
            SPDVer = DDR313; // default to rev 1.3
        else
        {
            SysInfo_DebugLogWriteLine(_T("Byte 1: SPD revision is not valid"));
            return false;
        }
    }
    else if (spdData[2] == SPDINFO_MEMTYPE_DDR4 || // DDR4
             spdData[2] == SPDINFO_MEMTYPE_DDR4E)  // DDR4E
    {
        MemoryInfo->spdRev = spdData[1];
        SysInfo_DebugLogWriteLine(_T("RAM type: DDR4"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[1]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // if it is DDR4 ram, use byte 1 to determine SPD revision
        if (spdData[1] >= 0x10 && spdData[1] <= 0x15)
            SPDVer = DDR410;
        else
        {
            SysInfo_DebugLogWriteLine(_T("Byte 1: SPD revision is not valid"));
            return false;
        }
    }
#if 0
    else if (spdData[2] == SPDINFO_MEMTYPE_LPDDR3 || // LPDDR3
        spdData[2] == SPDINFO_MEMTYPE_LPDDR4 ||  // LPDDR4
        spdData[2] == SPDINFO_MEMTYPE_LPDDR4X)  // LPDDR4X
    {
        MemoryInfo->spdRev = spdData[1];
        SysInfo_DebugLogWriteLine(_T("RAM type: LPDDR3/LPDDR4"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[1]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (spdData[1] >= 0x10 && spdData[1] <= 0x11)
            SPDVer = LPDDR411;
        else
        {
            SysInfo_DebugLogWriteLine(_T("Byte 1: SPD revision is not valid"));
            return false;
        }
    }
#endif
    else if (spdData[2] == SPDINFO_MEMTYPE_DDR5 ||        // DDR5
             spdData[2] == SPDINFO_MEMTYPE_DDR5_NVDIMM_P) // DDR5 NVDIMM-P
    {
        MemoryInfo->spdRev = spdData[1];
        MtSupportUCDebugWriteLine(_T("RAM type: DDR5"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[1]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        SPDVer = DDR510;

        if (spdDataLen < 1024)
        {
            _stprintf_s(g_szSITempDebug1, _T("Length of SPD (%d) not valid"), spdDataLen);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
            return false;
        }

        WORD crc16Calc = (WORD)Crc16(spdData, 510);
        WORD crc16Stored = (spdData[511] << 8) + spdData[510];
        if (crc16Calc != crc16Stored)
        {
            _stprintf_s(g_szSITempDebug1, _T("Calculated CRC16 (%04X) does not match stored CRC16 (%04X)"), crc16Calc, crc16Stored);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
            // return false;
        }

        if (spdData[0] & 0x80)
        {
            _stprintf_s(g_szSITempDebug1, _T("SPD Block 9 contains %s information"), (spdData[0] & 0x80) ? L"error logging" : L"manufacturing");
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
        }
    }
    else if (spdData[2] == SPDINFO_MEMTYPE_LPDDR5 || // LPDDR5
             spdData[2] == SPDINFO_MEMTYPE_LPDDR5X)  // LPDDR5X
    {
        MemoryInfo->spdRev = spdData[1];
        MtSupportUCDebugWriteLine(_T("RAM type: LPDDR5"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[1]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        SPDVer = LPDDR510;

        if (spdDataLen < 1024)
        {
            _stprintf_s(g_szSITempDebug1, _T("Length of SPD (%d) not valid"), spdDataLen);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
            return false;
        }

        WORD crc16Calc = (WORD)Crc16(spdData, 510);
        WORD crc16Stored = (spdData[511] << 8) + spdData[510];
        if (crc16Calc != crc16Stored)
        {
            _stprintf_s(g_szSITempDebug1, _T("Calculated CRC16 (%04X) does not match stored CRC16 (%04X)"), crc16Calc, crc16Stored);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
            // return false;
        }
    }
    else if (spdData[2] == SPDINFO_MEMTYPE_DDR) // DDR
    {
        SysInfo_DebugLogWriteLine(_T("RAM type: DDR"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[62]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        if (spdData[62] >= 0x10 && spdData[62] <= 0x15)
            SPDVer = DDRVER10;
        else
        {
            SysInfo_DebugLogWriteLine(_T("Byte 62: SPD revision is not valid"));
            return false;
        }
    }
    else if (spdData[2] == SPDINFO_MEMTYPE_SDRAM) // SDR SDRAM
    {
        MemoryInfo->spdRev = spdData[62];
        SysInfo_DebugLogWriteLine(_T("RAM type: SDR SDRAM"));
        _stprintf_s(g_szSITempDebug1, _T("SPD rev: 0x%02X"), spdData[62]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        if (spdData[62] >= 0x10 && spdData[62] <= 0x15)
            SPDVer = SDRAMREV123;
        else
        {
            SysInfo_DebugLogWriteLine(_T("Byte 62: SPD revision is not valid"));
            return false;
        }
    }
    else
    {
        _stprintf_s(g_szSITempDebug1, _T("Unable to decode RAM type"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        return false; // SI101006 - if we don't know the RAM type (e.g. garbage SPD data), then don't try to decode it
    }

    switch (SPDVer)
    {
    case SDRAMREV123:
    {
        int pcclk;
#ifdef WIN32
        float clk;
        float trcd;
        float trp;
        float tras;

#else
        int clk;
        int trcd;
        int trp;
        int tras;

#endif
        int index;
        int highestCAS = 0;
        unsigned char numCont = 0;
        // Manufacturing information
        // JEDEC ID
        // get the JedecID Bytes 64-71, shift each one by the right amount eg 0 for 1st part, 8 for second part
        // as ID is stored in 8 parts
        MemoryInfo->jedecID = 0;

        for (i = 64; i <= 71; i++)
        {
            if (spdData[i] == 0x7F)
                numCont++;
            else
            {
                MemoryInfo->jedecID = spdData[i];
                break;
            }
        }
        MemoryInfo->jedecBank = numCont + 1;
        GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
        _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleManufLoc = spdData[72];
        _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        for (i = 0; i < SPD_SDR_MODULE_PARTNO_LEN; i++)
        {
            if (spdData[73 + i] >= 0 && spdData[73 + i] < 128) // Sanity check for ASCII char
                MemoryInfo->modulePartNo[i] = spdData[73 + i];
            else
                break;
        }

#ifdef WIN32
        TCHAR wcPartNo[SPD_SDR_MODULE_PARTNO_LEN + 1];
        memset(wcPartNo, 0, sizeof(wcPartNo));
        MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_SDR_MODULE_PARTNO_LEN, wcPartNo, SPD_SDR_MODULE_PARTNO_LEN + 1);
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleRev = (spdData[91] << 8) | spdData[92];
        _stprintf_s(g_szSITempDebug1, _T("Revision Code: %04X"), MemoryInfo->moduleRev);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Get the date code for the memory, bytes 93 and 94
        // Interepret the stored value based on the SPD ver
        // Convert from BCD
        if (spdData[93] > 0) // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->year = 1900 + ((spdData[93] >> 4) * 10) + (spdData[93] & 0x0F);
        if (spdData[94] > 0) // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->week = ((spdData[94] >> 4) * 10) + (spdData[94] & 0x0F);

        _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleSerialNo = (spdData[95] << 24) | (spdData[96] << 16) | (spdData[97] << 8) | spdData[98];
        _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleExtSerialNo[0] = numsetbits32(numCont) & 1 ? numCont : (numCont | 0x80);
        MemoryInfo->moduleExtSerialNo[1] = MemoryInfo->jedecID;
        MemoryInfo->moduleExtSerialNo[2] = MemoryInfo->moduleManufLoc;
        MemoryInfo->moduleExtSerialNo[3] = spdData[93];
        MemoryInfo->moduleExtSerialNo[4] = spdData[94];
        memcpy(&MemoryInfo->moduleExtSerialNo[5], &spdData[95], 4);

        // speed
#ifdef WIN32
        MemoryInfo->tCK = (spdData[9] >> 4) + (spdData[9] & 0xf) * 0.1f;
        clk = 1000 / MemoryInfo->tCK;
        MemoryInfo->txWidth = (spdData[7] << 8) + spdData[6];
        if ((spdData[11] == 2) || (spdData[11] == 1))
        {
            MemoryInfo->txWidth = MemoryInfo->txWidth - 8;
        }
        pcclk = int(clk * MemoryInfo->txWidth / 8);
        if ((pcclk % 100) >= 50) // Round properly
            pcclk += 100;
        pcclk = pcclk - (pcclk % 100);
        MemoryInfo->clkspeed = (1000 / MemoryInfo->tCK);
#else
        MemoryInfo->tCK = (spdData[9] >> 4) * 1000 + (spdData[9] & 0xf) * 100;
        clk = 1000000 / MemoryInfo->tCK;
        MemoryInfo->txWidth = (spdData[7] << 8) + spdData[6];
        if ((spdData[11] == 2) || (spdData[11] == 1))
        {
            MemoryInfo->txWidth = MemoryInfo->txWidth - 8;
        }
        pcclk = (1000000 * MemoryInfo->txWidth) / (8 * MemoryInfo->tCK);
        if ((pcclk % 100) >= 50) // Round properly
            pcclk += 100;
        pcclk = pcclk - (pcclk % 100);
        MemoryInfo->clkspeed = (1000000 / MemoryInfo->tCK);
#endif
        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MHz (%dMB/s)"), (int)clk, pcclk);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(MemoryInfo->szModulespeed, _T("%dMB/s"), pcclk);

        MemoryInfo->numBanks = spdData[17];
        MemoryInfo->rowAddrBits = spdData[3] & 0x0f;
        MemoryInfo->colAddrBits = spdData[4] & 0x0f;
        MemoryInfo->busWidth = (spdData[7] << 8) | spdData[6];
        MemoryInfo->numRanks = spdData[5];

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"), spdData[17], spdData[3] & 0x0f, spdData[4] & 0x0f, (spdData[7] << 8) | spdData[6]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), spdData[5]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = spdData[13] & 0x7F;
        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d bits"), spdData[13] & 0x7F);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // size computation
        index = MemoryInfo->rowAddrBits + MemoryInfo->colAddrBits - 17;

        MemoryInfo->size = (1 << index) * MemoryInfo->numRanks * MemoryInfo->numBanks;
        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // ecc
        if (spdData[11] == 0x2)
            MemoryInfo->ecc = true;
        else
            MemoryInfo->ecc = false;

        _stprintf_s(g_szSITempDebug1, _T("ECC: %s"), MemoryInfo->ecc ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _tcscpy(MemoryInfo->moduleVoltage, VOLTAGE_LEVEL_TABLE[spdData[8] & 0x0F]);
        _stprintf_s(g_szSITempDebug1, _T("Voltage Interface Level: %s"), MemoryInfo->moduleVoltage);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // refresh rate
        _tcscpy(MemoryInfo->specific.SDRSDRAM.RefreshRate, REFRESH_RATE_TABLE[spdData[12] & 0x0F]);

        _stprintf_s(g_szSITempDebug1, _T("Refresh Rate: %s"), MemoryInfo->specific.SDRSDRAM.RefreshRate);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // burst
        _stprintf_s(MemoryInfo->specific.SDRSDRAM.BurstLengthsSupported, _T(""));

        for (i = 0x01; i <= 0x08; i = i << 1)
        {
            if (spdData[16] & i)
            {
                TCHAR tmpChar[8];
                _stprintf_s(tmpChar, _T("%d "), i);
                _tcscat(MemoryInfo->specific.SDRSDRAM.BurstLengthsSupported, tmpChar);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported Burst Lengths: %s"), MemoryInfo->specific.SDRSDRAM.BurstLengthsSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // CAS latencies supported
        _stprintf_s(MemoryInfo->CASSupported, _T(""));

        for (i = 0; i < 7; i++)
        {
            if (spdData[18] & (1 << i))
            {
                TCHAR strCAS[8];
                highestCAS = 1 + i;
                _stprintf_s(strCAS, _T("%d "), 1 + i);
                _tcscat(MemoryInfo->CASSupported, strCAS);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported CAS Latencies (tCL): %s"), MemoryInfo->CASSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        trcd = TO_TIMING_VAR(spdData[29]);
        trp = TO_TIMING_VAR(spdData[27]);
        tras = TO_TIMING_VAR(spdData[30]);
#ifdef WIN32
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz): %d-%d-%d-%d"),
                    MemoryInfo->clkspeed,
                    highestCAS,
                    (int)ceil(trcd / MemoryInfo->tCK),
                    (int)ceil(trp / MemoryInfo->tCK),
                    (int)ceil(tras / MemoryInfo->tCK));
#else
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz): %d-%d-%d-%d"),
                    MemoryInfo->clkspeed,
                    highestCAS,
                    CEIL(trcd, MemoryInfo->tCK),
                    CEIL(trp, MemoryInfo->tCK),
                    CEIL(tras, MemoryInfo->tCK));
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // latencies
        _stprintf_s(MemoryInfo->specific.SDRSDRAM.CSSupported, _T(""));

        for (i = 0; i < 7; i++)
        {
            if (spdData[19] & (1 << i))
            {
                TCHAR strCS[8];
                _stprintf_s(strCS, _T("%d "), i);
                _tcscat(MemoryInfo->specific.SDRSDRAM.CSSupported, strCS);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported CS Latencies: %s"), MemoryInfo->specific.SDRSDRAM.CSSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.SDRSDRAM.WESupported, _T(""));

        for (i = 0; i < 7; i++)
        {
            if (spdData[20] & (1 << i))
            {
                TCHAR strWE[8];
                _stprintf_s(strWE, _T("%d "), i);
                _tcscat(MemoryInfo->specific.SDRSDRAM.WESupported, strWE);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported WE Latencies: %s"), MemoryInfo->specific.SDRSDRAM.WESupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Module feature bitmap
        MemoryInfo->specific.SDRSDRAM.buffered = (spdData[21] & SPD_SDR_MODULE_ATTR_BUFFERED) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Buffered address and control inputs: %s"), spdData[21] & SPD_SDR_MODULE_ATTR_BUFFERED ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->registered = (spdData[21] & SPD_SDR_MODULE_ATTR_REGISTERED) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Registered address and control inputs: %s"), spdData[21] & SPD_SDR_MODULE_ATTR_REGISTERED ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.SDRSDRAM.OnCardPLL = (spdData[21] & SPD_SDR_MODULE_ATTR_ONCARD_PLL) != 0;
        _stprintf_s(g_szSITempDebug1, _T("On-Card PLL (Clock): %s"), spdData[21] & SPD_SDR_MODULE_ATTR_ONCARD_PLL ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.SDRSDRAM.BufferedDQMB = (spdData[21] & SPD_SDR_MODULE_ATTR_BUFFERED_DQMB) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Buffered DQMB Inputs: %s"), spdData[21] & SPD_SDR_MODULE_ATTR_BUFFERED_DQMB ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.SDRSDRAM.RegisteredDQMB = (spdData[21] & SPD_SDR_MODULE_ATTR_REGISTERED_DQMB) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Registered DQMB Inputs: %s"), spdData[21] & SPD_SDR_MODULE_ATTR_REGISTERED_DQMB ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.SDRSDRAM.DiffClockInput = (spdData[21] & SPD_SDR_MODULE_ATTR_DIFF_CLOCK) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Differential clock input: %s"), spdData[21] & SPD_SDR_MODULE_ATTR_DIFF_CLOCK ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Chip feature bitmap
        MemoryInfo->specific.SDRSDRAM.EarlyRASPrechargeSupported = (spdData[22] & SPD_SDR_CHIP_ATTR_EARLY_RAS_PRECH) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports Early RAS# Precharge: %s"), spdData[22] & SPD_SDR_CHIP_ATTR_EARLY_RAS_PRECH ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.SDRSDRAM.AutoPrechargeSupported = (spdData[22] & SPD_SDR_CHIP_ATTR_AUTO_PRECH) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports Concurrent Auto Precharge: %s"), spdData[22] & SPD_SDR_CHIP_ATTR_AUTO_PRECH ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.SDRSDRAM.PrechargeAllSupported = (spdData[22] & SPD_SDR_CHIP_ATTR_PRECH_ALL) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports Precharge All: %s"), spdData[22] & SPD_SDR_CHIP_ATTR_PRECH_ALL ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.SDRSDRAM.WriteReadBurstSupported = (spdData[22] & SPD_SDR_CHIP_ATTR_WRITE_READ_BURST) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports Write/Read Burst: %s"), spdData[22] & SPD_SDR_CHIP_ATTR_WRITE_READ_BURST ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Lower Vcc tolerance: %s"), spdData[22] & SPD_DDR_CHIP_ATTR_LOWER_VCC_TOL ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Upper Vcc tolerance: %s"), spdData[22] & SPD_DDR_CHIP_ATTR_UPPER_VCC_TOL ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // timing
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at highest CAS latency: "), MemoryInfo->tCK);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tAC = (spdData[10] >> 4) + (spdData[10] & 0xf) * 0.1f;
#else
        MemoryInfo->specific.SDRSDRAM.tAC = (spdData[10] >> 4) * 1000 + (spdData[10] & 0xf) * 100;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at highest CAS latency: "), MemoryInfo->specific.SDRSDRAM.tAC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tCKmed = (spdData[23] >> 4) + (spdData[23] & 0xf) * 0.1f;
#else
        MemoryInfo->specific.SDRSDRAM.tCKmed = (spdData[23] >> 4) * 1000 + (spdData[23] & 0xf) * 100;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at medium CAS latency: "), MemoryInfo->specific.SDRSDRAM.tAC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tACmed = (spdData[24] >> 4) + (spdData[24] & 0xf) * 0.1f;
#else
        MemoryInfo->specific.SDRSDRAM.tACmed = (spdData[24] >> 4) * 1000 + (spdData[24] & 0xf) * 100;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at medium CAS latency: "), MemoryInfo->specific.SDRSDRAM.tACmed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tCKshort = (spdData[25] >> 2) + (spdData[25] & 0x02) * 0.25f;
#else
        MemoryInfo->specific.SDRSDRAM.tCKshort = (spdData[25] >> 2) * 1000 + (spdData[25] & 0x02) * 250;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at shortest CAS latency: "), MemoryInfo->specific.SDRSDRAM.tCKshort);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tACshort = (spdData[26] >> 2) + (spdData[26] & 0x02) * 0.25f;
#else
        MemoryInfo->specific.SDRSDRAM.tACshort = (spdData[26] >> 2) * 1000 + (spdData[26] & 0x02) * 250;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at shortest CAS latency: "), MemoryInfo->specific.SDRSDRAM.tACshort);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // more timing information
#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tIS = ((spdData[32] >> 4) & 0x07) + (spdData[32] & 0x0f) * 0.1f;
        if (spdData[32] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tIS = -MemoryInfo->specific.SDRSDRAM.tIS;
#else
        MemoryInfo->specific.SDRSDRAM.tIS = ((spdData[32] >> 4) & 0x07) * 1000 + (spdData[32] & 0x0f) * 100;
        if (spdData[32] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tIS = -MemoryInfo->specific.SDRSDRAM.tIS;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Address/Command Setup Time Before Clock (tIS): "), MemoryInfo->specific.SDRSDRAM.tIS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tIH = ((spdData[33] >> 4) & 0x07) + (spdData[33] & 0x0f) * 0.1f;
        if (spdData[33] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tIH = -MemoryInfo->specific.SDRSDRAM.tIH;
#else
        MemoryInfo->specific.SDRSDRAM.tIH = ((spdData[33] >> 4) & 0x07) * 1000 + (spdData[33] & 0x0f) * 100;
        if (spdData[33] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tIH = -MemoryInfo->specific.SDRSDRAM.tIH;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Address/Command Hold Time After Clock (tIH): "), MemoryInfo->specific.SDRSDRAM.tIH);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tDS = ((spdData[34] >> 4) & 0x07) + (spdData[34] & 0x0f) * 0.1f;
        if (spdData[34] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tDS = -MemoryInfo->specific.SDRSDRAM.tDS;
#else
        MemoryInfo->specific.SDRSDRAM.tDS = ((spdData[34] >> 4) & 0x07) * 1000 + (spdData[34] & 0x0f) * 100;
        if (spdData[34] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tDS = -MemoryInfo->specific.SDRSDRAM.tDS;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data Input Setup Time Before Strobe (tDS): "), MemoryInfo->specific.SDRSDRAM.tDS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.SDRSDRAM.tDH = ((spdData[35] >> 4) & 0x07) + (spdData[35] & 0x0f) * 0.1f;
        if (spdData[35] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tDH = -MemoryInfo->specific.SDRSDRAM.tDH;
#else
        MemoryInfo->specific.SDRSDRAM.tDH = ((spdData[35] >> 4) & 0x07) * 1000 + (spdData[35] & 0x0f) * 100;
        if (spdData[35] & 0x80)
            MemoryInfo->specific.SDRSDRAM.tDH = -MemoryInfo->specific.SDRSDRAM.tDH;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data Input Hold Time After Strobe (tDH): "), MemoryInfo->specific.SDRSDRAM.tDH);

        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tAA = highestCAS * MemoryInfo->tCK;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = trp;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Precharge Delay (tRP): "), MemoryInfo->tRP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRRD = TO_TIMING_VAR(spdData[28]);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Active to Row Active Delay (tRRD): "), MemoryInfo->tRRD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = trcd;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS# to CAS# Delay (tRCD): "), MemoryInfo->tRCD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRAS = tras;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS# Pulse Width (tRAS): "), MemoryInfo->tRAS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
    }
    break;
    case DDRVER10:
    {
#ifdef WIN32
        float ddrclk;
        float highestCAS;
        float trcd;
        float trp;
        float tras;
#else
        int ddrclk;
        int highestCAS;
        int trcd;
        int trp;
        int tras;
#endif
        int pcclk;
        int index;
        unsigned char numCont = 0;

        // Manufacturing information
        // JEDEC ID
        // get the JedecID Bytes 64-71, shift each one by the right amount eg 0 for 1st part, 8 for second part
        // as ID is stored in 8 parts
        MemoryInfo->jedecID = 0;
        for (i = 64; i <= 71; i++)
        {
            if (spdData[i] == 0x7F)
                numCont++;
            else
            {
                MemoryInfo->jedecID = spdData[i];
                break;
            }
        }
        MemoryInfo->jedecBank = numCont + 1;
        GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
        _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleManufLoc = spdData[72];
        _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        for (i = 0; i < SPD_DDR_MODULE_PARTNO_LEN; i++)
        {
            if (spdData[73 + i] >= 0 && spdData[73 + i] < 128) // Sanity check for ASCII char
                MemoryInfo->modulePartNo[i] = spdData[73 + i];
            else
                break;
        }

#ifdef WIN32
        TCHAR wcPartNo[SPD_DDR_MODULE_PARTNO_LEN + 1];
        memset(wcPartNo, 0, sizeof(wcPartNo));
        MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_DDR_MODULE_PARTNO_LEN, wcPartNo, SPD_DDR_MODULE_PARTNO_LEN + 1);
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleRev = (spdData[91] << 8) | spdData[92];
        _stprintf_s(g_szSITempDebug1, _T("Revision Code: 0x%04X"), MemoryInfo->moduleRev);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Get the date code for the memory, bytes 93 and 94
        // Interepret the stored value based on the SPD ver
        // Convert from BCD
        if (spdData[93] > 0) // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->year = 2000 + ((spdData[93] >> 4) * 10) + (spdData[93] & 0x0F);
        if (spdData[94] > 0) // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->week = ((spdData[94] >> 4) * 10) + (spdData[94] & 0x0F);

        _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleSerialNo = (spdData[95] << 24) | (spdData[96] << 16) | (spdData[97] << 8) | spdData[98];
        _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleExtSerialNo[0] = numsetbits32(numCont) & 1 ? numCont : (numCont | 0x80);
        MemoryInfo->moduleExtSerialNo[1] = MemoryInfo->jedecID;
        MemoryInfo->moduleExtSerialNo[2] = MemoryInfo->moduleManufLoc;
        MemoryInfo->moduleExtSerialNo[3] = spdData[93];
        MemoryInfo->moduleExtSerialNo[4] = spdData[94];
        memcpy(&MemoryInfo->moduleExtSerialNo[5], &spdData[95], 4);

        // speed
#ifdef WIN32
        MemoryInfo->tCK = (spdData[9] >> 4) + (spdData[9] & 0xf) * 0.1f;
        ddrclk = 2 * (1000 / MemoryInfo->tCK);
        MemoryInfo->txWidth = (spdData[7] << 8) + spdData[6];
        if ((spdData[11] == 2) || (spdData[11] == 1))
        {
            MemoryInfo->txWidth = MemoryInfo->txWidth - 8;
        }
        pcclk = int(ddrclk * MemoryInfo->txWidth / 8);
        if ((pcclk % 100) >= 50) // Round properly
            pcclk += 100;
        pcclk = pcclk - (pcclk % 100);
        MemoryInfo->clkspeed = (1000 / MemoryInfo->tCK);
#else
        MemoryInfo->tCK = (spdData[9] >> 4) * 1000 + (spdData[9] & 0xf) * 100;
        ddrclk = 2 * (1000000 / MemoryInfo->tCK);
        MemoryInfo->txWidth = (spdData[7] << 8) + spdData[6];
        if ((spdData[11] == 2) || (spdData[11] == 1))
        {
            MemoryInfo->txWidth = MemoryInfo->txWidth - 8;
        }
        pcclk = (2 * 1000000 * MemoryInfo->txWidth) / (8 * MemoryInfo->tCK);
        if ((pcclk % 100) >= 50) // Round properly
            pcclk += 100;
        pcclk = pcclk - (pcclk % 100);
        MemoryInfo->clkspeed = (1000000 / MemoryInfo->tCK);
#endif
        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MT/s (PC%d)"), (int)ddrclk, pcclk);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("PC%d"), pcclk);
        wcscpy(MemoryInfo->szModulespeed, g_szSITempDebug1);

        // NOTE: Bank 2 may be different from bank 1
        MemoryInfo->numBanks = spdData[17];
        MemoryInfo->rowAddrBits = spdData[3] & 0x0f;
        MemoryInfo->colAddrBits = spdData[4] & 0x0f;
        MemoryInfo->busWidth = (spdData[7] << 8) | spdData[6];
        MemoryInfo->numRanks = spdData[5];

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"), spdData[17], spdData[3] & 0x0f, spdData[4] & 0x0f, (spdData[7] << 8) | spdData[6]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), spdData[5]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = spdData[13] & 0x7F;
        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d bits"), spdData[13] & 0x7F);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("SDRAM ECC/parity width: %d bits"), spdData[14] & 0x7F);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // size computation
        index = MemoryInfo->rowAddrBits + MemoryInfo->colAddrBits - 17;

        MemoryInfo->size = (1 << index) * MemoryInfo->numRanks * MemoryInfo->numBanks;
        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Bank density: "));
        for (i = 0; i < 8; i++)
        {
            if (spdData[31] & (1 << i))
            {
                _tcscpy(MemoryInfo->specific.DDR1SDRAM.bankDensity, BANK_DENSITY_TABLE[i]);
                _tcscat(g_szSITempDebug1, BANK_DENSITY_TABLE[i]);
                _tcscat(g_szSITempDebug1, _T(" "));
            }
        }
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // ecc
        if (spdData[11] == 0x2)
            MemoryInfo->ecc = true;
        else
            MemoryInfo->ecc = false;

        _stprintf_s(g_szSITempDebug1, _T("ECC: %s"), MemoryInfo->ecc ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _tcscpy(MemoryInfo->moduleVoltage, VOLTAGE_LEVEL_TABLE[spdData[8] & 0x0F]);
        _stprintf_s(g_szSITempDebug1, _T("Voltage Interface Level: %s"), MemoryInfo->moduleVoltage);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // refresh rate
        _tcscpy(MemoryInfo->specific.DDR1SDRAM.RefreshRate, REFRESH_RATE_TABLE[spdData[12] & 0x0F]);

        _stprintf_s(g_szSITempDebug1, _T("Refresh Rate: %s"), MemoryInfo->specific.DDR1SDRAM.RefreshRate);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // burst
        _stprintf_s(MemoryInfo->specific.DDR1SDRAM.BurstLengthsSupported, _T(""));

        for (i = 0x01; i <= 0x08; i = i << 1)
        {
            if (spdData[16] & i)
            {
                TCHAR tmpChar[8];
                _stprintf_s(tmpChar, _T("%d "), i);
                _tcscat(MemoryInfo->specific.DDR1SDRAM.BurstLengthsSupported, tmpChar);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported Burst Lengths: %s"), MemoryInfo->specific.DDR1SDRAM.BurstLengthsSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // CAS latencies supported
        _stprintf_s(MemoryInfo->CASSupported, _T(""));

#ifdef WIN32
        highestCAS = 0;

        for (i = 0; i < 7; i++)
        {
            if (spdData[18] & (1 << i))
            {
                TCHAR strCAS[8];
                highestCAS = 1 + (i * 0.5f);
                _stprintf_s(strCAS, _T("%.1f "), 1 + (i * 0.5));
                _tcscat(MemoryInfo->CASSupported, strCAS);
            }
        }
#else
        highestCAS = 0;

        for (i = 0; i < 7; i++)
        {
            if (spdData[18] & (1 << i))
            {
                TCHAR strCAS[8];
                highestCAS = 10 + (i * 5);
                _stprintf_s(strCAS, _T("%d.%01d "), highestCAS / 10, highestCAS % 10);
                _tcscat(MemoryInfo->CASSupported, strCAS);
            }
        }
#endif
        _stprintf_s(g_szSITempDebug1, _T("Supported CAS Latencies (tCL): %s"), MemoryInfo->CASSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        trcd = (spdData[29] >> 2) + ((spdData[29] & 3) * 0.25f);
        trp = (spdData[27] >> 2) + ((spdData[27] & 3) * 0.25f);
        tras = spdData[30];

        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz): %.1f-%d-%d-%d"),
                    MemoryInfo->clkspeed,
                    highestCAS,
                    (int)ceil(trcd / MemoryInfo->tCK),
                    (int)ceil(trp / MemoryInfo->tCK),
                    (int)ceil(tras / MemoryInfo->tCK));
#else
        trcd = (spdData[29] >> 2) * 1000 + ((spdData[29] & 3) * 250);
        trp = (spdData[27] >> 2) * 1000 + ((spdData[27] & 3) * 250);
        tras = spdData[30] * 1000;

        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz): %d-%d-%d-%d"),
                    MemoryInfo->clkspeed,
                    highestCAS / 10,
                    CEIL(trcd, MemoryInfo->tCK),
                    CEIL(trp, MemoryInfo->tCK),
                    CEIL(tras, MemoryInfo->tCK));
#endif

        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // latencies
        _stprintf_s(MemoryInfo->specific.DDR1SDRAM.CSSupported, _T(""));

        for (i = 0; i < 7; i++)
        {
            if (spdData[19] & (1 << i))
            {
                TCHAR strCS[8];
                _stprintf_s(strCS, _T("%d "), i);
                _tcscat(MemoryInfo->specific.DDR1SDRAM.CSSupported, strCS);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported CS Latencies: %s"), MemoryInfo->specific.DDR1SDRAM.CSSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR1SDRAM.WESupported, _T(""));

        for (i = 0; i < 7; i++)
        {
            if (spdData[20] & (1 << i))
            {
                TCHAR strWE[8];
                _stprintf_s(strWE, _T("%d "), i);
                _tcscat(MemoryInfo->specific.DDR1SDRAM.WESupported, strWE);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported WE Latencies: %s"), MemoryInfo->specific.DDR1SDRAM.WESupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Module feature bitmap
        MemoryInfo->specific.DDR1SDRAM.buffered = (spdData[21] & SPD_DDR_MODULE_ATTR_BUFFERED) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Buffered address and control inputs: %s"), spdData[21] & SPD_DDR_MODULE_ATTR_BUFFERED ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->registered = (spdData[21] & SPD_DDR_MODULE_ATTR_REGISTERED) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Registered address and control inputs: %s"), spdData[21] & SPD_DDR_MODULE_ATTR_REGISTERED ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR1SDRAM.OnCardPLL = (spdData[21] & SPD_DDR_MODULE_ATTR_ONCARD_PLL) != 0;
        _stprintf_s(g_szSITempDebug1, _T("On-Card PLL (Clock): %s"), spdData[21] & SPD_DDR_MODULE_ATTR_ONCARD_PLL ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR1SDRAM.FETOnCardEnable = (spdData[21] & SPD_DDR_MODULE_ATTR_FET_BOARD_EN) != 0;
        _stprintf_s(g_szSITempDebug1, _T("FET switch on-card enable: %s"), spdData[21] & SPD_DDR_MODULE_ATTR_FET_BOARD_EN ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR1SDRAM.FETExtEnable = (spdData[21] & SPD_DDR_MODULE_ATTR_FET_EXT_EN) != 0;
        _stprintf_s(g_szSITempDebug1, _T("FET switch external enable: %s"), spdData[21] & SPD_DDR_MODULE_ATTR_FET_EXT_EN ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR1SDRAM.DiffClockInput = (spdData[21] & SPD_DDR_MODULE_ATTR_DIFF_CLOCK) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Differential clock input: %s"), spdData[21] & SPD_DDR_MODULE_ATTR_DIFF_CLOCK ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Chip feature bitmap
        MemoryInfo->specific.DDR1SDRAM.WeakDriverIncluded = (spdData[22] & SPD_DDR_CHIP_ATTR_WEAK_DRIVER) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Includes weak driver: %s"), spdData[22] & SPD_DDR_CHIP_ATTR_WEAK_DRIVER ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR1SDRAM.ConcAutoPrecharge = (spdData[22] & SPD_DDR_CHIP_ATTR_CONC_AUTO_PRECH) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports Concurrent Auto Precharge: %s"), spdData[22] & SPD_DDR_CHIP_ATTR_CONC_AUTO_PRECH ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR1SDRAM.FastAPSupported = (spdData[22] & SPD_DDR_CHIP_ATTR_FAST_AP) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports Fast AP: %s"), spdData[22] & SPD_DDR_CHIP_ATTR_FAST_AP ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // timing
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at highest CAS latency: "), MemoryInfo->tCK);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tAC = (spdData[10] >> 4) * 0.1f + (spdData[10] & 0xf) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tAC = (spdData[10] >> 4) * 100 + (spdData[10] & 0xf) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at highest CAS latency: "), MemoryInfo->specific.DDR1SDRAM.tAC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tCKmed = (spdData[23] >> 4) + (spdData[23] & 0xf) * 0.1f;
#else
        MemoryInfo->specific.DDR1SDRAM.tCKmed = (spdData[23] >> 4) * 1000 + (spdData[23] & 0xf) * 100;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at medium CAS latency: "), MemoryInfo->specific.DDR1SDRAM.tCKmed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tACmed = (spdData[24] >> 4) * 0.1f + (spdData[24] & 0xf) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tACmed = (spdData[24] >> 4) * 100 + (spdData[24] & 0xf) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at medium CAS latency: "), MemoryInfo->specific.DDR1SDRAM.tACmed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tCKshort = (spdData[25] >> 4) + (spdData[25] & 0xf) * 0.1f;
#else
        MemoryInfo->specific.DDR1SDRAM.tCKshort = (spdData[25] >> 4) * 1000 + (spdData[25] & 0xf) * 100;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at shortest CAS latency: "), MemoryInfo->specific.DDR1SDRAM.tCKshort);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tACshort = (spdData[26] >> 4) * 0.1f + (spdData[26] & 0xf) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tACshort = (spdData[26] >> 4) * 100 + (spdData[26] & 0xf) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at shortest CAS latency: "), MemoryInfo->specific.DDR1SDRAM.tACshort);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tCKmax = (spdData[43] >> 2) + ((spdData[43] & 0x02) * 0.25f);
#else
        MemoryInfo->specific.DDR1SDRAM.tCKmax = (spdData[43] >> 2) * 1000 + ((spdData[43] & 0x02) * 250);
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum Cycle Time (tCK max): "), MemoryInfo->specific.DDR1SDRAM.tCKmax);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // more timing information
#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tIS = (spdData[32] >> 4) * 0.1f + (spdData[32] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tIS = (spdData[32] >> 4) * 100 + (spdData[32] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Address/Command Setup Time Before Clock (tIS): "), MemoryInfo->specific.DDR1SDRAM.tIS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tIH = (spdData[33] >> 4) * 0.1f + (spdData[33] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tIH = (spdData[33] >> 4) * 100 + (spdData[33] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Address/Command Hold Time After Clock (tIH): "), MemoryInfo->specific.DDR1SDRAM.tIH);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tDS = (spdData[34] >> 4) * 0.1f + (spdData[34] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tDS = (spdData[34] >> 4) * 100 + (spdData[34] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data Input Setup Time Before Strobe (tDS): "), MemoryInfo->specific.DDR1SDRAM.tDS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tDH = (spdData[35] >> 4) * 0.1f + (spdData[35] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tDH = (spdData[35] >> 4) * 100 + (spdData[35] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data Input Hold Time After Strobe (tDH): "), MemoryInfo->specific.DDR1SDRAM.tDH);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->tAA = highestCAS * MemoryInfo->tCK;
#else
        MemoryInfo->tAA = (highestCAS * MemoryInfo->tCK) / 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = trp;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Precharge Delay (tRP): "), MemoryInfo->tRP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->tRRD = (spdData[28] >> 2) + ((spdData[28] & 3) * 0.25f);
#else
        MemoryInfo->tRRD = (spdData[28] >> 2) * 1000 + ((spdData[28] & 3) * 250);
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Active to Row Active Delay (tRRD): "), MemoryInfo->tRRD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = trcd;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS# to CAS# Delay (tRCD): "), MemoryInfo->tRCD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRAS = tras;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS# Pulse Width (tRAS): "), MemoryInfo->tRAS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRC = TO_TIMING_VAR(spdData[41]);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Active to Auto-refresh Delay (tRC): "), MemoryInfo->tRC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRFC = TO_TIMING_VAR(spdData[42]);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC): "), MemoryInfo->tRFC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tDQSQ = spdData[44] * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tDQSQ = spdData[44] * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum DQS to DQ Skew (tDQSQ): "), MemoryInfo->specific.DDR1SDRAM.tDQSQ);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR1SDRAM.tQHS = (spdData[45] >> 4) * 0.1f + (spdData[45] & 0x0F) * 0.01f;
#else
        MemoryInfo->specific.DDR1SDRAM.tQHS = (spdData[45] >> 4) * 100 + (spdData[45] & 0x0F) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum Read Data Hold Skew (tQHS): "), MemoryInfo->specific.DDR1SDRAM.tQHS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // module attributes
        if ((spdData[47] & 0x03) == 0x01)
        {
            _stprintf_s(MemoryInfo->specific.DDR1SDRAM.moduleHeight, _T("1.125\" to 1.25\""));
        }
        else if ((spdData[47] & 0x03) == 0x02)
        {
            _stprintf_s(MemoryInfo->specific.DDR1SDRAM.moduleHeight, _T("1.7\""));
        }
        else if ((spdData[47] & 0x03) == 0x03)
        {
            _stprintf_s(MemoryInfo->specific.DDR1SDRAM.moduleHeight, _T("Other"));
        }
        else
        {
            _stprintf_s(MemoryInfo->specific.DDR1SDRAM.moduleHeight, _T("Unavailable"));
        }
        _stprintf_s(g_szSITempDebug1, _T("Module Height: %s"), MemoryInfo->specific.DDR1SDRAM.moduleHeight);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
    }
    break;
    case DDR210:
    case DDR212:
    {
#ifdef WIN32
        float ddrclk;
        float trcd;
        float trp;
        float tras;
#else
        int ddrclk;
        int trcd;
        int trp;
        int tras;
#endif
        int highestCAS;
        int pcclk;
        int index;
        unsigned char numCont = 0;
        // Manufacturing information
        // JEDEC ID
        // get the JedecID Bytes 64-71, shift each one by the right amount eg 0 for 1st part, 8 for second part
        // as ID is stored in 8 parts
        MemoryInfo->jedecID = 0;
        for (i = 64; i <= 71; i++)
        {
            if (spdData[i] == 0x7F)
                numCont++;
            else
            {
                MemoryInfo->jedecID = spdData[i];
                break;
            }
        }
        MemoryInfo->jedecBank = numCont + 1;
        GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
        _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleManufLoc = spdData[72];
        _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        for (i = 0; i < SPD_DDR2_MODULE_PARTNO_LEN; i++)
        {
            if (spdData[73 + i] >= 0 && spdData[73 + i] < 128) // Sanity check for ASCII char
                MemoryInfo->modulePartNo[i] = spdData[73 + i];
            else
                break;
        }

#ifdef WIN32
        TCHAR wcPartNo[SPD_DDR2_MODULE_PARTNO_LEN + 1];
        memset(wcPartNo, 0, sizeof(wcPartNo));
        MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_DDR2_MODULE_PARTNO_LEN, wcPartNo, SPD_DDR2_MODULE_PARTNO_LEN + 1);
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleRev = (spdData[91] << 8) | spdData[92];
        _stprintf_s(g_szSITempDebug1, _T("Revision Code: 0x%04X"), MemoryInfo->moduleRev);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Get the date code for the memory, bytes 93 and 94
        // Interepret the stored value based on the SPD ver
        // Convert from BCD
        if (spdData[93] > 0) // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->year = 2000 + ((spdData[93] >> 4) * 10) + (spdData[93] & 0x0F);
        if (spdData[94] > 0) // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->week = ((spdData[94] >> 4) * 10) + (spdData[94] & 0x0F);

        _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleSerialNo = (spdData[95] << 24) | (spdData[96] << 16) | (spdData[97] << 8) | spdData[98];
        _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleExtSerialNo[0] = numsetbits32(numCont) & 1 ? numCont : (numCont | 0x80);
        MemoryInfo->moduleExtSerialNo[1] = MemoryInfo->jedecID;
        MemoryInfo->moduleExtSerialNo[2] = MemoryInfo->moduleManufLoc;
        MemoryInfo->moduleExtSerialNo[3] = spdData[93];
        MemoryInfo->moduleExtSerialNo[4] = spdData[94];
        memcpy(&MemoryInfo->moduleExtSerialNo[5], &spdData[95], 4);

        // speed
#ifdef WIN32
        MemoryInfo->tCK = (float)(spdData[9] >> 4);

        if ((spdData[9] & 0x0f) <= 9)
        {
            MemoryInfo->tCK += (spdData[9] & 0xf) * 0.1f;
        }
        else if ((spdData[9] & 0x0f) == 10)
        {
            MemoryInfo->tCK += 0.25f;
        }
        else if ((spdData[9] & 0x0f) == 11)
        {
            MemoryInfo->tCK += 0.33f;
        }
        else if ((spdData[9] & 0x0f) == 12)
        {
            MemoryInfo->tCK += 0.66f;
        }
        else if ((spdData[9] & 0x0f) == 13)
        {
            MemoryInfo->tCK += 0.75f;
        }

        ddrclk = 2 * (1000 / MemoryInfo->tCK);

        MemoryInfo->txWidth = (spdData[7] << 8) + spdData[6];
        if (spdData[11] & 0x03)
        {
            MemoryInfo->txWidth = MemoryInfo->txWidth - 8;
        }
        pcclk = int(ddrclk * MemoryInfo->txWidth / 8);
        // Round down to comply with Jedec
        pcclk = pcclk - (pcclk % 100);
        MemoryInfo->clkspeed = (1000 / MemoryInfo->tCK);

#else
        MemoryInfo->tCK = (spdData[9] >> 4) * 1000;

        if ((spdData[9] & 0x0f) <= 9)
        {
            MemoryInfo->tCK += (spdData[9] & 0xf) * 100;
        }
        else if ((spdData[9] & 0x0f) == 10)
        {
            MemoryInfo->tCK += 250;
        }
        else if ((spdData[9] & 0x0f) == 11)
        {
            MemoryInfo->tCK += 330;
        }
        else if ((spdData[9] & 0x0f) == 12)
        {
            MemoryInfo->tCK += 660;
        }
        else if ((spdData[9] & 0x0f) == 13)
        {
            MemoryInfo->tCK += 750;
        }

        ddrclk = 2 * (1000000 / MemoryInfo->tCK);

        MemoryInfo->txWidth = (spdData[7] << 8) + spdData[6];
        if (spdData[11] & 0x03)
        {
            MemoryInfo->txWidth = MemoryInfo->txWidth - 8;
        }
        pcclk = (2 * 1000000 * MemoryInfo->txWidth) / (8 * MemoryInfo->tCK);
        // Round down to comply with Jedec
        pcclk = pcclk - (pcclk % 100);
        MemoryInfo->clkspeed = (1000000 / MemoryInfo->tCK);
#endif
        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MT/s (PC2-%d)"), (int)ddrclk, pcclk);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("PC2-%d"), pcclk);
        wcscpy(MemoryInfo->szModulespeed, g_szSITempDebug1);

        MemoryInfo->numBanks = spdData[17];
        MemoryInfo->rowAddrBits = spdData[3] & 0x0f;
        MemoryInfo->colAddrBits = spdData[4] & 0x0f;
        MemoryInfo->busWidth = spdData[6];
        MemoryInfo->numRanks = (spdData[5] & 0x07) + 1;

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"), spdData[17], spdData[3], spdData[4], spdData[6]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), (spdData[5] & 0x07) + 1);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = spdData[13];
        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d bits"), spdData[13]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // size computation
        index = MemoryInfo->rowAddrBits + MemoryInfo->colAddrBits - 17;

        MemoryInfo->size = (1 << index) * MemoryInfo->numRanks * MemoryInfo->numBanks;
        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // ecc
        if (spdData[11] == 0x2)
            MemoryInfo->ecc = true;
        else
            MemoryInfo->ecc = false;

        _stprintf_s(g_szSITempDebug1, _T("ECC: %s"), MemoryInfo->ecc ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _tcscpy(MemoryInfo->specific.DDR2SDRAM.moduleHeight, DDR2_MODULE_HEIGHT_TABLE[(spdData[5] >> 5) & 0x07]);
        _stprintf_s(g_szSITempDebug1, _T("Module Height: %s mm"), MemoryInfo->specific.DDR2SDRAM.moduleHeight);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.moduleType, _T(""));

        // special cases
        if ((spdData[20] & 0x3F) == 0x06)
        {
            _stprintf_s(MemoryInfo->specific.DDR2SDRAM.moduleType, _T("72b-SO-CDIMM"));
            MemoryInfo->registered = false;
        }
        else if ((spdData[20] & 0x3F) == 0x07)
        {
            _stprintf_s(MemoryInfo->specific.DDR2SDRAM.moduleType, _T("72b-SO-RDIMM"));
            MemoryInfo->registered = true;
        }
        else
        {
            for (i = 0; i <= 5; i++)
            {
                if (spdData[20] & (1 << i))
                {
                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.moduleType, _T("%s"), DDR2_MODULE_TYPES_TABLE[i]);

                    if (i == 0)
                        MemoryInfo->registered = true;
                    else if (i == 4)
                        MemoryInfo->registered = true;
                    else
                        MemoryInfo->registered = false;

                    break;
                }
            }
        }

        _stprintf_s(g_szSITempDebug1, _T("Module Type: %s"), MemoryInfo->specific.DDR2SDRAM.moduleType);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Module feature bitmap
        MemoryInfo->specific.DDR2SDRAM.numPLLs = SPD_DDR2_MODULE_ATTR_NUM_PLLS(spdData[21]);
        _stprintf_s(g_szSITempDebug1, _T("Number of PLLs: %d"), SPD_DDR2_MODULE_ATTR_NUM_PLLS(spdData[21]));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2SDRAM.FETExtEnable = (spdData[21] & SPD_DDR2_MODULE_ATTR_FET_EXT_EN) != 0;
        _stprintf_s(g_szSITempDebug1, _T("FET switch external enable: %s"), spdData[21] & SPD_DDR2_MODULE_ATTR_FET_EXT_EN ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2SDRAM.AnalysisProbeInstalled = (spdData[21] & SPD_DDR2_MODULE_ATTR_ANALYSIS_PROBE) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Analysis probe installed: %s"), spdData[21] & SPD_DDR2_MODULE_ATTR_ANALYSIS_PROBE ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Chip feature bitmap
        MemoryInfo->specific.DDR2SDRAM.WeakDriverSupported = (spdData[22] & SPD_DDR2_CHIP_ATTR_WEAK_DRIVER) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports weak driver: %s"), spdData[22] & SPD_DDR2_CHIP_ATTR_WEAK_DRIVER ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2SDRAM._50ohmODTSupported = (spdData[22] & SPD_DDR2_CHIP_ATTR_50_OHM_ODT) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports 50 ohm ODT: %s"), spdData[22] & SPD_DDR2_CHIP_ATTR_50_OHM_ODT ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2SDRAM.PASRSupported = (spdData[22] & SPD_DDR2_CHIP_ATTR_PASR) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Supports Partial Array Self Refresh: %s"), spdData[22] & SPD_DDR2_CHIP_ATTR_PASR ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _tcscpy(MemoryInfo->specific.DDR2SDRAM.DRAMPackage, spdData[5] & 0x10 ? _T("Stack") : _T("Planar"));
        _stprintf_s(g_szSITempDebug1, _T("DRAM Package: %s"), MemoryInfo->specific.DDR2SDRAM.DRAMPackage);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _tcscpy(MemoryInfo->moduleVoltage, VOLTAGE_LEVEL_TABLE[spdData[8] & 0x0F]);
        _stprintf_s(g_szSITempDebug1, _T("Voltage Interface Level: %s"), MemoryInfo->moduleVoltage);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // refresh rate
        _tcscpy(MemoryInfo->specific.DDR2SDRAM.RefreshRate, REFRESH_RATE_TABLE[spdData[12] & 0x0F]);

        _stprintf_s(g_szSITempDebug1, _T("Refresh Rate: %s"), MemoryInfo->specific.DDR2SDRAM.RefreshRate);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // burst
        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.BurstLengthsSupported, _T("%s%s"), spdData[16] & 0x04 ? _T("4 ") : _T(""), spdData[16] & 0x08 ? _T("8") : _T(""));
        _stprintf_s(g_szSITempDebug1, _T("Supported Burst Lengths: %s"), MemoryInfo->specific.DDR2SDRAM.BurstLengthsSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // CAS latencies supported
        _stprintf_s(MemoryInfo->CASSupported, _T(""));

        highestCAS = 0;

        for (i = 2; i < 7; i++)
        {
            if (spdData[18] & (1 << i))
            {
                TCHAR strCAS[8];
                highestCAS = i;
                _stprintf_s(strCAS, _T("%d "), i);
                _tcscat(MemoryInfo->CASSupported, strCAS);
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported CAS Latencies (tCL): %s"), MemoryInfo->CASSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        trcd = (spdData[29] >> 2) + ((spdData[29] & 3) * 0.25f);
        trp = (spdData[27] >> 2) + ((spdData[27] & 3) * 0.25f);
        tras = spdData[30];

        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz): %d-%d-%d-%d"),
                    (1000 / MemoryInfo->tCK),
                    highestCAS,
                    (int)ceil(trcd / MemoryInfo->tCK),
                    (int)ceil(trp / MemoryInfo->tCK),
                    (int)ceil(tras / MemoryInfo->tCK));
#else
        trcd = (spdData[29] >> 2) * 1000 + ((spdData[29] & 3) * 250);
        trp = (spdData[27] >> 2) * 1000 + ((spdData[27] & 3) * 250);
        tras = spdData[30] * 1000;

        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz): %d-%d-%d-%d"),
                    (1000000 / MemoryInfo->tCK),
                    highestCAS,
                    CEIL(trcd, MemoryInfo->tCK),
                    CEIL(trp, MemoryInfo->tCK),
                    CEIL(tras, MemoryInfo->tCK));
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // timings

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at highest CAS latency: "), MemoryInfo->tCK);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tAC = (spdData[10] >> 4) * 0.1f + (spdData[10] & 0xf) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tAC = (spdData[10] >> 4) * 100 + (spdData[10] & 0xf) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at highest CAS latency: "), MemoryInfo->specific.DDR2SDRAM.tAC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tCKmed = (float)(spdData[23] >> 4);
        if ((spdData[23] & 0x0f) <= 9)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += (spdData[23] & 0xf) * 0.1f;
        }
        else if ((spdData[23] & 0xf) == 10)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 0.25f;
        }
        else if ((spdData[23] & 0xf) == 11)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 0.33f;
        }
        else if ((spdData[23] & 0xf) == 12)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 0.66f;
        }
        else if ((spdData[23] & 0xf) == 13)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 0.75f;
        }
#else
        MemoryInfo->specific.DDR2SDRAM.tCKmed = (spdData[23] >> 4) * 1000;
        if ((spdData[23] & 0x0f) <= 9)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += (spdData[23] & 0xf) * 100;
        }
        else if ((spdData[23] & 0xf) == 10)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 250;
        }
        else if ((spdData[23] & 0xf) == 11)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 330;
        }
        else if ((spdData[23] & 0xf) == 12)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 660;
        }
        else if ((spdData[23] & 0xf) == 13)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmed += 750;
        }
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at medium CAS latency: "), MemoryInfo->specific.DDR2SDRAM.tCKmed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tACmed = (spdData[24] >> 4) * 0.1f + (spdData[24] & 0xf) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tACmed = (spdData[24] >> 4) * 100 + (spdData[24] & 0xf) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at medium CAS latency: "), MemoryInfo->specific.DDR2SDRAM.tACmed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tCKshort = (float)(spdData[25] >> 4);
        if ((spdData[25] & 0x0f) <= 9)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += (spdData[25] & 0xf) * 0.1f;
        }
        else if ((spdData[25] & 0xf) == 10)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 0.25f;
        }
        else if ((spdData[25] & 0xf) == 11)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 0.33f;
        }
        else if ((spdData[25] & 0xf) == 12)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 0.66f;
        }
        else if ((spdData[25] & 0xf) == 13)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 0.75f;
        }
#else
        MemoryInfo->specific.DDR2SDRAM.tCKshort = (spdData[25] >> 4) * 1000;
        if ((spdData[25] & 0x0f) <= 9)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += (spdData[25] & 0xf) * 100;
        }
        else if ((spdData[25] & 0xf) == 10)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 250;
        }
        else if ((spdData[25] & 0xf) == 11)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 330;
        }
        else if ((spdData[25] & 0xf) == 12)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 660;
        }
        else if ((spdData[25] & 0xf) == 13)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKshort += 750;
        }
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Clock cycle time at shortest CAS latency: "), MemoryInfo->specific.DDR2SDRAM.tCKshort);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tACshort = (spdData[26] >> 4) * 0.1f + (spdData[26] & 0xf) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tACshort = (spdData[26] >> 4) * 100 + (spdData[26] & 0xf) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data access time at shortest CAS latency: "), MemoryInfo->specific.DDR2SDRAM.tACshort);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tCKmax = (float)(spdData[43] >> 4);
        if ((spdData[43] & 0x0f) <= 9)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += (spdData[43] & 0xf) * 0.1f;
        }
        else if ((spdData[43] & 0xf) == 10)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 0.25f;
        }
        else if ((spdData[43] & 0xf) == 11)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 0.33f;
        }
        else if ((spdData[43] & 0xf) == 12)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 0.66f;
        }
        else if ((spdData[43] & 0xf) == 13)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 0.75f;
        }
#else
        MemoryInfo->specific.DDR2SDRAM.tCKmax = (spdData[43] >> 4) * 1000;
        if ((spdData[43] & 0x0f) <= 9)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += (spdData[43] & 0xf) * 100;
        }
        else if ((spdData[43] & 0xf) == 10)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 250;
        }
        else if ((spdData[43] & 0xf) == 11)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 330;
        }
        else if ((spdData[43] & 0xf) == 12)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 660;
        }
        else if ((spdData[43] & 0xf) == 13)
        {
            MemoryInfo->specific.DDR2SDRAM.tCKmax += 750;
        }
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum Cycle Time (tCK max): "), MemoryInfo->specific.DDR2SDRAM.tCKmax);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // more timing information
#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tIS = (spdData[32] >> 4) * 0.1f + (spdData[32] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tIS = (spdData[32] >> 4) * 100 + (spdData[32] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Address/Command Setup Time Before Clock (tIS): "), MemoryInfo->specific.DDR2SDRAM.tIS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tIH = (spdData[33] >> 4) * 0.1f + (spdData[33] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tIH = (spdData[33] >> 4) * 100 + (spdData[33] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Address/Command Hold Time After Clock (tIH): "), MemoryInfo->specific.DDR2SDRAM.tIH);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tDS = (spdData[34] >> 4) * 0.1f + (spdData[34] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tDS = (spdData[34] >> 4) * 100 + (spdData[34] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data Input Setup Time Before Strobe (tDS): "), MemoryInfo->specific.DDR2SDRAM.tDS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tDH = (spdData[35] >> 4) * 0.1f + (spdData[35] & 0x0f) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tDH = (spdData[35] >> 4) * 100 + (spdData[35] & 0x0f) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Data Input Hold Time After Strobe (tDH): "), MemoryInfo->specific.DDR2SDRAM.tDH);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tAA = highestCAS * MemoryInfo->tCK;

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = trp;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Precharge Delay (tRP): "), MemoryInfo->tRP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->tRRD = (spdData[28] >> 2) + ((spdData[28] & 3) * 0.25f);
#else
        MemoryInfo->tRRD = (spdData[28] >> 2) * 1000 + ((spdData[28] & 3) * 250);
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Active to Row Active Delay (tRRD): "), MemoryInfo->tRRD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = trcd;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS# to CAS# Delay (tRCD): "), MemoryInfo->tRCD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRAS = tras;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS# Pulse Width (tRAS): "), MemoryInfo->tRAS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tWR = (spdData[36] >> 2) + ((spdData[36] & 3) * 0.25f);
#else
        MemoryInfo->specific.DDR2SDRAM.tWR = (spdData[36] >> 2) * 1000 + ((spdData[36] & 3) * 250);
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Write Recovery Time (tWR): "), MemoryInfo->specific.DDR2SDRAM.tWR);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tWTR = (spdData[37] >> 2) + ((spdData[37] & 3) * 0.25f);
#else
        MemoryInfo->specific.DDR2SDRAM.tWTR = (spdData[37] >> 2) * 1000 + ((spdData[37] & 3) * 250);
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Write to Read CMD Delay (tWTR): "), MemoryInfo->specific.DDR2SDRAM.tWTR);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tRTP = (spdData[38] >> 2) + ((spdData[38] & 3) * 0.25f);
#else
        MemoryInfo->specific.DDR2SDRAM.tRTP = (spdData[38] >> 2) * 1000 + ((spdData[38] & 3) * 250);
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Read to Pre-charge CMD Delay (tRTP): "), MemoryInfo->specific.DDR2SDRAM.tRTP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRC = FRACTIONAL_NS_TABLE[(spdData[40] >> 4) & 0x07] + TO_TIMING_VAR(spdData[41]);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Active to Auto-refresh Delay (tRC): "), MemoryInfo->tRC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRFC = FRACTIONAL_NS_TABLE[(spdData[40] >> 1) & 0x07] + TO_TIMING_VAR(spdData[42]);
#ifdef WIN32
        if (spdData[40] & 1)
            MemoryInfo->tRFC += 256;
#else
        if (spdData[40] & 1)
            MemoryInfo->tRFC += 256000;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC): "), MemoryInfo->tRFC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tDQSQ = spdData[44] * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tDQSQ = spdData[44] * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum DQS to DQ Skew (tDQSQ): "), MemoryInfo->specific.DDR2SDRAM.tDQSQ);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
        MemoryInfo->specific.DDR2SDRAM.tQHS = (spdData[45] >> 4) * 0.1f + (spdData[45] & 0x0F) * 0.01f;
#else
        MemoryInfo->specific.DDR2SDRAM.tQHS = (spdData[45] >> 4) * 100 + (spdData[45] & 0x0F) * 10;
#endif
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum Read Data Hold Skew (tQHS): "), MemoryInfo->specific.DDR2SDRAM.tQHS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2SDRAM.tPLLRelock = TO_TIMING_VAR(spdData[46]);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("PLL Relock Time: "), MemoryInfo->specific.DDR2SDRAM.tPLLRelock);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // EPP
        if (spdData[99] == SPD_DDR2_EPP_ID_1 && spdData[100] == SPD_DDR2_EPP_ID_2 && spdData[101] == SPD_DDR2_EPP_ID_3)
        {
            MemoryInfo->specific.DDR2SDRAM.EPPSupported = true;
            _stprintf_s(g_szSITempDebug1, _T("EPP supported: Yes"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR2SDRAM.EPP.profileType = spdData[102];

            if (MemoryInfo->specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_ABBR)
            {
                _stprintf_s(g_szSITempDebug1, _T("EPP Profile Type: Abbreviated Profiles"));
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR2SDRAM.EPP.optimalProfile = spdData[103] & 0x3;
                _stprintf_s(g_szSITempDebug1, _T("EPP optimal profile: Profile AP%d"), MemoryInfo->specific.DDR2SDRAM.EPP.optimalProfile);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                for (i = 0; i < 4; i++)
                {
                    int j;

                    _stprintf_s(g_szSITempDebug1, _T("[EPP profile AP%d]"), i);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if (spdData[103] & (1 << (i + 4)))
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].enabled = true;
                    _stprintf_s(g_szSITempDebug1, _T("  EPP profile enabled: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].enabled ? _T("Yes") : _T("No"));
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].voltageLevel, _T("%.1fV"), 1.8f + ((spdData[104 + (i * 6)] & 0x7F) * 0.025f));
#else
                    {
                        int millivolts = 1800 + ((spdData[104 + (i * 6)] & 0x7F) * 25);
                        getVoltageStr(millivolts, MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].voltageLevel, sizeof(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].voltageLevel));
                    }
#endif
                    _stprintf_s(g_szSITempDebug1, _T("  Voltage Level: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].voltageLevel);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].cmdRate = ((spdData[104 + (i * 6)] >> 7) & 0x01) + 1;
                    _stprintf_s(g_szSITempDebug1, _T("  Command Rate: %dT"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].cmdRate);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK = (float)(spdData[105 + (i * 6)] >> 4);

                    if ((spdData[105 + (i * 6)] & 0x0f) <= 9)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += (spdData[105 + (i * 6)] & 0xf) * 0.1f;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 10)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 0.25f;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 11)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 0.33f;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 12)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 0.66f;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 13)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 0.75f;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 14)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 0.875f;
                    }
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK = (spdData[105 + (i * 6)] >> 4) * 1000;

                    if ((spdData[105 + (i * 6)] & 0x0f) <= 9)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += (spdData[105 + (i * 6)] & 0xf) * 100;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 10)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 250;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 11)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 330;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 12)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 660;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 13)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 750;
                    }
                    else if ((spdData[105 + (i * 6)] & 0x0f) == 14)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK += 875;
                    }

#endif

                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Cycle Time (tCK): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].clkspeed = TO_TIMING_VAR(1000 / MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tCK);
                    _stprintf_s(g_szSITempDebug1, _T("  Maximum clock speed: %d MHz"), (int)MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].clkspeed);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    // CAS latencies supported
                    for (j = 2; j < 7; j++)
                    {
                        if (spdData[106 + (i * 6)] & (1 << j))
                        {
                            MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].CASSupported = j;
                            break;
                        }
                    }
                    _stprintf_s(g_szSITempDebug1, _T("  Supported CAS Latency: %d"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].CASSupported);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRCD = (spdData[107 + (i * 6)] >> 2) + ((spdData[107 + (i * 6)] & 3) * 0.25f);
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRCD = (spdData[107 + (i * 6)] >> 2) * 1000 + ((spdData[107 + (i * 6)] & 3) * 250);
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum RAS to CAS delay (tRCD): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRCD);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRP = (spdData[108 + (i * 6)] >> 2) + ((spdData[108 + (i * 6)] & 3) * 0.25f);
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRP = (spdData[108 + (i * 6)] >> 2) * 1000 + ((spdData[108 + (i * 6)] & 3) * 250);
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Row Precharge Time (tRP): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRP);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRAS = spdData[109 + (i * 6)];
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRAS = spdData[109 + (i * 6)] * 1000;
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Precharge Time (tRAS): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.abbrProfile[i].tRAS);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);
                }
            }
            else if (MemoryInfo->specific.DDR2SDRAM.EPP.profileType == SPDINFO_DDR2_EPP_PROFILE_FULL)
            {
                _stprintf_s(g_szSITempDebug1, _T("EPP Profile Type: Full Profiles"));
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR2SDRAM.EPP.optimalProfile = spdData[103] & 0x3;
                _stprintf_s(g_szSITempDebug1, _T("EPP optimal profile: Profile FP%d"), MemoryInfo->specific.DDR2SDRAM.EPP.optimalProfile);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                for (i = 0; i < 2; i++)
                {
                    int j;

                    _stprintf_s(g_szSITempDebug1, _T("[EPP profile FP%d]"), i);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if (spdData[103] & (1 << (i + 4)))
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].enabled = true;
                    _stprintf_s(g_szSITempDebug1, _T("  EPP profile enabled: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].enabled ? _T("Yes") : _T("No"));
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].voltageLevel, _T("%.1fV"), 1.8f + ((spdData[104 + (i * 12)] & 0x7F) * 0.025f));
#else
                    {
                        int millivolts = 1800 + ((spdData[104 + (i * 12)] & 0x7F) * 25);
                        getVoltageStr(millivolts, MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].voltageLevel, sizeof(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].voltageLevel));
                    }
#endif
                    _stprintf_s(g_szSITempDebug1, _T("  Voltage Level: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].voltageLevel);

                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].cmdRate = ((spdData[104 + (i * 12)] >> 7) & 0x01) + 1;
                    _stprintf_s(g_szSITempDebug1, _T("  Command Rate: %dT"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].cmdRate);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if ((spdData[105 + (i * 12)] & 0x3) == 0)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrDriveStrength, _T("1.0x"));
                    else if ((spdData[105 + (i * 12)] & 0x3) == 1)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrDriveStrength, _T("1.25x"));
                    else if ((spdData[105 + (i * 12)] & 0x3) == 2)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrDriveStrength, _T("1.5x"));
                    else if ((spdData[105 + (i * 12)] & 0x3) == 3)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrDriveStrength, _T("2.0x"));
                    else
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrDriveStrength, _T("N/A"));

                    _stprintf_s(g_szSITempDebug1, _T("  Address Drive Strength: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrDriveStrength);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if (((spdData[105 + (i * 12)] >> 2) & 0x3) == 0)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSDriveStrength, _T("1.0x"));
                    else if (((spdData[105 + (i * 12)] >> 2) & 0x3) == 1)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSDriveStrength, _T("1.25x"));
                    else if (((spdData[105 + (i * 12)] >> 2) & 0x3) == 2)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSDriveStrength, _T("1.5x"));
                    else if (((spdData[105 + (i * 12)] >> 2) & 0x3) == 3)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSDriveStrength, _T("2.0x"));
                    else
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSDriveStrength, _T("N/A"));

                    _stprintf_s(g_szSITempDebug1, _T("  Chip Select Drive Strength: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSDriveStrength);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].clockDriveStrength, _T("%.2fx"), 0.75f + (((spdData[105 + (i * 12)] >> 4) & 0x3) * 0.25f));
#else
                    {
                        int clkdrive = 750 + (((spdData[105 + (i * 12)] >> 4) & 0x3) * 250);
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].clockDriveStrength, _T("%d.%03dx"), clkdrive / 1000, clkdrive % 1000);
                    }
#endif

                    _stprintf_s(g_szSITempDebug1, _T("  Clock Drive Strength: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].clockDriveStrength);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].dataDriveStrength, _T("%.2fx"), 0.75f + (((spdData[105 + (i * 12)] >> 6) & 0x3) * 0.25f));
#else
                    {
                        int datadrive = 750 + (((spdData[105 + (i * 12)] >> 6) & 0x3) * 250);
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].dataDriveStrength, _T("%d.%03dx"), datadrive / 1000, datadrive % 1000);
                    }
#endif
                    _stprintf_s(g_szSITempDebug1, _T("  Data Drive Strength: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].dataDriveStrength);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].DQSDriveStrength, _T("%.2fx"), 0.75f + ((spdData[106 + (i * 12)] & 0x3) * 0.25f));
#else
                    {
                        int DQSdrive = 750 + ((spdData[106 + (i * 12)] & 0x3) * 250);
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].DQSDriveStrength, _T("%d.%03dx"), DQSdrive / 1000, DQSdrive % 1000);
                    }
#endif
                    _stprintf_s(g_szSITempDebug1, _T("  DQS drive strength: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].DQSDriveStrength);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if ((spdData[107 + (i * 12)] & 0x1F) == 0)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrCmdFineDelay, _T("0 MEMCLK"));
                    else
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrCmdFineDelay, _T("%d/64 MEMCLK"), spdData[107 + (i * 12)] & 0x1F);
                    _stprintf_s(g_szSITempDebug1, _T("  Address/Command fine delay: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrCmdFineDelay);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrCmdSetupTime, _T("%s"), (spdData[107 + (i * 12)] & 0x20) == 0 ? _T("1/2 MEMCLK") : _T("1 MEMCLK"));
                    _stprintf_s(g_szSITempDebug1, _T("  Address/Command setup time: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].addrCmdSetupTime);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if ((spdData[108 + (i * 12)] & 0x1F) == 0)
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSFineDelay, _T("0 MEMCLK"));
                    else
                        _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSFineDelay, _T("%d/64 MEMCLK"), spdData[108 + (i * 12)] & 0x1F);
                    _stprintf_s(g_szSITempDebug1, _T("  Chip Select fine delay: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSFineDelay);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    _stprintf_s(MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSSetupTime, _T("%s"), (spdData[108 + (i * 12)] & 0x20) == 0 ? _T("1/2 MEMCLK") : _T("1 MEMCLK"));
                    _stprintf_s(g_szSITempDebug1, _T("  Chip Select setup time: %s"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CSSetupTime);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK = (float)(spdData[109 + (i * 12)] >> 4);

                    if ((spdData[109 + (i * 12)] & 0x0f) <= 9)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += (spdData[109 + (i * 12)] & 0xf) * 0.1f;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 10)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 0.25f;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 11)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 0.33f;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 12)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 0.66f;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 13)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 0.75f;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 14)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 0.875f;
                    }
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK = (spdData[109 + (i * 12)] >> 4) * 1000;

                    if ((spdData[109 + (i * 12)] & 0x0f) <= 9)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += (spdData[109 + (i * 12)] & 0xf) * 100;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 10)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 250;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 11)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 330;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 12)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 660;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 13)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 750;
                    }
                    else if ((spdData[109 + (i * 12)] & 0x0f) == 14)
                    {
                        MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK += 875;
                    }
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Cycle Time (tCK): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].clkspeed = TO_TIMING_VAR(1000 / MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tCK);
                    _stprintf_s(g_szSITempDebug1, _T("  Maximum clock speed: %d MHz"), (int)MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].clkspeed);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    // CAS latencies supported
                    for (j = 2; j < 7; j++)
                    {
                        if (spdData[110 + (i * 12)] & (1 << j))
                        {
                            MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CASSupported = j;
                            break;
                        }
                    }
                    _stprintf_s(g_szSITempDebug1, _T("  Supported CAS Latency: %d"), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].CASSupported);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRCD = (spdData[111 + (i * 12)] >> 2) + ((spdData[111 + (i * 12)] & 3) * 0.25f);
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRCD = (spdData[111 + (i * 12)] >> 2) * 1000 + ((spdData[111 + (i * 12)] & 3) * 250);
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum RAS to CAS delay (tRCD): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRCD);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRP = (spdData[112 + (i * 12)] >> 2) + ((spdData[112 + (i * 12)] & 3) * 0.25f);
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRP = (spdData[112 + (i * 12)] >> 2) * 1000 + ((spdData[112 + (i * 12)] & 3) * 250);
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Row Precharge Time (tRP): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRP);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRAS = spdData[113 + (i * 12)];
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRAS = spdData[113 + (i * 12)] * 1000;
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Precharge Time (tRAS): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRAS);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tWR = (spdData[114 + (i * 12)] >> 2) + ((spdData[114 + (i * 12)] & 3) * 0.25f);
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tWR = (spdData[114 + (i * 12)] >> 2) * 1000 + ((spdData[114 + (i * 12)] & 3) * 250);
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Write Recovery Time (tWR): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tWR);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRC = spdData[115 + (i * 12)] + FRACTIONAL_NS_TABLE[(spdData[115 + (i * 12)] >> 4) & 0x07];
#else
                    MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRC = spdData[115 + (i * 12)] * 1000 + FRACTIONAL_NS_TABLE[(spdData[115 + (i * 12)] >> 4) & 0x07];
#endif
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Auto-refresh Delay (tRC): "), MemoryInfo->specific.DDR2SDRAM.EPP.profileData.fullProfile[i].tRC);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);
                }
            }
        }
    }
    break;
    case DDR2FB10:
    case DDR2FB11:
    {
        // manufacture information
        // get the number of continuation codes
        unsigned char numCont = spdData[117] & 0x7F;

#ifdef WIN32
        float dividend;
        float divisor;
        float mtb;
        float ddrclk;
#else
        int dividend;
        int divisor;
        int mtb;
        int ddrclk;
#endif
        int pcclk;
        int index;

        int taa;
        int trcd;
        int trp;
        int tras;

        MemoryInfo->jedecBank = numCont + 1;
        MemoryInfo->jedecID = spdData[118];

        GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
        _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);

        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleManufLoc = spdData[119];

        _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (spdData[120] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->year = 2000 + ((spdData[120] >> 4) * 10) + (spdData[120] & 0x0F); // convert from BCD
        if (spdData[121] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->week = ((spdData[121] >> 4) * 10) + (spdData[121] & 0x0F);        // convert from BCD

        _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleSerialNo = (spdData[122] << 24) | (spdData[123] << 16) | (spdData[124] << 8) | spdData[125];
        _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        memcpy(MemoryInfo->moduleExtSerialNo, &spdData[117], sizeof(MemoryInfo->moduleExtSerialNo));

        for (i = 0; i < SPD_DDR3_MODULE_PARTNO_LEN; i++)
        {
            if (spdData[128 + i] >= 0 && spdData[128 + i] < 128) // Sanity check for ASCII char
                MemoryInfo->modulePartNo[i] = spdData[128 + i];
            else
                break;
        }

#ifdef WIN32
        TCHAR wcPartNo[SPD_DDR3_MODULE_PARTNO_LEN + 1];
        memset(wcPartNo, 0, sizeof(wcPartNo));
        MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_DDR3_MODULE_PARTNO_LEN, wcPartNo, SPD_DDR3_MODULE_PARTNO_LEN + 1);
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleRev = (spdData[146] << 8) | spdData[147];
        _stprintf_s(g_szSITempDebug1, _T("Revision Code: 0x%04X"), MemoryInfo->moduleRev);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        numCont = spdData[148] & 0x7F;

        MemoryInfo->specific.DDR2FBSDRAM.DRAMManufBank = numCont + 1;
        MemoryInfo->specific.DDR2FBSDRAM.DRAMManufID = spdData[149];

        GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR2FBSDRAM.DRAMManufID & 0x7F, numCont, MemoryInfo->specific.DDR2FBSDRAM.DRAMManuf, SHORT_STRING_LEN);
        _stprintf_s(g_szSITempDebug1, _T("DRAM Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->specific.DDR2FBSDRAM.DRAMManuf, MemoryInfo->specific.DDR2FBSDRAM.DRAMManufBank, MemoryInfo->specific.DDR2FBSDRAM.DRAMManufID);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Module Type
        _tcscpy(MemoryInfo->specific.DDR2FBSDRAM.moduleType, DDR2FB_MODULE_TYPES_TABLE[spdData[6] & 0x0F]);
        _stprintf_s(g_szSITempDebug1, _T("Module type: %s"), MemoryInfo->specific.DDR2FBSDRAM.moduleType);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if ((spdData[6] & 0x0F) == 0x01)
            MemoryInfo->registered = true;
        else if ((spdData[6] & 0x0F) == 0x05)
            MemoryInfo->registered = true;
        else
            MemoryInfo->registered = false;

        if (spdData[81] & 0x02)
            MemoryInfo->ecc = true;

        // Speed
        dividend = (spdData[8] >> 4) & 0x0F;
        divisor = spdData[8] & 0x0F;

        if (dividend != 0 && divisor != 0)
        {
            SPRINTF_FLOAT(g_szSITempDebug1, _T("Fine time base (ps): "), TO_TIMING_VAR(dividend / divisor));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        dividend = spdData[9];
        divisor = spdData[10];

        if (dividend == 0 || divisor == 0)
        {
            SysInfo_DebugLogWriteLine(_T("Medium time base not valid."));
            return false;
        }
        mtb = TO_TIMING_VAR(dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Medium time base (ns): "), TO_TIMING_VAR(dividend / divisor));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tCK = TO_TIMING_VAR(spdData[11] * dividend / divisor);
        ddrclk = (2 * 1000 * divisor) / (spdData[11] * dividend);
        MemoryInfo->txWidth = 64; // is this correct?
        pcclk = (int)((2 * 1000 * divisor * MemoryInfo->txWidth) / (8 * spdData[11] * dividend));
        MemoryInfo->clkspeed = (1000 * divisor) / (spdData[11] * dividend);

        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        pcclk = (pcclk / 100) * 100; // SI101011 - truncate the speed e.g. DDR3 533MHz is PC3-8533, but should be displayed as PC3-8500
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MT/s (PC-%d)"), (int)ddrclk, pcclk);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("PC-%d"), pcclk);
        wcscpy(MemoryInfo->szModulespeed, g_szSITempDebug1);

        MemoryInfo->numBanks = 1 << ((spdData[4] & 0x03) + 2);
        MemoryInfo->rowAddrBits = ((spdData[4] >> 5) & 0x07) + 12;
        MemoryInfo->colAddrBits = ((spdData[4] >> 2) & 0x07) + 9;
        MemoryInfo->busWidth = 64; // is this correct?
        MemoryInfo->numRanks = (spdData[7] >> 3) & 0x07;

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"),
                    MemoryInfo->numBanks,
                    MemoryInfo->rowAddrBits,
                    MemoryInfo->colAddrBits,
                    MemoryInfo->busWidth);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), MemoryInfo->numRanks);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = 1 << ((spdData[7] & 0x07) + 2);

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d"), MemoryInfo->deviceWidth);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // size computation
        index = MemoryInfo->rowAddrBits + MemoryInfo->colAddrBits - 17;

        MemoryInfo->size = (1 << index) * MemoryInfo->numRanks * MemoryInfo->numBanks;
        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.moduleHeight, DDR2FB_MODULE_HEIGHT_TABLE[(spdData[5] >> 3) & 0x07]);
        _stprintf_s(g_szSITempDebug1, _T("Module height: %s"), MemoryInfo->specific.DDR2FBSDRAM.moduleHeight);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.moduleThickness, MODULE_THICKNESS_TABLE[spdData[5] & 0x07]);
        _stprintf_s(g_szSITempDebug1, _T("Module thickness: %s"), MemoryInfo->specific.DDR2FBSDRAM.moduleThickness);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // latencies
        _stprintf_s(MemoryInfo->CASSupported, _T(""));

        for (i = 0; i < (spdData[13] >> 4); i++)
        {
            TCHAR strCAS[8];
            _stprintf_s(strCAS, _T("%d"), (spdData[13] & 0x0F) + i);
            _tcscat(MemoryInfo->CASSupported, strCAS);
            _tcscat(MemoryInfo->CASSupported, _T(" "));
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported CAS: %s"), MemoryInfo->CASSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum cycle time (tCK_min): "), MemoryInfo->tCK);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2FBSDRAM.tCKmax = TO_TIMING_VAR(spdData[12] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum cycle time (tCK_max): "), MemoryInfo->specific.DDR2FBSDRAM.tCKmax);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // more timing information
        MemoryInfo->tAA = TO_TIMING_VAR(spdData[14] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = TO_TIMING_VAR(spdData[19] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS to CAS delay time (tRCD): "), MemoryInfo->tRCD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = TO_TIMING_VAR(spdData[21] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum row precharge time (tRP): "), MemoryInfo->tRP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRAS = TO_TIMING_VAR((((spdData[22] & 0x0F) << 8) + spdData[23]) * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum active to precharge time (tRAS): "), MemoryInfo->tRAS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        taa = (int)CEIL(MemoryInfo->tAA, MemoryInfo->tCK);
        trcd = (int)CEIL(MemoryInfo->tRCD, MemoryInfo->tCK);
        trp = (int)CEIL(MemoryInfo->tRP, MemoryInfo->tCK);
        tras = (int)CEIL(MemoryInfo->tRAS, MemoryInfo->tCK);

#ifdef WIN32
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#else
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2FBSDRAM.tWR = TO_TIMING_VAR(spdData[16] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Write Recovery time (tWR): "), MemoryInfo->specific.DDR2FBSDRAM.tWR);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.WRSupported, _T(""));

        for (i = 0; i < (spdData[15] >> 4); i++)
        {
            TCHAR strWR[8];
            _stprintf_s(strWR, _T("%d"), (spdData[15] & 0x0F) + i);
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.WRSupported, strWR);
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.WRSupported, _T(" "));
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported Write recovery: %s"), MemoryInfo->specific.DDR2FBSDRAM.WRSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.WESupported, _T(""));

        for (i = 0; i < (spdData[17] >> 4); i++)
        {
            TCHAR strWE[8];
            _stprintf_s(strWE, _T("%d"), (spdData[17] & 0x0F) + i);
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.WESupported, strWE);
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.WESupported, _T(" "));
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported Write latencies: %s"), MemoryInfo->specific.DDR2FBSDRAM.WESupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.ALSupported, _T(""));

        for (i = 0; i < (spdData[18] >> 4); i++)
        {
            TCHAR strAL[8];
            _stprintf_s(strAL, _T("%d"), (spdData[18] & 0x0F) + i);
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.ALSupported, strAL);
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.ALSupported, _T(" "));
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported additive latencies: %s"), MemoryInfo->specific.DDR2FBSDRAM.ALSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRRD = TO_TIMING_VAR(spdData[20] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Active to Row Active Delay (tRRD): "), MemoryInfo->tRRD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRC = TO_TIMING_VAR(((((spdData[22] >> 4) & 0x0F) << 8) + spdData[24]) * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Active to Auto-Refresh Delay (tRC): "), MemoryInfo->tRC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRFC = TO_TIMING_VAR(((spdData[26] << 8) + spdData[25]) * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC): "), MemoryInfo->tRFC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2FBSDRAM.tWTR = TO_TIMING_VAR(spdData[28] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Write to Read CMD Delay (tWTR): "), MemoryInfo->specific.DDR2FBSDRAM.tWTR);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2FBSDRAM.tRTP = TO_TIMING_VAR(spdData[29] * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Read to Pre-charge CMD Delay (tRTP): "), MemoryInfo->specific.DDR2FBSDRAM.tRTP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR2FBSDRAM.tFAW = TO_TIMING_VAR((((spdData[28] & 0x0F) << 8) + spdData[29]) * dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Four Activate Window Delay (tFAW): "), MemoryInfo->specific.DDR2FBSDRAM.tFAW);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // miscellaneous stuff

        _stprintf_s(MemoryInfo->moduleVoltage, _T(""));

        switch (spdData[3] & 0x0F)
        {
        case 0:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS1: Undefined"));
            break;
        case 1:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS1: 1.8V"));
            break;
        case 2:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS1: 1.5V"));
            break;
        case 3:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS1: 1.2V"));
            break;
        default:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS1: TBD"));
            break;
        }

        _tcscat(MemoryInfo->moduleVoltage, _T(", "));

        switch ((spdData[3] & 0xF0) >> 4)
        {
        case 0:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS2: Undefined"));
            break;
        case 1:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS2: 1.8V"));
            break;
        case 2:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS2: 1.5V"));
            break;
        case 3:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS2: 1.2V"));
            break;
        default:
            _tcscat(MemoryInfo->moduleVoltage, _T("PS2: TBD"));
            break;
        }

        _stprintf_s(g_szSITempDebug1, _T("Voltage level: %s"), MemoryInfo->moduleVoltage);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.BurstLengthsSupported, _T(""));

        if (spdData[29] & 0x01)
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.BurstLengthsSupported, _T("4 "));
        if (spdData[29] & 0x02)
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.BurstLengthsSupported, _T("8 "));
        if (spdData[29] & 0x80)
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.BurstLengthsSupported, _T("Burst Chop "));

        _stprintf_s(g_szSITempDebug1, _T("Burst lengths supported: %s"), MemoryInfo->specific.DDR2FBSDRAM.BurstLengthsSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.TerminationsSupported, _T(""));

        if (spdData[30] & 0x01)
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.TerminationsSupported, _T("150ohm ODT "));
        if (spdData[30] & 0x02)
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.TerminationsSupported, _T("75ohm ODT "));
        if (spdData[30] & 0x04)
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.TerminationsSupported, _T("50ohm ODT "));
        _stprintf_s(g_szSITempDebug1, _T("Terminations Supported: %s"), MemoryInfo->specific.DDR2FBSDRAM.TerminationsSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(MemoryInfo->specific.DDR2FBSDRAM.DriversSupported, _T(""));

        if (spdData[31] & 0x01)
            _tcscat(MemoryInfo->specific.DDR2FBSDRAM.DriversSupported, _T("Weak driver"));

        _stprintf_s(g_szSITempDebug1, _T("Drivers supported: %s"), MemoryInfo->specific.DDR2FBSDRAM.DriversSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _tcscpy(MemoryInfo->specific.DDR2FBSDRAM.RefreshRate, REFRESH_RATE_TABLE[spdData[32] & 0x0F]);

        _stprintf_s(g_szSITempDebug1, _T("Refresh Rate: %s"), MemoryInfo->specific.DDR2FBSDRAM.RefreshRate);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Requires double refresh rate for temperatures > 85C: %s"), spdData[32] & 0x80 ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("High temperature self-refresh entry supported: %s"), spdData[32] & 0x40 ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
    }
    break;
    case DDR310:
    case DDR311:
    case DDR312: // added SI101043
    case DDR313:
    {
        // manufacture information
        // get the number of continuation codes
        unsigned char numCont = spdData[117] & 0x7F;

#ifdef WIN32
        float dividend;
        float divisor;
        float ftb;
        float mtb;
        float ddrclk;
#else
        int dividend;
        int divisor;
        int ftb;
        int mtb;
        int ddrclk;
#endif
        int capacity;
        int ranks;
        int CASBitmap;
        int pcclk;

        int taa;
        int trcd;
        int trp;
        int tras;

        MemoryInfo->jedecBank = numCont + 1;
        MemoryInfo->jedecID = spdData[118];

        GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
        _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);

        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleManufLoc = spdData[119];

        _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (spdData[120] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->year = 2000 + ((spdData[120] >> 4) * 10) + (spdData[120] & 0x0F); // convert from BCD
        if (spdData[121] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
            MemoryInfo->week = ((spdData[121] >> 4) * 10) + (spdData[121] & 0x0F);        // convert from BCD

        _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleSerialNo = (spdData[122] << 24) | (spdData[123] << 16) | (spdData[124] << 8) | spdData[125];
        _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        memcpy(MemoryInfo->moduleExtSerialNo, &spdData[117], sizeof(MemoryInfo->moduleExtSerialNo));

        for (i = 0; i < SPD_DDR3_MODULE_PARTNO_LEN; i++)
        {
            if (spdData[128 + i] >= 0 && spdData[128 + i] < 128) // Sanity check for ASCII char
                MemoryInfo->modulePartNo[i] = spdData[128 + i];
            else
                break;
        }

#ifdef WIN32
        TCHAR wcPartNo[SPD_DDR3_MODULE_PARTNO_LEN + 1];
        memset(wcPartNo, 0, sizeof(wcPartNo));
        MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_DDR3_MODULE_PARTNO_LEN, wcPartNo, SPD_DDR3_MODULE_PARTNO_LEN + 1);
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
        _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->moduleRev = (spdData[146] << 8) | spdData[147];
        _stprintf_s(g_szSITempDebug1, _T("Revision Code: 0x%04X"), MemoryInfo->moduleRev);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        numCont = spdData[148] & 0x7F;
        MemoryInfo->specific.DDR3SDRAM.DRAMManufBank = numCont + 1;
        MemoryInfo->specific.DDR3SDRAM.DRAMManufID = spdData[149];

        GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR3SDRAM.DRAMManufID & 0x7F, numCont, MemoryInfo->specific.DDR3SDRAM.DRAMManuf, SHORT_STRING_LEN);
        _stprintf_s(g_szSITempDebug1, _T("DRAM Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->specific.DDR3SDRAM.DRAMManuf, MemoryInfo->specific.DDR3SDRAM.DRAMManufBank, MemoryInfo->specific.DDR3SDRAM.DRAMManufID);

        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
        // Bytes 150 ~ 175: Manufacturer's Specific Data
        MemoryInfo->manufDataOff = 150;
        MemoryInfo->manufDataLen = 175 - 150 + 1;

        _stprintf_s(tmpMsg, _T("Module Manufacturer's Specific Data raw bytes:"));
        MtSupportUCDebugWriteLine(tmpMsg);

        _stprintf_s(tmpMsg, _T(""));
        for (i = 0; i < MemoryInfo->manufDataLen; i++)
        {
            TCHAR strSPDBytes[8];
            _stprintf_s(strSPDBytes, _T("%02X "), spdData[MemoryInfo->manufDataOff + i]);
            _tcscat(tmpMsg, strSPDBytes);

            if ((i % 16 == 15) || (i == MemoryInfo->manufDataLen - 1))
            {
                MtSupportUCDebugWriteLine(tmpMsg);
                _stprintf_s(tmpMsg, _T(""));
            }
        }
#endif
        // Module Type
        _tcscpy(MemoryInfo->specific.DDR3SDRAM.moduleType, DDR3_MODULE_TYPES_TABLE[spdData[3] & 0x0F]);
        _stprintf_s(g_szSITempDebug1, _T("Module type: %s"), MemoryInfo->specific.DDR3SDRAM.moduleType);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if ((spdData[3] & 0x0F) == 0x01)
            MemoryInfo->registered = true;
        else if ((spdData[3] & 0x0F) == 0x05)
            MemoryInfo->registered = true;
        else if ((spdData[3] & 0x0F) == 0x09)
            MemoryInfo->registered = true;
        else
            MemoryInfo->registered = false;

        // Speed
        dividend = (spdData[9] >> 4) & 0x0F;
        divisor = spdData[9] & 0x0F;

        if (dividend != 0 && divisor != 0)
        {
            ftb = TO_TIMING_VAR((dividend / divisor) / 1000);

            SPRINTF_FLOAT(g_szSITempDebug1, _T("Fine time base (ps): "), TO_TIMING_VAR(dividend / divisor));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
        else
            ftb = 0;

        dividend = spdData[10];
        divisor = spdData[11];

        if (dividend == 0 || divisor == 0)
        {
            SysInfo_DebugLogWriteLine(_T("Medium time base not valid. Using default value of 0.125"));
            dividend = 1;
            divisor = 8;
        }

        mtb = TO_TIMING_VAR(dividend / divisor);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Medium time base: "), TO_TIMING_VAR(dividend / divisor));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tCK = (spdData[12] * mtb) + ((char)spdData[34] * ftb);
        ddrclk = TO_TIMING_VAR((2 * 1000) / MemoryInfo->tCK); // (2 * 1000 * divisor) / (spdData[12] * dividend);
        MemoryInfo->txWidth = 1 << ((spdData[8] & 0x07) + 3);
        pcclk = (int)TO_TIMING_VAR((2 * 1000 * MemoryInfo->txWidth) / (8 * MemoryInfo->tCK)); // (int)((2 * 1000 * MemoryInfo->txWidth * divisor) / (8 * spdData[12] * dividend));
        MemoryInfo->clkspeed = TO_TIMING_VAR(1000 / (MemoryInfo->tCK));

        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        pcclk = (pcclk / 100) * 100; // SI101011 - truncate the speed e.g. DDR3 533MHz is PC3-8533, but should be displayed as PC3-8500
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MT/s (PC3-%d)"), (int)ddrclk, pcclk);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("PC3-%d"), pcclk);
        wcscpy(MemoryInfo->szModulespeed, g_szSITempDebug1);

        // Size computation
        capacity = (spdData[4] & 0x0F) + 28;
        capacity += (spdData[8] & 0x07) + 3;
        capacity -= (spdData[7] & 0x07) + 2;
        capacity -= 20 + 3;
        ranks = ((spdData[7] >> 3) & 0x1F) + 1;
        MemoryInfo->size = (1 << capacity) * ranks;
        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->numBanks = 1 << (((spdData[4] >> 4) & 0x07) + 3);
        MemoryInfo->rowAddrBits = ((spdData[5] >> 3) & 0x1F) + 12;
        MemoryInfo->colAddrBits = (spdData[5] & 0x07) + 9;
        MemoryInfo->busWidth = 1 << ((spdData[8] & 0x07) + 3);
        MemoryInfo->numRanks = ranks;

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"),
                    MemoryInfo->numBanks,
                    MemoryInfo->rowAddrBits,
                    MemoryInfo->colAddrBits,
                    MemoryInfo->busWidth);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), ranks);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = 1 << ((spdData[7] & 0x07) + 2);

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d"), MemoryInfo->deviceWidth);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // ecc
        if ((spdData[8] >> 3) & 0x03)
            MemoryInfo->ecc = true;
        else
            MemoryInfo->ecc = false;

        _stprintf_s(g_szSITempDebug1, _T("ECC: %s"), MemoryInfo->ecc ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // latencies
        CASBitmap = (spdData[15] << 8) + spdData[14];
        _stprintf_s(MemoryInfo->CASSupported, _T(""));

        for (i = 0; i < 15; i++)
        {
            if (CASBitmap & (1 << i))
            {
                TCHAR strCAS[8];
                _stprintf_s(strCAS, _T("%d"), i + 4);
                _tcscat(MemoryInfo->CASSupported, strCAS);
                _tcscat(MemoryInfo->CASSupported, _T(" "));
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported CAS: %s"), MemoryInfo->CASSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // more timing information
        MemoryInfo->tAA = (spdData[16] * mtb) + ((char)spdData[35] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = (spdData[18] * mtb) + ((char)spdData[36] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS to CAS delay time (tRCD): "), MemoryInfo->tRCD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = (spdData[20] * mtb) + ((char)spdData[37] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum row precharge time (tRP): "), MemoryInfo->tRP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRAS = ((((spdData[21] & 0x0F) << 8) + spdData[22]) * mtb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum active to precharge time (tRAS): "), MemoryInfo->tRAS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        taa = (int)CEIL(MemoryInfo->tAA, MemoryInfo->tCK);
        trcd = (int)CEIL(MemoryInfo->tRCD, MemoryInfo->tCK);
        trp = (int)CEIL(MemoryInfo->tRP, MemoryInfo->tCK);
        tras = (int)CEIL(MemoryInfo->tRAS, MemoryInfo->tCK);

#ifdef WIN32
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#else
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.tWR = spdData[17] * mtb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Write Recovery time (tWR): "), MemoryInfo->specific.DDR3SDRAM.tWR);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRRD = spdData[19] * mtb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Row Active to Row Active Delay (tRRD): "), MemoryInfo->tRRD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRC = (((((spdData[21] >> 4) & 0x0F) << 8) + spdData[23]) * mtb) + ((char)spdData[38] * ftb);
        ;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Active to Auto-Refresh Delay (tRC): "), MemoryInfo->tRC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRFC = (((spdData[25] << 8) + spdData[24]) * mtb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC): "), MemoryInfo->tRFC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.tWTR = (spdData[26] * mtb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Write to Read CMD Delay (tWTR): "), MemoryInfo->specific.DDR3SDRAM.tWTR);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.tRTP = (spdData[27] * mtb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Read to Pre-charge CMD Delay (tRTP): "), MemoryInfo->specific.DDR3SDRAM.tRTP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.tFAW = ((((spdData[28] & 0x0F) << 8) + spdData[29]) * mtb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Four Activate Window Delay (tFAW): "), MemoryInfo->specific.DDR3SDRAM.tFAW);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // miscellaneous stuff

        _stprintf_s(MemoryInfo->moduleVoltage, _T("1.5V"));

        if (spdData[6] & 1)
        {
            _tcscat(MemoryInfo->moduleVoltage, _T(" tolerant"));
        }
        if (spdData[6] & 2)
        {
            _tcscat(MemoryInfo->moduleVoltage, _T(", 1.35V "));
        }
        if (spdData[6] & 4)
        {
            _tcscat(MemoryInfo->moduleVoltage, _T(", 1.2X V"));
        }
        _stprintf_s(g_szSITempDebug1, _T("Operable voltages: %s"), MemoryInfo->moduleVoltage);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.RZQ6Supported = (spdData[30] & 0x01) != 0;
        _stprintf_s(g_szSITempDebug1, _T("RZQ/6 supported: %s"), MemoryInfo->specific.DDR3SDRAM.RZQ6Supported ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.RZQ7Supported = (spdData[30] & 0x02) != 0;
        _stprintf_s(g_szSITempDebug1, _T("RZQ/7 supported: %s"), MemoryInfo->specific.DDR3SDRAM.RZQ7Supported ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.DLLOffModeSupported = (spdData[30] & 0x80) != 0;
        _stprintf_s(g_szSITempDebug1, _T("DLL-Off Mode supported: %s"), MemoryInfo->specific.DDR3SDRAM.DLLOffModeSupported ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.OperatingTempRange = (spdData[31] & 0x01) ? 95 : 85;
        _stprintf_s(g_szSITempDebug1, _T("Operating temperature range: 0-%dC"), MemoryInfo->specific.DDR3SDRAM.OperatingTempRange);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.RefreshRateExtTempRange = (spdData[31] & 0x02) ? 1 : 2;
        _stprintf_s(g_szSITempDebug1, _T("Refresh Rate in extended temp range: %dX"), MemoryInfo->specific.DDR3SDRAM.RefreshRateExtTempRange);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.autoSelfRefresh = (spdData[31] & 0x04) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Auto Self-Refresh: %s"), MemoryInfo->specific.DDR3SDRAM.autoSelfRefresh ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.onDieThermalSensorReadout = (spdData[31] & 0x08) != 0;
        _stprintf_s(g_szSITempDebug1, _T("On-Die Thermal Sensor readout: %s"), MemoryInfo->specific.DDR3SDRAM.onDieThermalSensorReadout ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.partialArraySelfRefresh = (spdData[31] & 0x80) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Partial Array Self-Refresh: %s"), MemoryInfo->specific.DDR3SDRAM.partialArraySelfRefresh ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR3SDRAM.thermalSensorPresent = (spdData[32] & 0x80) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensor present: %s"), MemoryInfo->specific.DDR3SDRAM.thermalSensorPresent ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (spdData[33] & 0x80)
            _stprintf_s(MemoryInfo->specific.DDR3SDRAM.nonStdSDRAMType, _T("%02X"), spdData[33] & 0x7F);
        else
            _stprintf_s(MemoryInfo->specific.DDR3SDRAM.nonStdSDRAMType, _T("Standard Monolithic"));

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Type: %s"), MemoryInfo->specific.DDR3SDRAM.nonStdSDRAMType);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (spdData[3] >= 1 && spdData[3] <= 0xA)
        {
            MemoryInfo->specific.DDR3SDRAM.moduleHeight = (spdData[60] & 0x1F) + 15;
            _stprintf_s(g_szSITempDebug1, _T("Module Height (mm): %d - %d"), MemoryInfo->specific.DDR3SDRAM.moduleHeight - 1, MemoryInfo->specific.DDR3SDRAM.moduleHeight);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR3SDRAM.moduleThicknessFront = (spdData[61] & 0xF) + 1;
            MemoryInfo->specific.DDR3SDRAM.moduleThicknessBack = ((spdData[61] >> 4) & 0xF) + 1;
            _stprintf_s(g_szSITempDebug1, _T("Module Thickness (mm): front %d-%d , back %d-%d "), MemoryInfo->specific.DDR3SDRAM.moduleThicknessFront - 1, MemoryInfo->specific.DDR3SDRAM.moduleThicknessFront, MemoryInfo->specific.DDR3SDRAM.moduleThicknessBack - 1, MemoryInfo->specific.DDR3SDRAM.moduleThicknessBack);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(MemoryInfo->specific.DDR3SDRAM.moduleWidth, _T("%s"), (spdData[3] <= 2) ? _T("133.5") : (spdData[3] == 3) ? _T("67.6")
                                                                                                                                  : _T("TBD"));
            _stprintf_s(g_szSITempDebug1, _T("Module Width (mm): %s"), MemoryInfo->specific.DDR3SDRAM.moduleWidth);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if ((spdData[62] & 0x1F) == 0x1F)
            {
                _stprintf_s(MemoryInfo->specific.DDR3SDRAM.moduleRefCard, _T("Raw Card ZZ"));
            }
            else
            {
                int cardRev = ((spdData[62] >> 5) & 0x03);
                if ((spdData[62] & 0x80) == 0)
                {
                    static const TCHAR *REF_RAW_CARD_TABLE[] = {_T("A"),
                                                                _T("B"),
                                                                _T("C"),
                                                                _T("D"),
                                                                _T("E"),
                                                                _T("F"),
                                                                _T("G"),
                                                                _T("H"),
                                                                _T("J"),
                                                                _T("K"),
                                                                _T("L"),
                                                                _T("M"),
                                                                _T("N"),
                                                                _T("P"),
                                                                _T("R"),
                                                                _T("T"),
                                                                _T("U"),
                                                                _T("V"),
                                                                _T("W"),
                                                                _T("Y"),
                                                                _T("AA"),
                                                                _T("AB"),
                                                                _T("AC"),
                                                                _T("AD"),
                                                                _T("AE"),
                                                                _T("AF"),
                                                                _T("AG"),
                                                                _T("AH"),
                                                                _T("AJ"),
                                                                _T("AK"),
                                                                _T("AL")};

                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.moduleRefCard, _T("Raw Card %s Rev. %d"), REF_RAW_CARD_TABLE[spdData[62] & 0x1F], cardRev);
                }
                else
                {
                    static const TCHAR *REF_RAW_CARD_TABLE[] = {
                        _T("AM"),
                        _T("AN"),
                        _T("AP"),
                        _T("AR"),
                        _T("AT"),
                        _T("AU"),
                        _T("AV"),
                        _T("AW"),
                        _T("AY"),
                        _T("BA"),
                        _T("BB"),
                        _T("BC"),
                        _T("BD"),
                        _T("BE"),
                        _T("BF"),
                        _T("BG"),
                        _T("BH"),
                        _T("BJ"),
                        _T("BK"),
                        _T("BL"),
                        _T("BM"),
                        _T("BN"),
                        _T("BP"),
                        _T("BR"),
                        _T("BT"),
                        _T("BU"),
                        _T("BV"),
                        _T("BW"),
                        _T("BY"),
                        _T("CA"),
                        _T("CB")};

                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.moduleRefCard, _T("Raw Card %s Rev. %d"), REF_RAW_CARD_TABLE[spdData[62] & 0x1F], cardRev);
                }
            }
            _stprintf_s(g_szSITempDebug1, _T("Module reference card: %s"), MemoryInfo->specific.DDR3SDRAM.moduleRefCard);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        if (spdData[3] == 1 || spdData[3] == 5 || spdData[3] == 9)
        {

            static const int NUM_REGISTERS_MAP[] = {0, 1, 2, 4};
            static const int NUM_ROWS_MAP[] = {0, 1, 2, 4};
            MemoryInfo->specific.DDR3SDRAM.numRegisters = NUM_REGISTERS_MAP[spdData[63] & 0x03];
            _stprintf_s(g_szSITempDebug1, _T("# Registers: %d"), MemoryInfo->specific.DDR3SDRAM.numRegisters);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR3SDRAM.numDRAMRows = NUM_ROWS_MAP[(spdData[63] >> 2) & 0x03];
            _stprintf_s(g_szSITempDebug1, _T("# DRAM Rows: %d"), MemoryInfo->specific.DDR3SDRAM.numDRAMRows);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR3SDRAM.heatSpreaderSolution = (spdData[64] & 0x80) != 0;
            _stprintf_s(g_szSITempDebug1, _T("Heat spreader solution incorporated: %s"), MemoryInfo->specific.DDR3SDRAM.heatSpreaderSolution ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            numCont = spdData[65] & 0x7F;
            MemoryInfo->specific.DDR3SDRAM.regManufBank = numCont + 1;
            MemoryInfo->specific.DDR3SDRAM.regManufID = spdData[66];

            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR3SDRAM.regManufID & 0x7F, numCont, MemoryInfo->specific.DDR3SDRAM.regManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("Register Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->specific.DDR3SDRAM.regManuf, MemoryInfo->specific.DDR3SDRAM.regManufBank, MemoryInfo->specific.DDR3SDRAM.regManufID);

            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR3SDRAM.regRev = spdData[67];
            _stprintf_s(g_szSITempDebug1, _T("Register revision: 0x%02X"), MemoryInfo->specific.DDR3SDRAM.regRev);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if ((spdData[68] & 7) == 0)
                _tcscpy(MemoryInfo->specific.DDR3SDRAM.regDeviceType, L"SSTE32882");
            else
                _tcscpy(MemoryInfo->specific.DDR3SDRAM.regDeviceType, L"Undefined");

            _stprintf_s(g_szSITempDebug1, _T("Register type: %s"), MemoryInfo->specific.DDR3SDRAM.regDeviceType);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR3SDRAM.cmdAddrADriveStrength, DRIVE_STRENGTH_TABLE[(spdData[70] >> 4) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Command/Address A Outputs Drive Strength: %s"), MemoryInfo->specific.DDR3SDRAM.cmdAddrADriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR3SDRAM.cmdAddrBDriveStrength, DRIVE_STRENGTH_TABLE[(spdData[70] >> 6) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Command/Address B Outputs Drive Strength: %s"), MemoryInfo->specific.DDR3SDRAM.cmdAddrBDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR3SDRAM.ctrlSigADriveStrength, DRIVE_STRENGTH_TABLE[spdData[71] & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Control Signals A Outputs Drive Strength: %s"), MemoryInfo->specific.DDR3SDRAM.ctrlSigADriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR3SDRAM.ctrlSigBDriveStrength, DRIVE_STRENGTH_TABLE[(spdData[71] >> 2) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Control Signals B Outputs Drive Strength: %s"), MemoryInfo->specific.DDR3SDRAM.ctrlSigBDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR3SDRAM.clkY1Y3DriveStrength, DRIVE_STRENGTH_TABLE[(spdData[71] >> 4) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Y1/Y1# and Y3/Y3# Clock Outputs Drive Strength: %s"), MemoryInfo->specific.DDR3SDRAM.clkY1Y3DriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR3SDRAM.clkY0Y2DriveStrength, DRIVE_STRENGTH_TABLE[(spdData[71] >> 6) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Y0/Y0# and Y2/Y2# Clock Outputs Drive Strength: %s"), MemoryInfo->specific.DDR3SDRAM.clkY0Y2DriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        // XMP
        if (spdData[176] == SPD_DDR3_XMP_ID_1 && spdData[177] == SPD_DDR3_XMP_ID_2)
        {
            MemoryInfo->specific.DDR3SDRAM.XMPSupported = true;
            _stprintf_s(g_szSITempDebug1, _T("XMP supported: Yes"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR3SDRAM.XMP.revision = spdData[179];
            _stprintf_s(g_szSITempDebug1, _T("XMP revision: %02X"), MemoryInfo->specific.DDR3SDRAM.XMP.revision);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            // Profile 1 and 2 bytes
            for (i = 0; i < 2; i++)
            {
                int j;

                if (i == 0)
                {
                    MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].enabled = (spdData[178] & 0x1) != 0;
                    _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] Enthusiast / Certified Profile enabled: %s"), i + 1, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].enabled ? _T("Yes") : _T("No"));
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if (MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].enabled == false)
                        continue;

                    MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].dimmsPerChannel = ((spdData[178] >> 2) & 0x03) + 1;
                    _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] recommended DIMMs per channel: %d"), i + 1, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].dimmsPerChannel);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    dividend = spdData[180];
                    divisor = spdData[181];
                }
                else
                {
                    MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].enabled = (spdData[178] & 0x2) != 0;
                    _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] Extreme Profile enabled: %s"), i + 1, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].enabled ? _T("Yes") : _T("No"));
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    if (MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].enabled == false)
                        continue;

                    MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].dimmsPerChannel = ((spdData[178] >> 4) & 0x03) + 1;
                    _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] recommended DIMMs per channel: %d"), i + 1, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].dimmsPerChannel);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    dividend = spdData[182];
                    divisor = spdData[183];
                }

                if (dividend == 0 || divisor == 0)
                {
                    SysInfo_DebugLogWriteLine(_T("  Medium time base not valid."));
                    continue;
                }

                mtb = TO_TIMING_VAR(dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Medium time base: "), TO_TIMING_VAR(dividend / divisor));
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                if (spdData[184] != 0)
                {
                    int dividend_ftb = spdData[184] >> 4 & 0xF;
                    int divisor_ftb = spdData[184] & 0xF;

                    ftb = TO_TIMING_VAR((dividend_ftb / divisor_ftb) / 1000);

                    SPRINTF_FLOAT(g_szSITempDebug1, _T("Fine time base (ps): "), TO_TIMING_VAR(dividend_ftb / divisor_ftb));
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);
                }
                else
                    ftb = 0;

#ifdef WIN32
                _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].moduleVdd, _T("%.2fV"), (float)((spdData[185 + i * 35] & 0x60) >> 5) + (spdData[185 + i * 35] & 0x1F) * 0.05f);
#else
                {
                    int millivolts = ((spdData[185 + i * 35] & 0x60) >> 5) * 1000 + (spdData[185 + i * 35] & 0x1F) * 50;
                    getVoltageStr(millivolts, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].moduleVdd, sizeof(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].moduleVdd));
                }
#endif

                _stprintf_s(g_szSITempDebug1, _T("  Voltage: %s"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].moduleVdd);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK = TO_TIMING_VAR(spdData[186 + i * 35] * dividend / divisor) + (char)spdData[211 + i * 35] * ftb;
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum SDRAM Cycle Time (tCKmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].clkspeed = TO_TIMING_VAR(1000 / MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK);
                _stprintf_s(g_szSITempDebug1, _T("  Maximum clock speed: %d MHz"), (int)MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].clkspeed);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tAA = TO_TIMING_VAR(spdData[187 + i * 35] * dividend / divisor) + (char)spdData[212 + i * 35] * ftb;
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum CAS Latency Time (tAAmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tAA);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                // latencies
                CASBitmap = (spdData[189 + i * 35] << 8) + spdData[188 + i * 35];
                _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].CASSupported, _T(""));

                for (j = 0; j < 15; j++)
                {
                    if (CASBitmap & (1 << j))
                    {
                        TCHAR strCAS[8];
                        _stprintf_s(strCAS, _T("%d"), j + 4);
                        _tcscat(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].CASSupported, strCAS);
                        _tcscat(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].CASSupported, _T(" "));
                    }
                }
                _stprintf_s(g_szSITempDebug1, _T("  Supported CAS: %s"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].CASSupported);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCWL = TO_TIMING_VAR(spdData[190 + i * 35] * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum CAS Write Latency Time (tCWLmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCWL);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRP = TO_TIMING_VAR(spdData[191 + i * 35] * dividend / divisor) + (char)spdData[213 + i * 35] * ftb;
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Row Precharge Delay Time (tRPmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRP);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRCD = TO_TIMING_VAR(spdData[192 + i * 35] * dividend / divisor) + (char)spdData[214 + i * 35] * ftb;
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum RAS# to CAS# Delay Time (tRCDmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRCD);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tWR = TO_TIMING_VAR(spdData[193 + i * 35] * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Write Recovery Time (tWRmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tWR);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRC = TO_TIMING_VAR(((((spdData[194 + i * 35] >> 4) & 0x0F) << 8) + spdData[196 + i * 35]) * dividend / divisor) + (char)spdData[215 + i * 35] * ftb;
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Active/Refresh Delay Time (tRCmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRC);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRAS = TO_TIMING_VAR((((spdData[194 + i * 35] & 0x0F) << 8) + spdData[195 + i * 35]) * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Precharge Delay Time (tRASmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRAS);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tREFI = TO_TIMING_VAR(((spdData[198 + i * 35] << 8) + spdData[197 + i * 35]) * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Maximum tREFI Time (Average Periodic Refresh Interval): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tREFI);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRFC = TO_TIMING_VAR(((spdData[200 + i * 35] << 8) + spdData[199 + i * 35]) * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFCmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRFC);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRTP = TO_TIMING_VAR(spdData[201 + i * 35] * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Internal Read to Precharge Command Delay Time (tRTPmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRTP);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRRD = TO_TIMING_VAR(spdData[202 + i * 35] * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Row Active to Row Active Delay Time (tRRDmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRRD);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tFAW = TO_TIMING_VAR((((spdData[203 + i * 35] & 0x0F) << 8) + spdData[204 + i * 35]) * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Four Activate Window Delay Time (tFAWmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tFAW);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tWTR = TO_TIMING_VAR(spdData[205 + i * 35] * dividend / divisor);
                SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Internal Write to Read Command Delay Time (tWTRmin): "), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tWTR);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                taa = (int)CEIL(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tAA, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK);
                trcd = (int)CEIL(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRCD, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK);
                trp = (int)CEIL(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRP, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK);
                tras = (int)CEIL(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tRAS, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK);

#ifdef WIN32
                _stprintf_s(g_szSITempDebug1, _T("  Supported timing at highest clock speed (%.1f MHz) : %d-%d-%d-%d"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].clkspeed, taa, trcd, trp, tras);
#else
                _stprintf_s(g_szSITempDebug1, _T("  Supported timing at highest clock speed (%d MHz) : %d-%d-%d-%d"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].clkspeed, taa, trcd, trp, tras);
#endif

                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                if ((spdData[206 + i * 35] & 0x07) != 0)
                {
                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].WRtoRDTurnaround, _T("%d Clock %s"), spdData[206 + i * 35] & 0x07, spdData[206 + i * 35] & 0x08 ? _T("Push-out") : _T("Push-in"));
                }
                else
                {
                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].WRtoRDTurnaround, _T("No adjustment"));
                }
                _stprintf_s(g_szSITempDebug1, _T("  Write to Read CMD Turn-around Time Optimizations: %s"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].WRtoRDTurnaround);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                if (((spdData[206 + i * 35] >> 4) & 0x07) != 0)
                {
                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].RDtoWRTurnaround, _T("%d Clock %s"), (spdData[206 + i * 35] >> 4) & 0x07, spdData[206 + i * 35] & 0x80 ? _T("Push-out") : _T("Push-in"));
                }
                else
                {
                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].RDtoWRTurnaround, _T("No adjustment"));
                }
                _stprintf_s(g_szSITempDebug1, _T("  Read to Write CMD Turn-around Time Optimizations: %s"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].RDtoWRTurnaround);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                if ((spdData[207 + i * 35] & 0x07) != 0)
                {
                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].back2BackTurnaround, _T("%d Clock %s"), spdData[207 + i * 35] & 0x07, spdData[207 + i * 35] & 0x08 ? _T("Push-out") : _T("Push-in"));
                }
                else
                {
                    _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].back2BackTurnaround, _T("No adjustment"));
                }
                _stprintf_s(g_szSITempDebug1, _T("  Back 2 Back CMD Turn-around Time Optimizations: %s"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].back2BackTurnaround);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].cmdRateMode = (int)(spdData[208 + i * 35] * mtb * MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].tCK);
#else
                MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].cmdRateMode = (int)((spdData[208 + i * 35] * spdData[186 + i * 35] * dividend * dividend) / (divisor * divisor));
#endif
                _stprintf_s(g_szSITempDebug1, _T("  System CMD Rate Mode: %dN"), MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].cmdRateMode);
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                // SDRAM Auto Self Refresh Performance (TBD)

#ifdef WIN32
                _stprintf_s(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].memCtrlVdd, _T("%.2fV"), (float)((spdData[210 + i * 35] & 0x60) >> 5) + (spdData[210 + i * 35] & 0x1F) * 0.05f);
#else
                {
                    int millivolts = ((spdData[210 + i * 35] & 0x60) >> 5) * 1000 + (spdData[210 + i * 35] & 0x1F) * 50;
                    getVoltageStr(millivolts, MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].memCtrlVdd, sizeof(MemoryInfo->specific.DDR3SDRAM.XMP.profile[i].memCtrlVdd));
                }
#endif
            }
        }

        // TODO: EPP2.0 Bytes 0x96-0xAF - specifications not publically available
    }
    break;

    case DDR410:
    {
#ifdef WIN32
        float ftb;
        float mtb;
        float ddrclk;
#else
        int ftb;
        int mtb;
        int ddrclk;
#endif
        int capacity;
        int ranks;
        int CASBitmap;
        int pcclk;

        int taa;
        int trcd;
        int trp;
        int tras;

        // manufacture information
        // get the number of continuation codes
        unsigned char numCont;

        MemoryInfo->jedecID = 0;
        if (spdDataLen >= 320)
        {
            numCont = spdData[320] & 0x7F;
            MemoryInfo->jedecBank = numCont + 1;
            MemoryInfo->jedecID = spdData[321];

            GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);

            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleManufLoc = spdData[322];

            _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (spdData[323] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
                MemoryInfo->year = 2000 + ((spdData[323] >> 4) * 10) + (spdData[323] & 0x0F); // convert from BCD
            if (spdData[324] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
                MemoryInfo->week = ((spdData[324] >> 4) * 10) + (spdData[324] & 0x0F);        // convert from BCD

            _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleSerialNo = (spdData[325] << 24) | (spdData[326] << 16) | (spdData[327] << 8) | spdData[328];
            _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            memcpy(MemoryInfo->moduleExtSerialNo, &spdData[320], sizeof(MemoryInfo->moduleExtSerialNo));

            for (i = 0; i < SPD_DDR4_MODULE_PARTNO_LEN; i++)
            {
                if (spdData[329 + i] >= 0 && spdData[329 + i] < 128) // Sanity check for ASCII char
                    MemoryInfo->modulePartNo[i] = spdData[329 + i];
                else
                    break;
            }

#ifdef WIN32
            TCHAR wcPartNo[SPD_DDR4_MODULE_PARTNO_LEN + 1];
            memset(wcPartNo, 0, sizeof(wcPartNo));
            MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_DDR4_MODULE_PARTNO_LEN, wcPartNo, SPD_DDR4_MODULE_PARTNO_LEN + 1);
            _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
            _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleRev = spdData[349];
            _stprintf_s(g_szSITempDebug1, _T("Revision Code: 0x%02X"), MemoryInfo->moduleRev);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            numCont = spdData[350] & 0x7F;
            MemoryInfo->specific.DDR4SDRAM.DRAMManufBank = numCont + 1;
            MemoryInfo->specific.DDR4SDRAM.DRAMManufID = spdData[351];

            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR4SDRAM.DRAMManufID & 0x7F, numCont, MemoryInfo->specific.DDR4SDRAM.DRAMManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("DRAM Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->specific.DDR4SDRAM.DRAMManuf, MemoryInfo->specific.DDR4SDRAM.DRAMManufBank, MemoryInfo->specific.DDR4SDRAM.DRAMManufID);

            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR4SDRAM.DRAMStepping = spdData[352];
            _stprintf_s(g_szSITempDebug1, _T("DRAM Stepping: 0x%02X"), MemoryInfo->specific.DDR4SDRAM.DRAMStepping);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
            // 353~381 0x161~0x17D Module Manufacturer's Specific Data
            MemoryInfo->manufDataOff = 353;
            MemoryInfo->manufDataLen = 381 - 353 + 1;

            _stprintf_s(tmpMsg, _T("Module Manufacturer's Specific Data raw bytes:"));
            MtSupportUCDebugWriteLine(tmpMsg);

            _stprintf_s(tmpMsg, _T(""));
            for (i = 0; i < MemoryInfo->manufDataLen; i++)
            {
                TCHAR strSPDBytes[8];
                _stprintf_s(strSPDBytes, _T("%02X "), spdData[MemoryInfo->manufDataOff + i]);
                _tcscat(tmpMsg, strSPDBytes);

                if ((i % 16 == 15) || (i == MemoryInfo->manufDataLen - 1))
                {
                    MtSupportUCDebugWriteLine(tmpMsg);
                    _stprintf_s(tmpMsg, _T(""));
                }
            }
#endif
        }

        // Module Type
        _tcscpy(MemoryInfo->specific.DDR4SDRAM.moduleType, DDR4_MODULE_TYPES_TABLE[spdData[3] & 0x0F]);
        _stprintf_s(g_szSITempDebug1, _T("Module type: %s"), MemoryInfo->specific.DDR4SDRAM.moduleType);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if ((spdData[3] & 0x0F) == 0x01)
            MemoryInfo->registered = true;
        else if ((spdData[3] & 0x0F) == 0x05)
            MemoryInfo->registered = true;
        else if ((spdData[3] & 0x0F) == 0x08)
            MemoryInfo->registered = true;
        else
            MemoryInfo->registered = false;

        // Size computation
        capacity = (spdData[4] & 0x0F) + 28;
        capacity += (spdData[13] & 0x07) + 3;
        capacity -= (spdData[12] & 0x07) + 2;
        capacity -= 20 + 3;
        ranks = ((spdData[12] >> 3) & 0x07) + 1;
        if ((spdData[6] & 0x03) == 0x02)
            ranks *= ((spdData[6] >> 4) & 0x07) + 1;

        MemoryInfo->size = (1 << capacity) * ranks;
        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->numBanks = 1 << ((((spdData[4] >> 4) & 0x03) + 2) + ((spdData[4] >> 6) & 0x03));
        MemoryInfo->rowAddrBits = ((spdData[5] >> 3) & 0x07) + 12;
        MemoryInfo->colAddrBits = (spdData[5] & 0x07) + 9;
        MemoryInfo->busWidth = 1 << ((spdData[13] & 0x07) + 3);
        MemoryInfo->numRanks = ranks;

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"),
                    MemoryInfo->numBanks,
                    MemoryInfo->rowAddrBits,
                    MemoryInfo->colAddrBits,
                    MemoryInfo->busWidth);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), ranks);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = 1 << ((spdData[12] & 0x07) + 2);

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d"), MemoryInfo->deviceWidth);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.SDRAMPkgType.monolithic = (spdData[6] & 0x80) == 0;
        MemoryInfo->specific.DDR4SDRAM.SDRAMPkgType.dieCount = ((spdData[6] >> 4) & 0x07) + 1;
        MemoryInfo->specific.DDR4SDRAM.SDRAMPkgType.multiLoadStack = (spdData[6] & 0x03) == 0x01;

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Package Type: %s, %d die, %s"), MemoryInfo->specific.DDR4SDRAM.SDRAMPkgType.monolithic ? _T("Monolithic") : _T("Non-monolithic"), MemoryInfo->specific.DDR4SDRAM.SDRAMPkgType.dieCount, MemoryInfo->specific.DDR4SDRAM.SDRAMPkgType.multiLoadStack ? _T("Multi load stack") : _T("Single load stack"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (((spdData[7] >> 4) & 0x03) == 0x00)
            MemoryInfo->specific.DDR4SDRAM.tMAW = 8192;
        else if (((spdData[7] >> 4) & 0x03) == 0x01)
            MemoryInfo->specific.DDR4SDRAM.tMAW = 4096;
        else if (((spdData[7] >> 4) & 0x03) == 0x02)
            MemoryInfo->specific.DDR4SDRAM.tMAW = 2048;

        _stprintf_s(g_szSITempDebug1, _T("Maximum Activate Window (tMAW) in units of tREFI: %d"), MemoryInfo->specific.DDR4SDRAM.tMAW);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _tcscpy(MemoryInfo->specific.DDR4SDRAM.maxActivateCount, MAX_ACTIVATE_COUNT_TABLE[spdData[7] & 0x0F]);

        _stprintf_s(g_szSITempDebug1, _T("Maximum Activate Count (MAC): %s"), MemoryInfo->specific.DDR4SDRAM.maxActivateCount);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.PPRSupported = ((spdData[9] >> 6) & 0x03) == 0x01;
        _stprintf_s(g_szSITempDebug1, _T("Post Package Repair (PPR) supported: %s"), MemoryInfo->specific.DDR4SDRAM.PPRSupported ? _T("Yes") : _T("No"));

        MemoryInfo->specific.DDR4SDRAM.SoftPPRSupported = ((spdData[9] >> 5) & 0x01) == 0x01;
        _stprintf_s(g_szSITempDebug1, _T("Soft Post Package Repair (PPR) supported: %s"), MemoryInfo->specific.DDR4SDRAM.SoftPPRSupported ? _T("Yes") : _T("No"));

        MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.monolithic = (spdData[10] & 0x80) == 0;
        MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.dieCount = ((spdData[10] >> 4) & 0x07) + 1;
        MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.DRAMDensityRatio = ((spdData[10] >> 2) & 0x03);
        MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.multiLoadStack = (spdData[10] & 0x03) == 0x01;

        _stprintf_s(g_szSITempDebug1, _T("Secondary SDRAM Package Type: %s, %d die, DRAM Density Ratio: %d, %s"), MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.monolithic ? _T("Monolithic") : _T("Non-monolithic"), MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.dieCount, MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.DRAMDensityRatio, MemoryInfo->specific.DDR4SDRAM.SecSDRAMPkgType.multiLoadStack ? _T("Multi load stack") : _T("Single load stack"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // ecc
        if ((spdData[13] >> 3) & 0x03)
            MemoryInfo->ecc = true;
        else
            MemoryInfo->ecc = false;

        _stprintf_s(g_szSITempDebug1, _T("ECC: %s"), MemoryInfo->ecc ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Speed
        ftb = 0;
        if ((spdData[17] & 0x3) == 0)
            ftb = TO_TIMING_VAR(1 / 1000);

        if (ftb == 0)
        {
            SysInfo_DebugLogWriteLine(_T("Fine time base not valid."));
            return false;
        }

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Fine time base (ns): "), ftb);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        mtb = 0;
        if (((spdData[17] >> 2) & 0x3) == 0)
            mtb = TO_TIMING_VAR(1 / 8);

        if (mtb == 0)
        {
            SysInfo_DebugLogWriteLine(_T("Medium time base not valid."));
            return false;
        }

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Medium time base (ns): "), mtb);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tCK = (spdData[18] * mtb) + ((char)spdData[125] * ftb);

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum clock cycle time (ns): "), MemoryInfo->tCK);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.tCKmax = (spdData[19] * mtb) + ((char)spdData[124] * ftb);

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum clock cycle time (ns): "), MemoryInfo->specific.DDR4SDRAM.tCKmax);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        ddrclk = TO_TIMING_VAR((2 * 1000) / MemoryInfo->tCK);
        MemoryInfo->txWidth = 1 << ((spdData[13] & 0x07) + 3);
        pcclk = (int)TO_TIMING_VAR((2 * 1000 * MemoryInfo->txWidth) / (8 * MemoryInfo->tCK));
        MemoryInfo->clkspeed = TO_TIMING_VAR(1000 / (MemoryInfo->tCK));

        // Adjust for rounding errors
        if ((int)MemoryInfo->clkspeed == 1066)
        {
#ifdef WIN32
            MemoryInfo->clkspeed = 1000 / 0.9375f;
#else
            MemoryInfo->clkspeed = 1067;
#endif
        }
        else if ((int)MemoryInfo->clkspeed == 933)
        {
#ifdef WIN32
            MemoryInfo->clkspeed = 1000 / (15.0f / 14.0f);
#else
            MemoryInfo->clkspeed = 933;
#endif
        }

        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        pcclk = (pcclk / 100) * 100;
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MT/s (PC4-%d)"), (int)ddrclk, pcclk);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("PC4-%d"), pcclk);
        wcscpy(MemoryInfo->szModulespeed, g_szSITempDebug1);

        // latencies
        CASBitmap = (spdData[23] << 24) + (spdData[22] << 16) + (spdData[21] << 8) + spdData[20];
        _stprintf_s(MemoryInfo->CASSupported, _T(""));

        for (i = 0; i < 32; i++)
        {
            if (CASBitmap & (1 << i))
            {
                TCHAR strCAS[8];
                _stprintf_s(strCAS, _T("%d"), i + 7);
                _tcscat(MemoryInfo->CASSupported, strCAS);
                _tcscat(MemoryInfo->CASSupported, _T(" "));
            }
        }
        _stprintf_s(g_szSITempDebug1, _T("Supported CAS: %s"), MemoryInfo->CASSupported);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // more timing information
        MemoryInfo->tAA = (spdData[24] * mtb) + ((char)spdData[123] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = (spdData[25] * mtb) + ((char)spdData[122] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS to CAS delay time (tRCD): "), MemoryInfo->tRCD);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = (spdData[26] * mtb) + ((char)spdData[121] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum row precharge time (tRP): "), MemoryInfo->tRP);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRAS = ((((spdData[27] & 0x0F) << 8) + spdData[28]) * mtb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum active to precharge time (tRAS): "), MemoryInfo->tRAS);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        taa = (int)CEIL(MemoryInfo->tAA, MemoryInfo->tCK);
        trcd = (int)CEIL(MemoryInfo->tRCD, MemoryInfo->tCK);
        trp = (int)CEIL(MemoryInfo->tRP, MemoryInfo->tCK);
        tras = (int)CEIL(MemoryInfo->tRAS, MemoryInfo->tCK);

#ifdef WIN32
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#else
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#endif
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRC = (((((spdData[27] >> 4) & 0x0F) << 8) + spdData[29]) * mtb) + ((char)spdData[120] * ftb);
        ;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Active to Auto-Refresh Delay (tRC): "), MemoryInfo->tRC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRFC = (((spdData[31] << 8) + spdData[30]) * mtb); // <km>: Conflict in standard?
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC): "), MemoryInfo->tRFC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.tRFC2 = (((spdData[33] << 8) + spdData[32]) * mtb); // <km>: Conflict in standard?
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC2): "), MemoryInfo->specific.DDR4SDRAM.tRFC2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.tRFC4 = (((spdData[35] << 8) + spdData[34]) * mtb); // <km>: Conflict in standard?
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC4): "), MemoryInfo->specific.DDR4SDRAM.tRFC4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.tFAW = ((((spdData[36] & 0x0F) << 8) + spdData[37]) * mtb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Four Activate Window Delay (tFAW): "), MemoryInfo->specific.DDR4SDRAM.tFAW);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRRD = MemoryInfo->specific.DDR4SDRAM.tRRD_S = (spdData[38] * mtb) + ((char)spdData[119] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Activate to Activate Delay Time, different bank group (tRRD_S): "), MemoryInfo->specific.DDR4SDRAM.tRRD_S);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.tRRD_L = (spdData[39] * mtb) + ((char)spdData[118] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Activate to Activate Delay Time, same bank group (tRRD_L): "), MemoryInfo->specific.DDR4SDRAM.tRRD_L);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.tCCD_L = (spdData[40] * mtb) + ((char)spdData[117] * ftb);
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS to CAS Delay Time, same bank group (tCCD_Lmin): "), MemoryInfo->specific.DDR4SDRAM.tCCD_L);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // miscellaneous stuff

        _stprintf_s(MemoryInfo->moduleVoltage, _T(""));

        if (spdData[11] & 1)
        {
            _tcscat(MemoryInfo->moduleVoltage, _T("1.2V"));
        }
        else if (spdData[11] & 2)
        {
            _tcscat(MemoryInfo->moduleVoltage, _T("1.2V tolerant"));
        }

        if (spdData[11] & 4)
        {
            if (MemoryInfo->moduleVoltage[0])
                _tcscat(MemoryInfo->moduleVoltage, _T(", "));
            _tcscat(MemoryInfo->moduleVoltage, _T("TBD1 V"));
        }
        else if (spdData[11] & 8)
        {
            if (MemoryInfo->moduleVoltage[0])
                _tcscat(MemoryInfo->moduleVoltage, _T(", "));
            _tcscat(MemoryInfo->moduleVoltage, _T("TBD1 V tolerant"));
        }

        if (spdData[11] & 0x10)
        {
            if (MemoryInfo->moduleVoltage[0])
                _tcscat(MemoryInfo->moduleVoltage, _T(", "));
            _tcscat(MemoryInfo->moduleVoltage, _T("TBD2 V"));
        }
        else if (spdData[11] & 0x20)
        {
            if (MemoryInfo->moduleVoltage[0])
                _tcscat(MemoryInfo->moduleVoltage, _T(", "));
            _tcscat(MemoryInfo->moduleVoltage, _T("TBD2 V tolerant"));
        }

        _stprintf_s(g_szSITempDebug1, _T("Operable voltages: %s"), MemoryInfo->moduleVoltage);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.thermalSensorPresent = (spdData[14] & 0x80) != 0;
        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensor present: %s"), MemoryInfo->specific.DDR4SDRAM.thermalSensorPresent ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR4SDRAM.BaseCfgCRC16 = (spdData[127] << 8) | spdData[126];
        _stprintf_s(g_szSITempDebug1, _T("Base Configuration Section CRC16: 0x%04X"), MemoryInfo->specific.DDR4SDRAM.BaseCfgCRC16);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // Unbuffered, Registered, Load Reduced Memory
        if (spdData[3] == 2 || spdData[3] == 3 || spdData[3] == 1 || spdData[3] == 4)
        {
            MemoryInfo->specific.DDR4SDRAM.moduleHeight = (spdData[128] & 0x1F) + 15;
            _stprintf_s(g_szSITempDebug1, _T("Module Height (mm): %d - %d"), MemoryInfo->specific.DDR4SDRAM.moduleHeight - 1, MemoryInfo->specific.DDR4SDRAM.moduleHeight);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR4SDRAM.moduleThicknessFront = (spdData[129] & 0xF) + 1;
            MemoryInfo->specific.DDR4SDRAM.moduleThicknessBack = ((spdData[129] >> 4) & 0xF) + 1;
            _stprintf_s(g_szSITempDebug1, _T("Module Thickness (mm): front %d-%d , back %d-%d "), MemoryInfo->specific.DDR4SDRAM.moduleThicknessFront - 1, MemoryInfo->specific.DDR4SDRAM.moduleThicknessFront, MemoryInfo->specific.DDR4SDRAM.moduleThicknessBack - 1, MemoryInfo->specific.DDR4SDRAM.moduleThicknessBack);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if ((spdData[130] & 0x1F) == 0x1F)
            {
                _stprintf_s(MemoryInfo->specific.DDR4SDRAM.moduleRefCard, _T("Raw Card ZZ"));
            }
            else
            {
                int cardRev = ((spdData[130] >> 5) & 0x03) + spdData[128];

                if ((spdData[130] & 0x80) == 0)
                {
                    static const TCHAR *REF_RAW_CARD_TABLE[] = {_T("A"),
                                                                _T("B"),
                                                                _T("C"),
                                                                _T("D"),
                                                                _T("E"),
                                                                _T("F"),
                                                                _T("G"),
                                                                _T("H"),
                                                                _T("J"),
                                                                _T("K"),
                                                                _T("L"),
                                                                _T("M"),
                                                                _T("N"),
                                                                _T("P"),
                                                                _T("R"),
                                                                _T("T"),
                                                                _T("U"),
                                                                _T("V"),
                                                                _T("W"),
                                                                _T("Y"),
                                                                _T("AA"),
                                                                _T("AB"),
                                                                _T("AC"),
                                                                _T("AD"),
                                                                _T("AE"),
                                                                _T("AF"),
                                                                _T("AG"),
                                                                _T("AH"),
                                                                _T("AJ"),
                                                                _T("AK"),
                                                                _T("AL")};

                    _stprintf_s(MemoryInfo->specific.DDR4SDRAM.moduleRefCard, _T("Raw Card %s Rev. %d"), REF_RAW_CARD_TABLE[spdData[130] & 0x1F], cardRev);
                }
                else
                {
                    static const TCHAR *REF_RAW_CARD_TABLE[] = {
                        _T("AM"),
                        _T("AN"),
                        _T("AP"),
                        _T("AR"),
                        _T("AT"),
                        _T("AU"),
                        _T("AV"),
                        _T("AW"),
                        _T("AY"),
                        _T("BA"),
                        _T("BB"),
                        _T("BC"),
                        _T("BD"),
                        _T("BE"),
                        _T("BF"),
                        _T("BG"),
                        _T("BH"),
                        _T("BJ"),
                        _T("BK"),
                        _T("BL"),
                        _T("BM"),
                        _T("BN"),
                        _T("BP"),
                        _T("BR"),
                        _T("BT"),
                        _T("BU"),
                        _T("BV"),
                        _T("BW"),
                        _T("BY"),
                        _T("CA"),
                        _T("CB")};

                    _stprintf_s(MemoryInfo->specific.DDR4SDRAM.moduleRefCard, _T("Raw Card %s Rev. %d"), REF_RAW_CARD_TABLE[spdData[130] & 0x1F], cardRev);
                }
            }
            _stprintf_s(g_szSITempDebug1, _T("Module reference card: %s"), MemoryInfo->specific.DDR4SDRAM.moduleRefCard);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        // Registered memory
        if (spdData[3] == 1 || spdData[3] == 4)
        {
            MemoryInfo->specific.DDR4SDRAM.numRegisters = spdData[131] & 0x03;
            _stprintf_s(g_szSITempDebug1, _T("# Registers: %d"), MemoryInfo->specific.DDR4SDRAM.numRegisters);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR4SDRAM.numDRAMRows = (spdData[131] >> 2) & 0x03;
            _stprintf_s(g_szSITempDebug1, _T("# DRAM Rows: %d"), MemoryInfo->specific.DDR4SDRAM.numDRAMRows);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR4SDRAM.heatSpreaderSolution = (spdData[131] & 0x80) != 0;
            _stprintf_s(g_szSITempDebug1, _T("Heat spreader solution incorporated: %s"), MemoryInfo->specific.DDR4SDRAM.heatSpreaderSolution ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            numCont = spdData[133] & 0x7F;
            MemoryInfo->specific.DDR4SDRAM.regManufBank = numCont + 1;
            MemoryInfo->specific.DDR4SDRAM.regManufID = spdData[134];

            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR4SDRAM.regManufID & 0x7F, numCont, MemoryInfo->specific.DDR4SDRAM.regManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("Register Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->specific.DDR4SDRAM.regManuf, MemoryInfo->specific.DDR4SDRAM.regManufBank, MemoryInfo->specific.DDR4SDRAM.regManufID);

            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR4SDRAM.regRev = spdData[135];
            _stprintf_s(g_szSITempDebug1, _T("Register revision: 0x%02X"), MemoryInfo->specific.DDR4SDRAM.regRev);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.CKEDriveStrength, DRIVE_STRENGTH_TABLE[spdData[137] & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("CKE Drive Strength: %s"), MemoryInfo->specific.DDR4SDRAM.CKEDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTDriveStrength, DRIVE_STRENGTH_TABLE[(spdData[137] >> 2) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("ODT Drive Strength: %s"), MemoryInfo->specific.DDR4SDRAM.ODTDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.cmdAddrDriveStrength, DRIVE_STRENGTH_TABLE[(spdData[137] >> 4) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Command/Address Drive Strength: %s"), MemoryInfo->specific.DDR4SDRAM.cmdAddrDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.CSDriveStrength, DRIVE_STRENGTH_TABLE[(spdData[137] >> 6) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Chip Select Drive Strength: %s"), MemoryInfo->specific.DDR4SDRAM.CSDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.Y0Y2DriveStrength, DRIVE_STRENGTH_TABLE[spdData[138] & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Y0/Y2 Output Drive Strength: %s"), MemoryInfo->specific.DDR4SDRAM.cmdAddrDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.Y1Y3DriveStrength, DRIVE_STRENGTH_TABLE[(spdData[138] >> 2) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("Y1/Y3 Output Drive Strength: %s"), MemoryInfo->specific.DDR4SDRAM.cmdAddrDriveStrength);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        // Load Reduced Memory
        if (spdData[3] == 4)
        {
            MemoryInfo->specific.DDR4SDRAM.databufferRev = spdData[139];
            _stprintf_s(g_szSITempDebug1, _T("Data Buffer revision: 0x%02X"), MemoryInfo->specific.DDR4SDRAM.databufferRev);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.MDQDriveStrength1866, MDQ_DRIVE_STRENGTH_TABLE[spdData[145] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.MDQReadTermStrength1866, MDQ_READ_TERM_STRENGTH_TABLE[(spdData[145] >> 4) & 0x7]);

            _stprintf_s(g_szSITempDebug1, _T("Data Buffer MDQ Drive Strength and RTT for data rate < 1866: (Drive Strength) %s, (RTT) %s"), MemoryInfo->specific.DDR4SDRAM.MDQDriveStrength1866, MemoryInfo->specific.DDR4SDRAM.MDQReadTermStrength1866);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.DRAMDriveStrength1866, DRAM_DRIVE_STRENGTH_TABLE[spdData[148] & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("DRAM Drive Strength for data rate < 1866: %s"), MemoryInfo->specific.DDR4SDRAM.DRAMDriveStrength1866);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttNom1866, NOM_ODT_DRIVE_TABLE[spdData[149] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttWR1866, DYN_ODT_DRIVE_TABLE[(spdData[149] >> 3) & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttPARK1866_R0R1, PARK_ODT_DRIVE_TABLE[spdData[152] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttPARK1866_R2R3, PARK_ODT_DRIVE_TABLE[(spdData[152] >> 3) & 0x7]);

            _stprintf_s(g_szSITempDebug1, _T("DRAM ODT for data rate < 1866: (RTT_NOM) %s, (RTT_WR) %s, (RTT_PARK Rank 0,1) %s, (RTT_PARK Rank 2,3) %s"), MemoryInfo->specific.DDR4SDRAM.ODTRttNom1866, MemoryInfo->specific.DDR4SDRAM.ODTRttWR1866, MemoryInfo->specific.DDR4SDRAM.ODTRttPARK1866_R0R1, MemoryInfo->specific.DDR4SDRAM.ODTRttPARK1866_R2R3);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.MDQDriveStrength2400, MDQ_DRIVE_STRENGTH_TABLE[spdData[146] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.MDQReadTermStrength2400, MDQ_READ_TERM_STRENGTH_TABLE[(spdData[146] >> 4) & 0x7]);

            _stprintf_s(g_szSITempDebug1, _T("Data Buffer MDQ Drive Strength and RTT for 1866 < data rate <= 2400: (Drive Strength) %s, (RTT) %s"), MemoryInfo->specific.DDR4SDRAM.MDQDriveStrength2400, MemoryInfo->specific.DDR4SDRAM.MDQReadTermStrength2400);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.DRAMDriveStrength2400, DRAM_DRIVE_STRENGTH_TABLE[(spdData[148] >> 2) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("DRAM Drive Strength for 1866 < data rate <= 2400: %s"), MemoryInfo->specific.DDR4SDRAM.DRAMDriveStrength2400);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttNom2400, NOM_ODT_DRIVE_TABLE[spdData[150] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttWR2400, DYN_ODT_DRIVE_TABLE[(spdData[150] >> 3) & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttPARK2400_R0R1, PARK_ODT_DRIVE_TABLE[spdData[153] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttPARK2400_R2R3, PARK_ODT_DRIVE_TABLE[(spdData[153] >> 3) & 0x7]);

            _stprintf_s(g_szSITempDebug1, _T("DRAM ODT for 1866 < data rate <= 2400: (RTT_NOM) %s, (RTT_WR) %s, (RTT_PARK Rank 0,1) %s, (RTT_PARK Rank 2,3) %s"), MemoryInfo->specific.DDR4SDRAM.ODTRttNom2400, MemoryInfo->specific.DDR4SDRAM.ODTRttWR2400, MemoryInfo->specific.DDR4SDRAM.ODTRttPARK2400_R0R1, MemoryInfo->specific.DDR4SDRAM.ODTRttPARK2400_R2R3);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.MDQDriveStrength3200, MDQ_DRIVE_STRENGTH_TABLE[spdData[147] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.MDQReadTermStrength3200, MDQ_READ_TERM_STRENGTH_TABLE[(spdData[147] >> 4) & 0x7]);

            _stprintf_s(g_szSITempDebug1, _T("Data Buffer MDQ Drive Strength and RTT for 2400 < data rate <= 3200: (Drive Strength) %s, (RTT) %s"), MemoryInfo->specific.DDR4SDRAM.MDQDriveStrength3200, MemoryInfo->specific.DDR4SDRAM.MDQReadTermStrength3200);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.DRAMDriveStrength3200, DRAM_DRIVE_STRENGTH_TABLE[(spdData[148] >> 4) & 0x3]);

            _stprintf_s(g_szSITempDebug1, _T("DRAM Drive Strength for 2400 < data rate <= 3200: %s"), MemoryInfo->specific.DDR4SDRAM.DRAMDriveStrength3200);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttNom3200, NOM_ODT_DRIVE_TABLE[spdData[151] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttWR3200, DYN_ODT_DRIVE_TABLE[(spdData[151] >> 3) & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttPARK3200_R0R1, PARK_ODT_DRIVE_TABLE[spdData[154] & 0x7]);
            _tcscpy(MemoryInfo->specific.DDR4SDRAM.ODTRttPARK3200_R2R3, PARK_ODT_DRIVE_TABLE[(spdData[154] >> 3) & 0x7]);

            _stprintf_s(g_szSITempDebug1, _T("DRAM ODT for 2400 < data rate <= 3200: (RTT_NOM) %s, (RTT_WR) %s, (RTT_PARK Rank 0,1) %s, (RTT_PARK Rank 2,3) %s"), MemoryInfo->specific.DDR4SDRAM.ODTRttNom3200, MemoryInfo->specific.DDR4SDRAM.ODTRttWR3200, MemoryInfo->specific.DDR4SDRAM.ODTRttPARK3200_R0R1, MemoryInfo->specific.DDR4SDRAM.ODTRttPARK3200_R2R3);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        MemoryInfo->specific.DDR4SDRAM.ModuleCRC16 = (spdData[255] << 8) | spdData[254];
        _stprintf_s(g_szSITempDebug1, _T("Module Specific Section CRC16: 0x%04X"), MemoryInfo->specific.DDR4SDRAM.ModuleCRC16);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // XMP
        if (spdData[384] == SPD_DDR4_XMP2_ID_1 && spdData[385] == SPD_DDR4_XMP2_ID_2)
        {
            MemoryInfo->specific.DDR4SDRAM.XMPSupported = true;
            _stprintf_s(g_szSITempDebug1, _T("XMP supported: Yes"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR4SDRAM.XMP.revision = spdData[387];
            _stprintf_s(g_szSITempDebug1, _T("XMP revision: %02X"), MemoryInfo->specific.DDR4SDRAM.XMP.revision);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            // Check if v2.x revision
            if ((MemoryInfo->specific.DDR4SDRAM.XMP.revision & 0xF0) == 0x20)
            {
                // Profile 1 and 2 bytes
                for (i = 0; i < 2; i++)
                {
                    int j;

                    if (i == 0)
                    {
                        MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].enabled = (spdData[386] & 0x1) != 0;
                        _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] Enthusiast / Certified Profile enabled: %s"), i + 1, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].enabled ? _T("Yes") : _T("No"));
                        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                        if (MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].enabled == false)
                            continue;

                        MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].dimmsPerChannel = ((spdData[386] >> 2) & 0x03) + 1;
                        _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] recommended DIMMs per channel: %d"), i + 1, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].dimmsPerChannel);
                        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                        if ((spdData[388] & 0x03) == 0)
                            ftb = TO_TIMING_VAR(1 / 1000);
                        if (((spdData[388] >> 2) & 0x03) == 0)
                            mtb = TO_TIMING_VAR(125 / 1000);
                    }
                    else
                    {
                        MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].enabled = (spdData[386] & 0x2) != 0;
                        _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] Extreme Profile enabled: %s"), i + 1, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].enabled ? _T("Yes") : _T("No"));
                        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                        if (MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].enabled == false)
                            continue;

                        MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].dimmsPerChannel = ((spdData[386] >> 4) & 0x03) + 1;
                        _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] recommended DIMMs per channel: %d"), i + 1, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].dimmsPerChannel);
                        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                        if ((spdData[389] & 0x03) == 0)
                            ftb = TO_TIMING_VAR(1 / 1000);
                        if (((spdData[389] >> 2) & 0x03) == 0)
                            mtb = TO_TIMING_VAR(125 / 1000);
                    }
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Fine time base: "), TO_TIMING_VAR(ftb));
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Medium time base: "), TO_TIMING_VAR(mtb));
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].moduleVdd, _T("%.2fV"), (float)(((spdData[393 + i * 47] >> 7) & 0x01) + (spdData[393 + i * 47] & 0x7F) * 0.01f));
#else
                    {
                        int millivolts = ((spdData[393 + i * 47] >> 7) & 0x01) * 1000 + (spdData[393 + i * 47] & 0x7F) * 10;
                        getVoltageStr(millivolts, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].moduleVdd, sizeof(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].moduleVdd));
                    }
#endif

                    _stprintf_s(g_szSITempDebug1, _T("  Voltage: %s"), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].moduleVdd);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tCK = (spdData[396 + i * 47] * mtb) + ((char)spdData[431 + i * 47] * ftb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum SDRAM Cycle Time (tCKmin): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tCK);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].clkspeed = TO_TIMING_VAR(1000 / MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tCK);
                    _stprintf_s(g_szSITempDebug1, _T("  Maximum clock speed: %d MHz"), (int)MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].clkspeed);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    // latencies
                    CASBitmap = (spdData[400 + i * 47] << 24) + (spdData[399 + i * 47] << 16) + (spdData[398 + i * 47] << 8) + spdData[397 + i * 47];
                    _stprintf_s(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].CASSupported, _T(""));

                    for (j = 0; j <= 17; j++)
                    {
                        if (CASBitmap & (1 << j))
                        {
                            TCHAR strCAS[8];
                            _stprintf_s(strCAS, _T("%d"), j + 7);
                            _tcscat(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].CASSupported, strCAS);
                            _tcscat(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].CASSupported, _T(" "));
                        }
                    }
                    _stprintf_s(g_szSITempDebug1, _T("  Supported CAS: %s"), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].CASSupported);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tAA = (spdData[401 + i * 47] * mtb) + ((char)spdData[430 + i * 47] * ftb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum CAS Latency Time (tAAmin): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tAA);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRCD = (spdData[402 + i * 47] * mtb) + ((char)spdData[429 + i * 47] * ftb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum RAS# to CAS# Delay Time (tRCDmin): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRCD);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRP = (spdData[403 + i * 47] * mtb) + ((char)spdData[428 + i * 47] * ftb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Row Precharge Delay Time (tRPmin): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRP);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRAS = ((((spdData[404 + i * 47] & 0x0F) << 8) + spdData[405 + i * 47]) * mtb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Precharge Delay Time (tRASmin): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRAS);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRC = (((((spdData[404 + i * 47] >> 4) & 0x0F) << 8) + spdData[406 + i * 47]) * mtb) + ((char)spdData[427 + i * 47] * ftb);
                    ;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Active/Refresh Delay Time (tRCmin): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRC);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRFC1 = (((spdData[408 + i * 47] << 8) + spdData[407 + i * 47]) * mtb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFC1min): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRFC1);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRFC2 = (((spdData[410 + i * 47] << 8) + spdData[409 + i * 47]) * mtb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFC2min): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRFC2);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRFC4 = (((spdData[412 + i * 47] << 8) + spdData[411 + i * 47]) * mtb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFC4min): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRFC4);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tFAW = ((((spdData[413 + i * 47] & 0x0F) << 8) + spdData[414 + i * 47]) * mtb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Four Activate Window Delay Time (tFAWmin): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tFAW);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRRD_S = (spdData[415 + i * 47] * mtb) + ((char)spdData[426 + i * 47] * ftb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Activate to Activate Delay Time, different bank group (tRRD_S): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRRD_S);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRRD_L = (spdData[416 + i * 47] * mtb) + ((char)spdData[425 + i * 47] * ftb);
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Activate to Activate Delay Time, same bank group (tRRD_L): "), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRRD_L);
                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

                    taa = (int)CEIL(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tAA, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tCK);
                    trcd = (int)CEIL(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRCD, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tCK);
                    trp = (int)CEIL(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRP, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tCK);
                    tras = (int)CEIL(MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tRAS, MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].tCK);

#ifdef WIN32
                    _stprintf_s(g_szSITempDebug1, _T("  Supported timing at highest clock speed (%.1f MHz) : %d-%d-%d-%d"), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].clkspeed, taa, trcd, trp, tras);
#else
                    _stprintf_s(g_szSITempDebug1, _T("  Supported timing at highest clock speed (%d MHz) : %d-%d-%d-%d"), MemoryInfo->specific.DDR4SDRAM.XMP.profile[i].clkspeed, taa, trcd, trp, tras);
#endif

                    SysInfo_DebugLogWriteLine(g_szSITempDebug1);
                }
            }
        }
    }
    break;

    case DDR510:
    {
#ifdef WIN32
        float tb = 0.001f;
        float ddrclk;
#else
        int tb = 1;
        int ddrclk;
#endif
        int channels;
        int buswidth;
        int iowidth;
        int dieperpkg;
        unsigned long long density;
        int ranks;
        int asymmetrical;

        unsigned long long capacity;
        int pcclk;

        int taa;
        int trcd;
        int trp;
        int tras;

        // manufacture information
        // get the number of continuation codes
        unsigned char numCont;

        // Module Type
        MemoryInfo->specific.DDR5SDRAM.keyByteModuleType.raw = spdData[3];
        _tcscpy(MemoryInfo->specific.DDR5SDRAM.moduleType, DDR5_MODULE_TYPES_TABLE[MemoryInfo->specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType]);

        if (MemoryInfo->specific.DDR5SDRAM.keyByteModuleType.bits.hybridMedia == 0x1)
        {
            _tcscat(MemoryInfo->specific.DDR5SDRAM.moduleType, _T(", NVDIMM-N Hybrid"));
        }
        else if (MemoryInfo->specific.DDR5SDRAM.keyByteModuleType.bits.hybridMedia == 0x2)
        {
            _tcscat(MemoryInfo->specific.DDR5SDRAM.moduleType, _T(", NVDIMM-P Hybrid"));
        }

        _stprintf_s(g_szSITempDebug1, _T("Module type: %s"), MemoryInfo->specific.DDR5SDRAM.moduleType);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.DDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0x01) // RDIMM
            MemoryInfo->registered = true;
        else
            MemoryInfo->registered = false;

        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMDensityPackage.raw = spdData[4];
        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAddressing.raw = spdData[5];
        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMIOWidth.raw = spdData[6];
        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMBankGroups.raw = spdData[7];

        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMDensityPackage.raw = spdData[8];
        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAddressing.raw = spdData[9];
        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMIOWidth.raw = spdData[10];
        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMBankGroups.raw = spdData[11];

        // Size computation
        // Capacity in bytes =
        // 	Number of channels per DIMM *
        // 	Primary bus width per channel / SDRAM I/O Width *
        // 	Die per package *
        // 	SDRAM density per die / 8 *
        // 	Package ranks per channel

        channels = 1 << ((spdData[235] >> 5) & 0x3);
        buswidth = 1 << ((spdData[235] & 0x07) + 3);
        iowidth = 1 << (((spdData[6] >> 5) & 0x7) + 2);
        dieperpkg = ((spdData[4] >> 5) & 0x7);
        dieperpkg = dieperpkg == 0 ? 1 : (1 << (dieperpkg - 1));

        static const int SDRAM_DENSITY_PER_DIE[32] = {0, 4, 8, 12, 16, 24, 32, 48, 64};

#ifdef WIN32
        density = ((unsigned long long)(SDRAM_DENSITY_PER_DIE[spdData[4] & 0x1F]) << 30) / 8;
#else
        density = DivU64x32(LShiftU64(SDRAM_DENSITY_PER_DIE[spdData[4] & 0x1F], 30), 8);
#endif
        ranks = ((spdData[234] >> 3) & 0x07) + 1;
        asymmetrical = (spdData[234] >> 6) & 0x1;

        if (!asymmetrical)
        {
#ifdef WIN32
            capacity = density * channels * (buswidth / iowidth) * dieperpkg * ranks;
#else
            capacity = MultU64x32(density, channels * (buswidth / iowidth) * dieperpkg * ranks);
#endif
            capacity = (capacity >> 20);
        }
        else
        {
            // Capacity in bytes =
            //	Capacity of even ranks(first SDRAM type) +
            //	Capacity of odd ranks(second SDRAM type)

            int iowidth_odd = 1 << (((spdData[10] >> 5) & 0x7) + 2);
            int dieperpkg_odd = ((spdData[8] >> 5) & 0x7);
            dieperpkg_odd = dieperpkg_odd == 0 ? 1 : (1 << (dieperpkg_odd - 1));
#ifdef WIN32
            unsigned long long density_odd = ((unsigned long long)(SDRAM_DENSITY_PER_DIE[spdData[8] & 0x1F]) >> 30) / 8;
#else
            unsigned long long density_odd = DivU64x32(LShiftU64(SDRAM_DENSITY_PER_DIE[spdData[8] & 0x1F], 30), 8);
#endif
            int even_ranks = ranks / 2 + ranks % 2;
            int odd_ranks = ranks / 2;

            int numbanks_odd = 1 << ((spdData[11] & 0x07) + ((spdData[11] >> 5) & 0x7));
            int rowbits_odd = (spdData[9] & 0x1F) + 16;
            int colbits_odd = ((spdData[9] >> 5) & 0x07) + 10;
#ifdef WIN32
            capacity = density * channels * (buswidth / iowidth) * dieperpkg * even_ranks;
            capacity += density_odd * channels * (buswidth / iowidth_odd) * dieperpkg_odd * odd_ranks;

#else
            capacity = MultU64x32(density, channels * (buswidth / iowidth) * dieperpkg * even_ranks);
            capacity += MultU64x32(density_odd, channels * (buswidth / iowidth_odd) * dieperpkg_odd * odd_ranks);
#endif
            _stprintf_s(g_szSITempDebug1, _T("Second SDRAM Banks x Rows x Columns x Bits: %d x %d x %d x %d"),
                        numbanks_odd,
                        rowbits_odd,
                        colbits_odd,
                        buswidth);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("Odd Ranks: %d"), odd_ranks);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("Second SDRAM Device Width: %d"), iowidth_odd);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.SecondSDRAMDensityPackage.bits.diePerPackage == 0)
                _stprintf_s(g_szSITempDebug1, _T("Second SDRAM Package Type: Monolithic SDRAM"));
            else
                _stprintf_s(g_szSITempDebug1, _T("Second SDRAM Package Type: %d die 3DS"), 1 << (MemoryInfo->specific.DDR5SDRAM.SecondSDRAMDensityPackage.bits.diePerPackage - 1));
            MtSupportUCDebugWriteLine(g_szSITempDebug1);
        }

        MemoryInfo->size = (int)capacity;

        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->numBanks = 1 << ((spdData[7] & 0x07) + ((spdData[7] >> 5) & 0x7));
        MemoryInfo->rowAddrBits = (spdData[5] & 0x1F) + 16;
        MemoryInfo->colAddrBits = ((spdData[5] >> 5) & 0x07) + 10;
        MemoryInfo->busWidth = buswidth;
        MemoryInfo->numRanks = ranks;

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"),
                    MemoryInfo->numBanks,
                    MemoryInfo->rowAddrBits,
                    MemoryInfo->colAddrBits,
                    MemoryInfo->busWidth);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), ranks);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = iowidth;

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d"), MemoryInfo->deviceWidth);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.DDR5SDRAM.FirstSDRAMDensityPackage.bits.diePerPackage == 0)
            _stprintf_s(g_szSITempDebug1, _T("SDRAM Package Type: Monolithic SDRAM"));
        else
            _stprintf_s(g_szSITempDebug1, _T("SDRAM Package Type: %d die 3DS"), 1 << (MemoryInfo->specific.DDR5SDRAM.FirstSDRAMDensityPackage.bits.diePerPackage - 1));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.BL32PostPackageRepair.raw = spdData[12];

        _stprintf_s(g_szSITempDebug1, _T("sPPR Granularity: One repair element per %s"), MemoryInfo->specific.DDR5SDRAM.BL32PostPackageRepair.bits.sPPRGranularity ? _T("bank") : _T("bank group"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("sPPR Undo/Lock: %s"), MemoryInfo->specific.DDR5SDRAM.BL32PostPackageRepair.bits.sPPRUndoLock ? _T("supported") : _T("not supported"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Burst length 32: %s"), MemoryInfo->specific.DDR5SDRAM.BL32PostPackageRepair.bits.BL32 ? _T("supported") : _T("not supported"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("MBIST/mPPR: %s"), MemoryInfo->specific.DDR5SDRAM.BL32PostPackageRepair.bits.MBISTmPPR ? _T("supported") : _T("not supported"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("mPPR/hPPR Abort: %s"), MemoryInfo->specific.DDR5SDRAM.BL32PostPackageRepair.bits.mPPRhPPRAbort ? _T("supported") : _T("not supported"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.DutyCycleAdjusterPartialArraySelfRefresh.raw = spdData[13];

        _stprintf_s(g_szSITempDebug1, _T("PASR: %s"), MemoryInfo->specific.DDR5SDRAM.DutyCycleAdjusterPartialArraySelfRefresh.bits.PASR ? _T("supported") : _T("not supported"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("DCA Types Supported: %s"), DDR5_DCA_PASR_TABLE[MemoryInfo->specific.DDR5SDRAM.DutyCycleAdjusterPartialArraySelfRefresh.bits.DCATypesSupported]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.FaultHandling.raw = spdData[14];

        _stprintf_s(g_szSITempDebug1, _T("x4 RMW/ECS Writeback Suppression: %s"), MemoryInfo->specific.DDR5SDRAM.FaultHandling.bits.x4RMWECSWritebackSuppression ? _T("supported") : _T("not supported"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("x4 RMW/ECS Writeback Suppression MR selector: %s"), MemoryInfo->specific.DDR5SDRAM.FaultHandling.bits.x4RMWECSWritebackSuppressionMRSelector ? _T("MR15") : _T("MR9"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Bounded Fault: %s"), MemoryInfo->specific.DDR5SDRAM.FaultHandling.bits.BoundedFault ? _T("supported") : _T("not supported"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.NominalVoltageVDD.raw = spdData[16];
        MemoryInfo->specific.DDR5SDRAM.NominalVoltageVDDQ.raw = spdData[17];
        MemoryInfo->specific.DDR5SDRAM.NominalVoltageVPP.raw = spdData[18];

        _stprintf_s(MemoryInfo->moduleVoltage, MemoryInfo->specific.DDR5SDRAM.NominalVoltageVDD.bits.operable == 0 ? _T("1.1V") : _T(""));

        _stprintf_s(g_szSITempDebug1, _T("Operable voltages: %s"), MemoryInfo->moduleVoltage);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Nominal Voltage, VDD: %s"), MemoryInfo->specific.DDR5SDRAM.NominalVoltageVDD.bits.nominal == 0 ? _T("1.1V") : _T("reserved"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Nominal Voltage, VDDQ: %s"), MemoryInfo->specific.DDR5SDRAM.NominalVoltageVDDQ.bits.nominal == 0 ? _T("1.1V") : _T("reserved"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Nominal Voltage, VPP: %s"), MemoryInfo->specific.DDR5SDRAM.NominalVoltageVPP.bits.nominal == 0 ? _T("1.8V") : _T("reserved"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        // ecc
        if ((spdData[235] >> 3) & 0x03)
            MemoryInfo->ecc = true;
        else
            MemoryInfo->ecc = false;

        _stprintf_s(g_szSITempDebug1, _T("ECC: %s"), MemoryInfo->ecc ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        // Speed
        MemoryInfo->tCK = ((spdData[21] << 8) + spdData[20]) * tb;

        if (MemoryInfo->tCK == 0)
        {
            MtSupportUCDebugWriteLine(_T("tCK not valid."));
            return false;
        }

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum clock cycle time (ns): "), MemoryInfo->tCK);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.tCKmax = ((spdData[23] << 8) + spdData[22]) * tb;

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum clock cycle time (ns): "), MemoryInfo->specific.DDR5SDRAM.tCKmax);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        ddrclk = TO_TIMING_VAR((2 * 1000) / MemoryInfo->tCK);
        if (((int)ddrclk % 100) < 10)
            ddrclk = ((int)ddrclk / 100) * 100;
        MemoryInfo->txWidth = buswidth;
        pcclk = (int)TO_TIMING_VAR((2 * 1000 * 8) / (MemoryInfo->tCK));
        MemoryInfo->clkspeed = TO_TIMING_VAR(1000 / (MemoryInfo->tCK));
        if (((int)MemoryInfo->clkspeed % 100) < 10)
            MemoryInfo->clkspeed = ((int)MemoryInfo->clkspeed / 100) * 100;

        // Adjust for rounding errors
        if ((int)MemoryInfo->clkspeed == 1066)
        {
#ifdef WIN32
            MemoryInfo->clkspeed = 1000 / 0.9375f;
#else
            MemoryInfo->clkspeed = 1067;
#endif
        }
        else if ((int)MemoryInfo->clkspeed == 933)
        {
#ifdef WIN32
            MemoryInfo->clkspeed = 1000 / (15.0f / 14.0f);
#else
            MemoryInfo->clkspeed = 933;
#endif
        }

        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        pcclk = (pcclk / 100) * 100;
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MT/s (PC5-%d)"), (int)ddrclk, pcclk);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("PC5-%d"), pcclk);
        wcscpy(MemoryInfo->szModulespeed, g_szSITempDebug1);

        // latencies
        _stprintf_s(MemoryInfo->CASSupported, _T(""));

        for (i = 0; i <= 4; i++)
        {
            int j;
            for (j = 0; j < 8; j++)
            {
                if (spdData[24 + i] & (1 << j))
                {
                    TCHAR strCAS[8];
                    _stprintf_s(strCAS, _T("%d"), i * 16 + j * 2 + 20);
                    _tcscat(MemoryInfo->CASSupported, strCAS);
                    _tcscat(MemoryInfo->CASSupported, _T(" "));
                }
            }
        }

        _stprintf_s(g_szSITempDebug1, _T("Supported CAS: %s"), MemoryInfo->CASSupported);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        // more timing information
        MemoryInfo->tAA = ((spdData[31] << 8) + spdData[30]) * tb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = ((spdData[33] << 8) + spdData[32]) * tb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS to CAS delay time (tRCD): "), MemoryInfo->tRCD);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = ((spdData[35] << 8) + spdData[34]) * tb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum row precharge time (tRP): "), MemoryInfo->tRP);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->tRAS = ((spdData[37] << 8) + spdData[36]) * tb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum active to precharge time (tRAS): "), MemoryInfo->tRAS);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        // See JESD400-5 DDR5 Serial Presence Detect (SPD) - Rounding Algorithms
        taa = (int)(((MemoryInfo->tAA * 997 / MemoryInfo->tCK) + 1000) / 1000);
        // Systems may prefer to use other methods to calculate CAS Latency settings, including applying the
        // Rounding Algorithm on tAAmin(SPD bytes 30~31) to determine the desired CL setting, rounding up if
        // necessary to ensure that only even nCK values are used,
        taa += taa % 2;
        trcd = (int)(((MemoryInfo->tRCD * 997 / MemoryInfo->tCK) + 1000) / 1000);
        trp = (int)(((MemoryInfo->tRP * 997 / MemoryInfo->tCK) + 1000) / 1000);
        tras = (int)(((MemoryInfo->tRAS * 997 / MemoryInfo->tCK) + 1000) / 1000);

#ifdef WIN32
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#else
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz) : %d-%d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp, tras);
#endif
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->tRC = ((spdData[39] << 8) + spdData[38]) * tb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Active to Auto-Refresh Delay (tRC): "), MemoryInfo->tRC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.tWR = ((spdData[41] << 8) + spdData[40]) * tb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Write Recovery Time (tWR): "), MemoryInfo->specific.DDR5SDRAM.tWR);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->tRFC = ((spdData[43] << 8) + spdData[42]) * tb * 1000;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC): "), MemoryInfo->tRFC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.tRFC2 = ((spdData[45] << 8) + spdData[44]) * tb * 1000;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC2): "), MemoryInfo->specific.DDR5SDRAM.tRFC2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.tRFC4 = ((spdData[47] << 8) + spdData[46]) * tb * 1000;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC4): "), MemoryInfo->specific.DDR5SDRAM.tRFC4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.tRFC1_dlr = ((spdData[49] << 8) + spdData[48]) * tb * 1000;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Refresh Recovery Delay Time, 3DS Different Logical Rank (tRFC1_dlr min): "), MemoryInfo->specific.DDR5SDRAM.tRFC1_dlr);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.tRFC2_dlr = ((spdData[51] << 8) + spdData[50]) * tb * 1000;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Refresh Recovery Delay Time, 3DS Different Logical Rank (tRFC2_dlr min): "), MemoryInfo->specific.DDR5SDRAM.tRFC2_dlr);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.tRFCsb_dlr = ((spdData[53] << 8) + spdData[52]) * tb * 1000;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Refresh Recovery Delay Time, 3DS Different Logical Rank (tRFCsb_dlr min): "), MemoryInfo->specific.DDR5SDRAM.tRFCsb_dlr);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.raw[0] = spdData[54];
        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.raw[1] = spdData[55];

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM RFM Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM RFM Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM RFM Required: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RFMRequired ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM RFM Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMRefreshManagement.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.raw[0] = spdData[56];
        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.raw[1] = spdData[57];

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM RFM Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM RFM Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM RFM Required: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RFMRequired ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM RFM Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMRefreshManagement.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.raw[0] = spdData[58];
        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.raw[1] = spdData[59];

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level A Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level A Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level A supported: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.ARFMLevelSupported ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level A Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelA.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.raw[0] = spdData[60];
        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.raw[1] = spdData[61];

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level A Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level A Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level A supported: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.ARFMLevelSupported ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level A Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelA.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.raw[0] = spdData[62];
        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.raw[1] = spdData[63];

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level B Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level B Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level B supported: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.ARFMLevelSupported ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level B Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelB.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.raw[0] = spdData[64];
        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.raw[1] = spdData[65];

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level B Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level B Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level B supported: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.ARFMLevelSupported ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level B Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelB.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.raw[0] = spdData[66];
        MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.raw[1] = spdData[67];

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level C Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level C Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level C supported: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.ARFMLevelSupported ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("First SDRAM ARFM Level C Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.FirstSDRAMAdaptiveRefreshManagementLevelC.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.raw[0] = spdData[68];
        MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.raw[1] = spdData[69];

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level C Rolling Accumulated ACT Maximum Management Threshold (RAAMMT): %dX (FGR: %dX)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAMMT * 2);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level C Rolling Accumulated ACT Initial Management Threshold (RAAIMT): %d (FGR: %d)"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 8, MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RAAIMT * 4);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level C supported: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.ARFMLevelSupported ? _T("yes") : _T("no"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Second SDRAM ARFM Level C Rolling Accumulated ACT (RAA) Counter Decrement per REF command: %s"), MemoryInfo->specific.DDR5SDRAM.SecondSDRAMAdaptiveRefreshManagementLevelC.bits.RFMRAACounterDecrementPerREFcommand ? _T("RAAIMT / 2") : _T("RAAIMT"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // miscellaneous stuff

        MemoryInfo->specific.DDR5SDRAM.moduleSPDRev = spdData[192];

        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecBank = (spdData[194] & 0x7F) + 1;
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecID = spdData[195];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.deviceType.raw = spdData[196];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.deviceRev = spdData[197];

        _stprintf_s(g_szSITempDebug1, _T("SPD present: %s"), MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecID & 0x7F, (spdData[194] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("SPD Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecBank, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("SPD device type: %s"), SPD_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank = (spdData[198] & 0x7F) + 1;
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID = spdData[199];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.raw = spdData[200];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.deviceRev = spdData[201];

        _stprintf_s(g_szSITempDebug1, _T("PMIC 0 present: %s"), MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID & 0x7F, (spdData[198] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("PMIC 0 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("PMIC 0 device type: %s"), PMIC_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank = (spdData[202] & 0x7F) + 1;
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID = spdData[203];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.raw = spdData[204];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.deviceRev = spdData[205];

        _stprintf_s(g_szSITempDebug1, _T("PMIC 1 present: %s"), MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID & 0x7F, (spdData[202] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("PMIC 1 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("PMIC 1 device type: %s"), PMIC_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank = (spdData[206] & 0x7F) + 1;
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID = spdData[207];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.raw = spdData[208];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.deviceRev = spdData[209];

        _stprintf_s(g_szSITempDebug1, _T("PMIC 2 present: %s"), MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID & 0x7F, (spdData[206] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("PMIC 2 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("PMIC 2 device type: %s"), PMIC_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank = (spdData[210] & 0x7F) + 1;
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID = spdData[211];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.raw = spdData[212];
        MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceRev = spdData[213];

        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensor 0 present: %s"), MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensor 1 present: %s"), MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed1 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID & 0x7F, (spdData[210] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("Thermal Sensors Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank, MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensors device type: %s"), TS_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.moduleHeight = (spdData[230] & 0x1F) + 15;
        _stprintf_s(g_szSITempDebug1, _T("Module Height (mm): %d - %d"), MemoryInfo->specific.DDR5SDRAM.moduleHeight - 1, MemoryInfo->specific.DDR5SDRAM.moduleHeight);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.moduleThicknessFront = (spdData[231] & 0xF) + 1;
        MemoryInfo->specific.DDR5SDRAM.moduleThicknessBack = ((spdData[231] >> 4) & 0xF) + 1;
        _stprintf_s(g_szSITempDebug1, _T("Module Thickness (mm): front %d-%d , back %d-%d "), MemoryInfo->specific.DDR5SDRAM.moduleThicknessFront - 1, MemoryInfo->specific.DDR5SDRAM.moduleThicknessFront, MemoryInfo->specific.DDR5SDRAM.moduleThicknessBack - 1, MemoryInfo->specific.DDR5SDRAM.moduleThicknessBack);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.ReferenceRawCardUsed.raw = spdData[232];

        static const TCHAR *REF_RAW_CARD_TABLE[] = {_T("A"),
                                                    _T("B"),
                                                    _T("C"),
                                                    _T("D"),
                                                    _T("E"),
                                                    _T("F"),
                                                    _T("G"),
                                                    _T("H"),
                                                    _T("J"),
                                                    _T("K"),
                                                    _T("L"),
                                                    _T("M"),
                                                    _T("N"),
                                                    _T("P"),
                                                    _T("R"),
                                                    _T("T"),
                                                    _T("U"),
                                                    _T("V"),
                                                    _T("W"),
                                                    _T("Y"),
                                                    _T("AA"),
                                                    _T("AB"),
                                                    _T("AC"),
                                                    _T("AD"),
                                                    _T("AE"),
                                                    _T("AF"),
                                                    _T("AG"),
                                                    _T("AH"),
                                                    _T("AJ"),
                                                    _T("AK"),
                                                    _T("Reserved"),
                                                    _T("ZZ")};

        _stprintf_s(MemoryInfo->specific.DDR5SDRAM.moduleRefCard, _T("Raw Card %s Rev. %d"), REF_RAW_CARD_TABLE[MemoryInfo->specific.DDR5SDRAM.ReferenceRawCardUsed.bits.ReferenceDesign], MemoryInfo->specific.DDR5SDRAM.ReferenceRawCardUsed.bits.DesignRevision);

        _stprintf_s(g_szSITempDebug1, _T("Module reference card: %s"), MemoryInfo->specific.DDR5SDRAM.moduleRefCard);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.DIMMAttributes.raw = spdData[233];

        _stprintf_s(g_szSITempDebug1, _T("# DRAM Rows: %d"), 1 << (MemoryInfo->specific.DDR5SDRAM.DIMMAttributes.bits.NumDRAMRows - 1));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Heat spreader installed: %s"), MemoryInfo->specific.DDR5SDRAM.DIMMAttributes.bits.HeatSpreader ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Operating Temperature Range: %s"), OPERATING_TEMP_RANGE[MemoryInfo->specific.DDR5SDRAM.DIMMAttributes.bits.OperatingTemperatureRange]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.ModuleOrganization.raw = spdData[234];

        _stprintf_s(g_szSITempDebug1, _T("Rank Mix: %s"), MemoryInfo->specific.DDR5SDRAM.ModuleOrganization.bits.RankMix ? _T("Asymmetrical") : _T("Symmetrical"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Number of Package Ranks per Sub-Channel: %d"), MemoryInfo->specific.DDR5SDRAM.ModuleOrganization.bits.NumPackageRanksPerSubChannel + 1);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.DDR5SDRAM.MemoryChannelBusWidth.raw = spdData[235];

        _stprintf_s(g_szSITempDebug1, _T("Number of Sub-Channels per DIMM: %d"), 1 << MemoryInfo->specific.DDR5SDRAM.MemoryChannelBusWidth.bits.NumSubChannelsPerDIMM);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Bus width extension per Sub-Channel: %d bits"), MemoryInfo->specific.DDR5SDRAM.MemoryChannelBusWidth.bits.BusWidthExtensionPerSubChannel * 4);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Primary bus width per Sub-Channel: %d bits"), 1 << (MemoryInfo->specific.DDR5SDRAM.MemoryChannelBusWidth.bits.PrimaryBusWidthPerSubChannel + 3));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        // Error logging data
        // Bytes 576~639 (0x240~27F) : Manufacturer's Specific Data or Error Logging Data
        // If SPD Byte 0 bit 7 = 0, the content of these bytes(block 9 of the SPD device) contain Manufacturer's Specific
        // Data as defined by the manufacturer.Write Protect for block 9 may be set or reset.
        // If SPD Byte 0 bit 7 = 1, the content of these bytes contain Error Logging Data.Write Protect for Block 9
        // must be reset, allowing writing to this block.

        // Solder Down
        if (spdData[3] == 0x0B)
        {
        }

        // Unbuffered Memory Module Types
        if (spdData[3] == 2 ||
            spdData[3] == 3)
        {
        }

        // Registered and Load Reduced Memory Module Types
        if ((spdData[3] & 0x0F) == 0x1 ||
            (spdData[3] & 0x0F) == 0x4)
        {
            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecBank = (spdData[240] & 0x7F) + 1;
            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecID = spdData[241];
            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.deviceType.raw = spdData[242];
            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.deviceRev = spdData[243];

            _stprintf_s(g_szSITempDebug1, _T("Registering Clock Driver (RCD) present: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecID & 0x7F, (spdData[240] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("RCD Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecBank, MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("RCD device type: %s"), RCD_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RegisteringClockDriver.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecBank = (spdData[244] & 0x7F) + 1;
            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecID = spdData[245];
            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.deviceType.raw = spdData[246];
            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.deviceRev = spdData[247];

            _stprintf_s(g_szSITempDebug1, _T("Data Buffers (DB) present: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecID & 0x7F, (spdData[244] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("Data Buffers Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecBank, MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("Data Buffers device type: %s"), DB_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DataBuffers.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.raw = spdData[248];

            _stprintf_s(g_szSITempDebug1, _T("BCK_t/BCK_c: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.BCK_tBCK_c ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QDCK_t/QDCK_c: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QDCK_tQDCK_c ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QCCK_t/QCCK_c: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QCCK_tQCCK_c ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QBCK_t/QBCK_c: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QBCK_tQBCK_c ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QACK_t/QACK_c: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW08ClockDriverEnable.bits.QACK_tQACK_c ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.raw = spdData[249];

            _stprintf_s(g_szSITempDebug1, _T("QBCS[1:0]_n output: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QBCS10_nOutput ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QACS[1:0]_n output: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QACS10_nOutput ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("Q[B:A]CA13 output driver: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QBACA13OutputDriver ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("BCS_n, BCOM[2:0] & BRST_n outputs: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.BCS_nBCOM20BRST_nOutputs ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("DCS1_n input buffer & QxCS1_n outputs: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.DCS1_nInputBufferQxCS1_nOutputs ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QBCA outputs: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QBCAOutputs ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QACA outputs: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW09OutputAddressControlEnable.bits.QACAOutputs ? _T("enabled") : _T("disabled"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.raw = spdData[250];

            _stprintf_s(g_szSITempDebug1, _T("QDCK_t/QDCK_c: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QDCK_tQDCK_c]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QCCK_t/QCCK_c: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QCCK_tQCCK_c]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QBCK_t/QBCK_c: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QBCK_tQBCK_c]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QACK_t/QACK_c: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0AQCKDriverCharacteristics.bits.QACK_tQACK_c]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0B.raw = spdData[251];

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0CQxCAQxCS_nDriverCharacteristics.raw = spdData[252];

            _stprintf_s(g_szSITempDebug1, _T("Driver Strength QxCS0_n, QxCS1_n, QCCK_t/QCCK_c: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0CQxCAQxCS_nDriverCharacteristics.bits.DriverStrengthQxCS0_nQxCS1_nQCCK_tQCCK_c]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("Driver Strength Address/Command for both A&B outputs: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0CQxCAQxCS_nDriverCharacteristics.bits.DriverStrengthAddressCommandABOutputs]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0DDataBufferInterfaceDriverCharacteristics.raw = spdData[253];

            _stprintf_s(g_szSITempDebug1, _T("Driver Strength BCK_t/BCK_c: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0DDataBufferInterfaceDriverCharacteristics.bits.DriverStrengthBCK_tBCK_c]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("Driver Strength BCOM[2:0], BCS_n: %s"), QCK_DRIVER_CHARACTERISTICS_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0DDataBufferInterfaceDriverCharacteristics.bits.DriverStrengthBCOM20BCS_n]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0EQCKQCAQCOutputSlewRate.raw = spdData[254];

            _stprintf_s(g_szSITempDebug1, _T("Q[B:A]CS[1:0]_n Single Ended Slew Rate: %s"), SINGLE_ENDED_SLEW_RATE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0EQCKQCAQCOutputSlewRate.bits.QBACS10_nSingleEndedSlewRate]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("Q[B:A]CA[13:0] Single Ended Slew Rate: %s"), SINGLE_ENDED_SLEW_RATE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0EQCKQCAQCOutputSlewRate.bits.QBACA130SingleEndedSlewRate]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("QCK[D:A]_t/QCK[D:A]_c Differential Slew Rate: %s"), DIFFERENTIAL_SLEW_RATE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0EQCKQCAQCOutputSlewRate.bits.QCKDA_tQCKDA_cDifferentialSlewRate]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0FBCKBCOMBCSOutputSlewRate.raw = spdData[255];

            _stprintf_s(g_szSITempDebug1, _T("BCK_t/BCK_c Differential Slew Rate: %s"), DIFFERENTIAL_SLEW_RATE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0FBCKBCOMBCSOutputSlewRate.bits.BCK_tBCK_cDifferentialSlewRate]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            _stprintf_s(g_szSITempDebug1, _T("BCOM[2:0], BCS_n Single Ended slew rate: %s"), RCDRW0F_SINGLE_ENDED_SLEW_RATE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.RCDRW0FBCKBCOMBCSOutputSlewRate.bits.BCOM20BCS_nSingleEndedSlewRate]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DBRW86DQSRTTParkTermination.raw = spdData[256];

            _stprintf_s(g_szSITempDebug1, _T("DQS RTT Park Termination: %s"), DQS_RTT_PARK_TERMINATION_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.RDIMM.DBRW86DQSRTTParkTermination.bits.DQSRTTParkTermination]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        // Differential Memory Module Types
        if ((spdData[3] & 0x0F) == 0xA)
        {
            MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecBank = (spdData[240] & 0x7F) + 1;
            MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecID = spdData[241];
            MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.deviceType.raw = spdData[242];
            MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.deviceRev = spdData[243];

            _stprintf_s(g_szSITempDebug1, _T("Differential Memory Buffer (DMB) present: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecID & 0x7F, (spdData[240] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("DMB Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecBank, MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("DMB device type: %s"), DMB_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.DDIMM.DifferentialMemoryBuffer.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        // NVDIMM-N Memory Module Types
        if ((spdData[3] & 0xF0) == 0x90)
        {
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecBank = (spdData[240] & 0x7F) + 1;
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecID = spdData[241];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.deviceType.raw = spdData[242];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.deviceRev = spdData[243];

            _stprintf_s(g_szSITempDebug1, _T("Registering Clock Driver (RCD) present: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecID & 0x7F, (spdData[240] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("RCD Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecBank, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("RCD device type: %s"), RCD_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.RegisteringClockDriver.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecBank = (spdData[244] & 0x7F) + 1;
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecID = spdData[245];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.deviceType.raw = spdData[246];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.deviceRev = spdData[247];

            _stprintf_s(g_szSITempDebug1, _T("Data Buffers (DB) present: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecID & 0x7F, (spdData[244] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("Data Buffers Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecBank, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("Data Buffers device type: %s"), DB_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMN.DataBuffers.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        // NVDIMM-P Memory Module Types
        if ((spdData[3] & 0xF0) == 0xA0)
        {
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecBank = (spdData[240] & 0x7F) + 1;
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecID = spdData[241];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.deviceType.raw = spdData[242];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.deviceRev = spdData[243];

            _stprintf_s(g_szSITempDebug1, _T("Registering Clock Driver (RCD) present: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecID & 0x7F, (spdData[240] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("RCD Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecBank, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("RCD device type: %s"), RCD_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.RegisteringClockDriver.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecBank = (spdData[244] & 0x7F) + 1;
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecID = spdData[245];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.deviceType.raw = spdData[246];
            MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.deviceRev = spdData[247];

            _stprintf_s(g_szSITempDebug1, _T("Data Buffers (DB) present: %s"), MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecID & 0x7F, (spdData[244] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("Data Buffers Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecBank, MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("Data Buffers device type: %s"), DB_DEVICE_TYPE_TABLE[MemoryInfo->specific.DDR5SDRAM.baseModule.NVDIMMP.DataBuffers.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        MemoryInfo->specific.DDR5SDRAM.SPD509CRC16 = (spdData[511] << 8) | spdData[510];
        _stprintf_s(g_szSITempDebug1, _T("SPD bytes 0-509 CRC16: 0x%04X"), MemoryInfo->specific.DDR5SDRAM.SPD509CRC16);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->jedecID = 0;
        if (spdDataLen >= 512)
        {
            numCont = spdData[512] & 0x7F;
            MemoryInfo->jedecBank = numCont + 1;
            MemoryInfo->jedecID = spdData[513];

            GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);

            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleManufLoc = spdData[514];

            _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            if (spdData[515] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
                MemoryInfo->year = 2000 + ((spdData[515] >> 4) * 10) + (spdData[515] & 0x0F); // convert from BCD
            if (spdData[516] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
                MemoryInfo->week = ((spdData[516] >> 4) * 10) + (spdData[516] & 0x0F);        // convert from BCD

            _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleSerialNo = (spdData[517] << 24) | (spdData[518] << 16) | (spdData[519] << 8) | spdData[520];
            _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            memcpy(MemoryInfo->moduleExtSerialNo, &spdData[512], sizeof(MemoryInfo->moduleExtSerialNo));

            for (i = 0; i < SPD_DDR5_MODULE_PARTNO_LEN; i++)
            {
                if (spdData[521 + i] >= 0 && spdData[521 + i] < 128) // Sanity check for ASCII char
                    MemoryInfo->modulePartNo[i] = spdData[521 + i];
                else
                    break;
            }

#ifdef WIN32
            TCHAR wcPartNo[SPD_DDR5_MODULE_PARTNO_LEN + 1];
            memset(wcPartNo, 0, sizeof(wcPartNo));
            MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_DDR5_MODULE_PARTNO_LEN, wcPartNo, SPD_DDR5_MODULE_PARTNO_LEN + 1);
            _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
            _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleRev = spdData[551];
            _stprintf_s(g_szSITempDebug1, _T("Revision Code: 0x%02X"), MemoryInfo->moduleRev);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            numCont = spdData[552] & 0x7F;
            MemoryInfo->specific.DDR5SDRAM.DRAMManufBank = numCont + 1;
            MemoryInfo->specific.DDR5SDRAM.DRAMManufID = spdData[553];

            GetManNameFromJedecIDContCode(MemoryInfo->specific.DDR5SDRAM.DRAMManufID & 0x7F, numCont, MemoryInfo->specific.DDR5SDRAM.DRAMManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("DRAM Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->specific.DDR5SDRAM.DRAMManuf, MemoryInfo->specific.DDR5SDRAM.DRAMManufBank, MemoryInfo->specific.DDR5SDRAM.DRAMManufID);

            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.DRAMStepping = spdData[554];
            _stprintf_s(g_szSITempDebug1, _T("DRAM Stepping: 0x%02X"), MemoryInfo->specific.DDR5SDRAM.DRAMStepping);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

#ifndef WIN32
            // Bytes 555~639 (0x22B~27F): Manufacturer's Specific Data
            MemoryInfo->manufDataOff = 555;
            MemoryInfo->manufDataLen = 639 - 555 + 1;

            _stprintf_s(tmpMsg, _T("Module Manufacturer's Specific Data raw bytes:"));
            MtSupportUCDebugWriteLine(tmpMsg);

            _stprintf_s(tmpMsg, _T(""));
            for (i = 0; i < MemoryInfo->manufDataLen; i++)
            {
                TCHAR strSPDBytes[8];
                _stprintf_s(strSPDBytes, _T("%02X "), spdData[MemoryInfo->manufDataOff + i]);
                _tcscat(tmpMsg, strSPDBytes);

                if ((i % 16 == 15) || (i == MemoryInfo->manufDataLen - 1))
                {
                    MtSupportUCDebugWriteLine(tmpMsg);
                    _stprintf_s(tmpMsg, _T(""));
                }
            }
#endif
        }

        // XMP
        if (spdData[640] == SPD_DDR5_XMP3_ID_1 && spdData[641] == SPD_DDR5_XMP3_ID_2)
        {
            MemoryInfo->specific.DDR5SDRAM.XMPSupported = true;
            _stprintf_s(g_szSITempDebug1, _T("XMP supported: Yes"));
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.DDR5SDRAM.XMP.version = spdData[642];
            _stprintf_s(g_szSITempDebug1, _T("XMP version: %d.%d"), BitExtract(spdData[642], 7, 4), BitExtract(spdData[642], 3, 0));
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            // Check if v3.x revision
            if ((MemoryInfo->specific.DDR5SDRAM.XMP.version & 0xF0) == 0x30)
            {
                MemoryInfo->specific.DDR5SDRAM.XMP.PMICVendorID[0] = spdData[645];
                MemoryInfo->specific.DDR5SDRAM.XMP.PMICVendorID[1] = spdData[646];
                _stprintf_s(g_szSITempDebug1, _T("XMP PMIC Vendor ID: %02X%02X"), MemoryInfo->specific.DDR5SDRAM.XMP.PMICVendorID[0], MemoryInfo->specific.DDR5SDRAM.XMP.PMICVendorID[1]);
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR5SDRAM.XMP.numPMIC = spdData[647];
                _stprintf_s(g_szSITempDebug1, _T("XMP number of PMICs: %d"), MemoryInfo->specific.DDR5SDRAM.XMP.numPMIC);
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR5SDRAM.XMP.PMICCapabilities.raw = spdData[648];
                _stprintf_s(g_szSITempDebug1, _T("XMP PMIC OC capabilities (capable: %s, enabled: %s, voltage default step size: %dmV, global reset function: %s)"),
                            MemoryInfo->specific.DDR5SDRAM.XMP.PMICCapabilities.bits.OCCapable ? _T("Yes") : _T("No"),
                            MemoryInfo->specific.DDR5SDRAM.XMP.PMICCapabilities.bits.OCEnable ? _T("Yes") : _T("No"),
                            MemoryInfo->specific.DDR5SDRAM.XMP.PMICCapabilities.bits.VoltageDefaultStepSize ? 10 : 5,
                            MemoryInfo->specific.DDR5SDRAM.XMP.PMICCapabilities.bits.OCGlobalResetEnable ? _T("Enabled") : _T("Disabled"));
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR5SDRAM.XMP.revision = spdData[649];
                _stprintf_s(g_szSITempDebug1, _T("XMP spec revision: %d.%d"), BitExtract(spdData[649], 7, 4), BitExtract(spdData[649], 3, 0));
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR5SDRAM.XMP.ValidationCertification.raw = spdData[650];
                _stprintf_s(g_szSITempDebug1, _T("XMP Validation and Certification Capabilities (Self-certified: %s, PMIC validated by Intel AVL level: %s"),
                            MemoryInfo->specific.DDR5SDRAM.XMP.ValidationCertification.bits.SelfCertified ? _T("Yes") : _T("No"),
                            MemoryInfo->specific.DDR5SDRAM.XMP.ValidationCertification.bits.PMICIntelAVLValidated ? _T("Yes") : _T("No"));
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR5SDRAM.XMP.BaseCfgCRC16 = (spdData[703] << 8) | spdData[702];
                _stprintf_s(g_szSITempDebug1, _T("XMP Base Configuration Section CRC16: 0x%04X"), MemoryInfo->specific.DDR5SDRAM.XMP.BaseCfgCRC16);
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                unsigned short crc16 = (unsigned short)Crc16(spdData + 640, 701 - 640 + 1);
                if (crc16 != MemoryInfo->specific.DDR5SDRAM.XMP.BaseCfgCRC16)
                {
                    _stprintf_s(g_szSITempDebug1, _T("XMP Calculated CRC16 (%04X) does not match stored CRC16 (%04X)"), crc16, MemoryInfo->specific.DDR5SDRAM.XMP.BaseCfgCRC16);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);
                }

                for (i = 0; i < MAX_XMP3_PROFILES; i++)
                {
                    // Each profile is 64 bytes
#define PROFILE_OFFSET(i, offset) (offset + (i * 64))
                    if (i < 3)
                    {
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].enabled = BitExtract(spdData[643], i, i);
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].certified = BitExtract(spdData[643], 4 + i, 4 + i);
                        _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] Profile enabled: %s (certified: %s)"), i + 1, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].enabled ? _T("Yes") : _T("No"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].certified ? _T("Yes") : _T("No"));
                        MtSupportUCDebugWriteLine(g_szSITempDebug1);

                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].dimmsPerChannel = BitExtract(spdData[644], i * 2 + 1, i * 2) + 1;
                        _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] recommended DIMMs per channel: %d"), i + 1, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].dimmsPerChannel);
                        MtSupportUCDebugWriteLine(g_szSITempDebug1);

                        memcpy(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].name, spdData + 654 + MAX_XMP3_PROFILE_NAME_LEN * i, MAX_XMP3_PROFILE_NAME_LEN);
                    }
                    else
                    {
                        _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] User profile"), i + 1);
                        MtSupportUCDebugWriteLine(g_szSITempDebug1);

                        const unsigned char ZERO_BYTES[16] = {0};
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].enabled = memcmp(spdData + PROFILE_OFFSET(i, 704), ZERO_BYTES, sizeof(ZERO_BYTES)) == 0 ? false : true;

                        sprintf_s(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].name, "Rewritable Profile %d", i + 1);
                    }
                    if (MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].enabled == false)
                        continue;

#ifdef WIN32
                    TCHAR wcProfile[MAX_XMP3_PROFILE_NAME_LEN + 1];
                    memset(wcProfile, 0, sizeof(wcProfile));
                    MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].name, MAX_XMP3_PROFILE_NAME_LEN, wcProfile, MAX_XMP3_PROFILE_NAME_LEN + 1);
                    _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] profile name: %s"), i + 1, wcProfile);
#else
                    _stprintf_s(g_szSITempDebug1, _T("[XMP Profile %d] profile name: %a"), i + 1, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].name);
#endif
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    if (spdData[PROFILE_OFFSET(i, 704)] == 0 || spdData[PROFILE_OFFSET(i, 705)] == 0 || spdData[PROFILE_OFFSET(i, 706)] == 0 || spdData[PROFILE_OFFSET(i, 708)] == 0)
                    {
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].enabled = false;
                        continue;
                    }

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVPP, _T("%.2fV"), (float)BitExtract(spdData[PROFILE_OFFSET(i, 704)], 6, 5) + BitExtract(spdData[PROFILE_OFFSET(i, 704)], 4, 0) * 0.05f);
                    _stprintf_s(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDD, _T("%.2fV"), (float)BitExtract(spdData[PROFILE_OFFSET(i, 705)], 6, 5) + BitExtract(spdData[PROFILE_OFFSET(i, 705)], 4, 0) * 0.05f);
                    _stprintf_s(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDDQ, _T("%.2fV"), (float)BitExtract(spdData[PROFILE_OFFSET(i, 706)], 6, 5) + BitExtract(spdData[PROFILE_OFFSET(i, 706)], 4, 0) * 0.05f);
                    _stprintf_s(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].memCtrlVoltage, _T("%.2fV"), (float)BitExtract(spdData[PROFILE_OFFSET(i, 708)], 6, 5) + BitExtract(spdData[PROFILE_OFFSET(i, 708)], 4, 0) * 0.05f);
#else
                    {
                        int millivolts = (BitExtract(spdData[PROFILE_OFFSET(i, 704)], 6, 5) * 1000 + BitExtract(spdData[PROFILE_OFFSET(i, 704)], 4, 0) * 50);
                        getVoltageStr(millivolts, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVPP, sizeof(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVPP));
                        millivolts = (BitExtract(spdData[PROFILE_OFFSET(i, 705)], 6, 5) * 1000 + BitExtract(spdData[PROFILE_OFFSET(i, 705)], 4, 0) * 50);
                        getVoltageStr(millivolts, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDD, sizeof(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDD));
                        millivolts = (BitExtract(spdData[PROFILE_OFFSET(i, 706)], 6, 5) * 1000 + BitExtract(spdData[PROFILE_OFFSET(i, 706)], 4, 0) * 50);
                        getVoltageStr(millivolts, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDDQ, sizeof(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDDQ));
                        millivolts = (BitExtract(spdData[PROFILE_OFFSET(i, 708)], 6, 5) * 1000 + BitExtract(spdData[PROFILE_OFFSET(i, 708)], 4, 0) * 50);
                        getVoltageStr(millivolts, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].memCtrlVoltage, sizeof(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].memCtrlVoltage));
                    }
#endif

                    _stprintf_s(g_szSITempDebug1, _T("  Module VPP Voltage Level: %s"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVPP);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    _stprintf_s(g_szSITempDebug1, _T("  Module VDD Voltage Level: %s"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDD);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    _stprintf_s(g_szSITempDebug1, _T("  Module VDDQ Voltage Level: %s"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].moduleVDDQ);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    _stprintf_s(g_szSITempDebug1, _T("  Memory controller Voltage Level: %s"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].memCtrlVoltage);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCK = ((spdData[PROFILE_OFFSET(i, 710)] << 8) + spdData[PROFILE_OFFSET(i, 709)]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Cycle Time (tCKAVGmin): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCK);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed = TO_TIMING_VAR(1000 / MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCK);
                    if (((int)MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed % 100) < 10)
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed = ((int)MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed / 100) * 100;

                    // Adjust for rounding errors
                    if ((int)MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed == 1066)
                    {
#ifdef WIN32
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed = 1000 / 0.9375f;
#else
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed = 1067;
#endif
                    }
                    else if ((int)MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed == 933)
                    {
#ifdef WIN32
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed = 1000 / (15.0f / 14.0f);
#else
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed = 933;
#endif
                    }

                    if (MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed < 100 || MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed > 100000)
                    {
                        MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].enabled = false;
                        continue;
                    }

                    _stprintf_s(g_szSITempDebug1, _T("  Maximum clock speed: %d MHz"), (int)MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].clkspeed);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    for (int CASInd = 0; CASInd <= 4; CASInd++)
                    {
                        for (int bit = 0; bit < 8; bit++)
                        {
                            if (spdData[PROFILE_OFFSET(i, 711 + CASInd)] & (1 << bit))
                            {
                                TCHAR strCAS[8];
                                _stprintf_s(strCAS, _T("%d"), CASInd * 16 + bit * 2 + 20);
                                _tcscat(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].CASSupported, strCAS);
                                _tcscat(MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].CASSupported, _T(" "));
                            }
                        }
                    }

                    _stprintf_s(g_szSITempDebug1, _T("  Supported CAS: %s"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].CASSupported);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tAA = ((spdData[PROFILE_OFFSET(i, 718)] << 8) + spdData[PROFILE_OFFSET(i, 717)]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum CAS Latency Time (tAAmin): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tAA);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRCD = ((spdData[PROFILE_OFFSET(i, 720)] << 8) + spdData[PROFILE_OFFSET(i, 719)]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum RAS to CAS Delay Time (tRCDmin): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRCD);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRP = ((spdData[PROFILE_OFFSET(i, 722)] << 8) + spdData[PROFILE_OFFSET(i, 721)]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Row Precharge Delay Time (tRPmin): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRP);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRAS = ((spdData[PROFILE_OFFSET(i, 724)] << 8) + spdData[PROFILE_OFFSET(i, 723)]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Precharge Delay Time (tRASmin): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRAS);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRC = ((spdData[PROFILE_OFFSET(i, 726)] << 8) + spdData[PROFILE_OFFSET(i, 725)]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Active/Refresh Delay Time (tRCmin): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRC);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tWR = ((spdData[PROFILE_OFFSET(i, 728)] << 8) + spdData[PROFILE_OFFSET(i, 727)]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Write Recovery Time (tWRmin): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tWR);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRFC1 = ((spdData[PROFILE_OFFSET(i, 730)] << 8) + spdData[PROFILE_OFFSET(i, 729)]) * tb * 1000;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFC1min): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRFC1);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRFC2 = ((spdData[PROFILE_OFFSET(i, 732)] << 8) + spdData[PROFILE_OFFSET(i, 731)]) * tb * 1000;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFC2min): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRFC2);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRFCsb = ((spdData[PROFILE_OFFSET(i, 734)] << 8) + spdData[PROFILE_OFFSET(i, 733)]) * tb * 1000;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFCsb): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRFCsb);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L = ((spdData[PROFILE_OFFSET(i, 736)] << 8) + spdData[PROFILE_OFFSET(i, 735)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_nCK = spdData[PROFILE_OFFSET(i, 737)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Read to Read Command Delay Time, Same Bank Group (tCCD_L): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WR = ((spdData[PROFILE_OFFSET(i, 739)] << 8) + spdData[PROFILE_OFFSET(i, 738)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WR_nCK = spdData[PROFILE_OFFSET(i, 740)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Write to Write Command Delay Time, Same Bank Group (tCCD_L_WR): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WR);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WR2 = ((spdData[PROFILE_OFFSET(i, 742)] << 8) + spdData[PROFILE_OFFSET(i, 741)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WR2_nCK = spdData[PROFILE_OFFSET(i, 743)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Write to Write Command Delay Time, Second Write not RMW, Same Bank Group (tCCD_L_WR2): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WR2);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WTR = ((spdData[PROFILE_OFFSET(i, 745)] << 8) + spdData[PROFILE_OFFSET(i, 744)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WTR_nCK = spdData[PROFILE_OFFSET(i, 746)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Write to Read Command Delay Time, Same Bank Group (tCCD_L_WTR): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_L_WTR);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_S_WTR = ((spdData[PROFILE_OFFSET(i, 748)] << 8) + spdData[PROFILE_OFFSET(i, 747)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_S_WTR_nCK = spdData[PROFILE_OFFSET(i, 749)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Write to Read Command Delay Time, Different Bank Group (tCCD_S_WTR): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tCCD_S_WTR);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRRD_L = ((spdData[PROFILE_OFFSET(i, 751)] << 8) + spdData[PROFILE_OFFSET(i, 750)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRRD_L_nCK = spdData[PROFILE_OFFSET(i, 752)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Active to Active Command Delay Time, Same Bank Group (tRRD_L): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRRD_L);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRTP = ((spdData[PROFILE_OFFSET(i, 754)] << 8) + spdData[PROFILE_OFFSET(i, 753)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRTP_nCK = spdData[PROFILE_OFFSET(i, 755)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Read to Precharge Command Delay Time, (tRTP): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tRTP);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tFAW = ((spdData[PROFILE_OFFSET(i, 757)] << 8) + spdData[PROFILE_OFFSET(i, 756)]) * tb;
                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tFAW_nCK = spdData[PROFILE_OFFSET(i, 758)];
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Four Activate Window (tFAW min): "), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].tFAW);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].AdvancedMemoryOverclockingFeatures.raw = spdData[PROFILE_OFFSET(i, 763)];
                    _stprintf_s(g_szSITempDebug1, _T("  Advanced Memory Overclocking Features: Real-Time Memory Frequency Overclocking: %s, Intel Dynamic Memory Boost: %s"),
                                MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].AdvancedMemoryOverclockingFeatures.bits.RealTimeMemoryFrequencyOverclocking ? _T("supported") : _T("not supported"),
                                MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].AdvancedMemoryOverclockingFeatures.bits.IntelDynamicMemoryBoost ? _T("supported") : _T("not supported"));
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].cmdRateMode = spdData[PROFILE_OFFSET(i, 764)];
                    _stprintf_s(g_szSITempDebug1, _T("  System CMD Rate Mode: %dn"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].cmdRateMode);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].VendorPersonalityByte = spdData[PROFILE_OFFSET(i, 765)];
                    _stprintf_s(g_szSITempDebug1, _T("  Vendor Personality Byte: %02X"), MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].VendorPersonalityByte);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].BaseCfgCRC16 = (spdData[PROFILE_OFFSET(i, 767)] << 8) | spdData[PROFILE_OFFSET(i, 766)];

                    crc16 = (unsigned short)Crc16(spdData + PROFILE_OFFSET(i, 704), 765 - 704 + 1);
                    if (crc16 != MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].BaseCfgCRC16)
                    {
                        _stprintf_s(g_szSITempDebug1, _T("XMP Calculated CRC16 (%04X) does not match stored CRC16 (%04X)"), crc16, MemoryInfo->specific.DDR5SDRAM.XMP.profile[i].BaseCfgCRC16);
                        MtSupportUCDebugWriteLine(g_szSITempDebug1);
                    }
                }
            }
        }

        // Blocks 10-15: End User programmable
        for (int block = 10; block < 15; block++)
        {
            int blkoffset = block * 64;
            if (spdData[blkoffset] == SPD_DDR5_EXPO_ID_0 && spdData[blkoffset + 1] == SPD_DDR5_EXPO_ID_1 && spdData[blkoffset + 2] == SPD_DDR5_EXPO_ID_2 && spdData[blkoffset + 3] == SPD_DDR5_EXPO_ID_3)
            {
                MemoryInfo->specific.DDR5SDRAM.EXPOSupported = true;
                _stprintf_s(g_szSITempDebug1, _T("EXPO supported: Yes"));
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR5SDRAM.EXPO.version = spdData[blkoffset + 4];
                _stprintf_s(g_szSITempDebug1, _T("EXPO version: %d.%d"), BitExtract(spdData[blkoffset + 4], 7, 4), BitExtract(spdData[blkoffset + 4], 3, 0));
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                MemoryInfo->specific.DDR5SDRAM.EXPO.PMICFeatureSupport.raw = spdData[blkoffset + 7];
                _stprintf_s(g_szSITempDebug1, _T("EXPO PMIC Feature Support (PMIC 10 mV step size support: %s)"),
                            MemoryInfo->specific.DDR5SDRAM.EXPO.PMICFeatureSupport.bits.stepSize10mV ? _T("Yes") : _T("No"));
                MtSupportUCDebugWriteLine(g_szSITempDebug1);

                for (i = 0; i < MAX_EXPO_PROFILES; i++)
                {
                    int profoff = blkoffset + i * 50;
                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].enabled = BitExtract(spdData[blkoffset + 5], i * 4, i * 4);

                    _stprintf_s(g_szSITempDebug1, _T("[EXPO Profile %d] Profile enabled: %s"), i + 1, MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].enabled ? _T("Yes") : _T("No"));
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    if (MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].enabled == false)
                        continue;

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].dimmsPerChannel = BitExtract(spdData[blkoffset + 5], i * 4 + 3, i * 4 + 1);
                    _stprintf_s(g_szSITempDebug1, _T("[EXPO Profile %d] supported DIMMs per channel: %d"), i + 1, MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].dimmsPerChannel);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].block1Enabled = BitExtract(spdData[blkoffset + 6], i * 4, i * 4);
                    _stprintf_s(g_szSITempDebug1, _T("[EXPO Profile %d] Block 1 enabled: %s"), i + 1, MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].block1Enabled ? _T("Yes") : _T("No"));
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    int VDDmV = BitExtract(spdData[profoff + 10], 6, 5) * 1000 + BitExtract(spdData[profoff + 10], 4, 1) * 100 + BitExtract(spdData[profoff + 10], 0, 0) * 50;
                    int VDDQmV = BitExtract(spdData[profoff + 11], 6, 5) * 1000 + BitExtract(spdData[profoff + 11], 4, 1) * 100 + BitExtract(spdData[profoff + 11], 0, 0) * 50;
                    int VPPmV = BitExtract(spdData[profoff + 12], 6, 5) * 1000 + BitExtract(spdData[profoff + 12], 4, 1) * 100 + BitExtract(spdData[profoff + 12], 0, 0) * 50;

#ifdef WIN32
                    _stprintf_s(MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDD, _T("%.2fV"), (float)VDDmV / 1000.0);
                    _stprintf_s(MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDDQ, _T("%.2fV"), (float)VDDQmV / 1000.0);
                    _stprintf_s(MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVPP, _T("%.2fV"), (float)VPPmV / 1000.0);
#else
                    getVoltageStr(VDDmV, MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDD, sizeof(MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDD));
                    getVoltageStr(VDDQmV, MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDDQ, sizeof(MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDDQ));
                    getVoltageStr(VPPmV, MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVPP, sizeof(MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVPP));
#endif

                    _stprintf_s(g_szSITempDebug1, _T("  Module VPP Voltage Level: %s"), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVPP);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    _stprintf_s(g_szSITempDebug1, _T("  Module VDD Voltage Level: %s"), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDD);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    _stprintf_s(g_szSITempDebug1, _T("  Module VDDQ Voltage Level: %s"), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].moduleVDDQ);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCK = ((spdData[profoff + 15] << 8) + spdData[profoff + 14]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Cycle Time (tCKAVGmin): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCK);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed = TO_TIMING_VAR(1000 / MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCK);
                    if (((int)MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed % 100) < 10)
                        MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed = ((int)MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed / 100) * 100;

                    // Adjust for rounding errors
                    if ((int)MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed == 1066)
                    {
#ifdef WIN32
                        MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed = 1000 / 0.9375f;
#else
                        MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed = 1067;
#endif
                    }
                    else if ((int)MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed == 933)
                    {
#ifdef WIN32
                        MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed = 1000 / (15.0f / 14.0f);
#else
                        MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed = 933;
#endif
                    }

                    if (MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed < 100 || MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed > 100000)
                    {
                        MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].enabled = false;
                        continue;
                    }
                    _stprintf_s(g_szSITempDebug1, _T("  Maximum clock speed: %d MHz"), (int)MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].clkspeed);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tAA = ((spdData[profoff + 17] << 8) + spdData[profoff + 16]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum CAS Latency Time (tAAmin): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tAA);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRCD = ((spdData[profoff + 19] << 8) + spdData[profoff + 18]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum RAS to CAS Delay Time (tRCDmin): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRCD);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRP = ((spdData[profoff + 21] << 8) + spdData[profoff + 20]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Row Precharge Delay Time (tRPmin): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRP);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRAS = ((spdData[profoff + 23] << 8) + spdData[profoff + 22]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Precharge Delay Time (tRASmin): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRAS);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRC = ((spdData[profoff + 25] << 8) + spdData[profoff + 24]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Active to Active/Refresh Delay Time (tRCmin): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRC);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tWR = ((spdData[profoff + 27] << 8) + spdData[profoff + 26]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Write Recovery Time (tWRmin): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tWR);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRFC1 = ((spdData[profoff + 29] << 8) + spdData[profoff + 28]) * tb * 1000;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFC1min): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRFC1);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRFC2 = ((spdData[profoff + 31] << 8) + spdData[profoff + 30]) * tb * 1000;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFC2min): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRFC2);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRFCsb = ((spdData[profoff + 33] << 8) + spdData[profoff + 32]) * tb * 1000;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Refresh Recovery Delay Time (tRFCsb): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRFCsb);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRRD_L = ((spdData[profoff + 35] << 8) + spdData[profoff + 34]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Active to Active Command Delay Time, Same Bank Group (tRRD_L): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRRD_L);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCCD_L = ((spdData[profoff + 37] << 8) + spdData[profoff + 36]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Read to Read Command Delay Time, Same Bank Group (tCCD_L): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCCD_L);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCCD_L_WR = ((spdData[profoff + 39] << 8) + spdData[profoff + 38]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  Minimum Write to Write Command Delay Time, Same Bank Group (tCCD_L_WR): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCCD_L_WR);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCCD_L_WR2 = ((spdData[profoff + 41] << 8) + spdData[profoff + 40]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Write to Write Command Delay Time, Second Write not RMW, Same Bank Group (tCCD_L_WR2): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tCCD_L_WR2);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tFAW = ((spdData[profoff + 43] << 8) + spdData[profoff + 42]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Four Activate Window (tFAW min): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tFAW);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tWTR_L = ((spdData[profoff + 45] << 8) + spdData[profoff + 44]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Write to Read Command Delay Time, Same Bank Group (tWTR_L): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tWTR_L);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tWTR_S = ((spdData[profoff + 47] << 8) + spdData[profoff + 46]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Write to Read Command Delay Time, Different Bank Group (tWTR_S): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tWTR_S);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);

                    MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRTP = ((spdData[profoff + 49] << 8) + spdData[profoff + 48]) * tb;
                    SPRINTF_FLOAT(g_szSITempDebug1, _T("  SDRAM Minimum Read to Precharge Command Delay Time, (tRTP): "), MemoryInfo->specific.DDR5SDRAM.EXPO.profile[i].tRTP);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);
                }

                MemoryInfo->specific.DDR5SDRAM.EXPO.CRC16 = (spdData[blkoffset + 127] << 8) | spdData[blkoffset + 126];

                unsigned short crc16 = (unsigned short)Crc16(spdData + blkoffset, 126);
                if (crc16 != MemoryInfo->specific.DDR5SDRAM.EXPO.CRC16)
                {
                    _stprintf_s(g_szSITempDebug1, _T("EXPO Calculated CRC16 (%04X) does not match stored CRC16 (%04X)"), crc16, MemoryInfo->specific.DDR5SDRAM.EXPO.CRC16);
                    MtSupportUCDebugWriteLine(g_szSITempDebug1);
                }
            }
        }
    }
    break;

    case LPDDR510:
    {
#ifdef WIN32
        float ftb;
        float mtb;
        float ddrclk;
#else
        int ftb;
        int mtb;
        int ddrclk;
#endif
        int channels;
        int buswidth;
        int datawidth;
        unsigned long long density;
        int ranks;
        int asymmetrical;

        unsigned long long capacity;
        int pcclk;

        int taa;
        int trcd;
        int trp;

        // manufacture information
        // get the number of continuation codes
        unsigned char numCont;

        // Module Type
        MemoryInfo->specific.LPDDR5SDRAM.keyByteModuleType.raw = spdData[3];
        _tcscpy(MemoryInfo->specific.LPDDR5SDRAM.moduleType, DDR5_MODULE_TYPES_TABLE[MemoryInfo->specific.LPDDR5SDRAM.keyByteModuleType.bits.baseModuleType]);

        if (MemoryInfo->specific.LPDDR5SDRAM.keyByteModuleType.bits.hybridMedia == 0x1)
        {
            _tcscat(MemoryInfo->specific.LPDDR5SDRAM.moduleType, _T(", NVDIMM-N Hybrid"));
        }
        else if (MemoryInfo->specific.LPDDR5SDRAM.keyByteModuleType.bits.hybridMedia == 0x2)
        {
            _tcscat(MemoryInfo->specific.LPDDR5SDRAM.moduleType, _T(", NVDIMM-P Hybrid"));
        }

        _stprintf_s(g_szSITempDebug1, _T("Module type: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleType);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0x01) // RDIMM
            MemoryInfo->registered = true;
        else
            MemoryInfo->registered = false;

        MemoryInfo->specific.LPDDR5SDRAM.SDRAMDensityBanks.raw = spdData[4];
        MemoryInfo->specific.LPDDR5SDRAM.SDRAMAddressing.raw = spdData[5];
        MemoryInfo->specific.LPDDR5SDRAM.SDRAMPackageType.raw = spdData[6];
        MemoryInfo->specific.LPDDR5SDRAM.OptionalSDRAMFeatures.raw = spdData[9];
        MemoryInfo->specific.LPDDR5SDRAM.LPDDR5ModuleOrganization.raw = spdData[12];
        MemoryInfo->specific.LPDDR5SDRAM.SystemSubChannelBusWidth.raw = spdData[13];
        MemoryInfo->specific.LPDDR5SDRAM.SignalLoading.raw = spdData[16];

        // To calculate the total capacity in bytes for a symmetric module, the following math applies:
        // Capacity in bytes =
        //  Number of sub-channels per DIMM *
        //  Primary bus width per sub-channel / SDRAM Die Data Width *
        //  SDRAM density per die / 8 *
        //  Package ranks per sub-channel

        channels = 1 << BitExtract(spdData[235], 7, 5);
        buswidth = 1 << (BitExtract(spdData[235], 2, 0) + 3);
        datawidth = 1 << (BitExtract(spdData[12], 2, 0) + 2);

        static const int SDRAM_DENSITY_PER_DIE[16] = {0, 0, 1, 2, 4, 8, 16, 32, 12, 24, 3, 6};

#ifdef WIN32
        density = ((unsigned long long)(SDRAM_DENSITY_PER_DIE[BitExtract(spdData[4], 3, 0)]) << 30) / 8;
#else
        density = DivU64x32(LShiftU64(SDRAM_DENSITY_PER_DIE[BitExtract(spdData[4], 3, 0)], 30), 8);
#endif
        ranks = BitExtract(spdData[234], 5, 3) + 1;
        asymmetrical = BitExtract(spdData[234], 6, 6);

        if (!asymmetrical)
        {
#ifdef WIN32
            capacity = density * channels * (buswidth / datawidth) * ranks;
#else
            capacity = MultU64x32(density, channels * (buswidth / datawidth) * ranks);
#endif
            capacity = (capacity >> 20);
        }
        else
        {
        }

        MemoryInfo->size = (int)capacity;

        _stprintf_s(g_szSITempDebug1, _T("Size: %d MB"), MemoryInfo->size);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->numBanks = 1 << (MemoryInfo->specific.LPDDR5SDRAM.SDRAMDensityBanks.bits.bankAddrBits + 2);
        MemoryInfo->rowAddrBits = MemoryInfo->specific.LPDDR5SDRAM.SDRAMAddressing.bits.rowAddrBits + 12;

        static const int COLUMN_ADDR_BITS[8] = {6, 6};
        MemoryInfo->colAddrBits = COLUMN_ADDR_BITS[MemoryInfo->specific.LPDDR5SDRAM.SDRAMAddressing.bits.bankColAddrBits];
        MemoryInfo->busWidth = buswidth;
        MemoryInfo->numRanks = ranks;

        _stprintf_s(g_szSITempDebug1, _T("Banks x Rows x Columns x Bits: %d x %d x %d x %d"),
                    MemoryInfo->numBanks,
                    MemoryInfo->rowAddrBits,
                    MemoryInfo->colAddrBits,
                    MemoryInfo->busWidth);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Ranks: %d"), ranks);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->deviceWidth = datawidth;

        _stprintf_s(g_szSITempDebug1, _T("SDRAM Device Width: %d"), MemoryInfo->deviceWidth);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.SDRAMPackageType.bits.packageType == 0)
            _stprintf_s(g_szSITempDebug1, _T("SDRAM Package Type: Monolithic DRAM Device"));
        else
            _stprintf_s(g_szSITempDebug1, _T("SDRAM Package Type: %d die Non-Monolithic Device"), MemoryInfo->specific.LPDDR5SDRAM.SDRAMPackageType.bits.diePerPackage + 1);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        ftb = TO_TIMING_VAR(1 / 1000);
        mtb = TO_TIMING_VAR(125 / 1000);

        // ecc
        if (BitExtract(spdData[235], 4, 3))
            MemoryInfo->ecc = true;
        else
            MemoryInfo->ecc = false;

        _stprintf_s(g_szSITempDebug1, _T("ECC: %s"), MemoryInfo->ecc ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        // Speed
        MemoryInfo->tCK = spdData[18] * mtb + (char)spdData[125] * ftb;

        if (MemoryInfo->tCK == 0)
        {
            MtSupportUCDebugWriteLine(_T("tCK not valid."));
            return false;
        }

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum clock cycle time (ns): "), MemoryInfo->tCK);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.tCKmax = spdData[19] * mtb + (char)spdData[124] * ftb;

        SPRINTF_FLOAT(g_szSITempDebug1, _T("Maximum clock cycle time (ns): "), MemoryInfo->specific.LPDDR5SDRAM.tCKmax);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        ddrclk = TO_TIMING_VAR((2 * 1000) / MemoryInfo->tCK);
        if (((int)ddrclk % 100) < 10)
            ddrclk = ((int)ddrclk / 100) * 100;
        MemoryInfo->txWidth = buswidth;
        pcclk = (int)TO_TIMING_VAR((2 * 1000 * 8) / (MemoryInfo->tCK));
        MemoryInfo->clkspeed = TO_TIMING_VAR(1000 / (MemoryInfo->tCK));
        if (((int)MemoryInfo->clkspeed % 100) < 10)
            MemoryInfo->clkspeed = ((int)MemoryInfo->clkspeed / 100) * 100;

        // Adjust for rounding errors
        if ((int)MemoryInfo->clkspeed == 1066)
        {
#ifdef WIN32
            MemoryInfo->clkspeed = 1000 / 0.9375f;
#else
            MemoryInfo->clkspeed = 1067;
#endif
        }
        else if ((int)MemoryInfo->clkspeed == 933)
        {
#ifdef WIN32
            MemoryInfo->clkspeed = 1000 / (15.0f / 14.0f);
#else
            MemoryInfo->clkspeed = 933;
#endif
        }

        _stprintf_s(g_szSITempDebug1, _T("Maximum clock speed: %d MHz"), (int)MemoryInfo->clkspeed);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        pcclk = (pcclk / 100) * 100;
        _stprintf_s(g_szSITempDebug1, _T("Maximum module speed: %d MT/s (PC5-%d)"), (int)ddrclk, pcclk);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);
        _stprintf_s(g_szSITempDebug1, _T("PC5-%d"), pcclk);
        wcscpy(MemoryInfo->szModulespeed, g_szSITempDebug1);

        // more timing information
        MemoryInfo->tAA = spdData[24] * mtb + (char)spdData[123] * ftb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum CAS latency time (tAA): "), MemoryInfo->tAA);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->tRCD = spdData[26] * mtb + (char)spdData[122] * ftb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum RAS to CAS delay time (tRCD): "), MemoryInfo->tRCD);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->tRP = spdData[27] * mtb + (char)spdData[121] * ftb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum row precharge time (tRP_ab): "), MemoryInfo->tRP);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.tRP_pb = spdData[28] * mtb + (char)spdData[120] * ftb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum row precharge time (tRP_pb): "), MemoryInfo->specific.LPDDR5SDRAM.tRP_pb);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        // See JESD400-5 DDR5 Serial Presence Detect (SPD) - Rounding Algorithms
        taa = (int)(((MemoryInfo->tAA * 997 / MemoryInfo->tCK) + 1000) / 1000);
        // Systems may prefer to use other methods to calculate CAS Latency settings, including applying the
        // Rounding Algorithm on tAAmin(SPD bytes 30~31) to determine the desired CL setting, rounding up if
        // necessary to ensure that only even nCK values are used,
        taa += taa % 2;
        trcd = (int)(((MemoryInfo->tRCD * 997 / MemoryInfo->tCK) + 1000) / 1000);
        trp = (int)(((MemoryInfo->tRP * 997 / MemoryInfo->tCK) + 1000) / 1000);

        // latencies
        _stprintf_s(MemoryInfo->CASSupported, _T("%d"), taa);
        _stprintf_s(g_szSITempDebug1, _T("Supported CAS: %s"), MemoryInfo->CASSupported);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

#ifdef WIN32
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%.1f MHz) : %d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp);
#else
        _stprintf_s(g_szSITempDebug1, _T("Supported timing at highest clock speed (%d MHz) : %d-%d-%d"), MemoryInfo->clkspeed, taa, trcd, trp);
#endif
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->tRFC = ((spdData[30] << 8) + spdData[29]) * mtb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC_ab): "), MemoryInfo->tRFC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.tRFC_pb = ((spdData[32] << 8) + spdData[31]) * mtb;
        SPRINTF_FLOAT(g_szSITempDebug1, _T("Minimum Recovery Delay (tRFC_pb): "), MemoryInfo->specific.LPDDR5SDRAM.tRFC_pb);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        // miscellaneous stuff

        MemoryInfo->specific.LPDDR5SDRAM.moduleSPDRev = spdData[192];
        MemoryInfo->specific.LPDDR5SDRAM.HashingSequence.raw = spdData[192];

        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecBank = (spdData[194] & 0x7F) + 1;
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecID = spdData[195];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.deviceType.raw = spdData[196];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.deviceRev = spdData[197];

        _stprintf_s(g_szSITempDebug1, _T("SPD present: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecID & 0x7F, (spdData[194] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("SPD Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecBank, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("SPD device type: %s"), SPD_DEVICE_TYPE_TABLE[MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.SPD.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank = (spdData[198] & 0x7F) + 1;
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID = spdData[199];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.raw = spdData[200];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.deviceRev = spdData[201];

        _stprintf_s(g_szSITempDebug1, _T("PMIC 0 present: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID & 0x7F, (spdData[198] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("PMIC 0 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecBank, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("PMIC 0 device type: %s"), PMIC_DEVICE_TYPE_TABLE[MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC0.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank = (spdData[202] & 0x7F) + 1;
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID = spdData[203];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.raw = spdData[204];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.deviceRev = spdData[205];

        _stprintf_s(g_szSITempDebug1, _T("PMIC 1 present: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID & 0x7F, (spdData[202] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("PMIC 1 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecBank, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("PMIC 1 device type: %s"), PMIC_DEVICE_TYPE_TABLE[MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC1.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank = (spdData[206] & 0x7F) + 1;
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID = spdData[207];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.raw = spdData[208];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.deviceRev = spdData[209];

        _stprintf_s(g_szSITempDebug1, _T("PMIC 2 present: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID & 0x7F, (spdData[206] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("PMIC 2 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecBank, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("PMIC 2 device type: %s"), PMIC_DEVICE_TYPE_TABLE[MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.PMIC2.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank = (spdData[210] & 0x7F) + 1;
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID = spdData[211];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.raw = spdData[212];
        MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceRev = spdData[213];

        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensor 0 present: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensor 1 present: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.installed1 ? _T("Yes") : _T("No"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID != 0)
        {
            GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID & 0x7F, (spdData[210] & 0x7F), tmpMsg, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("Thermal Sensors Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecBank, MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.jedecID);
        }

        _stprintf_s(g_szSITempDebug1, _T("Thermal Sensors device type: %s"), TS_DEVICE_TYPE_TABLE[MemoryInfo->specific.LPDDR5SDRAM.moduleDeviceInfo.ThermalSensors.deviceType.bits.type]);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.moduleHeight = (spdData[230] & 0x1F) + 15;
        _stprintf_s(g_szSITempDebug1, _T("Module Height (mm): %d - %d"), MemoryInfo->specific.LPDDR5SDRAM.moduleHeight - 1, MemoryInfo->specific.LPDDR5SDRAM.moduleHeight);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.moduleThicknessFront = (spdData[231] & 0xF) + 1;
        MemoryInfo->specific.LPDDR5SDRAM.moduleThicknessBack = ((spdData[231] >> 4) & 0xF) + 1;
        _stprintf_s(g_szSITempDebug1, _T("Module Thickness (mm): front %d-%d , back %d-%d "), MemoryInfo->specific.LPDDR5SDRAM.moduleThicknessFront - 1, MemoryInfo->specific.LPDDR5SDRAM.moduleThicknessFront, MemoryInfo->specific.LPDDR5SDRAM.moduleThicknessBack - 1, MemoryInfo->specific.LPDDR5SDRAM.moduleThicknessBack);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.ReferenceRawCardUsed.raw = spdData[232];

        static const TCHAR *REF_RAW_CARD_TABLE[] = {_T("A"),
                                                    _T("B"),
                                                    _T("C"),
                                                    _T("D"),
                                                    _T("E"),
                                                    _T("F"),
                                                    _T("G"),
                                                    _T("H"),
                                                    _T("J"),
                                                    _T("K"),
                                                    _T("L"),
                                                    _T("M"),
                                                    _T("N"),
                                                    _T("P"),
                                                    _T("R"),
                                                    _T("T"),
                                                    _T("U"),
                                                    _T("V"),
                                                    _T("W"),
                                                    _T("Y"),
                                                    _T("AA"),
                                                    _T("AB"),
                                                    _T("AC"),
                                                    _T("AD"),
                                                    _T("AE"),
                                                    _T("AF"),
                                                    _T("AG"),
                                                    _T("AH"),
                                                    _T("AJ"),
                                                    _T("AK"),
                                                    _T("Reserved"),
                                                    _T("ZZ")};

        _stprintf_s(MemoryInfo->specific.LPDDR5SDRAM.moduleRefCard, _T("Raw Card %s Rev. %d"), REF_RAW_CARD_TABLE[MemoryInfo->specific.LPDDR5SDRAM.ReferenceRawCardUsed.bits.ReferenceDesign], MemoryInfo->specific.LPDDR5SDRAM.ReferenceRawCardUsed.bits.DesignRevision);

        _stprintf_s(g_szSITempDebug1, _T("Module reference card: %s"), MemoryInfo->specific.LPDDR5SDRAM.moduleRefCard);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.DIMMAttributes.raw = spdData[233];

        _stprintf_s(g_szSITempDebug1, _T("# DRAM Rows: %d"), 1 << (MemoryInfo->specific.LPDDR5SDRAM.DIMMAttributes.bits.NumDRAMRows - 1));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Heat spreader installed: %s"), MemoryInfo->specific.LPDDR5SDRAM.DIMMAttributes.bits.HeatSpreader ? _T("Yes") : _T("No"));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Operating Temperature Range: %s"), OPERATING_TEMP_RANGE[MemoryInfo->specific.LPDDR5SDRAM.DIMMAttributes.bits.OperatingTemperatureRange]);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.ModuleOrganization.raw = spdData[234];

        _stprintf_s(g_szSITempDebug1, _T("Rank Mix: %s"), MemoryInfo->specific.LPDDR5SDRAM.ModuleOrganization.bits.RankMix ? _T("Asymmetrical") : _T("Symmetrical"));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Number of Package Ranks per Sub-Channel: %d"), MemoryInfo->specific.LPDDR5SDRAM.ModuleOrganization.bits.NumPackageRanksPerSubChannel + 1);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        MemoryInfo->specific.LPDDR5SDRAM.MemoryChannelBusWidth.raw = spdData[235];

        _stprintf_s(g_szSITempDebug1, _T("Number of Sub-Channels per DIMM: %d"), 1 << MemoryInfo->specific.LPDDR5SDRAM.MemoryChannelBusWidth.bits.NumSubChannelsPerDIMM);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Bus width extension per Sub-Channel: %d bits"), MemoryInfo->specific.LPDDR5SDRAM.MemoryChannelBusWidth.bits.BusWidthExtensionPerSubChannel * 4);
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        _stprintf_s(g_szSITempDebug1, _T("Primary bus width per Sub-Channel: %d bits"), 1 << (MemoryInfo->specific.LPDDR5SDRAM.MemoryChannelBusWidth.bits.PrimaryBusWidthPerSubChannel + 3));
        MtSupportUCDebugWriteLine(g_szSITempDebug1);

        if (MemoryInfo->specific.LPDDR5SDRAM.keyByteModuleType.bits.baseModuleType == 0x8) // CAMM2
        {
            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecBank = (spdData[240] & 0x7F) + 1;
            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecID = spdData[241];
            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.deviceType.raw = spdData[242];
            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.deviceRev = spdData[243];

            _stprintf_s(g_szSITempDebug1, _T("Clock Driver (CK) 0 present: %s"), MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecID & 0x7F, (spdData[240] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("Clock Driver 0 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecBank, MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("Clock Driver 0 device type: %s"), CK_DEVICE_TYPE_TABLE[MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver0.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecBank = (spdData[244] & 0x7F) + 1;
            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecID = spdData[245];
            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.deviceType.raw = spdData[246];
            MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.deviceRev = spdData[247];

            _stprintf_s(g_szSITempDebug1, _T("Clock Driver (CK) 1 present: %s"), MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.deviceType.bits.installed0 ? _T("Yes") : _T("No"));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecID != 0)
            {
                GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecID & 0x7F, (spdData[244] & 0x7F), tmpMsg, SHORT_STRING_LEN);
                _stprintf_s(g_szSITempDebug1, _T("Clock Driver 1 Manufacturer: %s (Bank: %d, ID: 0x%02X)"), tmpMsg, MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecBank, MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.jedecID);
            }

            _stprintf_s(g_szSITempDebug1, _T("Clock Driver 1 device type: %s"), CK_DEVICE_TYPE_TABLE[MemoryInfo->specific.LPDDR5SDRAM.baseModule.CAMM2.ClockDriver1.deviceType.bits.type]);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        MemoryInfo->specific.LPDDR5SDRAM.SPD509CRC16 = (spdData[511] << 8) | spdData[510];
        _stprintf_s(g_szSITempDebug1, _T("SPD bytes 0-509 CRC16: 0x%04X"), MemoryInfo->specific.LPDDR5SDRAM.SPD509CRC16);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        MemoryInfo->jedecID = 0;
        if (spdDataLen >= 512)
        {
            numCont = spdData[512] & 0x7F;
            MemoryInfo->jedecBank = numCont + 1;
            MemoryInfo->jedecID = spdData[513];

            GetManNameFromJedecIDContCode(MemoryInfo->jedecID & 0x7F, numCont, MemoryInfo->jedecManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("Module Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->jedecManuf, MemoryInfo->jedecBank, MemoryInfo->jedecID);

            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleManufLoc = spdData[514];

            _stprintf_s(g_szSITempDebug1, _T("Module manufacturing location: 0x%02X"), MemoryInfo->moduleManufLoc);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            if (spdData[515] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
                MemoryInfo->year = 2000 + ((spdData[515] >> 4) * 10) + (spdData[515] & 0x0F); // convert from BCD
            if (spdData[516] > 0)                                                             // SI101011 - if the raw value is invalid, don't decode it
                MemoryInfo->week = ((spdData[516] >> 4) * 10) + (spdData[516] & 0x0F);        // convert from BCD

            _stprintf_s(g_szSITempDebug1, _T("Manufacturing Date: Year %d Week %d"), MemoryInfo->year, MemoryInfo->week);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleSerialNo = (spdData[517] << 24) | (spdData[518] << 16) | (spdData[519] << 8) | spdData[520];
            _stprintf_s(g_szSITempDebug1, _T("Module serial number: 0x%08X"), MemoryInfo->moduleSerialNo);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            memcpy(MemoryInfo->moduleExtSerialNo, &spdData[512], sizeof(MemoryInfo->moduleExtSerialNo));

            for (i = 0; i < SPD_DDR5_MODULE_PARTNO_LEN; i++)
            {
                if (spdData[521 + i] >= 0 && spdData[521 + i] < 128) // Sanity check for ASCII char
                    MemoryInfo->modulePartNo[i] = spdData[521 + i];
                else
                    break;
            }

#ifdef WIN32
            TCHAR wcPartNo[SPD_DDR5_MODULE_PARTNO_LEN + 1];
            memset(wcPartNo, 0, sizeof(wcPartNo));
            MultiByteToWideChar(CP_ACP, 0, (CHAR *)MemoryInfo->modulePartNo, SPD_DDR5_MODULE_PARTNO_LEN, wcPartNo, SPD_DDR5_MODULE_PARTNO_LEN + 1);
            _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %s"), wcPartNo);
#else
            _stprintf_s(g_szSITempDebug1, _T("Module Part Number: %a"), MemoryInfo->modulePartNo);
#endif
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->moduleRev = spdData[551];
            _stprintf_s(g_szSITempDebug1, _T("Revision Code: 0x%02X"), MemoryInfo->moduleRev);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            numCont = spdData[552] & 0x7F;
            MemoryInfo->specific.LPDDR5SDRAM.DRAMManufBank = numCont + 1;
            MemoryInfo->specific.LPDDR5SDRAM.DRAMManufID = spdData[553];

            GetManNameFromJedecIDContCode(MemoryInfo->specific.LPDDR5SDRAM.DRAMManufID & 0x7F, numCont, MemoryInfo->specific.LPDDR5SDRAM.DRAMManuf, SHORT_STRING_LEN);
            _stprintf_s(g_szSITempDebug1, _T("DRAM Manufacturer: %s (Bank: %d, ID: 0x%02X)"), MemoryInfo->specific.LPDDR5SDRAM.DRAMManuf, MemoryInfo->specific.LPDDR5SDRAM.DRAMManufBank, MemoryInfo->specific.LPDDR5SDRAM.DRAMManufID);

            MtSupportUCDebugWriteLine(g_szSITempDebug1);

            MemoryInfo->specific.LPDDR5SDRAM.DRAMStepping = spdData[554];
            _stprintf_s(g_szSITempDebug1, _T("DRAM Stepping: 0x%02X"), MemoryInfo->specific.LPDDR5SDRAM.DRAMStepping);
            MtSupportUCDebugWriteLine(g_szSITempDebug1);

#ifndef WIN32
            // Bytes 555~639 (0x22B~27F): Manufacturer's Specific Data
            MemoryInfo->manufDataOff = 555;
            MemoryInfo->manufDataLen = 639 - 555 + 1;

            _stprintf_s(tmpMsg, _T("Module Manufacturer's Specific Data raw bytes:"));
            MtSupportUCDebugWriteLine(tmpMsg);

            _stprintf_s(tmpMsg, _T(""));
            for (i = 0; i < MemoryInfo->manufDataLen; i++)
            {
                TCHAR strSPDBytes[8];
                _stprintf_s(strSPDBytes, _T("%02X "), spdData[MemoryInfo->manufDataOff + i]);
                _tcscat(tmpMsg, strSPDBytes);

                if ((i % 16 == 15) || (i == MemoryInfo->manufDataLen - 1))
                {
                    MtSupportUCDebugWriteLine(tmpMsg);
                    _stprintf_s(tmpMsg, _T(""));
                }
            }
#endif
        }
    }
    break;

    default:
        break;
    }

    trim(MemoryInfo->modulePartNo);
    _stprintf_s(tmpMsg, _T("DIMM#%d (Channel %d, Slot %d) serial number: %08X"), MemoryInfo->dimmNum, MemoryInfo->channel, MemoryInfo->slot, MemoryInfo->moduleSerialNo);
#ifdef WIN32
    SysInfo_DebugLogWriteLine(tmpMsg);
#else
    MtSupportUCDebugWriteLine(tmpMsg);
#endif

    // Copy RAW data to memory info
    int maxToCopy = spdDataLen;
    if (maxToCopy > sizeof(MemoryInfo->rawSPDData))
        maxToCopy = sizeof(MemoryInfo->rawSPDData);
    memcpy(MemoryInfo->rawSPDData, spdData, maxToCopy);

    return true;
}

bool CheckSPDBytes(BYTE *spdData, int spdDataLen)
{
    int i;
    bool bSPDSanityOK = false;
    // Just check if all bytes are 0xFF or 0x00 - if so, don't decode the SPD data as it is clearly junk //SI101005

    for (i = 0; i < spdDataLen; i++)
    {
        if (i == 0)
        {
            if (spdData[0] != 0x00 && spdData[0] != 0xFF)
            {
                bSPDSanityOK = true;
                break;
            }
        }
        else if (spdData[0] != spdData[i])
        {
            bSPDSanityOK = true;
            break;
        }
    }
    return bSPDSanityOK;
}

bool CheckCLTTPolling(WORD tsodData[MAX_TSOD_LENGTH])
{
    // If Closed Loop Thermal Throttling (CLTT) is enabled, the TSOD data read from SMBus may be
    // data obtained from hardware-based TSOD polling. In this case, we will see repetitive words
    // in the raw data. Eg.
    //
    // 2025-05-07 14:09:05 - **WARNING** TSOD may be invalid. Raw TSOD bytes for DIMM#0:
    // 2025-05-07 14:09:05 - 4584 4538 4538 4538 4538 4538 4538 4538 4538 4538 4538 4538 4538 4538 4538 4538
    //
    // If this can be detected, the word data can be parsed as temperature directly

    // Count the max number of consecutive, repeated words
    int iRepeated = 0, iMaxRepeated = 0;
    for (int i = 0; i < MAX_TSOD_LENGTH - 1; i++)
    {
        if (tsodData[i] != 0 && tsodData[i] != 0xffff &&
            tsodData[i] == tsodData[i + 1])
            iRepeated++;
        else
        {
            iMaxRepeated = MAX(iMaxRepeated, iRepeated);
            iRepeated = 0;
        }
    }
    iMaxRepeated = MAX(iMaxRepeated, iRepeated);

    // Accept if consecutive, repeated words is greater than threshold
    return iMaxRepeated >= MAX_TSOD_LENGTH / 4;
}

bool CheckTSOD(TSODINFO *tsodInfo)
{
    // TS Capabilities Register
    // Bits 15 - Bit 8 - RFU - Reserved for future use. These bits will always read '0'
    // Bit 7 - EVSD - EVENT_n with Shutdown action.Must be 1.
    // Bit 6 - TMOUT - Bus timeout period during normal operation.Must be 1.
#if 0
    if (!(BitExtract(tsodInfo->raw.DDR4.wCap, 15, 8) == 0 && BitExtract(tsodInfo->raw.DDR4.wCap, 7, 7) && BitExtract(tsodInfo->raw.DDR4.wCap, 6, 6)*/))
        return false;

    // TS Configuration Register
    // Bits 15 - 11 - RFU - Reserved for future use. These bits will always read '0' and writing to them will have no affect. For future compatibility, all RFU bits must be programmed as '0'.
    if (!(BitExtract(tsodInfo->raw.DDR4.wCfg, 15, 8) == 0))
        return false;
#endif
    // High Limit Register
    if (!(BitExtract(tsodInfo->raw.DDR4.wHiLim, 15, 13) == 0 && BitExtract(tsodInfo->raw.DDR4.wHiLim, 1, 0) == 0))
        return false;

    // Low Limit Register
    if (!(BitExtract(tsodInfo->raw.DDR4.wLoLim, 15, 13) == 0 && BitExtract(tsodInfo->raw.DDR4.wLoLim, 1, 0) == 0))
        return false;

    // TCRIT Limit Register
    if (!(BitExtract(tsodInfo->raw.DDR4.wCritLim, 15, 13) == 0 && BitExtract(tsodInfo->raw.DDR4.wCritLim, 1, 0) == 0))
        return false;

    return true;
}

// Reference: JESD400-5
int Crc16(BYTE *ptr, int count)
{
    int crc, i;
    crc = 0;
    while (--count >= 0)
    {
        crc = crc ^ (int)*ptr++ << 8;
        for (i = 0; i < 8; ++i)
            if (crc & 0x8000)
                crc = crc << 1 ^ 0x1021;
            else
                crc = crc << 1;
    }
    return (crc & 0xFFFF);
}

bool IsDDR4SPD(BYTE *spdData, int spdDataLen)
{
    return spdData[2] == SPDINFO_MEMTYPE_DDR4 || spdData[2] == SPDINFO_MEMTYPE_DDR4E || spdData[2] == SPDINFO_MEMTYPE_LPDDR3 || spdData[2] == SPDINFO_MEMTYPE_LPDDR4 || spdData[2] == SPDINFO_MEMTYPE_LPDDR4X;
}

bool IsDDR5SPD(BYTE *spdData, int spdDataLen)
{
    return spdData[2] == SPDINFO_MEMTYPE_DDR5 || spdData[2] == SPDINFO_MEMTYPE_DDR5_NVDIMM_P;
}
