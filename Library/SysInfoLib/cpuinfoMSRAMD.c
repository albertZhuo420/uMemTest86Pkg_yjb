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
//	cpuinfoMSRAMD.c
//
// Author(s):
//	Keith Mah, Ian Robinson
//
// Description:
//	Detects and gathers information about AMD CPU(s).
//	Including speed, manufacturer, capabilities, available L2 cache, etc..
//
//   Based on source code from SysInfo

#include "uMemTest86.h"

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>
// #include <Library/TimerLib.h>

#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>
#include <Library/Cpuid.h>
#include <SysInfoLib/pci.h>
#include "cpuinfoMSRAMD.h"
#include "cpuinfo.h"
#include "cputemp.h"

#define _tcscmp StrCmp
#define _T(x) L##x

// Pstate registers
#define AMD_PSTATES_MSRC0010068_64 5
#define AMD_PSTATES_MSRC001006B_64 8
#define MAX_AMD_PSTATES AMD_PSTATES_MSRC001006B_64

// Registers
#define AMD_MSR_HWCR 0xC0010015             // MSRC001_0015 Hardware Configuration Register (HWCR)
#define AMD_MSR_PSTATECURLIM 0xC0010061     // MSRC001_0061 [P-state Current Limit] (PStateCurLim)
#define AMD_MSR_PSTATECTL 0xC0010062        // MSRC001_0062 [P-state Control] (PStateCtl)
#define AMD_MSR_PSTATESTAT 0xC0010063       // MSRC001_0063 [P-state Status] (PStateStat)
#define AMD_MSR_COFVID_STATUS 0xC0010071    // MSRC001_0071 COFVID Status
#define AMD_MSR_PMGT_MISC 0xC0010292        // MSRC001_0292 [Power Management Miscellaneous] (PMGT_MISC)
#define AMD_MSR_HW_PSTATE_STATUS 0xC0010293 // MSRC001_0293 [Hardware PState Status.] (HW_PSTATE_STATUS)

#define AMD_CPBDIS 25

// CPUID
#define AMD_CPUID_APMI 0x80000007 // CPUID Fn8000_0007 Advanced Power Management Information
#define AMD_CPB 9                 // CPB - Core performacne boost (AMB Turbo Core)

#define NOVALUEAVAILABLE 0
#define INT_NOVALUEAVAILABLE NOVALUEAVAILABLE

#define SysInfo_IsLogging() FALSE
#define SysInfo_DebugLogWriteLine MtSupportUCDebugWriteLine
#define IsDebugLog() SysInfo_IsLogging()
#define DebugLog SysInfo_DebugLogWriteLine
#define SysInfo_DebugString g_wszBuffer
#define g_szSITempDebug1 g_wszBuffer
#define SysInfo_DebugLogF(...)                  \
    if (SysInfo_IsLogging())                    \
    {                                           \
        FPrint(DEBUG_FILE_HANDLE, __VA_ARGS__); \
    }
#define _T(x) L##x
#define wcslen StrLen
#define wcsstr StrStr
#define _stprintf_s(buf, ...) UnicodeSPrint(buf, sizeof(buf), __VA_ARGS__)
#define Sleep(x) gBS->Stall(x * 1000)

BOOLEAN IsAMDTurboCoreSupported(CPUINFO *pCPUInfo);
BOOLEAN DisableAMDCPB(UINT64 *ullAMD_MSR_HWCR_orig);
BOOLEAN EnableAMDCPB();
BOOLEAN ReEnableAMDCPB(UINT64 ullAMD_MSR_HWCR);
int GetAMDMult(BOOLEAN bLog, CPUINFO *pCPUInfo, UINT64 ullMSRResult, int *iCPUDid_P, int *iCPUFid_P);
BOOLEAN Get_MPERF_APERF(CPUINFO *pCPUInfo, int *flAPERF, int *flMPERF, int *flMult, int flCPUFreq_RDTSC, BOOLEAN bTurbo, int flTargetMult);
BOOLEAN GetAMD10Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD11Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD12Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD14Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD15Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD16Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD17Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetHygon18Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetAMD19Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
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

// AMD temperature
bool Get_AMD_PCI_bits(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD dwIndexHigh, DWORD dwIndexLow, int *piReturn);
int Get_AMD_10_CurTmp();
int Get_AMD_10_LnkFreq();
int Get_AMD_11_CurTmp();
int Get_AMD_11_LnkFreq();
int Get_AMD_12_14_CurTmp();
int Get_AMD_15_CurTmp(int iModel);
int Get_AMD_16_CurTmp();
int Get_AMD_17_CurTmp(wchar_t *szTypeString);
int Get_AMD_19_CurTmp(wchar_t *szTypeString);
int Get_AMD_1A_CurTmp(wchar_t *szTypeString);

int Get_AMD_12_PCI_NumberBoostedStates();

extern UINTN cpu_speed;

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
//
// Get AMD cache info
//
// Note the special work around for the AMD Duron which has a bug in it that
// causes the L2 cache size to be reported wrongly. See AMD Tech doc TN13 revision 3
//
void GetAMDCacheInfo(CPUINFO *pCPUInfo)
{
    // BIT601000
    unsigned int CPUInfoL1Cache[4];
    unsigned int CPUInfoL2Cache[4];
    unsigned int CPUInfoL3Cache[4];
    int iAMDCoresPerPackage = 0;

    pCPUInfo->L2_cache_size = 0;
    pCPUInfo->L3_cache_size = 0; // Level 3 cahce size not detected yet
    pCPUInfo->cacheinfo_num = 0;

    // BIT601000
    iAMDCoresPerPackage = GetCoresPerPackage(pCPUInfo);

    // L1 Cache
    if (pCPUInfo->MaxExtendedCPUID >= 0x80000005) // L1 Cache info is available
    {
        AsmCpuid(0x80000005, &CPUInfoL1Cache[0], &CPUInfoL1Cache[1], &CPUInfoL1Cache[2], &CPUInfoL1Cache[3]);

        pCPUInfo->L1_instruction_cache_size = BitExtract(CPUInfoL1Cache[2], 31, 24); // ECX[31:24] L1 data cache (KB) per CORE
        pCPUInfo->L1_data_cache_size = BitExtract(CPUInfoL1Cache[3], 31, 24);        // EDX[31:24] L1 instr. cache (KB) per CORE

        pCPUInfo->L1_instruction_caches_per_package = iAMDCoresPerPackage;
        pCPUInfo->L1_data_caches_per_package = iAMDCoresPerPackage;
    }

    // L2 Cache
    if (pCPUInfo->MaxExtendedCPUID >= 0x80000006) // L2 Cache info is available
    {
        // AMD Duron
        if (pCPUInfo->Family == 6 && (pCPUInfo->Model == 3 || pCPUInfo->Model == 7) && pCPUInfo->stepping == 0)
            pCPUInfo->L2_cache_size = SIZE_64KB;
        else
        {
            AsmCpuid(0x80000006, &CPUInfoL2Cache[0], &CPUInfoL2Cache[1], &CPUInfoL2Cache[2], &CPUInfoL2Cache[3]);
            pCPUInfo->L2_cache_size = (CPUInfoL2Cache[2] >> 16) & 0xffff; // ECX[31:16] L2 cache (KB) per CORE
        }
        pCPUInfo->L2_caches_per_package = iAMDCoresPerPackage;
    }

    // L3 Cache
    if (pCPUInfo->MaxExtendedCPUID >= 0x80000006) // L3 Cache info is available
    {
        AsmCpuid(0x80000006, &CPUInfoL3Cache[0], &CPUInfoL3Cache[1], &CPUInfoL3Cache[2], &CPUInfoL3Cache[3]);

        pCPUInfo->L3_cache_size = BitExtract(CPUInfoL3Cache[3], 31, 18) * 512; // EDX[31:18] L3 data cache * 512 KB per PACKAGE //BIT601000.0014 (*512)
        if (pCPUInfo->L3_cache_size > 0)
            pCPUInfo->L3_caches_per_package = 1;
        else
            pCPUInfo->L3_caches_per_package = 0;
    }
}

// GetAMDFamilyMSRInfo
//
//  Get AMD CPU info that is specific to the products MSRs (Model Specific Registers). Like CPU multipliers.
//
//  Note: From Family 15h, these MSRs are per node, rather than per package. Seems only relevant to dual node Opertons.
//
//  1) Determine if CPB supported
//  2) Get the number of P-states
//  3) Get the number of Boosted Pstates
//  4) Get the current and Maximum Pstate index
//  5) Get the multiplier for all P-states
//  6) Measure the Actual and Maximum performance after loading a single CPU
//  7) If CPB supported, disable CPB and measure the Actual and Maximum performance after loading a single CPU, re-enable CPB
//  8) Use the minimum multiplier, from 4 & 5.
//  9) Use the maximum non-boosted multiplier, from 4 & 5. This assumes the performance measured in 7 (or 6) is the max. non-boosted CPU freq - i.e. reached max.
//  10) Use the maximum boosted multiplier, using 4 & 5. This may not actually be acheivable as the speed measured in 6 may never get to this Pstate.
//  11) Derive the Base block using the Maximum perf and max non-boosted multiplier.
//  12) Sanity check that the boosted perf is higher than the non-boosted perf, if not, remove the boosted info.
//  13) Set the overclocking info based on the stock base clock speed. Unknown if multiplier overclocked.
//
void GetAMDFamilyMSRInfo(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    int iMSR = 0;
    UINT64 ullMSRResult = 0;
    int iNumBoostPStates = 0, iCurrentPState = 0, iMaxPState = 0;
    int iCPUDid_P[MAX_AMD_PSTATES], iCPUFid_P[MAX_AMD_PSTATES];
    int flAPERF = 0, flMPERF = 0, flMult = 0, flAPERF_CPB_disabled = 0, flMPERF_CPB_disabled = 0, flMult_CPB_disabled = 0, flBaseClock = 0;
    int flMultiplier_P[MAX_AMD_PSTATES];
    int flTargetMult = 0;

    BOOLEAN bAMDTurboCoreSupported = FALSE;
    BOOLEAN bTurboEnabled = FALSE;

    int iPState = 0;
    int iMaxNumPstates = 0;

    SetMem(iCPUDid_P, sizeof(iCPUDid_P), 0);
    SetMem(iCPUFid_P, sizeof(iCPUFid_P), 0);
    SetMem(flMultiplier_P, sizeof(flMultiplier_P), 0);

    AsciiFPrint(DEBUG_FILE_HANDLE, "GetAMDFamilyMSRInfo - CPU %s:\n"
                                   "  Family:   0x%02X\n"
                                   "  Model:    0x%02X\n"
                                   "  Stepping: 0x%X",
                pCPUInfo->typestring, pCPUInfo->Family, pCPUInfo->Model, pCPUInfo->stepping);

    bAMDTurboCoreSupported = IsAMDTurboCoreSupported(pCPUInfo);

    iNumBoostPStates = -1; // Use -1 to mark that the number of boost states could not be determined and that this value should not be used - SI101022b
    if (StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        switch (pCPUInfo->Family)
        {
        case 0x10:
            iMaxNumPstates = AMD_PSTATES_MSRC0010068_64;
            if (bAMDTurboCoreSupported)
                iNumBoostPStates = 1;
            break;
        case 0x11:
            iMaxNumPstates = AMD_PSTATES_MSRC001006B_64;
            iNumBoostPStates = 0; // No boost states
            break;
        case 0x12:
        case 0x14:
        case 0x15:
        case 0x16: // SI101036
            iMaxNumPstates = AMD_PSTATES_MSRC001006B_64;
            if (bAMDTurboCoreSupported)
            {
                // iNumBoostPStates = Get_AMD_PCI_bits(0, 0x18, 0x4, 0x15, 4, 2);	//D18F4x15C[4:2]	//SI101021 rev2 - BIT7010060002 for 0x14/0x15
                BOOLEAN bRet = Get_AMD_PCI_bits(0, 0x18, 0x4, 0x15C, 4, 2, &iNumBoostPStates);
                if (!bRet)
                    iNumBoostPStates = -1; // SI101022c
            }
            break;
        case 0x17:
        case 0x19:
        case 0x1A:
            iMaxNumPstates = AMD_PSTATES_MSRC001006B_64;
            if (bAMDTurboCoreSupported)
                iNumBoostPStates = -1;
            break;
        default:
            // AMD CPU not supported
            return;
        }

        // Get some P-state info
        if (pCPUInfo->Family < 0x17) // COFVID Status Does not exist on >= 17h
        {
            iMSR = AMD_MSR_COFVID_STATUS;
            ullMSRResult = AsmReadMsr64(iMSR);
            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - MSR[0x%08x] = %016lx", iMSR, ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);

            iMaxPState = (int)(BitExtractULL(ullMSRResult, 58, 56)); // Normally 0 i.e. the higest Psate allowed. If for example, on 10h family if CPB is disabled, then P0 is not allowed, and the highest Pstate is 1. This value would then be one. i.e. can't have a higher Pstate
        }
        else
        {
            iMSR = AMD_MSR_PMGT_MISC;
            ullMSRResult = AsmReadMsr64(iMSR);
            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - MSR[0x%08x] = %016lx", iMSR, ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);

            iMaxPState = (int)(BitExtractULL(ullMSRResult, 2, 0)); // Normally 0 i.e. the higest Psate allowed. If for example, on 10h family if CPB is disabled, then P0 is not allowed, and the highest Pstate is 1. This value would then be one. i.e. can't have a higher Pstate
        }

        // Init
        // Base clock: 0x14 and 0x15 - should this use D0F0xE4_x0130_80F1[ClockRate] BIOS Timer Control
        switch (pCPUInfo->Family)
        {
        case 0x12: // SI101022 - Stock clock is 100MHz (as per A4-3400 in office)
        case 0x16: // SI101036 - BKDG - "REFCLK Reference Clock, refers to the clock frequency (100 MHz) or ..." (and based on CPU-Z submitted results)
        case 0x17:
        case 0x19:
        case 0x1A:
            pCPUMSRInfo->flCPUExternalClockStock = 100000;
            pCPUMSRInfo->flCPUExternalClockBoosted = 0;
            pCPUMSRInfo->flFSB = 0;
            break;
        default:
            pCPUMSRInfo->flCPUExternalClockStock = 200000; // Stock clock is 200MHz - OK for family 10h, not sure about others ("CLKIN. The reference clock provided to the processor, nominally 200Mhz")
            pCPUMSRInfo->flCPUExternalClockBoosted = 0;
            pCPUMSRInfo->flFSB = 0;
        }
    }
    else if (StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0)
    {
        switch (pCPUInfo->Family)
        {
        case 0x18: // Same as AMD 17h
            iMaxNumPstates = AMD_PSTATES_MSRC001006B_64;
            if (bAMDTurboCoreSupported)
                iNumBoostPStates = -1;
            break;
        default:
            // AMD CPU not supported
            return;
        }

        // Get some P-state info
        if (pCPUInfo->Family < 0x18) // COFVID Status Does not exist on 17h
        {
            iMSR = AMD_MSR_COFVID_STATUS;
            ullMSRResult = AsmReadMsr64(iMSR);
            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - MSR[0x%08x] = %016lx", iMSR, ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);

            iMaxPState = (int)(BitExtractULL(ullMSRResult, 58, 56)); // Normally 0 i.e. the higest Psate allowed. If for example, on 10h family if CPB is disabled, then P0 is not allowed, and the highest Pstate is 1. This value would then be one. i.e. can't have a higher Pstate
        }
        else
        {
            iMSR = AMD_MSR_PMGT_MISC;
            ullMSRResult = AsmReadMsr64(iMSR);
            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - MSR[0x%08x] = %016lx", iMSR, ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);

            iMaxPState = (int)(BitExtractULL(ullMSRResult, 2, 0)); // Normally 0 i.e. the higest Psate allowed. If for example, on 10h family if CPB is disabled, then P0 is not allowed, and the highest Pstate is 1. This value would then be one. i.e. can't have a higher Pstate
        }

        // Init
        // Base clock: 0x14 and 0x15 - should this use D0F0xE4_x0130_80F1[ClockRate] BIOS Timer Control
        switch (pCPUInfo->Family)
        {
        case 0x18:
            pCPUMSRInfo->flCPUExternalClockStock = 100000;
            pCPUMSRInfo->flCPUExternalClockBoosted = 0;
            pCPUMSRInfo->flFSB = 0;
            break;
        default:
            pCPUMSRInfo->flCPUExternalClockStock = 200000; // Stock clock is 200MHz - OK for family 10h, not sure about others ("CLKIN. The reference clock provided to the processor, nominally 200Mhz")
            pCPUMSRInfo->flCPUExternalClockBoosted = 0;
            pCPUMSRInfo->flFSB = 0;
        }
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - MSR Max Pstates/#boosted states: %d, %d", iMaxNumPstates, iNumBoostPStates);
    MtSupportDebugWriteLine(gBuffer);

    // Get all of the P-states
    for (iPState = 0; iPState < iMaxNumPstates; iPState++)
    {
        iMSR = 0xC0010064 + iPState; // MSRC001_00[6B:64] P-state [7:0]
        ullMSRResult = AsmReadMsr64(iMSR);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - MSR[%x] = %016lx", iMSR, ullMSRResult);
        MtSupportDebugWriteLine(gBuffer);
        {
            flMultiplier_P[iPState] = GetAMDMult(TRUE, pCPUInfo, ullMSRResult, &iCPUDid_P[iPState], &iCPUFid_P[iPState]);

            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - Multiplier_P[%d] = %d", iPState, flMultiplier_P[iPState]);
            MtSupportDebugWriteLine(gBuffer);
        }
    }

    // Measure CPU dynamic attributes: speed, mutliplier with Turbo mode disabled
    if (bAMDTurboCoreSupported)
        bTurboEnabled = EnableAMDCPB();

    // Get the CPU frequencies
    flTargetMult = bTurboEnabled ? flMultiplier_P[0] * 115 : flMultiplier_P[0]; // If turbo, set the target to 1.15x the max non-turbo multiplier
    Get_MPERF_APERF(pCPUInfo, &flAPERF, &flMPERF, &flMult, (int)cpu_speed, bTurboEnabled, flTargetMult);

    AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - APERF = %d, MPERF = %d, Mult = %d", flAPERF, flMPERF, flMult);
    MtSupportDebugWriteLine(gBuffer);

    if (flMult > 0)
        flBaseClock = flAPERF * 100 / flMult;

    AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - BaseClock = %d", flBaseClock);
    MtSupportDebugWriteLine(gBuffer);

    // AMD 12h - Some Llano's appear to have a bug where MPERF and APERF appear back the front. This is a workaround (won't work for underclocking).	//SI101024
    // If the multiplier is not the maximum configured multiplier and MPERF is at least 1 multiplier higher than APERF and it looks like we can use the highest multiplier, the swap
    if (pCPUInfo->Family == 0x12 && (flMult != flMultiplier_P[0]) && (flMPERF > (flAPERF + 90000)) && (ABS(flBaseClock * flMultiplier_P[0] - flMPERF * 100) < 20000))
    {
        int flTemp = flAPERF;
        flAPERF = flMPERF;
        flMPERF = flTemp;           // Swap APERF/MPERF
        flMult = flMultiplier_P[0]; // Use the maximum configured multiplier
    }

    // Save the frequency, base clock and multiplier information
    if (bAMDTurboCoreSupported && bTurboEnabled)
    {
        // Meaure the frequencies with CPB disabled
        UINT64 ullAMD_MSR_HWCR = 0ULL;

        MtSupportDebugWriteLine("GetAMDFamilyMSRInfo - Turbo Core supported");

        // if (DisableAMDCPB(&ullAMD_MSR_HWCR))
        DisableAMDCPB(&ullAMD_MSR_HWCR);
        {
            // Get CPU frequencies
            Get_MPERF_APERF(pCPUInfo, &flAPERF_CPB_disabled, &flMPERF_CPB_disabled, &flMult_CPB_disabled, (int)cpu_speed, FALSE, flMultiplier_P[0]);

            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - (CPB disabled) APERF = %d, MPERF = %d, Mult = %d", flAPERF_CPB_disabled, flMPERF_CPB_disabled, flMult_CPB_disabled);
            MtSupportDebugWriteLine(gBuffer);

            if (flMult_CPB_disabled > 0)
                flBaseClock = flAPERF_CPB_disabled * 100 / flMult_CPB_disabled; // Use the non-boosted measurements for base clock

            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - (CPB disabled) BaseClock = %d", flBaseClock);
            MtSupportDebugWriteLine(gBuffer);

            // AMD 12h - Some Llano's appear to have a bug where MPERF and APERF appear back the front. This is a workaround (won't work for underclocking).	//SI101024
            // If the multiplier is not the maximum configured multiplier and MPERF is at least 1 multiplier higher than APERF and it looks like we can use the highest multiplier, the swap
            if (pCPUInfo->Family == 0x12 && iNumBoostPStates >= 0 && iNumBoostPStates < iMaxNumPstates && (flMult_CPB_disabled != flMultiplier_P[iNumBoostPStates]) &&
                (flMPERF_CPB_disabled > (flAPERF_CPB_disabled + 90000)) && (ABS(flBaseClock * flMultiplier_P[iNumBoostPStates] - flMPERF_CPB_disabled * 100) < 20000))
            {
                int flTemp = flAPERF_CPB_disabled;
                flAPERF_CPB_disabled = flMPERF_CPB_disabled;
                flMPERF_CPB_disabled = flTemp;                          // Swap APERF/MPERF
                flMult_CPB_disabled = flMultiplier_P[iNumBoostPStates]; // Use the maximum configured multiplier
            }

            ReEnableAMDCPB(ullAMD_MSR_HWCR);
        }

        // Save Min, max and boosted Multipliers
        if ((iMaxNumPstates - 1) > 0 && (iMaxNumPstates - 1) < iMaxNumPstates)
            pCPUMSRInfo->flMinMult = flMultiplier_P[iMaxNumPstates - 1]; // The min mult. is to the last P-state
        if (iNumBoostPStates >= 0 && iNumBoostPStates < iMaxNumPstates)
            pCPUMSRInfo->flMaxMult = flMultiplier_P[iNumBoostPStates]; // The max mult. is to the first P-state, taking into account the Pstates that are limited - this can be a bootedor non-boosted P-state
        if (iNumBoostPStates > 0 && iNumBoostPStates < iMaxNumPstates)
            pCPUMSRInfo->flBoostedMult = flMultiplier_P[0];

        // Store results
        pCPUMSRInfo->raw_freq_cpu = flAPERF_CPB_disabled;
        pCPUMSRInfo->flBusFrequencyRatio = flMult_CPB_disabled;
        pCPUMSRInfo->flExternalClock = flBaseClock;

        pCPUMSRInfo->flCPUSpeedTurbo = flAPERF;
        pCPUMSRInfo->flCPUMultTurbo = flMult;
        pCPUMSRInfo->flCPUExternalClockBoosted = flBaseClock;

        // Calculate the maximum theorectical CPU speed based on the higest power state multiplier and the base clock //SI101022c
        // As an AMD example. Lets say the turbo multiplier was stock at x21, but OC'ed to x25. But the speed was only measured to be 3600MHz (200x18).
        // ie.the system did not get up to the highest trubo speed (due to heat or due to another program runnign on the same CPU core during measurement).
        // PT could get the programmed multiplier for the highest power state, ie. x25, this could be stored in the baseline and DB. This could then be
        // compared against the stock turbo multiplier, e.g. x21 - and this would show the turbo multiplier was OCed (even though the speed measrued does
        // not show this).
        pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax = pCPUMSRInfo->flExternalClock * flMultiplier_P[0] / 100;

        AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - CPUSpeedTurboTheoreticalMax = %d", pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax);
        MtSupportDebugWriteLine(gBuffer);

        // Sanity check that CPB is enabled/working - check that the turbo and non turbo speeds are at least (0.5 (multipler) x 200 MHz (base clock) - 20 (reasonale delta))
        // if (flAPERF-flAPERF_CPB_disabled < 80)
        // SI101022c - if the trubo speed is less than the non-turbo speed then we have a measurement problem e.g. 3300 non turbo, blank turbo is < 3220
        if (flAPERF < flAPERF_CPB_disabled - 80000)
        {
            pCPUMSRInfo->flCPUSpeedTurbo = 0;
            pCPUMSRInfo->flCPUMultTurbo = 0;
            pCPUMSRInfo->flBoostedMult = 0;
        }
        // Sanity check theorectical max		//SI101022c
        if (pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax > CPUSPEEDMAX)
            pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax = pCPUMSRInfo->flCPUExternalClockStock * flMultiplier_P[0] / 100;
        if (pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax < pCPUMSRInfo->flCPUSpeedTurbo)
            pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax = pCPUMSRInfo->flCPUSpeedTurbo;
    }
    else
    {
        // If we measured an APERF frequency then use that, otherwise derive it from the already measured RDTSC frequency
        if (flAPERF > 100000 /*KHz*/)
        {
            pCPUMSRInfo->raw_freq_cpu = flAPERF;
            pCPUMSRInfo->flBusFrequencyRatio = flMult;
            pCPUMSRInfo->flExternalClock = flBaseClock;
            if ((iMaxNumPstates - 1) > 0 && (iMaxNumPstates - 1) < iMaxNumPstates)
                pCPUMSRInfo->flMinMult = flMultiplier_P[iMaxNumPstates - 1]; // The min mult. is to the last P-state
            pCPUMSRInfo->flMaxMult = flMultiplier_P[0];                      // The max mult. is to the first P-state, taking into account the Pstates that are limited - this can be a bootedor non-boosted P-state
        }
        else
        {
            int flMinMult = 0, flMaxMult = 0, flBoostedMult = 0;

            // Get some P-state info
            iMSR = AMD_MSR_PSTATECURLIM;
            ullMSRResult = AsmReadMsr64(iMSR);

            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - MSR[0x%08x] = %016lx", iMSR, ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);
            {
                iCurrentPState = (int)(BitExtractULL(ullMSRResult, 2, 0));
                iMaxPState = (int)(BitExtractULL(ullMSRResult, 6, 4)); // Highest index into PState (3 for Turbo core, else 4). for lowest multiplier
            }

            iNumBoostPStates = 0;

            if (iMaxPState >= 0 && iMaxPState < AMD_PSTATES_MSRC0010068_64)
                flMinMult = flMultiplier_P[iMaxPState];
            flMaxMult = flMultiplier_P[0];
            flBoostedMult = 0;

            if (flMaxMult > 0) // sanity
            {
                pCPUMSRInfo->raw_freq_cpu = (int)cpu_speed;
                pCPUMSRInfo->flBusFrequencyRatio = flMaxMult;
                pCPUMSRInfo->flExternalClock = (int)(cpu_speed * 100 / flMaxMult); // Derive base clock, assume was max speed
                pCPUMSRInfo->flMinMult = flMinMult;
                pCPUMSRInfo->flMaxMult = flMaxMult;

                if (pCPUInfo->Family == 0x10)
                    pCPUMSRInfo->flHTFreq = Get_AMD_10_LnkFreq() * pCPUMSRInfo->flExternalClock / 200; // Get stock HT link and rebase
            }
        }
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - (Before sanity check) Freq: %dKHz (Ratio: %d, ExtClk: %dKhz), Turbo: %dKHz (Ratio: %d, ExtClk: %dKHz) (Theoretical Max: %dKhz)",
                pCPUMSRInfo->raw_freq_cpu, pCPUMSRInfo->flBusFrequencyRatio, pCPUMSRInfo->flExternalClock,
                pCPUMSRInfo->flCPUSpeedTurbo, pCPUMSRInfo->flCPUMultTurbo, pCPUMSRInfo->flCPUExternalClockBoosted,
                pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax); // SI101022c
    MtSupportDebugWriteLine(gBuffer);

    // Sanity checks
    if (pCPUMSRInfo->raw_freq_cpu * 100 < (60 * (int)cpu_speed))
    {
        pCPUMSRInfo->raw_freq_cpu = (int)cpu_speed;
        pCPUMSRInfo->flBusFrequencyRatio = 0;
        pCPUMSRInfo->flExternalClock = 0;

        pCPUMSRInfo->flMinMult = 0;
        pCPUMSRInfo->flMaxMult = 0;
    }

    if (pCPUMSRInfo->flCPUSpeedTurbo * 100 < (60 * (int)cpu_speed))
    {
        pCPUMSRInfo->flCPUSpeedTurbo = 0;
        pCPUMSRInfo->flCPUMultTurbo = 0;
        pCPUMSRInfo->flCPUExternalClockBoosted = 0;

        pCPUMSRInfo->flBoostedMult = 0;
    }

    // Do a sanity check on the Turbo multiplier, as this can be wrong in some cases
    if (ABS((pCPUMSRInfo->flCPUExternalClockBoosted * pCPUMSRInfo->flCPUMultTurbo) - pCPUMSRInfo->flCPUSpeedTurbo * 100) > 90000 /*KHz*/) // SI101022c
    {
        pCPUMSRInfo->flCPUMultTurbo = 0;
    }

    //
    // Set Overclock flags - Don't know if this is correct as it assumes the base clock speed to 200MHz - not sure about this
    //
    // If the base clock is more than 5% greater than the stock base clock, then set overclocked flag. assume base clk of 200.f AMD 12h???
    pCPUMSRInfo->OCLKBaseClock = CPU_OVERCLOCKED_UNKNOWN;
    if (pCPUMSRInfo->flExternalClock * 100 > (pCPUMSRInfo->flCPUExternalClockStock * 105))
        pCPUMSRInfo->OCLKBaseClock = CPU_OVERCLOCKED;
    else if (pCPUMSRInfo->flExternalClock * 100 < (pCPUMSRInfo->flCPUExternalClockStock * 95))
        pCPUMSRInfo->OCLKBaseClock = CPU_UNDERCLOCKED;
    else
        pCPUMSRInfo->OCLKBaseClock = CPU_OVERCLOCKED_NO;

    pCPUMSRInfo->OCLKMultiplier = CPU_OVERCLOCKED_UNKNOWN;

    pCPUMSRInfo->flFSB = 0;

    AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDFamilyMSRInfo - Freq: %d, %d, %d, Turbo: %d, %d, %d (%d)",
                pCPUMSRInfo->raw_freq_cpu, pCPUMSRInfo->flBusFrequencyRatio, pCPUMSRInfo->flExternalClock,
                pCPUMSRInfo->flCPUSpeedTurbo, pCPUMSRInfo->flCPUMultTurbo, pCPUMSRInfo->flCPUExternalClockBoosted,
                pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax); // SI101022c
    MtSupportDebugWriteLine(gBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 10-15h Processor MSRs - common
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDTurboCoreSupported
//
//  Determines if a CPU supports AMD Turbo Core.
//  Support for core performance boost is indicated by EDX bit 9 as returned by CPUID function.
//
BOOLEAN IsAMDTurboCoreSupported(CPUINFO *pCPUInfo)
{
    BOOLEAN bTurboCore = FALSE;
    BOOLEAN bCPBDisable = FALSE;
    UINT32 CPUInfo[4];
    int iMSR = 0;
    UINT64 ullMSRResult = 0;

    if (StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0 || StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0)
    {
        // Support for core performance boost is indicated by EDX bit 9 as returned by CPUID function 8000_0007h

        if (pCPUInfo->MaxExtendedCPUID >= AMD_CPUID_APMI) // 0x8000_0007, Advanced power management information, supported //CPB - "Core Performance Boost" - AMD version of Turbo
        {
            AsmCpuid(AMD_CPUID_APMI, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

            AsciiSPrint(gBuffer, BUF_SIZE, "IsAMDTurboCoreSupported - CPUID[0x80000007][EDX] = %08x", CPUInfo[3]);
            MtSupportDebugWriteLine(gBuffer);

            bTurboCore = (BitExtractULL(CPUInfo[3], AMD_CPB, AMD_CPB) == 1);
#if 0
      //MSRC001_0292 [Power Management Miscellaneous] (PMGT_MISC)
      iMSR = AMD_MSR_PMGT_MISC;
      ullMSRResult = AsmReadMsr64(iMSR);

      AsciiSPrint(gBuffer, BUF_SIZE, "IsAMDTurboCoreSupported - Power Management Miscellaneous (PMGT_MISC): %016lx", ullMSRResult);
      MtSupportDebugWriteLine(gBuffer);
#endif
            // MSRC001_0015 Hardware Configuration Register (HWCR)
            iMSR = AMD_MSR_HWCR;
            ullMSRResult = AsmReadMsr64(iMSR);

            MtSupportDebugWriteLine("HWCR register found. CPB enable/disable functionality supported");
            bTurboCore = TRUE;

            bCPBDisable = (BitExtractULL(ullMSRResult, AMD_CPBDIS, AMD_CPBDIS) == 1);
            AsciiSPrint(gBuffer, BUF_SIZE, "CPB is: %s", bCPBDisable ? L"disabled" : L"enabled");
            MtSupportDebugWriteLine(gBuffer);
        }
    }

    return bTurboCore;
}

// GetAMDMult
//
//  Get the multiplier for a particular Pstate
//
int GetAMDMult(BOOLEAN bLog, CPUINFO *pCPUInfo, UINT64 ullMSRResult, int *iCPUDid_P, int *iCPUFid_P)
{
    int flMultiplier = 0, flDivisor = -1;
    int iMainPllOpFreqId = 0, iCPUDidRaw = 0;
    BOOLEAN bRet = FALSE;

    if (StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        switch (pCPUInfo->Family)
        {
        case 0x10:
        case 0x11:
        case 0x15: // SI101021 rev2 - BIT7010060002
        case 0x16: // SI101036
            // The COF for core P-states is a function of half the CLKIN frequency (nominally 100 MHz) and the DID may be 1, 2, 4, 8, and 16.
            *iCPUDid_P = (int)(BitExtractULL(ullMSRResult, 8, 6));
            *iCPUFid_P = (int)(BitExtractULL(ullMSRResult, 5, 0));

            if (*iCPUDid_P >= 0 && *iCPUDid_P <= 4) // SI101021 rev2 - BIT7010060002 - validate value
                flMultiplier = (100 * (*iCPUFid_P + 0x10)) >> (*iCPUDid_P + 1);
            break;

        case 0x12:
            // The COF for core P-states
            *iCPUDid_P = (int)(BitExtractULL(ullMSRResult, 3, 0));
            *iCPUFid_P = (int)(BitExtractULL(ullMSRResult, 8, 4));

            iCPUDidRaw = *iCPUDid_P;
            if (iCPUDidRaw == 0)
                flDivisor = 10;
            else if (iCPUDidRaw == 1)
                flDivisor = 15;
            else if (iCPUDidRaw == 2)
                flDivisor = 20;
            else if (iCPUDidRaw == 3)
                flDivisor = 30;
            else if (iCPUDidRaw == 4)
                flDivisor = 40;
            else if (iCPUDidRaw == 5)
                flDivisor = 60;
            else if (iCPUDidRaw == 6)
                flDivisor = 80;
            else if (iCPUDidRaw == 7)
                flDivisor = 120;
            else if (iCPUDidRaw == 8)
                flDivisor = 160;
            else
                flDivisor = -1;

            if (flDivisor > 0)
                flMultiplier = 1000 * (*iCPUFid_P + 0x10) / flDivisor;

            break;

        case 0x14: // Note: for both AMD_BIOS_DEV_GUIDE_14h_models00h-0Fh_(Ontario)_(FT1) & AMD_BIOS_DEV_GUIDE_14h_models10h-1Fh_(Krishna)_(FT2)
            // The COF for core P-states
            *iCPUDid_P = (int)(BitExtractULL(ullMSRResult, 3, 0));
            *iCPUFid_P = (int)(BitExtractULL(ullMSRResult, 8, 4));

            // D18F3xD4 Clock Power/Timing Control 0, 5:0 MainPllOpFreqId: main PLL operating frequency ID
            // iMainPllOpFreqId = Get_AMD_PCI_bits(0, 0x18, 0xF3, 0xD4, 5, 0);
            bRet = Get_AMD_PCI_bits(0, 0x18, 0xF3, 0xD4, 5, 0, &iMainPllOpFreqId);
            if (bRet && iMainPllOpFreqId != 0) // SI101022c
                flMultiplier = 10000 * (iMainPllOpFreqId + 0x10) / (100 * (*iCPUFid_P) + (*iCPUDid_P * 25) + 100);
            break;

        case 0x17:
        case 0x19:
        case 0x1A:
            // The COF for core P-states
            *iCPUDid_P = (int)(BitExtractULL(ullMSRResult, 13, 8));
            *iCPUFid_P = (int)(BitExtractULL(ullMSRResult, 7, 0));

            iCPUDidRaw = *iCPUDid_P;
            if (iCPUDidRaw >= 0x08)
                flDivisor = iCPUDidRaw;
            else
                flDivisor = -1;

            if (flDivisor > 0)
                flMultiplier = (1000 * (*iCPUFid_P) / flDivisor) * 2; // CoreCOF = (Core::X86::Msr::PStateDef[CpuFid[7:0]] / Core::X86::Msr::PStateDef[CpuDfsId])*200.

            break;

        default:
            break;
        }
    }
    else if (StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0)
    {
        switch (pCPUInfo->Family)
        {
        case 0x18: // Same as AMD 17h
            // The COF for core P-states
            *iCPUDid_P = (int)(BitExtractULL(ullMSRResult, 13, 8));
            *iCPUFid_P = (int)(BitExtractULL(ullMSRResult, 7, 0));

            iCPUDidRaw = *iCPUDid_P;
            if (iCPUDidRaw >= 0x08)
                flDivisor = iCPUDidRaw;
            else
                flDivisor = -1;

            if (flDivisor > 0)
                flMultiplier = (1000 * (*iCPUFid_P) / flDivisor) * 2; // CoreCOF = (Core::X86::Msr::PStateDef[CpuFid[7:0]] / Core::X86::Msr::PStateDef[CpuDfsId])*200.

            break;

        default:
            break;
        }
    }

    if (flMultiplier < 0 || flMultiplier > 100000)
        flMultiplier = 0; // sanity

    AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDMult - CPUDid_P = %d, CPUFid_P = %d", *iCPUDid_P, *iCPUFid_P);
    MtSupportDebugWriteLine(gBuffer);
    return flMultiplier;
}

// DisableAMDCPB
//
//  Disable AMD Core Performacne Boost (Turbo)
//
BOOLEAN DisableAMDCPB(UINT64 *ullAMD_MSR_HWCR_orig)
{
    // MSRC001_0015 Hardware Configuration Register (HWCR)
    // 25 CpbDis: core performance boost disable. Read-write. Specifies whether core performance boost is enabled or disabled. 0=CPB is enabled. 1=CPB is disabled.
    int iMSR = 0;
    UINT64 ullAMD_MSR_HWCR = 0ULL;
    UINT64 ullAMD_MSR_PSTATECTL = 0ULL;
    BOOLEAN bCPDDisabled = FALSE;

    // Disable AMD CPB
    iMSR = AMD_MSR_HWCR;
    ullAMD_MSR_HWCR = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "DisableAMDCPB - MSR[0x%08x] = %016lx", iMSR, ullAMD_MSR_HWCR);
    MtSupportDebugWriteLine(gBuffer);
    {
        *ullAMD_MSR_HWCR_orig = ullAMD_MSR_HWCR; // Return the original state, for the re-enable

        // BOOLEAN bCPDDisabled = BitExtractULL(ullAMD_MSR_HWCR, 25, 25) != 0;
        // if (bCPDDisabled)
        {
            UINT64 ullValue = ullAMD_MSR_HWCR | (1 << 25); // Set MSRC001_0015[25:25] to 1 - disable CPB

            AsmWriteMsr64(iMSR, ullValue);
            ullAMD_MSR_HWCR = AsmReadMsr64(iMSR);
            AsciiSPrint(gBuffer, BUF_SIZE, "DisableAMDCPB - New MSR[0x%08x] = %016lx", iMSR, ullAMD_MSR_HWCR);
            MtSupportDebugWriteLine(gBuffer);
            {
                bCPDDisabled = BitExtractULL(ullAMD_MSR_HWCR, 25, 25) != 0;

                if (bCPDDisabled)
                {
                    // Set P-state to 0
                    iMSR = AMD_MSR_PSTATECTL;
                    ullAMD_MSR_PSTATECTL = AsmReadMsr64(iMSR);
                    AsciiSPrint(gBuffer, BUF_SIZE, "AMD MSR disable CPD: MSR[%016lX] = 0x%016lX",
                                iMSR, ullAMD_MSR_PSTATECTL);
                    MtSupportDebugWriteLine(gBuffer);

                    ullAMD_MSR_PSTATECTL &= ~0x7ULL;
                    AsmWriteMsr64(iMSR, ullAMD_MSR_PSTATECTL);
                    AsciiSPrint(gBuffer, BUF_SIZE, "AMD Set PStateCtl: 0x%016lX", ullAMD_MSR_PSTATECTL);
                    MtSupportDebugWriteLine(gBuffer);

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// EnableAMDCPB
//
//  Enable AMD Core Performacne Boost (Turbo)
//
BOOLEAN EnableAMDCPB()
{
    // MSRC001_0015 Hardware Configuration Register (HWCR)
    // 25 CpbDis: core performance boost disable. Read-write. Specifies whether core performance boost is enabled or disabled. 0=CPB is enabled. 1=CPB is disabled.
    int iMSR = 0;
    UINT64 ullAMD_MSR_HWCR = 0ULL;
    UINT64 ullAMD_MSR_PSTATECTL = 0ULL;
    BOOLEAN bCPDEnabled = FALSE;

    // Disable AMD CPB
    iMSR = AMD_MSR_HWCR;
    ullAMD_MSR_HWCR = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "EnableAMDCPB - MSR[0x%08x] = %016lx", iMSR, ullAMD_MSR_HWCR);
    MtSupportDebugWriteLine(gBuffer);
    {
        // BOOLEAN bCPDDisabled = BitExtractULL(ullAMD_MSR_HWCR, 25, 25) != 0;
        // if (bCPDDisabled)
        {
            UINT64 ullValue = ullAMD_MSR_HWCR & ~(1ULL << 25); // Set MSRC001_0015[25:25] to 0 - enable CPB

            AsmWriteMsr64(iMSR, ullValue);
            ullAMD_MSR_HWCR = AsmReadMsr64(iMSR);
            AsciiSPrint(gBuffer, BUF_SIZE, "EnableAMDCPB - New MSR[0x%08x] = %016lx", iMSR, ullAMD_MSR_HWCR);
            MtSupportDebugWriteLine(gBuffer);
            {
                bCPDEnabled = BitExtractULL(ullAMD_MSR_HWCR, 25, 25) == 0;

                if (bCPDEnabled)
                {
                    // Set P-state to 0
                    iMSR = AMD_MSR_PSTATECTL;
                    ullAMD_MSR_PSTATECTL = AsmReadMsr64(iMSR);
                    AsciiSPrint(gBuffer, BUF_SIZE, "AMD MSR enable CPD: MSR[%016lX] = 0x%016lX",
                                iMSR, ullAMD_MSR_PSTATECTL);
                    MtSupportDebugWriteLine(gBuffer);

                    ullAMD_MSR_PSTATECTL &= ~0x7ULL;
                    AsmWriteMsr64(iMSR, ullAMD_MSR_PSTATECTL);
                    AsciiSPrint(gBuffer, BUF_SIZE, "AMD Set PStateCtl: 0x%016lX", ullAMD_MSR_PSTATECTL);
                    MtSupportDebugWriteLine(gBuffer);

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// ReEnableAMDCPB
//
//  Enable AMD Core Performacne Boost (Turbo)
//
BOOLEAN ReEnableAMDCPB(UINT64 ullAMD_MSR_HWCR)
{

    // MSRC001_0015 Hardware Configuration Register (HWCR)
    // 25 CpbDis: core performance boost disable. Read-write. Specifies whether core performance boost is enabled or disabled. 0=CPB is enabled. 1=CPB is disabled.
    // Enable Turbo
    AsmWriteMsr64(AMD_MSR_HWCR, ullAMD_MSR_HWCR);

    return FALSE;
}

DISABLE_OPTIMISATIONS() // Optimisation must be turned off
// Get_MPERF_APERF
//  Force a single CPU core to a high power state and get the current:
//  - Actual Performance Clock Counter - IA32_APERF
//  - Maximum Performance Clock Counter - IA32_MPERF
//  - Multiplier
//
BOOLEAN Get_MPERF_APERF(CPUINFO *pCPUInfo, int *flAPERF, int *flMPERF, int *flMult, int flCPUFreq_RDTSC, BOOLEAN bTurbo, int flTargetMult)
{
    BOOLEAN bMSR0xE8_supported = FALSE;
    UINT32 CPUInfo[4];
    int iMSR = 0;
    UINT64 ullMSRResult = 0;

    UINT64 count_freq; // High Resolution Performance Counter frequency
    UINT64 t0, t1;     // Variables for High-Resolution Performance Counter reads
    int tries = 0;     // Number of times a calculation has
    int i = 0, j = 0;
    UINT64 total_ticks = 0, ticks = 0; // Microseconds elapsed  during test
    UINT64 ullAPERFDelta, ullMPERFDelta;
    int flFreqAPERF = 0, flFreqMPERF = 0, fFreqMaxAPERF = 0, flFreqMaxMPERF = 0, flMultiplierMax = 0;
    int flCurrentMult = 0;
    int flMultiplier = 0;
    int iCPUDid = 0, iCPUFid = 0;
    UINT64 ullActivity = 0;

    // Check IA32_APERF (actual performnce counter) supported
    if (pCPUInfo->MaxBasicInputValue >= 6)
    {
        AsmCpuid(0x6, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

        AsciiSPrint(gBuffer, BUF_SIZE, "Get_MPERF_APERF - CPUID[0x6][ECX] = %08x", CPUInfo[2]);
        MtSupportDebugWriteLine(gBuffer);

        if (BitExtract(CPUInfo[2], 0, 0) == 1) // ECX[bit0] == 1, MSR 0xE8 is available //Hardware Coordination Feedback Capability (Presence of IA32_APERF, IA32_MPERF MSRs)
        {
            bMSR0xE8_supported = TRUE;
        }
    }
    else
    {
    }

    if (bMSR0xE8_supported) // RDTSC supported && //MSR 0xE8 supported (CPUID.06H:ECX[0] = 1
    {
        count_freq = cpu_speed * 1000; // GetPerformanceCounterProperties(NULL, NULL);

        AsciiSPrint(gBuffer, BUF_SIZE, "Get_MPERF_APERF - high perf counter freq = %ld", count_freq);
        MtSupportDebugWriteLine(gBuffer);

        // Force a higher power state //BIT601001
        j = 0;
        for (i = 0; i < 100000000; ++i)
            j += i;

        do
        {
            // This do loop runs up to 20 times or until the average of the previous  three calculated frequencies is within 1 MHz of each of the
            // individual calculated frequencies. This resampling increases the accuracy of the results since outside factors could affect this calculation
            tries++; // Increment number of times sampled on this call to cpuspeed

            // Don't do the 3rd measurement if it has already taken more than 15 seconds
            if (tries > 2 && total_ticks > 15000000) // SI101025
                break;

            // Force higher power state to measure max speed.
            // j = 0;
            // for (i = 0; i < 100000000; ++i)
            //	j +=i;
            // Force higher power state to measure max speed. SI101022c
            ullActivity = 400000000;
            if (flTargetMult > 0) // <km> Use current multiplier instead of APERF freq because it appears that the OS may reset the APERF/MPERF register values
            {
                if (flCurrentMult * 100 < (flTargetMult * 95))
                {
                    // if (tries == 2)
                    //	ullActivity = 100000000;		//SI101014 - reduce time measuring
                    // else
                    if (bTurbo && tries > 2) // Stick in more activity if trying to measure the Turbo speed as it is harder to get to the higest power state
                        // ullActivity = 1000000000;
                        // ullActivity = 500000000;		//SI101014 - reduce time measuring
                        ullActivity = MultU64x32(ullActivity, 4); // SI101014 - reduce time measuring
                    else if (tries > 1)
                        ullActivity = MultU64x32(ullActivity, 2);
                }

                if (flCurrentMult > flTargetMult) // We have reached our target multiplier, we can stop.
                    break;
            }
            else
            {
                if (fFreqMaxAPERF * 100 < (flCPUFreq_RDTSC * 75))
                {
                    if (bTurbo && tries > 2) // Stick in more activity if trying to measure the Turbo speed as it is harder to get to the higest power state
                    {
                        ullActivity = 3000000000;
                    }
                    else if (tries > 1)
                        ullActivity = 1000000000;
                }
                else if (fFreqMaxAPERF * 100 < (flCPUFreq_RDTSC * 95))
                {
                    ullActivity = 500000000;
                    if (bTurbo && tries > 2) // SI101008
                        ullActivity = 3000000000;
                }
            }
            // Force a higher power state //BIT601001
            j = 0;
            for (i = 0; i < ullActivity; ++i)
                j += i;

            // Clear Actual and max Performance Clock Counters - IA32_APERF and IA32_MPERF (Must reset both. see Volume 3B description)
            AsmWriteMsr64(0xE7, 0);
            AsmWriteMsr64(0xE8, 0);

            // Get current high res counter value
            t0 = AsmReadTsc(); // GetPerformanceCounter();
            t1 = t0;

            // Force higher power state to measure max speed.
            j = 0;
            for (i = 0; i < 500000; ++i) // Reduce to reduce likelyhood of being caught by the Operating System resetting this value //BIT6010230002
                j += i;

            t1 = AsmReadTsc(); // GetPerformanceCounter();

            // read Actual Performance Colock Counter - IA32_APERF
            ullAPERFDelta = AsmReadMsr64(0xE8);

            ullMPERFDelta = AsmReadMsr64(0xE7);

            if ((StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0 && pCPUInfo->Family >= 0x17) ||
                (StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0 && pCPUInfo->Family >= 0x18))
            {
                // Get the current multiplier	//moved to directly follow the CPU frequency measurement //SI101022b
                iMSR = AMD_MSR_HW_PSTATE_STATUS;
                ullMSRResult = AsmReadMsr64(iMSR);

                AsciiSPrint(gBuffer, BUF_SIZE, "MSR 0x%08X: %016lX", iMSR, ullMSRResult);
                MtSupportDebugWriteLine(gBuffer);

                flMultiplier = GetAMDMult(true, pCPUInfo, ullMSRResult, &iCPUDid, &iCPUFid); // SI101022c - don't use logging during CPU measurement

                iMSR = AMD_MSR_PSTATESTAT;
                ullMSRResult = AsmReadMsr64(iMSR);

                AsciiSPrint(gBuffer, BUF_SIZE, "MSR 0x%08X: %016lX", iMSR, ullMSRResult);
                MtSupportDebugWriteLine(gBuffer);

                iMSR = AMD_MSR_PSTATECTL;
                ullMSRResult = AsmReadMsr64(iMSR);

                AsciiSPrint(gBuffer, BUF_SIZE, "MSR 0x%08X: %016lX", iMSR, ullMSRResult);
                MtSupportDebugWriteLine(gBuffer);

                iMSR = AMD_MSR_PSTATECURLIM;
                ullMSRResult = AsmReadMsr64(iMSR);
                {
                    int iCurrentPState = (int)(BitExtractULL(ullMSRResult, 2, 0));
                    int iMaxPState = (int)(BitExtractULL(ullMSRResult, 6, 4)); // Highest index into PState (3 for Turbo core, else 4). for lowest multiplier

                    AsciiSPrint(gBuffer, BUF_SIZE, "MSR 0x%08X: %016lX (CurPState=%d, MaxPState=%d)", iMSR, ullMSRResult, iCurrentPState, iMaxPState);
                    MtSupportDebugWriteLine(gBuffer);
                }
            }
            else // COFVID_STATUS doesn't exist in AMD 17h
            {
                // Get the current multiplier	//moved to directly follow the CPU frequency measurement //SI101022b
                iMSR = AMD_MSR_COFVID_STATUS; // MSRC001_0071 COFVID Status
                ullMSRResult = AsmReadMsr64(iMSR);
                flMultiplier = GetAMDMult(FALSE, pCPUInfo, ullMSRResult, &iCPUDid, &iCPUFid); // SI101022c - don't use logging during CPU measurement

                AsciiSPrint(gBuffer, BUF_SIZE, "Get_MPERF_APERF - Multiplier = %d", flMultiplier);
                MtSupportDebugWriteLine(gBuffer);
            }
            // Calculate measurement time period difference
            ticks = t1 - t0;                                                             // Number of external ticks is difference between two hi-res counter reads.
            ticks = DivU64x64Remainder(MultU64x32(ticks, 1000000000), count_freq, NULL); // Ticks / ( ticks/ns ) = nanoseconds (ns)
            total_ticks += ticks;

            AsciiSPrint(gBuffer, BUF_SIZE, "Get_MPERF_APERF - time in ns = %ld", ticks);
            MtSupportDebugWriteLine(gBuffer);

            if (ticks > 0)
            {
                // Cycles * 1000000 / ns  = kHz
                flFreqMPERF = (int)DivU64x64Remainder(MultU64x32(ullMPERFDelta, 1000000), ticks, NULL);
                flFreqAPERF = (int)DivU64x64Remainder(MultU64x32(ullAPERFDelta, 1000000), ticks, NULL);

                AsciiSPrint(gBuffer, BUF_SIZE, "Get_MPERF_APERF - FreqMPERF in KHz = %d, FreqAPERF in KHz = %d", flFreqMPERF, flFreqAPERF);
                MtSupportDebugWriteLine(gBuffer);
            }

            // Get the multiplier and base bus speed as close to the CPU measurement

            // Store max measured freq (with sanity check
            if (flFreqAPERF > fFreqMaxAPERF && flFreqAPERF < CPUSPEEDMAX)
                fFreqMaxAPERF = flFreqAPERF;

            if (flFreqMPERF > flFreqMaxMPERF && flFreqMPERF < CPUSPEEDMAX)
                flFreqMaxMPERF = flFreqMPERF;

            if (flMultiplier > flMultiplierMax)
                flMultiplierMax = flMultiplier; // Store the multiplier for the max actual frequency - SI101022b
            flCurrentMult = flMultiplier;

        } while (tries < 3);

        /*
        //Get the current multiplier
        iMSR = 0xC0010071;
        if (DeviceIoControl(g_hDirectIO, IOCTL_DIRECTIO_READMSR, &iMSR, sizeof(iMSR), &ullMSRResult, sizeof(ullMSRResult), &dwBytesReturned, NULL))
        {
            if (IsDebugLog())
            {
                _stprintf_s(SysInfo_DebugString, _T("MSR 0x%0.8X: %0.16llX"), iMSR, ullMSRResult);
                DebugLog(SysInfo_DebugString);
            }

            flMultiplier = GetAMDMult(pCPUInfo->Family, ullMSRResult, &iCPUDid, &iCPUFid);
        }
        */
    }

    *flAPERF = fFreqMaxAPERF;
    *flMPERF = flFreqMaxMPERF;
    *flMult = flMultiplierMax;

    return TRUE;
}

ENABLE_OPTIMISATIONS() // Optimisation must be turned off

// GetAMD1ATemp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD1ATemp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily1Ah(pCPUInfo))
    {
        int iCurTmp = -1;

        iCurTmp = Get_AMD_1A_CurTmp(pCPUInfo->typestring);
        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = true;
            CPUTemp->iReadingValid = true;
            CPUTemp->iTemperature = iCurTmp;
            bRet = true;
        }
    }
    return bRet;
}

// GetAMD19Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD19Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily19h(pCPUInfo))
    {
        int iCurTmp = -1;

        iCurTmp = Get_AMD_19_CurTmp(pCPUInfo->typestring);
        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = true;
            CPUTemp->iReadingValid = true;
            CPUTemp->iTemperature = iCurTmp;
            bRet = true;
        }
    }
    return bRet;
}

// GetHygon18Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetHygon18Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsHygonFamily18h(pCPUInfo))
    {
        int iCurTmp = -1;

        iCurTmp = Get_AMD_17_CurTmp(pCPUInfo->typestring);
        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = true;
            CPUTemp->iReadingValid = true;
            CPUTemp->iTemperature = iCurTmp;
            bRet = true;
        }
    }
    return bRet;
}

// GetAMD17Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD17Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily17h(pCPUInfo))
    {
        int iCurTmp = -1;

        iCurTmp = Get_AMD_17_CurTmp(pCPUInfo->typestring);
        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = true;
            CPUTemp->iReadingValid = true;
            CPUTemp->iTemperature = iCurTmp;
            bRet = true;
        }
    }
    return bRet;
}

// GetAMD16Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD16Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily16h(pCPUInfo))
    {
        int iCurTmp = Get_AMD_16_CurTmp();
        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = true;
            CPUTemp->iReadingValid = true;
            CPUTemp->iTemperature = iCurTmp;
            bRet = true;
        }
    }
    return bRet;
}

// GetAMD15Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD15Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp) // SI101021 - added temp for AMD Bulldozer
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily15h(pCPUInfo))
    {
        int iCurTmp = Get_AMD_15_CurTmp(pCPUInfo->Model);

        SysInfo_DebugLogF(L"GetAMD15Temp - Temperature: %d", iCurTmp);

        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = TRUE;
            CPUTemp->iReadingValid = TRUE;
            CPUTemp->iTemperature = iCurTmp;
            bRet = TRUE;
        }
    }
    return bRet;
}

// GetAMD14Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD14Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp) // SI101021 - added temp for AMD Bobcat
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily14h(pCPUInfo))
    {
        int iCurTmp = Get_AMD_12_14_CurTmp(); // Same as 12h series

        SysInfo_DebugLogF(L"GetAMD14Temp - Temperature: %d", iCurTmp);

        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = TRUE;
            CPUTemp->iReadingValid = TRUE;
            CPUTemp->iTemperature = iCurTmp;
            bRet = TRUE;
        }
    }
    return bRet;
}

// GetAMD12Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD12Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily12h(pCPUInfo))
    {
        int iCurTmp = Get_AMD_12_14_CurTmp();

        SysInfo_DebugLogF(L"GetAMD12Temp - Temperature: %d", iCurTmp);

        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = TRUE;
            CPUTemp->iReadingValid = TRUE;
            CPUTemp->iTemperature = iCurTmp;
            bRet = TRUE;
        }
    }
    return bRet;
}

// GetAMD11Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD11Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily11h(pCPUInfo))
    {
        int iCurTmp = Get_AMD_11_CurTmp() / 8;

        SysInfo_DebugLogF(L"GetAMD11Temp - Temperature: %d", iCurTmp);

        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = TRUE;
            CPUTemp->iReadingValid = TRUE;
            CPUTemp->iTemperature = iCurTmp;
            bRet = TRUE;
        }
    }
    return bRet;
}

// GetAMD10Temp
//
//  CPU Temp = Value / 8
//  where Value = F3xA4[31:21] CurTmp: current temperature (with slew rates applied). But still relative to another value.
//
//  PCI-defined configuration space:
//  byte address of the configuration register: 0xA4
//  function number: 3
//
BOOLEAN GetAMD10Temp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (IsAMDFamily10h(pCPUInfo))
    {
        int iCurTmp = Get_AMD_10_CurTmp() / 8;

        SysInfo_DebugLogF(L"GetAMD10Temp - Temperature: %d", iCurTmp);

        // if (iCurTmp > 0 && iCurTmp < 200)	//sanity
        if (iCurTmp > MIN_TEMP_SANITY && iCurTmp < 200) // sanity
        {
            CPUTemp->bDataCollected = TRUE;
            CPUTemp->iReadingValid = TRUE;
            CPUTemp->iTemperature = iCurTmp;
            bRet = TRUE;
        }
    }
    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 1Ah Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily1Ah
BOOLEAN IsAMDFamily1Ah(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x1A) // MSR's for: FamilyName_DisplayModel == 1A_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 19h Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily19h
BOOLEAN IsAMDFamily19h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x19) // MSR's for: FamilyName_DisplayModel == 19_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Hygon Family 18h Processor MSRs (Same architecture as AMD 17h Ref: https://lkml.org/lkml/2018/6/9/115)
///////////////////////////////////////////////////////////////////////////////////////////

// IsHygonFamily18h
BOOLEAN IsHygonFamily18h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"HygonGenuine", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x18) // MSR's for: FamilyName_DisplayModel == 18_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 17h Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily17h
BOOLEAN IsAMDFamily17h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x17) // MSR's for: FamilyName_DisplayModel == 17_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 16h Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily16h
BOOLEAN IsAMDFamily16h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x16) // MSR's for: FamilyName_DisplayModel == 16_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 15h Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily15h
BOOLEAN IsAMDFamily15h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x15) // MSR's for: FamilyName_DisplayModel == 15_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 14h Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily14h
BOOLEAN IsAMDFamily14h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x14) // MSR's for: FamilyName_DisplayModel == 14_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 12h Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily12h
BOOLEAN IsAMDFamily12h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x12) // MSR's for: FamilyName_DisplayModel == 12_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 11h Processor MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily11h
BOOLEAN IsAMDFamily11h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x11) // MSR's for: FamilyName_DisplayModel == 11_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD Family 10h Processor MSRs (K10 Phenom, Opteron)
///////////////////////////////////////////////////////////////////////////////////////////

// IsAMDFamily10h
BOOLEAN IsAMDFamily10h(CPUINFO *pCPUInfo)
{
    if (_tcscmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x10) // MSR's for: FamilyName_DisplayModel == 10_??H
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD K10 functions
///////////////////////////////////////////////////////////////////////////////////////////

/*
//Get_AMD_10_NdFid
//
// F3xD4[4:0]
int Get_AMD_10_NdFid()
{
    DWORD		Bus, Dev, Fun;
    DWORD		Data;

    Bus = 0;
    for (Dev = 24; Dev <= 31; Dev++)		//Processor configuration space Bus 0, dev 24..31
    {
        Fun = 3;
        Data = Get_PCI_Reg(Bus, Dev, Fun, 0xD4);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return ((int) (BitExtract(Data, 4, 0) ));
    }

    return 0;
}
*/

// Get_AMD_10_CurTmp
//
//  F3xA4[31:21]
int Get_AMD_10_CurTmp()
{
    DWORD Bus, Dev, Fun;
    DWORD Data;

    Bus = 0;
    for (Dev = 24; Dev <= 31; Dev++) // Processor configuration space Bus 0, dev 24..31
    {
        Fun = 3;
        Data = Get_PCI_Reg(Bus, Dev, Fun, 0xA4);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return ((int)(BitExtract(Data, 31, 21)));
    }

    return INT_NOVALUEAVAILABLE; // SI101065
}

int Get_AMD_10_LnkFreq_Value(DWORD Data)
{
    int iLinkFreq = 0;

    switch (Data)
    {
    case 0x0:
        iLinkFreq = 200;
        break;
    case 0x2:
        iLinkFreq = 400;
        break;
    case 0x4:
        iLinkFreq = 600;
        break;
    case 0x5:
        iLinkFreq = 800;
        break;
    case 0x6:
        iLinkFreq = 1000;
        break;
    case 0x7:
        iLinkFreq = 1200;
        break;
    case 0x8:
        iLinkFreq = 1400;
        break;
    case 0x9:
        iLinkFreq = 1600;
        break;
    case 0xA:
        iLinkFreq = 1800;
        break;
    case 0xB:
        iLinkFreq = 2000;
        break;
    case 0xC:
        iLinkFreq = 2200;
        break;
    case 0xD:
        iLinkFreq = 2400;
        break;
    case 0xE:
        iLinkFreq = 2600;
        break;
    }

    return iLinkFreq;
}

// Get_AMD_10_LnkFreq
//
//  F0x[E8,C8,A8,88] Link Frequency/Revision Registers
int Get_AMD_10_LnkFreq()
{
    DWORD Bus, Dev, Fun;
    DWORD Data;

    // F0x[E8,C8,A8,88] Link Frequency/Revision Registers
    Bus = 0;
    for (Dev = 24; Dev <= 31; Dev++) // Processor configuration space Bus 0, dev 24..31
    {
        Fun = 0;
        Data = Get_PCI_Reg(Bus, Dev, Fun, 0x88);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return Get_AMD_10_LnkFreq_Value(BitExtract(Data, 11, 8));

        Data = Get_PCI_Reg(Bus, Dev, Fun, 0xA8);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return Get_AMD_10_LnkFreq_Value(BitExtract(Data, 11, 8));

        Data = Get_PCI_Reg(Bus, Dev, Fun, 0xC8);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return Get_AMD_10_LnkFreq_Value(BitExtract(Data, 11, 8));

        Data = Get_PCI_Reg(Bus, Dev, Fun, 0xE8);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return Get_AMD_10_LnkFreq_Value(BitExtract(Data, 11, 8));
    }

    return 0;
}

bool Get_AMD_10_HTCState()
{
    // Check if HTC Capable
    DWORD data = Get_PCI_Reg(0, 0x18, 3, 0xE8);
    bool bHTCCapable = (int)BitExtract(data, 10, 10) != 0;

    if (bHTCCapable)
    {
        /*
            While in the HTC-active state, the following power reduction actions are taken:
                . CPU cores are limited to a P-state (specified by D18F3x64[HtcPstateLimit]); see 2.5.3 [CPU Power
                Management]
                . The GPU may be placed in a low power state based on the state of D18F5x178[ProcHotToGnbEn]
            The processor enters the HTC-active state if all of the following conditions are true:
                . D18F3xE8[HtcCapable]=1
                . D18F3x64[HtcEn]=1
                . PWROK=1
                . THERMTRIP_L=1
            and any of the following conditions are true:
                . Tctl is greater than or equal to the HTC temperature limit (D18F3x64[HtcTmpLmt]).
                . PROCHOT_L=0
            The processor exits the HTC-active state when all of the following are true:
                . Tctl is less than the HTC temperature limit (D18F3x64[HtcTmpLmt]).
                . Tctl has become less than the HTC temperature limit (D18F3x64[HtcTmpLmt]) minus the HTC hysteresis limit
                (D18F3x64[HtcHystLmt]) since being greater than or equal to the HTC temperature limit (D18F3x64[HtcTmpLmt]).
                . PROCHOT_L=1.
        */
        // Get Hardware Thermal Control (HTC)
        DWORD dwMSRResult = Get_PCI_Reg(0, 0x18, 3, 0x64);

        SysInfo_DebugLogF(_T("D18F3x64: %0.16llX"), dwMSRResult);

        return BitExtract(dwMSRResult, 4, 4) != 0 ? true : false;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD K11 functions
///////////////////////////////////////////////////////////////////////////////////////////

// Get_AMD_11_CurTmp
//
//  F3xA4[31:21]
int Get_AMD_11_CurTmp()
{
    DWORD Bus, Dev, Fun;
    DWORD Data;

    Bus = 0;
    for (Dev = 24; Dev <= 31; Dev++) // Processor configuration space Bus 0, dev 24..31
    {
        Fun = 3;
        Data = Get_PCI_Reg(Bus, Dev, Fun, 0xA4);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return ((int)(BitExtract(Data, 31, 21)));
    }

    return INT_NOVALUEAVAILABLE; // SI101065
}

int Get_AMD_11_LnkFreq_Value(DWORD Data)
{
    int iLinkFreq = 0;

    switch (Data)
    {
    case 0x0:
        iLinkFreq = 200;
        break;
    case 0x2:
        iLinkFreq = 400;
        break;
    case 0x4:
        iLinkFreq = 600;
        break;
    case 0x5:
        iLinkFreq = 800;
        break;
    case 0x6:
        iLinkFreq = 1000;
        break;
    case 0x7:
        iLinkFreq = 1200;
        break;
    case 0x8:
        iLinkFreq = 1400;
        break;
    case 0x9:
        iLinkFreq = 1600;
        break;
    case 0xA:
        iLinkFreq = 1800;
        break;
    case 0xB:
        iLinkFreq = 2000;
        break;
    case 0xC:
        iLinkFreq = 2200;
        break;
    case 0xD:
        iLinkFreq = 2400;
        break;
    case 0xE:
        iLinkFreq = 2600;
        break;
    }

    return iLinkFreq;
}

// Get_AMD_11_LnkFreq
//
//  F0x88 Link Frequency/Revision Registers (Note: Unlike K10, only register 0x88 used)
int Get_AMD_11_LnkFreq()
{
    DWORD Bus, Dev, Fun;
    DWORD Data;

    Bus = 0;
    for (Dev = 24; Dev <= 31; Dev++) // Processor configuration space Bus 0, dev 24..31
    {
        Fun = 0;
        Data = Get_PCI_Reg(Bus, Dev, Fun, 0x88);
        if ((Data != 0xFFFFFFFF) && (Data != 0))
            return Get_AMD_11_LnkFreq_Value(BitExtract(Data, 11, 8));
    }

    return 0;
}

#ifdef WIN32
bool Get_AMD_MmioCfgBaseAddr(UINT64 *pBaseAddr)
{
    DWORD dwBytesReturned = 0;
    int iMSR = 0xC0010058; // MMIO Configuration Base Address
    ULONGLONG ullMSRResult = 0;
    if (DeviceIoControl(g_hDirectIO, IOCTL_DIRECTIO_READMSR, &iMSR, sizeof(iMSR), &ullMSRResult, sizeof(ullMSRResult), &dwBytesReturned, NULL))
    {
        BOOL MMIOCfgEnable = (BOOL)BitExtractULL(ullMSRResult, 0, 0); // Enable.
        UINT32 BusRange = (UINT32)BitExtractULL(ullMSRResult, 5, 2);  // BusRange: bus range identifier
        UINT64 MMIOCfgBaseAddr = ullMSRResult & 0xFFFFFFF00000ULL;    // MmioCfgBaseAddr[47:20]

        SysInfo_DebugLogF(L"MSR[%x] = %016llx (Enable=%d, BusRange=%x, BaseAddr=%llx)", iMSR, ullMSRResult, MMIOCfgEnable, BusRange, MMIOCfgBaseAddr);

        if (MMIOCfgEnable == FALSE)
        {
            tagWriteMSR WriteMSR;
            ULONGLONG ullValue = 0ULL;
            DWORD dwBytesReturned = 0;

            SysInfo_DebugLogF(L"Enabling MMIO config space");

            // MSRC001_0015 Hardware Configuration Register (HWCR)
            // 25 CpbDis: core performance boost disable. Read-write. Specifies whether core performance boost is enabled or disabled. 0=CPB is enabled. 1=CPB is disabled.
            // Enable Turbo
            WriteMSR.dwMSR = iMSR;
            WriteMSR.ullValue = ullMSRResult | 1;
            if (DeviceIoControl(g_hDirectIO, IOCTL_DIRECTIO_WRITEMSR, &WriteMSR, sizeof(WriteMSR), &ullValue, sizeof(ullValue), &dwBytesReturned, NULL))
            {
                SysInfo_DebugLogF(L"Enabling MMIO config space OK: %08lX, 0x%016llX, 0x%016llX", WriteMSR.dwMSR, WriteMSR.ullValue, ullValue);
            }
        }

        *pBaseAddr = MMIOCfgBaseAddr;
        return true;
    }
    return false;
}
#endif

// Get_AMD_12_PCI_NumberBoostedStates
//
//  D18F4x15C Core Performance Boost Control
//	D18F4x15C[4:2]: NumBoostStates: number of boosted states
//
// int Get_AMD_PCI_bits(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD dwIndexHigh, DWORD dwIndexLow)
bool Get_AMD_PCI_bits(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD dwIndexHigh, DWORD dwIndexLow, int *piReturn) // SI101022c - return success or failure
{
    int iResult = 0;
    DWORD Data = 0;

    if (Reg <= 0xFF)
        Data = Get_PCI_Reg(Bus, Dev, Fun, Reg);
    else
    {
#ifdef WIN32
        DWORD dwBytesReturned = 0;
        UINT64 MMIOCfgBaseAddr;
        if (Get_AMD_MmioCfgBaseAddr(&MMIOCfgBaseAddr))
        {
            UINT64 MMIOCfgAddr = MMIOCfgBaseAddr + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000) + Reg;

            // Ask driver to map MMIO base address to pointer we can use
            BYTE *pbLinAddr = NULL;
            tagPhys32Struct Phys32Struct = {0};
            // HANDLE		hPhysicalMemory	= NULL;
            pbLinAddr = MapPhysToLin((PBYTE)MMIOCfgAddr, 4, &Phys32Struct);
            if (pbLinAddr)
            {
                Data = *((DWORD *)pbLinAddr);

                UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
            }
        }
#else
        UINT32 MSR = 0xC0010058; // MMIO Configuration Base Address
        UINT64 MSRValue = AsmReadMsr64(MSR);
        BOOL MMIOCfgEnable = BitExtract(MSRValue, 0, 0);       // Enable.
        UINT32 BusRange = BitExtract(MSRValue, 5, 2);          // BusRange: bus range identifier
        UINT64 MMIOCfgBaseAddr = MSRValue & 0xFFFFFFF00000ULL; // MmioCfgBaseAddr[47:20]
        UINT64 MMIOCfgAddr;

        SysInfo_DebugLogF(L"MSR[%x] = %016lx (Enable=%d, BusRange=%x, BaseAddr=%llx)", MSR, MSRValue, MMIOCfgEnable, BusRange, MMIOCfgBaseAddr);

        if (MMIOCfgEnable == FALSE)
        {
            SysInfo_DebugLogF(L"Enabling MMIO config space");
            AsmWriteMsr64(MSR, MSRValue | 1);
        }

        MMIOCfgAddr = MMIOCfgBaseAddr + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000) + Reg;
        Data = MmioRead32((UINTN)MMIOCfgAddr);
#endif
    }
    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
        iResult = (int)(BitExtract(Data, dwIndexHigh, dwIndexLow));
        SysInfo_DebugLogF(L"Get PCI 0x%x, 0x%x, 0x%x, 0x%x [%u:%u]: %d (%u)", Bus, Dev, Fun, Reg, dwIndexHigh, dwIndexLow, iResult, Data);
        *piReturn = iResult;
        return true;
    }

    SysInfo_DebugLogF(L"Get PCI failed 0x%x, 0x%x, 0x%x, 0x%x [%u:%u]: %d (%u)", Bus, Dev, Fun, Reg, dwIndexHigh, dwIndexLow, iResult, Data);

    *piReturn = 0;
    return false;
}

bool Set_AMD_PCIEx_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD dwValue)
{
#ifdef WIN32
    return DirectIo_WritePCIReg(Bus, Dev, Fun, Reg, dwValue) == ERROR_SUCCESS;
#else
    Set_PCI_Reg(Bus, Dev, Fun, Reg, dwValue);
    return true;
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
// AMD K12 functions
///////////////////////////////////////////////////////////////////////////////////////////

// Get_AMD_12_PCI_NumberBoostedStates
//
//  D18F4x15C Core Performance Boost Control
//	D18F4x15C[4:2]: NumBoostStates: number of boosted states
//
int Get_AMD_12_PCI_NumberBoostedStates()
{
    DWORD Bus, Dev, Fun;
    DWORD Data;

    Bus = 0;
    Dev = 0x18; // Device 18h Configuration Registers
    Fun = 4;
    Data = Get_PCI_Reg(Bus, Dev, Fun, 0x15);
    if ((Data != 0xFFFFFFFF) && (Data != 0))
        return ((int)(BitExtract(Data, 4, 2)));

    return 0;
}

// Get_AMD_12_14_CurTmp
//
//  D18F3xA4 Reported Temperature Control Register
//	D18F3xA4[31:21]
//
int Get_AMD_12_14_CurTmp()
{
    DWORD Bus, Dev, Fun;
    DWORD Data;

    Bus = 0;
    Dev = 0x18; // Device 18h Configuration Registers
    Fun = 3;
    Data = Get_PCI_Reg(Bus, Dev, Fun, 0xA4);
    SysInfo_DebugLogF(_T("DEBUG: Get_AMD_12_14_CurTmp raw:%u\n"), Data);
    if ((Data != 0xFFFFFFFF) && (Data != 0))
        return (((int)(BitExtract(Data, 31, 21))) / 8);

    return INT_NOVALUEAVAILABLE; // SI101065
}

static int g_Get_AMD_15_CurTmp_debug = 0;

// Get_AMD_15_CurTmp
//
//  D18F3xA4 Reported Temperature Control Register
//	D18F3xA4[31:21]
//
#define SMU_INDEX_0 0xB8 // PCI register offset
#define SMU_DATA_0 0xBC  // PCI register offset

#define SMU_HTC 0xD8200C64      // SMU register offset
#define SMU_CUR_TEMP 0xD8200CA4 // SMU register offset

int Get_AMD_15_CurTmp(int iModel)
{
    DWORD Bus, Dev, Fun, Reg;
    DWORD Data;

    if (iModel <= 0x3F)
    {
        Bus = 0;
        Dev = 0x18; // Device 18h Configuration Registers
        Fun = 3;
        Reg = 0xA4;

        Data = Get_PCI_Reg(Bus, Dev, Fun, Reg);
    }
    else
    {
        DWORD Index;

        Bus = 0;
        Dev = 0;
        Fun = 0;

        // Read the current value of SMU_INDEX_0/SMU_DATA_0
        Index = Get_PCI_Reg(Bus, Dev, Fun, SMU_INDEX_0);
        Data = Get_PCI_Reg(Bus, Dev, Fun, SMU_DATA_0);

        SysInfo_DebugLogF(_T("Get_AMD_15_CurTmp: SMU_INDEX_0=0x%08X SMU_DATA_0=0x%08X"), Index, Data);

        // Set SMU_INDEX_0 to D820_0CA4 for Reported Temperature Control
        Set_PCI_Reg(Bus, Dev, Fun, SMU_INDEX_0, SMU_CUR_TEMP);

        SysInfo_DebugLogF(_T("Get_AMD_15_CurTmp: Setting SMU_INDEX_0 to 0x%08X"), SMU_CUR_TEMP);

        Sleep(1);

        // Read SMU_DATA_0 to read SMU thermal control register
        Data = Get_PCI_Reg(Bus, Dev, Fun, SMU_DATA_0);

        SysInfo_DebugLogF(_T("Get_AMD_15_CurTmp: SMU_DATA_0=0x%08X"), Data);
    }

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
        int iCurTmpTjSel = (int)(BitExtract(Data, 17, 16));
        int iCurTmpRaw = (int)(BitExtract(Data, 31, 21));

        if (g_Get_AMD_15_CurTmp_debug++ < 3) // SI101055
        {
            SysInfo_DebugLogF(_T("AMD Reported Temperature Control: 0x%X, %d, %d"), Data, iCurTmpTjSel, iCurTmpRaw);
        }

        if (iCurTmpTjSel != 3 /*11b*/)
        {
            return iCurTmpRaw >> 3;
        }
        else
        {
            // return (iCurTmpRaw >> 1)-49;		//SI101036
            if (iModel >= 0x00 && iModel <= 0x0F)
                return ((int)(BitExtract(iCurTmpRaw, 10, 2)) >> 1) - 49; // SI101055
            else if (iModel >= 0x10 && iModel <= 0x1F)
                return (iCurTmpRaw >> 3) - 49; // SI101055
            else if (iModel >= 0x30 && iModel <= 0x3F)
                return (iCurTmpRaw >> 3) - 49; // SI101055
            else if (iModel >= 0x60 && iModel <= 0x6F)
                return (iCurTmpRaw >> 3) - 49;
            else if (iModel >= 0x70 && iModel <= 0x7F)
                return (iCurTmpRaw >> 3) - 49;
            else
                return (iCurTmpRaw >> 1) - 49; // SI101055
        }
    }

    return INT_NOVALUEAVAILABLE; // SI101065
}

bool Get_AMD_15_HTCState(int iModel)
{
    if (iModel <= 0x3f)
    {
        // Check if HTC Capable
        DWORD data = Get_PCI_Reg(0, 0x18, 3, 0xE8);
        bool bHTCCapable = (int)BitExtract(data, 10, 10) != 0;

        if (bHTCCapable)
        {
            /*
                While in the HTC-active state, the following power reduction actions are taken:
                    . CPU cores are limited to a P-state (specified by D18F3x64[HtcPstateLimit]); see 2.5.3 [CPU Power
                    Management]
                    . The GPU may be placed in a low power state based on the state of D18F5x178[ProcHotToGnbEn]
                The processor enters the HTC-active state if all of the following conditions are true:
                    . D18F3xE8[HtcCapable]=1
                    . D18F3x64[HtcEn]=1
                    . PWROK=1
                    . THERMTRIP_L=1
                and any of the following conditions are true:
                    . Tctl is greater than or equal to the HTC temperature limit (D18F3x64[HtcTmpLmt]).
                    . PROCHOT_L=0
                The processor exits the HTC-active state when all of the following are true:
                    . Tctl is less than the HTC temperature limit (D18F3x64[HtcTmpLmt]).
                    . Tctl has become less than the HTC temperature limit (D18F3x64[HtcTmpLmt]) minus the HTC hysteresis limit
                    (D18F3x64[HtcHystLmt]) since being greater than or equal to the HTC temperature limit (D18F3x64[HtcTmpLmt]).
                    . PROCHOT_L=1.
            */
            // Get Hardware Thermal Control (HTC)
            DWORD dwMSRResult = Get_PCI_Reg(0, 0x18, 3, 0x64);

            SysInfo_DebugLogF(_T("D18F3x64: %0.16llX"), dwMSRResult);

            return BitExtract(dwMSRResult, 4, 4) != 0 ? true : false;
        }
        else
        {
        }
    }
    else
    {
        DWORD Bus, Dev, Fun;
        DWORD Data;
        DWORD Index;

        Bus = 0;
        Dev = 0;
        Fun = 0;

        // Read the current value of SMU_INDEX_0/SMU_DATA_0
        Index = Get_PCI_Reg(Bus, Dev, Fun, SMU_INDEX_0);
        Data = Get_PCI_Reg(Bus, Dev, Fun, SMU_DATA_0);

        SysInfo_DebugLogF(_T("Get_AMD_15_HTCState: SMU_INDEX_0=0x%08X SMU_DATA_0=0x%08X"), Index, Data);

        Set_PCI_Reg(Bus, Dev, Fun, SMU_INDEX_0, SMU_HTC);

        SysInfo_DebugLogF(_T("Get_AMD_15_HTCState: Setting SMU_INDEX_0 to 0x%08X"), SMU_HTC);

        Sleep(1);

        Data = Get_PCI_Reg(Bus, Dev, Fun, SMU_DATA_0);

        SysInfo_DebugLogF(_T("Get_AMD_15_CurTmp: SMU_DATA_0=0x%08X"), Data);

        if ((Data != 0xFFFFFFFF) && (Data != 0))
        {
            BOOL bHTCEn = BitExtract(Data, 0, 0); // HTC enable. Read-only; Updated-by-SMU. Reset: 0. 1=HTC is enabled; the processor is capable of entering the HTC - active state.
            if (bHTCEn)
            {
                return BitExtract(Data, 4, 4) != 0 ? true : false; // HTC_ACTIVE: HTC-active state. Read-only; Updated-by-hardware. Reset: X. 1=The processor is currently in the HTC - active state. 0 = The processor is not in the HTC - active state.
            }
        }
    }
    return false;
}

// Get_AMD_16_CurTmp
//
//  D18F3xA4 Reported Temperature Control Register
//	D18F3xA4[31:21]
//
int Get_AMD_16_CurTmp() // SI101036
{
    DWORD Bus, Dev, Fun;
    DWORD Data;

    Bus = 0;
    Dev = 0x18; // Device 18h Configuration Registers
    Fun = 3;
    Data = Get_PCI_Reg(Bus, Dev, Fun, 0xA4);
    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
        int iCurTmpTjSel = (int)(BitExtract(Data, 17, 16));
        int iCurTmpRaw = ((int)(BitExtract(Data, 31, 21)));

        if (iCurTmpTjSel != 3 /*11b*/)
            return iCurTmpRaw >> 3;
        else
            return (iCurTmpRaw >> 3) - 49; // Spec look maybe wrong - but this is waht it says. Note: different to 15h family which is *0.5
    }

    return INT_NOVALUEAVAILABLE; // SI10
}

// Get_AMD_17_CurTmp
//
//   Reference: Preliminary Processor Programming Reference(PPR) for AMD Family 17h Model 01h, Revision B1 Processors (54945 Rev 1.13)
//
//   SMU thermal register (Offset 0005_9800h) accessed via SMN_INDEX/SMN_DATA
//   1) Set PCI register D00F0x060 (NB_SMN_INDEX_0) to 0005_9800h (ie. Read SMU thermal register at offset 0005_9800h)
//   2) Read PCI register D00F0x064 (NB_SMN_DATA_0) to read SMU thermal register THM_TCON_CUR_TMP
//
//
#define SMUTHM_BASE 0x00059800
#define THM_TCON_CUR_TMP (SMUTHM_BASE + 0) // SMU register offset // FAMILY_17H_M01H_THM_TCON_TEMP
#define THM_TCON_HTC (SMUTHM_BASE + 4)
#define SMUSBI_SBIREGADDR (SMUTHM_BASE + 0x300)
#define SMUSBI_SBIREGDATA (SMUTHM_BASE + 0x304)
#define SMUSBI_ERRATA_STAT_REG (SMUTHM_BASE + 0x314)

#define FAMILY_17H_M01H_THM_TCON_TEMP_RANGE_SEL 0x80000 // From AMD17CPU.cs in hardware monitor http://holly/cerb5/index.php/display/TPZ-25384-965

#define NB_SMN_INDEX_0 0x60 // PCI register offset
#define NB_SMN_DATA_0 0x64  // PCI register offset

#define SBTSI_CPUTEMPINT 0x01
#define SBTSI_STATUS 0x02
#define SBTSI_CONFIG 0x03
#define SBTSI_UPDATERATE 0x04
#define SBTSI_HITEMPINT 0x07
#define SBTSI_LOTEMPINT 0x08
#define SBTSI_CONFIGWR 0x09
#define SBTSI_CPUTEMPDEC 0x10
#define SBTSI_CPUTEMPOFFINT 0x11
#define SBTSI_CPUTEMPOFFDEC 0x12
#define SBTSI_HITEMPDEC 0x13
#define SBTSI_LOTEMPDEC 0x14
#define SBTSI_TIMEOUTCONFIG 0x22
#define SBTSI_ALERTTHRESHOLD 0x32
#define SBTSI_ALERTCONFIG 0xBF
#define SBTSI_MANID 0xFE
#define SBTSI_REVISION 0xFF
#if 0
#define IOHC_MISC_BASE 0x13B10000
#define SMU_BASE_ADDR_LO 0x000002E8
#define SMU_BASE_ADDR_HI 0x000002EC
#endif

#if 0 // See AMD Family 19H PPR: SB Temperature Sensor Interface (SB-TSI)
DISABLE_OPTIMISATIONS() //Optimisation must be turned off 
static DWORD smn_read(DWORD node, DWORD reg)
{
  // Set SMN_INDEX_0 to the register to read
  Set_PCI_Reg(node * 0x20, 0, 0, NB_SMN_INDEX_0, reg);

  Sleep(10);

  // Read NB_SMN_DATA_0 to read the register value
  return Get_PCI_Reg(node * 0x20, 0, 0, NB_SMN_DATA_0);
}

static void smn_write(DWORD node, DWORD reg, DWORD value)
{
  // Set SMN_INDEX_0 to the register to read
  Set_PCI_Reg(node * 0x20, 0, 0, NB_SMN_INDEX_0, reg);

  Sleep(10);

  // Write NB_SMN_DATA_0 to write the register value
  Set_PCI_Reg(node * 0x20, 0, 0, NB_SMN_DATA_0, value);

}
ENABLE_OPTIMISATIONS()
#endif

int Get_AMD_17_CurTmp(wchar_t *szTypeString) // SI101036
{

    DWORD Bus, Dev, Fun;
    DWORD Index;
    DWORD Data;

    Bus = 0;
    Dev = 0;
    Fun = 0;

    bool usingBlanketOffset = false; // Set to tru if using the -49 based on setting

    // Read the current value of SMN_INDEX_0/SMN_DATA_0
    Index = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0);
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: %s - NB_SMN_INDEX_0=0x%08X NB_SMN_DATA_0=0x%08X"), szTypeString, Index, Data);

    SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: Setting NB_SMN_INDEX_0 to 0x%08X"), THM_TCON_CUR_TMP);

    // Set SMN_INDEX_0 to THM_TCON_CUR_TMP to read SMU thermal register
    Set_PCI_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0, THM_TCON_CUR_TMP);

    SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: NB_SMN_INDEX_0 was set to 0x%08X"), THM_TCON_CUR_TMP);

    // This sleep didn't seem to be enough when system was under heavy load
    Sleep(10);

    // Read NB_SMN_DATA_0 to read SMU thermal register THM_TCON_CUR_TMP
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: NB_SMN_DATA_0=0x%08X"), Data);

    // If value read was 0 try again as we see under heavy load temperatures being read as 0 a lot on AMD chips
    // Also when another tmeporature reading application (liek Ryzen master) was running
    // sleep briefly then attempt another read
    if (Data == 1)
    {
        // Try a much longer sleep
        Sleep(50);

        Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);
        SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: Second read : NB_SMN_DATA_0=0x%08X"), Data);
    }

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
        // int iCurTmpTjSel = (int)(BitExtract(Data, 17, 16));
        // int iCurTmpRaw = ((int)(BitExtract(Data, 31, 21)));
        int iTrueTmp = 0;
        int temperature = ((Data >> 21) & 0x7FF) >> 3;
        iTrueTmp = (int)temperature;

        SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: 0x%X, %f, %d"), Data, temperature, iTrueTmp);

        if ((Data & FAMILY_17H_M01H_THM_TCON_TEMP_RANGE_SEL) != 0)
        {
            iTrueTmp -= 49;
            usingBlanketOffset = true; // Use this to stop doing a double - (eg for 2600)
            SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: usingBlanketOffset %d"), iTrueTmp);
        }

        // iTrueTmp = iCurTmpRaw >> 3;

        // According to  http://holly/cerb/index.php/profiles/ticket/TPZ-25384-965/conversation
        //  However this might break all the other -49s?
        /*
        if ((Data & FAMILY_17H_M01H_THM_TCON_TEMP_RANGE_SEL) != 0)
        {
          if (iCurTmpTjSel == 3) //Otherwise we've already done this
          {
            if (IsDebugLog())
            {
              SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: FAMILY_17H_M01H_THM_TCON_TEMP_RANGE_SEL flag set old temp: %d new temp: %d"), iTrueTmp, iTrueTmp - 49);
              DebugLog(SysInfo_DebugString);
            }

            iTrueTmp -= 49;

          }
          else
          {
            if (IsDebugLog())
            {
              SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: FAMILY_17H_M01H_THM_TCON_TEMP_RANGE_SEL flag set old temp: %d new temp: %d"), iTrueTmp, iTrueTmp);
              DebugLog(SysInfo_DebugString);
            }
          }
        }
        */

        // For Ryzen 1600X, 1700X, 1800X, there is a -20C offset
        // Reference: https://community.amd.com/community/gaming/blog/2017/03/13/amd-ryzen-community-update
        /*
        Greetings. It turns out that different Threadripper OPNs have two
        temperature offsets, and those offsets are different from Summit's offsets.
        This is the mapping list, with spaces removed from within the brand string
        to simplify the matchup.

        The specified value is the offset in C from the Tctl reading.  For example,
        if the TR 1950X Tctl reads 100C, then the offset of -27 is applied for a
        display of 73C.

        The CPU brand strings and associated values are:
        AMDRyzen71800XEight-CoreProcessor -20
        AMDRyzen71700XEight-CoreProcessor -20
        AMDRyzen51600XSix-CoreProcessor -20
        AMDRyzenThreadripper1950X16-CoreProcessor -27
        AMDRyzenThreadripper195016-CoreProcessor -10
        AMDRyzenThreadripper192012-CoreProcessor -10
        AMDRyzenThreadripper1900X8-CoreProcessor -27
        AMDRyzenThreadripper1920X12-CoreProcessor -27
        AMDRyzenThreadripper19008-CoreProcessor -10

         AMD Ryzen 7 4800H -48

        For 2600 seems to be -49
        2700X seems to be -59

        Threadripper 2990WX seems to be -76

        */
        if (iTrueTmp != 0 && usingBlanketOffset == true)
        {
            SysInfo_DebugLogF(L"iTrueTmp != 0 && usingBlanketOffset == true");

            /*
            From open hardware monitor as suggested by AMD
              new TctlOffsetItem { Name = "AMD Ryzen 5 1600X", Offset = 20.0f },
              new TctlOffsetItem { Name = "AMD Ryzen 7 1700X", Offset = 20.0f },
              new TctlOffsetItem { Name = "AMD Ryzen 7 1800X", Offset = 20.0f },
              new TctlOffsetItem { Name = "AMD Ryzen 7 2700X", Offset = 10.0f },
              new TctlOffsetItem { Name = "AMD Ryzen Threadripper 19", Offset = 27.0f },
              new TctlOffsetItem { Name = "AMD Ryzen Threadripper 29", Offset = 27.0f }
            */

            if (wcsstr(szTypeString, L"1600X") != NULL ||
                wcsstr(szTypeString, L"1700X") != NULL ||
                wcsstr(szTypeString, L"1800X") != NULL)
                iTrueTmp -= 20;
            else if (wcsstr(szTypeString, L"2700X") != NULL)
                iTrueTmp -= 10;
            else if (wcsstr(szTypeString, L"Threadripper 19") != NULL ||
                     wcsstr(szTypeString, L"Threadripper 29") != NULL)
                iTrueTmp -= 27;

            SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: iTrueTmp %d"), iTrueTmp);
        }
        else if (iTrueTmp != 0 && usingBlanketOffset == false)
        {
            SysInfo_DebugLogF(L"iTrueTmp != 0 && usingBlanketOffset == false");

            // Old method, left in to catch
            if (wcsstr(szTypeString, L"1600X") != NULL ||
                wcsstr(szTypeString, L"1700X") != NULL ||
                wcsstr(szTypeString, L"1800X") != NULL)
                iTrueTmp -= 20;
            else if (wcsstr(szTypeString, L"1950X16") != NULL ||
                     wcsstr(szTypeString, L"1900X8") != NULL ||
                     wcsstr(szTypeString, L"1920X12") != NULL ||
                     wcsstr(szTypeString, L"1900X") != NULL ||
                     wcsstr(szTypeString, L"1920X") != NULL ||
                     wcsstr(szTypeString, L"1950X") != NULL ||
                     wcsstr(szTypeString, L"2990X") != NULL)
                iTrueTmp -= 27;
            else if (wcsstr(szTypeString, L"195016") != NULL ||
                     wcsstr(szTypeString, L"192012") != NULL ||
                     wcsstr(szTypeString, L"19008") != NULL)
                iTrueTmp -= 10;
            else if (wcsstr(szTypeString, L"2700X") != NULL)
                iTrueTmp -= 59;
            else if (wcsstr(szTypeString, L"2700") && wcsstr(szTypeString, L"PRO") == NULL) // 2700 (non pro) seems to be 51
                iTrueTmp -= 51;
            else if (wcsstr(szTypeString, L"2990WX") != NULL || wcsstr(szTypeString, L"2950X") != NULL || wcsstr(szTypeString, L"2920X") != NULL || wcsstr(szTypeString, L"2970WX") != NULL)
                iTrueTmp -= 76;
            else if ((wcsstr(szTypeString, L"2600") != NULL && wcsstr(szTypeString, L"2600H") == NULL) || // Need to try and not pick up 2600H (unless it turns out they need -49 as well), R5 PRO 2600 seems to be -49 but may be closer to -46
                     (wcsstr(szTypeString, L"2700") != NULL && wcsstr(szTypeString, L"PRO") != NULL) ||   // 2700 PRO seems to be -49 based on customer report
                     (wcsstr(szTypeString, L"1600") != NULL) ||                                           // Ryzen 5 1600 is -49 based on customer report
                     (wcsstr(szTypeString, L"2300X") != NULL) ||                                          // R3 2300X is -49 based on customer report
                     (wcsstr(szTypeString, L"2500X") != NULL)                                             // R5 2500X  is -49 based on customer report
                                                                                                          // No longer need to do this for the 4000 series due to the way we're handling the blanket -49 offset now? Can be either on or off for this ones i think
                                                                                                          //|| (wcsstr(szTypeString, L"4800") != NULL) ||													// AMD Ryzen 7 4800H -49 on customer report
                                                                                                          //(wcsstr(szTypeString, L"4600") != NULL)														// AMD Ryzen 7 4600H -49 on customer report
            )
                iTrueTmp -= 49;
        }

        SysInfo_DebugLogF(_T("Get_AMD_17_CurTmp: iTrueTmp %d"), iTrueTmp);

        return iTrueTmp;
    }

    return INT_NOVALUEAVAILABLE; // SI10
}

bool Get_AMD_17_HTCState()
{
    DWORD Bus, Dev, Fun;
    DWORD Index;
    DWORD Data;

    Bus = 0;
    Dev = 0;
    Fun = 0;

    // Read the current value of SMN_INDEX_0/SMN_DATA_0
    Index = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0);
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_17_HTCState: NB_SMN_INDEX_0=0x%08X NB_SMN_DATA_0=0x%08X"), Index, Data);

    SysInfo_DebugLogF(_T("Get_AMD_17_HTCState: Setting NB_SMN_INDEX_0 to 0x%08X"), THM_TCON_HTC);

    // Set SMN_INDEX_0 to THM_TCON_CUR_TMP to read SMU thermal register
    Set_PCI_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0, THM_TCON_HTC);

    SysInfo_DebugLogF(_T("Get_AMD_17_HTCState: NB_SMN_INDEX_0 was set to 0x%08X"), THM_TCON_HTC);

    // This sleep didn't seem to be enough when system was under heavy load
    Sleep(10);

    // Read NB_SMN_DATA_0 to read SMU thermal register THM_TCON_CUR_TMP
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_17_HTCState: NB_SMN_DATA_0=0x%08X"), Data);

    // If value read was 0 try again as we see under heavy load temperatures being read as 0 a lot on AMD chips
    // Also when another tmeporature reading application (liek Ryzen master) was running
    // sleep briefly then attempt another read
    if (Data == 1)
    {
        // Try a much longer sleep
        Sleep(50);

        Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);
        SysInfo_DebugLogF(_T("Get_AMD_17_HTCState: Second read : NB_SMN_DATA_0=0x%08X"), Data);
    }

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
        BOOL bHTCEn = BitExtract(Data, 0, 0); // HTC_EN. Read-write. Reset: 0. HTC feature enable.
        if (bHTCEn)
        {
            return BitExtract(Data, 4, 4) != 0 ? true : false; // HTC_ACTIVE. Read-only. Reset: 0. HTC active status. 1=HTC is active. 0=HTC is inactive.
        }
    }
    return false;
}

int Get_AMD_19_CurTmp(wchar_t *szTypeString) // SI101036
{
    bool usingBlanketOffset = false; // Set to tru if using the -49 based on setting

    DWORD Bus, Dev, Fun;
    DWORD Index;
    DWORD Data;

    Bus = 0;
    Dev = 0;
    Fun = 0;

    // Read the current value of SMN_INDEX_0/SMN_DATA_0
    Index = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0);
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: %s - NB_SMN_INDEX_0=0x%08X NB_SMN_DATA_0=0x%08X"), szTypeString, Index, Data);

    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: Setting NB_SMN_INDEX_0 to 0x%08X"), THM_TCON_CUR_TMP);

    // Set SMN_INDEX_0 to THM_TCON_CUR_TMP to read SMU thermal register
    if (!Set_AMD_PCIEx_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0, THM_TCON_CUR_TMP))
    {
        SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: Failed to set NB_SMN_INDEX_0 to 0x%08X"), THM_TCON_CUR_TMP);
        return INT_NOVALUEAVAILABLE;
    }

    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: NB_SMN_INDEX_0 was set to 0x%08X"), THM_TCON_CUR_TMP);

    // This sleep didn't seem to be enough when system was under heavy load
    Sleep(10);

    // Read NB_SMN_DATA_0 to read SMU thermal register THM_TCON_CUR_TMP
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: NB_SMN_DATA_0=0x%08X"), Data);

    // If value read was 0 try again as we see under heavy load temperatures being read as 0 a lot on AMD chips
    // Also when another tmeporature reading application (liek Ryzen master) was running
    // sleep briefly then attempt another read
    if (Data == 1)
    {
        // Try a much longer sleep
        Sleep(50);

        Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);
        SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: Second read : NB_SMN_DATA_0=0x%08X"), Data);
    }

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
        int iCurTmpTjSel = (int)(BitExtract(Data, 17, 16));
        int iCurTmpRangeSel = (int)(BitExtract(Data, 19, 19));
        int iCurTmpRaw = ((int)(BitExtract(Data, 31, 21)));
        int iTrueTmp = 0;

        SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: 0x%X, %d (RANGE_SEL=%d, TJ_SEL=%d)"), Data, iCurTmpRaw, iCurTmpRangeSel, iCurTmpTjSel);

        iTrueTmp = iCurTmpRaw >> 3;
        if (iCurTmpRangeSel) // CUR_TEMP_RANGE_SEL.Reset: 0. 0 = Report on 0C to 225C scale range. 1 = Report on - 49C to 206C scale range.
        {
            iTrueTmp -= 49;
            usingBlanketOffset = true; // Use this to stop doing a double - (eg for 2600)
            SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: usingBlanketOffset %d"), iTrueTmp);
        }

        if (iTrueTmp != 0)
        {
            if (wcsstr(szTypeString, L"5900X") != NULL)
                iTrueTmp -= 10;

            if (usingBlanketOffset)
            {
            }
            else
            {
                // https://www.amd.com/en/processors/epyc-7003-series
                if (wcsstr(szTypeString, L"EPYC 72") != NULL || // https://cerb.passmark.com/profiles/ticket/FJD-91118-223/#message373196
                    wcsstr(szTypeString, L"EPYC 73") != NULL || // https://cerb.passmark.com/profiles/ticket/FJD-91118-223/#message373196
                    wcsstr(szTypeString, L"EPYC 74") != NULL || // https://cerb.passmark.com/profiles/ticket/XFW-19221-554/#message381850
                    wcsstr(szTypeString, L"EPYC 75") != NULL || // https://cerb.passmark.com/profiles/ticket/CMK-25293-611/#message371447
                    wcsstr(szTypeString, L"EPYC 76") != NULL ||
                    wcsstr(szTypeString, L"EPYC 77") != NULL)
                {
                    iTrueTmp -= 49;
                    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: EPYC 7003 series offset"));
                }
                else if (iCurTmpTjSel == 0x3) // https://lore.kernel.org/lkml/20230413213958.847634-1-babu.moger@amd.com/T/
                {
                    /*
                    Spec says, when CUR_TEMP_TJ_SEL == 3 and CUR_TEMP_RANGE_SEL == 0,
                    it should use RangeUnadjusted is 0, which is (CurTmp*0.125 -49) C. The
                    CUR_TEMP register is read-write when CUR_TEMP_TJ_SEL == 3 (bit 17-16).
                    */
                    if (wcsstr(szTypeString, L"EPYC 91") != NULL || // https://cerb.passmark.com/profiles/ticket/JAK-21369-963/#message389202
                        wcsstr(szTypeString, L"EPYC 92") != NULL || // https://cerb.passmark.com/profiles/ticket/MWH-85677-383/#message388652
                        wcsstr(szTypeString, L"EPYC 93") != NULL || // https://cerb.passmark.com/profiles/ticket/JAK-21369-963/#message389202
                        wcsstr(szTypeString, L"EPYC 94") != NULL || // https://cerb.passmark.com/profiles/ticket/JAK-21369-963/#message389202
                        wcsstr(szTypeString, L"EPYC 95") != NULL || // https://cerb.passmark.com/profiles/ticket/JAK-21369-963/#message389202
                        wcsstr(szTypeString, L"EPYC 96") != NULL)   // https://cerb.passmark.com/profiles/ticket/WXJ-86272-161/#message387906
                    {
                        iTrueTmp -= 49;
                        SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: EPYC 9004 series offset"));
                    }
                    else if (wcsstr(szTypeString, L"Eng Sample") != NULL) // https://cerb.passmark.com/profiles/ticket/QXJ-25143-851/#message388181
                    {
                        iTrueTmp -= 49;
                        SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: Eng Sample offset"));
                    }
                    else
                    {
                        iTrueTmp -= 49;

                        SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: %s offset"), szTypeString);
                    }
                }
            }
        }

        SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: iTrueTmp %d"), iTrueTmp);

        return iTrueTmp;
    }
#if 0 // See AMD Family 19H PPR: SB Temperature Sensor Interface (SB-TSI)
  Data = smn_read(0, SMUSBI_SBIREGADDR);

  if (IsDebugLog())
  {
    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: SMUSBI_SBIREGADDR=0x%08X"), Data);
    DebugLog(SysInfo_DebugString);
  }

  Sleep(10);

  smn_write(0, SMUSBI_SBIREGADDR, SBTSI_CONFIG);

  Sleep(10);

  Data = smn_read(0, SMUSBI_SBIREGDATA);

  if (IsDebugLog())
  {
    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: SBTSI_CONFIG=0x%08X"), Data);
    DebugLog(SysInfo_DebugString);
  }

  Sleep(10);

  smn_write(0, SMUSBI_SBIREGADDR, SBTSI_CPUTEMPINT);

  Sleep(10);

  Data = smn_read(0, SMUSBI_SBIREGDATA);

  if (IsDebugLog())
  {
    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: SBTSI_CPUTEMPINT=0x%08X"), Data);
    DebugLog(SysInfo_DebugString);
  }

  Sleep(10);

  smn_write(0, SMUSBI_SBIREGADDR, SBTSI_CPUTEMPDEC);

  Sleep(10);

  Data = smn_read(0, SMUSBI_SBIREGDATA);

  if (IsDebugLog())
  {
    SysInfo_DebugLogF(_T("Get_AMD_19_CurTmp: SBTSI_CPUTEMPDEC=0x%08X"), Data);
    DebugLog(SysInfo_DebugString);
  }
#endif
    return INT_NOVALUEAVAILABLE; // SI10
}

int Get_AMD_1A_CurTmp(wchar_t *szTypeString) // SI101036
{
    bool usingBlanketOffset = false; // Set to tru if using the -49 based on setting

    DWORD Bus, Dev, Fun;
    DWORD Index;
    DWORD Data;

    Bus = 0;
    Dev = 0;
    Fun = 0;

    // Read the current value of SMN_INDEX_0/SMN_DATA_0
    Index = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0);
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: %s - NB_SMN_INDEX_0=0x%08X NB_SMN_DATA_0=0x%08X"), szTypeString, Index, Data);

    SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: Setting NB_SMN_INDEX_0 to 0x%08X"), THM_TCON_CUR_TMP);

    // Set SMN_INDEX_0 to THM_TCON_CUR_TMP to read SMU thermal register
    if (!Set_AMD_PCIEx_Reg(Bus, Dev, Fun, NB_SMN_INDEX_0, THM_TCON_CUR_TMP))
    {
        SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: Failed to set NB_SMN_INDEX_0 to 0x%08X"), THM_TCON_CUR_TMP);
        return INT_NOVALUEAVAILABLE;
    }

    SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: NB_SMN_INDEX_0 was set to 0x%08X"), THM_TCON_CUR_TMP);

    // This sleep didn't seem to be enough when system was under heavy load
    Sleep(10);

    // Read NB_SMN_DATA_0 to read SMU thermal register THM_TCON_CUR_TMP
    Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);

    SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: NB_SMN_DATA_0=0x%08X"), Data);

    // If value read was 0 try again as we see under heavy load temperatures being read as 0 a lot on AMD chips
    // Also when another tmeporature reading application (liek Ryzen master) was running
    // sleep briefly then attempt another read
    if (Data == 1)
    {
        // Try a much longer sleep
        Sleep(50);

        Data = Get_PCI_Reg(Bus, Dev, Fun, NB_SMN_DATA_0);
        SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: Second read : NB_SMN_DATA_0=0x%08X"), Data);
    }

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
        int iCurTmpTjSel = (int)(BitExtract(Data, 17, 16));
        int iCurTmpRangeSel = (int)(BitExtract(Data, 19, 19));
        int iCurTmpRaw = ((int)(BitExtract(Data, 31, 21)));
        int iTrueTmp = 0;

        SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: 0x%X, %d (RANGE_SEL=%d, TJ_SEL=%d)"), Data, iCurTmpRaw, iCurTmpRangeSel, iCurTmpTjSel);

        iTrueTmp = iCurTmpRaw >> 3;
        if (iCurTmpRangeSel) // CUR_TEMP_RANGE_SEL.Reset: 0. 0 = Report on 0C to 225C scale range. 1 = Report on - 49C to 206C scale range.
        {
            iTrueTmp -= 49;
            usingBlanketOffset = true; // Use this to stop doing a double - (eg for 2600)
            SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: usingBlanketOffset %d"), iTrueTmp);
        }

        if (iTrueTmp != 0)
        {
            if (usingBlanketOffset)
            {
            }
            else
            {
                if (0)
                {
                    iTrueTmp -= 49;
                    SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: EPYC 7003 series offset"));
                }
                else if (iCurTmpTjSel == 0x3) // https://lore.kernel.org/lkml/20230413213958.847634-1-babu.moger@amd.com/T/
                {
                    /*
                    Spec says, when CUR_TEMP_TJ_SEL == 3 and CUR_TEMP_RANGE_SEL == 0,
                    it should use RangeUnadjusted is 0, which is (CurTmp*0.125 -49) C. The
                    CUR_TEMP register is read-write when CUR_TEMP_TJ_SEL == 3 (bit 17-16).
                    */
                    if (0)
                    {
                        iTrueTmp -= 49;
                        SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: EPYC 9004 series offset"));
                    }
                    else
                    {
                        iTrueTmp -= 49;

                        SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: %s offset"), szTypeString);
                    }
                }
            }
        }

        SysInfo_DebugLogF(_T("Get_AMD_1A_CurTmp: iTrueTmp %d"), iTrueTmp);

        return iTrueTmp;
    }
    return INT_NOVALUEAVAILABLE; // SI10
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// AMD K14 functions
///////////////////////////////////////////////////////////////////////////////////////////

// Name:	GetAMDCPUTemps()
//
//  Gets temperatures from the CPU MSR's.
//
EFI_STATUS GetAMDCPUTemps(CPUINFO *pCPUInfo, UINTN ProcNum, MSR_TEMP *CPUMSRTemp)
{
    EFI_STATUS Status = EFI_DEVICE_ERROR;
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    UINTN OldBSP = MPSupportGetBspId();

    // Get the CPU core temperatures
    if (OldBSP != ProcNum)
    {
        Status = MPSupportSwitchBSP(ProcNum);
        if (EFI_ERROR(Status))
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDCPUTemps - Could not switch BSP to Proc#%d (%r)", ProcNum, Status);
            MtSupportDebugWriteLine(gBuffer);
            return Status;
        }
    }

    // Get the CPU MSR info
    if (IsAMDFamily10h(pCPUInfo))
        GetAMD10Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily11h(pCPUInfo))
        GetAMD11Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily12h(pCPUInfo))
        GetAMD12Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily14h(pCPUInfo))
        GetAMD14Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily15h(pCPUInfo))
        GetAMD15Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily16h(pCPUInfo))
        GetAMD16Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily17h(pCPUInfo))
        GetAMD17Temp(pCPUInfo, CPUMSRTemp);
    else if (IsHygonFamily18h(pCPUInfo))
        GetHygon18Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily19h(pCPUInfo))
        GetAMD19Temp(pCPUInfo, CPUMSRTemp);
    else if (IsAMDFamily1Ah(pCPUInfo))
        GetAMD1ATemp(pCPUInfo, CPUMSRTemp);
    else
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDCPUTemps - Unsupported AMD CPU  (Vendor ID: %s %x %x %x)", pCPUInfo->manufacture, pCPUInfo->Family, pCPUInfo->Model, pCPUInfo->stepping);
        MtSupportDebugWriteLine(gBuffer);
    }

    // Validate the temperature from the CPU core
    // if (CPUMSRTemp.iTemperature > 0)
    if (CPUMSRTemp->iTemperature > MIN_TEMP_SANITY)
    {
        Status = EFI_SUCCESS;
    }

    if (OldBSP != ProcNum)
    {
        if (MPSupportSwitchBSP(OldBSP) != EFI_SUCCESS)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "GetAMDCPUTemps - Could not switch BSP back to Proc#%d", OldBSP);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
#endif
    return Status;
}
