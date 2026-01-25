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
//	cpuinfoMSRIntel.h
//
// Author(s):
//	Keith Mah, Ian Robinson
//
// Description:
//	Detects and gathers information about Intel CPU(s).
//	Including speed, manufacturer, capabilities, available L2 cache, etc..
//
//   Based on source code from SysInfo
#ifndef _CPUINFOMSRINTEL_H_
#define _CPUINFOMSRINTEL_H_

typedef struct _CPUINFO CPUINFO;
typedef struct _CPUMSRINFO CPUMSRINFO;
typedef struct _MSR_TEMP MSR_TEMP;

BOOLEAN SysInfo_IsLunarLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsArrowLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsGraniteRapids(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsSierraForest(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsMeteorLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsEmeraldRapidsScalable(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsSapphireRapidsScalable(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsTremont(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsRaptorLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsIceLakeScalable(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsAlderLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsElkhartLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsRocketLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsTigerLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsIceLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsCometLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsKnightsMill(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsCannonLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsKabyLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsGeminiLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsGoldmontPlus(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsApolloLake(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsGoldmont(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsXeonBroadwellE(CPUINFO *pCPUInfo); // SI101112

BOOLEAN SysInfo_IsSkylakeScalable(CPUINFO *pCPUInfo); // SI101106

BOOLEAN SysInfo_IsSkylake(CPUINFO *pCPUInfo); // SI101106

BOOLEAN SysInfo_IsKnightsLanding(CPUINFO *pCPUInfo); // SI101100

BOOLEAN SysInfo_IsNextGenerationXeonHaswell(CPUINFO *pCPUInfo); // SI101073

BOOLEAN SysInfo_IsBroadwell(CPUINFO *pCPUInfo); // SI101073

BOOLEAN SysInfo_IsBroadwellCoreOrM(CPUINFO *pCPUInfo); // SI101100

BOOLEAN SysInfo_IsIvyBridgeEP(CPUINFO *pCPUInfo); // SI101039

BOOLEAN SysInfo_IsAirmont(CPUINFO *pCPUInfo); // SI101100

BOOLEAN SysInfo_IsSilvermont(CPUINFO *pCPUInfo); // SI101039

BOOLEAN SysInfo_IsHaswell(CPUINFO *pCPUInfo); // SI101032

BOOLEAN SysInfo_IsIvyBridgeOrLater(CPUINFO *pCPUInfo); // SI101052

BOOLEAN SysInfo_IsIvyBridge(CPUINFO *pCPUInfo); // SI101022

BOOLEAN SysInfo_IsXeonE7(CPUINFO *pCPUInfo); // SI101022

BOOLEAN SysInfo_IsSandyBridge(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsWestmere(CPUINFO *pCPUInfo);

BOOLEAN SysInfo_IsNehalemArhitecture(CPUINFO *pCPUInfo);

BOOLEAN IsIntelXeon_1D(CPUINFO *pCPUInfo); // BIT6010120002

BOOLEAN IsIntelAtom(CPUINFO *pCPUInfo);

BOOLEAN IsIntelCore2(CPUINFO *pCPUInfo);

BOOLEAN IsIntelXeon_17(CPUINFO *pCPUInfo);

BOOLEAN IsIntelCoreSoloDuo(CPUINFO *pCPUInfo); // BIT601000.0012

BOOLEAN IsIntelNetBurst(CPUINFO *pCPUInfo);

void GetGeminiLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetGoldmontFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetGoldmontFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetAirmontFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetAirmontFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetSilvermontFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetSilvermontFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetHaswellFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo); // SI101032

void GetIvyBridgeEPFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo); // SI101022

void GetIvyBridgeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo); // SI101022

void GetXeonE7FamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo); // BIT6010190001

void GetSandyBridgeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetWestmereFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo); // BIT6010190001

void GetNehalemFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetNextGenerationXeonHaswellFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetBroadwellFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetAtomFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

int GetCore2FamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetCoreSoloDuoMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetNetBurstFamilyMSRInfo_Dynamic(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetKnightsLandingFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetSkylakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetXeonBroadwellEFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo); // SI101112

void GetApolloLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetKabyLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetCannonLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetKnightsMillFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetCometLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetIceLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetTigerLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetRocketLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetElkhartLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetAlderLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetIceLakeScalableFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetRaptorLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetTremontFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetSapphireRapidsScalableFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetEmeraldRapidsScalableFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetMeteorLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetSierraForestFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetGraniteRapidsFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetArrowLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void GetLunarLakeFamilyMSRInfo_Static(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

BOOLEAN IsIntelTurboSupported(CPUINFO *pCPUInfo);

BOOLEAN IsDisableIntelTurboSupported(CPUINFO *pCPUInfo); // BIT6010170007

BOOLEAN DisableTurboMode(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

void EnableTurboMode(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

BOOLEAN IsIntelPerfMonSupported(CPUINFO *pCPUInfo);

BOOLEAN IsIntelFixedFunctionPerfCountersSupported(CPUINFO *pCPUInfo);

void GetIntelArchitectureSpecificMSRInfo(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRInfo);

int GetIntelCPUMaxSpeed_CPU_CLK_UNHALTED(CPUINFO *pCPUInfo, BOOLEAN bGetMSRs, int flTargetRatio); // BIT6010230002

int GetIntelCpuMaxSpeed_IA32_APERF(CPUINFO *pCPUInfo, BOOLEAN bGetMSRs, CPUMSRINFO *pCPUMSRInfo);

void GetIntelTraceCache(CPUINFO *pCPUInfo);

int GetIntelRegCacheInfo(unsigned int cpureg, int cache_item, int *cacheinfo, int num_elements);

void GetIntelCacheInfo(CPUINFO *pCPUInfo);

BOOLEAN GetIA32ArchitecturalTemp(CPUINFO *pCPUInfo, MSR_TEMP *CPUTemp);

EFI_STATUS GetIntelCPUTemps(CPUINFO *pCPUInfo, UINTN ProcNum, MSR_TEMP *CPUMSRTemp);

#endif
