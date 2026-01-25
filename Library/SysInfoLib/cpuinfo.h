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
//	cpuinfo.h
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

#ifndef _CPUINFO_H_INCLUDED_
#define _CPUINFO_H_INCLUDED_

#define CPUSPEEDMAX 15000000 // A sanity check value for CPU speed SI101022c - sanity check for overflowing CPU register etc

typedef enum
{
	CPU_OVERCLOCKED_UNKNOWN = 0,
	CPU_OVERCLOCKED = 1,
	CPU_UNDERCLOCKED = 2,
	CPU_OVERCLOCKED_NO = 3,
	CPU_MAXVAL = 4,
} CPU_OCLK;

typedef struct _CPUMSRINFO
{
	// GetIA32ArchitecturalMSRInfo
	UINT64 ullMSR0x17;
	UINT64 ullMSR0x1B;
	UINT64 ullMSR0xE7;
	UINT64 ullMSR0xE8;
	UINT64 ullMSR0x19C;
	UINT64 ullMSR0x1A0;

	// GetCore2FamilyMSRInfo_Dynamic
	UINT64 ullMSR0x2A;
	UINT64 ullMSR0x2C;
	UINT64 ullMSR0xCD;

	// Nehalem
	UINT64 ullMSR0xCE;
	UINT64 ullMSR0x1AD;
	UINT64 ullMSR0x1A2;
	UINT64 ullMSR0x1AC;

	int iNonIntegerBusRatio;
	/// Current Multiplier
	int flBusFrequencyRatio;
	/// Current Raw frequency of CPUn in MHz
	int raw_freq_cpu;

	int flScalableBusSpeed;
	int flFSB;

	/// Current base reference clock speed - AMD and Intel Core i7/i5... series
	int flExternalClock;

	/// AMD HyperTransport link speed (The value is not valid if set to 0)
	int flHTFreq;

	int iMinimumRatio;
	int iMaximumRatio;

	/// Intel Tubo boost max multiplier when running 1 core
	int iMaxTurbo1Core;
	/// Intel Tubo boost max multiplier when running 2 core
	int iMaxTurbo2Core;
	/// Intel Tubo boost max multiplier when running 3 core
	int iMaxTurbo3Core;
	/// Intel Tubo boost max multiplier when running 4 core
	int iMaxTurbo4Core;
	/// The TDP/power limit (where available)
	int iTDPLimit;
	/// The TDC/current limit (where available)
	int iTDCLimit;

	int iTemperatureTarget;

	// CPU Turbo
	/// Maximum measured Turbo speed	e.g. 3600MHz
	int flCPUSpeedTurbo;
	/// Maximum theoretical Turbo speed based on the maximum programmed muliplier and the derived base clock speed e.g. 200 x 18 (power state 0 multiplier) = 3600HMz
	int flCPUSpeedTurboTheoreticalMax;
	/// Programmed maximum Turbo muliplier	e.g. x18
	int flCPUMultTurbo;
	/// Derived base clock e.g. 200MHz
	int flCPUExternalClockTurbo;

	// CPU stock settings - Nehalem series
	int flCPUSpeedStock;
	int flCPUMultStock;
	int flCPUExternalClockStock;

	// CPU TSC CPU speed - generic
	/// TSC measured speed
	int flCPUTSC;

	// CPU stock settings - Core 2 series
	int flScalableBusSpeedStock;
	int flFSBStock;

	// Overclock settings
	/// The CPU has been overclocked, underclocked by changing the base CPU clock
	CPU_OCLK OCLKBaseClock;
	/// The CPU has been overclocked by increasing the maximum multiplier
	CPU_OCLK OCLKMultiplier;
	/// The CPU has been overclocked, method unknown
	CPU_OCLK OCLKFreq;

	// AMD Turbo core
	/// The CPU external clock when the CPU is in Turbo core
	int flCPUExternalClockBoosted;
	/// The minimum CPU multiplier
	int flMinMult;
	/// The maximum CPU multiplier, when not in Turbo core mode
	int flMaxMult;
	/// The maximum CPU multiplier when in Turbo core mode
	int flBoostedMult;

	// SI101039
	/// Intel Tubo boost max multiplier when running 5 core
	int iMaxTurbo5Core;
	/// Intel Tubo boost max multiplier when running 6 core
	int iMaxTurbo6Core;
	/// Intel Tubo boost max multiplier when running 7 core
	int iMaxTurbo7Core;
	/// Intel Tubo boost max multiplier when running 8 core
	int iMaxTurbo8Core;

} CPUMSRINFO;

// Structs
typedef struct _CPU_SPECIFICATION
{
	CHAR16 *szCPU;
	int iType;
	int iFamily;
	int iModel;
	/// Stepping number from CPU
	int iStepping;
	UINT16 wCPUCodeName;
	/// index into Stepping string
	UINT16 wStepping;
	UINT8 bySocket;
	UINT8 byFabrication;
	int iTJunction;
} CPU_SPECIFICATION;

typedef struct _CPUINFO
{
	int Family;
	int Model;
	int stepping;
	CHAR16 manufacture[20];
	CHAR16 typestring[50];

	/// The maximum CPUID basic information level supported
	int MaxBasicInputValue;
	/// The maximum CPUID extended information level supported
	unsigned int MaxExtendedCPUID;

	/// The number of cache elements. For Intel CPU's only
	int cacheinfo_num;
	/// Cache size. 128, 512, 1024, etc.., -1 = Unknown. For all CPU's
	int L2_cache_size;
	/// Cache size in KB. 128, 512, 1024, etc.., -1 = Unknown
	int L3_cache_size;
	/// L1 instruction cache size
	int L1_instruction_cache_size;
	/// Trace cache size
	int Trace_cache_size;
	/// L1 data cache size
	int L1_data_cache_size;
	/// CPU prefetch size
	int Prefetching;
	/// Num. L1 data caches per CPU package
	int L1_data_caches_per_package;
	/// Num. L1 instruction caches per CPU package
	int L1_instruction_caches_per_package;
	/// Num. L2 data caches per CPU package
	int L2_caches_per_package;
	/// Num. L3 data caches per CPU package
	int L3_caches_per_package;

	/// Themal monitor supported (via MSRs)
	int ACPI;
	/// Digital temperature sensor supported
	BOOLEAN bDTS;
	/// Intel Tubo boost supported
	BOOLEAN bIntelTurboBoost;

} CPUINFO;

void PopulateCPUIDInfo(struct cpu_ident *pCPUID, CPUINFO *pCPUInfo);

void GetCacheInfo(CPUINFO *pCPUInfo);

int GetCoresPerPackage(CPUINFO *pCPUInfo);

void GetMSRData(CPUINFO *pCPUInfo, CPUMSRINFO *pCPUMSRinfo);

#endif
