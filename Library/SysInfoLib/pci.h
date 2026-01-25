// PassMark SysInfo
//
// Copyright (c) 2005-2016
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
//	PCI.h
//
// Author(s):
//	Ian Robinson, Keith Mah
//
// Description:
//	Functions for retrieving PCI information.
//
// References:
//   http://dl.lm-sensors.org/lm-sensors/files/sensors-detect - maps VendorID:DeviceID to corresponding SMBus driver
//
// History
//	1 June 2005: Initial version
//	20 Oct 2008: BurnInTest specific info	//BIT601000.0014

#ifndef _PCI_H_
#define _PCI_H_

#ifdef WIN32
#include "SysInfo.h"
#include <windows.h>

#define MAX_TSOD_TEMP 256.0
#define MIN_TSOD_TEMP -256.0

#else
#include <Library/PciLib.h>
#include <Library/PciExpressLib.h>
#include "win32_defines.h"

#define MAX_TSOD_TEMP 256000
#define MIN_TSOD_TEMP -256000

typedef struct _TSODINFO
{
	/// Channel number
	int channel;
	/// Slot number
	int slot;
	/// Temperature
	int temperature;
	union
	{
		struct
		{
			/// Capabilities
			WORD wCap;
			///  Configuration
			WORD wCfg;
			///  High Limit
			WORD wHiLim;
			///  Low Limit
			WORD wLoLim;
			///  TCRIT Limit
			WORD wCritLim;
			///  Ambient Temperature
			WORD wAmbTemp;
			///  Manufacture ID
			WORD wManufID;
			///  Device/Revision
			WORD wDevRev;
			///  Vendor-defined
			WORD wVendor[8];
		} DDR4;
		struct
		{
			/// Thermal Sensor Configuration
			BYTE bTSConfig;
			///  Interrupt Configuration
			BYTE bIntConfig;
			///  High Limit
			WORD wHiLim;
			///  Low Limit
			WORD wLoLim;
			///  Critical High Limit
			WORD wCritHiLim;
			///  Critical Low Limit
			WORD wCritLoLim;
			///  Current Temperature
			WORD wCurTemp;
			///  Thermal Sensor Temperature Status
			BYTE bTSStatus;
			///  Hub & Thermal Sensor Error Status
			BYTE bError;
		} DDR5;
	} raw;
} TSODINFO;

extern int g_maxTSODModules;
#define MAX_TSOD_SLOTS g_maxTSODModules

#endif

// #define MAX_RESULT_BUFF_LEN	64000
#define MAX_RESULT_BUFF_LEN 80000 // 70000 //SI101017

// Define actions for the PCI bus
enum
{
	PCI_A_SCAN = 0,	   // Scan complete PCI bus and display results
	PCI_A_SMBUS,	   // Scan the PCI bus until we find the SMBus Controller
	PCI_A_ENABLESMBUS, // Scan the PCI bus to find the LPC interface bridge, and enable the SMBus controller
	PCI_A_TSOD,		   // Scan the PCI bus until we find the TSOD Controller
	PCI_INTEL_PCH_SMBUS
};

// PCI Define types
enum
{
	PCI_T_UNKNOWN,
	PCI_T_SMBUS,
	PCI_T_HOST_BRIDGE_DRAM
};
#define SUS_SYSTEM_OFFSET 0x2c // sybsystem vendor id and device id are at offset 2c and 2e respectively

#define MAX_SMBUS_CTRL 20
#define MAX_TSOD_CTRL 20

typedef enum
{
	SMBUS_NONE = 0,
	SMBUS_INTEL_801,
	SMBUS_INTEL_X79,
	SMBUS_INTEL_5000,
	SMBUS_INTEL_5100,
	SMBUS_INTEL_PCU,
	SMBUS_INTEL_PCU_IL,
	SMBUS_INTEL_801_SPD5,
	SMBUS_PIIX4,
	SMBUS_VIA,
	SMBUS_AMD756,
	SMBUS_NVIDIA,
	SMBUS_SIS96X,
	SMBUS_SIS968,
	SMBUS_SIS630,
	SMBUS_ALI1563
} SMBUS_DRIVER;

typedef enum
{
	TSOD_NONE = 0,
	TSOD_INTEL_801,
	TSOD_INTEL_PCU,
	TSOD_INTEL_PCU_IL,
	TSOD_INTEL_E3,
	TSOD_INTEL_E5,
	TSOD_INTEL_801_SPD5,
	TSOD_PIIX4,
} TSOD_DRIVER;

// Create the structures to get PCI device information
typedef struct _PCIINFO
{
	DWORD dwDeviceType;
	DWORD dwVendor_ID;
	DWORD dwDevice_ID;
	wchar_t tszVendor_Name[SHORT_STRING_LEN];
	wchar_t tszDevice_Name[SHORT_STRING_LEN];
	BYTE byRev;
	BYTE byHeaderType;
	BYTE byBus;
	BYTE byDev;
	BYTE byFun;
	DWORD dwSMB_Address;
	DWORD dwSMB_Address2;
	UINT_PTR MMCFGBase;
	UINT_PTR SMB_Address_MMIO;
	struct
	{
		DWORD dwBus;
		DWORD dwDev;
		DWORD dwFun;
		DWORD dwReg;
	} SMB_Address_PCI;

	SMBUS_DRIVER smbDriver;
	TSOD_DRIVER tsodDriver;
	DWORD dwGPIO_Address;
} PCIINFO;

// Function prototypes
DWORD Get_PCI_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg);
DWORD Set_PCI_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD Val);
BOOL Get_PCIEx_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD *Val);
BOOL Set_PCIEx_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD Val);

// DIMM temperatures
int Get_TSOD_Intel_E5_MMIO(DWORD dwVIDDID, BYTE Bus, BYTE Dev, BYTE Fun, PBYTE MMCFGBase);
int Get_TSOD_Intel_E3_MMIO(DWORD dwVIDDID, BYTE Bus, PBYTE MMCFGBase);

// PCI enumeration
BOOL Scan_PCI(int iScanType, PCIINFO *smbDevice, int *numDevices);
BOOL Find_PCI_SMBus(PCIINFO *smbDevice, int *numDevices);
BOOL Enable_PCI_SMBus(void);
BOOL Enable_Intel801_SMBus(const PCIINFO *smbDevice);
BOOL Find_PCI_TSOD(PCIINFO *tsodDevice, int *numDevices);

BOOL NeedBSODWorkaround();

#endif //_PCI_H_
