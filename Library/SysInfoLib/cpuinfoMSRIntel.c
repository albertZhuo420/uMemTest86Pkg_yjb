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
//	cpuinfoMSRIntel.c
//
// Author(s):
//	Keith Mah, Ian Robinson
//
// Description:
//	Detects and gathers information about Intel CPU(s).
//	Including speed, manufacturer, capabilities, available L2 cache, etc..
//
//   Based on source code from SysInfo

#include "uMemTest86.h"
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>
#include <Library/Cpuid.h>
#include <cpuinfoMSRIntel.h>
#include <cpuinfo.h>
#include <cputemp.h>
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
#include <cpuinfo_specifications.h>
#endif

#define _tcscmp StrCmp
#define _T(x) L##x

#define IA32_PERF_GLOBAL_CTRL 0x38F // Global Performance Counter Control
#define IA32_FIXED_CTR_CTL 0x38D    // Fixed-Function Performance Counter Control
#define IA32_FIXED_CTR1 0x30A       // Fixed-Function Performance Counter
#define IA32_FIXED_CTR2 0x30B       // Fixed-Function Performance Counter 2
#define IA32_PERF_CTL 0x199         // Performance Control
#define IA32_PERF_STATUS 0x198      // Performance Status
#define IA32_MISC_ENABLE 0x1A0

#define IDA_ENGAGE 32  // bit number
#define IDA_DISABLE 38 // bit number

#define CACHE_DATA 1
#define CACHE_INSTRUCTION 2

extern UINTN cpu_speed;

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
BOOLEAN MapTempIntel(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);
BOOLEAN GetCPUTempJunction(CPUINFO *pCPUInfo, int *pTJunction);

STATIC __inline void GetMinMaxMultiplier(CPUMSRINFO *CPUMSRinfo);

STATIC __inline void PopulateCPUClock(CPUMSRINFO *cpuMSRinfo, int flCPUExternalClockStock);

STATIC __inline void GetCPUThermalAndPowerLeaf(CPUINFO *cpuinfo);

STATIC __inline void GetTemperatureTarget(CPUMSRINFO *cpuMSRinfo);

STATIC __inline void GetTurboMultipliers(CPUMSRINFO *cpuMSRinfo);
#endif

/*
See Intel Volume 3C
Table B-1. CPUID Signature Values of DisplayFamily_DisplayModel
DisplayFamily_DisplayModel Processor Families/Processor Number Series
06_57H Next Generation Intel Xeon Phi Processor Family
06_4EH Future Generation Intel Core Processor
06_56H Next Generation Intel Xeon Processor D Product Family based on Broadwell microarchitecture
06_4FH Future Generation Intel Xeon processor based on Broadwell microarchitecture
06_3DH Intel Core M-5xxx Processor, 5th generation Intel Core processors based on Broadwell
microarchitecture
06_3FH Intel Xeon processor E5-2600/1600 v3 product families based on Haswell-E microarchitecture, Intel
Core i7-59xx Processor Extreme Edition
06_3CH, 06_45H, 06_46H 4th Generation Intel Core processor and Intel Xeon processor E3-1200 v3 product family based on
Haswell microarchitecture
06_3EH Intel Xeon processor E7-8800/4800/2800 v2 product families based on Ivy Bridge-E
microarchitecture
06_3EH Intel Xeon processor E5-2600/1600 v2 product families and Intel Xeon processor E5-2400 v2
product family based on Ivy Bridge-E microarchitecture, Intel Core i7-49xx Processor Extreme Edition
06_3AH 3rd Generation Intel Core Processor and Intel Xeon processor E3-1200 v2 product family based on Ivy
Bridge microarchitecture
06_2DH Intel Xeon processor E5 Family based on Intel microarchitecture code name Sandy Bridge, Intel Core
i7-39xx Processor Extreme Edition
06_2FH Intel Xeon Processor E7 Family
06_2AH Intel Xeon processor E3-1200 product family; 2nd Generation Intel Core i7, i5, i3 Processors 2xxx
Series
06_2EH Intel Xeon processor 7500, 6500 series
06_25H, 06_2CH Intel Xeon processors 3600, 5600 series, Intel Core i7, i5 and i3 Processors
06_1EH, 06_1FH Intel Core i7 and i5 Processors
06_1AH Intel Core i7 Processor, Intel Xeon processor 3400, 3500, 5500 series
06_1DH Intel Xeon processor MP 7400 series
06_17H Intel Xeon processor 3100, 3300, 5200, 5400 series, Intel Core 2 Quad processors 8000, 9000
series
06_0FH Intel Xeon processor 3000, 3200, 5100, 5300, 7300 series, Intel Core 2 Quad processor 6000 series,
Intel Core 2 Extreme 6000 series, Intel Core 2 Duo 4000, 5000, 6000, 7000 series processors, Intel
Pentium dual-core processors
06_0EH Intel Core Duo, Intel Core Solo processors
06_0DH Intel Pentium M processor
06_4CH Intel Atom processor Z8000 series
06_5DH Future Intel Atom Processor Based on Silvermont Microarchitecture
06_5AH Intel Atom processor Z3500 series
06_4AH Intel Atom processor Z3400 series
06_37H Intel Atom processor E3000 series, Z3600 series, Z3700 series
06_4DH Intel Atom processor C2000 series
06_36H Intel Atom processor S1000 Series
06_1CH, 06_26H, 06_27H,
06_35H, 06_36H
Intel Atom processor family, Intel Atom processor D2000, N2000, E2000, Z2000, C1000 series
0F_06H Intel Xeon processor 7100, 5000 Series, Intel Xeon Processor MP, Intel Pentium 4, Pentium D
processors
0F_03H, 0F_04H Intel Xeon processor, Intel Xeon processor MP, Intel Pentium 4, Pentium D processors
06_09H Intel Pentium M processor
0F_02H Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
0F_0H, 0F_01H Intel Xeon Processor, Intel Xeon processor MP, Intel Pentium 4 processors
06_7H, 06_08H, 06_0AH,
06_0BH
Intel Pentium III Xeon processor, Intel Pentium III processor
06_03H, 06_05H Intel Pentium II Xeon processor, Intel Pentium II processor
06_01H Intel Pentium Pro processor
05_01H, 05_02H, 05_04H Intel Pentium processor, Intel Pentium processor with MMX Technology
*/
///////////////////////////////////////////////////////////////////////////////////////////
// IA32 generic MSRs
///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
// Intel CPUs currently supported
BOOLEAN SysInfo_IsLunarLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xBD)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsArrowLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xAC)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xC6)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsGraniteRapids(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xAD)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xAE)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsSierraForest(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xAF)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsMeteorLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xAA)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsEmeraldRapidsScalable(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xcf)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsSapphireRapidsScalable(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x8f)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsTremont(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x86)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x96)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x9c)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsRaptorLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xBA)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xB7)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xBF)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsIceLakeScalable(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x6A)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x6C)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsAlderLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x97)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x9A)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsElkhartLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x96)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsRocketLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xA7)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsTigerLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x8C)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x8D)
            return TRUE;
    }
    return FALSE;
} ///////////////////////////////////////////////////////////////////////////////////////////

BOOLEAN SysInfo_IsIceLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if ((pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x7D) ||
            (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x7E))
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsCometLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if ((pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xA5) ||
            (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0xA6))
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsKnightsMill(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x85)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsCannonLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x66)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsKabyLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x8E)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x9E)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsGeminiLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x7A)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsGoldmontPlus(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x7A)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsApolloLake(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x5C)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsGoldmont(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x5C)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x5F)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsXeonBroadwellE(CPUINFO *pCPUInfo) // SI101112
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x4F)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x56)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsSkylakeScalable(CPUINFO *pCPUInfo) // SI101106
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x55)
            return TRUE; // e.g. Intel Core i9-7900X
    }
    return FALSE;
}

BOOLEAN SysInfo_IsSkylake(CPUINFO *pCPUInfo) // SI101106
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x4E)
            return TRUE; // e.g. Intel(R) Core(TM) m7-6Y75 CPU @ 1.20GHz [2967.8 MHz],
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x5E)
            return TRUE; // e.g. Intel(R) Core(TM) i5 - 6500T CPU @ 2.50GHz,
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x55)
            return TRUE; // e.g. Intel Core i9-7900X
    }
    return FALSE;
}

BOOLEAN SysInfo_IsKnightsLanding(CPUINFO *pCPUInfo) // SI101100
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x57)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsNextGenerationXeonHaswell(CPUINFO *pCPUInfo) // SI101073
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x3F)
            return TRUE;
    }
    return FALSE;
}
BOOLEAN SysInfo_IsBroadwell(CPUINFO *pCPUInfo) // SI101073
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x3D)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x47)
            return TRUE; // SI101109 Broadwell desktop i3/i5/i7 and Xeon E3
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x4F)
            return TRUE; // SI101100 Xeon
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x56)
            return TRUE; // SI101100 Xeon D
    }
    return FALSE;
}

BOOLEAN SysInfo_IsBroadwellCoreOrM(CPUINFO *pCPUInfo) // SI101100
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x3D)
            return TRUE; // Intel Core M-5xxx processors and 5th generation Intel Core Processors
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x47)
            return TRUE; // Intel Xeon Processor E3-1200 v4 family and the 5th generation Intel Core Processors
    }
    return FALSE;
}

BOOLEAN SysInfo_IsIvyBridgeEP(CPUINFO *pCPUInfo) // SI101039
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x3E)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsAirmont(CPUINFO *pCPUInfo) // SI101100
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x4C)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsSilvermont(CPUINFO *pCPUInfo) // SI101039
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x37)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x4D)
            return TRUE;

        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x4A)
            return TRUE; // SI101083 - Future Intel Atom Processor Based on Silvermont Microarchitecture
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x5A)
            return TRUE; // SI101083
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x5D)
            return TRUE; // SI101083
    }
    return FALSE;
}

BOOLEAN SysInfo_IsHaswell(CPUINFO *pCPUInfo) // SI101032
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x3C)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x45)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x46)
            return TRUE; // Crystal Well groupsed with Haswell in specs
    }
    return FALSE;
}

BOOLEAN SysInfo_IsIvyBridgeOrLater(CPUINFO *pCPUInfo) // SI101052
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model >= 0x3A)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsIvyBridge(CPUINFO *pCPUInfo) // SI101022
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x3A)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN SysInfo_IsXeonE7(CPUINFO *pCPUInfo) // SI101022
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x2F)
            return TRUE;
    } // Intel Xeon processor E7 family
    return FALSE;
}

BOOLEAN SysInfo_IsSandyBridge(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x2A)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x2D)
            return TRUE;
    }
    return FALSE;
}
BOOLEAN SysInfo_IsWestmere(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x25)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x2C)
            return TRUE;
    }
    return FALSE;
}
BOOLEAN SysInfo_IsNehalemArhitecture(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1A)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1E)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1F)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x2E)
            return TRUE;
    }
    return FALSE;
}
BOOLEAN IsIntelXeon_1D(CPUINFO *pCPUInfo) // BIT6010120002
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1d)
            return TRUE;
    }
    return FALSE;
}

BOOLEAN IsIntelAtom(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1C)
            return TRUE;
    } // Atom family
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x26)
            return TRUE;
    } // SI101034 Added 06_26H, 06_27H,06_35, 06_36 as per Intel spec
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x27)
            return TRUE;
    } // SI101034 Added 06_26H, 06_27H,06_35, 06_36 as per Intel spec
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x35)
            return TRUE;
    } // SI101034 Added 06_26H, 06_27H,06_35, 06_36 as per Intel spec
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x36)
            return TRUE;
    } // SI101034 Added 06_26H, 06_27H,06_35, 06_36 as per Intel spec
    return FALSE;
}

BOOLEAN IsIntelCore2(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x0f)
            return TRUE; // Intel core microsrchitecture is 06_0F (model 15), 06_17 (model 23)
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x17)
            return TRUE;
    }
    return FALSE;
}
BOOLEAN IsIntelXeon_17(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x17)
            return TRUE;
    }
    return FALSE;
}
BOOLEAN IsIntelCoreSoloDuo(CPUINFO *pCPUInfo) // BIT601000.0012
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x0e)
            return TRUE;
    }
    return FALSE;
}
BOOLEAN IsIntelNetBurst(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x0F)
            return TRUE;
    } // P4 and Xeon are 0F_03, 0F_04 and 0F_06
    return FALSE;
}

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
///////////////////////////////////////////////////////////////////////////////////////////
//  Intel Series 2 CoreT Ultra processors supporting Lunar Lake performance hybrid architecture
///////////////////////////////////////////////////////////////////////////////////////////

void GetLunarLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsLunarLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Arrow Lake
///////////////////////////////////////////////////////////////////////////////////////////

// GetArrowLakeFamilyMSRInfo_Static
//
void GetArrowLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsArrowLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
//  Intel Xeon 6 P-core processors based on Granite Rapids microarchitecture
///////////////////////////////////////////////////////////////////////////////////////////

void GetGraniteRapidsFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsGraniteRapids(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
//  Intel Xeon 6 P-core processors based on Sierra Forest microarchitecture
///////////////////////////////////////////////////////////////////////////////////////////

void GetSierraForestFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsSierraForest(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Meteor Lake
///////////////////////////////////////////////////////////////////////////////////////////

// GetMeteorLakeFamilyMSRInfo_Static
//
void GetMeteorLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsMeteorLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// 5th generation Intel Xeon Scalable Processor Family based on Emerald Rapids microarchitecture
///////////////////////////////////////////////////////////////////////////////////////////
void GetEmeraldRapidsScalableFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsEmeraldRapidsScalable(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// 4th generation Intel Xeon Scalable Processor Family based on Sapphire Rapids microarchitecture
///////////////////////////////////////////////////////////////////////////////////////////
void GetSapphireRapidsScalableFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsSapphireRapidsScalable(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Intel Atom processors, Intel Celeron processors, Intel Pentium processors, and Intel Pentium
// Silver processors based on Tremont Microarchitecture
///////////////////////////////////////////////////////////////////////////////////////////

// GetTremontFamilyMSRInfo_Static
//
void GetTremontFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsTremont(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Raptor Lake
///////////////////////////////////////////////////////////////////////////////////////////

// GetRaptorLakeFamilyMSRInfo_Static
//
void GetRaptorLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsRaptorLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Ice Lake SP (3rd Gen Scalable)
///////////////////////////////////////////////////////////////////////////////////////////

// GetIceLakeScalableFamilyMSRInfo_Static
//
void GetIceLakeScalableFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsIceLakeScalable(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Alder Lake
// 06_97H
///////////////////////////////////////////////////////////////////////////////////////////

// GetAlderLakeFamilyMSRInfo_Static
//
void GetAlderLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsAlderLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Elkhart Lake
// 06_96H
///////////////////////////////////////////////////////////////////////////////////////////

// GetElkhartLakeFamilyMSRInfo_Static
//
void GetElkhartLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsElkhartLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Rocket Lake
// 06_A7H
///////////////////////////////////////////////////////////////////////////////////////////

// GetRocketLakeFamilyMSRInfo_Static
//
void GetRocketLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsRocketLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Tiger Lake
// 06_8CH, 06_8DH
///////////////////////////////////////////////////////////////////////////////////////////

// GetTigerLakeFamilyMSRInfo_Static
//
void GetTigerLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsTigerLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Ice Lake
// 06_7DH, 06_7EH
///////////////////////////////////////////////////////////////////////////////////////////

// GetIceLakeFamilyMSRInfo_Static
//
void GetIceLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsIceLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Comet Lake
// 06_A6H
///////////////////////////////////////////////////////////////////////////////////////////

// GetCometLakeFamilyMSRInfo_Static
//
void GetCometLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsCometLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Knights Mill
// 06_85H
///////////////////////////////////////////////////////////////////////////////////////////

// GetKnightsMillFamilyMSRInfo_Static
//
void GetKnightsMillFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101073
{
    if (!SysInfo_IsKnightsMill(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
    {
        UINT64 ullMSRResult = AsmReadMsr64(0x1AD);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetKnightsMillFamilyMSRInfo_Static - MSR[0x1AD] = %016lx", ullMSRResult);
        MtSupportDebugWriteLine(gBuffer);
        pCPUMSRInfo->ullMSR0x1AD = ullMSRResult;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Cannon Lake
// 06_66H
///////////////////////////////////////////////////////////////////////////////////////////

// GeCannonLakeFamilyMSRInfo_Static
//
void GetCannonLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsCannonLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Kaby Lake
// 06_8EH, 06_9EH
///////////////////////////////////////////////////////////////////////////////////////////

// GetKabyLakeFamilyMSRInfo_Static
//
void GetKabyLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsKabyLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Gemini Lake Intel Atom processors based on Goldmont Plus Microarchitecture
// 06_7AH
///////////////////////////////////////////////////////////////////////////////////////////

// GetGeminiLakeFamilyMSRInfo_Static
//
void GetGeminiLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsGeminiLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
    {
        GetTurboMultipliers(pCPUMSRInfo);

        // Output MSR_TURBO_GROUP_CORE_CNT for debug
        UINT64 ullMSRResult = AsmReadMsr64(0x1AE);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetGeminiLakeFamilyMSRInfo_Static - MSR[0x1AE] = %016lx", ullMSRResult);
        MtSupportDebugWriteLine(gBuffer);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Apollo Lake Next Generation Intel Atom processors based on Goldmont microarchitecture
// 06_5CH
///////////////////////////////////////////////////////////////////////////////////////////

// GetApolloLakeFamilyMSRInfo_Static
//
void GetApolloLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsApolloLake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
    {
        GetTurboMultipliers(pCPUMSRInfo);

        // Output MSR_TURBO_GROUP_CORE_CNT for debug
        UINT64 ullMSRResult = AsmReadMsr64(0x1AE);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetApolloLakeFamilyMSRInfo_Static - MSR[0x1AE] = %016lx", ullMSRResult);
        MtSupportDebugWriteLine(gBuffer);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Broadwell-E/EP Future Generation Intel Xeon processor based on Broadwell microarchitecture
// 06_4FH
///////////////////////////////////////////////////////////////////////////////////////////

// GetXeonBroadwellEFamilyMSRInfo_Static
//
void GetXeonBroadwellEFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsXeonBroadwellE(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Skylake
// 06_4EH, 06_5EH
///////////////////////////////////////////////////////////////////////////////////////////

// GetSkylakeFamilyMSRInfo_Static
//
void GetSkylakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101106
{
    if (!SysInfo_IsSkylake(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// NEXT GENERATION INTEL XEON PHI PROCESSORS - Knights landing
// 06_57H
///////////////////////////////////////////////////////////////////////////////////////////

// GetKnightsLandingFamilyMSRInfo_Static
//
void GetKnightsLandingFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101073
{
    if (!SysInfo_IsKnightsLanding(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
    {
        UINT64 ullMSRResult = AsmReadMsr64(0x1AD);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetKnightsLandingFamilyMSRInfo_Static - MSR[0x1AD] = %016lx", ullMSRResult);
        MtSupportDebugWriteLine(gBuffer);
        pCPUMSRInfo->ullMSR0x1AD = ullMSRResult;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Next Generation Intel Xeon Processor based on Haswell microarchitecture
// 06_3FH
///////////////////////////////////////////////////////////////////////////////////////////

// GetNextGenerationXeonHaswellFamilyMSRInfo_Static
//
void GetNextGenerationXeonHaswellFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101073
{
    if (!SysInfo_IsNextGenerationXeonHaswell(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Broadwell (Intel CoreTM M Processor based on Broadwell microarchitecture.)
// 06_3DH
///////////////////////////////////////////////////////////////////////////////////////////

// GetBroadwellFamilyMSRInfo_Static
//
void GetBroadwellFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101073
{
    if (!SysInfo_IsBroadwell(pCPUInfo))
        return;

    if (SysInfo_IsBroadwellCoreOrM(pCPUInfo)) // SI101100 - from spec doesn't seem Xeons support these MSR's
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

        GetMinMaxMultiplier(pCPUMSRInfo);

        PopulateCPUClock(pCPUMSRInfo, 10000);

        // Processor Temperature
        if (pCPUInfo->bDTS)
            GetTemperatureTarget(pCPUMSRInfo);

        // Get the max multipliers for core groups
        if (pCPUInfo->bIntelTurboBoost)
            GetTurboMultipliers(pCPUMSRInfo);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// AIRMONT MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// GetAirmontFamilyMSRInfo_Dynamic
//
void GetAirmontFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101100
{
    int iMSR = 0;
    UINT64 ullMSRResult = 0;

    if (!SysInfo_IsAirmont(pCPUInfo))
        return;

    // Scalable Bus Speed - this is the intended scalable bus speed - not the current (???) beleive no need to measure under load
    iMSR = 0xCD;
    ullMSRResult = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetAirmontFamilyMSRInfo_Dynamic - MSR[0xCD] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0xCD = ullMSRResult;
        switch ((int)BitExtractULL(ullMSRResult, 3, 0))
        {
        case 0: // 000B
        case 4: // 100B
            pCPUMSRInfo->flScalableBusSpeed = 83333;
            pCPUMSRInfo->flFSB = 333332;
            break;
        case 1: // 001B
        case 5:
            pCPUMSRInfo->flScalableBusSpeed = 100000;
            pCPUMSRInfo->flFSB = 400000;
            break;
        case 2: // 010B
        case 6:
            pCPUMSRInfo->flScalableBusSpeed = 133333;
            pCPUMSRInfo->flFSB = 533332;
            break;
        case 3: // 011B
            pCPUMSRInfo->flScalableBusSpeed = 116500;
            pCPUMSRInfo->flFSB = 466000;
            break;
        case 7:
            pCPUMSRInfo->flScalableBusSpeed = 116667;
            pCPUMSRInfo->flFSB = 466668;
            break;
        case 0xC:
            pCPUMSRInfo->flScalableBusSpeed = 80000;
            pCPUMSRInfo->flFSB = 320000;
            break;
        case 0xD:
            pCPUMSRInfo->flScalableBusSpeed = 93333;
            pCPUMSRInfo->flFSB = 373332;
            break;
        case 0xE:
            pCPUMSRInfo->flScalableBusSpeed = 90000;
            pCPUMSRInfo->flFSB = 320000;
            break;
        case 0xF:
            pCPUMSRInfo->flScalableBusSpeed = 88889;
            pCPUMSRInfo->flFSB = 355556;
            break;
        case 0x14:
            pCPUMSRInfo->flScalableBusSpeed = 87500;
            pCPUMSRInfo->flFSB = 350000;
            break;
        default:
            pCPUMSRInfo->flScalableBusSpeed = 0;
            pCPUMSRInfo->flFSB = 0;
            break;
        }
    }

    // Power on - incudes Integer bus frequency ratio - dynamic?
    iMSR = 0x2A;
    ullMSRResult = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetAirmontFamilyMSRInfo_Dynamic - MSR[0x2A] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0x2A = ullMSRResult;
        pCPUMSRInfo->flBusFrequencyRatio = (int)BitExtractULL(ullMSRResult, 26, 22) * 100;
    }
}

void GetAirmontFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101100
{
    if (!SysInfo_IsAirmont(pCPUInfo))
        return;

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// SILVERMONT MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// GetSilvermontFamilyMSRInfo_Dynamic
//
void GetSilvermontFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101039
{
    int iMSR = 0;
    UINT64 ullMSRResult = 0;

    if (!SysInfo_IsSilvermont(pCPUInfo))
        return;

    // Scalable Bus Speed - this is the intended scalable bus speed - not the current (???) beleive no need to measure under load
    iMSR = 0xCD;
    ullMSRResult = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetSilvermontFamilyMSRInfo_Dynamic - MSR[0xCD] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0xCD = ullMSRResult;
        switch ((int)BitExtractULL(ullMSRResult, 2, 0))
        {
        case 4: // 100B
            pCPUMSRInfo->flScalableBusSpeed = 80000;
            pCPUMSRInfo->flFSB = 320000;
            break;
        case 0: // 000B
            pCPUMSRInfo->flScalableBusSpeed = 83333;
            pCPUMSRInfo->flFSB = 333332;
            break;
        case 1: // 001B
            pCPUMSRInfo->flScalableBusSpeed = 100000;
            pCPUMSRInfo->flFSB = 400000;
            break;
        case 2: // 010B
            pCPUMSRInfo->flScalableBusSpeed = 133333;
            pCPUMSRInfo->flFSB = 533332;
            break;
        case 3: // 011B
            pCPUMSRInfo->flScalableBusSpeed = 116667;
            pCPUMSRInfo->flFSB = 466668;
            break;
        default:
            pCPUMSRInfo->flScalableBusSpeed = 0;
            pCPUMSRInfo->flFSB = 0;
            break;
        }
    }

    // Power on - incudes Integer bus frequency ratio - dynamic?
    iMSR = 0x2A;
    ullMSRResult = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetSilvermontFamilyMSRInfo_Dynamic - MSR[0x2A] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0x2A = ullMSRResult;
        pCPUMSRInfo->flBusFrequencyRatio = (int)BitExtractULL(ullMSRResult, 26, 22) * 100;
    }
}

void GetSilvermontFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101039
{
    if (!SysInfo_IsSilvermont(pCPUInfo))
        return;

    // Get the min and max multipliers
    if (pCPUInfo->Model == 0x37) // SI101083 - Undocumented MSR for this CPU. Based on CPU-Z debug report log.
    {
        UINT64 ullMSRResult = AsmReadMsr64(0xCE);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetSilvermontFamilyMSRInfo_Static - MSR[0xCE] = %016lx", ullMSRResult);
        {
            pCPUMSRInfo->ullMSR0xCE = ullMSRResult;
            pCPUMSRInfo->iMinimumRatio = (int)(BitExtractULL(ullMSRResult, 47, 40)); // SI101084
            pCPUMSRInfo->iMaximumRatio = (int)(BitExtractULL(ullMSRResult, 15, 8));
        }
    }

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
    {
        if (pCPUInfo->Model == 0x37 || pCPUInfo->Model == 0x4C) // SI101083 - Undocumented MSR. From Linux intel_pstate.c
        {
            UINT64 ullMSRResult = AsmReadMsr64(0x66C);
            AsciiSPrint(gBuffer, BUF_SIZE, "GetSilvermontFamilyMSRInfo_Static - MSR[0x66C] = %016lx", ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);
            {
                pCPUMSRInfo->iMaxTurbo1Core = (int)(BitExtractULL(ullMSRResult, 7, 0));
                pCPUMSRInfo->iMaxTurbo2Core = (int)(BitExtractULL(ullMSRResult, 15, 8));
                pCPUMSRInfo->iMaxTurbo3Core = (int)(BitExtractULL(ullMSRResult, 23, 16));
                pCPUMSRInfo->iMaxTurbo4Core = (int)(BitExtractULL(ullMSRResult, 31, 24));

                pCPUMSRInfo->iMaxTurbo5Core = (int)(BitExtractULL(ullMSRResult, 39, 32));
                pCPUMSRInfo->iMaxTurbo6Core = (int)(BitExtractULL(ullMSRResult, 47, 40));
                pCPUMSRInfo->iMaxTurbo7Core = (int)(BitExtractULL(ullMSRResult, 55, 48));
                pCPUMSRInfo->iMaxTurbo8Core = (int)(BitExtractULL(ullMSRResult, 63, 56));
            }
        }
        else
        {
            GetTurboMultipliers(pCPUMSRInfo);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Haswell
// 06_3CH, 06_45H, 06_46H
///////////////////////////////////////////////////////////////////////////////////////////

// GetHaswellFamilyMSRInfo_Static
//
void GetHaswellFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101032
{
    if (!SysInfo_IsHaswell(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Ivy bridge-EP
// 06_3EH
///////////////////////////////////////////////////////////////////////////////////////////

// GetIvyBridgeEPFamilyMSRInfo_Static
//
void GetIvyBridgeEPFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101022
{
    if (!SysInfo_IsIvyBridgeEP(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Ivy bridge
// 06_3AH
///////////////////////////////////////////////////////////////////////////////////////////

// GetIvyBridgeFamilyMSRInfo_Static
//
void GetIvyBridgeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // SI101022
{
    if (!SysInfo_IsIvyBridge(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Get the max multipliers for core groups
    if (pCPUInfo->bIntelTurboBoost)
        GetTurboMultipliers(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// XEON E7 family
// 06_2FH
///////////////////////////////////////////////////////////////////////////////////////////

// GetXeonE7FamilyMSRInfo_Static
//
void GetXeonE7FamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // BIT6010190001
{
    if (!SysInfo_IsXeonE7(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 13333);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// 2nd Generation Intel Core 2 (Sandy Bridge)		//BIT6010280000
// 06_2AH 06_2DH
///////////////////////////////////////////////////////////////////////////////////////////

// GetSandyBridgeFamilyMSRInfo_Static
//
void GetSandyBridgeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsSandyBridge(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 10000);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    if (pCPUInfo->Family == 0x06)
    {
        // Get the max multipliers for core groups
        if (pCPUInfo->bIntelTurboBoost)
            GetTurboMultipliers(pCPUMSRInfo);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Next Generation Intel Processor (Westmere)
// 06_25H, 06_2CH
///////////////////////////////////////////////////////////////////////////////////////////

// GetWestmereFamilyMSRInfo_Static
//
void GetWestmereFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // BIT6010190001
{
    if (!SysInfo_IsWestmere(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 13333);

    // Processor Temperature
    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // TURBO_POWER_CURRENT_LIMIT (RO)	//BIT601000.0015b
    if (pCPUInfo->bIntelTurboBoost)
    {
        UINT64 ullMSRResult = AsmReadMsr64(0x1AC);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetWestmereFamilyMSRInfo_Static - MSR[0x1AC] = %016lx", ullMSRResult);
        MtSupportDebugWriteLine(gBuffer);
        pCPUMSRInfo->ullMSR0x1AC = ullMSRResult;
        // Overclocking Power and current limits (based on Email from Intel 26 Nov 2008)
        //  MSR 1ACh = TURBO_POWER_CURRENT_LIMIT (RO)
        //  How do you programmatically determine the TDP Power Limit TDP = MSR 1ACh[14:0] * 0.125 W
        //  How do you programmatically determine the TDC Current Limit TDC = MSR 1ACh[30:16] * 0.125 A
        pCPUMSRInfo->iTDPLimit = (int)(BitExtractULL(ullMSRResult, 14, 0) >> 3);  // Watts
        pCPUMSRInfo->iTDCLimit = (int)(BitExtractULL(ullMSRResult, 30, 16) >> 3); // Amps

        GetTurboMultipliers(pCPUMSRInfo);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Intel Processors based on Intel Microarchitecture (Nehalem)
// 06_1EH, 06_1FH, 06_2EH
///////////////////////////////////////////////////////////////////////////////////////////

// GetNehalemFamilyMSRInfo_Static
//
//  Nehalem
//
void GetNehalemFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    if (!SysInfo_IsNehalemArhitecture(pCPUInfo))
        return;

    AsciiFPrint(DEBUG_FILE_HANDLE, "%a - Get MSR info", __FUNCTION__);

    GetMinMaxMultiplier(pCPUMSRInfo);

    PopulateCPUClock(pCPUMSRInfo, 13333);

    if (pCPUInfo->bDTS)
        GetTemperatureTarget(pCPUMSRInfo);

    // Intel Xeon Processor 5500 and 3400 series support additional model-specific registers
    // listed in Table B-6. These MSRs also apply to Intel Core i7 and i5 processor family
    // CPUID signature with DisplayFamily_DisplayModel of 06_1AH, 06_1EH and 06_1FH,
    // see Table B-1.
    if ((pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1E) ||
        (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1F))
    {
        // Get the max multipliers for 1C (1CPU running in turbo mode), 2C, 3C, 4C
        if (pCPUInfo->bIntelTurboBoost)
        {
            // TURBO_POWER_CURRENT_LIMIT (RO)	//BIT601000.0015b
            UINT64 ullMSRResult = AsmReadMsr64(0x1AC);
            AsciiSPrint(gBuffer, BUF_SIZE, "GetNehalemFamilyMSRInfo_Static - MSR[0x1AC] = %016lx", ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);

            pCPUMSRInfo->ullMSR0x1AC = ullMSRResult;
            // Overclocking Power and current limits (based on Email from Intel 26 Nov 2008)
            //  MSR 1ACh = TURBO_POWER_CURRENT_LIMIT (RO)
            //  How do you programmatically determine the TDP Power Limit TDP = MSR 1ACh[14:0] * 0.125 W
            //  How do you programmatically determine the TDC Current Limit TDC = MSR 1ACh[30:16] * 0.125 A
            pCPUMSRInfo->iTDPLimit = (int)(BitExtractULL(ullMSRResult, 14, 0) >> 3);  // Watts
            pCPUMSRInfo->iTDCLimit = (int)(BitExtractULL(ullMSRResult, 30, 16) >> 3); // Amps

            GetTurboMultipliers(pCPUMSRInfo);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// newer XEON
///////////////////////////////////////////////////////////////////////////////////////////

BOOLEAN IsIntelBurstSupported(CPUINFO *pCPUInfo) // BIT801017
{
    return (IsIntelTurboSupported(pCPUInfo) &&
            (SysInfo_IsSilvermont(pCPUInfo) ||
             SysInfo_IsAirmont(pCPUInfo))); // SI101100
}

// IsDisableIntelTurboSupported
//
//  Can we disable Turbo mode? Nehalem based Xeon chips get BSOD when disabling.
//
BOOLEAN IsDisableIntelTurboSupported(CPUINFO *pCPUInfo) // BIT6010170007
{
    if (IsIntelTurboSupported(pCPUInfo))
    {
        if (SysInfo_IsIvyBridgeOrLater(pCPUInfo)) // SI101054 - allow disabling turbo mode on Xeons/W2008 for Ivy Bridge and later, and for W2008 for these newer CPUs	//SI101053 removed
        {
            return TRUE;
        }

        if (StrStr(pCPUInfo->typestring, _T("Xeon")) == NULL &&
            StrStr(pCPUInfo->typestring, _T("XEON")) == NULL)
            return TRUE;
    }
    MtSupportDebugWriteLine("IsDisableIntelTurboSupported - not supported");
    return FALSE;
}

// IsIntelTurboSupported
//
//  The Intel turbo mode indicator is also used by Intel for the a Turbo like feature on later Core2 CPUs (e.g. T7600).
//  Ignore these, as we don't want to report this as Turbo.
//  Note: Must have collected the CPUINFO before calling this function
//
//
BOOLEAN IsIntelTurboSupported(CPUINFO *pCPUInfo)
{
    BOOLEAN bIntelTurboBoost = FALSE;
    if (pCPUInfo->MaxBasicInputValue >= 6)
    {
        UINT32 CPUInfo[4];
        SetMem(CPUInfo, sizeof(CPUInfo), 0);

        AsmCpuid(0x6, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

        AsciiSPrint(gBuffer, BUF_SIZE, "IsIntelTurboSupported - CPUID[0x6][EAX] = %08x", CPUInfo[0]);
        MtSupportDebugWriteLine(gBuffer);

        bIntelTurboBoost = (BitExtract(CPUInfo[0], 1, 1) == 1); // CPUID.06H:EAX[1:1], Intel Turbo Boost supported
    }

    return bIntelTurboBoost;
}

BOOLEAN DisableTurboMode(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    int iMSR = IA32_MISC_ENABLE;
    UINT32 dwBit = IDA_DISABLE;
    UINT64 ullMask = 0x0000004000000000ULL;
    UINT64 ullMSRResult = 0ULL, ullTemp = 0ULL;
    UINT64 ull_MSR = 0ULL, ull_MSR_new = 0ULL;
    BOOLEAN bRet = FALSE;

#if 0
    if (!IsDisableIntelTurboSupported(pCPUInfo))	//set on startup only
        return bRet;
#endif

    // Disable Turbo mode
    if ((StrStr(pCPUInfo->typestring, _T("Xeon")) != NULL || StrStr(pCPUInfo->typestring, _T("XEON")) != NULL) && pCPUInfo->Family == 0x06 && pCPUInfo->Model <= 0x3E) // On Xeon and W2008 use a different method see: http://software.intel.com/en-us/forums/topic/385319, as MSR_IA32_MISC_ENABLE[38] method can cause crashing
    {
        iMSR = IA32_PERF_CTL;
        dwBit = IDA_ENGAGE;
        ullMask = 0x0100000000ULL;
    }
    else
    {
        iMSR = IA32_MISC_ENABLE; // 0x1A0;
        dwBit = IDA_DISABLE;     // bit 38
        ullMask = 0x0000004000000000ULL;
    }

    ull_MSR = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "DisableTurboMode - MSR[%x] = %016lx", iMSR, ull_MSR);
    MtSupportDebugWriteLine(gBuffer);
    {
        BOOLEAN bTurboDisabled = BitExtractULL(ull_MSR, dwBit, dwBit) != 0; //(199|1A0:[38:38]) == 0 => ENABLED
        pCPUMSRInfo->ullMSR0x1A0 = ull_MSR;                                 // BIT6010240005

        if (!bTurboDisabled)
        {
            ull_MSR_new = (UINT64)ullMask | ull_MSR; // Set Turbo flag to 1 - disable

            AsmWriteMsr64(iMSR, ull_MSR_new);
            ullMSRResult = AsmReadMsr64(iMSR);

            AsciiSPrint(gBuffer, BUF_SIZE, "DisableTurboMode - New MSR[%x] = %016lx", iMSR, ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);
            {
                ullTemp = BitExtractULL(ullMSRResult, dwBit, dwBit);
                pCPUMSRInfo->ullMSR0x1A0 = ullMSRResult; // BIT6010240005
                bRet = TRUE;
            }
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Disable Turbo mode failed: already disabled: %016lx", ull_MSR);
            MtSupportDebugWriteLine(gBuffer);
        }
    }

    return bRet;
}

void EnableTurboMode(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    UINT64 ullMSRResult = 0ULL, ullTemp = 0ULL;
    UINT64 ull_MSR = 0ULL, ull_MSR_new = 0ULL;
    int iMSR = IA32_MISC_ENABLE;
    UINT32 dwBit = IDA_DISABLE;
    UINT64 ullMask = 0x0000004000000000ULL;

#if 0
    if (!IsDisableIntelTurboSupported(pCPUInfo))	//set on startup only
        return;
#endif

    // Disable Turbo mode
    if ((StrStr(pCPUInfo->typestring, _T("Xeon")) != NULL || StrStr(pCPUInfo->typestring, _T("XEON")) != NULL) && pCPUInfo->Family == 0x06 && pCPUInfo->Model <= 0x3E) // On Xeon and W2008 use a different method see: http://software.intel.com/en-us/forums/topic/385319, as MSR_IA32_MISC_ENABLE[38] method can cause crashing
    {
        iMSR = IA32_PERF_CTL;
        dwBit = IDA_ENGAGE;
        ullMask = 0xFFFFFFFEFFFFFFFFULL;
    }
    else
    {
        iMSR = IA32_MISC_ENABLE; // 0x1A0;
        dwBit = IDA_DISABLE;     // bit 38
        ullMask = 0xFFFFFFBFFFFFFFFFULL;
    }

    // Enable Turbo
    ull_MSR = AsmReadMsr64(iMSR);
    AsciiSPrint(gBuffer, BUF_SIZE, "EnableTurboMode - MSR[%x] = %016lx", iMSR, ull_MSR);
    MtSupportDebugWriteLine(gBuffer);
    {
        BOOLEAN bTurboDisabled = BitExtractULL(ull_MSR, dwBit, dwBit) != 0;
        if (bTurboDisabled)
        {
            ull_MSR_new = (UINT64)ullMask & ull_MSR; // Set Diable Turbo to 0 (i.e. enable turbo)

            AsmWriteMsr64(iMSR, ull_MSR_new);
            ullMSRResult = AsmReadMsr64(iMSR);

            AsciiSPrint(gBuffer, BUF_SIZE, "EnableTurboMode - New MSR[%x] = %016lx", iMSR, ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);
            ullTemp = BitExtractULL(ullMSRResult, dwBit, dwBit);
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Enable Turbo mode failed: already enabled: %016lx", ull_MSR);
            MtSupportDebugWriteLine(gBuffer);
        }
    }

    // Check to see whether the performance state is set to max ratio
    {
        ullMSRResult = AsmReadMsr64(IA32_PERF_STATUS);
        AsciiSPrint(gBuffer, BUF_SIZE, "EnableTurboMode - MSR[0x198] = %016lx (Current multiplier: %d)", ullMSRResult, (UINT32)BitExtract(ullMSRResult, 15, 8));
        MtSupportDebugWriteLine(gBuffer);

        ullMSRResult = AsmReadMsr64(IA32_PERF_CTL);
        AsciiSPrint(gBuffer, BUF_SIZE, "EnableTurboMode - MSR[0x199] = %016lx (Target multiplier: %d, Max1CTurbo: %d)", ullMSRResult, (UINT32)BitExtract(ullMSRResult, 15, 8), pCPUMSRInfo->iMaxTurbo1Core);
        MtSupportDebugWriteLine(gBuffer);

        if (BitExtract(ullMSRResult, 15, 8) < pCPUMSRInfo->iMaxTurbo1Core && pCPUMSRInfo->iMaxTurbo1Core < 80 /* sanity check of multiplier */) // Set the target multiplier as the max 1C turbo multiplier limit
        {
            ullMSRResult &= ~0xFF00ULL;
            ullMSRResult |= LShiftU64(pCPUMSRInfo->iMaxTurbo1Core & 0xFF, 8);
            if (SysInfo_IsSilvermont(pCPUInfo) != FALSE && (pCPUInfo->Model == 0x37 || pCPUInfo->Model == 0x4C)) // Set P-State Voltage Target (R/W) to MAX_VID_1C (Reference Doc: 514147)
            {
                UINT64 ullMSRBurstVIDS = AsmReadMsr64(0x66d);
                AsciiSPrint(gBuffer, BUF_SIZE, "EnableTurboMode - MSR[0x66d] = %016lx (MAX_VID_1C: %d)", ullMSRBurstVIDS, (UINT32)BitExtract(ullMSRBurstVIDS, 6, 0));
                MtSupportDebugWriteLine(gBuffer);

                ullMSRResult &= ~0x00FFULL;
                ullMSRResult |= BitExtractULL(ullMSRBurstVIDS, 6, 0);
            }
            AsciiSPrint(gBuffer, BUF_SIZE, "EnableTurboMode - Setting MSR[0x199] to %016lx", ullMSRResult);
            MtSupportDebugWriteLine(gBuffer);

            AsmWriteMsr64(IA32_PERF_CTL, ullMSRResult);

            ullMSRResult = AsmReadMsr64(IA32_PERF_CTL);
            AsciiSPrint(gBuffer, BUF_SIZE, "EnableTurboMode - New MSR[0x199] = %016lx (Target multiplier: %d)", ullMSRResult, (UINT32)BitExtract(ullMSRResult, 15, 8));
            MtSupportDebugWriteLine(gBuffer);
        }
    }
}

BOOLEAN IsIntelPerfMonSupported(CPUINFO *pCPUInfo)
{
    BOOLEAN bIA32_FIXED_CTR1_supported = FALSE;
    unsigned int CPUInfo[4];

    if (pCPUInfo->MaxBasicInputValue >= 0xA)
    {
        AsmCpuid(0xA, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

        AsciiSPrint(gBuffer, BUF_SIZE, "IsIntelPerfMonSupported - CPUID[0xA][EDX] = %08x", CPUInfo[3]);
        MtSupportDebugWriteLine(gBuffer);

        if (BitExtract(CPUInfo[3], 4, 0) > 2) // ECX[bit0] == 1, MSR 0xE8 is available //Hardware Coordination Feedback Capability (Presence of IA32_APERF, IA32_MPERF MSRs)
            bIA32_FIXED_CTR1_supported = TRUE;
    }
    return bIA32_FIXED_CTR1_supported;
}

BOOLEAN IsIntelFixedFunctionPerfCountersSupported(CPUINFO *pCPUInfo)
{
    return SysInfo_IsLunarLake(pCPUInfo) ||
           SysInfo_IsArrowLake(pCPUInfo) ||
           SysInfo_IsGraniteRapids(pCPUInfo) ||
           SysInfo_IsSierraForest(pCPUInfo) ||
           SysInfo_IsMeteorLake(pCPUInfo) ||
           SysInfo_IsEmeraldRapidsScalable(pCPUInfo) ||
           SysInfo_IsSapphireRapidsScalable(pCPUInfo) ||
           SysInfo_IsTremont(pCPUInfo) ||
           SysInfo_IsRaptorLake(pCPUInfo) ||
           SysInfo_IsIceLakeScalable(pCPUInfo) ||
           SysInfo_IsAlderLake(pCPUInfo) ||
           SysInfo_IsElkhartLake(pCPUInfo) ||
           SysInfo_IsRocketLake(pCPUInfo) ||
           SysInfo_IsTigerLake(pCPUInfo) ||
           SysInfo_IsIceLake(pCPUInfo) ||
           SysInfo_IsCometLake(pCPUInfo) ||
           SysInfo_IsKnightsMill(pCPUInfo) ||
           SysInfo_IsCannonLake(pCPUInfo) ||
           SysInfo_IsGoldmontPlus(pCPUInfo) ||
           SysInfo_IsKabyLake(pCPUInfo) ||
           SysInfo_IsApolloLake(pCPUInfo) ||
           SysInfo_IsXeonBroadwellE(pCPUInfo) || // SI101112
           SysInfo_IsGoldmont(pCPUInfo) ||
           SysInfo_IsSkylake(pCPUInfo) ||                   // SI101106
           SysInfo_IsKnightsLanding(pCPUInfo) ||            // SI101100
           SysInfo_IsNextGenerationXeonHaswell(pCPUInfo) || // SI101073
           SysInfo_IsBroadwell(pCPUInfo) ||                 // SI101073
           // SysInfo_IsSilvermont(pCPUInfo) ||	//SI101039		//SI101083 - Removed so as to use 2nd method for CPU freq measurement - as per older Atoms
           SysInfo_IsHaswell(pCPUInfo) ||     // SI101032
           SysInfo_IsIvyBridgeEP(pCPUInfo) || // SI101039
           SysInfo_IsIvyBridge(pCPUInfo) ||
           SysInfo_IsXeonE7(pCPUInfo) ||
           SysInfo_IsSandyBridge(pCPUInfo) ||
           SysInfo_IsWestmere(pCPUInfo) ||
           SysInfo_IsNehalemArhitecture(pCPUInfo);
}

///////////////////////////////////////////////////////////////////////////////////////////
// Corei7 MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// IsLaterThanIntelCorei7
BOOLEAN IsLaterThanIntelCorei7(CPUINFO *pCPUInfo) // BIT6010120007
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model >= 0x1a) // MSR's for: FamilyName_DisplayModel == 06_1aH or higher model
            return TRUE;
    }
    return FALSE;
}

// IsIntelFixedDDR3
//
//  Correct Windows for DDR3 for known systems that only support DDR3 //BIT6010120009
//
BOOLEAN IsIntelFixedDDR3(CPUINFO *pCPUInfo)
{
    if (_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0)
    {
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1a) // Core i7
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1e) // Core i7 (Dual DDR3)
            return TRUE;
        if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x2C) // Core i9
            return TRUE;
    }
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////
// ATOM MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// GetAtomFamilyMSRInfo_Dynamic
//
//
//  MSR's for: FamilyName_DisplayModel == 06_1CH
//
//	Intel Atom series
//
void GetAtomFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // BIT601000.0012
{
    UINT64 ullMSRResult = 0;

    // If not the atom family then exit
    if (!IsIntelAtom(pCPUInfo))
        return;

    // Scalable Bus Speed - this is the intended scalable bus speed - not the current (???) beleive no need to measure under load
    ullMSRResult = AsmReadMsr64(0xCD);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetAtomFamilyMSRInfo_Dynamic - MSR[0xCD] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0xCD = ullMSRResult;
        switch ((int)BitExtractULL(ullMSRResult, 2, 0))
        {
        case 5: // 101B 100MHz	(FSB 400)
            pCPUMSRInfo->flScalableBusSpeed = 100000;
            pCPUMSRInfo->flFSB = 400000;
            break;
        case 1: // 001B 133MHz	(FSB 533)
            pCPUMSRInfo->flScalableBusSpeed = 133333;
            pCPUMSRInfo->flFSB = 533333;
            break;
        case 3: // 011B 167MHz	(FSB 667)
            pCPUMSRInfo->flScalableBusSpeed = 166667;
            pCPUMSRInfo->flFSB = 666667;
            break;
        default:
            pCPUMSRInfo->flScalableBusSpeed = 0;
            pCPUMSRInfo->flFSB = 0;
            break;
        }
    }

    // Power on - incudes Integer bus frequency ratio - dynamic?
    ullMSRResult = AsmReadMsr64(0x2A);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetAtomFamilyMSRInfo_Dynamic - MSR[0x2A] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0x2A = ullMSRResult;
        pCPUMSRInfo->flBusFrequencyRatio = (int)BitExtractULL(ullMSRResult, 26, 22) * 100;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Core 2 MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// GetCore2FamilyMSRInfo_Dynamic
//
//
//  MSR's for: FamilyName_DisplayModel == 06_0FH
//
//	Intel Core 2 Duo processor
//	Intel Core 2 Duo mobile processor
//	Intel Core 2 Quad processor
//	Intel Core 2 Quad mobile processor
//	Intel Core 2 Extreme processor
//	Intel Pentium Dual-Core processor
//	Intel Xeon processor, model 0Fh
//
//	All processors are manufactured using the 65nm process
//
//
//  MSR's for: FamilyName_DisplayModel == 06_17H
//
//	Intel Core 2 Extreme processor
//	Intel Xeon processor, model 17h
//
//	All processors are manufactured using the 45nm process
//
int GetCore2FamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    UINT64 ullMSRResult = 0;
    int flTSCMultiplier = 0, flCurrentMultiplier = 0;
    int flCPUSpeed = 0;

    // If not the core 2 family then exit
    // if (!IsIntelCore2(pCPUInfo))
    if (!IsIntelCore2(pCPUInfo) && !(pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1D)) // SI101009
        return 0;

    // Scalable Bus Speed
    ullMSRResult = AsmReadMsr64(0xCD);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetCore2FamilyMSRInfo_Dynamic - MSR[0xCD] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0xCD = ullMSRResult;
        switch ((int)BitExtractULL(ullMSRResult, 2, 0))
        {
        case 5: // 101B 100MHz	(FSB 400)
            pCPUMSRInfo->flScalableBusSpeed = 100000;
            pCPUMSRInfo->flFSB = 400000;
            break;
        case 1: // 001B 133MHz	(FSB 533)
            pCPUMSRInfo->flScalableBusSpeed = 133333;
            pCPUMSRInfo->flFSB = 533333;
            break;
        case 3: // 011B 167MHz	(FSB 667)
            pCPUMSRInfo->flScalableBusSpeed = 166667;
            pCPUMSRInfo->flFSB = 666667;
            break;
        case 2: // 010B 200MHz	(FSB 800)
            pCPUMSRInfo->flScalableBusSpeed = 200000;
            pCPUMSRInfo->flFSB = 800000;
            break;
        case 0: // 000B 267MHz	(FSB 1067)
            // pCPUMSRInfo->flScalableBusSpeed = 267.67f ;
            // pCPUMSRInfo->flFSB = 1067 ;
            // pCPUMSRInfo->flScalableBusSpeed = 266.0f ;		//BIT601000.0008 - 318732.pdf
            pCPUMSRInfo->flScalableBusSpeed = 266667; // BIT601014 - for calc later
            pCPUMSRInfo->flFSB = 1066667;             // BIT601000.0008 - 318732.pdf
            break;
        case 4: // 100B 333MHz	(FSB 1333)
            // pCPUMSRInfo->flScalableBusSpeed = 333.33f ;
            // pCPUMSRInfo->flScalableBusSpeed = 333.0f ;		//BIT601000.0008 - 318732.pdf
            pCPUMSRInfo->flScalableBusSpeed = 333333; // BIT601014 - for calc later
            pCPUMSRInfo->flFSB = 1333333;
            break;
        case 6: // 110B 400MHz	(FSB 1600)
            pCPUMSRInfo->flScalableBusSpeed = 400000;
            pCPUMSRInfo->flFSB = 1600000;
            break;
        default:
            pCPUMSRInfo->flScalableBusSpeed = 0;
            pCPUMSRInfo->flFSB = 0;
            break;
        }
    }

    {
        // Get max multiplier to work out the actual scalable bus speed (rather than the inteneded/stock speed)	//BIT6010140003
        // UINT64	ullCurrentPerformanceStateValue, ullMaxBusRatio;
        ullMSRResult = AsmReadMsr64(0x198);
        {
            /*
            ullCurrentPerformanceStateValue = BitExtractULL(ullMSRResult, 15, 0);
            ullMaxBusRatio = BitExtractULL(ullMSRResult, 44, 40);
            bMaxBusRatioIsInteger = BitExtractULL(ullMSRResult, 46, 46);

            pCPUMSRInfo->flScalableBusSpeed = pCPUMSRInfo->flCPUTSC / ullMaxBusRatio;	//Base clock calculated from (max speed / max mult)///IR put back do for i7
            pCPUMSRInfo->flFSB = pCPUMSRInfo->flScalableBusSpeedStock * 4 ;
            */
        }
        ullMSRResult = AsmReadMsr64(0x199);
    }

    // Power on - incudes Integer bus frequency ratio
    ullMSRResult = AsmReadMsr64(0x2A);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetCore2FamilyMSRInfo_Dynamic - MSR[0x2A] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0x2A = ullMSRResult;
        pCPUMSRInfo->iNonIntegerBusRatio = (int)BitExtractULL(ullMSRResult, 18, 18);
        if (pCPUMSRInfo->iNonIntegerBusRatio == 0) //==0 means integer multiplier ratio
        {
            pCPUMSRInfo->flBusFrequencyRatio = (int)BitExtractULL(ullMSRResult, 26, 22) * 100;

            // OK, we have the CPU multiplier and the CPU speed, so we can calulculate the actual bus speed
            pCPUMSRInfo->flScalableBusSpeedStock = pCPUMSRInfo->flScalableBusSpeed;
            pCPUMSRInfo->flFSBStock = pCPUMSRInfo->flFSB;

            pCPUMSRInfo->flScalableBusSpeed = pCPUMSRInfo->raw_freq_cpu * 100 / pCPUMSRInfo->flBusFrequencyRatio;
            pCPUMSRInfo->flFSB = pCPUMSRInfo->flScalableBusSpeed * 4;
        }
        else if (pCPUMSRInfo->flScalableBusSpeed != 0)
        {
            pCPUMSRInfo->flBusFrequencyRatio = pCPUMSRInfo->raw_freq_cpu * 100 / pCPUMSRInfo->flScalableBusSpeed;
        }
    }

    // Improvement for caclulating the scalable base speed (esp. for overclcoked CPUs)	//BIT6010270004
    ullMSRResult = AsmReadMsr64(IA32_PERF_STATUS);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetCore2FamilyMSRInfo_Dynamic - MSR[0x198] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        // The maximum resolved bus ratio can be read from the following bit field:
        //  If XE operation is disabled, the maximum resolved bus ratio can be read in MSR_PLATFORM_ID[12:8]. It corresponds to the maximum qualified frequency.
        //  IF XE operation is enabled, the maximum resolved bus ratio is given in MSR_PERF_STAT[44:40], it corresponds to the maximum XE operation frequency configured by BIOS.
        // XE operation of an Intel 64 processor is implementation specific. XE operation can be enabled only by BIOS. If MSR_PERF_STAT[31] is set, XE operation is enabled. The
        //	MSR_PERF_STAT[31] field is read-only.

        unsigned int edx;
        unsigned int eax;
        // Get the Max Multipliter (see Vol 3B 30.10.5 XE Operation)
        edx = (unsigned int)BitExtractULL(ullMSRResult, 63, 32);
        flTSCMultiplier = 100 * ((edx >> 8) & 0x1F) + 50 * ((edx >> 14) & 1);

        AsciiSPrint(gBuffer, BUF_SIZE, "GetCore2FamilyMSRInfo_Dynamic - TSCMultiplier = %d", flTSCMultiplier);
        MtSupportDebugWriteLine(gBuffer);

        // Get the current multiplier
        eax = (unsigned int)BitExtractULL(ullMSRResult, 31, 0);
        flCurrentMultiplier = 100 * ((eax >> 8) & 0x1F) + 50 * ((eax >> 14) & 1);

        AsciiSPrint(gBuffer, BUF_SIZE, "GetCore2FamilyMSRInfo_Dynamic - CurrentMultiplier = %d", flCurrentMultiplier);
        MtSupportDebugWriteLine(gBuffer);

        if (flTSCMultiplier > 0 && flCurrentMultiplier > 0)
        {
            // Calc the Base freq based on the TSC freq and TSC Mult
            pCPUMSRInfo->flScalableBusSpeed = pCPUMSRInfo->flCPUTSC * 100 / flTSCMultiplier;
            pCPUMSRInfo->flFSB = pCPUMSRInfo->flScalableBusSpeed * 4;

            // Get the current multiplier
            pCPUMSRInfo->flBusFrequencyRatio = flCurrentMultiplier;

            // Current speed = Base Clk * Current multiplier
            flCPUSpeed = pCPUMSRInfo->flScalableBusSpeed * flCurrentMultiplier / 100;
            pCPUMSRInfo->raw_freq_cpu = flCPUSpeed;
        }
    }

    return flCPUSpeed;
}

///////////////////////////////////////////////////////////////////////////////////////////
// IsIntelCoreSoloDuo MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// GetCoreSoloDuoMSRInfo_Dynamic
//
//
//  MSR's for: FamilyName_DisplayModel == 06_0eH
//
//	Intel Core Duo/ Core Solo
//
void GetCoreSoloDuoMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // BIT601000.0012
{
    UINT64 ullMSRResult = 0;

    // If not the core 2 family then exit
    if (!IsIntelCoreSoloDuo(pCPUInfo))
        return;

    // Scalable Bus Speed
    ullMSRResult = AsmReadMsr64(0xCD);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetCoreSoloDuoMSRInfo_Dynamic - MSR[0xCD] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0xCD = ullMSRResult;
        switch ((int)BitExtractULL(ullMSRResult, 2, 0))
        {
        case 5: // 101B 100MHz	(FSB 400)
            pCPUMSRInfo->flScalableBusSpeed = 100000;
            pCPUMSRInfo->flFSB = 400000;
            break;
        case 1: // 001B 133MHz	(FSB 533)
            pCPUMSRInfo->flScalableBusSpeed = 133333;
            pCPUMSRInfo->flFSB = 533333;
            break;
        case 3: // 011B 167MHz	(FSB 667)
            pCPUMSRInfo->flScalableBusSpeed = 166667;
            pCPUMSRInfo->flFSB = 666667;
            break;
        default:
            pCPUMSRInfo->flScalableBusSpeed = 0;
            pCPUMSRInfo->flFSB = 0;
            break;
        }
    }

    // Power on - incudes Integer bus frequency ratio
    ullMSRResult = AsmReadMsr64(0x2A);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetCoreSoloDuoMSRInfo_Dynamic - MSR[0x2A] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    {
        pCPUMSRInfo->ullMSR0x2A = ullMSRResult;
        pCPUMSRInfo->flBusFrequencyRatio = (int)BitExtractULL(ullMSRResult, 26, 22) * 100;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////
// Netburst MSRs
///////////////////////////////////////////////////////////////////////////////////////////

// GetNetBurstFamilyMSRInfo_Dynamic
//
//  MSR's for: FamilyName == 0FH
//
//	Model 00h: Pentium 4, Xeon. 0.18 micron process
//	Model 01h: Pentium 4, Xeon, Xeon MP, Celeron. 0.18 micron process
//	Model 02h: Pentium 4, Mobile Intel Pentium 4, Xeon MP, Celeron, Mobile Celeron. 0.13 micron process
//	Model 03h: Pentium 4, Xeon, Celeron D. 90nm process
//	Model 04h: Pentium 4, Pentimum Extreme Edition, Pentium D, Xeon, Xeon MP, Celeron D. 90nm process
//	Model 06h: Pentium 4, Pentium D, Pentium Extreme Edition, Xeon, Xeon MP, Celeron D. 65nm process
//
void GetNetBurstFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo) // BIT601000.0008
{
    UINT64 ullMSRResult = 0;

    // Must be CPU Family 0FH
    if (!IsIntelNetBurst(pCPUInfo))
        return;

    // Processor Frequency Configuraiton (Scalable Bus Speed)
    if (pCPUInfo->Model == 0 || pCPUInfo->Model == 1 || pCPUInfo->Model == 2 || pCPUInfo->Model == 3 || pCPUInfo->Model == 4 || pCPUInfo->Model == 6)
    {
        ullMSRResult = AsmReadMsr64(0x2C);
        AsciiSPrint(gBuffer, BUF_SIZE, "GetNetBurstFamilyMSRInfo_Dynamic - MSR[0x2C] = %016lx", ullMSRResult);
        MtSupportDebugWriteLine(gBuffer);
        {

            if (pCPUInfo->Model == 2 || pCPUInfo->Model == 3 || pCPUInfo->Model == 4 || pCPUInfo->Model == 6)
            {
                pCPUMSRInfo->flBusFrequencyRatio = (int)BitExtractULL(ullMSRResult, 31, 24) * 100;

                pCPUMSRInfo->ullMSR0x2C = ullMSRResult;
                switch ((int)BitExtractULL(ullMSRResult, 18, 16))
                {
                case 0: // 000B
                    if (pCPUInfo->Model == 2)
                    {
                        pCPUMSRInfo->flScalableBusSpeed = 100000;
                        pCPUMSRInfo->flFSB = 400000;
                    }
                    else if (pCPUInfo->Model == 3 || pCPUInfo->Model == 4)
                    {
                        pCPUMSRInfo->flScalableBusSpeed = 266667;
                        pCPUMSRInfo->flFSB = 1066667;
                    }
                    break;
                case 1: // 001B
                    pCPUMSRInfo->flScalableBusSpeed = 133333;
                    pCPUMSRInfo->flFSB = 533333;
                    break;
                case 2: // 010B
                    pCPUMSRInfo->flScalableBusSpeed = 200000;
                    pCPUMSRInfo->flFSB = 800000;
                    break;
                case 3: // 011B
                    pCPUMSRInfo->flScalableBusSpeed = 166667;
                    pCPUMSRInfo->flFSB = 666667;
                    break;
                case 4: // 100B
                    if (pCPUInfo->Model == 6)
                    {
                        pCPUMSRInfo->flScalableBusSpeed = 333333;
                        pCPUMSRInfo->flFSB = 1333333;
                    }
                    break;
                default:
                    pCPUMSRInfo->flScalableBusSpeed = 0;
                    pCPUMSRInfo->flFSB = 0;
                    break;
                }
            }
            else if (pCPUInfo->Model == 0 || pCPUInfo->Model == 1)
            {
                pCPUMSRInfo->ullMSR0x2C = ullMSRResult;
                switch ((int)BitExtractULL(ullMSRResult, 23, 21))
                {
                case 0: // 000B
                    pCPUMSRInfo->flScalableBusSpeed = 100000;
                    pCPUMSRInfo->flFSB = 400000;
                    break;
                default:
                    pCPUMSRInfo->flScalableBusSpeed = 0;
                    pCPUMSRInfo->flFSB = 0;
                    break;
                }
            }
            //_stprintf (SysInfo_DebugString, _T("DEBUG PERF: MSR 0x2D - %I64X %f %f\n"),
            //	pCPUMSRInfo->iMSR0xCD, pCPUMSRInfo->flScalableBusSpeed, pCPUMSRInfo->flFSB);
            // SysInfo_DebugLogWriteLine (SysInfo_DebugString);
        }
    }
}

// GetIA32ArchitecturalTemp
//
//  Get CPU temperature from "Thermal Monitor Status" MSR 19CH (introduced in 0F_00H)
//  We will not support 0F_**H (Netburst), but only support from 06_0FH Core2 Duo and later
//
BOOLEAN GetIA32ArchitecturalTemp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    int iMSR = 0;
    UINT64 ullMSRResult = 0;
    BOOLEAN bRet = FALSE;

    // Check that the CPU supports monitoring CPU temp via MSR's
    if (!(pCPUInfo->ACPI))
    {
        MtSupportDebugWriteLine("GetIA32ArchitecturalTemp - ACPI not supported");
        return bRet;
    }

    if ((_tcscmp(_T("GenuineIntel"), pCPUInfo->manufacture) == 0) && pCPUInfo->Family == 0x06 && pCPUInfo->Model >= 0x0E) // BIT6010140000 - include newer model CPU temperature collection
    {
        iMSR = 0x19C;
        ullMSRResult = AsmReadMsr64(iMSR);

        CPUTemp->bDataCollected = TRUE;
        CPUTemp->ullMSR0x19C = ullMSRResult;

        CPUTemp->iPROCHOT1 = (ullMSRResult >> 2 & 0x01);                    // (int) BitExtractULL(ullMSRResult, 2, 2) ;						//BOOLEAN - digital output pin indicating the internal thermal control circuit has activated. This is for Fan control.
        CPUTemp->iPROCHOT2 = (ullMSRResult >> 3 & 0x01);                    // BOOLEAN - digital output pin indicating the internal thermal control circuit has activated. This is for Fan control.
        CPUTemp->iDigitalReadout = (ullMSRResult >> 16 & 0x7F);             // (int) BitExtractULL(ullMSRResult, 22, 16) ;				//Temp (celcicius) = TCC-pCPUMSRInfo->iDigitalReadout, where TCC is the critical temp where the CPU shuts down - see CPU datasheet for value
        CPUTemp->iResolutionInDegreesCelcius = (ullMSRResult >> 27 & 0x0F); // (int) BitExtractULL(ullMSRResult, 30, 27) ;	//Resolution or toerance
        CPUTemp->iReadingValid = (ullMSRResult >> 31 & 0x01);               // (int) BitExtractULL(ullMSRResult, 31, 31) ;

        // Now Tjunction can be a range of values, such as 85 or 100 depending on the chip and this is not puplically docuemtned by Intel,
        // so lets test and find out
        // http://www.hardforum.com/showthread.php?t=1192840
        // http://www.bugtrack.almico.com/view.php?id=664
        if (CPUTemp->iReadingValid)
            bRet = MapTempIntel(pCPUInfo, CPUTemp);
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "GetIA32ArchitecturalTemp - CPU temperature reading invalid (MSR[0x19C] = %16lx, Vendor ID: %s %x %x %x)", ullMSRResult, pCPUInfo->manufacture, pCPUInfo->Family, pCPUInfo->Model, pCPUInfo->stepping);
            MtSupportDebugWriteLine(gBuffer);
        }

        /*
        iMSR = 0x19B;
        if (DeviceIoControl(g_hDirectIO_BIT, IOCTL_DIRECTIO_READMSR, &iMSR, sizeof(iMSR), &ullMSRResult, sizeof(ullMSRResult), &dwBytesReturned, NULL))
        {
            iT1 = (int) (BitExtractULL(ullMSRResult, 14, 8));
            iT2 = (int) (BitExtractULL(ullMSRResult, 22, 16));


            //Now Tjunction can be a range of values, such as 85 or 100 depending on the chip and this is not puplically docuemtned by Intel,
            //so lets test and find out
            //http://www.hardforum.com/showthread.php?t=1192840
            //http://www.bugtrack.almico.com/view.php?id=664
            if (CPUTemp->iReadingValid)
            MapTempIntel(szCPU, CPUTemp);
            }
            */
    }
    else
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "GetIA32ArchitecturalTemp - Intel CPU not supported (Vendor ID: %s %x %x %x)", pCPUInfo->manufacture, pCPUInfo->Family, pCPUInfo->Model, pCPUInfo->stepping);
        MtSupportDebugWriteLine(gBuffer);
    }

    return bRet;
}

// MapTempIntel
//
//  Take the digital temp reading from the TJunction (either from the CPU MSR iTemperatureTarget, or from our lookup table)
BOOLEAN MapTempIntel(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp)
{
    BOOLEAN bRet = FALSE;
    UINT64 ullMSR0x1A2 = 0;
    int iTemperatureTarget = 0;

    if (pCPUInfo->bDTS)
    {
        ullMSR0x1A2 = AsmReadMsr64(0x1A2);

        iTemperatureTarget = (ullMSR0x1A2 >> 16) & 0xFF; // (int) (BitExtractULL(ullMSRResult, 23, 16));	//Minimum temperature at which PROCHOT# will be asserted, in Celcius
    }

    CPUTemp->iTemperature = 0;
    if (IsLaterThanIntelCorei7(pCPUInfo))
    {
        // For iCore7 use CPU provided TjTarget value
        if (iTemperatureTarget > 0)
        {
            CPUTemp->iTemperature = iTemperatureTarget - CPUTemp->iDigitalReadout;
            if (CPUTemp->iTemperature < 0)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "MapTempIntel - TjTarget=%d DigitalReadout=%d)", iTemperatureTarget, CPUTemp->iDigitalReadout);
                MtSupportDebugWriteLine(gBuffer);
            }
            // if (CPUTemp->iTemperature > 0 && CPUTemp->iTemperature < 200)		//sanity
            if (ABS(CPUTemp->iTemperature) < 200) // sanity
                bRet = TRUE;
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "MapTempIntel - Invalid TjTarget (MSR[0x1A2] = %16lx)", ullMSR0x1A2);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    else
    {
        int iTJunction = 0;

        // For Core 2 and Atom use CPU specification table TjTarget value
        if (GetCPUTempJunction(pCPUInfo, &iTJunction) && iTJunction > 0)
        {
            CPUTemp->iTemperature = iTJunction - CPUTemp->iDigitalReadout;
            // if (CPUTemp->iTemperature > 0 && CPUTemp->iTemperature < 200)		//sanity
            if (CPUTemp->iTemperature > MIN_TEMP_SANITY && CPUTemp->iTemperature < 200) // sanity
                bRet = TRUE;
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "MapTempIntel - Invalid TJunction (%d)", iTJunction);
            MtSupportDebugWriteLine(gBuffer);
        }
    }

    return bRet;
}

// GetCPUSpecification()
//
//  Get CPU speicification items like the fabrication, socket, codename and TJMaximum
//
BOOLEAN GetCPUTempJunction(CPUINFO *pCPUInfo, int *pTJunction) // BIT601000.0015
{
    BOOLEAN bRet = FALSE;
    BOOLEAN bPartial = FALSE;
    int i = 0;

    /*//Debugging
    wcscpy(pCPUInfo->typestring,L"Intel(R) Core(TM)2 Duo CPU E8400 @ 3.00GHz");
    pCPUInfo->Family = 6;
    pCPUInfo->Model = 23;
    pCPUInfo->stepping = 6;
    */

    // Lookup the CPU specification CPU type string, type, family, model and stepping
    i = 0;
    while (CPU_Specificaton[i].szCPU[0] != _T('\0'))
    {
        if (CPU_Specificaton[i].iType == 0 &&
            CPU_Specificaton[i].iFamily == pCPUInfo->Family &&
            CPU_Specificaton[i].iModel == pCPUInfo->Model &&
            StrStr(pCPUInfo->typestring, CPU_Specificaton[i].szCPU))
        {
            if (CPU_Specificaton[i].iStepping == pCPUInfo->stepping)
            {
                *pTJunction = CPU_Specificaton[i].iTJunction; // iTJunction moved in structure as was not populated SI101014 - correction for support of Intel Core 2 CPU temperature
                bRet = TRUE;
                break;
            }
            else
            {
                *pTJunction = CPU_Specificaton[i].iTJunction;
                bPartial = TRUE;
            }
        }
        i++;
    }

    // If we didn't find it, then lookup the CPU specification CPU type string, type, family and model, but not stepping
    if (!bRet)
    {
        if (bPartial)
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

// Intel 64 and IA-32 Architectures Software Developer's Manual
// Volume 4: Model-Specific Registers
void GetMinMaxMultiplier(CPUMSRINFO *CPUMSRinfo)
{
    // Get the min and max multipliers
    UINT64 ullMSRResult = AsmReadMsr64(0xCE);
    AsciiSPrint(gBuffer, BUF_SIZE, "MSR[0xCE] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);

    CPUMSRinfo->ullMSR0xCE = ullMSRResult;
    CPUMSRinfo->iMinimumRatio = (int)(BitExtractULL(ullMSRResult, 47, 40)); //  Maximum Efficiency Ratio (R/O) This is the minimum ratio (maximum efficiency) that the processor can operate, in units of 100MHz.
    CPUMSRinfo->iMaximumRatio = (int)(BitExtractULL(ullMSRResult, 15, 8));  //  Maximum Non-Turbo Ratio (R/O) This is the ratio of the frequency that invariant TSC runs at. Frequency = ratio * 100 MHz.
}

void PopulateCPUClock(CPUMSRINFO *cpuMSRinfo, int flCPUExternalClockStock)
{
    // Nehalem Family Processors use a base clock for CPU Clocking (No front side bus architecture, no direct link between CPU frequency and the QPI link frequency).
    cpuMSRinfo->flCPUExternalClockStock = flCPUExternalClockStock;                                                        // Fixes stock value
    cpuMSRinfo->flExternalClock = cpuMSRinfo->iMaximumRatio != 0 ? cpuMSRinfo->flCPUTSC / cpuMSRinfo->iMaximumRatio : -1; // Base clock calculated from (max speed / max mult)

    // Sanity check actual base clock
    if (cpuMSRinfo->flExternalClock < 60000 || cpuMSRinfo->flExternalClock > 300000)
        cpuMSRinfo->flExternalClock = cpuMSRinfo->flCPUExternalClockStock;

    cpuMSRinfo->flCPUExternalClockTurbo = cpuMSRinfo->flExternalClock;
}

void GetCPUThermalAndPowerLeaf(CPUINFO *cpuinfo)
{
    UINT32 CPUInfo[4];
    SetMem(CPUInfo, sizeof(CPUInfo), 0);
    AsmCpuid(0x6, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);
    cpuinfo->bDTS = (BitExtract(CPUInfo[0], 0, 0) == 1);             // CPUID.06H:EAX[0:0], Digital temperature sensor supported
    cpuinfo->bIntelTurboBoost = (BitExtract(CPUInfo[0], 1, 1) == 1); // CPUID.06H:EAX[1:1], Intel Turbo Boost supported
}

void GetTemperatureTarget(CPUMSRINFO *cpuMSRinfo)
{
    UINT64 ullMSRResult = AsmReadMsr64(0x1A2);
    AsciiSPrint(gBuffer, BUF_SIZE, "MSR[0x1A2] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    cpuMSRinfo->ullMSR0x1A2 = ullMSRResult;
    cpuMSRinfo->iTemperatureTarget = (int)(BitExtractULL(ullMSRResult, 23, 16)); // Minimum temperature at which PROCHOT# will be asserted, in Celcius
}

void GetTurboMultipliers(CPUMSRINFO *cpuMSRinfo)
{
    UINT64 ullMSRResult = AsmReadMsr64(0x1AD);
    AsciiSPrint(gBuffer, BUF_SIZE, "MSR[0x1AD] = %016lx", ullMSRResult);
    MtSupportDebugWriteLine(gBuffer);
    cpuMSRinfo->ullMSR0x1AD = ullMSRResult;
    cpuMSRinfo->iMaxTurbo1Core = (int)(BitExtractULL(ullMSRResult, 7, 0));
    cpuMSRinfo->iMaxTurbo2Core = (int)(BitExtractULL(ullMSRResult, 15, 8));
    cpuMSRinfo->iMaxTurbo3Core = (int)(BitExtractULL(ullMSRResult, 23, 16));
    cpuMSRinfo->iMaxTurbo4Core = (int)(BitExtractULL(ullMSRResult, 31, 24));
    cpuMSRinfo->iMaxTurbo5Core = (int)(BitExtractULL(ullMSRResult, 39, 32));
    cpuMSRinfo->iMaxTurbo6Core = (int)(BitExtractULL(ullMSRResult, 47, 40));
    cpuMSRinfo->iMaxTurbo7Core = (int)(BitExtractULL(ullMSRResult, 55, 48));
    cpuMSRinfo->iMaxTurbo8Core = (int)(BitExtractULL(ullMSRResult, 63, 56));
}

void GetIntelArchitectureSpecificMSRInfo(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo)
{
    AsciiFPrint(DEBUG_FILE_HANDLE, "GetIntelArchitectureSpecificMSRInfo - CPU %s:\n"
                                   "  Family:   0x%02X\n"
                                   "  Model:    0x%02X\n"
                                   "  Stepping: 0x%X",
                pCPUInfo->typestring, pCPUInfo->Family, pCPUInfo->Model, pCPUInfo->stepping);

    // Nehalem/Westmere architectures
    // Table B-5. MSRs in Processors Based on Intel Microarchitecture (Contd.)codename Nehalem: 06_1AH, 06_1EH, 06_1FH, 06_2EH
    // Table B-8. Additional MSRs supported by Next Generation Intel Processors (Intel microarchitecture codename Westmere): 06_25H and 06_2CH
    if (IsIntelFixedFunctionPerfCountersSupported(pCPUInfo))
    {
        int flMaxRatio = 0;
        int iMaxRatio;
        BOOLEAN bTurboDisableSupported = FALSE;
        BOOLEAN bTurboDisabled = FALSE;

        MtSupportDebugWriteLine("GetIntelArchitectureSpecificMSRInfo - Nehalem et al detected");

        // Get static CPU info, such as the max CPU Multiplier
        if (SysInfo_IsLunarLake(pCPUInfo))
            GetLunarLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsArrowLake(pCPUInfo))
            GetArrowLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsGraniteRapids(pCPUInfo))
            GetGraniteRapidsFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsSierraForest(pCPUInfo))
            GetSierraForestFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsMeteorLake(pCPUInfo))
            GetMeteorLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsEmeraldRapidsScalable(pCPUInfo))
            GetEmeraldRapidsScalableFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsSapphireRapidsScalable(pCPUInfo))
            GetSapphireRapidsScalableFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsTremont(pCPUInfo))
            GetTremontFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsRaptorLake(pCPUInfo))
            GetRaptorLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsIceLakeScalable(pCPUInfo))
            GetIceLakeScalableFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsAlderLake(pCPUInfo))
            GetAlderLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsElkhartLake(pCPUInfo))
            GetElkhartLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsRocketLake(pCPUInfo))
            GetRocketLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsTigerLake(pCPUInfo))
            GetTigerLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsIceLake(pCPUInfo))
            GetIceLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsCometLake(pCPUInfo))
            GetCometLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsKnightsMill(pCPUInfo))
            GetKnightsMillFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsCannonLake(pCPUInfo))
            GetCannonLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsKabyLake(pCPUInfo))
            GetKabyLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsGeminiLake(pCPUInfo))
            GetGeminiLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsApolloLake(pCPUInfo)) // SI101112
            GetApolloLakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsXeonBroadwellE(pCPUInfo)) // SI101112
            GetXeonBroadwellEFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsSkylake(pCPUInfo)) // SI101100
            GetSkylakeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsKnightsLanding(pCPUInfo)) // SI101100
            GetKnightsLandingFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsNextGenerationXeonHaswell(pCPUInfo))
            GetNextGenerationXeonHaswellFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsBroadwell(pCPUInfo))
            GetBroadwellFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        // else if (SysInfo_IsSilvermont(pCPUInfo))
        //	GetSilvermontFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsHaswell(pCPUInfo))
            GetHaswellFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsIvyBridgeEP(pCPUInfo))
            GetIvyBridgeEPFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsIvyBridge(pCPUInfo))
            GetIvyBridgeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsXeonE7(pCPUInfo))
            GetXeonE7FamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsSandyBridge(pCPUInfo))
            GetSandyBridgeFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsWestmere(pCPUInfo))
            GetWestmereFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsNehalemArhitecture(pCPUInfo))
            GetNehalemFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else
        {
        }

        // Measure CPU dynamic attributes: speed, mutliplier with Turbo mode disabled
        bTurboDisableSupported = IsDisableIntelTurboSupported(pCPUInfo);
        if (bTurboDisableSupported)
            bTurboDisabled = DisableTurboMode(pCPUInfo, pCPUMSRInfo);

        if (bTurboDisabled || !IsDisableIntelTurboSupported(pCPUInfo)) // BIT6010240005 - only measure the non turbo CPU speed if the CPU speed
        {
            // Get the current CPU ratio
            int flTargetRatio = 100 - 3; // Target ratio of almost 1
            int flMult;

            MtSupportDebugWriteLine("GetIntelArchitectureSpecificMSRInfo - Turbo disabled");

            if (IsIntelPerfMonSupported(pCPUInfo))
                flMaxRatio = GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED(pCPUInfo, TRUE, flTargetRatio);
            else
            {
                int flMaxCPUFreq = GetIntelCpuMaxSpeed_IA32_APERF(pCPUInfo, TRUE, pCPUMSRInfo);
                flMaxRatio = flMaxCPUFreq * 100 / pCPUMSRInfo->flCPUTSC;
            }

            flMaxRatio = GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED(pCPUInfo, TRUE, flTargetRatio);

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - MaxRatio = %d", flMaxRatio);
            MtSupportDebugWriteLine(gBuffer);

            // Determine the multiplier. Round, assumptions: integer multiplier & should be max freq
            flMult = pCPUMSRInfo->iMaximumRatio * flMaxRatio;

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - Mult = %d", flMult);
            MtSupportDebugWriteLine(gBuffer);

            if (ABS(flMult - pCPUMSRInfo->iMaximumRatio * 100) < 60)
                flMult = pCPUMSRInfo->iMaximumRatio * 100; // Try to match to the max muliplier - this is what it should be
            else
                flMult = flMult + 80; // If mult 0.2 above integer, round up. Is ave for period, can't be lower.

            pCPUMSRInfo->flBusFrequencyRatio = flMult; // Improve Muliplier (base clocking rounding errors possible) //BIT601022

            // Caclulate the CPU freqency (Base clock * Multiplier * CPU ratio)
            pCPUMSRInfo->raw_freq_cpu = pCPUMSRInfo->flExternalClock * pCPUMSRInfo->flBusFrequencyRatio / 100;

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - raw_freq_cpu = %d", pCPUMSRInfo->raw_freq_cpu);
            MtSupportDebugWriteLine(gBuffer);

            // Sanity check freq
            if (pCPUMSRInfo->raw_freq_cpu * 100 < (60 * pCPUMSRInfo->flCPUTSC))
                pCPUMSRInfo->raw_freq_cpu = pCPUMSRInfo->flCPUTSC;

            if (bTurboDisableSupported)
                EnableTurboMode(pCPUInfo, pCPUMSRInfo);
        }
        else
        {
            // If we failed to disable Turbo mode (as appears to be the case for many of the Mobile Core i5/i7's) then use the stock speed	//BIT6010240005
            pCPUMSRInfo->raw_freq_cpu = pCPUMSRInfo->flCPUTSC;
        }

        //
        // Measure CPU speed with Turbo mode enabled
        //
        if (bTurboDisableSupported)
        {
            // Set the target ratio for a 1 core maximum turbo, if less than this value, the activity duration will be higher during CPU ratio measurement
            int flTargetRatio = 0;
            int flMaxTurboRatio = 0;
            int flTurboMult = 0;

            if (pCPUMSRInfo->iMaximumRatio > 0)
                flTargetRatio = (pCPUMSRInfo->iMaxTurbo1Core * 100 / pCPUMSRInfo->iMaximumRatio) - 3; // Note 0.03 related to 0.9 below
            if (flTargetRatio < 97)
                flTargetRatio = 97; // Target ratio of almost 1

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - TargetRatio = %d", flTargetRatio);
            MtSupportDebugWriteLine(gBuffer);

            // Get the CPU ratio under load
            if (IsIntelPerfMonSupported(pCPUInfo))
                flMaxTurboRatio = GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED(pCPUInfo, FALSE, flTargetRatio);
            else
            {
                int flMaxCPUFreq = GetIntelCpuMaxSpeed_IA32_APERF(pCPUInfo, FALSE, pCPUMSRInfo);
                flMaxTurboRatio = flMaxCPUFreq * 100 / pCPUMSRInfo->flCPUTSC;
            }

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - MaxTurboRatio = %d", flMaxTurboRatio);
            MtSupportDebugWriteLine(gBuffer);

            // Determine the multiplier. Round, assumptions: integer multiplier & should be max freq
            flTurboMult = pCPUMSRInfo->iMaximumRatio * flMaxTurboRatio;

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - TurboMult = %d", flTurboMult);
            MtSupportDebugWriteLine(gBuffer);

            if (flTurboMult > (pCPUMSRInfo->iMaxTurbo1Core * 100 - 90) && flTurboMult < (pCPUMSRInfo->iMaxTurbo1Core * 100 + 50))
                flTurboMult = pCPUMSRInfo->iMaxTurbo1Core * 100;
            else if (flTurboMult > (pCPUMSRInfo->iMaxTurbo2Core * 100 - 90) && flTurboMult < (pCPUMSRInfo->iMaxTurbo2Core * 100 + 50))
                flTurboMult = pCPUMSRInfo->iMaxTurbo2Core * 100;
            else if (flTurboMult > (pCPUMSRInfo->iMaxTurbo3Core * 100 - 90) && flTurboMult < (pCPUMSRInfo->iMaxTurbo3Core * 100 + 50))
                flTurboMult = pCPUMSRInfo->iMaxTurbo3Core * 100;
            else if (flTurboMult > (pCPUMSRInfo->iMaxTurbo4Core * 100 - 90) && flTurboMult < (pCPUMSRInfo->iMaxTurbo4Core * 100 + 50))
                flTurboMult = pCPUMSRInfo->iMaxTurbo4Core * 100;
            else
                flTurboMult = flTurboMult + 80; // If mult 0.2 above integer, round up. Is ave for period, can't be lower.

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - new TurboMult = %d", flTurboMult);
            MtSupportDebugWriteLine(gBuffer);

            pCPUMSRInfo->flCPUMultTurbo = flTurboMult; // Improve Muliplier (base clocking rounding errors possible) //BIT601022

            pCPUMSRInfo->flCPUSpeedTurbo = pCPUMSRInfo->flExternalClock * pCPUMSRInfo->flCPUMultTurbo / 100;

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - CPUSpeedTurbo = %d", pCPUMSRInfo->flCPUSpeedTurbo);
            MtSupportDebugWriteLine(gBuffer);

            // Calculate the maximum theorectical CPU speed based on the higest power state multiplier and the base clock //SI101022c
            pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax = pCPUMSRInfo->flExternalClock * pCPUMSRInfo->iMaxTurbo1Core;

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - CPUSpeedTurboTheoreticalMax = %d", pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax);
            MtSupportDebugWriteLine(gBuffer);

            // Sanity check freq
            if (pCPUMSRInfo->flCPUSpeedTurbo * 100 < (60 * pCPUMSRInfo->flCPUTSC))
                pCPUMSRInfo->flCPUSpeedTurbo = pCPUMSRInfo->flCPUTSC;

            // Sanity check theorectical max		//SI101022c
            if (pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax > CPUSPEEDMAX)
                pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax = pCPUMSRInfo->flCPUExternalClockStock * pCPUMSRInfo->iMaxTurbo1Core;
            if (pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax < pCPUMSRInfo->flCPUSpeedTurbo)
                pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax = pCPUMSRInfo->flCPUSpeedTurbo;
        }

        //
        // Set Overclock flags
        //
        // If the base clock is more than 5% greater than the stock base clock, then set overclocked flag. assume base clk of 133.33 (Nehalem/Westmere OK)
        pCPUMSRInfo->OCLKBaseClock = CPU_OVERCLOCKED_UNKNOWN;
        if (pCPUMSRInfo->flExternalClock * 100 > (133333 * 105))
            pCPUMSRInfo->OCLKBaseClock = CPU_OVERCLOCKED;
        else if (pCPUMSRInfo->flExternalClock * 100 < (133333 * 95)) // BIT6010140005 - bug
            pCPUMSRInfo->OCLKBaseClock = CPU_UNDERCLOCKED;
        else
            pCPUMSRInfo->OCLKBaseClock = CPU_OVERCLOCKED_NO;

        // If the base clock is at least 1 more than maximum then the stock base clock, then set overclocked flag. Assume Multipliers are integer (Nehalem/Westmere OK).
        // Overclock based on non-turbo overclock, as this is what will increase the CPU trhoughput the most (e.g. for all cores loaded in PT CPU test)
        // The max Turbo multipliers could potentially also be increased, but for for now we won't consider this overclocking
        pCPUMSRInfo->OCLKMultiplier = CPU_OVERCLOCKED_UNKNOWN;
        // BIT6010240005 - Mobile chips diable turbo, but some don't seem to use the turbo multipliers (BIOS bug???) - so use max multiplier
        iMaxRatio = MAX(pCPUMSRInfo->iMaximumRatio, pCPUMSRInfo->iMaxTurbo1Core);
        if (pCPUMSRInfo->flBusFrequencyRatio > iMaxRatio * 100 + 90) // Use .9 (instead of 1) for round errors
            pCPUMSRInfo->OCLKMultiplier = CPU_OVERCLOCKED;
        else
            pCPUMSRInfo->OCLKMultiplier = CPU_OVERCLOCKED_NO;
    }
    /*
    //Burst Frequency
    // Silvermont
    else if (SysInfo_IsSilvermont(pCPUInfo, pCPUInfo->Family, pCPUInfo->Model))
    {
        SysInfo_DebugLogWriteLine(_T("DEBUG: GetMSRData Burst frequency"));

        GetSilvermontFamilyMSRInfo_Static(pCPUInfo, cpunumber, pCPUInfo->Family, pCPUInfo->Model);

        bool bTurboDisabled = false;
        //Measure Base freq
        if (IsDisableIntelTurboSupported(pCPUInfo))
            bTurboDisabled = DisableTurboMode(pCPUInfo, cpunumber);

        //if (bTurboDisabled)	//Measure the speed in any case
        {
            if (IsDebugLog())
                DebugLog(L"Measure CPU frequency with Turbo mode disabled");

            float flMaxCPUFreq = GetIntelCpuMaxSpeed_IA32_APERF(pCPUInfo, cpunumber, true, flCPUFreq_RDTSC);
            g_cpuMSRinfo[cpunumber].raw_freq_cpu  = flMaxCPUFreq;
            if (g_cpuMSRinfo[cpunumber].flExternalClock > 0)
            {
                g_cpuMSRinfo[cpunumber].flBusFrequencyRatio = g_cpuMSRinfo[cpunumber].raw_freq_cpu / g_cpuMSRinfo[cpunumber].flExternalClock;	//Improve Muliplier (base clocking rounding errors possible) //BIT601022
                g_cpuMSRinfo[cpunumber].flBusFrequencyRatio = (float) Round(g_cpuMSRinfo[cpunumber].flBusFrequencyRatio, 1);	//Round to 1 decimal places
            }

            if (bTurboDisabled && IsDisableIntelTurboSupported(pCPUInfo))
                EnableTurboMode(pCPUInfo);
        }

        //Measure Burst freq
        if (IsDisableIntelTurboSupported(pCPUInfo))
        {
            if (IsDebugLog())
                DebugLog(L"Measure CPU frequency with Turbo mode enabled");

            float flMaxCPUFreq = GetIntelCpuMaxSpeed_IA32_APERF(pCPUInfo, cpunumber, true, flCPUFreq_RDTSC);
            g_cpuMSRinfo[cpunumber].flCPUSpeedTurbo  = flMaxCPUFreq;
        }

        //Some debug logging
        if (IsDebugLog())
        {
            _stprintf_s(SysInfo_DebugString, _T("Turbo cpuspeed [a] %d: %0.1fMHz (%0.1fMHz x %0.1f) [%d, %d])"),
                cpunumber,
                g_cpuMSRinfo[cpunumber].flCPUSpeedTurbo,
                g_cpuMSRinfo[cpunumber].flExternalClock,
                g_cpuMSRinfo[cpunumber].flCPUExternalClockTurbo,
                g_cpuMSRinfo[cpunumber].iMaximumRatio,
                g_cpuMSRinfo[cpunumber].iMaxTurbo1Core);
            DebugLog(SysInfo_DebugString);
        }

    }
    */
    else // Use non-turbo CPU mechanism
    {
        // Table B-3. MSRs in Processors Based on Intel Core Microarchitecture: 06_0FH, 06_17H
        // Table B-4. MSRs in Intel Atom Processor Family: 06_0CH
        // Table B-9. MSRs in the Pentium 4 and Intel Xeon Processors
        // Table B-12. MSRs in Intel Core Solo, Intel Core Duo Processors, and Dual-Core Intel Xeon Processor LV
        // Table B-13. MSRs in Pentium M Processors
        // Table B-15. MSRs in the Pentium Processor
        int flMaxCPUFreq;

        BOOLEAN bTurboSupported = FALSE;
        BOOLEAN bTurboDisabled = FALSE;

        if (SysInfo_IsAirmont(pCPUInfo))
            GetAirmontFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);
        else if (SysInfo_IsSilvermont(pCPUInfo))
            GetSilvermontFamilyMSRInfo_Static(pCPUInfo, pCPUMSRInfo);

        bTurboSupported = IsDisableIntelTurboSupported(pCPUInfo);

        // Measure CPU dynamic attributes: speed, mutliplier with Turbo mode disabled
        if (bTurboSupported)
            bTurboDisabled = DisableTurboMode(pCPUInfo, pCPUMSRInfo);

        flMaxCPUFreq = GetIntelCpuMaxSpeed_IA32_APERF(pCPUInfo, TRUE, pCPUMSRInfo);

        AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - MaxCPUFreq = %d", flMaxCPUFreq);
        MtSupportDebugWriteLine(gBuffer);

        pCPUMSRInfo->raw_freq_cpu = flMaxCPUFreq;

        if (pCPUMSRInfo->flExternalClock > 0)
        {
            pCPUMSRInfo->flBusFrequencyRatio = pCPUMSRInfo->raw_freq_cpu * 100 / pCPUMSRInfo->flExternalClock; // Improve Muliplier (base clocking rounding errors possible) //BIT601022

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - BusFrequencyRatio = %d", pCPUMSRInfo->flBusFrequencyRatio);
            MtSupportDebugWriteLine(gBuffer);
        }

        // Sanity check freq
        if (pCPUMSRInfo->raw_freq_cpu * 100 < (60 * pCPUMSRInfo->flCPUTSC))
            pCPUMSRInfo->raw_freq_cpu = pCPUMSRInfo->flCPUTSC;

        if (bTurboSupported && bTurboDisabled) // BIT6010240005 - only measure the non turbo CPU speed if the CPU speed
        {
            EnableTurboMode(pCPUInfo, pCPUMSRInfo);
        }

        if (bTurboSupported)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - Measure CPU frequency with Turbo enabled");
            MtSupportDebugWriteLine(gBuffer);

            flMaxCPUFreq = GetIntelCpuMaxSpeed_IA32_APERF(pCPUInfo, FALSE, pCPUMSRInfo);
            pCPUMSRInfo->flCPUSpeedTurbo = flMaxCPUFreq;
        }

        //
        // Set Overclock flags
        //
        // if (g_cpuMSRinfo[cpunumber].raw_freq_cpu > (flCPUFreq_RDTSC * 1.1f))
#if 0
        if (pCPUMSRInfo->raw_freq_cpu * 100 > (pCPUMSRInfo->flCPUExternalClockStock * pCPUMSRInfo->flCPUMultStock * 105))	//SI101009
            pCPUMSRInfo->OCLKFreq = CPU_OVERCLOCKED;
        else
#endif
        pCPUMSRInfo->OCLKFreq = CPU_OVERCLOCKED_UNKNOWN;
    }

    // Not implemented:
    // Table B-6. Additional MSRs in Intel Xeon Processor 5500 and 3400 Series:  XX_YY??? and 06_1AH, 06_1EH and 06_1FH,
    // Table B-7. Additional MSRs in Intel Xeon Processor 7500 Series
    // Table B-10. MSRs Unique to 64-bit Intel Xeon Processor MP with Up to an 8 MB L3 Cache
    // Table B-14. MSRs in the P6 Family Processors
    {
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelArchitectureSpecificMSRInfo - Freq: %dKHz (Ratio: %d, ExtClk: %dKHz), Turbo: %dKHz, (Ratio: %d, ExtClk: %dKHz) (Theoretical max: %dKHz)",
                pCPUMSRInfo->raw_freq_cpu, pCPUMSRInfo->flBusFrequencyRatio, pCPUMSRInfo->flExternalClock,
                pCPUMSRInfo->flCPUSpeedTurbo, pCPUMSRInfo->flCPUMultTurbo, pCPUMSRInfo->flCPUExternalClockBoosted,
                pCPUMSRInfo->flCPUSpeedTurboTheoreticalMax); // SI101022c
    MtSupportDebugWriteLine(gBuffer);
}

DISABLE_OPTIMISATIONS() // Optimisation must be turned off
// GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED
//
//  Returns the max ratio of core to ref cycles (like cueent utilization, 1 = same as ref - 100%)
//
// 1. Compute and save the reference frequency. Bits 15:8 of the PLATFORM_INFO MSR (0CEH) are the Base Operating Ratio. Multiply by the
//    bus clock frequency (BCLK) to get the base operating frequency. The standard bus clock frequency is 133.33 MHz.
//    e.g. ratio * 20x multiplier x 133.33 = 2,666.6
// 2. Enable fixed Architectural Performance. Monitor counters 1 and 2 in the Global Performance Counter Control IA32_PERF_GLOBAL_CTRL (38FH) and the
//    Fixed-Function Performance Counter Control IA32_FIXED_CTR_CTL (38DH). Repeat this step for each logical processor in the system.
//    Fixed Counter 1 (CPU_CLK_UNHALTED.CORE) counts the number of core cycles while the core is not in a halted state. Fixed Counter 2
//    (CPU_CLK_UNHALTED.REF) counts the number of reference (base operating frequency) cycles while the core is not in a halted state.
// 3. Configure an auto-rearming timer with 1-second duration using an OS API.
// 4. Repeat steps 5 through 10 until the application exits.
// 5. Wait for the 1-second timer to expire.
// 6. Read the Fixed-Function Performance Counter 1 IA32_FIXED_CTR1 (30AH) and the Fixed-Function Performance Counter 2 IA32_FIXED_CTR2 (30BH). Repeat this step
//    for each logical processor in the system.
// 7. Compute the number of unhalted core cycles and unhalted reference cycles that have expired since the last iteration by subtracting
//    the previously sampled values from the currently sampled values.
// 8. Compute the actual frequency value for each logical processor as follows: Fcurrent = Base Operating Frequency * (Unhalted Core Cycles / Unhalted Ref Cycles)
//
//
int GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED(CPUINFO *pCPUInfo, BOOLEAN bGetMSRs, int flTargetRatio) // BIT6010230002
{
    BOOLEAN bWrittenIA32_PERF_GLOBAL_CTRL = FALSE, bWrittenIA32_FIXED_CTR_CTL = FALSE;
    BOOLEAN bIA32_FIXED_CTR1_supported = FALSE;
    UINT64 i = 0, j = 0, ullActivity = 0;
    UINT64 ullIA32_PERF_GLOBAL_CTRL = 0, ullIA32_FIXED_CTR_CTL = 0;
    UINT64 ullNew_IA32_PERF_GLOBAL_CTRL = 0, ullNew_IA32_FIXED_CTR_CTL = 0;
    UINT64 ullIA32_PERF_CTL = 0, ullIA32_PERF_STATUS = 0;
    UINT64 ullCore1 = 0, ullCore2 = 0, ullRef1 = 0, ullRef2 = 0;
    UINT64 count_freq;                 // High Resolution Performance Counter frequency
    UINT64 t0, t1;                     // Variables for High-Resolution Performance Counter reads
    UINT64 total_ticks = 0, ticks = 0; // Microseconds elapsed  during test
    UINT32 CPUInfo[4];
    int tries = 0;
    int flMaxRatio = 0, flCurrentRatio = 0;
    int flFreqCore, flFreqRef;
    SetMem(CPUInfo, sizeof(CPUInfo), 0);

    // Check for MSR support
    // IA32_FIXED_CTR1 - If CPUID.0AH:EDX[4:0] > 1
    // IA32_FIXED_CTR2 - If CPUID.0AH:EDX[4:0] > 2
    if (pCPUInfo->MaxBasicInputValue >= 0xA)
    {
        AsmCpuid(0xA, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

        AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - CPUID[0xA][EDX] = %08x", CPUInfo[3]);
        MtSupportDebugWriteLine(gBuffer);

        if (BitExtract(CPUInfo[3], 4, 0) > 2) // ECX[bit0] == 1, MSR 0xE8 is available //Hardware Coordination Feedback Capability (Presence of IA32_APERF, IA32_MPERF MSRs)
            bIA32_FIXED_CTR1_supported = TRUE;
    }

    if (!bIA32_FIXED_CTR1_supported)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - *WARNING* IA32_FIXED_CTR1 performance counter reported to be unavailable.");
        MtSupportDebugWriteLine(gBuffer);
        return 0;
    }

    // Get Maximum multiplier

    // Set up the performance counters
    ullIA32_PERF_GLOBAL_CTRL = AsmReadMsr64(IA32_PERF_GLOBAL_CTRL);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - MSR[0x38F] = %016lx", ullIA32_PERF_GLOBAL_CTRL);
    MtSupportDebugWriteLine(gBuffer);
    {
        BOOLEAN bEN_FIXED_CTR1 = (BitExtractULL(ullIA32_PERF_GLOBAL_CTRL, 33, 33) == 1);
        BOOLEAN bEN_FIXED_CTR2 = (BitExtractULL(ullIA32_PERF_GLOBAL_CTRL, 34, 34) == 1);
        if (!bEN_FIXED_CTR1 || !bEN_FIXED_CTR2)
        {
            ullNew_IA32_PERF_GLOBAL_CTRL = ullIA32_PERF_GLOBAL_CTRL | ((UINT64)1 << 33);
            ullNew_IA32_PERF_GLOBAL_CTRL = ullNew_IA32_PERF_GLOBAL_CTRL | ((UINT64)1 << 34);

            AsmWriteMsr64(IA32_PERF_GLOBAL_CTRL, ullNew_IA32_PERF_GLOBAL_CTRL);
            bWrittenIA32_PERF_GLOBAL_CTRL = TRUE;
        }
    }

    ullIA32_FIXED_CTR_CTL = AsmReadMsr64(IA32_FIXED_CTR_CTL);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - MSR[0x38D] = %016lx", ullIA32_FIXED_CTR_CTL);
    MtSupportDebugWriteLine(gBuffer);
    {
        BOOLEAN bEN1_OS = (BitExtractULL(ullIA32_FIXED_CTR_CTL, 4, 4) == 1);
        BOOLEAN bEN2_OS = (BitExtractULL(ullIA32_FIXED_CTR_CTL, 8, 8) == 1);
        if (!bEN1_OS || !bEN2_OS)
        {
            ullNew_IA32_FIXED_CTR_CTL = ullIA32_FIXED_CTR_CTL | (UINT64)(1 << 4);
            ullNew_IA32_FIXED_CTR_CTL = ullNew_IA32_FIXED_CTR_CTL | (UINT64)(1 << 8);
            AsmWriteMsr64(IA32_FIXED_CTR_CTL, ullNew_IA32_FIXED_CTR_CTL);
            bWrittenIA32_FIXED_CTR_CTL = TRUE;
        }
    }

    ullIA32_PERF_CTL = AsmReadMsr64(IA32_PERF_CTL);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - MSR[0x199] = %016lx", ullIA32_PERF_CTL);
    MtSupportDebugWriteLine(gBuffer);

    ullIA32_PERF_STATUS = AsmReadMsr64(IA32_PERF_STATUS);
    AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - MSR[0x198] = %016lx", ullIA32_PERF_STATUS);
    MtSupportDebugWriteLine(gBuffer);

    // Get the hig perf. counter freq
    count_freq = cpu_speed * 1000; // GetPerformanceCounterProperties(NULL, NULL);

    AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - high perf counter freq = %ld", count_freq);
    MtSupportDebugWriteLine(gBuffer);

    // Force a higher power state //BIT601001
    j = 0;
    // for (i = 0; i < 500000000; ++i)		//Increased to increase probablility to get to highest power state	100M to 500M //BIT6010230002 //SYSINFO - add this
    // for (i = 0; i < 300000000; ++i)		//Increased to increase probablility to get to highest power state	100M to 500M //BIT6010230002 //SYSINFO - add this//SI101014 - reduce time measuring
    //	j +=i;

    do
    {
        tries++;

        // Don't do the 2nd measurement if it has already taken more than 15 seconds
        if (tries > 1 && total_ticks > 15000000000ULL) // SI101025
            break;

        // QueryPerformanceCounter(&start);
        // Force higher power state to measure max speed. Note: on core i5 (& maybe i7) 0xE8 seems to be reset by external app or by CPU - so load CPU before measurement period
        // BIT6010230002 - check against reference clock speed and increase activity to raise power state (increase activity more if we are much lower than the reference clock)
        ullActivity = 200000000;
        if (flCurrentRatio < flTargetRatio)
        {
            // if (tries == 2)
            //	ullActivity = 100000000;		//SI101014 - reduce time measuring
            // else
            if (tries == 2)
                // ullActivity = 1000000000;
                // ullActivity = 500000000;		//SI101014 - reduce time measuring
                ullActivity = 500000000; // SI101014 - reduce time measuring
        }
        // Skip the 3rd try if the CPU ratio is already higher than the target. This is to reduce CPU measurement time.
        // Note: this may reduce the likelyhood of determining an overclocked multiplier
        // if (flCurrentRatio > flTargetRatio && tries > 2)//SI101014 - reduce time measuring
        if (flCurrentRatio > flTargetRatio) // SI101014 - reduce time measuring
            break;

        // Force a higher power state //BIT601001
        j = 0;
        for (i = 0; i < ullActivity; ++i)
            j += i;

        // Get current high res counter value

        t0 = AsmReadTsc(); // GetPerformanceCounter();
        t1 = t0;

        // Read the CPU_CLK_UNHALTED.CORE
        ullCore1 = AsmReadMsr64(IA32_FIXED_CTR1);

        // Read the CPU_CLK_UNHALTED.REF
        ullRef1 = AsmReadMsr64(IA32_FIXED_CTR2);

        // Activity to be measured
        j = 0;
        for (i = 0; i < 200000000; ++i)
            j += i;

        t1 = AsmReadTsc(); // GetPerformanceCounter();

        // Read the CPU_CLK_UNHALTED.CORE
        ullCore2 = AsmReadMsr64(IA32_FIXED_CTR1);

        // Read the CPU_CLK_UNHALTED.REF
        ullRef2 = AsmReadMsr64(IA32_FIXED_CTR2);

        // QueryPerformanceCounter(&end);

        // Calculate measurement time period difference
        ticks = (t1 - t0);                                                           // Number of external ticks is difference between two hi-res counter reads.
        ticks = DivU64x64Remainder(MultU64x32(ticks, 1000000000), count_freq, NULL); // Ticks / ( ticks/us ) = nanoseconds (ns)
        total_ticks += ticks;                                                        // SI101025

        AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - time in ns = %ld", ticks);
        MtSupportDebugWriteLine(gBuffer);

        if (ticks > 0)
        {
            // Cycles * 1000000 / ps  = KHz
            flFreqCore = (int)DivU64x64Remainder(MultU64x32((ullCore2 - ullCore1), 1000000), ticks, NULL);
            flFreqRef = (int)DivU64x64Remainder(MultU64x32((ullRef2 - ullRef1), 1000000), ticks, NULL);
            if (flFreqRef > 0)
                flCurrentRatio = (flFreqCore * 100) / flFreqRef;

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED - FreqCore in KHz = %d, FreqRef in KHz = %d, Ratio = %d", flFreqCore, flFreqRef, flCurrentRatio);
            MtSupportDebugWriteLine(gBuffer);
        }

        // Store max measured freq (with sanity check
        if (flCurrentRatio > flMaxRatio &&
            flCurrentRatio > 0 && flCurrentRatio < 1000) // Sanity
            flMaxRatio = flCurrentRatio;

        //} while (tries < 3);
    } while (tries < 2);

    // Get the Dynamic MSR's, such as multipliers while the system is loaded, before it can drop to a lower power state
    if (bGetMSRs)
    {
        // Currently none of interest
    }

    // Return the performance counters to their original values
    if (bWrittenIA32_PERF_GLOBAL_CTRL)
    {
        AsmWriteMsr64(IA32_PERF_GLOBAL_CTRL, ullIA32_PERF_GLOBAL_CTRL);
    }

    if (bWrittenIA32_FIXED_CTR_CTL)
    {
        AsmWriteMsr64(IA32_FIXED_CTR_CTL, ullIA32_FIXED_CTR_CTL);
    }

    // Sanity check for max CPU ratio. No one should be underclocking by 40% and we should always be able to rasie the power state higher than 60% max,
    // so if this happens, something has gone wrong, use a sensible value - the reference clock unhaulted cycles
    if (flMaxRatio < 60)
        flMaxRatio = 100;

    return flMaxRatio;
}
ENABLE_OPTIMISATIONS()

DISABLE_OPTIMISATIONS()
// GetIntelCpuMaxSpeed_IA32_APERF
//
//  Actual Performance Clock Counter - IA32_APERF
//  Maximum Performance Clock Counter - IA32_MPERF
//
int GetIntelCpuMaxSpeed_IA32_APERF(CPUINFO *pCPUInfo, BOOLEAN bGetMSRs, CPUMSRINFO *pCPUMSRInfo)
{
    BOOLEAN bMSR0xE8_supported = FALSE;
    UINT32 CPUInfo[4];

    UINT64 count_freq;     // High Resolution Performance Counter frequency
    UINT64 tStart, t0, t1; // Variables for High-Resolution Performance Counter reads
    int tries = 0;         // Number of times a calculation has
    UINT64 i = 0, j = 0;
    UINT64 ullActivity = 0;
    UINT64 total_ticks = 0, ticks = 0; // Microseconds elapsed  during test
    UINT64 elapsed = 0;
    UINT64 ullAPERFDelta, ullMPERFDelta;
    int flFreqAPERF = 0, flFreqMPERF = 0, fFreqMaxAPERF = 0;

    if (pCPUInfo->MaxBasicInputValue < 6)
        return 0;

    SetMem(CPUInfo, sizeof(CPUInfo), 0);
    // Check IA32_APERF (actual performnce counter) supported
    if (pCPUInfo->MaxBasicInputValue >= 6)
    {
        AsmCpuid(0x6, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

        AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCpuMaxSpeed_IA32_APERF - CPUID[0x6][ECX] = %08x", CPUInfo[2]);
        MtSupportDebugWriteLine(gBuffer);

        if (BitExtract(CPUInfo[2], 0, 0) == 1) // ECX[bit0] == 1, MSR 0xE8 is available //Hardware Coordination Feedback Capability (Presence of IA32_APERF, IA32_MPERF MSRs)
            bMSR0xE8_supported = TRUE;
    }

    if (bMSR0xE8_supported) // RDTSC supported && //MSR 0xE8 supported (CPUID.06H:ECX[0] = 1
    {
        count_freq = cpu_speed * 1000; // GetPerformanceCounterProperties(NULL, NULL);

        AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCpuMaxSpeed_IA32_APERF - high perf counter freq = %ld", count_freq);
        MtSupportDebugWriteLine(gBuffer);

        tStart = AsmReadTsc(); // GetPerformanceCounter();

        // Force a higher power state //BIT601001
        j = 0;
        for (i = 0; i < 500000000; ++i) // Increased to increase probablility to get to highest power state	100M to 500M //BIT6010230002 //SYSINFO - add this
            j += i;

        do
        {
            // This do loop runs up to 3 times or until the average of the previous  three calculated frequencies is within 1 MHz of each of the
            // individual calculated frequencies. This resampling increases the accuracy of the results since outside factors could affect this calculation
            tries++; // Increment number of times sampled on this call to cpuspeed

            // Force higher power state to measure max speed. Note: on core i5 (& maybe i7) 0xE8 seems to be reset by external app or by CPU - so load CPU before measurement period
            // BIT6010230002 - check against reference clock speed and increase activity to raise power state (increase activity more if we are much lower than the reference clock)
            ullActivity = 200000000;
            if (fFreqMaxAPERF * 100 < (pCPUMSRInfo->flCPUTSC * 75))
            {
                if (tries > 2)
                    ullActivity = 3000000000;
                else if (tries > 1)
                    ullActivity = 1000000000;
            }
            else if (fFreqMaxAPERF * 100 < (pCPUMSRInfo->flCPUTSC * 95))
            {
                ullActivity = 500000000;
                if (tries > 2) // SI101008
                    ullActivity = 3000000000;
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

            // Force higher power state to measure max speed. Note: on core i5 (& maybe i7) 0xE8 seems to be limited to 3 bytes - so keep smallish for the measurement period
            j = 0;
            for (i = 0; i < 500000; ++i) // Reduce to reduce likelyhood of being caught by the Operating System resetting this value //BIT6010230002
                j += i;

            t1 = AsmReadTsc(); // GetPerformanceCounter();

            // read Actual Performance Colock Counter - IA32_APERF
            ullAPERFDelta = AsmReadMsr64(0xE8);

            ullMPERFDelta = AsmReadMsr64(0xE7);

            // Calculate measurement time period difference
            ticks = t1 - t0;                                                             // Number of external ticks is difference between two hi-res counter reads.
            ticks = DivU64x64Remainder(MultU64x32(ticks, 1000000000), count_freq, NULL); // Ticks / ( ticks/ns ) = nanoseconds (ns)
            total_ticks += ticks;

            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCpuMaxSpeed_IA32_APERF - time in ns = %ld", ticks);
            MtSupportDebugWriteLine(gBuffer);

            if (ticks > 0)
            {
                // Cycles * 1000000 / ns  = kHz
                flFreqMPERF = (int)DivU64x64Remainder(MultU64x32(ullMPERFDelta, 1000000), ticks, NULL);
                flFreqAPERF = (int)DivU64x64Remainder(MultU64x32(ullAPERFDelta, 1000000), ticks, NULL);

                AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCpuMaxSpeed_IA32_APERF - FreqMPERF in KHz = %d, FreqAPERF in KHz = %d", flFreqMPERF, flFreqAPERF);
                MtSupportDebugWriteLine(gBuffer);
            }

            // Store max measured freq (with sanity check
            if (flFreqAPERF > fFreqMaxAPERF && flFreqAPERF < CPUSPEEDMAX)
                fFreqMaxAPERF = flFreqAPERF;

            // Just give up if it's taken more than 5 seconds
            elapsed = (t1 - tStart);
            elapsed = DivU64x64Remainder(elapsed, count_freq, NULL);

        } while (tries < 3 && ticks < 5);

        // No one is going to underclock 40%. Just use the TSC reference speed	//BIT6010230002
        if (fFreqMaxAPERF * 100 < pCPUMSRInfo->flCPUTSC * 60)
            fFreqMaxAPERF = pCPUMSRInfo->flCPUTSC;

        // Get the MSR's, such as multipliers while the system is loaded, before it can drop to a lower power state
        if (bGetMSRs)
        {

            pCPUMSRInfo->raw_freq_cpu = fFreqMaxAPERF; /// Used in some of the MSR functions below (e.g. in calculating the multiplier for Core 2

            if (SysInfo_IsAirmont(pCPUInfo))
                GetAirmontFamilyMSRInfo_Dynamic(pCPUInfo, pCPUMSRInfo);
            else if (SysInfo_IsSilvermont(pCPUInfo))
                GetSilvermontFamilyMSRInfo_Dynamic(pCPUInfo, pCPUMSRInfo);
            else if (IsIntelAtom(pCPUInfo)) // 06_1C
                GetAtomFamilyMSRInfo_Dynamic(pCPUInfo, pCPUMSRInfo);
            // else if (pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1D)				//06_1D //BIT6010140000 //Added CPU multiplier inform etc for Xeon MP 7400 series
            //	GetCore2FamilyMSRInfo_Dynamic(pCPUInfo, cpunumber, pCPUInfo->Family, pCPUInfo->Model);
            // else if (IsIntelCore2(pCPUInfo, pCPUInfo->Family, pCPUInfo->Model))					//06_0F, 06_17
            //	GetCore2FamilyMSRInfo_Dynamic(pCPUInfo, cpunumber, pCPUInfo->Family, pCPUInfo->Model);
            else if ((pCPUInfo->Family == 0x06 && pCPUInfo->Model == 0x1D) || // 06_1D //BIT6010140000 //Added CPU multiplier inform etc for Xeon MP 7400 series
                     (IsIntelCore2(pCPUInfo)))                                // 06_0F, 06_17
            {
                int flCPUSpeed = GetCore2FamilyMSRInfo_Dynamic(pCPUInfo, pCPUMSRInfo);
                if (flCPUSpeed > 0)
                    fFreqMaxAPERF = flCPUSpeed;
            }
            else if (IsIntelCoreSoloDuo(pCPUInfo)) // 06_0E
                GetCoreSoloDuoMSRInfo_Dynamic(pCPUInfo, pCPUMSRInfo);
            else if (IsIntelNetBurst(pCPUInfo)) // 0F_XX
                GetNetBurstFamilyMSRInfo_Dynamic(pCPUInfo, pCPUMSRInfo);
        }
    }

    return fFreqMaxAPERF; // BIT6010120002 - use the max measured frrequency, just in case one or more samples occured at a lower power state
}
ENABLE_OPTIMISATIONS()

// GetIntelTraceCache
//
//  Get the cache descriptor for the trace cache and prefetching information
//
void GetIntelTraceCache(CPUINFO *pCPUInfo)
{
    UINT32 CPUInfo[4];
    int cache_item = 0;
    int cacheinfo[16]; // BIT601000.0014 - bug can have more than 10

#if 0
    if ( !pCPUInfo->CPUIDSupport )  
        return;
#endif

    AsmCpuid(2, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

    cache_item = GetIntelRegCacheInfo(CPUInfo[0], cache_item, cacheinfo, 3);
    cache_item = GetIntelRegCacheInfo(CPUInfo[1], cache_item, cacheinfo, 4);
    cache_item = GetIntelRegCacheInfo(CPUInfo[2], cache_item, cacheinfo, 4);
    cache_item = GetIntelRegCacheInfo(CPUInfo[3], cache_item, cacheinfo, 4);

    pCPUInfo->cacheinfo_num = cache_item;
    for (cache_item = 0; cache_item < pCPUInfo->cacheinfo_num; cache_item++)
    {
        switch (cacheinfo[cache_item])
        {
        case 0x70: // BIT601000
            pCPUInfo->Trace_cache_size = 12;
            break;
        case 0x71: // BIT601000
            pCPUInfo->Trace_cache_size = 16;
            break;
        case 0x72: // BIT601000
            pCPUInfo->Trace_cache_size = 32;
            break;
        case 0x73: // BIT601000
            pCPUInfo->Trace_cache_size = 64;
            break;

        case 0xF0:                      // BIT601000
            pCPUInfo->Prefetching = 64; // Byte
            break;
        case 0xF1:                       // BIT601000
            pCPUInfo->Prefetching = 128; // Byte
            break;
        default:
            break;
        }
    }
}

//
// Get Cache info from a single Intel CPU register
//
// Inputs:
//	the contents of a CPU register after the CPUID instruction
//	the index into of cacheinfo of the next cache ID
//	cacheinfo, an array of cache ID's
//	num_elements, how many cache ID's are in the register
int GetIntelRegCacheInfo(unsigned int cpureg, int cache_item, int *cacheinfo, int num_elements)
{

    if (num_elements == 4)
    {
        cacheinfo[cache_item] = cpureg & 0x00000000FF;
        if (cacheinfo[cache_item] > 0)
            cache_item++;
    }

    cacheinfo[cache_item] = cpureg & 0x000000FF00;
    cacheinfo[cache_item] = cacheinfo[cache_item] >> 8;
    if (cacheinfo[cache_item] > 0)
        cache_item++;

    cacheinfo[cache_item] = cpureg & 0x0000FF0000;
    cacheinfo[cache_item] = cacheinfo[cache_item] >> 16;
    if (cacheinfo[cache_item] > 0)
        cache_item++;

    cacheinfo[cache_item] = cpureg & 0x00FF000000;
    cacheinfo[cache_item] = cacheinfo[cache_item] >> 24;
    if (cacheinfo[cache_item] > 0)
        cache_item++;

    return cache_item;
}

// GetIntelCacheInfo
//
//  GET DETERMINISTIC CACHE PARAMETERS - such as the number of cores each cache services
void GetIntelCacheInfo(CPUINFO *pCPUInfo) // BIT601000.0014 - rewritten to use an algorithim instead of a lookup table + to support cache sharing with CPU's supporting Hyperthreading (e.g. Nehalem)
{
    int iIndex;
    unsigned int eax, ebx, ecx, edx;
    int iCacheType = 0, iCacheLevel = 0;
    int iNumAPICIDsReserved = 1, iNumThreadsServicedByThisCache = 1;
    int iWays = 1, iPartitions = 1, iLine_Size = 1, iSets = 1, iSizeBytes = 0;

#if 0
    if ( !pCPUInfo->CPUIDSupport )  
        return;
#endif

    // Get the L1, L2 and L3 cache info
    eax = 0;
    for (iIndex = 0; iIndex == 0 || (eax != 0 && iIndex < 20); iIndex++)
    {
        AsmCpuidEx(4, iIndex, &eax, &ebx, &ecx, &edx);

        iCacheType = BitExtract(eax, 4, 0);
        iCacheLevel = BitExtract(eax, 7, 5);

        iWays = BitExtract(ebx, 31, 22) + 1;
        iPartitions = BitExtract(ebx, 21, 12) + 1;
        iLine_Size = BitExtract(ebx, 11, 0) + 1;
        iSets = ecx + 1;
        iSizeBytes = iWays * iPartitions * iLine_Size * iSets;

        iNumThreadsServicedByThisCache = BitExtract(eax, 25, 14) + 1; // 2 on Corei7 (correct), 2 on Corei5
        iNumAPICIDsReserved = BitExtract(eax, 31, 26) + 1;            // 8 on Corei7 and Corei5

        if (iCacheType == CACHE_DATA && iCacheLevel == 1) // L1 Data
        {
            pCPUInfo->L1_data_cache_size = iSizeBytes / SIZE_1KB;
            pCPUInfo->L1_data_caches_per_package = iNumAPICIDsReserved / iNumThreadsServicedByThisCache; // BIT6010140003

            AsciiSPrint(gBuffer, BUF_SIZE, "L1 data cache: %d, %d, %d [0x%8X]",
                        pCPUInfo->L1_data_cache_size, iNumThreadsServicedByThisCache, iNumAPICIDsReserved, eax);
            MtSupportDebugWriteLine(gBuffer);
        }
        else if (iCacheType == CACHE_INSTRUCTION && iCacheLevel == 1) // L1 Instruction
        {
            pCPUInfo->L1_instruction_cache_size = iSizeBytes / SIZE_1KB;
            pCPUInfo->L1_instruction_caches_per_package = iNumAPICIDsReserved / iNumThreadsServicedByThisCache; // BIT6010140003
        }
        else if (iCacheLevel == 2) // L2 Instruction
        {
            pCPUInfo->L2_cache_size = iSizeBytes / SIZE_1KB;
            pCPUInfo->L2_caches_per_package = iNumAPICIDsReserved / iNumThreadsServicedByThisCache; // BIT6010140003
        }
        else if (iCacheLevel == 3) // L3 Instruction
        {
            pCPUInfo->L3_cache_size = iSizeBytes / SIZE_1KB;
            pCPUInfo->L3_caches_per_package = iNumAPICIDsReserved / iNumThreadsServicedByThisCache; // BIT6010140003
        }
        else if (iSizeBytes > 0 && iNumThreadsServicedByThisCache != 0)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "L%d cache: %d KB (%d)",
                        iCacheLevel,
                        iSizeBytes / SIZE_1KB,
                        iNumAPICIDsReserved / iNumThreadsServicedByThisCache);
            MtSupportDebugWriteLine(gBuffer);
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "L%d%c t%u linesize %d linepart %d ways %d sets %d, size %luKB",
                    iCacheLevel,
                    iCacheType == CACHE_DATA ? 'd' : iCacheType == CACHE_INSTRUCTION ? 'i'
                                                                                     : 'u',
                    iNumThreadsServicedByThisCache,
                    iLine_Size,
                    iPartitions,
                    iWays,
                    iSets,
                    iSizeBytes / SIZE_1KB);
        MtSupportDebugWriteLine(gBuffer);
    }

#if 0
    //Number of Haswell-E caches is incorrect - so cludge fix here until worked out - //SI101086 
    if (SysInfo_IsNextGenerationXeonHaswell(pCPUInfo) ||
        SysInfo_IsKnightsLanding(pCPUInfo))		//SI101100)
    {
        if (pCPUInfo->L1_data_caches_per_package > 0)
            pCPUInfo->L1_data_caches_per_package = pCPUInfo->iCoresPerPackage;
        if (pCPUInfo->L1_instruction_caches_per_package > 0)
            pCPUInfo->L1_instruction_caches_per_package = pCPUInfo->iCoresPerPackage;
        if (pCPUInfo->L2_caches_per_package > 0)
            pCPUInfo->L2_caches_per_package = pCPUInfo->iCoresPerPackage;
    }
    
    //Generally speaking there should not be more than 1 cache per core. Current mechanism doesn't work for some CPU's, e.g. Bay trail. Shows 8 L1 caches, should be 4 (as per number of cores). //SI101087
    if (SysInfo_IsSilvermont(pCPUInfo)) ||
        SysInfo_IsAirmont(pCPUInfo))	//SI101100
    {
        if (pCPUInfo->L1_data_caches_per_package > pCPUInfo->iCoresPerPackage)
            pCPUInfo->L1_data_caches_per_package = pCPUInfo->iCoresPerPackage;
        if (pCPUInfo->L1_instruction_caches_per_package > pCPUInfo->iCoresPerPackage)
            pCPUInfo->L1_instruction_caches_per_package = pCPUInfo->iCoresPerPackage;
        if (pCPUInfo->L2_caches_per_package > pCPUInfo->iCoresPerPackage)
            pCPUInfo->L2_caches_per_package = pCPUInfo->iCoresPerPackage;
    }
#endif
    // Get the Trace cache and pre-fetching info
    GetIntelTraceCache(pCPUInfo);
}
#endif

// Name:	GetIntelCPUTemps()
//
//  Gets temperatures from the CPU MSR's.
//
EFI_STATUS GetIntelCPUTemps(CPUINFO *pCPUInfo, UINTN ProcNum, MSR_TEMP *CPUMSRTemp)
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
            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUTemps - Could not switch BSP to Proc#%d (%r)", ProcNum, Status);
            MtSupportDebugWriteLine(gBuffer);
            return Status;
        }
    }

    // Get the CPU MSR info
    if (GetIA32ArchitecturalTemp(pCPUInfo, CPUMSRTemp))
        Status = EFI_SUCCESS;

    if (OldBSP != ProcNum)
    {
        if (MPSupportSwitchBSP(OldBSP) != EFI_SUCCESS)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "GetIntelCPUTemps - Could not switch BSP back to Proc#%d", OldBSP);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
#endif
    return Status;
}
