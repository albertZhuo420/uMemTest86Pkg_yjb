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
//	PCI.cpp
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

int iTempDebug = 0;

#ifdef WIN32
#pragma optimize("", on)
#include <windows.h>
#include <Windowsx.h>
#include <iostream>

// using namespace std;
#include <map>

#include "cpuinfo.h"
#include "pci.h"
#include "DirectPort_Port32.h"
#include "DirectPort_DirectIo.h"
#include "SysInfoPrivate.h"
#include "SysInfolocalization.h"

// For MemTest86. SysInfo uses fixed size array so don't expand the array
#define expand_tsod_info_array() \
	{                            \
	}

typedef void* PTR;

// #include <stdio.h>
// #include <tchar.h>
#else
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemTestSupportLib.h>
#include <Library/IoLib.h>
#include <uMemTest86.h>
#include "pci.h"

#define SysInfo_IsLogging() TRUE
#define SysInfo_DebugLogWriteLine MtSupportUCDebugWriteLine
#define g_szSITempDebug1 g_wszBuffer
#define _T(x) L##x
#define _tcscpy(dest, src) StrCpyS(dest, sizeof(dest) / sizeof(dest[0]), src)
#define wcslen StrLen
#define wcsstr StrStr
#define _stprintf_s(buf, ...) UnicodeSPrint(buf, sizeof(buf), __VA_ARGS__)
#define memset(buf, val, size) SetMem(buf, size, val)
#define memcpy CopyMem
#define Sleep(x) gBS->Stall(x * 1000)
#define SysInfo_FindPciInfo(...)

typedef UINTN PTR;

// PCIINFO				g_PCIInfo[256][32][8];

extern TSODINFO* g_MemTSODInfo;
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
#endif

#ifdef WIN32
bool IsDebugLog();
void DebugLog(wchar_t* tszDebugString);
#else
#define IsDebugLog SysInfo_IsLogging
#define DebugLog SysInfo_DebugLogWriteLine
#define SysInfo_DebugString g_wszBuffer
#endif

#define INTEL_KABYLAKE_MCHBAR 0x48
#define INTEL_KABYLAKE_MCHBAR_HI 0x4C

#define INTEL_KABYLAKE_DDR_DIMM_THERMAL_CTL 0x5880
#define INTEL_KABYLAKE_DDR_DIMM_THERMAL_STATUS 0x588C
#define INTEL_KABYLAKE_MEM_THERMAL_DPPM_STATUS 0x6204
#define INTEL_KABYLAKE_DDR_DIMM_TEMP_C0 0x58B0
#define INTEL_KABYLAKE_DDR_DIMM_TEMP_C1 0x58B4

#define AMD_ACPI_FCH_PM_DECODEEN 0xFED80300

BOOL Check_Enable_PCI_SMBus(DWORD dwVendorID, DWORD dwDeviceID, DWORD Bus, DWORD Dev, DWORD Fun);

///////////////////////////////////////////////////////////////////////////////////////////
// Generic functions
///////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
DWORD Get_PCI_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
  DWORD Result = ~0;

#if defined(_M_IX86) || defined(_M_X64)
  if (Reg > 0xFF) // If extended configuration space, use DirectIo driver
  {
    if (SysInfo_IsLogging()) // && iTempDebug++ < 1000)
      SysInfo_DebugLogWriteLine(L"DEBUG: Get_PCI_Reg extended config space");
    DirectIo_ReadPCIReg(Bus, Dev, Fun, Reg, &Result);
    return Result;
  }

  if (g_bUseCF8PCIRead)
  {
    if (NeedBSODWorkaround())
      g_bUseCF8PCIRead = false;
  }

  if (g_bUseCF8PCIRead)
  {
    DWORD cc, t;
    int ret;

    // Pack the Bus number, Device number, Function and Register into a DWORD
    cc = 0x80000000;
    cc = cc | ((Bus & 0xFF) << 16);
    cc = cc | ((Dev & 0x1F) << 11);
    cc = cc | ((Fun & 0x07) << 8);
    cc = cc | ((Reg & 0xFC));

    // Save the current address requested (may be in use by another process)
    ret = GetPortVal(0xCF8, (DWORD*)&t, 4); // 4 Bytes
    // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);
    // Read the devideid:vendorid from the PCI config space for the Data
    ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);

    // Restore the current address requested (for another process)
    ret = SetPortVal(0xCF8, t, 4);
  }
  else
  {
    if (SysInfo_IsLogging()) // && iTempDebug++ < 1000)
      SysInfo_DebugLogWriteLine(L"DEBUG: DirectIo read PCI reg");
    DirectIo_ReadPCIReg(Bus, Dev, Fun, Reg, &Result);
    if (SysInfo_IsLogging()) // && iTempDebug++ < 1000)
    {
      _stprintf_s(SysInfo_DebugString, L"DEBUG: Get_PCI_Reg: %08x", Result);
      SysInfo_DebugLogWriteLine(SysInfo_DebugString);
    }
  }
#endif

#if 0
  if (SysInfo_IsLogging() && Result != 0xFFFFFFFF) //SI101076 // && iTempDebug++ < 1000) //SI101065
  {
    _stprintf_s(SysInfo_DebugString, L"DEBUG: Get_PCI_Reg: %d, %u)\n", ret, Result);
    SysInfo_DebugLogWriteLine(SysInfo_DebugString);
  }
#endif
  return Result;
}

DWORD Set_PCI_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD Val)
{
  DWORD Result = ~0;

#if defined(_M_IX86) || defined(_M_X64)
  if (Reg > 0xFF) // If extended configuration space, use DirectIo driver
  {
    DirectIo_WritePCIReg(Bus, Dev, Fun, Reg, Val);
    DirectIo_ReadPCIReg(Bus, Dev, Fun, Reg, &Result);
    return Result;
  }

  if (g_bUseCF8PCIWrite)
  {
    DWORD cc, t;
    int ret;

    // Pack the Bus number, Device number, Function and Register into a DWORD
    cc = 0x80000000;
    cc = cc | ((Bus & 0xFF) << 16);
    cc = cc | ((Dev & 0x1F) << 11);
    cc = cc | ((Fun & 0x07) << 8);
    cc = cc | ((Reg & 0xFC));

    // Save the current address requested (may be in use by another process)
    ret = GetPortVal(0xCF8, (DWORD*)&t, 4); // 4 Bytes

    // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);

    // Write the value to the PCI config space
    ret = SetPortVal(0xCFC, Val, 4);

    // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);

    // Read the current result from the PCI config space for the Data
    ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);

    // Restore the current address requested (for another process)
    ret = SetPortVal(0xCF8, t, 4);
  }
  else
  {
    DirectIo_WritePCIReg(Bus, Dev, Fun, Reg, Val);
    DirectIo_ReadPCIReg(Bus, Dev, Fun, Reg, &Result);
  }
#endif

  return Result;
}

BOOL Get_PCIEx_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD* Val)
{
  // return DirectIo_ReadPCIReg(Bus, Dev, Fun, Reg, Val) == ERROR_SUCCESS;
  return FALSE; // <km> Reading PCIe registers via the driver seems to crash on some machines
}

BOOL Set_PCIEx_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD Val)
{
  // return DirectIo_WritePCIReg(Bus, Dev, Fun, Reg, Val) == ERROR_SUCCESS;
  return FALSE;
}
#else

DWORD Get_PCI_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
  return PciRead32(PCI_LIB_ADDRESS(Bus, Dev, Fun, Reg));
}

DWORD Set_PCI_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD Val)
{
  return PciWrite32(PCI_LIB_ADDRESS(Bus, Dev, Fun, Reg), Val);
}

BOOL Get_PCIEx_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD* Val)
{
  *Val = PciExpressRead32(PCI_LIB_ADDRESS(Bus, Dev, Fun, Reg));
  return TRUE;
}

BOOL Set_PCIEx_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg, DWORD Val)
{
  PciExpressWrite32(PCI_LIB_ADDRESS(Bus, Dev, Fun, Reg), Val);
  return TRUE;
}

#endif

BOOL NeedBSODWorkaround()
{
#ifdef WIN32
  OSVERSIONINFOEX osInfo;
  SysInfo_GetOsVersion(&osInfo);

  if (osInfo.dwMajorVersion == 10 && osInfo.dwMinorVersion == 0 && osInfo.dwBuildNumber <= 19045)
  {
    switch (SysInfo_cpuinfo.Family)
    {
    case 0x6:
      return SysInfo_cpuinfo.Model == 0xBA || // Intel Raptor Lake (Family: 0x6, Model: 0xBA) on Windows 10.
        SysInfo_cpuinfo.Model == 0xAA || // Intel Meteor Lake (Family: 0x6, Model: 0xAA)
        SysInfo_cpuinfo.Model == 0xB5 || // Intel Arrow Lake (Family: 0x6, Model: 0xB5)
        SysInfo_cpuinfo.Model == 0x8C;	// Intel Tiger Lake (Family: 0x6, Model: 0x8C)

    case 0x19:
      return SysInfo_cpuinfo.Model == 0x74; // AMD Ryzen 7 PRO 7840U (Family: 0x19, Model: 0x74)

    default:
      break;
    }
  }
  else if (osInfo.dwMajorVersion == 10 && osInfo.dwMinorVersion == 0 && osInfo.dwBuildNumber >= 22631)
  {
    switch (SysInfo_cpuinfo.Family)
    {
    case 0x19:
      return SysInfo_cpuinfo.Model == 0x75; // AMD Ryzen 7 PRO 7840U (Family: 0x19, Model: 0x74)

    default:
      break;
    }
  }
#endif
  return FALSE;
}

inline void GetPCIInfoWorkaround()
{
#ifdef WIN32
  switch (SysInfo_cpuinfo.Family)
  {
  case 0x6:
  {
    auto& PCIDev = g_PCIInfo[0x00][0x1f][0x04];
    memset(&PCIDev, 0, sizeof(PCIDev));
    PCIDev.dwVendor_ID = 0x8086;
    PCIDev.byBus = 0x00;
    PCIDev.byDev = 0x1f;
    PCIDev.byFun = 0x04;
    PCIDev.dwDeviceType = PCI_T_SMBUS;
    PCIDev.smbDriver = SMBUS_INTEL_801_SPD5;
    PCIDev.tsodDriver = TSOD_INTEL_801_SPD5;
    _tcscpy(PCIDev.tszVendor_Name, _T("Intel"));

    switch (SysInfo_cpuinfo.Model)
    {
    case 0xAA:
      // Found SMBus device:  VID:8086 DID:7E22 Bus:00 Dev:1F Fun:04 IO Add:EFA0 IO (2) Add:0000 MMIO Add:0000000000000000 PCI Add:{00:00:00:0000} Rev:20 [Intel Meteor Lake SMBus]
      PCIDev.dwDevice_ID = 0x7E22;
      PCIDev.byRev = 0x20;
      PCIDev.dwSMB_Address = 0xEFA0;
      _tcscpy(PCIDev.tszDevice_Name, _T("Meteor Lake SMBus"));
      break;
    case 0xBA:
      // Found SMBus device:  VID:8086 DID:51A3 Bus:00 Dev:1F Fun:04 IO Add:EFA0 IO (2) Add:0000 MMIO Add:0000000000000000 PCI Add:{00:00:00:0000} Rev:01 [Intel Alder Lake-P SMBus]
      PCIDev.dwDevice_ID = 0x51A3;
      PCIDev.byRev = 0x01;
      PCIDev.dwSMB_Address = 0xEFA0;
      _tcscpy(PCIDev.tszDevice_Name, _T("Alder Lake-P SMBus"));
      break;
    case 0x8C:
      // Found SMBus device:  VID:8086 DID:A0A3 Bus:00 Dev:1F Fun:04 IO Add:EFA0 IO (2) Add:0000 MMIO Add:0 PCI Add:{00:00:00:0000} Rev:20 [Intel Tiger Lake-LP SMBus]
      PCIDev.dwDevice_ID = 0xA0A3;
      PCIDev.byRev = 0x20;
      PCIDev.dwSMB_Address = 0xEFA0;
      _tcscpy(PCIDev.tszDevice_Name, _T("Tiger Lake-LP SMBus"));
      break;
    }
  }
  break;

  case 0x19:
  {
    auto& PCIDev = g_PCIInfo[0x00][0x14][0x00];
    memset(&PCIDev, 0, sizeof(PCIDev));
    PCIDev.dwVendor_ID = 0x1022;
    PCIDev.byBus = 0x00;
    PCIDev.byDev = 0x14;
    PCIDev.byFun = 0x00;
    PCIDev.dwDeviceType = PCI_T_SMBUS;
    PCIDev.smbDriver = SMBUS_PIIX4;
    PCIDev.tsodDriver = TSOD_PIIX4;
    _tcscpy(PCIDev.tszVendor_Name, _T("AMD"));

    switch (SysInfo_cpuinfo.Model)
    {
    case 0x74:
      // Found SMBus device:  VID:1022 DID:790B Bus:00 Dev:14 Fun:00 IO Add:0B00 IO (2) Add:0000 MMIO Add:0000000000000000 PCI Add:{00:00:00:0000} Rev:71 [AMD Hudson-3 SMBus]
      PCIDev.dwDevice_ID = 0x790B;
      PCIDev.byRev = 0x71;
      PCIDev.dwSMB_Address = 0x0B00;
      _tcscpy(PCIDev.tszDevice_Name, _T("Hudson-3 SMBus"));
      break;
    }
  }

  default:
    break;
  }
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////
// SMBUS functions
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef WIN32
PCIINFO Get_Info(DWORD Data, DWORD Bus, DWORD Dev, DWORD Fun)
#else
VOID Get_Info(DWORD Data, DWORD Bus, DWORD Dev, DWORD Fun, PCIINFO* PCIDev)
#endif
{
  PCIINFO PCI_Structure;

#if 0
  if (SysInfo_IsLogging())	//SI101017 - only construct string if debugging
    SysInfo_DebugLogWriteLine(_T("Get SMBUS info"));
#endif

  PCI_Structure.dwSMB_Address = 0;
  PCI_Structure.dwSMB_Address2 = 0;
  PCI_Structure.MMCFGBase = 0;
  PCI_Structure.SMB_Address_MMIO = 0;
  PCI_Structure.SMB_Address_PCI.dwBus = 0;
  PCI_Structure.SMB_Address_PCI.dwDev = 0;
  PCI_Structure.SMB_Address_PCI.dwFun = 0;
  PCI_Structure.SMB_Address_PCI.dwReg = 0;
  PCI_Structure.dwVendor_ID = Data & 0xFFFF;
  PCI_Structure.dwDevice_ID = (Data >> 16) & 0xFFFF;
  PCI_Structure.byBus = (BYTE)Bus;
  PCI_Structure.byDev = (BYTE)Dev;
  PCI_Structure.byFun = (BYTE)Fun;
  PCI_Structure.smbDriver = SMBUS_NONE;
  PCI_Structure.tsodDriver = TSOD_NONE;

  switch (Data)
  {
  case 0x71138086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82371AB/EB (PIIX4)"));
    break;
  case 0x719B8086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82443MX Mobile (PIIX4)"));
    break;
  case 0x24138086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801AA/ICH"));
    break;
  case 0x24238086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801AB/ICH0"));
    break;
  case 0x24438086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801BA/ICH2"));
    break;
  case 0x24498086: // IR
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801BA/BAM ICH2/2-M"));
    break;
  case 0x24838086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801CA/CAM ICH3"));
    break;
  case 0x24C38086: // IR
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801DB ICH4"));
    break;
  case 0x24D38086: // IR
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801EB/ER ICH5/5R"));
    break;
  case 0x25A48086: // IR
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("6300ESB"));
    break;
  case 0x269B8086: // IR
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_5000;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Enterprise Southbridge - ESB2"));
    break;
  case 0x266A8086: // IR
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801FB/FR/FBM ICH6/6R/6-M"));
    break;
    /*
  case 0x244E8086:  //ICH6
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    if(PCI_Structure.dwSMB_Address == 0) 		//Temp dodgy hack for ICH6 (xw4200), can't enable to SMBus so use 0xFC00 as SMBus address
      PCI_Structure.dwSMB_Address = 0xFC00;

    PCI_Structure.byRev = (BYTE) Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801FB"));
    break;

  case 0x26408086: //ICH6
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;

    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    if(PCI_Structure.dwSMB_Address == 0) 		//Temp dodgy hack for ICH6 (xw4200), can't enable to SMBus so use 0xFC00 as SMBus address
      PCI_Structure.dwSMB_Address = 0xFC00;

    PCI_Structure.byRev = (BYTE) Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801FB/FR"));
    break;
*/
  case 0x27DA8086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801G ICH7"));
    break;
  case 0x283E8086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("82801H ICH8"));
    break;
  case 0x29308086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("ICH9"));
    break;
  case 0x65F08086:
    if (Fun == 1)
    {
      PCI_Structure.dwDeviceType = PCI_T_SMBUS;
      PCI_Structure.dwSMB_Address = 0;
      PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;

      PCI_Structure.SMB_Address_PCI.dwBus = Bus;
      PCI_Structure.SMB_Address_PCI.dwDev = Dev;
      PCI_Structure.SMB_Address_PCI.dwFun = Fun;
      PCI_Structure.SMB_Address_PCI.dwReg = 0x48;
      PCI_Structure.smbDriver = SMBUS_INTEL_5100;

      _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
      _tcscpy(PCI_Structure.tszDevice_Name, _T("5100"));
    }
    break;
  case 0x50328086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Tolapai"));
    break;
  case 0x3A308086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    PCI_Structure.dwGPIO_Address = Get_PCI_Reg(Bus, 31, 0, 0x48) & 0x0000FF80;
    _stprintf_s(g_szSITempDebug1, _T("GPIO Base Address=%08X"), PCI_Structure.dwGPIO_Address);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    {
      DWORD dwGC = Get_PCI_Reg(Bus, 31, 0, 0x4C);
      _stprintf_s(g_szSITempDebug1, _T("GPIO Control Register=%08X"), dwGC);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if ((dwGC & (1 << 4)) == 0)
      {
        SysInfo_DebugLogWriteLine(_T("GPIO disabled. Enabling..."));
        dwGC |= (1 << 4);
        Set_PCI_Reg(Bus, 31, 0, 0x4C, dwGC);
      }

      if (dwGC & 1)
      {
        SysInfo_DebugLogWriteLine(_T("GPIO registers lockdown enabled. Disabling..."));
        dwGC &= ~1;
        Set_PCI_Reg(Bus, 31, 0, 0x4C, dwGC & ~1);
      }
    }

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("ICH10"));
    break;
  case 0x3A608086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("ICH10"));
    break;
  case 0x3B308086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("3400/5 Series (PCH)"));
    break;
  case 0x1C228086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Cougar Point (PCH)"));
    break;
  case 0x1D228086:
  {
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
#if 0
    DeviceID = Get_PCI_Reg(0, 5, 0, 0);

    // For Sandy Bridge E processors, SPD EEPROMs are connected to the SMBUS on chip
    if (DeviceID == 0x3c288086 || // Intel Sandy Bridge-E/EN/EP IIO - Address Map, VTd Misc, System Management
      DeviceID == 0x0e288086) // Xeon E5 v2/Core i7 VTd/Memory Map/Misc
    {
      // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
      DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x84) & 0xFC000000;
      DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x88) & 0xFC000000;

      _stprintf_s(g_szSITempDebug1, _T("E5 %schipset detected. MMCFGBase=%08X MMCFGLimit=%08X"), DeviceID == 0x0e288086 ? L"v2 " : L"", dwMMCFGBase, dwMMCFGLimit);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if (dwMMCFGBase > 0 &&
        dwMMCFGLimit > 0 &&
        dwMMCFGBase <= dwMMCFGLimit) // Sanity check
      {
        DWORD SMB_Stat = 0;

        // Check if the device containing the SMBus registers exist
        if (((DeviceID = Get_PCI_Reg(0xFF, 0x0F, 0, 0)) == 0x3ca88086 || // Intel Sandy Bridge-E/EN/EP - IMC - Channel0 Target Address Decoder, Rank, Timing
          DeviceID == 0x0ea88086) && // Xeon E5 v2/Core i7 Integrated Memory Controller 0 Target Address/Thermal Registers
          Get_PCIEx_Reg(0xFF, 0x0F, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0xFF;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x0F;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0xFF * 0x100000) + (0x0F * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
        else if (((DeviceID = Get_PCI_Reg(0x7F, 0x0F, 0, 0)) == 0x3ca88086 || // Intel Sandy Bridge-E/EN/EP - IMC - Channel0 Target Address Decoder, Rank, Timing
          DeviceID == 0x0ea88086) && // Xeon E5 v2/Core i7 Integrated Memory Controller 0 Target Address/Thermal Registers
          Get_PCIEx_Reg(0x7F, 0x0F, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0x7F;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x0F;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0x7F * 0x100000) + (0x0F * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
        else if (((DeviceID = Get_PCI_Reg(0x3F, 0x0F, 0, 0)) == 0x3ca88086 ||  // Intel Sandy Bridge-E/EN/EP - IMC - Channel0 Target Address Decoder, Rank, Timing
          DeviceID == 0x0ea88086) && // Xeon E5 v2/Core i7 Integrated Memory Controller 0 Target Address/Thermal Registers
          Get_PCIEx_Reg(0x3F, 0x0F, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0x3F;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x0F;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0x3F * 0x100000) + (0x0F * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
        else if (((DeviceID = Get_PCI_Reg(0x1F, 0x0F, 0, 0)) == 0x3ca88086 || // Intel Sandy Bridge-E/EN/EP - IMC - Channel0 Target Address Decoder, Rank, Timing
          DeviceID == 0x0ea88086) && // Xeon E5 v2/Core i7 Integrated Memory Controller 0 Target Address/Thermal Registers
          Get_PCIEx_Reg(0x1F, 0x0F, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0x1F;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x0F;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0x1F * 0x100000) + (0x0F * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
      }
    }
#endif

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Patsburg (PCH)"));
  }
  break;

  case 0x3ca88086:
  {
    DWORD SMB_Stat = 0;
    {
      _stprintf_s(g_szSITempDebug1, _T("E5 chipset found"));
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      // For Sandy Bridge-E processors, SPD EEPROMs are connected to the SMBUS on chip
      if (Get_PCI_Reg(0, 5, 0, 0) == 0x3c288086) // Sandy Bridge-E Address Map, VTd_Misc, System Management
      {
        // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
        DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x84) & 0xFC000000;
        DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x88) & 0xFC000000;

        _stprintf_s(g_szSITempDebug1, _T("MMCFGBase=%08X MMCFGLimit=%08X"), dwMMCFGBase, dwMMCFGLimit);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (dwMMCFGBase > 0 &&
          dwMMCFGLimit > 0 &&
          dwMMCFGBase <= dwMMCFGLimit) // Sanity check
        {
          PCI_Structure.MMCFGBase = dwMMCFGBase;
          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000); // +0x180;

          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%p"), (PTR)PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
      }

      PCI_Structure.dwDeviceType = PCI_T_SMBUS;
      PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
      PCI_Structure.SMB_Address_PCI.dwBus = Bus;
      PCI_Structure.SMB_Address_PCI.dwDev = Dev;
      PCI_Structure.SMB_Address_PCI.dwFun = Fun;
      PCI_Structure.SMB_Address_PCI.dwReg = 0x180;
      PCI_Structure.smbDriver = SMBUS_INTEL_X79;
    }
    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Sandy Bridge-E (IMC)"));
  }
  break;

  case 0x0ea88086:
  case 0x0e688086:
  {
    DWORD SMB_Stat = 0;
    {
      _stprintf_s(g_szSITempDebug1, _T("E5v2 IMC %d found"), Data == 0x0ea88086 ? 0 : 1);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      // For Ivy Bridge-E processors, SPD EEPROMs are connected to the SMBUS on chip
      if (Get_PCI_Reg(0, 5, 0, 0) == 0x0e288086) // Ivy Bridge-E Address Map, VTd_Misc, System Management
      {
        // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
        DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x84) & 0xFC000000;
        DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x88) & 0xFC000000;

        _stprintf_s(g_szSITempDebug1, _T("MMCFGBase=%08X MMCFGLimit=%08X"), dwMMCFGBase, dwMMCFGLimit);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (dwMMCFGBase > 0 &&
          dwMMCFGLimit > 0 &&
          dwMMCFGBase <= dwMMCFGLimit) // Sanity check
        {
          PCI_Structure.MMCFGBase = dwMMCFGBase;
          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000); // +0x180;

          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%p"), (PTR)PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
      }
      else
      {
        SysInfo_DebugLogWriteLine(_T("E5v2 Device 5, Function 0 not found"));

        if (Get_PCI_Reg(Bus, 22, 1, 0) == 0x0ec98086) // Caching Agent (CBo)
        {
          DWORD dwMMCFGRule = Get_PCI_Reg(Bus, 22, 1, 0xc0);
          if (dwMMCFGRule & 0x01)
          {
            DWORD dwMMCFG = dwMMCFGRule & 0xFC000000;
            _stprintf_s(g_szSITempDebug1, _T("E5v2 MMCFG_Rule=%08x, MMCFG=%08x"), dwMMCFGRule, dwMMCFG);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (dwMMCFG > 0)
            {
              PCI_Structure.MMCFGBase = dwMMCFG;
              PCI_Structure.SMB_Address_MMIO = dwMMCFG + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000); // +0x180;

              _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%p"), (PTR)PCI_Structure.SMB_Address_MMIO);
              SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
              SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
              _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
              SysInfo_DebugLogWriteLine(g_szSITempDebug1);
            }
          }
        }
      }

      PCI_Structure.dwDeviceType = PCI_T_SMBUS;
      PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
      PCI_Structure.SMB_Address_PCI.dwBus = Bus;
      PCI_Structure.SMB_Address_PCI.dwDev = Dev;
      PCI_Structure.SMB_Address_PCI.dwFun = Fun;
      PCI_Structure.SMB_Address_PCI.dwReg = 0x180;
      PCI_Structure.smbDriver = SMBUS_INTEL_X79;
    }
    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _stprintf_s(PCI_Structure.tszDevice_Name, _T("Ivy Bridge-E (IMC %d)"), Data == 0x0ea88086 ? 0 : 1);
  }
  break;

  case 0x1D708086:
  case 0x1D718086:
  case 0x1D728086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
#if 0
    // For Sandy Bridge E processors, SPD EEPROMs are connected to the SMBUS on chip
    if (Get_PCI_Reg(0, 5, 0, 0) == 0x3c288086) // Intel Sandy Bridge-E/EN/EP IIO - Address Map, VTd Misc, System Management
    {
      // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
      DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x84);
      DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x88);

      _stprintf_s(g_szSITempDebug1, L"Sandy Bridge-E chipset detected. MMCFGBase=%08X MMCFGLimit=%08X", dwMMCFGBase, dwMMCFGLimit);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if (dwMMCFGBase < dwMMCFGLimit) // Sanity check
      {
        // Check if the device containing the SMBus registers exist
        if (Get_PCI_Reg(0xFF, 0x0F, 0, 0) == 0x3ca88086) // Intel Sandy Bridge-E/EN/EP - IMC - Channel0 Target Address Decoder, Rank, Timing
        {
          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0xFF * 0x100000) + (0x0F * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, L"Sandy Bridge-E SMBUS MMIO address=%08X", PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
      }
    }
#endif
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Patsburg (PCH) IDF"));
    break;

  case 0x8c228086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("8 Series/C220 Series (Lynx Point) (PCH)"));
    break;

  case 0x8ca28086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("9 Series (PCH)"));
    break;

  case 0x8d228086:
  {
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

#if 0
    DeviceID = Get_PCI_Reg(0, 5, 0, 0);

    // For Sandy Bridge E processors, SPD EEPROMs are connected to the SMBUS on chip
    if (DeviceID == 0x2f288086) // Haswell-E Address Map, VTd_Misc, System Management
    {
      // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
      DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x84) & 0xFC000000;
      DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x88) & 0xFC000000;

      _stprintf_s(g_szSITempDebug1, _T("E5v3 chipset detected. MMCFGBase=%08X MMCFGLimit=%08X"), dwMMCFGBase, dwMMCFGLimit);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if (dwMMCFGBase > 0 &&
        dwMMCFGLimit > 0 &&
        dwMMCFGBase <= dwMMCFGLimit) // Sanity check
      {
        DWORD SMB_Stat = 0;

        // Check if the device containing the SMBus registers exist
        if ((DeviceID = Get_PCI_Reg(0xFF, 0x13, 0, 0)) == 0x2fa88086 && // Haswell-E Integrated Memory Controller 0 Target Address, Thermal & RAS Registers
          Get_PCIEx_Reg(0xFF, 0x13, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0xFF;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x13;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0xFF * 0x100000) + (0x13 * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
        else if ((DeviceID = Get_PCI_Reg(0x7F, 0x13, 0, 0)) == 0x2fa88086 && // Haswell-E Integrated Memory Controller 0 Target Address, Thermal & RAS Registers
          Get_PCIEx_Reg(0x7F, 0x13, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0x7F;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x13;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0x7F * 0x100000) + (0x13 * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
        else if ((DeviceID = Get_PCI_Reg(0x3F, 0x13, 0, 0)) == 0x2fa88086 && // Haswell-E Integrated Memory Controller 0 Target Address, Thermal & RAS Registers
          Get_PCIEx_Reg(0x3F, 0x13, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0x3F;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x13;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0x3F * 0x100000) + (0x13 * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
        else if ((DeviceID = Get_PCI_Reg(0x1F, 0x13, 0, 0)) == 0x2fa88086 && // Haswell-E Integrated Memory Controller 0 Target Address, Thermal & RAS Registers
          Get_PCIEx_Reg(0x1F, 0x13, 0, 0x180, &SMB_Stat) != FALSE)
        {
          PCI_Structure.SMB_Address_PCI.dwBus = 0x1F;
          PCI_Structure.SMB_Address_PCI.dwDev = 0x13;
          PCI_Structure.SMB_Address_PCI.dwFun = 0;
          PCI_Structure.SMB_Address_PCI.dwReg = 0x180;

          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (0x1F * 0x100000) + (0x13 * 0x8000) + 0x180;
          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%08X"), PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
      }
    }
#endif
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Wellsburg (PCH)"));
  }
  break;

  case 0x2fa88086:
  case 0x2f688086:
  {
    DWORD SMB_Stat = 0;
    {
      _stprintf_s(g_szSITempDebug1, _T("E5v3 IMC %d found"), Data == 0x2fa88086 ? 0 : 1);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      // For Haswell-E processors, SPD EEPROMs are connected to the SMBUS on chip
      if (Get_PCI_Reg(0, 5, 0, 0) == 0x2f288086) // Haswell-E Address Map, VTd_Misc, System Management
      {
        // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
        DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x90) & 0xFC000000;
        DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x98) & 0xFC000000;

        _stprintf_s(g_szSITempDebug1, _T("MMCFGBase=%08X MMCFGLimit=%08X"), dwMMCFGBase, dwMMCFGLimit);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (dwMMCFGBase > 0 &&
          dwMMCFGLimit > 0 &&
          dwMMCFGBase <= dwMMCFGLimit) // Sanity check
        {
          PCI_Structure.MMCFGBase = dwMMCFGBase;
          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000); // +0x180;

          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%p"), (PTR)PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
      }
      else
      {
        SysInfo_DebugLogWriteLine(_T("E5v3 Device 5, Function 0 not found"));

        if (Get_PCI_Reg(Bus, 15, 5, 0) == 0x2ffd8086) // Caching Agent (CBo)
        {
          DWORD dwMMCFGRule = Get_PCI_Reg(Bus, 15, 5, 0xc0);
          if (dwMMCFGRule & 0x01)
          {
            DWORD dwMMCFG = dwMMCFGRule & 0xFC000000;
            _stprintf_s(g_szSITempDebug1, _T("E5v3 MMCFG_Rule=%08x, MMCFG=%08x"), dwMMCFGRule, dwMMCFG);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (dwMMCFG > 0)
            {
              PCI_Structure.MMCFGBase = dwMMCFG;
              PCI_Structure.SMB_Address_MMIO = dwMMCFG + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000); // +0x180;

              _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%p"), (PTR)PCI_Structure.SMB_Address_MMIO);
              SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
              SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
              _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
              SysInfo_DebugLogWriteLine(g_szSITempDebug1);
            }
          }
        }
      }

      PCI_Structure.dwDeviceType = PCI_T_SMBUS;
      PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
      PCI_Structure.SMB_Address_PCI.dwBus = Bus;
      PCI_Structure.SMB_Address_PCI.dwDev = Dev;
      PCI_Structure.SMB_Address_PCI.dwFun = Fun;
      PCI_Structure.SMB_Address_PCI.dwReg = 0x180;
      PCI_Structure.smbDriver = SMBUS_INTEL_X79;
    }
    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _stprintf_s(PCI_Structure.tszDevice_Name, _T("Haswell-E (IMC %d)"), Data == 0x2fa88086 ? 0 : 1);
  }
  break;

  case 0x20858086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.SMB_Address_PCI.dwBus = Bus;
    PCI_Structure.SMB_Address_PCI.dwDev = Dev;
    PCI_Structure.SMB_Address_PCI.dwFun = Fun;
    PCI_Structure.smbDriver = SMBUS_INTEL_PCU;
    PCI_Structure.tsodDriver = TSOD_INTEL_PCU;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Skylake-X"));
    break;

  case 0x34488086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.SMB_Address_PCI.dwBus = Bus;
    PCI_Structure.SMB_Address_PCI.dwDev = Dev;
    PCI_Structure.SMB_Address_PCI.dwFun = Fun;
    PCI_Structure.smbDriver = SMBUS_INTEL_PCU_IL;
    PCI_Structure.tsodDriver = TSOD_INTEL_PCU_IL;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Ice Lake-SP"));
    break;

  case 0x9c228086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Lynx Point-LP (PCH)"));
    break;

  case 0x9ca28086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Wildcat Point-LP (PCH)"));
    break;

  case 0x23308086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("DH89xxCC (PCH)"));
    break;

  case 0x1e228086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Panther Point (PCH)"));
    break;

  case 0x1f3c8086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.SMB_Address_MMIO = Get_PCI_Reg(Bus, Dev, Fun, 0x10) & 0xFFFFFFE0;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Avoton"));
    break;

  case 0x0f128086:
  case 0x0f138086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("ValleyView"));
    break;

  case 0x9d238086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Sunrise Point-LP"));
    break;

  case 0xA1238086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;
    PCI_Structure.tsodDriver = TSOD_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Sunrise Point-H"));
    break;

  case 0x5AD48086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;
    PCI_Structure.tsodDriver = TSOD_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Broxton"));
    break;

  case 0x6fa88086:
  case 0x6f688086:
  {
    DWORD SMB_Stat = 0;
    {
      _stprintf_s(g_szSITempDebug1, _T("Broadwell IMC found"));
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      // For Broadwell-DE processors, SPD EEPROMs are connected to the SMBUS on chip
      if (Get_PCI_Reg(0, 5, 0, 0) == 0x6f288086) // Broadwell Address Map, VTd_Misc, System Management
      {
        // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
        DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x90) & 0xFC000000;
        DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x98) & 0xFC000000;

        _stprintf_s(g_szSITempDebug1, _T("MMCFGBase=%08X MMCFGLimit=%08X"), dwMMCFGBase, dwMMCFGLimit);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (dwMMCFGBase > 0 &&
          dwMMCFGLimit > 0 &&
          dwMMCFGBase <= dwMMCFGLimit) // Sanity check
        {
          PCI_Structure.MMCFGBase = dwMMCFGBase;
          PCI_Structure.SMB_Address_MMIO = dwMMCFGBase + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000); // +0x180;

          _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%p"), (PTR)PCI_Structure.SMB_Address_MMIO);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
          SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
          _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }
      }
      else
      {
        SysInfo_DebugLogWriteLine(_T("Broadwell Device 5, Function 0 not found"));

        if (Get_PCI_Reg(Bus, 15, 5, 0) == 0x6ffd8086) // Caching Agent (CBo)
        {
          DWORD dwMMCFGRule = Get_PCI_Reg(Bus, 15, 5, 0xc0);
          if (dwMMCFGRule & 0x01)
          {
            DWORD dwMMCFG = dwMMCFGRule & 0xFC000000;
            _stprintf_s(g_szSITempDebug1, _T("Broadwell MMCFG_Rule=%08x, MMCFG=%08x"), dwMMCFGRule, dwMMCFG);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            if (dwMMCFG > 0)
            {
              PCI_Structure.MMCFGBase = dwMMCFG;
              PCI_Structure.SMB_Address_MMIO = dwMMCFG + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000); // +0x180;

              _stprintf_s(g_szSITempDebug1, _T("SMBUS MMIO address=%p"), (PTR)PCI_Structure.SMB_Address_MMIO);
              SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifndef WIN32
              SMB_Stat = MmioRead32(PCI_Structure.SMB_Address_MMIO);
#endif
              _stprintf_s(g_szSITempDebug1, _T("SMB_STAT[0]=%08X"), SMB_Stat);
              SysInfo_DebugLogWriteLine(g_szSITempDebug1);
            }
          }
        }
      }

      PCI_Structure.dwDeviceType = PCI_T_SMBUS;
      PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
      PCI_Structure.SMB_Address_PCI.dwBus = Bus;
      PCI_Structure.SMB_Address_PCI.dwDev = Dev;
      PCI_Structure.SMB_Address_PCI.dwFun = Fun;
      PCI_Structure.SMB_Address_PCI.dwReg = 0x180;
      PCI_Structure.smbDriver = SMBUS_INTEL_X79;
    }
    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Broadwell"));
  }
  break;

  case 0x22928086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Braswell"));
    break;

  case 0xA2238086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Lewisburg PCH"));
    break;

  case 0xA2A38086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("200 Series PCH SMBus Controller"));
    break;

  case 0xa3238086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Cannon Lake PCH SMBus Controller"));
    break;

  case 0xa1a38086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("C620 Series Chipset Family SMBus"));
    break;

  case 0x06A38086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Comet Lake PCH SMBus"));
    break;

  case 0x34A38086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Ice Lake-LP SMBus"));
    break;

  case 0x7AA38086:
  case 0x51A38086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801_SPD5;
    PCI_Structure.tsodDriver = TSOD_INTEL_801_SPD5;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    if (Data == 0x7AA38086)
      _tcscpy(PCI_Structure.tszDevice_Name, _T("Alder Lake-S SMBus"));
    else if (Data == 0x51A38086)
      _tcscpy(PCI_Structure.tszDevice_Name, _T("Alder Lake-P SMBus"));
    break;

  case 0x7A238086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801_SPD5;
    PCI_Structure.tsodDriver = TSOD_INTEL_801_SPD5;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Raptor Lake-S SMBus"));
    break;

  case 0x7E228086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801_SPD5;
    PCI_Structure.tsodDriver = TSOD_INTEL_801_SPD5;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Meteor Lake SMBus"));
    break;

  case 0xA0A38086:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_INTEL_801_SPD5;
    PCI_Structure.tsodDriver = TSOD_INTEL_801_SPD5;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Tiger Lake-LP SMBus"));
    break;

  case 0x59008086:
  case 0x59048086:
  case 0x590C8086:
  case 0x590F8086:
  case 0x59108086:
  case 0x59188086:
  case 0x591F8086:
  {
    DWORD dwMCHBAR_LO;
    DWORD dwMCHBAR_HI;
    // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
    dwMCHBAR_LO = Get_PCI_Reg(0, 0, 0, INTEL_KABYLAKE_MCHBAR);

    _stprintf_s(g_szSITempDebug1, _T("MCHBAR_LO=%08x"), dwMCHBAR_LO);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // Enable MCHBAR
    if (!(dwMCHBAR_LO & 0x1))
    {
      Set_PCI_Reg(0, 0, 0, INTEL_KABYLAKE_MCHBAR, dwMCHBAR_LO | 1);
    }

    dwMCHBAR_HI = Get_PCI_Reg(0, 0, 0, INTEL_KABYLAKE_MCHBAR_HI);

    _stprintf_s(g_szSITempDebug1, _T("MCHBAR_HI=%08x"), dwMCHBAR_HI);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    PCI_Structure.MMCFGBase = (((UINT64)dwMCHBAR_HI << 32) | dwMCHBAR_LO) & 0x7FFFFF8000ULL;

    _stprintf_s(g_szSITempDebug1, _T("MCHBAR=%p"), (PTR)PCI_Structure.MMCFGBase);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    PCI_Structure.dwDeviceType = PCI_T_HOST_BRIDGE_DRAM;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.SMB_Address_PCI.dwBus = Bus;
    PCI_Structure.SMB_Address_PCI.dwDev = Dev;
    PCI_Structure.SMB_Address_PCI.dwFun = Fun;
    PCI_Structure.tsodDriver = TSOD_INTEL_E3;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Xeon E3 v6 Host Bridge/DRAM Registers"));
  }
  break;

  case 0x2F988086:
  case 0x2F9C8086:
  {
    DWORD MEM_TRML_TEMPERATURE_REPORT = Get_PCI_Reg(Bus, Dev, Fun, 0x60);

    _stprintf_s(g_szSITempDebug1, _T("E5v3 (Haswell-E) Power Management and Control found"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    _stprintf_s(g_szSITempDebug1, _T("MEM_TRML_TEMPERATURE_REPORT=%08X"), MEM_TRML_TEMPERATURE_REPORT);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("E5v3 (Haswell-E) Power Management and Control found"));
  }
  break;

#if 0
  case 0x2FB08086:
  case 0x2FB18086:
  case 0x2FB48086:
  case 0x2FB58086:
  case 0x2FD08086:
  case 0x2FD18086:
  case 0x2FD48086:
  case 0x2FD58086:
  {
    _stprintf_s(g_szSITempDebug1, _T("E5v3 (Haswell-E) IMC found"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // For Haswell-E processors, SPD EEPROMs are connected to the SMBUS on chip
    if (Get_PCI_Reg(0, 5, 0, 0) == 0x2f288086) // Haswell-E Address Map, VTd_Misc, System Management
    {
      // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
      DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x90) & 0xFC000000;
      DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x98) & 0xFC000000;

      _stprintf_s(g_szSITempDebug1, _T("MMCFGBase=%08X MMCFGLimit=%08X"), dwMMCFGBase, dwMMCFGLimit);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if (dwMMCFGBase > 0 &&
        dwMMCFGLimit > 0 &&
        dwMMCFGBase <= dwMMCFGLimit) // Sanity check
      {
        PCI_Structure.MMCFGBase = dwMMCFGBase;
      }
    }
    else
    {
      SysInfo_DebugLogWriteLine(_T("E5v3 (Haswell-E) Device 5, Function 0 not found"));

      if (Get_PCI_Reg(Bus, 15, 5, 0) == 0x2ffd8086) // Caching Agent (CBo)
      {
        DWORD dwMMCFGRule = Get_PCI_Reg(Bus, 15, 5, 0xc0);
        if (dwMMCFGRule & 0x01)
        {
          DWORD dwMMCFG = dwMMCFGRule & 0xFC000000;
          _stprintf_s(g_szSITempDebug1, _T("E5v3 MMCFG_Rule=%08x, MMCFG=%08x"), dwMMCFGRule, dwMMCFG);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

          if (dwMMCFG > 0)
          {
            PCI_Structure.MMCFGBase = dwMMCFG;
          }
        }
      }
    }

    PCI_Structure.dwDeviceType = PCI_T_HOST_BRIDGE_DRAM;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.SMB_Address_PCI.dwBus = Bus;
    PCI_Structure.SMB_Address_PCI.dwDev = Dev;
    PCI_Structure.SMB_Address_PCI.dwFun = Fun;
    PCI_Structure.tsodDriver = TSOD_INTEL_E5;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("E5v3 (Haswell-E) Channel 0 Thermal Control"));
  }
  break;

  case 0x6FB08086:
  case 0x6FB18086:
  case 0x6FB48086:
  case 0x6FB58086:
  case 0x6FD08086:
  case 0x6FD18086:
  case 0x6FD48086:
  case 0x6FD58086:
  {
    _stprintf_s(g_szSITempDebug1, _T("Broadwell IMC found"));
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    // For Broadwell-DE processors, SPD EEPROMs are connected to the SMBUS on chip
    if (Get_PCI_Reg(0, 5, 0, 0) == 0x6f288086) // Broadwell Address Map, VTd_Misc, System Management
    {
      // Get MMCFG register. This is the PCIe configuration space base address for memory mapped access
      DWORD dwMMCFGBase = Get_PCI_Reg(0, 5, 0, 0x90) & 0xFC000000;
      DWORD dwMMCFGLimit = Get_PCI_Reg(0, 5, 0, 0x98) & 0xFC000000;

      _stprintf_s(g_szSITempDebug1, _T("MMCFGBase=%08X MMCFGLimit=%08X"), dwMMCFGBase, dwMMCFGLimit);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if (dwMMCFGBase > 0 &&
        dwMMCFGLimit > 0 &&
        dwMMCFGBase <= dwMMCFGLimit) // Sanity check
      {
        PCI_Structure.MMCFGBase = dwMMCFGBase;
      }
    }
    else
    {
      SysInfo_DebugLogWriteLine(_T("Broadwell Device 5, Function 0 not found"));

      if (Get_PCI_Reg(Bus, 15, 5, 0) == 0x6ffd8086) // Caching Agent (CBo)
      {
        DWORD dwMMCFGRule = Get_PCI_Reg(Bus, 15, 5, 0xc0);
        if (dwMMCFGRule & 0x01)
        {
          DWORD dwMMCFG = dwMMCFGRule & 0xFC000000;
          _stprintf_s(g_szSITempDebug1, _T("Broadwell MMCFG_Rule=%08x, MMCFG=%08x"), dwMMCFGRule, dwMMCFG);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

          if (dwMMCFG > 0)
          {
            PCI_Structure.MMCFGBase = dwMMCFG;
          }
        }
      }
    }

    PCI_Structure.dwDeviceType = PCI_T_HOST_BRIDGE_DRAM;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.SMB_Address_PCI.dwBus = Bus;
    PCI_Structure.SMB_Address_PCI.dwDev = Dev;
    PCI_Structure.SMB_Address_PCI.dwFun = Fun;
    PCI_Structure.tsodDriver = TSOD_INTEL_E5;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Broadwell-DE Channel 0 Thermal Control"));
  }
  break;
#endif
  case 0x006410DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce2 SMBus (MCP)"));
    break;
  case 0x008410DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce2 Ultra 400 SMBus (MCP)"));
    break;
  case 0x00D410DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce3 Pro150 SMBus (MCP)"));
    break;
  case 0x00E410DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce3 250Gb SMBus (MCP)"));
    break;
  case 0x005210DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce4 SMBus (MCP)"));
    break;
  case 0x003410DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce4 SMBus (MCP-04)"));
    break;
  case 0x026410DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP51)"));
    break;
  case 0x036810DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP55)"));
    break;
  case 0x03EB10DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP61)"));
    break;
  case 0x044610DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP65)"));
    break;
  case 0x054210DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP67)"));
    break;
  case 0x07d810DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP73)"));
    break;
  case 0x075210DE:
  {
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP78S)"));
  }
  break;
  case 0x0aa210DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
#if 0
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; //0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
#endif
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.dwSMB_Address2 = Get_PCI_Reg(Bus, Dev, Fun, 0x24) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP79)"));
    break;
  case 0x0d7910DE:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets

    if (PCI_Structure.dwSMB_Address == 0) // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;

    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_NVIDIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("nForce SMBus (MCP89)"));
    break;
  case 0x43531002:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("ATI"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("SMBus"));
    break;
  case 0x43631002:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("ATI"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("SMBus"));
    break;
  case 0x43721002:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("ATI"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("IXP SB400 SMBus Controller"));
    break;
  case 0x43851002:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    if (PCI_Structure.byRev >= 0x40) // SB800
    {
      DWORD hi, lo;
      SetPortVal(0xCD6, 0x2C, 1);
      GetPortVal(0xCD7, &lo, 1);
      SetPortVal(0xCD6, 0x2D, 1);
      GetPortVal(0xCD7, &hi, 1);

      PCI_Structure.dwSMB_Address = ((hi << 8) | lo) & 0xFFE0;
      _tcscpy(PCI_Structure.tszVendor_Name, _T("ATI"));
      _tcscpy(PCI_Structure.tszDevice_Name, _T("SB800 SMBus"));
    }
    else
    {
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
      _tcscpy(PCI_Structure.tszVendor_Name, _T("ATI"));
      _tcscpy(PCI_Structure.tszDevice_Name, _T("SB600/SB700 SMBus"));
    }
    break;
  case 0x740B1022:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x58) & 0xFFF0;
    PCI_Structure.dwSMB_Address += 0xE0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_AMD756;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("AMD"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("AMD-756 Athlon ACPI"));
    break;
  case 0x74131022:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x58) & 0xFFF0;
    PCI_Structure.dwSMB_Address += 0xE0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_AMD756;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("AMD"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("AMD-766 Athlon ACPI"));
    break;
  case 0x74431022:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x58) & 0xFFF0;
    PCI_Structure.dwSMB_Address += 0xE0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_AMD756;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("AMD"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("AMD-768 System Management"));
    break;
  case 0x746b1022:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x58) & 0xFFF0;
    PCI_Structure.dwSMB_Address += 0xE0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_AMD756;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("AMD"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("AMD-8111 ACPI"));
    break;

  case 0x780B1022:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    {
      DWORD hi, lo;
      SetPortVal(0xCD6, PCI_Structure.byRev >= 0x41 ? 0x00 : 0x2C, 1);
      GetPortVal(0xCD7, &lo, 1);
      SetPortVal(0xCD6, PCI_Structure.byRev >= 0x41 ? 0x01 : 0x2D, 1);
      GetPortVal(0xCD7, &hi, 1);

      if (PCI_Structure.byRev >= 0x41)
        PCI_Structure.dwSMB_Address = (hi << 8);
      else
        PCI_Structure.dwSMB_Address = ((hi << 8) | lo) & 0xFFE0;
    }

    _tcscpy(PCI_Structure.tszVendor_Name, _T("AMD"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Hudson-2 SMBus"));
    break;

  case 0x790B1022:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;
    PCI_Structure.tsodDriver = TSOD_PIIX4;
    {
      DWORD hi, lo;
      SetPortVal(0xCD6, PCI_Structure.byRev >= 0x49 ? 0x00 : 0x2C, 1);
      GetPortVal(0xCD7, &lo, 1);
      SetPortVal(0xCD6, PCI_Structure.byRev >= 0x49 ? 0x01 : 0x2D, 1);
      GetPortVal(0xCD7, &hi, 1);

      if (PCI_Structure.byRev >= 0x49)
      {
        DWORD decodeen2;
        BYTE smbus0sel;
        if (lo != 0xff && hi != 0xff)
        {
          PCI_Structure.dwSMB_Address = (hi << 8);

          SetPortVal(0xCD6, 0x02, 1);
          GetPortVal(0xCD7, &decodeen2, 1);

          smbus0sel = (decodeen2 >> 3) & 0x3;
          _stprintf_s(g_szSITempDebug1, _T("PM DECODEEN[0]=%08X"), lo & 0xff);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
          _stprintf_s(g_szSITempDebug1, _T("PM DECODEEN[1]=%08X"), hi & 0xff);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
          _stprintf_s(g_szSITempDebug1, _T("PM DECODEEN[2]=%08X (smbus0sel=%d)"), decodeen2 & 0xff, smbus0sel);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

          if (smbus0sel != 0)
          {
            decodeen2 &= ~(0x3 << 3);

            _stprintf_s(g_szSITempDebug1, _T("Setting PM DECODEEN[2]=%08X"), (decodeen2 & 0xff));
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);

            SetPortVal(0xCD6, 0x02, 1);
            SetPortVal(0xCD7, decodeen2, 1);
          }
        }
        DWORD DecodeEn;

#ifdef WIN32
        // Ask driver to map MMIO base address to pointer we can use
        BYTE* pbLinAddr = NULL;
        tagPhys32Struct Phys32Struct = { 0 };
        // HANDLE		hPhysicalMemory	= NULL;
        pbLinAddr = MapPhysToLin((PBYTE)(UINT_PTR)AMD_ACPI_FCH_PM_DECODEEN, 4, &Phys32Struct);
        if (pbLinAddr)
        {
          DecodeEn = *((DWORD*)pbLinAddr);
#else
        DecodeEn = MmioRead32(AMD_ACPI_FCH_PM_DECODEEN);
#endif
        _stprintf_s(g_szSITempDebug1, _T("MMIO PM DECODEEN: %08X (smbus0sel=%d)"), DecodeEn, BitExtract(DecodeEn, 20, 19));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (PCI_Structure.dwSMB_Address == 0)
          PCI_Structure.dwSMB_Address = (DecodeEn & 0xFF00);
#ifdef WIN32
        UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
        }
#endif
      }
    else
      PCI_Structure.dwSMB_Address = ((hi << 8) | lo) & 0xFFE0;
    }

  _tcscpy(PCI_Structure.tszVendor_Name, _T("AMD"));
  _tcscpy(PCI_Structure.tszDevice_Name, _T("Hudson-3 SMBus"));
  break;

	case 0x790B1D94:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_PIIX4;

    {
      DWORD hi, lo;
      SetPortVal(0xCD6, PCI_Structure.byRev >= 0x49 ? 0x00 : 0x2C, 1);
      GetPortVal(0xCD7, &lo, 1);
      SetPortVal(0xCD6, PCI_Structure.byRev >= 0x49 ? 0x01 : 0x2D, 1);
      GetPortVal(0xCD7, &hi, 1);

      if (PCI_Structure.byRev >= 0x49)
        PCI_Structure.dwSMB_Address = (hi << 8);
      else
        PCI_Structure.dwSMB_Address = ((hi << 8) | lo) & 0xFFE0;
    }

    _tcscpy(PCI_Structure.tszVendor_Name, _T("Hygon"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Hudson-3 SMBus"));
    break;

  case 0x30501106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT82C596 Apollo ACPI"));
    break;
  case 0x30511106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT82C596B ACPI"));
    break;
  case 0x30571106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT82C686 Apollo ACPI"));
    break;
  case 0x30741106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8233 VLink South Bridge"));
    break;
  case 0x31471106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8233A South Bridge"));
    break;
  case 0x31771106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8233A/8235 South Bridge"));
    break;
  case 0x32271106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8237 South Bridge"));
    break;
  case 0x33371106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8237A South Bridge"));
    break;
  case 0x33721106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8237S South Bridge"));
    break;
  case 0x82351106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8231 South Bridge"));
    break;
  case 0x32871106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VT8251 South Bridge"));
    break;
  case 0x83241106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("CX700 South Bridge"));
    break;
  case 0x83531106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VX800/VX820 South Bridge"));
    break;
  case 0x84091106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VX855/VX875 South Bridge"));
    break;
  case 0x84101106:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_VIA;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("VX900 South Bridge"));
    break;

  case 0x00161039:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    // <km> Some BIOS's disable the SMBus - see drivers/pci/quirks.c (quirk_sis_96x_smbus function)
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_SIS96X;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("SIS"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("SIS96X SMBus Controller"));
    break;

  case 0x09681039:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    // No datasheet. Looking at the PCI dump seems like the I/O address is stored at 0x9A. Also see:
    // https://code.google.com/p/aspia/source/search?q=spd.c&origq=spd.c&btnG=Search+Trunk
    PCI_Structure.dwSMB_Address = (Get_PCI_Reg(Bus, Dev, Fun, 0x98) >> 16) & 0x0000FF00;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_SIS968;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("SIS"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("SIS968 SMBus Controller"));
    break;

  case 0x09641039:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    {
      WORD ACPIBase = Get_PCI_Reg(Bus, Dev, Fun, 0x74) & 0x0000FFFF;

      PCI_Structure.dwSMB_Address = ACPIBase + 0xE0;

      _stprintf_s(g_szSITempDebug1, _T("SiS964 detected. ACPIBase=%04X SMBBase=%04X"), ACPIBase, PCI_Structure.dwSMB_Address);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);
    }
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_SIS630;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("SIS"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("SIS964 SMBus Controller"));
    break;

  case 0x156310B9:
    PCI_Structure.dwDeviceType = PCI_T_SMBUS;
    PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x80) & 0xFFF0;
    // <km> May need to enable SMB host controller/IO space - see drivers/i2c/busses/i2c-ali1563.c (ali1563_setup function)
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.smbDriver = SMBUS_ALI1563;

    _tcscpy(PCI_Structure.tszVendor_Name, _T("ALI"));
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Acer Labs 1563"));
    break;
  default:
  {
    // Try to autodetect
    DWORD Class_Code = (Get_PCI_Reg(Bus, Dev, Fun, 0x08) >> 8);
    if (Class_Code == 0x0C0500)
    {
      PCI_Structure.dwDeviceType = PCI_T_SMBUS;
      if (SysInfo_IsLogging()) // SI101017 - only construct string if debug logging
      {
        _stprintf_s(g_szSITempDebug1, _T("Found device with SMBUS class code."));
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
      }
    }
    else
      PCI_Structure.dwDeviceType = PCI_T_UNKNOWN;

    if (PCI_Structure.dwVendor_ID == 0x8086)
    {
      _tcscpy(PCI_Structure.tszVendor_Name, _T("Intel"));
    }
    else if (PCI_Structure.dwVendor_ID == 0x10DE)
    {
      _tcscpy(PCI_Structure.tszVendor_Name, _T("nVidia"));
    }
    else if (PCI_Structure.dwVendor_ID == 0x1002)
    {
      _tcscpy(PCI_Structure.tszVendor_Name, _T("ATI"));
    }
    else if (PCI_Structure.dwVendor_ID == 0x1106)
    {
      _tcscpy(PCI_Structure.tszVendor_Name, _T("VIA"));
    }
    else if (PCI_Structure.dwVendor_ID == 0x1022)
    {
      _tcscpy(PCI_Structure.tszVendor_Name, _T("AMD"));
    }
    else if (PCI_Structure.dwVendor_ID == 0x1D94)
    {
      _tcscpy(PCI_Structure.tszVendor_Name, _T("Hygon"));
    }
    else if (PCI_Structure.dwVendor_ID == 0x1039)
    {
      _tcscpy(PCI_Structure.tszVendor_Name, _T("SIS"));
    }
    else
    {
#ifdef WIN32
      _tcscpy(PCI_Structure.tszVendor_Name, SYSIN_GETLPSTR(SYSIN::SI_IDS_UNKNOWN));
#else
      _tcscpy(PCI_Structure.tszVendor_Name, _T("Unknown"));
#endif
    }

    if (Class_Code == 0x0C0500 && PCI_Structure.dwVendor_ID == 0x8086) // Intel
    {
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
      PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
      PCI_Structure.smbDriver = SMBUS_INTEL_801_SPD5;
      PCI_Structure.tsodDriver = TSOD_INTEL_801_SPD5;
    }
    else if (Class_Code == 0x0C0500 && PCI_Structure.dwVendor_ID == 0x10DE) // NVIDIA
    {
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x50) & 0xFFF0; // 0x50 and 0x54, there are 2 smbusses with nforce chipsets
      if (PCI_Structure.dwSMB_Address == 0)									 // For newer devices, standard BARs 4 and 5 are used (0x20/0x24)
        PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    }
    else if (Class_Code == 0x0C0500 && PCI_Structure.dwVendor_ID == 0x1002) // ATI (PIIX4)
    {
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    }
    else if (Class_Code == 0x0C0500 && PCI_Structure.dwVendor_ID == 0x1106) // VIA
    {
      PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0xD0 /*0x90 for some devices*/) & 0xFFF0;

      if (PCI_Structure.dwSMB_Address == 0)
        PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x90) & 0xFFF0;
    }

    // LSI card causing crash when at bus location 128 when queried for it's revision, (VID:1000 DID:0056)
    //   PCI_Structure.dwSMB_Address = Get_PCI_Reg(Bus, Dev, Fun, 0x20) & 0xFFF0;
    PCI_Structure.byRev = (BYTE)Get_PCI_Reg(Bus, Dev, Fun, 8) & 0xFF;
    PCI_Structure.byHeaderType = (BYTE)BitExtract(Get_PCI_Reg(Bus, Dev, Fun, 0xC), 23, 16);
#ifdef WIN32
    _tcscpy(PCI_Structure.tszDevice_Name, SYSIN_GETLPSTR(SYSIN::SI_IDS_UNKNOWN));
#else
    _tcscpy(PCI_Structure.tszDevice_Name, _T("Unknown"));
#endif
  }
  break;
  }

if (SysInfo_IsLogging())
{
  if (PCI_Structure.smbDriver != SMBUS_NONE)
  {
    DWORD Reg;

    _stprintf_s(g_szSITempDebug1, _T("{%02X:%02X:%02X} SMBus Configuration Registers:"), Bus, Dev, Fun);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    for (Reg = 0; Reg <= 0xFF; Reg += 16)
    {
      DWORD dwVal0, dwVal1, dwVal2, dwVal3;

      dwVal0 = Get_PCI_Reg(Bus, Dev, Fun, Reg);
      dwVal1 = Get_PCI_Reg(Bus, Dev, Fun, Reg + 4);
      dwVal2 = Get_PCI_Reg(Bus, Dev, Fun, Reg + 8);
      dwVal3 = Get_PCI_Reg(Bus, Dev, Fun, Reg + 12);

      _stprintf_s(g_szSITempDebug1, _T("%02X: %08X %08X %08X %08X"), Reg, dwVal0, dwVal1, dwVal2, dwVal3);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);
    }
  }
}

#if 0
if (SysInfo_IsLogging())	//SI101017 - only construct string if debugging
SysInfo_DebugLogWriteLine(_T("Finished getting SMBUS info"));
#endif

#ifdef WIN32
return (PCI_Structure);
#else
CopyMem(PCIDev, &PCI_Structure, sizeof(PCI_Structure));
#endif
}

BOOL Enable_PCI_SMBus_Reg(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
#if defined(_M_IX86) || defined(_M_X64)
  DWORD cc, t, Result, OriginalResult;
  int ret;

  // Must be the LPC Interface Bridge, register 0xF2 (Function Disable)
  if (Dev != 31 || Fun != 0 || Reg != 0xF2)
    return false;

  // Pack the Bus number, Device number, Function and Register into a DWORD
  cc = 0x80000000;
  cc = cc | ((Bus & 0xFF) << 16);
  cc = cc | ((Dev & 0x1F) << 11);
  cc = cc | ((Fun & 0x07) << 8);
  cc = cc | ((Reg & 0xFC));

  // Save the current address requested (may be in use by another process)
  ret = GetPortVal(0xCF8, (DWORD*)&t, 4); // 4 Bytes

  // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
  ret = SetPortVal(0xCF8, cc, 4);
  // Read the current result from the PCI config space for the Data
  ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);
  OriginalResult = Result;

  // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
  ret = SetPortVal(0xCF8, cc, 4);
  // Turn off bits 0 and 3 to enable the SMBus Host Controller (Note: in High Word)
  // See "Intel 82801EB ICH5 ... Datasheet" page 343 (section on FUNC_DIS)
  // bit	name
  // 3	D31_F3_Disable
  //	R/W. Software sets this bit to disable the SMBus Host Controller
  //	function. BIOS must not enable I/O or memory address space decode,
  //	interrupt generation, or any other functionality of functions that
  //	are to be disabled.
  //		0 = SMBus controller is enabled.
  //		1 = SMBus controller is disabled.
  //
  // 0	SMB_FOR_BIOS
  //	R/W. This bit is used in conjunction with bit 3 in this register.
  //		0 = No effect.
  //		1 = Allows the SMBus I/O space to be accessible by software
  //		    when bit 3 in this register is set. The PCI configuration
  //		    space is hidden in this case. Note that if bit 3 is set
  //		    alone, the decode of both SMBus PCI configuration and I/O
  //		    space will be disabled.
  // Ian's PC 0x0040 000f == 0000000001000000 0000000000001111 Bits 0 and 3 (of the High WORD) are off hence enabled
  // HP8200PC 0x0049 000f == 0000000001001001 0000000000001111 Bits 0 and 3 (of the High WORD) are on hence disabled
  Result &= 0xFFF6FFFF; // Turn off bits 0 and 3 in the High Word

  // So if SMBus is disabled - then enable it
  if (OriginalResult != Result)
  {
    // Write the result from the PCI config space for the Data
    ret = SetPortVal(0xCFC, Result, 4);

    // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);
    // Read the current result from the PCI config space for the Data
    ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);

    // Restore the current address requested (for another process)
    ret = SetPortVal(0xCF8, t, 4);

    return true;
  }
#endif
  return false;
}

bool Enable_PCI_SIS96X_SMBus(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
#if defined(_M_IX86) || defined(_M_X64)
  DWORD cc, t, Result;
  int ret;

  _stprintf_s(g_szSITempDebug1, _T("Enable_PCI_SIS96X_SMBus 0x%x, 0x%x, 0x%x, 0x%x"), Bus, Dev, Fun, Reg);
  SysInfo_DebugLogWriteLine(g_szSITempDebug1);

  if (Dev != 2 || Fun != 0 || Reg != 0x77)
    return false;

  // Pack the Bus number, Device number, Function and Register into a DWORD
  cc = 0x80000000;
  cc = cc | ((Bus & 0xFF) << 16);
  cc = cc | ((Dev & 0x1F) << 11);
  cc = cc | ((Fun & 0x07) << 8);
  cc = cc | ((Reg & 0xFC));

  // Save the current address requested (may be in use by another process)
  ret = GetPortVal(0xCF8, (DWORD*)&t, 4); // 4 Bytes

  // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
  ret = SetPortVal(0xCF8, cc, 4);
  // Read the current result from the PCI config space for the Data
  ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);

  // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
  ret = SetPortVal(0xCF8, cc, 4);

  _stprintf_s(g_szSITempDebug1, _T("Enable_PCI_SIS96X_SMBus Result=0x%08X"), Result);
  SysInfo_DebugLogWriteLine(g_szSITempDebug1);

  if (Result & 0x10)
  {
#if 0
    Result &= ~0x10;

    //In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);
    //Write the result from the PCI config space for the Data
    ret = SetPortVal(0xCFC, Result, 4);

    //In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);
    //Read the current result from the PCI config space for the Data
    ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);

    //Restore the current address requested (for another process)
    ret = SetPortVal(0xCF8, t, 4);

    return true;
#endif
  }
#endif
  return false;
}

bool Enable_PCI_SIS964_SMBus(DWORD Bus, DWORD Dev, DWORD Fun, DWORD Reg)
{
#if defined(_M_IX86) || defined(_M_X64)
  DWORD cc, t, Result;
  int ret;

  _stprintf_s(g_szSITempDebug1, _T("Enable_PCI_SIS964_SMBus 0x%x, 0x%x, 0x%x, 0x%x"), Bus, Dev, Fun, Reg);
  SysInfo_DebugLogWriteLine(g_szSITempDebug1);

  if (Dev != 2 || Fun != 0 || Reg != 0x76)
    return false;

  // Pack the Bus number, Device number, Function and Register into a DWORD
  cc = 0x80000000;
  cc = cc | ((Bus & 0xFF) << 16);
  cc = cc | ((Dev & 0x1F) << 11);
  cc = cc | ((Fun & 0x07) << 8);
  cc = cc | ((Reg & 0xFC));

  // Save the current address requested (may be in use by another process)
  ret = GetPortVal(0xCF8, (DWORD*)&t, 4); // 4 Bytes

  // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
  ret = SetPortVal(0xCF8, cc, 4);
  // Read the current result from the PCI config space for the Data
  ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);

  // In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
  ret = SetPortVal(0xCF8, cc, 4);

  _stprintf_s(g_szSITempDebug1, _T("Enable_PCI_SIS964_SMBus Result=0x%08X"), Result);
  SysInfo_DebugLogWriteLine(g_szSITempDebug1);

  if (!(Result & 0x01))
  {
#if 0
    Result |= 0x01;

    //In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);
    //Write the result from the PCI config space for the Data
    ret = SetPortVal(0xCFC, Result, 4);

    //In the PCI config space for the Address, set the address for Bus/Dev/Fun/Reg
    ret = SetPortVal(0xCF8, cc, 4);
    //Read the current result from the PCI config space for the Data
    ret = GetPortVal(0xCFC, (DWORD*)&Result, 4);

    //Restore the current address requested (for another process)
    ret = SetPortVal(0xCF8, t, 4);

    return true;
#endif
  }
#endif
  return false;
}

#ifdef WIN32
TCHAR PciInfoStr[MAX_RESULT_BUFF_LEN] = { _T('\0') };
struct new_ltstr
{
  bool operator()(std::wstring s1, std::wstring s2) const
  {
    if (s1.compare(s2) > 0)
      return true;
    return false;
  }
};

std::map<std::wstring, std::wstring, new_ltstr> vendor_list;

// Function to process value read from pci config space and find corresponding device
int SysInfo_FindPciInfo(DWORD Vendor_ID, DWORD Device_ID, DWORD SubID)
{
  if (SysInfo_IsLogging())							// SI101017 - only construct string if debugging
    SysInfo_DebugLogWriteLine(_T("Find PCI info")); // SI101017

  std::map<std::wstring, std::wstring, new_ltstr>::iterator vendor_info;
  std::map<std::wstring, std::wstring, new_ltstr>::iterator device_info;

  TCHAR vendor_key[VSHORT_STRING_LEN] = { _T('\0') };
  // TCHAR		device_key[SHORT_STRING_LEN] = {_T('\0')};
  TCHAR device_key[LONG_STRING_LEN] = { _T('\0') }; // SI101017 - lengthened - shoudln't be needed

  TCHAR szTmpLine[VLONG_STRING_LEN] = { _T('\0') };
  TCHAR vendorName[VLONG_STRING_LEN] = { _T('\0') };				   // SI101017 - lengthened
  SYSIN_GETSTR(SYSIN::SI_IDS_UNKNOWN, vendorName, VLONG_STRING_LEN); // "Unknown"

  // cause a fault
  // DWORD *pdw = NULL;
  //*pdw = 1;
  ////////DEBUGGING

  // SubID is only significant if SubID > 0
  if (SubID > 0)
  {
    DWORD SubVenID = SubID & 0xffff;
    DWORD SubDevID = (SubID >> 16);
  }

  // Create a separate key from vendor and device
  _stprintf_s(vendor_key, _T("%04x"), Vendor_ID);
  _stprintf_s(device_key, _T("%04x:%04x"), Vendor_ID, Device_ID);

  // Change to upper case to match pcidevs.txt
  CharUpperBuff(vendor_key, VSHORT_STRING_LEN);
  CharUpperBuff(device_key, SHORT_STRING_LEN);

  // Add vendor information
  vendor_info = vendor_list.find(vendor_key);
  if (vendor_info != vendor_list.end())
    wcscpy(vendorName, vendor_info->second.c_str());

  device_info = vendor_list.find(device_key);

  // Add device information
  if (device_info != vendor_list.end())
  {
    // all strings are prefixed with ; to serve as delimiter
    _stprintf_s(szTmpLine, _T("Vendor %ls Device %ls "), vendorName, device_info->second.c_str());
  }
  else
  {
    _stprintf_s(szTmpLine, _T("Vendor %ls Device info %ls "), vendorName, device_key);
  }
  if (SysInfo_IsLogging())				  // SI101017 - only construct string if debugging
    SysInfo_DebugLogWriteLine(szTmpLine); // added SI101017

  // Add additional information (if any)
  // SubID is only significant if SubID > 0
  if (SubID > 0)
  {
    std::map<std::wstring, std::wstring, new_ltstr>::iterator sub_device_info;

    DWORD SubVenID = SubID & 0xffff;
    DWORD SubDevID = (SubID >> 16);
    // TCHAR		temp[SHORT_STRING_LEN] = {_T('\0')};
    TCHAR temp[LONG_STRING_LEN] = { _T('\0') }; // SI101017 - lengthened - shoudln't be needed
    _stprintf(temp, _T(":%04x:%04x"), SubVenID, SubDevID);

    if (SysInfo_IsLogging())								 // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Finding PCI info 1")); // SI101017

    // Create the key to find additional information
    //_tcscat_s(device_key, SHORT_STRING_LEN, temp);
    if (wcslen(temp) < LONG_STRING_LEN) // SI101017 - sanity check
    {
      wcscat(device_key, temp);
      // CharUpperBuff(device_key, SHORT_STRING_LEN);
      CharUpperBuff(device_key, LONG_STRING_LEN);
    }

    if (SysInfo_IsLogging())								 // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Finding PCI info 2")); // SI101017

    sub_device_info = vendor_list.find(device_key);

    if (SysInfo_IsLogging())								 // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Finding PCI info 3")); // SI101017

    // Add device information
    if (sub_device_info != vendor_list.end())
    {
      if (SysInfo_IsLogging())								 // SI101017 - only construct string if debugging
        SysInfo_DebugLogWriteLine(_T("Finding PCI info 4")); // SI101017

      // all strings are prefixed with ; to serve as delimiter
      _tcscat(szTmpLine, (sub_device_info->second.c_str()));
      if (SysInfo_IsLogging())				  // SI101017 - only construct string if debugging
        SysInfo_DebugLogWriteLine(szTmpLine); // added SI101017
    }
  }

  // copy device info to global storage
  if (wcslen(PciInfoStr) + wcslen(szTmpLine) < MAX_RESULT_BUFF_LEN)
  {
    _tcscat_s(PciInfoStr, MAX_RESULT_BUFF_LEN, szTmpLine);

    // Add delimiter
    _tcscat_s(PciInfoStr, MAX_RESULT_BUFF_LEN, _T(";"));
  }

  if (SysInfo_IsLogging())										// SI101017 - only construct string if debugging
    SysInfo_DebugLogWriteLine(_T("Finished finding PCI info")); // SI101017

  return 0;
}

/* function which stores all the device information read from pcidevs.txt to map container*/
int MakeNewVendorList(TCHAR* str)
{

  TCHAR* temp1 = NULL, * temp2 = NULL;

  temp1 = _tcstok(str, _T("="));
  temp2 = _tcstok(NULL, _T("="));

  if (temp1 && temp2)
  {
    vendor_list[temp1] = temp2;
    return TRUE;
  }

  return false;
}

/* read the file pcidevs.txt which has information regarding vendor and device
  Store the information as map container to retrive relevent vendor and device info when required
  */

int ReadPcidevFile(void)
{
  TCHAR self_filepath[MAX_FILENAME] = { '\0' };
  TCHAR* tempStr, Line[VLONG_STRING_LEN] = { '\0' };
  FILE* hRead;
  DWORD dwBytesRead = 0;

  tempStr = Line;
  DWORD dwRet = GetModuleFileName(NULL, self_filepath, MAX_PATH);

  if (dwRet == 0)
    return false;

  // Move one levels up in directory structure
  TCHAR* ext = _tcsrchr(self_filepath, '\\');
  if (ext != NULL)
    *ext = '\0';

  // add file name
  _tcscat(self_filepath, _T("\\pcidevs.txt"));

  // Open output file
  if ((hRead = _tfopen(_T("pcidevs.txt"), _T("r, ccs= UNICODE"))) == NULL)
  {
    return false;
  }

  // Calculate the size of file
  fseek(hRead, 0, SEEK_END);	 // go to end
  DWORD dwSize = ftell(hRead); // get position at end (length)
  fseek(hRead, 0, SEEK_SET);	 // go to beg.

  BYTE* buffer = new BYTE[dwSize + 1];
  memset(buffer, 0, sizeof(BYTE) * (dwSize + 1));

  while (fgetws(Line, LONG_STRING_LEN, hRead))
  {
    if ((tempStr = _tcschr(Line, '=')) != NULL)
    {
      MakeNewVendorList(Line);
    }
    // add line to map container
  }

  // Close the handle to the file
  delete[] buffer;
  fclose(hRead);
  return true;
}
#endif

BOOL Scan_PCI(int iScanType, PCIINFO* smbDevice, int* numDevices)
{

  DWORD Bus, Dev, Fun;
  PCIINFO Info;
  DWORD Data;

  int maxSMB = 0;
  int numSMB = 0;

  memset(&Info, 0, sizeof(Info));

  if (iScanType == PCI_A_SMBUS)
  {
    if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Searching PCI for SMBus Controller"));

    if (numDevices)
    {
      maxSMB = *numDevices;
      *numDevices = 0;
    }
  }
  else if (iScanType == PCI_A_TSOD)
  {
    if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Searching PCI for TSOD Controller"));

    if (numDevices)
    {
      maxSMB = *numDevices;
      *numDevices = 0;
    }
  }
  else if (iScanType == PCI_A_SCAN)
  {
    if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Scanning PCI for devices:"));
#ifdef WIN32
    // Read complete pcidevs.txt file
    ReadPcidevFile();
#endif
  }
  else if (iScanType == PCI_A_ENABLESMBUS)
  {
    if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Attempting to enable SMBUS:"));

    Data = Get_PCI_Reg(0, 31, 0, 0);

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
      DWORD dwVendorID = Data & 0x0000ffff;
      DWORD dwDeviceID = (Data >> 16) & 0x0000ffff;
      if (Check_Enable_PCI_SMBus(dwVendorID, dwDeviceID, 0, 31, 0))
        return TRUE;
    }

    Data = Get_PCI_Reg(0, 2, 0, 0);

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
      DWORD dwVendorID = Data & 0x0000ffff;
      DWORD dwDeviceID = (Data >> 16) & 0x0000ffff;
      if (Check_Enable_PCI_SMBus(dwVendorID, dwDeviceID, 0, 2, 0))
        return TRUE;
    }

    return FALSE;
  }
#ifdef WIN32
  else if (iScanType == PCI_INTEL_PCH_SMBUS)
  {
    if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
      SysInfo_DebugLogWriteLine(_T("Scanning Intel PCH for SMBus:"));
    Data = Get_PCI_Reg(0x00, 0x1f, 0x04, 0);
    if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
    {
      _stprintf_s(g_szSITempDebug1, _T("Intel PCH DID: %08x"), Data);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);
    }

    if ((Data != 0xFFFFFFFF) && (Data != 0))
    {
      Info = Get_Info(0x00, 0x1f, 0x04, 0);

      _stprintf_s(g_szSITempDebug1, _T(" Found SMBus device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p PCI Add:{%02X:%02X:%02X:%04X} Rev:%.2X [%s %s]"),
        Info.dwVendor_ID, Info.dwDevice_ID,
        Info.byBus, Info.byDev, Info.byFun,
        Info.dwSMB_Address,
        Info.dwSMB_Address2,
        (PTR)Info.SMB_Address_MMIO,
        Info.SMB_Address_PCI.dwBus, Info.SMB_Address_PCI.dwDev, Info.SMB_Address_PCI.dwFun, Info.SMB_Address_PCI.dwReg,
        Info.byRev,
        Info.tszVendor_Name, Info.tszDevice_Name);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      memcpy(&g_PCIInfo[0x00][0x1f][0x04], &Info, sizeof(Info));
      return TRUE;
    }
    return FALSE;
  }
#endif
  else
    return FALSE;

  for (Bus = 0; Bus <= 0xFF; Bus++)
  {
    for (Dev = 0; Dev <= 0x1F; Dev++)
    {
      for (Fun = 0; Fun <= 0x07; Fun++)
      {
        Data = Get_PCI_Reg(Bus, Dev, Fun, 0);

        // If function 0 does not exist, don't enumerate other function numbers
        if (Data == 0xffffffff && Fun == 0)
          break;

        Info.dwSMB_Address = 0;
        if ((Data != 0xFFFFFFFF) && (Data != 0))
        {
#ifdef WIN32
          if (Data == 0x028C9005) // crash on probing Adaptec RAID card
          {
            _stprintf_s(g_szSITempDebug1, _T("Adaptec Series 7 6G SAS/PCIe 3 detected on Bus:%.2X. Skipping..."), Bus);
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
            Bus++;
            continue;
          }
#endif

          // Get info about the PCI device, translate SMB contorller devices into a textual desctiption
#ifdef WIN32
          Info = Get_Info(Data, Bus, Dev, Fun);
#else
          Get_Info(Data, Bus, Dev, Fun, &Info);
#endif

          // Enable the SMBus Controller
          if (iScanType == PCI_A_ENABLESMBUS)
          {
            DWORD dwVendorID = Data & 0x0000ffff;
            DWORD dwDeviceID = (Data >> 16) & 0x0000ffff;
            if (Check_Enable_PCI_SMBus(dwVendorID, dwDeviceID, Bus, Dev, Fun))
              return TRUE;
          }
          else if (iScanType == PCI_A_SMBUS)
          {
            if (Info.dwDeviceType == PCI_T_SMBUS)
            {
#ifdef WIN32
              memcpy(&g_PCIInfo[Bus][Dev][Fun], &Info, sizeof(Info));
#endif
              if (SysInfo_IsLogging()) // SI101017 - only construct string if debug logging
              {
                if ((wcslen(Info.tszVendor_Name) < LONG_STRING_LEN) && (wcslen(Info.tszDevice_Name) < LONG_STRING_LEN)) // SI101017 - length check for overrun
                {
                  _stprintf_s(g_szSITempDebug1, _T(" Found SMBus device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p PCI Add:{%02X:%02X:%02X:%04X} Rev:%.2X [%s %s]"),
                    Info.dwVendor_ID, Info.dwDevice_ID,
                    Info.byBus, Info.byDev, Info.byFun,
                    Info.dwSMB_Address,
                    Info.dwSMB_Address2,
                    (PTR)Info.SMB_Address_MMIO,
                    Info.SMB_Address_PCI.dwBus, Info.SMB_Address_PCI.dwDev, Info.SMB_Address_PCI.dwFun, Info.SMB_Address_PCI.dwReg,
                    Info.byRev,
                    Info.tszVendor_Name, Info.tszDevice_Name);
                }
                else
                {
                  _stprintf_s(g_szSITempDebug1, _T(" Found SMBus device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p PCI Add:{%02X:%02X:%02X:%04X} Rev:%.2X"),
                    Info.dwVendor_ID, Info.dwDevice_ID,
                    Info.byBus, Info.byDev, Info.byFun,
                    Info.dwSMB_Address,
                    Info.dwSMB_Address2,
                    (PTR)Info.SMB_Address_MMIO,
                    Info.SMB_Address_PCI.dwBus, Info.SMB_Address_PCI.dwDev, Info.SMB_Address_PCI.dwFun, Info.SMB_Address_PCI.dwReg,
                    Info.byRev);
                }
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);
              }

              // Check if we have a valid SMBus base address
              if (Info.dwSMB_Address != 0 || Info.dwSMB_Address2 != 0 || Info.SMB_Address_MMIO != 0 || Info.SMB_Address_PCI.dwReg != 0 || Info.smbDriver == SMBUS_INTEL_PCU || Info.smbDriver == SMBUS_INTEL_PCU_IL)
              {
                if (numSMB < maxSMB)
                {
                  memcpy(&smbDevice[numSMB], &Info, sizeof(Info));
                  numSMB++;
                }
              }
              else
              {
                if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
                  SysInfo_DebugLogWriteLine(_T(" SMBus/MMIO address invalid!"));
              }
            }
          }
          else if (iScanType == PCI_A_TSOD)
          {
            if (Info.tsodDriver != TSOD_NONE)
            {
#ifdef WIN32
              memcpy(&g_PCIInfo[Bus][Dev][Fun], &Info, sizeof(Info));
#endif
              if (SysInfo_IsLogging()) // SI101017 - only construct string if debug logging
              {
                if ((wcslen(Info.tszVendor_Name) < LONG_STRING_LEN) && (wcslen(Info.tszDevice_Name) < LONG_STRING_LEN)) // SI101017 - length check for overrun
                {
                  _stprintf_s(g_szSITempDebug1, _T(" Found TSOD device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p Rev:%.2X [%s %s]"),
                    Info.dwVendor_ID, Info.dwDevice_ID,
                    Info.byBus, Info.byDev, Info.byFun,
                    Info.dwSMB_Address,
                    Info.dwSMB_Address2,
                    (PTR)Info.MMCFGBase,
                    Info.byRev,
                    Info.tszVendor_Name, Info.tszDevice_Name);
                }
                else
                {
                  _stprintf_s(g_szSITempDebug1, _T(" Found TSOD device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p Rev:%.2X"),
                    Info.dwVendor_ID, Info.dwDevice_ID,
                    Info.byBus, Info.byDev, Info.byFun,
                    Info.dwSMB_Address,
                    Info.dwSMB_Address2,
                    (PTR)Info.MMCFGBase,
                    Info.byRev);
                }
                SysInfo_DebugLogWriteLine(g_szSITempDebug1);
              }

              // Check if we have a valid SMBus base address
              if (Info.dwSMB_Address != 0 || Info.MMCFGBase != 0 || Info.tsodDriver == TSOD_INTEL_PCU || Info.tsodDriver == TSOD_INTEL_PCU_IL)
              {
                if (numSMB < maxSMB)
                {
                  memcpy(&smbDevice[numSMB], &Info, sizeof(Info));
                  numSMB++;
                }
              }
              else
              {
                if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
                  SysInfo_DebugLogWriteLine(_T(" SMBus/MMIO address invalid!"));
              }
            }
          }
          else if (iScanType == PCI_A_SCAN)
          {
#ifdef WIN32
            memcpy(&g_PCIInfo[Bus][Dev][Fun], &Info, sizeof(Info));
#endif
            if (SysInfo_IsLogging()) // SI101017 - only construct string if debug logging
            {
              if ((wcslen(Info.tszVendor_Name) < LONG_STRING_LEN) && (wcslen(Info.tszDevice_Name) < LONG_STRING_LEN)) // SI101017 - length check for overrun
              {

                _stprintf_s(g_szSITempDebug1, _T(" VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p PCI Add:{%02X:%02X:%02X:%04X} Rev:%.2X HeaderType:%.2X [%s %s]"),
                  Info.dwVendor_ID, Info.dwDevice_ID,
                  Info.byBus, Info.byDev, Info.byFun,
                  Info.dwSMB_Address,
                  Info.dwSMB_Address2,
                  (PTR)Info.SMB_Address_MMIO,
                  Info.SMB_Address_PCI.dwBus, Info.SMB_Address_PCI.dwDev, Info.SMB_Address_PCI.dwFun, Info.SMB_Address_PCI.dwReg,
                  Info.byRev,
                  Info.byHeaderType,
                  Info.tszVendor_Name, Info.tszDevice_Name);
              }
              else
              {
                _stprintf_s(g_szSITempDebug1, _T(" VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p PCI Add:{%02X:%02X:%02X:%04X} Rev:%.2X HeaderType:%.2X"),
                  Info.dwVendor_ID, Info.dwDevice_ID,
                  Info.byBus, Info.byDev, Info.byFun,
                  Info.dwSMB_Address,
                  Info.dwSMB_Address2,
                  (PTR)Info.SMB_Address_MMIO,
                  Info.SMB_Address_PCI.dwBus, Info.SMB_Address_PCI.dwDev, Info.SMB_Address_PCI.dwFun, Info.SMB_Address_PCI.dwReg,
                  Info.byRev,
                  Info.byHeaderType);
              }
              SysInfo_DebugLogWriteLine(g_szSITempDebug1);
            }
#ifdef WIN32
            if (vendor_list.size() > 0)
            {
              DWORD SubReg;
              SubReg = Get_PCI_Reg(Bus, Dev, Fun, SUS_SYSTEM_OFFSET);
              SysInfo_FindPciInfo(Info.dwVendor_ID, Info.dwDevice_ID, SubReg);
            }
#endif
          }

          // If single-function device, don't enumerate other function numbers
          if (BitExtract(Info.byHeaderType, 7, 7) == 0 && Fun == 0)
            break;
        }
      }
    }
  }

  if (SysInfo_IsLogging())														  // SI101017 - only construct string if debugging
    SysInfo_DebugLogWriteLine(_T("Finished searching PCI for SMBus Controller")); // SI101017

  if (numDevices)
    *numDevices = numSMB;

  if (numSMB > 0)
    return TRUE;

  return FALSE;
}

BOOL Check_Enable_PCI_SMBus(DWORD dwVendorID, DWORD dwDeviceID, DWORD Bus, DWORD Dev, DWORD Fun)
{
  // If this is the LPC interface bridge for a 82801EB ICH5, then enale the SMBus controller at register 0xF2
  // Expand this for other chipsets (eg. ICH6) - but check the datasheet first (0xF2 is good for Intel ICH3/4/5 at least)
  if (Dev == 31 && Fun == 0 && dwVendorID == 0x8086 &&
    (dwDeviceID == 0x2480 || // ICH3 LPC Interface Bridge
      dwDeviceID == 0x24D0 || // ICH5 LPC Interface Bridge
      dwDeviceID == 0x24C0 || // ICH4 LPC Interface Bridge
      dwDeviceID == 0x24CC))	 // Intel 82801DBM I/O Controller Hub 4 Mobile (ICH4-M)
  {
    return Enable_PCI_SMBus_Reg(Bus, Dev, Fun, 0xF2);
  }
  else if (Dev == 31 && Fun == 0 && dwVendorID == 0x8086 &&
    (dwDeviceID == 0x2670 ||						  // Intel 631xspd
      dwDeviceID == 0x3B14 ||						  // Intel 3420
      dwDeviceID == 0x3B07 ||						  // Intel QM57 Chipset
      dwDeviceID == 0x2641 || dwDeviceID == 0x2640 || // Intel 82801FB/FR/82801FBM ICH6/ICH6M - LPC Bridge
      dwDeviceID == 0x27B8 ||						  // 82801GB/GR (ICH7 Family) LPC Interface Bridge
      dwDeviceID == 0x27B9 ||						  // 82801GBM (ICH7-M) LPC Interface Bridge
      dwDeviceID == 0x27BD ||						  // 82801GHM (ICH7-M DH) LPC Interface Bridge
      dwDeviceID == 0x2810 ||						  // 82801HB/HR (ICH8/R) LPC Interface Controller
      dwDeviceID == 0x2815 ||						  // 82801HM (ICH8M) LPC Interface Controller
      dwDeviceID == 0x2812 ||						  // 82801HH (ICH8DH) LPC Interface Controller
      dwDeviceID == 0x2814 ||						  // 82801HO (ICH8DO) LPC Interface Controller
      dwDeviceID == 0x2811 ||						  // 82801HEM (ICH8M-E) LPC Interface Controller
      dwDeviceID == 0x2912 ||						  // 82801IH (ICH9DH) LPC Interface Controller
      dwDeviceID == 0x2914 ||						  // 82801IO (ICH9DO) LPC Interface Controller
      dwDeviceID == 0x2916 ||						  // 82801IR (ICH9R) LPC Interface Controller
      dwDeviceID == 0x2918 ||						  // 82801IB (ICH9) LPC Interface Controller
      dwDeviceID == 0x2917 ||						  // ICH9M-E LPC Interface Controller
      dwDeviceID == 0x2919 ||						  // ICH9M LPC Interface Controller
      dwDeviceID == 0x3A14 ||						  // Intel 82801JDO ICH10R - LPC Bridge
      dwDeviceID == 0x3A16 ||						  // Intel 82801JB ICH10R - LPC Bridge
      dwDeviceID == 0x3A18 ||						  // Intel 82801JIB ICH10 - LPC Bridge
      dwDeviceID == 0x3B02 ||						  // Intel P55 Chipset
      dwDeviceID == 0x3B03 ||						  // Intel PM55 Chipset
      dwDeviceID == 0x3B06 ||						  // Intel H55 Chipset
      dwDeviceID == 0x3B07 ||						  // Intel QM57 Chipset
      dwDeviceID == 0x3B08 ||						  // Intel H57 Chipset
      dwDeviceID == 0x3B09 ||						  // Intel HM55 Chipset
      dwDeviceID == 0x3B0A ||						  // Intel Q57 Chipset
      dwDeviceID == 0x3B0B ||						  // Intel HM57 Chipset
      dwDeviceID == 0x3B0F ||						  // Intel QS57 Chipset
      dwDeviceID == 0x3B12 ||						  // Intel 3400 Chipset
      dwDeviceID == 0x3B14 ||						  // Intel 3420 Chipset
      dwDeviceID == 0x3B16 ||						  // Intel 3450 Chipset
      dwDeviceID == 0x1C4E ||						  // Intel Q67 Chipset
      dwDeviceID == 0x1C4C ||						  // Intel Q65 Chipset
      dwDeviceID == 0x1C50 ||						  // Intel B65 Chipset
      dwDeviceID == 0x1C4A ||						  // Intel H67 Chipset
      dwDeviceID == 0x1C44 ||						  // Intel Z68 Chipset
      dwDeviceID == 0x1C46 ||						  // Intel P67 Chipset
      dwDeviceID == 0x1C5C ||						  // Intel H61 Chipset
      dwDeviceID == 0x1C52 ||						  // Intel C202 Chipset
      dwDeviceID == 0x1C54 ||						  // Intel C204 Chipset
      dwDeviceID == 0x1C56 ||						  // Intel C206 Chipset
      dwDeviceID == 0x1C4F ||						  // Intel QM67 Chipset
      dwDeviceID == 0x1C47 ||						  // Intel UM67 Chipset
      dwDeviceID == 0x1C48 ||						  // Intel HM67 Chipset
      dwDeviceID == 0x1C49 ||						  // Intel HM65 Chipset - LPC Bridge
      dwDeviceID == 0x1C4D ||						  // Intel QS67 Chipset
      dwDeviceID == 0x1D40 ||						  // Intel X79 Express Chipset
      dwDeviceID == 0x1D41 ||						  // Intel X79 Express Chipset
      dwDeviceID == 0x1E47 ||						  // Intel Q77 Express Chipset
      dwDeviceID == 0x1E48 ||						  // Intel Q75 Express Chipset
      dwDeviceID == 0x1E49 ||						  // Intel B75 Express Chipset
      dwDeviceID == 0x1E44 ||						  // Intel Z77 Express Chipset
      dwDeviceID == 0x1E46 ||						  // Intel Z75 Express Chipset
      dwDeviceID == 0x1E4A ||						  // Intel H77 Express Chipset
      dwDeviceID == 0x1E53 ||						  // Intel C216 Chipset
      dwDeviceID == 0x1E55 ||						  // Mobile Intel QM77 Express Chipset - LPC Bridge
      dwDeviceID == 0x1E58 ||						  // Mobile Intel UM77 Express Chipset
      dwDeviceID == 0x1E57 ||						  // Mobile Intel HM77 Express Chipset - LPC Bridge
      dwDeviceID == 0x1E59 ||						  // Mobile Intel HM76 Express Chipset - LPC Bridge
      dwDeviceID == 0x1E5D ||						  // Mobile Intel HM75 Express Chipset - LPC Bridge
      dwDeviceID == 0x1E5E ||						  // Mobile Intel HM70 Express Chipset - LPC Bridge
      dwDeviceID == 0x1E56 ||						  // Mobile Intel QS77 Express Chipset - LPC Bridge
      dwDeviceID == 0x1E5F ||						  // Mobile Intel NM70 Express Chipset - LPC Bridge
      dwDeviceID == 0x27BC ||						  // Intel NM10 Family Express Chipset - LPC Bridge
      dwDeviceID == 0x8C41 ||						  // Intel 8 Series Mobile Engineering Sample - LPC Bridge
      dwDeviceID == 0x8C42 ||						  // Intel 8 Series Desktop Engineering Sample - LPC Bridge
      dwDeviceID == 0x8C44 ||						  // Intel Z87 Chipset
      dwDeviceID == 0x8C46 ||						  // Intel Z85 Chipset
      dwDeviceID == 0x8C49 ||						  // Intel HM86 Chipset
      dwDeviceID == 0x8C4A ||						  // Intel H87 Chipset
      dwDeviceID == 0x8C4B ||						  // Intel HM87 Chipset
      dwDeviceID == 0x8C4C ||						  // Intel Q85 Chipset
      dwDeviceID == 0x8C4E ||						  // Intel Q87 Chipset
      dwDeviceID == 0x8C4F ||						  // Intel QM87 Chipset
      dwDeviceID == 0x8C50 ||						  // Intel B85 Chipset
      dwDeviceID == 0x8C52 ||						  // Intel C222 Chipset
      dwDeviceID == 0x8C54 ||						  // Intel C224 Chipset
      dwDeviceID == 0x8C56 ||						  // Intel C226 Chipset
      dwDeviceID == 0x8C5C ||						  // Intel H81 Chipset
      dwDeviceID == 0x8D44))						  // Intel X99 (Wellsburg) Chipset - LPC Bridge
  {
    UINT_PTR FDRegAddr_MMIO;
    DWORD RCBAReg;
    DWORD FDRegVal;
    BOOL bRet = FALSE;

    RCBAReg = Get_PCI_Reg(Bus, 31, 0, 0xF0);
    _stprintf_s(g_szSITempDebug1, _T("RCBA Register: %08X"), RCBAReg);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

    FDRegAddr_MMIO = (RCBAReg & 0xFFFFC000) + 0x3418;
    _stprintf_s(g_szSITempDebug1, _T("Function Disable Register MMIO Addr: %p"), (PTR)FDRegAddr_MMIO);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
    // Ask driver to map MMIO base address to pointer we can use
    BYTE* pbLinAddr = NULL;
    tagPhys32Struct Phys32Struct = { 0 };
    // HANDLE		hPhysicalMemory	= NULL;
    pbLinAddr = MapPhysToLin((PBYTE)FDRegAddr_MMIO, 4, &Phys32Struct);
    if (pbLinAddr)
    {
      FDRegVal = *((DWORD*)pbLinAddr);
#else
    FDRegVal = MmioRead32(FDRegAddr_MMIO);
#endif
    _stprintf_s(g_szSITempDebug1, _T("Function Disable Register Value: %08X"), FDRegVal);
    SysInfo_DebugLogWriteLine(g_szSITempDebug1);
    if (FDRegVal & 0x08)
    {
      SysInfo_DebugLogWriteLine(_T("SMBUS controller is disabled. Enabling..."));
#ifdef WIN32
      DirectIo_FunctionDisableClear((PBYTE)FDRegAddr_MMIO, 3);
      FDRegVal = *((DWORD*)pbLinAddr);
#else
      MmioAnd32(FDRegAddr_MMIO, 0xFFFFFFF7);
      FDRegVal = MmioRead32(FDRegAddr_MMIO);
#endif

      _stprintf_s(g_szSITempDebug1, _T("Enabling complete. New Function Disable Register Value: %08X"), FDRegVal);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);
      bRet = TRUE;
    }
#ifdef WIN32
    UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
    }
#endif
  return bRet;
  }
  else if (Dev == 31 && Fun == 0 && dwVendorID == 0x8086 &&
    (dwDeviceID == 0x9D4E || // Intel 11th Gen Intel Core Processor Family PCH
    dwDeviceID == 0x02B4 || // Intel 400 series chipset family Platform Controller Hub(PCH)
    dwDeviceID == 0xA082))  // //Intel 8th Gen Intel Core Processor Family PCH
    {
      UINT_PTR FDRegAddr_MMIO;
      DWORD P2SB_PCIID;
      DWORD SBREG_BAR;
      DWORD SBREG_BARH;
      DWORD FDRegVal;
      BOOL bRet = FALSE;

      P2SB_PCIID = Get_PCI_Reg(Bus, 31, 1, 0x0);

      _stprintf_s(g_szSITempDebug1, _T("P2SB PCI ID: %08X"), P2SB_PCIID);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if ((P2SB_PCIID & 0xffff) == 0xffff)
      {
        DWORD P2SBC = Get_PCI_Reg(Bus, 31, 1, 0xE0);

        _stprintf_s(g_szSITempDebug1, _T("P2SBC: %08X"), P2SBC);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);

        if (P2SBC != 0xffffffff)
        {
          Set_PCI_Reg(Bus, 31, 1, 0xE0, P2SBC & ~(1 << 8));

          P2SBC = Get_PCI_Reg(Bus, 31, 1, 0xE0);

          _stprintf_s(g_szSITempDebug1, _T("New P2SBC: %08X"), P2SBC);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);

          P2SB_PCIID = Get_PCI_Reg(Bus, 31, 1, 0x0);

          _stprintf_s(g_szSITempDebug1, _T("New P2SB PCI ID: %08X"), P2SB_PCIID);
          SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        }

        if ((P2SB_PCIID & 0xffff) == 0xffff)
          return FALSE;
      }

      SBREG_BAR = Get_PCI_Reg(Bus, 31, 1, 0x10);
      _stprintf_s(g_szSITempDebug1, _T("SBREG_BAR Register: %08X"), SBREG_BAR);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if (SBREG_BAR == 0xffffffff || SBREG_BAR == 0)
        return FALSE;

      SBREG_BARH = Get_PCI_Reg(Bus, 31, 1, 0x14);
      _stprintf_s(g_szSITempDebug1, _T("SBREG_BARH Register: %08X"), SBREG_BARH);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      if (SBREG_BARH == 0xffffffff)
        return FALSE;

      // (SBREG_BAR + (port << 16) + reg)
      // SMBus Port ID: 0xC6
      // General Control (GC) - Offset Ch
      // Bit 0 - Function disable (FD)
      FDRegAddr_MMIO = (UINT_PTR)(((unsigned long long)SBREG_BARH << 32) + (SBREG_BAR & 0xFF000000) + (0xC6 << 16) + 0xC);
      _stprintf_s(g_szSITempDebug1, _T("Function Disable Register MMIO Addr: %p"), (PTR)FDRegAddr_MMIO);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
      // Ask driver to map MMIO base address to pointer we can use
      BYTE* pbLinAddr = NULL;
      tagPhys32Struct Phys32Struct = { 0 };
      // HANDLE		hPhysicalMemory	= NULL;
      pbLinAddr = MapPhysToLin((PBYTE)FDRegAddr_MMIO, 4, &Phys32Struct);
      if (pbLinAddr)
      {
        FDRegVal = *((DWORD*)pbLinAddr);
#else
      FDRegVal = MmioRead32(FDRegAddr_MMIO);
#endif
      _stprintf_s(g_szSITempDebug1, _T("Function Disable Register Value: %08X"), FDRegVal);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);
      if ((FDRegVal != 0xffffffff) && (FDRegVal & 1))
      {
        SysInfo_DebugLogWriteLine(_T("SMBUS controller is disabled. Enabling..."));
#ifdef WIN32
        DirectIo_FunctionDisableClear((PBYTE)FDRegAddr_MMIO, 0);
        FDRegVal = *((DWORD*)pbLinAddr);
#else
        MmioAnd32(FDRegAddr_MMIO, ~(UINT32)1);
        FDRegVal = MmioRead32(FDRegAddr_MMIO);
#endif

        _stprintf_s(g_szSITempDebug1, _T("Enabling complete. New Function Disable Register Value: %08X"), FDRegVal);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
        bRet = TRUE;
      }
#ifdef WIN32
      UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
      }
#endif
      return bRet;
      }
  else if (Dev == 2 && Fun == 0 && dwVendorID == 0x1039)
  {
    switch (dwDeviceID)
    {
    case 0x0961:
    case 0x0962:
    case 0x0963:
    {
      return Enable_PCI_SIS96X_SMBus(Bus, 2, 0, 0x77);
    }

    case 0x0964:
    case 0x0965:
    case 0x0966:
    {
      return Enable_PCI_SIS964_SMBus(Bus, 2, 0, 0x76);
    }
    default:
      break;
    }
  }
  return FALSE;
}

BOOL Enable_Intel801_SMBus(const PCIINFO* smbDevice)
{
  DWORD dwCMD;
  DWORD dwHCFG;

  if (smbDevice == NULL)
    return FALSE;

  if (smbDevice->smbDriver != SMBUS_INTEL_801 && smbDevice->smbDriver != SMBUS_INTEL_801_SPD5)
    return FALSE;

  // Workaround on Windows 10.
  // BSOD occurs when attempting to read PCI registers (either CF8/CFC or IOCTL_DIRECTIO_READPCI)
  if (NeedBSODWorkaround())
    return FALSE;

  // Check Command (CMD) 0x4h register - I/O Space Enable (IOSE)
  dwCMD = Get_PCI_Reg(smbDevice->byBus, smbDevice->byDev, smbDevice->byFun, 0x04);

  _stprintf_s(g_szSITempDebug1, _T("CMD: %08X"), dwCMD);
  SysInfo_DebugLogWriteLine(g_szSITempDebug1);

  if ((dwCMD & 1) == 0)
  {
    SysInfo_DebugLogWriteLine(_T("Setting IOSE=1"));
    Set_PCI_Reg(smbDevice->byBus, smbDevice->byDev, smbDevice->byFun, 0x04, dwCMD | 1);
  }

  // Check Host Configuration (HCFG) 0x40h register - HST_EN (HSTEN)
  dwHCFG = Get_PCI_Reg(smbDevice->byBus, smbDevice->byDev, smbDevice->byFun, 0x40);

  _stprintf_s(g_szSITempDebug1, _T("HCFG: %08X"), dwHCFG);
  SysInfo_DebugLogWriteLine(g_szSITempDebug1);

  if ((dwHCFG & 1) == 0)
  {
    SysInfo_DebugLogWriteLine(_T("Setting HSTEN=1"));
    Set_PCI_Reg(smbDevice->byBus, smbDevice->byDev, smbDevice->byFun, 0x40, dwHCFG | 1);
  }

  return TRUE;
}

#ifdef WIN32
BOOL Find_PCI_SMBus(PCIINFO* smbDevice, int* numDevices)
{
  DWORD Bus, Dev, Fun;

  int maxSMB = 0;
  int numSMB = 0;

  if (numDevices)
  {
    maxSMB = *numDevices;
    *numDevices = 0;
  }

  // Enumerate the PCI bus
  if (NeedBSODWorkaround())
    GetPCIInfoWorkaround();
  else
    Scan_PCI(PCI_A_SCAN, NULL, NULL);

  // Search for SMBus device
  for (Bus = 0; Bus <= 0xFF; Bus++)
  {
    for (Dev = 0; Dev <= 0x1F; Dev++)
    {
      for (Fun = 0; Fun <= 0x07; Fun++)
      {
        if (g_PCIInfo[Bus][Dev][Fun].dwDeviceType == PCI_T_SMBUS)
        {

          if (SysInfo_IsLogging()) // SI101017 - only construct string if debug logging
          {
            if ((wcslen(g_PCIInfo[Bus][Dev][Fun].tszVendor_Name) < LONG_STRING_LEN) && (wcslen(g_PCIInfo[Bus][Dev][Fun].tszDevice_Name) < LONG_STRING_LEN)) // SI101017 - length check for overrun
            {
              _stprintf_s(g_szSITempDebug1, _T(" Found SMBus device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p PCI Add:{%02X:%02X:%02X:%04X} Rev:%.2X [%s %s]"),
                g_PCIInfo[Bus][Dev][Fun].dwVendor_ID, g_PCIInfo[Bus][Dev][Fun].dwDevice_ID,
                g_PCIInfo[Bus][Dev][Fun].byBus, g_PCIInfo[Bus][Dev][Fun].byDev, g_PCIInfo[Bus][Dev][Fun].byFun,
                g_PCIInfo[Bus][Dev][Fun].dwSMB_Address,
                g_PCIInfo[Bus][Dev][Fun].dwSMB_Address2,
                (PTR)g_PCIInfo[Bus][Dev][Fun].SMB_Address_MMIO,
                g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwBus, g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwDev, g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwFun, g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwReg,
                g_PCIInfo[Bus][Dev][Fun].byRev,
                g_PCIInfo[Bus][Dev][Fun].tszVendor_Name, g_PCIInfo[Bus][Dev][Fun].tszDevice_Name);
            }
            else
            {
              _stprintf_s(g_szSITempDebug1, _T(" Found SMBus device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X IO Add:%.4X IO (2) Add:%.4X MMIO Add:%p PCI Add:{%02X:%02X:%02X:%04X} Rev:%.2X"),
                g_PCIInfo[Bus][Dev][Fun].dwVendor_ID, g_PCIInfo[Bus][Dev][Fun].dwDevice_ID,
                g_PCIInfo[Bus][Dev][Fun].byBus, g_PCIInfo[Bus][Dev][Fun].byDev, g_PCIInfo[Bus][Dev][Fun].byFun,
                g_PCIInfo[Bus][Dev][Fun].dwSMB_Address,
                g_PCIInfo[Bus][Dev][Fun].dwSMB_Address2,
                (PTR)g_PCIInfo[Bus][Dev][Fun].SMB_Address_MMIO,
                g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwBus, g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwDev, g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwFun, g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwReg,
                g_PCIInfo[Bus][Dev][Fun].byRev);
            }
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
          }

          // Check if we have a valid SMBus base address
          if (g_PCIInfo[Bus][Dev][Fun].dwSMB_Address != 0 || g_PCIInfo[Bus][Dev][Fun].dwSMB_Address2 != 0 || g_PCIInfo[Bus][Dev][Fun].SMB_Address_MMIO != 0 || g_PCIInfo[Bus][Dev][Fun].SMB_Address_PCI.dwReg != 0 || g_PCIInfo[Bus][Dev][Fun].smbDriver == SMBUS_INTEL_PCU)
          {
            if (numSMB < maxSMB)
            {
              memcpy(&smbDevice[numSMB], &g_PCIInfo[Bus][Dev][Fun], sizeof(g_PCIInfo[Bus][Dev][Fun]));
              numSMB++;
            }
          }
          else
          {
            if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
              SysInfo_DebugLogWriteLine(_T(" SMBus address invalid!"));
          }
        }
      }
    }
  }

  if (SysInfo_IsLogging())														  // SI101017 - only construct string if debugging
    SysInfo_DebugLogWriteLine(_T("Finished searching PCI for SMBus Controller")); // SI101017

  DebugMessageBoxes(_T("Finished searching PCI for SMBus Controller"));

  if (numDevices)
    *numDevices = numSMB;

  if (numSMB > 0)
    return TRUE;

  return FALSE;
}

BOOL Enable_PCI_SMBus(void)
{
  DWORD Bus;

  // Search for SMBus device
  for (Bus = 0; Bus <= 0xFF; Bus++)
  {
    if (Check_Enable_PCI_SMBus(g_PCIInfo[Bus][31][0].dwVendor_ID, g_PCIInfo[Bus][31][0].dwDevice_ID, Bus, 31, 0))
      return TRUE;
    else if (Check_Enable_PCI_SMBus(g_PCIInfo[Bus][2][0].dwVendor_ID, g_PCIInfo[Bus][2][0].dwDevice_ID, Bus, 2, 0))
      return TRUE;

#if 0
    //If this is the LPC interface bridge for a 82801EB ICH5, then enale the SMBus controller at register 0xF2
    //Expand this for other chipsets (eg. ICH6) - but check the datasheet first (0xF2 is good for Intel ICH3/4/5 at least)		
    if (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && (g_PCIInfo[Bus][31][0].dwDevice_ID == 0x24D0 || g_PCIInfo[Bus][31][0].dwDevice_ID == 0x24C0)) //ICH5 and ICH4 LPC Interface Bridges, respectively.
    {
      Enable_PCI_SMBus_Reg(Bus, 31, 0, 0xF2);
      return TRUE;
    }
    else if (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x24CC)  //Intel 82801DBM I/O Controller Hub 4 Mobile (ICH4-M)
    {
      Enable_PCI_SMBus_Reg(Bus, 31, 0, 0xF2);
      return TRUE;
    }
    /*
    else if (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x2640) //ICH6
    {

      Enable_PCI_SMBus(Bus, 31, 0, 0xF2);
      return TRUE;
    }
    */
    else if (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x2670) //Intel 631xspd
    {
      // <km> Special case - we cannot use the usual method. We need to write into
      //      the Chipset Configuration Registers in memory space
      //      See datasheet for Intel 631xESB
    }
    else if ((g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x3B14) || //Intel 3420
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x3B07) || //Intel QM57 Chipset) 
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 &&
        (g_PCIInfo[Bus][31][0].dwDevice_ID == 0x2641 || g_PCIInfo[Bus][31][0].dwDevice_ID == 0x2640)) || //Intel 82801FB/FR/82801FBM ICH6/ICH6M - LPC Bridge
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x27B8) || //82801GB/GR (ICH7 Family) LPC Interface Bridge
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x3A14) || //Intel 82801JDO ICH10R - LPC Bridge
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x3A16) || //Intel 82801JB ICH10R - LPC Bridge
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x1C4F) || //Intel C200
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x1C49) || //Intel HM65 Chipset - LPC Bridge
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x1E55) || //Mobile Intel QM77 Express Chipset - LPC Bridge
      (g_PCIInfo[Bus][31][0].dwVendor_ID == 0x8086 && g_PCIInfo[Bus][31][0].dwDevice_ID == 0x8D44)) //Intel X99 (Wellsburg) Chipset - LPC Bridge
    {
      UINT_PTR FDRegAddr_MMIO;
      DWORD RCBAReg;
      DWORD FDRegVal;

      RCBAReg = Get_PCI_Reg(Bus, 31, 0, 0xF0);
      _stprintf_s(g_szSITempDebug1, _T("Intel 3420/5 Chipset RCBA Register: %08X"), RCBAReg);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

      FDRegAddr_MMIO = (RCBAReg & 0xFFFFC000) + 0x3418;
      _stprintf_s(g_szSITempDebug1, _T("Function Disable Register MMIO Addr: %08X"), FDRegAddr_MMIO);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);

#ifdef WIN32
      // Ask driver to map MMIO base address to pointer we can use
      BYTE* pbLinAddr = NULL;
      HANDLE		hPhysicalMemory = NULL;
      pbLinAddr = MapPhysToLin((PBYTE)FDRegAddr_MMIO, 4, &hPhysicalMemory, TRUE);
      if (pbLinAddr)
      {
        FDRegVal = *((DWORD*)pbLinAddr);
#else
      FDRegVal = MmioRead32(FDRegAddr_MMIO);
#endif
      _stprintf_s(g_szSITempDebug1, _T("Function Disable Register Value: %08X"), FDRegVal);
      SysInfo_DebugLogWriteLine(g_szSITempDebug1);
      if (FDRegVal & 0x08)
      {
        SysInfo_DebugLogWriteLine(_T("SMBUS controller is disabled. Enabling..."));
#ifdef WIN32
        * ((DWORD*)pbLinAddr) &= 0xFFFFFFF7;
        FDRegVal = *((DWORD*)pbLinAddr);
#else
        MmioAnd32(FDRegAddr_MMIO, 0xFFFFFFF7);
        FDRegVal = MmioRead32(FDRegAddr_MMIO);
#endif

        _stprintf_s(g_szSITempDebug1, _T("Enabling complete. New Function Disable Register Value: %08X"), FDRegVal);
        SysInfo_DebugLogWriteLine(g_szSITempDebug1);
      }
#ifdef WIN32
      UnmapPhysicalMemory(hPhysicalMemory, pbLinAddr);
      return TRUE;
      }
#endif
    return TRUE;
    }
    else if (g_PCIInfo[Bus][2][0].dwVendor_ID == 0x1039)
    {
      DWORD dwDevice_ID = g_PCIInfo[Bus][2][0].dwDevice_ID;
      switch (dwDevice_ID)
      {
      case 0x0961:
      case 0x0962:
      case 0x0963:
      case 0x0965:
      case 0x0966:
      {
        Enable_PCI_SIS96X_SMBus(Bus, 2, 0, 0x77);
        return TRUE;
      }
      default:
        break;
      }
      }
  }
#endif
}
return FALSE;
}

BOOL Find_PCI_TSOD(PCIINFO* tsodDevice, int* numDevices)
{
  DWORD Bus, Dev, Fun;

  int maxTSOD = 0;
  int numTSOD = 0;

  if (numDevices)
  {
    maxTSOD = *numDevices;
    *numDevices = 0;
  }

  // Enumerate the PCI bus
  if (NeedBSODWorkaround())
    GetPCIInfoWorkaround();
  else
    Scan_PCI(PCI_A_SCAN, NULL, NULL);

  // Search for SMBus device
  for (Bus = 0; Bus <= 0xFF; Bus++)
  {
    for (Dev = 0; Dev <= 0x1F; Dev++)
    {
      for (Fun = 0; Fun <= 0x07; Fun++)
      {
        if (g_PCIInfo[Bus][Dev][Fun].tsodDriver != TSOD_NONE)
        {
          if (SysInfo_IsLogging()) // SI101017 - only construct string if debug logging
          {
            if ((wcslen(g_PCIInfo[Bus][Dev][Fun].tszVendor_Name) < LONG_STRING_LEN) && (wcslen(g_PCIInfo[Bus][Dev][Fun].tszDevice_Name) < LONG_STRING_LEN)) // SI101017 - length check for overrun
            {
              _stprintf_s(g_szSITempDebug1, _T(" Found TSOD device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X MMIO Add:%p Rev:%.2X [%s %s]"),
                g_PCIInfo[Bus][Dev][Fun].dwVendor_ID, g_PCIInfo[Bus][Dev][Fun].dwDevice_ID,
                g_PCIInfo[Bus][Dev][Fun].byBus, g_PCIInfo[Bus][Dev][Fun].byDev, g_PCIInfo[Bus][Dev][Fun].byFun,
                (PTR)g_PCIInfo[Bus][Dev][Fun].MMCFGBase,
                g_PCIInfo[Bus][Dev][Fun].byRev,
                g_PCIInfo[Bus][Dev][Fun].tszVendor_Name, g_PCIInfo[Bus][Dev][Fun].tszDevice_Name);
            }
            else
            {
              _stprintf_s(g_szSITempDebug1, _T(" Found TSOD device:  VID:%.4X DID:%.4X Bus:%.2X Dev:%.2X Fun:%.2X MMIO Add:%p Rev:%.2X"),
                g_PCIInfo[Bus][Dev][Fun].dwVendor_ID, g_PCIInfo[Bus][Dev][Fun].dwDevice_ID,
                g_PCIInfo[Bus][Dev][Fun].byBus, g_PCIInfo[Bus][Dev][Fun].byDev, g_PCIInfo[Bus][Dev][Fun].byFun,
                (PTR)g_PCIInfo[Bus][Dev][Fun].MMCFGBase,
                g_PCIInfo[Bus][Dev][Fun].byRev);
            }
            SysInfo_DebugLogWriteLine(g_szSITempDebug1);
          }

          // Check if we have a valid SMBus base address
          if (g_PCIInfo[Bus][Dev][Fun].dwSMB_Address != 0 || g_PCIInfo[Bus][Dev][Fun].MMCFGBase != 0)
          {
            if (numTSOD < maxTSOD)
            {
              memcpy(&tsodDevice[numTSOD], &g_PCIInfo[Bus][Dev][Fun], sizeof(g_PCIInfo[Bus][Dev][Fun]));
              numTSOD++;
            }
          }
          else
          {
            if (SysInfo_IsLogging()) // SI101017 - only construct string if debugging
              SysInfo_DebugLogWriteLine(_T(" SMBus/MMIO address invalid!"));
          }
        }
      }
    }
  }

  if (SysInfo_IsLogging())											  // SI101017 - only construct string if debugging
    SysInfo_DebugLogWriteLine(_T("Finished searching PCI for TSOD")); // SI101017

  DebugMessageBoxes(_T("Finished searching PCI for TSOD"));

  if (numDevices)
    *numDevices = numTSOD;

  if (numTSOD > 0)
    return TRUE;

  return FALSE;
}
#endif

#define MEM_TRML_TEMPERATURE_REPORT_0 0x60

int Get_TSOD_Intel_E5_MMIO(DWORD dwVIDDID, BYTE Bus, BYTE Dev, BYTE Fun, PBYTE MMCFGBase)
{
  int ch;
  int numsl = 3;
  int numTSOD = 0;
  wchar_t tmpMsg[VLONG_STRING_LEN];
  PBYTE BaseAddr = MMCFGBase + (Bus * 0x100000) + (Dev * 0x8000) + (Fun * 0x1000) + 0x150;
  BYTE* pbLinAddr = BaseAddr;
#ifdef WIN32
  // HANDLE		hPhysicalMemory	= NULL;
  tagPhys32Struct Phys32Struct = { 0 };

  // Ask driver to map MMIO base address to pointer we can use
  pbLinAddr = MapPhysToLin(BaseAddr, 16, &Phys32Struct, FALSE);

  if (pbLinAddr)
  {
#endif
    switch (dwVIDDID)
    {
    case 0x2FB08086:
    case 0x6FB08086:
      ch = 0;
      break;
    case 0x2FB18086:
    case 0x6FB18086:
      ch = 1;
      break;
    case 0x2FB48086:
    case 0x6FB48086:
      ch = 2;
      break;
    case 0x2FB58086:
    case 0x6FB58086:
      ch = 3;
      break;
    case 0x2FD08086:
    case 0x6FD08086:
      ch = 2;
      break;
    case 0x2FD18086:
    case 0x6FD18086:
      ch = 3;
      break;
    case 0x2FD48086:
    case 0x6FD48086:
      ch = 4;
      break;
    case 0x2FD58086:
    case 0x6FD58086:
      ch = 5;
      break;
    }

    int sl;
    for (sl = 0; sl < numsl; sl++)
    {
      PBYTE pbLinBaseAddr = NULL;
      DWORD dwDIMMTEMPSTAT = 0;
      BOOL bValidTemp = FALSE;

      pbLinBaseAddr = pbLinAddr + sl * 4;

      // Get the current value of the registers
      dwDIMMTEMPSTAT = *((DWORD*)pbLinBaseAddr);

      _stprintf_s(tmpMsg, _T("[Ch %d] DIMMTEMPSTAT_%d=%08X"), ch, sl, dwDIMMTEMPSTAT);
      SysInfo_DebugLogWriteLine(tmpMsg);

      if (g_numTSODModules >= MAX_TSOD_SLOTS)
        expand_tsod_info_array();

      g_MemTSODInfo[g_numTSODModules].channel = ch;
      g_MemTSODInfo[g_numTSODModules].slot = sl;
#ifdef WIN32
      g_MemTSODInfo[g_numTSODModules].temperature = (float)(dwDIMMTEMPSTAT & 0xFF);
      bValidTemp = abs((int)g_MemTSODInfo[g_numTSODModules].temperature) < MAX_TSOD_TEMP;
#else
      g_MemTSODInfo[g_numTSODModules].temperature = (int)(dwDIMMTEMPSTAT & 0xFF) * 1000;
      bValidTemp = ABS(g_MemTSODInfo[g_numTSODModules].temperature) < MAX_TSOD_TEMP;
#endif
      if (bValidTemp)
      {
        g_numTSODModules++;
        numTSOD++;
      }
    }

#ifdef WIN32
    UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 16);
  }
#endif

  return numTSOD;
}

int Get_TSOD_Intel_E3_MMIO(DWORD dwVIDDID, BYTE Bus, PBYTE MCHBAR)
{
  int ch;
  int numTSOD = 0;
  wchar_t tmpMsg[VLONG_STRING_LEN];
  PBYTE BaseAddr = MCHBAR + INTEL_KABYLAKE_DDR_DIMM_TEMP_C0;
  BYTE* pbLinAddr = BaseAddr;
  PBYTE pbLinBaseAddr = NULL;
  volatile DWORD dwVal = 0;
#ifdef WIN32
  // HANDLE		hPhysicalMemory	= NULL;
  tagPhys32Struct Phys32Struct = { 0 };

  // Ask driver to map MMIO base address to pointer we can use
  pbLinAddr = MapPhysToLin(BaseAddr, 8, &Phys32Struct, FALSE);
#endif

  if (pbLinAddr)
  {
    for (ch = 0; ch < 2; ch++)
    {
      volatile DWORD dwDIMM_TEMP = 0;
      BOOL bValidTemp = FALSE;

      pbLinBaseAddr = pbLinAddr + (ch * 4);
      dwDIMM_TEMP = *((DWORD*)pbLinBaseAddr);

      _stprintf_s(tmpMsg, _T("[Ch %d] DDR_DIMM_TEMP=%08X"), ch, dwDIMM_TEMP);
      SysInfo_DebugLogWriteLine(tmpMsg);

      if (g_numTSODModules >= MAX_TSOD_SLOTS)
        expand_tsod_info_array();

      g_MemTSODInfo[g_numTSODModules].channel = ch;
      g_MemTSODInfo[g_numTSODModules].slot = 0;
#ifdef WIN32
      g_MemTSODInfo[g_numTSODModules].temperature = (float)(dwDIMM_TEMP & 0xFF);
      bValidTemp = abs((int)g_MemTSODInfo[g_numTSODModules].temperature) < MAX_TSOD_TEMP;
#else
      g_MemTSODInfo[g_numTSODModules].temperature = (int)(dwDIMM_TEMP & 0xFF) * 1000;
      bValidTemp = ABS(g_MemTSODInfo[g_numTSODModules].temperature) < MAX_TSOD_TEMP;
#endif
      if (bValidTemp)
      {
        g_numTSODModules++;
        numTSOD++;
      }

      if (g_numTSODModules >= MAX_TSOD_SLOTS)
        expand_tsod_info_array();

      g_MemTSODInfo[g_numTSODModules].channel = ch;
      g_MemTSODInfo[g_numTSODModules].slot = 1;
#ifdef WIN32
      g_MemTSODInfo[g_numTSODModules].temperature = (float)((dwDIMM_TEMP >> 8) & 0xFF);
      bValidTemp = abs((int)g_MemTSODInfo[g_numTSODModules].temperature) < MAX_TSOD_TEMP;
#else
      g_MemTSODInfo[g_numTSODModules].temperature = (int)((dwDIMM_TEMP >> 8) & 0xFF) * 1000;
      bValidTemp = ABS(g_MemTSODInfo[g_numTSODModules].temperature) < MAX_TSOD_TEMP;
#endif
      if (bValidTemp)
      {
        g_numTSODModules++;
        numTSOD++;
      }
    }
#ifdef WIN32
    UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 8);
#endif
  }

  BaseAddr = MCHBAR + INTEL_KABYLAKE_DDR_DIMM_THERMAL_CTL;
  pbLinAddr = BaseAddr;
#ifdef WIN32
  // Ask driver to map MMIO base address to pointer we can use
  pbLinAddr = MapPhysToLin(BaseAddr, 16, &Phys32Struct, FALSE);
#endif

  if (pbLinAddr)
  {
    pbLinBaseAddr = pbLinAddr;
    dwVal = *((DWORD*)pbLinBaseAddr);

    _stprintf_s(tmpMsg, _T("DIMM_THERMAL_CTL=%08X"), dwVal);
    SysInfo_DebugLogWriteLine(tmpMsg);

    dwVal = *((DWORD*)(pbLinBaseAddr + 0xC));

    _stprintf_s(tmpMsg, _T("DIMM_THERMAL_STATUS=%08X"), dwVal);
    SysInfo_DebugLogWriteLine(tmpMsg);

#ifdef WIN32
    UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 16);
#endif
  }

  BaseAddr = MCHBAR + INTEL_KABYLAKE_MEM_THERMAL_DPPM_STATUS;
  pbLinAddr = BaseAddr;
#ifdef WIN32
  // Ask driver to map MMIO base address to pointer we can use
  pbLinAddr = MapPhysToLin(BaseAddr, 4, &Phys32Struct, FALSE);
#endif

  if (pbLinAddr)
  {
    pbLinBaseAddr = pbLinAddr;
    dwVal = *((DWORD*)pbLinBaseAddr);

    _stprintf_s(tmpMsg, _T("MEM_THERMAL_DPPM_STATUS=%08X"), dwVal);
    SysInfo_DebugLogWriteLine(tmpMsg);

#ifdef WIN32
    UnmapPhysicalMemory(&Phys32Struct, pbLinAddr, 4);
#endif
  }

  return numTSOD;
}

/*
///////////////IR
mechanisms to get PCIe slot version and link width, in the context of adding this info to PCIeTest and also adding a system slot info section to BurnInTest(PCIe and M.2 slots).

System slots :
Intel(R) 6 Series / C200 Series Chipset Family PCI Express Root Port 3 - 1C14 :
Version 2.0, maximum link width : x1, current link width x1
Device : PassMark PCIe 2.0 Test Card

The mechanism I propose we use relies of getting the PCI registers from the PCI configuration space(a bit like we are doing for SMBUS).I have read that this is not the way to do it(as it's dangerous), and that a PCI filter driver should be written to do this.I can't see us doing that, so propose I use a combination of SetupDi APIs to get the PCIe bridge and the connected device info, then using the PCIe bridge info Root that

:
-SMBIOS
- WMI
- VID / DID lookup tables
- SetupDi APIs
- PCI config space

// ConsoleApplication2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

typedef struct _PCI_CAPABILITIES_HEADER {
UCHAR CapabilityID;
UCHAR Next;
} PCI_CAPABILITIES_HEADER, *PPCI_CAPABILITIES_HEADER;

typedef union _PCI_EXPRESS_CAPABILITIES_REGISTER {
struct {
USHORT CapabilityVersion : 4;
USHORT DeviceType : 4;
USHORT SlotImplemented : 1;
USHORT InterruptMessageNumber : 5;
USHORT Rsvd : 2;
};
USHORT AsUSHORT;
} PCI_EXPRESS_CAPABILITIES_REGISTER, *PPCI_EXPRESS_CAPABILITIES_REGISTER;


typedef union _PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER {
struct {
ULONG MaxPayloadSizeSupported : 3;
ULONG PhantomFunctionsSupported : 2;
ULONG ExtendedTagSupported : 1;
ULONG L0sAcceptableLatency : 3;
ULONG L1AcceptableLatency : 3;
ULONG Undefined : 3;
ULONG RoleBasedErrorReporting : 1;
ULONG Rsvd1 : 2;
ULONG CapturedSlotPowerLimit : 8;
ULONG CapturedSlotPowerLimitScale : 2;
ULONG Rsvd2 : 4;
};
ULONG  AsULONG;
} PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER, *PPCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER;


typedef union _PCI_EXPRESS_DEVICE_CONTROL_REGISTER {
struct {
USHORT CorrectableErrorEnable : 1;
USHORT NonFatalErrorEnable : 1;
USHORT FatalErrorEnable : 1;
USHORT UnsupportedRequestErrorEnable : 1;
USHORT EnableRelaxedOrder : 1;
USHORT MaxPayloadSize : 3;
USHORT ExtendedTagEnable : 1;
USHORT PhantomFunctionsEnable : 1;
USHORT AuxPowerEnable : 1;
USHORT NoSnoopEnable : 1;
USHORT MaxReadRequestSize : 3;
USHORT BridgeConfigRetryEnable : 1;
};
USHORT AsUSHORT;
} PCI_EXPRESS_DEVICE_CONTROL_REGISTER, *PPCI_EXPRESS_DEVICE_CONTROL_REGISTER;

typedef union _PCI_EXPRESS_DEVICE_STATUS_REGISTER {
struct {
USHORT CorrectableErrorDetected : 1;
USHORT NonFatalErrorDetected : 1;
USHORT FatalErrorDetected : 1;
USHORT UnsupportedRequestDetected : 1;
USHORT AuxPowerDetected : 1;
USHORT TransactionsPending : 1;
USHORT Rsvd : 10;
};
USHORT AsUSHORT;
} PCI_EXPRESS_DEVICE_STATUS_REGISTER, *PPCI_EXPRESS_DEVICE_STATUS_REGISTER;


typedef union _PCI_EXPRESS_LINK_CAPABILITIES_REGISTER {
struct {
ULONG MaximumLinkSpeed : 4;
ULONG MaximumLinkWidth : 6;
ULONG ActiveStatePMSupport : 2;
ULONG L0sExitLatency : 3;
ULONG L1ExitLatency : 3;
ULONG ClockPowerManagement : 1;
ULONG SurpriseDownErrorReportingCapable : 1;
ULONG DataLinkLayerActiveReportingCapable : 1;
ULONG Rsvd : 3;
ULONG PortNumber : 8;
};
ULONG  AsULONG;
} PCI_EXPRESS_LINK_CAPABILITIES_REGISTER, *PPCI_EXPRESS_LINK_CAPABILITIES_REGISTER;

typedef union _PCI_EXPRESS_LINK_CONTROL_REGISTER {
struct {
USHORT ActiveStatePMControl : 2;
USHORT Rsvd1 : 1;
USHORT ReadCompletionBoundary : 1;
USHORT LinkDisable : 1;
USHORT RetrainLink : 1;
USHORT CommonClockConfig : 1;
USHORT ExtendedSynch : 1;
USHORT EnableClockPowerManagement : 1;
USHORT Rsvd2 : 7;
};
USHORT AsUSHORT;
} PCI_EXPRESS_LINK_CONTROL_REGISTER, *PPCI_EXPRESS_LINK_CONTROL_REGISTER;



typedef union _PCI_EXPRESS_LINK_STATUS_REGISTER {
struct {
USHORT LinkSpeed : 4;
USHORT LinkWidth : 6;
USHORT Undefined : 1;
USHORT LinkTraining : 1;
USHORT SlotClockConfig : 1;
USHORT DataLinkLayerActive : 1;
USHORT Rsvd : 2;
};
USHORT AsUSHORT;
} PCI_EXPRESS_LINK_STATUS_REGISTER, *PPCI_EXPRESS_LINK_STATUS_REGISTER;


typedef union _PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER {
struct {
ULONG AttentionButtonPresent : 1;
ULONG PowerControllerPresent : 1;
ULONG MRLSensorPresent : 1;
ULONG AttentionIndicatorPresent : 1;
ULONG PowerIndicatorPresent : 1;
ULONG HotPlugSurprise : 1;
ULONG HotPlugCapable : 1;
ULONG SlotPowerLimit : 8;
ULONG SlotPowerLimitScale : 2;
ULONG ElectromechanicalLockPresent : 1;
ULONG NoCommandCompletedSupport : 1;
ULONG PhysicalSlotNumber : 13;
};
ULONG  AsULONG;
} PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER, *PPCI_EXPRESS_SLOT_CAPABILITIES_REGISTER;



typedef union _PCI_EXPRESS_SLOT_CONTROL_REGISTER {
struct {
USHORT AttentionButtonEnable : 1;
USHORT PowerFaultDetectEnable : 1;
USHORT MRLSensorEnable;
USHORT PresenceDetectEnable : 1;
USHORT CommandCompletedEnable : 1;
USHORT HotPlugInterruptEnable : 1;
USHORT AttentionIndicatorControl : 2;
USHORT PowerIndicatorControl : 2;
USHORT PowerControllerControl : 1;
USHORT ElectromechanicalLockControl : 1;
USHORT DataLinkStateChangeEnable : 1;
USHORT Rsvd : 3;
};
USHORT AsUSHORT;
} PCI_EXPRESS_SLOT_CONTROL_REGISTER, *PPCI_EXPRESS_SLOT_CONTROL_REGISTER;


typedef union _PCI_EXPRESS_SLOT_STATUS_REGISTER {
struct {
USHORT AttentionButtonPressed : 1;
USHORT PowerFaultDetected : 1;
USHORT MRLSensorChanged : 1;
USHORT PresenceDetectChanged : 1;
USHORT CommandCompleted : 1;
USHORT MRLSensorState : 1;
USHORT PresenceDetectState : 1;
USHORT ElectromechanicalLockEngaged : 1;
USHORT DataLinkStateChanged : 1;
USHORT Rsvd : 7;
};
USHORT AsUSHORT;
} PCI_EXPRESS_SLOT_STATUS_REGISTER, *PPCI_EXPRESS_SLOT_STATUS_REGISTER;



typedef union _PCI_EXPRESS_ROOT_CONTROL_REGISTER {
struct {
USHORT CorrectableSerrEnable : 1;
USHORT NonFatalSerrEnable : 1;
USHORT FatalSerrEnable : 1;
USHORT PMEInterruptEnable : 1;
USHORT CRSSoftwareVisibilityEnable : 1;
USHORT Rsvd : 11;
};
USHORT AsUSHORT;
} PCI_EXPRESS_ROOT_CONTROL_REGISTER, *PPCI_EXPRESS_ROOT_CONTROL_REGISTER;




typedef union _PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER {
struct {
USHORT CRSSoftwareVisibility : 1;
USHORT Rsvd : 15;
};
USHORT AsUSHORT;
} PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER, *PPCI_EXPRESS_ROOT_CAPABILITIES_REGISTER;


typedef union _PCI_EXPRESS_ROOT_STATUS_REGISTER {
struct {
ULONG PMERequestorId : 16;
ULONG PMEStatus : 1;
ULONG PMEPending : 1;
ULONG Rsvd : 14;
};
ULONG  AsULONG;
} PCI_EXPRESS_ROOT_STATUS_REGISTER, *PPCI_EXPRESS_ROOT_STATUS_REGISTER;


typedef struct _PCI_EXPRESS_CAPABILITY {
PCI_CAPABILITIES_HEADER                  Header;
PCI_EXPRESS_CAPABILITIES_REGISTER        ExpressCapabilities;
PCI_EXPRESS_DEVICE_CAPABILITIES_REGISTER DeviceCapabilities;
PCI_EXPRESS_DEVICE_CONTROL_REGISTER      DeviceControl;
PCI_EXPRESS_DEVICE_STATUS_REGISTER       DeviceStatus;
PCI_EXPRESS_LINK_CAPABILITIES_REGISTER   LinkCapabilities;
PCI_EXPRESS_LINK_CONTROL_REGISTER        LinkControl;
PCI_EXPRESS_LINK_STATUS_REGISTER         LinkStatus;
PCI_EXPRESS_SLOT_CAPABILITIES_REGISTER   SlotCapabilities;
PCI_EXPRESS_SLOT_CONTROL_REGISTER        SlotControl;
PCI_EXPRESS_SLOT_STATUS_REGISTER         SlotStatus;
PCI_EXPRESS_ROOT_CONTROL_REGISTER        RootControl;
PCI_EXPRESS_ROOT_CAPABILITIES_REGISTER   RootCapabilities;
PCI_EXPRESS_ROOT_STATUS_REGISTER         RootStatus;
} PCI_EXPRESS_CAPABILITY, *PPCI_EXPRESS_CAPABILITY;





int _tmain(int argc, _TCHAR* argv[])
{
PCI_EXPRESS_CAPABILITY pci;
*/
/*
PCI capability
Caps class		PCI Express
Caps offset		0x40
Device type		Root Port of PCI-E Root Complex
Port			2
Version			2.0
Physical slot		#0
Presence detect		yes
Link width		1x (max 1x)
*/
/*

static UCHAR pciConfig[] =
{
0x10, 0x80, 0x42, 0x01, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x12, 0x3C, 0x11, 0x02,
0x40, 0x00, 0x12, 0x70, 0x00, 0xB2, 0x0C, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
int ipci = sizeof(PCI_EXPRESS_CAPABILITY);
memcpy(&pci, pciConfig, sizeof(PCI_EXPRESS_CAPABILITY));

return 0;
}


#ifdef _DEBUG
Use SetuPDi to get PCIe bus port <n>, then
GetPCIConfigSpace(0, 1, 0);	//Just testing //////////////IR to delete
SMBIOS to get physical port designator
#endif


bool GetPCIConfigSpace(DWORD Bus, DWORD Dev, DWORD Fun)
{
//256 / sizeof(DWORD);
#define CONFIGSPACESIZE 64
DWORD pci[CONFIGSPACESIZE] = { 0 };
DWORD lastError = 0;

PCI_EXPRESS_CAPABILITY pciConfig;

//if (InitializeDirectIo(&lastError))
{
for (int i = 0; i < CONFIGSPACESIZE; i++)
{
int iReg = i*sizeof(DWORD);
pci[i] = Get_PCI_Reg(Bus, Dev, Fun, iReg);

}
}

memcpy(&pciConfig, pci, sizeof(PCI_EXPRESS_CAPABILITY));

return true;
}
*/
