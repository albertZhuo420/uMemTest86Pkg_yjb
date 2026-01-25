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
//	SMBus.h
//
// Author(s):
//	Ian Robinson, Keith Mah
//
// Description:
//	Functions for retrieving and decoding SPD information via SMBUS
//
// History
//	1 June 2005: Initial version

#ifndef _SMBUS_H_
#define _SMBUS_H_

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#else // For MemTest86
#include "win32_defines.h"

#define MODULE_PARTNO_LEN 32
#define EXT_SERIAL_NUM_LEN 9
extern int g_maxMemModules;
#define MAX_MEMORY_SLOTS g_maxMemModules
#define MAX_SPD_LENGTH 1024

/*--- SPDINFO.type ---*/
enum
{
	SPDINFO_MEMTYPE_SDRAM = 4,
	SPDINFO_MEMTYPE_DDR_SGRAM = 6,
	SPDINFO_MEMTYPE_DDR = 7,
	SPDINFO_MEMTYPE_DDR2 = 8,
	SPDINFO_MEMTYPE_DDR2FB = 9,
	SPDINFO_MEMTYPE_DDR2FBPROBE = 10,
	SPDINFO_MEMTYPE_DDR3 = 11,
	SPDINFO_MEMTYPE_DDR4 = 12,
	SPDINFO_MEMTYPE_DDR4E = 14,
	SPDINFO_MEMTYPE_LPDDR3 = 15,
	SPDINFO_MEMTYPE_LPDDR4 = 16,
	SPDINFO_MEMTYPE_LPDDR4X = 17,
	SPDINFO_MEMTYPE_DDR5 = 18,
	SPDINFO_MEMTYPE_LPDDR5 = 19,
	SPDINFO_MEMTYPE_DDR5_NVDIMM_P = 20,
	SPDINFO_MEMTYPE_LPDDR5X = 21,
};

enum
{
	DDR5_MODULE_TYPE_RDIMM = 1,
	DDR5_MODULE_TYPE_UDIMM = 2,
	DDR5_MODULE_TYPE_SODIMM = 3,
	DDR5_MODULE_TYPE_LRDIMM = 4,
	DDR5_MODULE_TYPE_CUDIMM = 5,
	DDR5_MODULE_TYPE_CSODIMM = 6,
	DDR5_MODULE_TYPE_MRDIMM = 7,
	DDR5_MODULE_TYPE_CAMM2 = 8,
	DDR5_MODULE_TYPE_DDIMM = 10,
	DDR5_MODULE_TYPE_SOLDER_DOWN = 11,
};

/*--- SPDINFO.DDR2SDRAM.EPP.profileType ---*/
#define SPDINFO_DDR2_EPP_PROFILE_ABBR 0xA1
#define SPDINFO_DDR2_EPP_PROFILE_FULL 0XB1

#define MAX_XMP3_PROFILE_NAME_LEN 16
#define MAX_XMP3_PROFILES 5

#define MAX_EXPO_PROFILES 2

typedef struct _DEVINFO
{
	/// Jedec ID code
	unsigned char jedecID;
	/// Jedec Bank
	int jedecBank;
	union
	{
		struct
		{
			unsigned int type : 4;
			unsigned int reserved : 2;
			unsigned int installed1 : 1;
			unsigned int installed0 : 1;
		} bits;
		unsigned char raw;
	} deviceType;
	int deviceRev;
} DEVINFO;

typedef struct _SPDINFO
{
#if 0
    /// String summaries used in the formatted reports
    char szHeading[SHORT_STRING_LEN];
    /// Line 1 of the report memory summary
    char szLine1[SHORT_STRING_LEN];
    /// Line 2 of the report memory summary
    char szLine2[LONG_STRING_LEN]; //Changed from SHORT_STRING_LEN as too short for some systems
    /// Line 3 of the report memory summary
    char szLine3[SHORT_STRING_LEN];
#endif

	/// Type of RAM. See #define \link #SPDINFO_MEMTYPE_SDRAM SPDINFO_MEMTYPE_* \endlink
	int type;
	/// SPD revision
	int spdRev;
	/// TRUE for registered
	bool registered;
	/// TRUE for ECC
	bool ecc;
	/// Year of manufacture (only valid if > 0)
	int year;
	/// Week of manufacture (only valid if > 0)
	int week;
	/// DIMM slot number
	int dimmNum;
	/// Channel number
	int channel;
	/// Slot number
	int slot;
	/// SMBIOS index number
	int smbiosInd;
	/// Jedec ID code
	unsigned char jedecID;
	/// Jedec Bank
	int jedecBank;
	/// Jedec manufacture name
	CHAR16 jedecManuf[SHORT_STRING_LEN];

	/// Module part number
	char modulePartNo[MODULE_PARTNO_LEN];
	/// Module revision
	int moduleRev;
	/// Module serial number
	int moduleSerialNo;
	/// Module manufacturing location
	unsigned char moduleManufLoc;
	/// Module extended serial number
	unsigned char moduleExtSerialNo[EXT_SERIAL_NUM_LEN];

	/// Number of row addressing bits
	int rowAddrBits;
	/// Number of column addressing bits
	int colAddrBits;
	/// Number of banks
	int numBanks;
	/// Number of ranks
	int numRanks;
	/// Device width in bits
	int deviceWidth;
	/// Bus width in bits
	int busWidth;
	/// Transfer width in bits
	int txWidth;
	/// Module voltage
	CHAR16 moduleVoltage[SHORT_STRING_LEN];

	/// Clock speed in MHz
	int clkspeed;
	/// Module speed
	CHAR16 szModulespeed[SHORT_STRING_LEN];
	/// Memory capacity in MB
	int size;

	/// CAS latencies supported
	CHAR16 CASSupported[SHORT_STRING_LEN];
	/// Minimum clock cycle time in ps
	int tCK;
	/// Minimum CAS latency time in ps
	int tAA;
	/// Minimum RAS to CAS delay in ps
	int tRCD;
	/// Minimum Row Precharge time in ps
	int tRP;
	/// Minimum Active to Precharge Time in ps
	int tRAS;
	/// Minimum Row Active to Row Active delay in ps
	int tRRD;
	/// Minimum Auto-Refresh to Active/Auto-Refresh Time in ps
	int tRC;
	/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
	int tRFC;

	/// Original RAW / Unprocessed SPD data
	unsigned char rawSPDData[MAX_SPD_LENGTH];

	/// Module Manufacturer's Specific Data offset
	unsigned short manufDataOff;

	/// Module Manufacturer's Specific Data length
	unsigned char manufDataLen;

	/// RAM-type specific attributes
	union SPECIFICINFO
	{
		/// SDRAM-specific attributes
		struct SDRSDRAMINFO
		{
			/// Data access time from clock
			int tAC;
			/// Clock cycle time at medium CAS latency in ps
			int tCKmed;
			/// Data access time at medium CAS latency in ps
			int tACmed;
			/// Clock cycle time at short CAS latency in ps
			int tCKshort;
			/// Data access time at short CAS latency in ps
			int tACshort;

			/// Address/Command Setup Time before clock in ps
			int tIS;
			/// Address/Command Hold Time after clock in ps
			int tIH;
			/// Data Input Setup Time before strobe in ps
			int tDS;
			/// Data Input Hold Time after strobe in ps
			int tDH;

			/// CS latencies supported
			CHAR16 CSSupported[SHORT_STRING_LEN];
			/// WE latencies supported
			CHAR16 WESupported[SHORT_STRING_LEN];

			/// Burst Lengths supported
			CHAR16 BurstLengthsSupported[SHORT_STRING_LEN];
			/// Refresh rate
			CHAR16 RefreshRate[SHORT_STRING_LEN];

			/// TRUE for buffered address/control inputs
			bool buffered;
			/// TRUE for on-card PLL
			bool OnCardPLL;
			/// TRUE for buffered DQMB inputs
			bool BufferedDQMB;
			/// TRUE for registerd DQMB inputs
			bool RegisteredDQMB;
			/// TRUE for differential clock input
			bool DiffClockInput;

			/// TRUE for Early RAS# Precharge supported
			bool EarlyRASPrechargeSupported;
			/// TRUE for Auto-Precharge supported
			bool AutoPrechargeSupported;
			/// TRUE for Precharge all supported
			bool PrechargeAllSupported;
			/// TRUE for write/read burst supported
			bool WriteReadBurstSupported;

		} SDRSDRAM;
		/// DDR-specific attributes
		struct DDR1SDRAMINFO
		{
			/// Data access time from clock
			int tAC;
			/// Clock cycle time at medium CAS latency in ps
			int tCKmed;
			/// Data access time at medium CAS latency in ps
			int tACmed;
			/// Clock cycle time at short CAS latency in ps
			int tCKshort;
			/// Data access time at short CAS latency in ps
			int tACshort;
			/// Maximum clock cycle time in ps
			int tCKmax;

			/// Address/Command Setup Time before clock in ps
			int tIS;
			/// Address/Command Hold Time after clock in ps
			int tIH;
			/// Data Input Setup Time before strobe in ps
			int tDS;
			/// Data Input Hold Time after strobe in ps
			int tDH;

			/// Maximum skew between DQS and DQ signals in ps
			int tDQSQ;
			/// Maximum Read Data Hold Skew Factor in ps
			int tQHS;

			/// CS latencies supported
			CHAR16 CSSupported[SHORT_STRING_LEN];
			/// WE latencies supported
			CHAR16 WESupported[SHORT_STRING_LEN];

			/// Burst Lengths supported
			CHAR16 BurstLengthsSupported[SHORT_STRING_LEN];
			/// Refresh rate
			CHAR16 RefreshRate[SHORT_STRING_LEN];

			/// TRUE for buffered address/control inputs
			bool buffered;
			/// TRUE for on-card PLL
			bool OnCardPLL;
			/// TRUE for FET Switch On-Card Enable
			bool FETOnCardEnable;
			/// TRUE for FET Switch External Enable
			bool FETExtEnable;
			/// TRUE for Differential Clock Input
			bool DiffClockInput;

			/// TRUE for Weak Driver included
			bool WeakDriverIncluded;
			/// TRUE for Concurrent Auto Precharge supported
			bool ConcAutoPrecharge;
			/// TRUE for Fast AP supported
			bool FastAPSupported;

			/// Module bank density
			CHAR16 bankDensity[VSHORT_STRING_LEN];
			/// Module height
			CHAR16 moduleHeight[VSHORT_STRING_LEN];
		} DDR1SDRAM;
		/// DDR2-specific attributes
		struct DDR2SDRAMINFO
		{
			/// Data access time from clock
			int tAC;
			/// Clock cycle time at medium CAS latency in ps
			int tCKmed;
			/// Data access time at medium CAS latency in ps
			int tACmed;
			/// Clock cycle time at short CAS latency in ps
			int tCKshort;
			/// Data access time at short CAS latency in ps
			int tACshort;
			/// Maximum clock cycle time in ps
			int tCKmax;

			/// Write recovery time in ps
			int tWR;
			/// Internal write to read command delay in ps
			int tWTR;
			/// Internal read to precharge command delay in ps
			int tRTP;

			/// Address/Command Setup Time before clock in ps
			int tIS;
			/// Address/Command Hold Time after clock in ps
			int tIH;
			/// Data Input Setup Time before strobe in ps
			int tDS;
			/// Data Input Hold Time after strobe in ps
			int tDH;

			/// Maximum skew between DQS and DQ signals in ps
			int tDQSQ;
			/// Maximum Read Data Hold Skew Factor in ps
			int tQHS;
			/// PLL Relock Time in ps
			int tPLLRelock;

			/// DRAM Package type
			CHAR16 DRAMPackage[VSHORT_STRING_LEN];

			/// Burst Lengths supported
			CHAR16 BurstLengthsSupported[SHORT_STRING_LEN];
			/// Refresh rate
			CHAR16 RefreshRate[SHORT_STRING_LEN];

			/// Number of PLLs on DIMM
			int numPLLs;
			/// TRUE for FET Switch External Enable
			bool FETExtEnable;
			/// TRUE for Analysis probe installed
			bool AnalysisProbeInstalled;
			/// TRUE for Weak Driver supported
			bool WeakDriverSupported;
			/// TRUE for 50 ohm ODT supported
			bool _50ohmODTSupported;
			/// TRUE for Partial Array Self Refresh supported
			bool PASRSupported;

			/// Module type
			CHAR16 moduleType[SHORT_STRING_LEN];
			/// Module height
			CHAR16 moduleHeight[VSHORT_STRING_LEN];

			/// TRUE if Enhanced Performance Profiles (EPP) supported
			bool EPPSupported;

			/// EPP-specific attributes
			struct EPPInfo
			{
				/// EPP Profile Type. See #define \link #SPDINFO_DDR2_EPP_PROFILE_ABBR SPDINFO_MEMTYPE_* \endlink
				int profileType;
				/// EPP Profile that should be loaded to maximize system performance
				int optimalProfile;

				/// EPP Profile attributes
				union PROFILEINFO
				{
					/// EPP Abbreviated Profile attributes
					struct ABBRPROFILEINFO
					{
						/// TRUE if this profile contains valid data
						bool enabled;
						/// The voltage level required for this profile
						CHAR16 voltageLevel[SHORT_STRING_LEN];
						/// Clock speed in MHz
						int clkspeed;
						/// Address command rate. 1T - commands can be sent on every clock edge. 2T - commands can be sent on every other clock edge.
						int cmdRate;

						/// CAS latency supported
						int CASSupported;
						/// Minimum clock cycle time in ps
						int tCK;

						/// Minimum RAS to CAS delay in ps
						int tRCD;
						/// Minimum Row Precharge time in ps
						int tRP;
						/// Minimum Active to Precharge Time in ps
						int tRAS;

					} abbrProfile[4];
					struct FULLPROFILEINFO
					{
						/// TRUE if this profile contains valid data
						bool enabled;
						/// Voltage level required for this profile
						CHAR16 voltageLevel[SHORT_STRING_LEN];
						/// Clock speed in MHz
						int clkspeed;
						/// Address command rate. 1T - commands can be sent on every clock edge. 2T - commands can be sent on every other clock edge.
						int cmdRate;

						/// Address Drive Strength
						CHAR16 addrDriveStrength[VSHORT_STRING_LEN];
						/// Chip Select Drive Strength
						CHAR16 CSDriveStrength[VSHORT_STRING_LEN];
						/// Clock Drive Strength
						CHAR16 clockDriveStrength[VSHORT_STRING_LEN];
						/// Data Drive Strength
						CHAR16 dataDriveStrength[VSHORT_STRING_LEN];
						/// DQS Drive Strength
						CHAR16 DQSDriveStrength[VSHORT_STRING_LEN];
						/// Address/Command Fine Delay
						CHAR16 addrCmdFineDelay[VSHORT_STRING_LEN];
						/// Address/Command Setup Time
						CHAR16 addrCmdSetupTime[VSHORT_STRING_LEN];
						/// Chip Select Fine Delay
						CHAR16 CSFineDelay[VSHORT_STRING_LEN];
						/// Chip Select Setup Time
						CHAR16 CSSetupTime[VSHORT_STRING_LEN];

						/// CAS latency supported
						int CASSupported;
						/// Minimum clock cycle time in ps
						int tCK;

						/// Minimum RAS to CAS delay in ps
						int tRCD;
						/// Minimum Row Precharge time in ps
						int tRP;
						/// Minimum Active to Precharge Time in ps
						int tRAS;

						/// Write recovery time in ps
						int tWR;
						/// Minimum Auto-Refresh to Active/Auto-Refresh Time in ps
						int tRC;

					} fullProfile[2];
				} profileData;
			} EPP;

		} DDR2SDRAM;
		/// DDR2FB-specific attributes
		struct DDR2FBSDRAMINFO
		{
			/// Maximum clock cycle time in ps
			int tCKmax;
			/// Write recovery time in ps
			int tWR;
			/// Internal write to read command delay in ps
			int tWTR;
			/// Internal Read to Precharge Command Delay in ps
			int tRTP;
			/// Minimum Four Activate Window Delay in ps
			int tFAW;

			/// Write Recovery Times Supported
			CHAR16 WRSupported[SHORT_STRING_LEN];
			/// Write Latencies Supported
			CHAR16 WESupported[SHORT_STRING_LEN];
			/// Additive Latencies Supported
			CHAR16 ALSupported[SHORT_STRING_LEN];

			/// Burst Lengths supported
			CHAR16 BurstLengthsSupported[SHORT_STRING_LEN];
			/// Refresh rate
			CHAR16 RefreshRate[SHORT_STRING_LEN];
			/// Terminations Supported
			CHAR16 TerminationsSupported[SHORT_STRING_LEN];
			/// Drivers Supported
			CHAR16 DriversSupported[SHORT_STRING_LEN];

			/// Module type
			CHAR16 moduleType[SHORT_STRING_LEN];
			/// Module height
			CHAR16 moduleHeight[VSHORT_STRING_LEN];
			/// Module thickness
			CHAR16 moduleThickness[VSHORT_STRING_LEN];

			/// DRAM manufacture ID
			unsigned char DRAMManufID;
			/// DRAM manufacture bank
			int DRAMManufBank;
			/// DRAM manufacture name
			wchar_t DRAMManuf[SHORT_STRING_LEN];
		} DDR2FBSDRAM;
		/// DDR3-specific attributes
		struct DDR3SDRAMINFO
		{
			/// Write recovery time in ps
			int tWR;
			/// Internal write to read command delay in ps
			int tWTR;
			/// Internal Read to Precharge Command Delay in ps
			int tRTP;
			/// Minimum Four Activate Window Delay in ps
			int tFAW;
			/// Maximum Activate Window in units of tREFI
			int tMAW;

			/// TRUE for RZQ / 6 supported
			bool RZQ6Supported;
			/// TRUE for RZQ / 7 supported
			bool RZQ7Supported;
			/// TRUE for DLL-Off Mode supported
			bool DLLOffModeSupported;
			/// Maximum operating temperature
			int OperatingTempRange;
			/// Refresh rate at extended operating temperature range
			int RefreshRateExtTempRange;
			/// TRUE forAuto self refresh supported
			bool autoSelfRefresh;
			/// TRUE forOn-die thermal sensor readout supported
			bool onDieThermalSensorReadout;
			/// TRUE forPartial Array Self Refresh supported
			bool partialArraySelfRefresh;
			/// TRUE for thermal sensor present
			bool thermalSensorPresent;

			/// Non-standard SDRAM type
			CHAR16 nonStdSDRAMType[SHORT_STRING_LEN];

			/// Maxium Activate Count (MAC)
			CHAR16 maxActivateCount[VSHORT_STRING_LEN];

			/// Module type
			CHAR16 moduleType[SHORT_STRING_LEN];
			/// Module height
			int moduleHeight;
			/// Module thickness (front)
			int moduleThicknessFront;
			/// Module thickness (back)
			int moduleThicknessBack;
			/// Module width
			CHAR16 moduleWidth[VSHORT_STRING_LEN];

			/// Reference raw card used
			CHAR16 moduleRefCard[SHORT_STRING_LEN];
			/// DRAM manufacture ID
			unsigned char DRAMManufID;
			/// DRAM manufacture bank
			int DRAMManufBank;
			/// DRAM manufacture name
			wchar_t DRAMManuf[SHORT_STRING_LEN];
			/// Number of DRAM rows
			int numDRAMRows;
			/// Number of registers
			int numRegisters;
			/// TRUE if Heat Spreader Solution is present
			bool heatSpreaderSolution;
			/// Register manufacturer ID
			unsigned char regManufID;
			/// Register manufacturer bank
			int regManufBank;
			/// Regoister manufacture name
			wchar_t regManuf[SHORT_STRING_LEN];
			/// Register type
			wchar_t regDeviceType[VSHORT_STRING_LEN];
			/// Register revision
			int regRev;

			/// Command/Address A Outputs Drive Strength
			wchar_t cmdAddrADriveStrength[SHORT_STRING_LEN];
			/// Command/Address B Outputs Drive Strength
			wchar_t cmdAddrBDriveStrength[SHORT_STRING_LEN];
			/// Control Signals A Outputs Drive Strength
			wchar_t ctrlSigADriveStrength[SHORT_STRING_LEN];
			/// Control Signals B Outputs Drive Strength
			wchar_t ctrlSigBDriveStrength[SHORT_STRING_LEN];
			/// Y1/Y1# and Y3/Y3# Clock Outputs Drive Strength
			wchar_t clkY1Y3DriveStrength[SHORT_STRING_LEN];
			/// Y0/Y0# and Y2/Y2# Clock Outputs Drive Strength
			wchar_t clkY0Y2DriveStrength[SHORT_STRING_LEN];

			/// TRUE if Intel Extreme Memory Profile (XMP) supported
			bool XMPSupported;

			/// XMP-specific attributes
			struct XMPInfo
			{
				/// Intel Extreme Memory Profile Revision
				int revision;

				/// XMP profile attributes
				struct XMPProfile
				{
					/// TRUE if this profile contains valid data
					bool enabled;

					/// Recommended number of DIMMs per channel
					int dimmsPerChannel;

					/// Module VDD Voltage Level
					CHAR16 moduleVdd[SHORT_STRING_LEN];
					/// Clock speed in MHz
					int clkspeed;

					/// CAS latencies supported
					CHAR16 CASSupported[SHORT_STRING_LEN];
					/// Minimum clock cycle time in ps
					int tCK;
					/// Minimum CAS latency time in ps
					int tAA;
					/// Minimum RAS to CAS delay in ps
					int tRCD;
					/// Minimum Row Precharge time in ps
					int tRP;
					/// Minimum Active to Precharge Time in ps
					int tRAS;
					/// Minimum Row Active to Row Active delay in ps
					int tRRD;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Time in ps
					int tRC;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
					int tRFC;

					/// Write recovery time in ps
					int tWR;
					/// Internal write to read command delay in ps
					int tWTR;
					/// Internal Read to Precharge Command Delay in ps
					int tRTP;
					/// Minimum Four Activate Window Delay in ps
					int tFAW;

					/// Minimum CAS Write Latency Time in ps
					int tCWL;
					/// Maximum REFI (Average Periodic Refresh Interval) in us
					int tREFI;

					/// Write to Read CMD Turn-around Time Optimizations
					CHAR16 WRtoRDTurnaround[SHORT_STRING_LEN];

					/// Read to Write CMD Turn-around Time Optimizations
					CHAR16 RDtoWRTurnaround[SHORT_STRING_LEN];

					/// Back to Back CMD Turn-around Time Optimizations
					CHAR16 back2BackTurnaround[SHORT_STRING_LEN];

					/// System CMD Rate Mode
					int cmdRateMode;

					/// Memory Controller Voltage Level
					CHAR16 memCtrlVdd[SHORT_STRING_LEN];
				} profile[2];
			} XMP;
		} DDR3SDRAM;
		/// DDR4-specific attributes
		struct DDR4SDRAMINFO
		{
			/// Maximum clock cycle time in ps
			int tCKmax;
			/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
			int tRFC2;
			/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
			int tRFC4;
			/// Minimum Activate to Activate Delay Time (tRRD_Smin), different bank group
			int tRRD_S;
			/// Minimum Activate to Activate Delay Time (tRRD_Lmin), same bank group
			int tRRD_L;
			/// Minimum CAS to CAS Delay Time (tCCD_Lmin), same bank group
			int tCCD_L;
			/// Minimum Four Activate Window Delay in ps
			int tFAW;
			/// Maximum Activate Window in units of tREFI
			int tMAW;

			/// TRUE for thermal sensor present
			bool thermalSensorPresent;

			/// DRAM Stepping
			int DRAMStepping;

			/// SDRAM Package Type
			struct
			{
				/// TRUE if Monolithic DRAM Device
				bool monolithic;
				/// Die Count
				int dieCount;
				/// Signal Loading
				bool multiLoadStack;
			} SDRAMPkgType;

			/// SDRAM Package Type
			struct
			{
				/// TRUE if Monolithic DRAM Device
				bool monolithic;
				/// Die Count
				int dieCount;
				/// DRAM Density Ratio (Number of standard device densities rank 1 and 3 is smaller than rank 0 and 2)
				int DRAMDensityRatio;
				/// Signal Loading
				bool multiLoadStack;
			} SecSDRAMPkgType;

			/// Maxium Activate Count (MAC)
			wchar_t maxActivateCount[VSHORT_STRING_LEN];

			/// TRUE if Post Package Repair is supported
			bool PPRSupported;

			/// TRUE if Soft Post Package Repair is supported
			bool SoftPPRSupported;

			/// Cyclical Redundancy Code (CRC) for Base Configuration Section
			unsigned short BaseCfgCRC16;

			/// Module type
			wchar_t moduleType[SHORT_STRING_LEN];

			/// Module height
			int moduleHeight;
			/// Module thickness (front)
			int moduleThicknessFront;
			/// Module thickness (back)
			int moduleThicknessBack;

			/// Reference raw card used
			wchar_t moduleRefCard[SHORT_STRING_LEN];
			/// DRAM manufacture ID
			unsigned char DRAMManufID;
			/// DRAM manufacture bank
			int DRAMManufBank;
			/// DRAM manufacture name
			wchar_t DRAMManuf[SHORT_STRING_LEN];

			/// Number of DRAM rows
			int numDRAMRows;
			/// Number of registers
			int numRegisters;

			/// TRUE if Heat Spreader Solution is present
			bool heatSpreaderSolution;

			/// Register manufacturer ID
			unsigned char regManufID;
			/// Register manufacturer bank
			int regManufBank;
			/// Regoister manufacture name
			wchar_t regManuf[SHORT_STRING_LEN];

			/// Register revision
			int regRev;

			/// Chip Select Drive Strength
			wchar_t CSDriveStrength[SHORT_STRING_LEN];
			/// Command/Address Drive Strength
			wchar_t cmdAddrDriveStrength[SHORT_STRING_LEN];
			/// ODT Drive Strength
			wchar_t ODTDriveStrength[SHORT_STRING_LEN];
			/// CKE Drive Strength
			wchar_t CKEDriveStrength[SHORT_STRING_LEN];

			/// Y0,Y2 Drive Strength
			wchar_t Y0Y2DriveStrength[SHORT_STRING_LEN];
			/// Y1,Y3 Drive Strength
			wchar_t Y1Y3DriveStrength[SHORT_STRING_LEN];

			/// Data Buffer revision
			int databufferRev;

			/// MQT Drive Strength for data rate <= 1866
			wchar_t MDQDriveStrength1866[VSHORT_STRING_LEN];
			/// MDQ Read Termination Strength for data rate <= 1866
			wchar_t MDQReadTermStrength1866[VSHORT_STRING_LEN];
			/// DRAM Drive Strength for data rate <= 1866
			wchar_t DRAMDriveStrength1866[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_WR) for data rate <= 1866
			wchar_t ODTRttWR1866[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_NOM) for data rate <= 1866
			wchar_t ODTRttNom1866[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_PARK), Package Ranks 0 & 1 for data rate <= 1866
			wchar_t ODTRttPARK1866_R0R1[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_PARK), Package Ranks 2 & 3 for data rate <= 1866
			wchar_t ODTRttPARK1866_R2R3[VSHORT_STRING_LEN];

			/// MQT Drive Strength for 1866 < data rate <= 2400
			wchar_t MDQDriveStrength2400[VSHORT_STRING_LEN];
			/// MDQ Read Termination Strength for 1866 < data rate <= 2400
			wchar_t MDQReadTermStrength2400[VSHORT_STRING_LEN];
			/// DRAM Drive Strength for data rate for 1866 < data rate <= 2400
			wchar_t DRAMDriveStrength2400[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_WR) for 1866 < data rate <= 2400
			wchar_t ODTRttWR2400[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_NOM) for 1866 < data rate <= 2400
			wchar_t ODTRttNom2400[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_PARK), Package Ranks 0 & 1 for 1866 < data rate <= 2400
			wchar_t ODTRttPARK2400_R0R1[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_PARK), Package Ranks 2 & 3 for 1866 < data rate <= 2400
			wchar_t ODTRttPARK2400_R2R3[VSHORT_STRING_LEN];

			/// MQT Drive Strength for 2400 < data rate <= 3200
			wchar_t MDQDriveStrength3200[VSHORT_STRING_LEN];
			/// MDQ Read Termination Strength for 2400 < data rate <= 3200
			wchar_t MDQReadTermStrength3200[VSHORT_STRING_LEN];
			/// DRAM Drive Strength for data rate for 2400 < data rate <= 3200
			wchar_t DRAMDriveStrength3200[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_WR) for 2400 < data rate <= 3200
			wchar_t ODTRttWR3200[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_NOM) for 2400 < data rate <= 3200
			wchar_t ODTRttNom3200[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_PARK), Package Ranks 0 & 1 for 2400 < data rate <= 3200
			wchar_t ODTRttPARK3200_R0R1[VSHORT_STRING_LEN];
			/// DRAM ODT Strength (RTT_PARK), Package Ranks 2 & 3 for 2400 < data rate <= 3200
			wchar_t ODTRttPARK3200_R2R3[VSHORT_STRING_LEN];

			/// Cyclical Redundancy Code (CRC) for Module Specific Section
			unsigned short ModuleCRC16;

			/// TRUE if Intel Extreme Memory Profile (XMP) supported
			bool XMPSupported;

			/// XMP-specific attributes
			struct
			{
				/// Intel Extreme Memory Profile Revision
				int revision;

				/// XMP profile attributes
				struct
				{
					/// TRUE if this profile contains valid data
					bool enabled;

					/// Recommended number of DIMMs per channel
					int dimmsPerChannel;

					/// Module VDD Voltage Level
					CHAR16 moduleVdd[SHORT_STRING_LEN];
					/// Clock speed in MHz
					int clkspeed;

					/// CAS latencies supported
					CHAR16 CASSupported[SHORT_STRING_LEN];
					/// Minimum clock cycle time in ps
					int tCK;
					/// Minimum CAS latency time in ps
					int tAA;
					/// Minimum RAS to CAS delay in ps
					int tRCD;
					/// Minimum Row Precharge time in ps
					int tRP;
					/// Minimum Active to Precharge Time in ps
					int tRAS;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Time in ps
					int tRC;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
					int tRFC1;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
					int tRFC2;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
					int tRFC4;
					/// Minimum Four Activate Window Delay in ps
					int tFAW;
					/// Minimum Activate to Activate Delay Time (tRRD_Smin), different bank group
					int tRRD_S;
					/// Minimum Activate to Activate Delay Time (tRRD_Lmin), same bank group
					int tRRD_L;

				} profile[2];
			} XMP;
		} DDR4SDRAM;
		/// DDR5-specific attributes
		struct DDR5SDRAMINFO
		{
			/// Maximum clock cycle time in ps
			int tCKmax;
			/// Write recovery time in ps
			int tWR;
			/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
			int tRFC2;
			/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps
			int tRFC4;

			/// Minimum Refresh Recovery Delay Time, 3DS Different Logical Rank in ps
			int tRFC1_dlr;
			/// Minimum Refresh Recovery Delay Time, 3DS Different Logical Rank in ps
			int tRFC2_dlr;
			/// Minimum Refresh Recovery Delay Time, 3DS Different Logical Rank in ps
			int tRFCsb_dlr;

			union
			{
				struct
				{
					unsigned int baseModuleType : 4;
					unsigned int hybridMedia : 3;
					unsigned int hybrid : 1;
				} bits;
				unsigned char raw;
			} keyByteModuleType;

			/// Module type
			CHAR16 moduleType[SHORT_STRING_LEN];

			union
			{
				struct
				{
					unsigned int densityPerDie : 5;
					unsigned int diePerPackage : 3;
				} bits;
				unsigned char raw;
			} FirstSDRAMDensityPackage;

			union
			{
				struct
				{
					unsigned int rowAddrBits : 5;
					unsigned int colAddrBits : 3;
				} bits;
				unsigned char raw;
			} FirstSDRAMAddressing;

			union
			{
				struct
				{
					unsigned int reserved : 5;
					unsigned int IOWidth : 3;
				} bits;
				unsigned char raw;
			} FirstSDRAMIOWidth;

			union
			{
				struct
				{
					unsigned int banksPerGroup : 3;
					unsigned int reserved : 2;
					unsigned int bankGroups : 3;
				} bits;
				unsigned char raw;
			} FirstSDRAMBankGroups;

			union
			{
				struct
				{
					unsigned int densityPerDie : 5;
					unsigned int diePerPackage : 3;
				} bits;
				unsigned char raw;
			} SecondSDRAMDensityPackage;

			union
			{
				struct
				{
					unsigned int rowAddrBits : 5;
					unsigned int colAddrBits : 3;
				} bits;
				unsigned char raw;
			} SecondSDRAMAddressing;

			union
			{
				struct
				{
					unsigned int reserved : 5;
					unsigned int IOWidth : 3;
				} bits;
				unsigned char raw;
			} SecondSDRAMIOWidth;

			union
			{
				struct
				{
					unsigned int banksPerGroup : 3;
					unsigned int reserved : 2;
					unsigned int bankGroups : 3;
				} bits;
				unsigned char raw;
			} SecondSDRAMBankGroups;

			union
			{
				struct
				{
					unsigned int mPPRhPPRAbort : 1;
					unsigned int MBISTmPPR : 1;
					unsigned int reserved32 : 2;
					unsigned int BL32 : 1;
					unsigned int sPPRUndoLock : 1;
					unsigned int reserved6 : 1;
					unsigned int sPPRGranularity : 1;
				} bits;
				unsigned char raw;
			} BL32PostPackageRepair;

			union
			{
				struct
				{
					unsigned int DCATypesSupported : 2;
					unsigned int reserved32 : 2;
					unsigned int PASR : 1;
					unsigned int reserved75 : 3;
				} bits;
				unsigned char raw;
			} DutyCycleAdjusterPartialArraySelfRefresh;

			union
			{
				struct
				{
					unsigned int BoundedFault : 1;
					unsigned int x4RMWECSWritebackSuppressionMRSelector : 1;
					unsigned int x4RMWECSWritebackSuppression : 1;
					unsigned int reserved73 : 5;
				} bits;
				unsigned char raw;
			} FaultHandling;

			union
			{
				struct
				{
					unsigned int endurant : 2;
					unsigned int operable : 2;
					unsigned int nominal : 4;
				} bits;
				unsigned char raw;
			} NominalVoltageVDD;

			union
			{
				struct
				{
					unsigned int endurant : 2;
					unsigned int operable : 2;
					unsigned int nominal : 4;
				} bits;
				unsigned char raw;
			} NominalVoltageVDDQ;

			union
			{
				struct
				{
					unsigned int endurant : 2;
					unsigned int operable : 2;
					unsigned int nominal : 4;
				} bits;
				unsigned char raw;
			} NominalVoltageVPP;

			union
			{
				struct
				{
					unsigned int RFMRequired : 1;
					unsigned int RAAIMT : 4;
					unsigned int RAAMMT : 3;
					unsigned int reserved : 4;
					unsigned int ARFMLevel : 2;
					unsigned int RFMRAACounterDecrementPerREFcommand : 2;
				} bits;
				unsigned char raw[2];
			} FirstSDRAMRefreshManagement;

			union
			{
				struct
				{
					unsigned int RFMRequired : 1;
					unsigned int RAAIMT : 4;
					unsigned int RAAMMT : 3;
					unsigned int reserved : 4;
					unsigned int ARFMLevel : 2;
					unsigned int RFMRAACounterDecrementPerREFcommand : 2;
				} bits;
				unsigned char raw[2];
			} SecondSDRAMRefreshManagement;

			union ARFM
			{
				struct
				{
					unsigned int ARFMLevelSupported : 1;
					unsigned int RAAIMT : 4;
					unsigned int RAAMMT : 3;
					unsigned int reserved : 4;
					unsigned int ARFMLevel : 2;
					unsigned int RFMRAACounterDecrementPerREFcommand : 2;
				} bits;
				unsigned char raw[2];
			} FirstSDRAMAdaptiveRefreshManagementLevelA;

			union ARFM FirstSDRAMAdaptiveRefreshManagementLevelB;
			union ARFM FirstSDRAMAdaptiveRefreshManagementLevelC;

			union ARFM SecondSDRAMAdaptiveRefreshManagementLevelA;
			union ARFM SecondSDRAMAdaptiveRefreshManagementLevelB;
			union ARFM SecondSDRAMAdaptiveRefreshManagementLevelC;

			/// SPD revision
			int moduleSPDRev;

			struct
			{
				DEVINFO SPD;
				DEVINFO PMIC0;
				DEVINFO PMIC1;
				DEVINFO PMIC2;
				DEVINFO ThermalSensors;
			} moduleDeviceInfo;

			/// Module height
			int moduleHeight;
			/// Module thickness (front)
			int moduleThicknessFront;
			/// Module thickness (back)
			int moduleThicknessBack;

			union
			{
				struct
				{
					unsigned int ReferenceDesign : 5;
					unsigned int DesignRevision : 3;
				} bits;
				unsigned char raw;
			} ReferenceRawCardUsed;
			/// Reference raw card used
			CHAR16 moduleRefCard[SHORT_STRING_LEN];

			/// DRAM manufacture ID
			unsigned char DRAMManufID;
			/// DRAM manufacture bank
			int DRAMManufBank;
			/// DRAM manufacture name
			wchar_t DRAMManuf[SHORT_STRING_LEN];

			/// DRAM Stepping
			int DRAMStepping;

			union
			{
				struct
				{
					unsigned int NumDRAMRows : 2;
					unsigned int HeatSpreader : 1;
					unsigned int reserved : 1;
					unsigned int OperatingTemperatureRange : 4;
				} bits;
				unsigned char raw;
			} DIMMAttributes;

			union
			{
				struct
				{
					unsigned int reserved20 : 3;
					unsigned int NumPackageRanksPerSubChannel : 3;
					unsigned int RankMix : 1;
					unsigned int reserved7 : 1;
				} bits;
				unsigned char raw;
			} ModuleOrganization;

			union
			{
				struct
				{
					unsigned int PrimaryBusWidthPerSubChannel : 3;
					unsigned int BusWidthExtensionPerSubChannel : 2;
					unsigned int NumSubChannelsPerDIMM : 2;
					unsigned int reserved : 1;
				} bits;
				unsigned char raw;
			} MemoryChannelBusWidth;

			union
			{
				struct
				{
					DEVINFO RegisteringClockDriver;
					DEVINFO DataBuffers;

					union
					{
						struct
						{
							unsigned int QACK_tQACK_c : 1;
							unsigned int QBCK_tQBCK_c : 1;
							unsigned int QCCK_tQCCK_c : 1;
							unsigned int QDCK_tQDCK_c : 1;
							unsigned int reserved4 : 1;
							unsigned int BCK_tBCK_c : 1;
							unsigned int reserved76 : 2;
						} bits;
						unsigned char raw;
					} RCDRW08ClockDriverEnable;

					union
					{
						struct
						{
							unsigned int QACAOutputs : 1;
							unsigned int QBCAOutputs : 1;
							unsigned int DCS1_nInputBufferQxCS1_nOutputs : 1;
							unsigned int BCS_nBCOM20BRST_nOutputs : 1;
							unsigned int QBACA13OutputDriver : 1;
							unsigned int QACS10_nOutput : 1;
							unsigned int QBCS10_nOutput : 1;
							unsigned int reserved : 1;
						} bits;
						unsigned char raw;
					} RCDRW09OutputAddressControlEnable;

					union
					{
						struct
						{
							unsigned int QACK_tQACK_c : 2;
							unsigned int QBCK_tQBCK_c : 2;
							unsigned int QCCK_tQCCK_c : 2;
							unsigned int QDCK_tQDCK_c : 2;
						} bits;
						unsigned char raw;
					} RCDRW0AQCKDriverCharacteristics;

					union
					{
						struct
						{
							unsigned int reserved : 8;
						} bits;
						unsigned char raw;
					} RCDRW0B;

					union
					{
						struct
						{
							unsigned int DriverStrengthAddressCommandABOutputs : 2;
							unsigned int reserved32 : 2;
							unsigned int DriverStrengthQxCS0_nQxCS1_nQCCK_tQCCK_c : 2;
							unsigned int reserved76 : 2;
						} bits;
						unsigned char raw;
					} RCDRW0CQxCAQxCS_nDriverCharacteristics;

					union
					{
						struct
						{
							unsigned int DriverStrengthBCOM20BCS_n : 2;
							unsigned int reserved2 : 1;
							unsigned int DriverStrengthBCK_tBCK_c : 2;
							unsigned int reserved75 : 3;
						} bits;
						unsigned char raw;
					} RCDRW0DDataBufferInterfaceDriverCharacteristics;

					union
					{
						struct
						{
							unsigned int QCKDA_tQCKDA_cDifferentialSlewRate : 2;
							unsigned int QBACA130SingleEndedSlewRate : 2;
							unsigned int QBACS10_nSingleEndedSlewRate : 2;
							unsigned int reserved76 : 2;
						} bits;
						unsigned char raw;
					} RCDRW0EQCKQCAQCOutputSlewRate;

					union
					{
						struct
						{
							unsigned int BCOM20BCS_nSingleEndedSlewRate : 2;
							unsigned int BCK_tBCK_cDifferentialSlewRate : 2;
							unsigned int reserved74 : 4;
						} bits;
						unsigned char raw;
					} RCDRW0FBCKBCOMBCSOutputSlewRate;

					union
					{
						struct
						{
							unsigned int DQSRTTParkTermination : 3;
							unsigned int reserved73 : 5;
						} bits;
						unsigned char raw;
					} DBRW86DQSRTTParkTermination;

				} RDIMM;
				struct
				{
					DEVINFO DifferentialMemoryBuffer;
				} DDIMM;
				struct
				{
					DEVINFO RegisteringClockDriver;
					DEVINFO DataBuffers;
				} NVDIMMN;
				struct
				{
					DEVINFO RegisteringClockDriver;
					DEVINFO DataBuffers;
				} NVDIMMP;
			} baseModule;

			/// Cyclical Redundancy Code (CRC) for SPD Bytes 0~509
			unsigned short SPD509CRC16;

			/// TRUE if Intel Extreme Memory Profile (XMP) supported
			bool XMPSupported;

			/// XMP-specific attributes
			struct
			{
				/// Intel Extreme Memory Profile Version
				unsigned char version;

				/// PMIC Vendor ID
				unsigned char PMICVendorID[2];

				/// Number of PMICs
				int numPMIC;

				/// PMIC Capabilities
				union
				{
					struct
					{
						unsigned int OCCapable : 1;
						unsigned int OCEnable : 1;
						unsigned int VoltageDefaultStepSize : 1;
						unsigned int OCGlobalResetEnable : 1;
						unsigned int reserved75 : 3;
					} bits;
					unsigned char raw;
				} PMICCapabilities;

				/// Validation and Certification Capabilities
				union
				{
					struct
					{
						unsigned int SelfCertified : 1;
						unsigned int PMICIntelAVLValidated : 1;
						unsigned int reserved72 : 6;
					} bits;
					unsigned char raw;
				} ValidationCertification;

				/// Intel Extreme Memory Profile Spec Revision
				unsigned char revision;

				/// Cyclical Redundancy Code (CRC) for Base Configuration Section
				unsigned short BaseCfgCRC16;

				/// XMP profile attributes
				struct
				{
					/// TRUE if this profile contains valid data
					bool enabled;

					/// 0 if this profile is XMP ready, 1 if this profile is XMP certified
					int certified;

					/// Recommended number of DIMMs per channel
					int dimmsPerChannel;

					/// Profile string name
					char name[SHORT_STRING_LEN];

					/// Module VPP Voltage Level
					CHAR16 moduleVPP[SHORT_STRING_LEN];

					/// Module VDD Voltage Level
					CHAR16 moduleVDD[SHORT_STRING_LEN];

					/// Module VDDQ Voltage Level
					CHAR16 moduleVDDQ[SHORT_STRING_LEN];

					/// Memory Controller Voltage Level
					CHAR16 memCtrlVoltage[SHORT_STRING_LEN];

					/// Clock speed in MHz
					int clkspeed;

					/// CAS latencies supported
					CHAR16 CASSupported[SHORT_STRING_LEN];
					/// Minimum clock cycle time in ps
					int tCK;
					/// Minimum CAS latency time in ps
					int tAA;
					/// Minimum RAS to CAS delay in ps
					int tRCD;
					/// Minimum Row Precharge time in ps
					int tRP;
					/// Minimum Active to Precharge Time in ps
					int tRAS;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Time in ps
					int tRC;
					/// Minimum Write Recovery Time in ps
					int tWR;
					/// SDRAM Minimum Refresh Recovery Delay Time in ps
					int tRFC1;
					/// SDRAM Minimum Refresh Recovery Delay Time in ps
					int tRFC2;
					/// SDRAM Minimum Refresh Recovery Delay Time in ps
					int tRFCsb;
					/// SDRAM Minimum Read to Read Command Delay Time, Same Bank Group in ps
					int tCCD_L;
					/// SDRAM Minimum Read to Read Command Delay Time, Same Bank Group in nCK
					int tCCD_L_nCK;
					/// SDRAM Minimum Write to Write Command Delay Time, Same Bank Group in ps
					int tCCD_L_WR;
					/// SDRAM Minimum Write to Write Command Delay Time, Same Bank Group in nCK
					int tCCD_L_WR_nCK;
					/// SDRAM Minimum Write to Write Command Delay Time, Second Write not RMW, Same Bank Group in ps
					int tCCD_L_WR2;
					/// SDRAM Minimum Write to Write Command Delay Time, Second Write not RMW, Same Bank Group in nCK
					int tCCD_L_WR2_nCK;
					/// SDRAM Minimum Write to Read Command Delay Time, Same Bank Group in ps
					int tCCD_L_WTR;
					/// SDRAM Minimum Write to Read Command Delay Time, Same Bank Group in nCK
					int tCCD_L_WTR_nCK;
					/// SDRAM Minimum Write to Read Command Delay Time, Different Bank Group in ps
					int tCCD_S_WTR;
					/// SDRAM Minimum Write to Read Command Delay Time, Different Bank Group in nCK
					int tCCD_S_WTR_nCK;
					/// SDRAM Minimum Active to Active Command Delay Time, Same Bank Group in ps
					int tRRD_L;
					/// SDRAM Minimum Active to Active Command Delay Time, Same Bank Group in nCK
					int tRRD_L_nCK;
					/// SDRAM Minimum Read to Precharge Command Delay Time in ps
					int tRTP;
					/// SDRAM Minimum Read to Precharge Command Delay Time in nCK
					int tRTP_nCK;
					/// SDRAM Minimum Four Activate Window in ps
					int tFAW;
					/// SDRAM Minimum Four Activate Window in nCK
					int tFAW_nCK;

					/// Advanced Memory Overclocking Features
					union
					{
						struct
						{
							unsigned int RealTimeMemoryFrequencyOverclocking : 1;
							unsigned int IntelDynamicMemoryBoost : 1;
							unsigned int reserved72 : 6;
						} bits;
						unsigned char raw;
					} AdvancedMemoryOverclockingFeatures;

					/// System CMD Rate Mode
					int cmdRateMode;
					/// Vendor Personality Byte
					unsigned char VendorPersonalityByte;
					/// Cyclical Redundancy Code (CRC) for Base Configuration Section
					unsigned short BaseCfgCRC16;

				} profile[MAX_XMP3_PROFILES];
			} XMP;

			/// TRUE if AMD EXtended Profiles for Overclocking (EXPO) supported
			bool EXPOSupported;

			/// EXPO-specific attributes
			struct EXPOInfo
			{
				/// EXPO Version
				unsigned char version;

				/// PMIC Feature Support
				union
				{
					struct
					{
						unsigned int stepSize10mV : 1;
						unsigned int reserved71 : 3;
					} bits;
					unsigned char raw;
				} PMICFeatureSupport;

				struct
				{
					/// TRUE if this profile contains valid data
					bool enabled;

					/// DIMMs/channel supported
					int dimmsPerChannel;

					/// Optional Block 1 Enable
					bool block1Enabled;

					/// SDRAM VDD
					union
					{
						struct
						{
							unsigned int n50mV : 1;
							unsigned int n100mV : 4;
							unsigned int n1V : 2;
							unsigned int reserved7 : 1;
						} bits;
						unsigned char raw;
					} SDRAMVDD;

					/// SDRAM VDDQ
					union
					{
						struct
						{
							unsigned int n50mV : 1;
							unsigned int n100mV : 4;
							unsigned int n1V : 2;
							unsigned int reserved7 : 1;
						} bits;
						unsigned char raw;
					} SDRAMVDDQ;

					/// SDRAM VPP
					union
					{
						struct
						{
							unsigned int n50mV : 1;
							unsigned int n100mV : 4;
							unsigned int n1V : 2;
							unsigned int reserved7 : 1;
						} bits;
						unsigned char raw;
					} SDRAMVPP;

					/// Module VPP Voltage Level
					CHAR16 moduleVPP[SHORT_STRING_LEN];

					/// Module VDD Voltage Level
					CHAR16 moduleVDD[SHORT_STRING_LEN];

					/// Module VDDQ Voltage Level
					CHAR16 moduleVDDQ[SHORT_STRING_LEN];

					/// Clock speed in MHz
					int clkspeed;

					/// Minimum clock cycle time in ns
					int tCK;
					/// Minimum CAS latency time in ns
					int tAA;
					/// Minimum RAS to CAS delay in ns
					int tRCD;
					/// Minimum Row Precharge time in ns
					int tRP;
					/// Minimum Active to Precharge Time in ns
					int tRAS;
					/// Minimum Auto-Refresh to Active/Auto-Refresh Time in ns
					int tRC;
					/// Minimum Write Recovery Time in ns
					int tWR;
					/// SDRAM Minimum Refresh Recovery Delay Time in ns
					int tRFC1;
					/// SDRAM Minimum Refresh Recovery Delay Time in ns
					int tRFC2;
					/// SDRAM Minimum Refresh Recovery Delay Time in ns
					int tRFCsb;

					/// SDRAM Minimum Active to Active Command Delay Time, Same Bank Group in ns
					int tRRD_L;
					/// SDRAM Minimum Read to Read Command Delay Time, Same Bank Group in ns
					int tCCD_L;
					/// SDRAM Minimum Write to Write Command Delay Time, Same Bank Group in ns
					int tCCD_L_WR;
					/// SDRAM Minimum Write to Write Command Delay Time, Second Write not RMW, Same Bank Group in ns
					int tCCD_L_WR2;
					/// SDRAM Minimum Four Activate Window in ns
					int tFAW;
					/// SDRAM Minimum Write to Read Command Delay Time, Same Bank Group in ns
					int tWTR_L;
					/// SDRAM Minimum Write to Read Command Delay Time, Different Bank Group in ns
					int tWTR_S;
					/// SDRAM Minimum Read to Precharge Command Delay Time in ns
					int tRTP;

				} profile[MAX_EXPO_PROFILES];
				unsigned short CRC16;
			} EXPO;
		} DDR5SDRAM;
		/// LPDDR5-specific attributes
		struct LPDDR5SDRAMINFO
		{
			/// Maximum clock cycle time in ps
			int tCKmax;
			/// Minimum Row Precharge Delay Time in ps, Per Bank
			int tRP_pb;
			/// Minimum Auto-Refresh to Active/Auto-Refresh Command Period in ps, Per Bank
			int tRFC_pb;

			union
			{
				struct
				{
					unsigned int baseModuleType : 4;
					unsigned int hybridMedia : 3;
					unsigned int hybrid : 1;
				} bits;
				unsigned char raw;
			} keyByteModuleType;

			/// Module type
			CHAR16 moduleType[SHORT_STRING_LEN];

			union
			{
				struct
				{
					unsigned int capacityPerDie : 4;
					unsigned int bankAddrBits : 2;
					unsigned int bankGroupBits : 2;
				} bits;
				unsigned char raw;
			} SDRAMDensityBanks;

			union
			{
				struct
				{
					unsigned int bankColAddrBits : 3;
					unsigned int rowAddrBits : 3;
					unsigned int reserved : 2;
				} bits;
				unsigned char raw;
			} SDRAMAddressing;

			union
			{
				struct
				{
					unsigned int signalLoadingIndex : 1;
					unsigned int totalPkgDQsDividedbyDRAMDieDataWidth : 3;
					unsigned int diePerPackage : 3;
					unsigned int packageType : 1;
				} bits;
				unsigned char raw;
			} SDRAMPackageType;

			union
			{
				struct
				{
					unsigned int reserved : 5;
					unsigned int softPPR : 1;
					unsigned int PPR : 2;
				} bits;
				unsigned char raw;
			} OptionalSDRAMFeatures;

			union
			{
				struct
				{
					unsigned int DRAMDieDataWidth : 3;
					unsigned int packageRanksPerSubChannel : 3;
					unsigned int byteModeIdentification : 1;
					unsigned int reserved : 1;
				} bits;
				unsigned char raw;
			} LPDDR5ModuleOrganization;

			union
			{
				struct
				{
					unsigned int busWidthBits : 3;
					unsigned int reserved : 5;
				} bits;
				unsigned char raw;
			} SystemSubChannelBusWidth;

			union
			{
				struct
				{
					unsigned int chipSelectLoading : 3;
					unsigned int cmdAddrClockLoading : 3;
					unsigned int dataStrobeMaskLoading : 2;
				} bits;
				unsigned char raw;
			} SignalLoading;

			/// SPD revision
			int moduleSPDRev;

			union
			{
				struct
				{
					unsigned int serialNumberHashingSequence : 3;
					unsigned int reserved : 5;
				} bits;
				unsigned char raw;
			} HashingSequence;

			struct
			{
				DEVINFO SPD;
				DEVINFO PMIC0;
				DEVINFO PMIC1;
				DEVINFO PMIC2;
				DEVINFO ThermalSensors;
			} moduleDeviceInfo;

			/// Module height
			int moduleHeight;
			/// Module thickness (front)
			int moduleThicknessFront;
			/// Module thickness (back)
			int moduleThicknessBack;

			union
			{
				struct
				{
					unsigned int ReferenceDesign : 5;
					unsigned int DesignRevision : 3;
				} bits;
				unsigned char raw;
			} ReferenceRawCardUsed;
			/// Reference raw card used
			CHAR16 moduleRefCard[SHORT_STRING_LEN];

			/// DRAM manufacture ID
			unsigned char DRAMManufID;
			/// DRAM manufacture bank
			int DRAMManufBank;
			/// DRAM manufacture name
			wchar_t DRAMManuf[SHORT_STRING_LEN];

			/// DRAM Stepping
			int DRAMStepping;

			union
			{
				struct
				{
					unsigned int NumDRAMRows : 2;
					unsigned int HeatSpreader : 1;
					unsigned int reserved : 1;
					unsigned int OperatingTemperatureRange : 4;
				} bits;
				unsigned char raw;
			} DIMMAttributes;

			union
			{
				struct
				{
					unsigned int reserved20 : 3;
					unsigned int NumPackageRanksPerSubChannel : 3;
					unsigned int RankMix : 1;
					unsigned int reserved7 : 1;
				} bits;
				unsigned char raw;
			} ModuleOrganization;

			union
			{
				struct
				{
					unsigned int PrimaryBusWidthPerSubChannel : 3;
					unsigned int BusWidthExtensionPerSubChannel : 2;
					unsigned int NumSubChannelsPerDIMM : 2;
					unsigned int reserved : 1;
				} bits;
				unsigned char raw;
			} MemoryChannelBusWidth;

			union
			{
				struct
				{
					DEVINFO ClockDriver0;
					DEVINFO ClockDriver1;
				} CAMM2;
			} baseModule;

			/// Cyclical Redundancy Code (CRC) for SPD Bytes 0~509
			unsigned short SPD509CRC16;
		} LPDDR5SDRAM;
	} specific;

} SPDINFO;

// extern SPDINFO				g_MemoryInfo[MAX_MEMORY_SLOTS];		//Var to hold info from smbus on memory
// extern SPDINFO				*g_MemoryInfo;							//Var to hold info from smbus on memory

extern const CHAR16 *SPD_DEVICE_TYPE_TABLE[];

extern const CHAR16 *PMIC_DEVICE_TYPE_TABLE[];

extern const CHAR16 *TS_DEVICE_TYPE_TABLE[];

extern const CHAR16 *RCD_DEVICE_TYPE_TABLE[];

extern const CHAR16 *DB_DEVICE_TYPE_TABLE[];

extern const CHAR16 *DMB_DEVICE_TYPE_TABLE[];

extern const CHAR16 *OPERATING_TEMP_RANGE[];

extern const CHAR16 *DDR5_DCA_PASR_TABLE[];

extern const CHAR16 *QCK_DRIVER_CHARACTERISTICS_TABLE[];

extern const CHAR16 *SINGLE_ENDED_SLEW_RATE_TABLE[];

extern const CHAR16 *DIFFERENTIAL_SLEW_RATE_TABLE[];

extern const CHAR16 *RCDRW0F_SINGLE_ENDED_SLEW_RATE_TABLE[];

extern const CHAR16 *DQS_RTT_PARK_TERMINATION_TABLE[];

#endif

#define MAX_TSOD_LENGTH 16

#define RW_WRITE 0
#define RW_READ 1

// Intel ICHx
void Intel801SelectSMBusSeg(WORD BaseAddr, WORD seg);
int smbGetSPDIntel801(WORD BaseAddr);
int smbGetTSODIntel801(WORD BaseAddr);
BOOL smbCallBusIntel801(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbBlockDataIntel801(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE *Data, DWORD Len);
BOOL smbWaitForFreeIntel801(WORD BaseAddr);
BOOL smbWaitForEndIntel801(WORD BaseAddr);
BOOL smbWaitForByteDoneIntel801(WORD BaseAddr);
BOOL smbAcquireSMBusIntel801(WORD BaseAddr, DWORD dwTimeout);
BOOL smbReleaseSMBusIntel801(WORD BaseAddr);
BOOL smbSetBankAddrIntel801(WORD BaseAddr, BYTE bBankNo);

int smbGetSPD5Intel801(DWORD Bus, DWORD Dev, DWORD Fun, WORD BaseAddr);
BOOL smbSetSPD5PageAddrIntel801(WORD BaseAddr, BYTE Slave, BYTE Page);
BOOL smbSetSPD5AddrModeIntel801(WORD BaseAddr, BYTE Slave, BYTE AddrMode);
BOOL smbSetSPD5MRIntel801(WORD BaseAddr, BYTE Slave, BYTE MR, BYTE Data);
int smbGetSPD5TSODIntel801(WORD BaseAddr);

int smbGetSPDIntel801_MMIO(BYTE *pbLinAddr);
BOOL smbCallBusIntel801_MMIO(BYTE *pbLinAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbSetBankAddrIntel801_MMIO(BYTE *pbLinAddr, BYTE bBankNo);
BOOL smbWaitForFreeIntel801_MMIO(BYTE *pbLinAddr);
BOOL smbWaitForEndIntel801_MMIO(BYTE *pbLinAddr);
BOOL smbAcquireSMBusIntel801_MMIO(BYTE *pbLinAddr, DWORD dwTimeout);
BOOL smbReleaseSMBusIntel801_MMIO(BYTE *pbLinAddr);

int smbGetSPDIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg);
BOOL smbCallBusIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, BYTE Ch, BYTE bOffset, BYTE bSlot, BOOL bWord, DWORD *Data);
BOOL smbWaitForFreeIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, BYTE Ch);
BOOL smbWaitForEndIntel801_PCI(DWORD Bus, DWORD Dev, DWORD Fun, BYTE Ch);

int smbGetSPDIntelX79_MMIO(DWORD Bus, DWORD Dev, DWORD Fun, PBYTE MMCFGBase, PBYTE BaseAddr);
BOOL smbCallBusIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh, BYTE bDTI, BYTE bOffset, BYTE bSlot, BOOL bWord, BOOL bTSOD, DWORD *Data);
BOOL smbSetBankAddrIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh, BYTE bBankNo);
BOOL smbWaitForFreeIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh);
BOOL smbWaitForEndIntelX79_MMIO(BYTE *pbLinAddr, BYTE bCh);

int smbGetSPDIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun);
int smbGetTSODIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun);
BOOL smbCallBusIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bDTI, BYTE bOffset, BYTE bSlot, BOOL bWord, BOOL bTSOD, DWORD *Data);
BOOL smbSetBankAddrIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bBankNo);
BOOL smbWaitForFreeIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort);
BOOL smbWaitForEndIntelPCU(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort);

int smbGetSPDIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun);
int smbGetTSODIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun);
BOOL smbCallBusIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bDTI, BYTE bOffset, BYTE bSlot, BOOL bWord, BOOL bTSOD, DWORD *Data);
BOOL smbSetBankAddrIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort, BYTE bBankNo);
BOOL smbWaitForFreeIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort);
BOOL smbWaitForEndIntelPCU_IL(DWORD Bus, DWORD Dev, DWORD Fun, DWORD SMBPort);

int smbGetSPDIntel5000_PCI();
BOOL smbCallBusIntel5000_PCI(BYTE bOffset, BYTE bBranch, BYTE bChannel, BYTE bSA, BOOL bWord, DWORD *Data);
BOOL smbWaitForFreeIntel5000_PCI(BYTE bBranch, BYTE bChannel);
BOOL smbWaitForEndIntel5000_PCI(BYTE bBranch, BYTE bChannel);

int smbGetSPDIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg);
BOOL smbCallBusIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, BYTE bOffset, BYTE bSlot, DWORD *Data);
BOOL smbWaitForFreeIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg);
BOOL smbWaitForEndIntel5100_PCI(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg);

// nVidia nForce4
int smbGetSPDNvidia(WORD BaseAddr);
BOOL smbCallBusNvidia(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForEndNvidia(WORD BaseAddr);

// ATI
int smbGetSPDPIIX4(DWORD Bus, DWORD Dev, DWORD Fun, WORD BaseAddr, BYTE byRev);
int smbGetTSODPIIX4(WORD BaseAddr);
BOOL smbCallBusPIIX4(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForFreePIIX4(WORD BaseAddr);
BOOL smbWaitForEndPIIX4(WORD BaseAddr);
BOOL smbSetBankAddrPIIX4(WORD BaseAddr, BYTE bBankNo);

int smbGetSPD5PIIX4(DWORD Bus, DWORD Dev, DWORD Fun, WORD BaseAddr, BYTE byRev);
BOOL smbSetSPD5PageAddrPIIX4(WORD BaseAddr, BYTE Slave, BYTE Page);
BOOL smbSetSPD5AddrModePIIX4(WORD BaseAddr, BYTE Slave, BYTE AddrMode);
BOOL smbSetSPD5MRPIIX4(WORD BaseAddr, BYTE Slave, BYTE MR, BYTE Data);
int smbGetSPD5TSODPIIX4(WORD BaseAddr);

// VIA
int smbGetSPDVia(WORD BaseAddr);
BOOL smbCallBusVia(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForFreeVia(WORD BaseAddr);
BOOL smbWaitForEndVia(WORD BaseAddr);

// AMD
int smbGetSPDAmd756(WORD BaseAddr);
BOOL smbCallBusAmd756(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForFreeAmd756(WORD BaseAddr);
BOOL smbWaitForEndAmd756(WORD BaseAddr);

// SiS96x
int smbGetSPDSiS96x(WORD BaseAddr);
BOOL smbCallBusSiS96x(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForFreeSiS96x(WORD BaseAddr);
BOOL smbWaitForEndSiS96x(WORD BaseAddr);

// SiS968
int smbGetSPDSiS968(WORD BaseAddr);
BOOL smbCallBusSiS968(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForFreeSiS968(WORD BaseAddr);
BOOL smbWaitForEndSiS968(WORD BaseAddr);

// SiS630
int smbGetSPDSiS630(WORD BaseAddr);
BOOL smbCallBusSiS630(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForFreeSiS630(WORD BaseAddr);
BOOL smbWaitForEndSiS630(WORD BaseAddr);

// Ali1563
int smbGetSPDAli1563(WORD BaseAddr);
BOOL smbCallBusAli1563(WORD BaseAddr, BYTE CMD, BYTE Slave, BYTE RW, BYTE Prot, DWORD *Data);
BOOL smbWaitForFreeAli1563(WORD BaseAddr);
BOOL smbWaitForEndAli1563(WORD BaseAddr);

#endif
