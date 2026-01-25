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
//	cpuinfo.c
//
// Author(s):
//	Keith Mah, David Wren, Ian Robinson
//
// Description:
//	Detects and gathers information about the CPU(s).
//	Including speed, manufacturer, capabilities, available L2 cache, etc..
//
//	This code should work on all CPUs from x486 to current 32bit CPUs
//	from Intel and AMD (and most others as well)
//
//	This modules make fairly extensive us of x86 assembler code to get
//	the information that it needs.
//
//   Based on source code from SysInfo

#include "uMemTest86.h"

#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
// #include <Library/TimerLib.h>
#include <Library/Cpuid.h>

#include <Library/MemTestSupportLib.h>
#include "cpuinfo.h"
#include "cpuinfoMSRIntel.h"
#include "cpuinfoMSRAMD.h"

// Dual core definitions
#define NUMCORESMASK 0xFC000000
#define NUMCORESSHFT 26

extern UINTN cpu_speed;

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
void GetViaCacheInfo(CPUINFO *pCPUInfo);

// GetCoresPerPackage
//
//	Get the number of cores per physical CPU Package (from CPUID 4)
//
int GetCoresPerPackage(CPUINFO *pCPUInfo)
{
    unsigned int CPUInfo[4];

    if (StrCmp(L"GenuineIntel", pCPUInfo->manufacture) == 0)
    {
        AsmCpuidEx(4, 0, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);

        AsciiSPrint(gBuffer, BUF_SIZE, "CPU CorePPack: %d (0x%8X)", (((CPUInfo[0] & NUMCORESMASK) >> NUMCORESSHFT) + 1), CPUInfo[0]);
        MtSupportDebugWriteLine(gBuffer);

        // return the number of cores in the package (located in eax[31:26]).
        return ((((CPUInfo[0] & NUMCORESMASK) >> NUMCORESSHFT) + 1));
    }
    else if (StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0 || StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0)
    {
        unsigned int reg_ecx = 1;
        int iNumCores = 1;

        /* Get extended feature flags, only save EDX */
        if (pCPUInfo->MaxExtendedCPUID >= 0x80000008)
        {
            AsmCpuid(0x80000008, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);
            reg_ecx = CPUInfo[2];
        }

        iNumCores = (reg_ecx & 0xFF) + 1;    // Only keep bits 0-7	//IR updated 15 Sept 2005
        if (iNumCores < 1 || iNumCores > 64) // do a little sanity check	//BIT6010120008 - increased sanity check from 8 to 32 cores (for AMD Magny-Cours - 12 core)
            iNumCores = 1;

        AsciiSPrint(gBuffer, BUF_SIZE, "CPU CorePPack: %d (0x%8X)", iNumCores, reg_ecx);
        MtSupportDebugWriteLine(gBuffer);

        return iNumCores;
    }
    else
        return 1;
}

//
// Get Cache info
//
// Assumes manufacture value has been set.
void GetCacheInfo(CPUINFO *pCPUInfo)
{
    pCPUInfo->L1_instruction_cache_size = 0;
    pCPUInfo->Trace_cache_size = 0;

    pCPUInfo->L1_instruction_caches_per_package = 0;
    pCPUInfo->L1_data_cache_size = 0;
    pCPUInfo->L1_data_caches_per_package = 0;
    pCPUInfo->L2_cache_size = 0;
    pCPUInfo->L2_caches_per_package = 0;
    pCPUInfo->L3_cache_size = 0;
    pCPUInfo->L3_caches_per_package = 0;

    pCPUInfo->cacheinfo_num = 0;
    pCPUInfo->Prefetching = 0;

    if (StrCmp(L"GenuineIntel", pCPUInfo->manufacture) == 0)
    {
        GetIntelCacheInfo(pCPUInfo);
    }
    else if (StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0 || StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0)
    {
        GetAMDCacheInfo(pCPUInfo);
    }
    else if (StrCmp(L"CentaurHauls", pCPUInfo->manufacture) == 0)
    {
        GetViaCacheInfo(pCPUInfo);
    }
    else if (StrCmp(L"CentaulsaurH", pCPUInfo->manufacture) == 0)
    {
        GetViaCacheInfo(pCPUInfo);
    }
    else
    {
        // Future
    }
}

//
// GetViaCacheInfo
//
void GetViaCacheInfo(CPUINFO *pCPUInfo)
{
    unsigned int CPUInfoL1Cache[4];
    unsigned int CPUInfoL2Cache[4];
    int iViaCoresPerPackage = 0;

    pCPUInfo->L2_cache_size = 0;
    pCPUInfo->L3_cache_size = 0; // Level 3 cahce size not detected yet
    pCPUInfo->cacheinfo_num = 0;

    // BIT601000
    iViaCoresPerPackage = GetCoresPerPackage(pCPUInfo);

    // L1 Cache

    if (pCPUInfo->MaxExtendedCPUID >= 0x80000005) // L1 Cache info is available
    {
        AsmCpuid(0x80000005, &CPUInfoL1Cache[0], &CPUInfoL1Cache[1], &CPUInfoL1Cache[2], &CPUInfoL1Cache[3]);
        pCPUInfo->L1_instruction_cache_size = BitExtract(CPUInfoL1Cache[2], 31, 24); // ECX[31:24] L1 data cache (KB) per CORE
        pCPUInfo->L1_data_cache_size = BitExtract(CPUInfoL1Cache[3], 31, 24);        // EDX[31:24] L1 instr. cache (KB) per CORE

        pCPUInfo->L1_instruction_caches_per_package = iViaCoresPerPackage;
        pCPUInfo->L1_data_caches_per_package = iViaCoresPerPackage;
    }

    // L2 Cache
    if (pCPUInfo->MaxExtendedCPUID >= 0x80000006) // L2 Cache info is available
    {
        AsmCpuid(0x80000006, &CPUInfoL2Cache[0], &CPUInfoL2Cache[1], &CPUInfoL2Cache[2], &CPUInfoL2Cache[3]);
        pCPUInfo->L2_cache_size = (CPUInfoL2Cache[2] >> 16) & 0xffff; // ECX[31:16] L2 cache (KB) per CORE
        pCPUInfo->L2_caches_per_package = iViaCoresPerPackage;
    }

    // L3 Cache
    // No L3 cache described in "VIA CPU note Version 0.20 2/3/08"
    pCPUInfo->L3_cache_size = 0; // Level 3 cahce size not detected yet
    pCPUInfo->L3_caches_per_package = 0;
}

void GetCPUThermalAndPowerLeaf(CPUINFO *cpuinfo)
{
    if (cpuinfo->MaxBasicInputValue >= 6)
    {
        UINT32 CPUInfo[4];
        SetMem(CPUInfo, sizeof(CPUInfo), 0);
        AsmCpuid(0x6, &CPUInfo[0], &CPUInfo[1], &CPUInfo[2], &CPUInfo[3]);
        cpuinfo->bDTS = (BitExtract(CPUInfo[0], 0, 0) == 1);             // CPUID.06H:EAX[0:0], Digital temperature sensor supported
        cpuinfo->bIntelTurboBoost = (BitExtract(CPUInfo[0], 1, 1) == 1); // CPUID.06H:EAX[1:1], Intel Turbo Boost supported
    }
}

// GetMSRData
//
//  Get CPU Model Specific register information
//
//  Note: Assumes already locked to a single CPU
//
//  Parms:
//	IN: cpuinfo - CPU info already collected (e.g. to include CPU type and family to determine which MSR's are available)
//	OUT: pCPUMSRinfo - Collected MSR data
//   RETURNS:
//		CPUINFO_SUCCESS
//		CPUINFO_ACCESS_DENIED;
//		CPUINFO_INIT_DIRECTIO;
//
void GetMSRData(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRinfo)
{
    SetMem(pCPUMSRinfo, sizeof(*pCPUMSRinfo), 0);

    GetCPUThermalAndPowerLeaf(pCPUInfo);

    // Initialize to speed calculated using TSC
    pCPUMSRinfo->raw_freq_cpu = (int)cpu_speed;

    // Get the CPU MSR info
    if (StrCmp(L"GenuineIntel", pCPUInfo->manufacture) == 0) // Get Intel CPU MSR info
    {
        pCPUMSRinfo->flCPUTSC = (int)cpu_speed; // We have already worked out the CPU speed with TSC (save it for calculations) in below

        // Get the MSR info specific to each CPU model - such as speed, mult, base clock and get other MSR info (such as max mult, temp. targets)
        GetIntelArchitectureSpecificMSRInfo(pCPUInfo, pCPUMSRinfo);
    }
    else if (StrCmp(L"AuthenticAMD", pCPUInfo->manufacture) == 0 || StrCmp(L"HygonGenuine", pCPUInfo->manufacture) == 0) // Get AMD CPU MSR info
    {
        // GetAMDArchitecturalMSRInfo(pCPUInfo->Family, pCPUInfo->Model);
        GetAMDFamilyMSRInfo(pCPUInfo, pCPUMSRinfo); // SI101021 - added AMD Bobcat/Bulldozer and additional 12h info
    }
}
#endif

void PopulateCPUIDInfo(struct cpu_ident *pCPUID, CPUINFO *pCPUInfo)
{
    pCPUInfo->MaxBasicInputValue = pCPUID->max_cpuid;
    pCPUInfo->MaxExtendedCPUID = pCPUID->max_xcpuid;
    pCPUInfo->Family = CPUID_FAMILY(pCPUID);
    pCPUInfo->Model = CPUID_MODEL(pCPUID);
    pCPUInfo->stepping = CPUID_STEPPING(pCPUID);

    pCPUInfo->ACPI = pCPUID->fid.bits.acpi;

    AsciiStrToUnicodeStrS(pCPUID->vend_id.char_array, pCPUInfo->manufacture, ARRAY_SIZE(pCPUInfo->manufacture));
    AsciiStrToUnicodeStrS(pCPUID->brand_id.char_array, pCPUInfo->typestring, ARRAY_SIZE(pCPUInfo->typestring));
}
