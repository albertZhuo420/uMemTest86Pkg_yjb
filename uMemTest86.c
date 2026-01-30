// PassMark MemTest86
//
// Copyright (c) 2013-2016
//   This software is the confidential and proprietary information of PassMark
//   Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//   such Confidential Information and shall use it only in accordance with
//   the terms of the license agreement you entered into with PassMark
//   Software.
//
// Program:
//   MemTest86
//
// Module:
//   uMemTest86.c
//
// Author(s):
//   Keith Mah
//
// Description:
//   Entry point for MemTest86
//
// References:
//   https://github.com/jljusten/MemTestPkg
//
// History
//   27 Feb 2013: Initial version

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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/HiiLib.h>

#include <Library/MemTestUiLib.h>
#include <Library/MemTestRangesLib.h>
#include <Library/MemTestSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/Cpuid.h>
#include <Library/Rand.h>
#include <Library/TimerLib.h>

#include <Library/libeg/libeg.h>
#include <Library/libeg/BmLib.h>
#include <Library/libeg/lodepng.h>
#include <Library/DevicePathLib.h>
#include <Library/FileHandleLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/UnicodeCollation.h>
#include <Protocol/Runtime.h>
#include <Protocol/Cpu.h>
#include <Protocol/HiiFont.h>
#include <Protocol/HiiString.h>
#include <Protocol/PxeBaseCode.h>
#include <Protocol/Dhcp6.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/PartitionInfo.h>
#ifdef TCPIP_DEBUG
#include <Protocol/ServiceBinding.h>
#include <Protocol/Ip4.h>
#include <Protocol/Ip6.h>
#include <Protocol/Udp4.h>
#include <Protocol/Udp6.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Tcp6.h>
#include <Protocol/Http.h>
#endif
#include <Guid/Acpi.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/HighPrecisionEventTimerTable.h>

#ifdef SHELLAPP
#include <Library/ShellLib.h>
#include <Protocol/EfiShellEnvironment2.h>
#endif

#include <PointerLib/Pointer.h>
#include <SysInfoLib/pci.h>
#include <SysInfoLib/smbus.h>
#include <SysInfoLib/cputemp.h>
#include <SysInfoLib/cpuinfo.h>
#include <SysInfoLib/cpuinfoMSRIntel.h>
#include <SysInfoLib/cpuinfoARMv8.h>
#include <SysInfoLib/SMBIOS.h>

#include <YAMLLib/yaml.h>

#include <Library/MPSupportLib.h>

#include <Guid/GlobalVariable.h>
#include <Guid/HardwareErrorVariable.h>
#include <Guid/Cper.h>

#include <SysInfoLib/controller.h>

#include "imsdefect.h"

#include "AdvancedMemoryTest.h"

#include "LineChart.h"

#include "TestResultsStorage.h"

// #include "font.h"

// #include "sha2.h"

#include <Tests/AddressWalkingOnes/AddressWalkingOnes.h>
#include <Tests/AddressOwn/AddressOwn.h>
#include <Tests/MovingInv/MovingInv.h>
#include <Tests/BlockMove/BlockMove.h>
#include <Tests/MovingInv32bitPattern/MovingInv32bitPattern.h>
#include <Tests/RandomNumSequence/RandomNumSequence.h>
#include <Tests/Modulo20/Modulo20.h>
#include <Tests/BitFade/BitFade.h>
#include <Tests/RandomNumSequence64/RandomNumSequence64.h>
#include <Tests/RandomNumSequence128/RandomNumSequence128.h>
#include <Tests/Hammer/Hammer.h>
#include <Tests/DMATest/DMATest.h>

#include "uMemTest86.h"

#include "images/bench.h"
#include "images/config.h"
#include "images/cpu_sel.h"
#include "images/cputext.h"
#include "images/upgrade.h"
#include "images/exit.h"
#include "images/restart.h"
#include "images/shutdown.h"
#include "images/exit_app.h"
#include "images/mem_addr.h"
#include "images/memtext.h"
#include "images/mt86.h"
#include "images/mt86logo.h"
#include "images/mt86text.h"
#include "images/runtest.h"
#include "images/select.h"
#include "images/settings.h"
#include "images/sys_info.h"
#include "images/test_sel.h"
#include "images/dimm.h"
#include "images/dimm_none.h"
#include "images/dimm_vert.h"
#include "images/dimm_none_vert.h"
#include "images/sodimm.h"
#include "images/sodimm_none.h"
#include "images/ok.h"
#include "images/qrcode.h"
#include "images/qrcode_asus.h"
#include <Library/OKN/PortingLibs/cJSONLib.h>
#include <Library/OKN/OknUdp4SocketLib.h>
#include <Library/BaseCryptLib.h>
#include <Protocol/Smbios.h>

#define OKN_DDR4

#ifdef OKN_DDR4



STATIC VOID DisableUdpRxSocket(IN UDP4_SOCKET *Sock)
{
  if (NULL == Sock || NULL == Sock->Udp4) {
    return;
  }
  // Cancel the pending receive token to stop further callbacks.
  Sock->Udp4->Cancel(Sock->Udp4, &Sock->TokenReceive);
}

STATIC volatile BOOLEAN gUdpNicBound = FALSE;

STATIC EFI_STATUS WaitForUdpNicBind(UINTN TimeoutMs);
STATIC VOID PollAllUdpSockets(VOID);

STATIC EFI_STATUS StartUdp4ReceiveOnAllNics(IN EFI_UDP4_CONFIG_DATA *RxCfg);

VOID EFIAPI Udp4ReceiveHandler(IN EFI_EVENT Event, IN VOID *Context);
VOID EFIAPI Udp4NullHandler(IN EFI_EVENT Event, IN VOID *Context);


// TX 完成后释放本次发送申请的内存, 避免泄漏
STATIC VOID EFIAPI Udp4TxFreeHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  UDP4_SOCKET *Sock = (UDP4_SOCKET *)Context;
  if (NULL == Sock) {
    return;
  }

  // 释放本次发送的 Payload（我们在 ReceiveHandler 里用 AllocateCopyPool 分配）
  if (Sock->TxPayload != NULL) {
    FreePool(Sock->TxPayload);
    Sock->TxPayload = NULL;
  }

  // 释放本次发送的 SessionData
  if (Sock->TxSession != NULL) {
    FreePool(Sock->TxSession);
    Sock->TxSession = NULL;
  }

  Sock->TxInProgress = FALSE;
}

#endif  // OKN_DDR4

#define SEAVO
#include <Protocol/UsbIo.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/Dhcp4.h>

STATIC UINTN CountHandlesByProtocol(IN EFI_GUID *Guid);
STATIC VOID DumpNetProtoCounts(VOID);
STATIC VOID ConnectAllSnpControllers(VOID);
STATIC EFI_STATUS EnsureDhcpIp4Ready(EFI_HANDLE Handle, UINTN TimeoutMs, EFI_IPv4_ADDRESS *OutIp);
STATIC BOOLEAN IsZeroIp4(IN EFI_IPv4_ADDRESS *A);

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

STATIC CHAR8 gDbgBuf[512];

#define UDP_LOG(fmt, ...) \
  do { \
    AsciiSPrint(gDbgBuf, sizeof(gDbgBuf), "[UDP][%a:%d] " fmt "\r\n", __FILE__, __LINE__, ##__VA_ARGS__); \
    MtSupportDebugWriteLine(gDbgBuf); \
  } while (0)

#define UDP_LOG_STATUS(tag, st) \
  do { \
    UDP_LOG("%a: %r", tag, (st)); \
  } while (0)

#ifndef BUF_SIZE
#define BUF_SIZE 512
#endif

STATIC CHAR8 gUdpDbg[BUF_SIZE];

#define UDPDBG(fmt, ...) \
  do { \
    AsciiSPrint(gUdpDbg, sizeof(gUdpDbg), "[UDP][RXALL] " fmt, ##__VA_ARGS__); \
    MtSupportDebugWriteLine(gUdpDbg); \
  } while (0)

#define UDPDBG_ST(tag, st) \
  UDPDBG("%a: %r", tag, (st))

STATIC
CONST CHAR8*
OKN_BaseNameA(IN CONST CHAR8* Path)
{
  CONST CHAR8* p = Path;
  CONST CHAR8* last = Path;
  while (*p != '\0') {
    if (*p == '/' || *p == '\\') {
      last = p + 1;
    }
    p++;
  }
  return last;
}

#define PrintAndStop() do {\
	                         UINTN EventIndex_tmp;\
                            EFI_INPUT_KEY       key;\
                            Print(L"[%a:%d] ...\r\n", OKN_BaseNameA(__FILE__), __LINE__);\
                            gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex_tmp);\
                            gST->ConIn->ReadKeyStroke(gST->ConIn, &key);\
                       }while(0)

STATIC
VOID
PrintIpv4A(IN CONST EFI_IPv4_ADDRESS *Ip)
{
    Print(L"%u.%u.%u.%u",
          (UINTN)Ip->Addr[0],
          (UINTN)Ip->Addr[1],
          (UINTN)Ip->Addr[2],
          (UINTN)Ip->Addr[3]);
}

#define MIN_SCREEN_WIDTH 1024 // Minimum horizontal resolution
#define MIN_SCREEN_HEIGHT 768 // Minimum vertical resolution

#define MAX_SCREEN_WIDTH 1920  // Maximum horizontal resolution
#define MAX_SCREEN_HEIGHT 1200 // Maximum vertical resolution

#define COUNTDOWN_TIME_MS 10000 // Splash screen countdown time in milliseconds

#define PROG_BAR_WIDTH 350     // Progress bar width
#define BOM_UTF16LE "\xff\xfe" // UTF16-LE BOM

#define RESTRICT_ADDR_LOW_LIMIT 0x100000 // 0x0-0xFFFFF BIOS area

#define EFI_TEXT_ATTR_BG(Attr) ((Attr >> 4) & 0x0F)
#define BG_COLOUR (mEfiColors[EFI_TEXT_ATTR_BG(gST->ConOut->Mode->Attribute)])

#ifdef SHELLAPP // not used
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
    {L"-d", TypeFlag},
    {NULL, TypeMax}};

EFI_SHELL_ENVIRONMENT2 *mEfiShellEnvironment2;
EFI_HANDLE mEfiShellEnvironment2Handle;
#endif

/* Globals */
EFI_TIME gBootTime;
EFI_PXE_BASE_CODE_PROTOCOL *PXEProtocol = NULL;    // Protocol to handle PXE booting
EFI_SIMPLE_NETWORK_PROTOCOL *SimpleNetwork = NULL; // Protocol for transmitting/receiving packets
EFI_IP_ADDRESS ServerIP;                           // IP address of the PXE server
EFI_IP_ADDRESS ClientIP;                           // IP address of the client;
EFI_MAC_ADDRESS ClientMAC;                         // MAC address of the client;
CHAR16 gTftpRoot[256];                             // The path in the PXE server to read/store MemTest86 related files
EFI_IP_ADDRESS gTFTPServerIp;
UINTN gTFTPStatusSecs = DEFAULT_TFTP_STATUS_SECS; // Management console status report period
BOOLEAN gPMPDisable = FALSE;                      // Disable XML reporting to PXE server
BOOLEAN gTCPDisable = TRUE;
BOOLEAN gDHCPDisable = TRUE;
BOOLEAN gCloudEnable; // Connect to cloud instance with specified API key
CHAR16 gAPIKey[256];

BOOLEAN gRTCSync = FALSE; // Sync real-time clock with PXE server

EFI_DEVICE_PATH_PROTOCOL *SelfDevicePath = NULL; // Loaded binary device path
EFI_FILE *SelfRootDir = NULL;                    // Root directory of binary
EFI_FILE *SelfDir = NULL;                        // Directory path of binary
CHAR16 *SelfDirPath = NULL;                      // Directory path of binary
EFI_LOADED_IMAGE *SelfLoadedImage = NULL;        // Loaded binary info

EFI_HANDLE *FSHandleList = NULL;
UINTN NumFSHandles = 0;
BOOLEAN IsVirtualDisk = FALSE; // Is running from virtual disk (ie. image embedded in BIOS)

static const EFI_GUID DATA_PART_GUID =
    {0x50415353, 0xce04, 0x4f8f, {0xAC, 0x32, 0x6f, 0x36, 0x93, 0xf4, 0x37, 0x04}};

static const EFI_GUID EFI_PART_GUID =
    {0x50415353, 0x6343, 0x46c0, {0xb6, 0x8c, 0x69, 0xee, 0x63, 0x26, 0xf3, 0x2b}};

static const EFI_GUID DMA_TEST_PART_GUID =
    {0x50415353, 0x2A40, 0x4503, {0xB2, 0xF7, 0x47, 0x4E, 0x00, 0xEB, 0xFB, 0xAA}};

EFI_BLOCK_IO_PROTOCOL *DMATestPart = NULL; // Block I/O Protocol for DMA test partition

BOOL CheckTcp4Support()
{
#if defined(TCPIP_DEBUG) && defined(SITE_EDITION) && !defined(TRIAL)
    EFI_STATUS Status;
    EFI_SERVICE_BINDING_PROTOCOL *Tcp4Service = NULL, *Tcp6Service = NULL, *Udp4Service = NULL, *Udp6Service = NULL, *Ip4Service = NULL, *Ip6Service = NULL, *HttpService = NULL;
    EFI_HANDLE Tcp4SocketHandle = NULL;
    EFI_TCP4_PROTOCOL *Tcp4 = NULL;
#if 0
    EFI_TCP4_CONFIG_DATA                Tcp4CfgData;
    EFI_TCP4_CONNECTION_TOKEN           Tcp4ConnToken;
    EFI_TCP4_TRANSMIT_DATA              Tcp4TxData;
    EFI_TCP4_RECEIVE_DATA               Tcp4RxData;
    EFI_TCP4_IO_TOKEN                   Tcp4IoToken;
    CHAR8                               RequestData[] = "GET / HTTP/1.1\nHost:localhost\nAccept:* / * \nConnection:Keep-Alive\n\n";
    CHAR8                               *RecvBuffer = NULL;
    UINTN                               EventIndex;
#endif

    AsciiFPrint(DEBUG_FILE_HANDLE, "Locating Ip4 service binding protocol");
    Status = gBS->LocateProtocol(&gEfiIp4ServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&Ip4Service);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Ip4 service binding protocol found");
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate Ip4 service binding protocol: %r", Status);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Locating Ip6 service binding protocol");
    Status = gBS->LocateProtocol(&gEfiIp6ServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&Ip6Service);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Ip6 service binding protocol found");
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate Ip6 service binding protocol: %r", Status);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Locating Udp4 service binding protocol");
    Status = gBS->LocateProtocol(&gEfiUdp4ServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&Udp4Service);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Udp4 service binding protocol found");
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate Udp4 service binding protocol: %r", Status);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Locating Udp6 service binding protocol");
    Status = gBS->LocateProtocol(&gEfiUdp6ServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&Udp6Service);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Udp6 service binding protocol found");
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate Udp6 service binding protocol: %r", Status);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Locating Tcp4 service binding protocol");
    Status = gBS->LocateProtocol(&gEfiTcp4ServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&Tcp4Service);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Tcp4 service binding protocol found");
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate Tcp4 service binding protocol: %r", Status);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Locating Tcp6 service binding protocol");
    Status = gBS->LocateProtocol(&gEfiTcp6ServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&Tcp6Service);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Tcp6 service binding protocol found");
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate Tcp6 service binding protocol: %r", Status);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Locating Http service binding protocol");
    Status = gBS->LocateProtocol(&gEfiHttpServiceBindingProtocolGuid,
                                 NULL,
                                 (VOID **)&HttpService);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Http service binding protocol found");
    else
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate Http service binding protocol: %r", Status);

    if (Tcp4Service == NULL)
        goto Error;

    AsciiFPrint(DEBUG_FILE_HANDLE, "Creating Tcp4 socket handle");
    Status = Tcp4Service->CreateChild(Tcp4Service, &Tcp4SocketHandle);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Tcp4 socket handle created");
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not create Tcp4 socket handle: %r", Status);
        goto Error;
    }

    AsciiFPrint(DEBUG_FILE_HANDLE, "Opening Tcp4 protocol");
    Status = gBS->OpenProtocol(Tcp4SocketHandle,
                               &gEfiTcp4ProtocolGuid,
                               (VOID **)&Tcp4,
                               gImageHandle,
                               Tcp4SocketHandle,
                               EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (Status == EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "Tcp4 protocol opened");
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not open Tcp4 protocol: %r", Status);
        goto Error;
    }

#if 0
    AsciiFPrint(DEBUG_FILE_HANDLE, "Getting Tcp4 Mode data");
    Status = Tcp4->GetModeData(Tcp4, NULL, &Tcp4CfgData, NULL, NULL, NULL);
    if (EFI_ERROR(Status)) {
        AsciiFPrint(DEBUG_FILE_HANDLE, "GetModeData failed: %r", Status);
        return FALSE;
    }

    CopyMem(Tcp4CfgData.AccessPoint.RemoteAddress.Addr, ServerIP.v4.Addr, sizeof(Tcp4CfgData.AccessPoint.RemoteAddress.Addr));
#endif
#if 0
    SetMem(&Tcp4CfgData, sizeof(Tcp4CfgData), 0);
    Tcp4CfgData.AccessPoint.RemotePort = 80;
    *(UINT32*)(Tcp4CfgData.AccessPoint.SubnetMask.Addr) = (255 | 255 << 8 | 255 << 16 | 0 << 24);
    Tcp4CfgData.AccessPoint.UseDefaultAddress = TRUE;
    //*(UINT32*)(m_pTcp4ConfigData->AccessPoint.StationAddress.Addr) = LocalIp;
    Tcp4CfgData.AccessPoint.StationPort = 61558;
    Tcp4CfgData.AccessPoint.ActiveFlag = TRUE;
    Tcp4CfgData.ControlOption = NULL;

    AsciiFPrint(DEBUG_FILE_HANDLE, "Configuring TCP connection");
    Status = Tcp4->Configure(Tcp4, &Tcp4CfgData);
    if (EFI_ERROR(Status)) {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Configure failed: %r", Status);
        return FALSE;
    }

    gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &Tcp4ConnToken.CompletionToken.Event);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Initiating TCP connection");
    Status = Tcp4->Connect(Tcp4, &Tcp4ConnToken);
    if (EFI_ERROR(Status)) {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Connect failed: %r", Status);
        return FALSE;
    }

    Status = gBS->WaitForEvent(1, &Tcp4ConnToken.CompletionToken.Event, &EventIndex);
    if (EFI_ERROR(Status)) {
        AsciiFPrint(DEBUG_FILE_HANDLE, "WaitForEvent failed: %r", Status);
        return FALSE;
    }

    Tcp4TxData.Push = TRUE;
    Tcp4TxData.Urgent = TRUE;
    Tcp4TxData.DataLength = (UINT32)AsciiStrLen(RequestData) + 2;
    Tcp4TxData.FragmentCount = 1;
    Tcp4TxData.FragmentTable[0].FragmentLength = Tcp4TxData.DataLength;
    Tcp4TxData.FragmentTable[0].FragmentBuffer = RequestData;
    Tcp4IoToken.Packet.TxData = &Tcp4TxData;

    gBS->CreateEvent(0, TPL_NOTIFY, NULL, NULL, &Tcp4IoToken.CompletionToken.Event);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Sending TCP data");
    Status = Tcp4->Transmit(Tcp4, &Tcp4IoToken);

    if (EFI_ERROR(Status)) {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Transmit failed: %r", Status);
        return FALSE;
    }

    Status = gBS->WaitForEvent(1, &Tcp4IoToken.CompletionToken.Event, &EventIndex);
    if (EFI_ERROR(Status)) {
        AsciiFPrint(DEBUG_FILE_HANDLE, "WaitForEvent failed: %r", Status);
        return FALSE;
    }

    RecvBuffer = AllocatePool(1024);
    Tcp4RxData.UrgentFlag = TRUE;
    Tcp4RxData.DataLength = 1024;
    Tcp4RxData.FragmentCount = 1;
    Tcp4RxData.FragmentTable[0].FragmentLength = Tcp4RxData.DataLength;
    Tcp4RxData.FragmentTable[0].FragmentBuffer = (void*)RecvBuffer;
    Tcp4IoToken.Packet.RxData = &Tcp4RxData;

    AsciiFPrint(DEBUG_FILE_HANDLE, "Receiving TCP data");
    Status = Tcp4->Receive(Tcp4, &Tcp4IoToken);
    if (EFI_ERROR(Status))
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Receive failed: %r", Status);
        FreePool(RecvBuffer);
        return FALSE;
    }

    Status = gBS->WaitForEvent(1, &Token.CompletionToken.Event, &EventIndex);
    if (EFI_ERROR(Status)) {
        AsciiFPrint(DEBUG_FILE_HANDLE, "WaitForEvent failed: %r", Status);
        FreePool(RecvBuffer);
        return FALSE;
    }
#endif

Error:
    if (Tcp4)
    {
        Status = gBS->CloseProtocol(Tcp4SocketHandle,
                                    &gEfiTcp4ProtocolGuid,
                                    gImageHandle,
                                    Tcp4SocketHandle);
        if (EFI_ERROR(Status))
            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to close Tcp4 protocol: %r", Status);
    }
    if (Tcp4SocketHandle)
    {
        Status = Tcp4Service->DestroyChild(Tcp4Service, Tcp4SocketHandle);
        if (EFI_ERROR(Status))
            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to destroy Tcp4 socket handle: %r", Status);
    }
#endif
    return TRUE;
}

EFI_FILE *DataEfiDir = NULL; // Directory path of non-EFI system partition
INTN SaveFSHandle = -1;
EFI_FILE *SaveDir = NULL;
UINTN ScreenMode;                       // Screen mode (determines ScreenHeight and ScreenWidth)
UINTN ScreenHeight;                     // Screen Height
UINTN ScreenWidth;                      // Screen Width
UINTN WindowHeight = MIN_SCREEN_HEIGHT; // Window Height
UINTN WindowWidth = MIN_SCREEN_WIDTH;   // Window Width
UINTN ConHeight;                        // Console Height
UINTN ConWidth;                         // Console Width
UINTN MaxUIWidth;                       // Maximum width within the console

UINTN gTextHeight; // Text height
UINTN gCharWidth;  // half-width character height

UINTN gMainFrameLeft;   // x-coordinates of the main frame to the right of the sidebar
UINTN gMainFrameRight;  // x-coordinates of the main frame to the right of the sidebar
UINTN gMainFrameTop;    // y-coordinates of the main frame
UINTN gMainFrameBottom; // y-coordinates of the main frame

#define NUM_WELCOME_BUTTONS 2

// Splashscreen icon binary data
const unsigned char *WELCOME_BTN_BIN[] = {
    exit_png_bin,
    config_png_bin};

UINTN WELCOME_BTN_BIN_SIZE[] = {
    sizeof(exit_png_bin),
    sizeof(config_png_bin)};

#define NUM_EXIT_BUTTONS 3

const unsigned char *EXIT_BTN_BIN[] = {
    restart_png_bin,
    shutdown_png_bin,
    exit_app_png_bin};
UINTN EXIT_BTN_BIN_SIZE[] = {
    sizeof(restart_png_bin),
    sizeof(shutdown_png_bin),
    sizeof(exit_app_png_bin)};

// Sidebar icon binary data
const unsigned char *SIDEBAR_BTN_BIN[] = {
    sys_info_png_bin,
    test_sel_png_bin,
    mem_addr_png_bin,
    cpu_sel_png_bin,
    runtest_png_bin,
    bench_png_bin,
    settings_png_bin,
#ifndef PRO_RELEASE
    upgrade_png_bin,
#endif
    exit_png_bin};
UINTN SIDEBAR_BTN_BIN_SIZE[] = {
    sizeof(sys_info_png_bin),
    sizeof(test_sel_png_bin),
    sizeof(mem_addr_png_bin),
    sizeof(cpu_sel_png_bin),
    sizeof(runtest_png_bin),
    sizeof(bench_png_bin),
    sizeof(settings_png_bin),
#ifndef PRO_RELEASE
    sizeof(upgrade_png_bin),
#endif
    sizeof(exit_png_bin)};

// Splashscreen icon text label
const EFI_STRING_ID WELCOME_BTN_CAPTION_STR_ID[NUM_WELCOME_BUTTONS] = {
    STRING_TOKEN(STR_EXIT),
    STRING_TOKEN(STR_CONFIG)};

// Splashscreen icon text label
const EFI_STRING_ID EXIT_BTN_CAPTION_STR_ID[] = {
    STRING_TOKEN(STR_RESTART),
    STRING_TOKEN(STR_SHUTDOWN),
    STRING_TOKEN(STR_EXIT_APP)};

// Sidebar icon text label
const EFI_STRING_ID SIDEBAR_BTN_CAPTION_STR_ID[] = {
    STRING_TOKEN(STR_SIDEBAR_SYSINFO),
    STRING_TOKEN(STR_SIDEBAR_TESTSEL),
    STRING_TOKEN(STR_SIDEBAR_MEMRANGE),
    STRING_TOKEN(STR_SIDEBAR_CPUSEL),
    STRING_TOKEN(STR_SIDEBAR_START),
    STRING_TOKEN(STR_SIDEBAR_BENCHMARK),
    STRING_TOKEN(STR_SIDEBAR_SETTINGS),
#ifndef PRO_RELEASE
    STRING_TOKEN(STR_SIDEBAR_UPGRADE),
#endif
    STRING_TOKEN(STR_SIDEBAR_EXIT)};

typedef struct
{
    UINT16 Id;
    EG_IMAGE *Image;
    EFI_STRING_ID Caption;
    EG_RECT ScreenPos;
} BUTTON;

// Splashscreen images
EG_IMAGE *HeaderLogo = NULL;
EG_IMAGE *SelectionOverlay;

// SysInfo screen images
EG_IMAGE *SysInfoCPU = NULL;
EG_IMAGE *SysInfoMem = NULL;

// SPD screen images
EG_IMAGE *DimmIcon = NULL;
EG_IMAGE *NoDimmIcon = NULL;

// Upgrade Pro screen images
EG_IMAGE *OKBullet = NULL;
EG_IMAGE *QRCode = NULL;

BUTTON SidebarButtons[NUM_SIDEBAR_BUTTONS]; // Sidebar buttons

// Debug variables
#ifdef SHELLAPP // not used
BOOLEAN gDebugMode = FALSE;
#else
BOOLEAN gDebugMode = TRUE;
#endif
UINTN gVerbosity = 0;

char gBuffer[BUF_SIZE];
CHAR16 g_wszBuffer[BUF_SIZE]; // 这个似乎是专门用来debug的

/* HW details */
struct cpu_ident cpu_id; // results of execution cpuid instruction
// int l1_cache=0, l2_cache=0, l3_cache=0;
CPUINFO gCPUInfo; // CPU details
UINT64 memsize;   // total physical memory in bytes
UINT64 minaddr;   // min system address
UINT64 maxaddr;   // max system address

UINTN clks_msec; // CPU speed in KHz or # clock cycles / msec

UINTN cpu_speed;   // CPU speed in KHz
UINTN mem_speed;   // RAM speed in MB/s
UINTN mem_latency; // in ps
UINTN l1_speed;    // L1 cache speed in MB/s
UINTN l2_speed;    // L2 cache speed in MB/s
UINTN l3_speed;    // L3 cache speed in MB/s

CPUMSRINFO cpu_msr;       // CPU MSR info
BOOLEAN gSIMDSupported;   // SIMD instructions supported
BOOLEAN gNTLoadSupported; // Non-temporal SIMD load supported

extern void cpu_type(CHAR16 *szwCPUType, UINTN bufSize);

int g_numMemModules;    // Num of memory SPD modules detected
int g_maxMemModules;    // size of g_MemoryInfo structure
SPDINFO *g_MemoryInfo;  // SPD details obtained via SMBUS
int g_numSMBIOSMem;     // Num of SMBIOS memory devices
int g_numSMBIOSModules; // Num of SMBIOS memory devices occupied

PCIINFO *g_TSODController;
int g_numTSODCtrl;
int g_numTSODModules;
int g_maxTSODModules; // size of g_MemoryInfo structure
TSODINFO *g_MemTSODInfo;

SMBIOS_MEMORY_DEVICE *g_SMBIOSMemDevices;    // RAM details obtained from SMBIOS
SMBIOS_SYSTEM_INFORMATION gSystemInfo;       // SMBIOS systeminformation
SMBIOS_BIOS_INFORMATION gBIOSInfo;           // SMBIOS BIOSinformation
SMBIOS_BASEBOARD_INFORMATION gBaseBoardInfo; // SMBIOS baseboardinformation
BOOL gConCtrlWorkaround = TRUE;              // if TRUE, use the obsolete Console Control protocol required by certain firmware (Apple, Lenovo)
BOOL gFixedScreenRes = FALSE;                // if TRUE, requires the screen resolution to be fixed
BOOL gRunFromBIOS = FALSE;                   // if TRUE, MemTest86 is running from BIOS (ie. no file write support)
BOOL gTest12SingleCPU = FALSE;               // if TRUE, run Test 12 in SINGLE CPU mode.
BOOL gDisableFontInstall = FALSE;            // if TRUE, disable font language support
BOOL gDisableCPUInfo = FALSE;                // if TRUE, disable CPU info collection
BOOL gTextDirectToScreen = FALSE;
#if 0
static const FIRMWAREINFO BASEBOARD_PRODUCT_BLACKLIST[] =
{
    { "Mac-F42C88C8", FALSE, 0, RESTRICT_FLAG_STARTUP },
    { "80AF", FALSE, 0, RESTRICT_FLAG_MP },
    { "Z97MX-Gaming 5", FALSE, 0, RESTRICT_FLAG_MP },
    { "Z170MX-Gaming 5", FALSE, 0, RESTRICT_FLAG_MP },
    { "Z170X-Gaming 3", FALSE, 0, RESTRICT_FLAG_MP },
    { "Z170X-Gaming 7", FALSE, 0, RESTRICT_FLAG_MP },
    { "Z170X-Gaming GT", FALSE, 0, RESTRICT_FLAG_MP },
    { "Z170X-UD3-CF", FALSE, 0, RESTRICT_FLAG_MP },
    { "Z170-HD3P", FALSE, 0, RESTRICT_FLAG_MP },
    //  { "Z170", TRUE, 0, RESTRICT_FLAG_MP },
        { "990FXA-UD3", FALSE, 0, RESTRICT_FLAG_MP },
        { "970A-DS3P", FALSE, 0, RESTRICT_FLAG_MP },
        { "M5A97 R2.0", FALSE, 0, RESTRICT_FLAG_MP },
        { "M5A97 EVO R2.0", FALSE, 0, RESTRICT_FLAG_MP },
        { "M5A99FX PRO R2.0", FALSE, 0, RESTRICT_FLAG_MP },
        { "X99-A", FALSE, 0, RESTRICT_FLAG_MP },
        { "X99-A/USB 3.1", FALSE, 0, RESTRICT_FLAG_MP },
        { "X99-A II", FALSE, 0, RESTRICT_FLAG_MP },
        { "X99-A", TRUE, 0, RESTRICT_FLAG_MP },
        { "Sabertooth X99", FALSE, 0, RESTRICT_FLAG_MP },
        { "STRIX X99 GAMING", FALSE, 0, RESTRICT_FLAG_MP },
        { "X99-DELUXE", FALSE, 0, RESTRICT_FLAG_MP },
        { "X99-DELUXE II", FALSE, 0, RESTRICT_FLAG_MP },
        { "RAMPAGE V EXTREME", FALSE, 0, RESTRICT_FLAG_MP },
        { "MAXIMUS VIII RANGER", FALSE, 0, RESTRICT_FLAG_MP },
        { "Z10PE-D8 WS", FALSE, 0, RESTRICT_FLAG_MP },
        { "X9DRW", FALSE, 0, RESTRICT_FLAG_MP },
        { "X9DRW-3LN4F+/X9DRW-3TF+", FALSE, 0, RESTRICT_FLAG_MP },
        { "X9SRL-F", FALSE, 0, RESTRICT_FLAG_MP },
        { "151-BE-E097", FALSE, 0, RESTRICT_FLAG_MP },
        { "131-HE-E095-KR", FALSE, 0, RESTRICT_FLAG_MP },
        { "151-HE-E999-KR", FALSE, 0, RESTRICT_FLAG_MP },
        { "PRIME B350-PLUS", FALSE, 0, RESTRICT_FLAG_MP },
        { "PRIME X370-PRO", FALSE, 0, RESTRICT_FLAG_MPDISABLE },
        { "CROSSHAIR VI HERO", FALSE, 0, RESTRICT_FLAG_MPDISABLE },
        { NULL, FALSE, 0, 0 }
};
#endif

UINT32 gRestrictFlags; // Contains any restrictions to apply based on the baseboard blacklist

BOOLEAN gMPSupported; // if TRUE, multiprocessor services are supported
BOOLEAN gDisableMP;   // if TRUE, disable multiprocessor support. This may be used as a workaround for multiprocessor issues.
BOOLEAN gEnableHT;    // if TRUE, enable hyperthreading during testing. Enable hyperthreading generally slows down the testing

BOOLEAN gHwErrRecSupport; // if TRUE, EFI hardware error record is supported. Not used

EFI_TPL gTPL = TPL_APPLICATION;

/* HW details functions */

// init
//
// Gather hardware details
static void init(void);

// cpuspeed
//
// Measure CPU clock speed using the legacy PIT timer
static int cpuspeed(void);

static UINTN sHPETBase = 0;  // HPET timer base address in MMIO
static UINT64 sPeriodfs = 0; // HPET timer period in fs

// hpet_init
//
// Initialize the HPET timer, if available
static int hpet_init(void);

// cpuspeed_hpet
//
// Measure CPU clock speed using the HPET timer
static int cpuspeed_hpet(void);

// memspeed
//
// Measure memory bandwidth
static int memspeed(UINTN src, UINT32 len, int iter);

extern void tests_memASM_latency(void *memory, UINTN loops);
typedef enum
{
    MEM_LATENCY_LINEAR,
    MEM_LATENCY_RANDOM,
    MEM_LATENCY_RANDOMRANGE
} MEM_LATENCY_MODE;

// tests_memASM_latency
//
// Measure memory latency
static int test_mem_latency(int TestDuration, MEM_LATENCY_MODE mode);

// get_cache_size
//
// Get the cache size
static void get_cache_size();

// cpu_cache_speed
//
// Measure the cache speed
static int cpu_cache_speed();

// get_mem_speed
//
// Measure the cache and memory speed
static int get_mem_speed();

// mem_size
//
// Traverse the memory map and get the memory size
static void mem_size(void);

// mem_spd_info
//
// Obtain the RAM SPD info from SPD
static void mem_spd_info(void);

// mem_temp_info
//
// Obtain the RAM temperature info
static void mem_temp_info(void);

// smbios_mem_info
//
// Obtain the SMBIOS memory info
static void smbios_mem_info(void);

// map_spd_to_smbios
//
// Map the SPD data to the SMBIOS index
static void map_spd_to_smbios(void);

extern void getCPUSpeedLine(int khzspeed, int khzspeedturbo, CHAR16 *szwCPUSpeed, UINTN bufSize);

BOOLEAN EnableSSE();

#ifdef SHELLAPP // not used
STATIC
EFI_STATUS
EFIAPI
ShellFindSE2(
    IN EFI_HANDLE ImageHandle);

EFI_STATUS
EFIAPI
LoadDriver(
    IN CHAR16 *FileName);

EFI_STATUS
EFIAPI
ConnectAllEfi(
    VOID);
#endif

// Path/Device functions
typedef struct
{
    CHAR16 *Str;
    UINTN Len;
    UINTN Maxlen;
} POOL_PRINT;

STATIC CHAR16 *FindPath(IN CHAR16 *FullPath);
STATIC VOID CleanUpPathNameSlashes(IN OUT CHAR16 *PathName);
STATIC CHAR16 *SplitDeviceString(IN OUT CHAR16 *InString);

// Welcome screen functions
EFI_GRAPHICS_OUTPUT_BLT_PIXEL DEFAULT_BG_PIXEL = {0, 0, 0, 0};          // default bg colour
EFI_GRAPHICS_OUTPUT_BLT_PIXEL DEFAULT_SEL_PIXEL = {127, 127, 127, 255}; // default bg colour for selected items
EFI_GRAPHICS_OUTPUT_BLT_PIXEL DEFAULT_TEXT_PIXEL = {255, 255, 255, 0};  // default text colour
STATIC EG_PIXEL LINE_COLOUR_PIXEL = {0x2b, 0x80, 0xd4, 0};              // sidebar line colour
STATIC EG_PIXEL PROGBAR_COLOUR_PIXEL = {40, 211, 0, 0};                 // progress bar colour

// BltImageCompositeBadge
//
// Overlay an image on top of another for highlighting
STATIC VOID BltImageCompositeBadge(IN EG_IMAGE *BaseImage, IN EG_IMAGE *TopImage, IN EG_IMAGE *BadgeImage, IN UINTN XPos, IN UINTN YPos);

STATIC VOID FreeStringTable(CHAR16 **StringTable, int NumOpts);

STATIC CHAR16 **AllocateStringTable(const EFI_STRING_ID StringIdTable[], int NumOpts);

// getval
//
// Prompt the user to enter a value
STATIC int getval(UINTN x, UINTN y, int len, int decimalonly, UINT64 *val);

// getoption
//
// Prompt the user to select an option
STATIC int getoption(UINTN x, UINTN y, CHAR16 *OptionText[], int numoptions);

STATIC EFI_STATUS WaitForKeyboardMouseInput();

// MouseMoveSideBar
//
// Process any mouse movement in the sidebar
STATIC int MouseMoveSideBar();

// MouseClickSideBar
//
// Process any mouse clicks in the sidebar
STATIC int MouseClickSideBar();

// KeyPressSideBar
//
// Process any keyboard shortcuts for the sidebar
STATIC int KeyPressSideBar(EFI_INPUT_KEY *Key);

// ShowWelcomeScreen
//
// Display the splash screen
STATIC UINT16 ShowWelcomeScreen();

// ExitScreen
//
// Display the exit screen
STATIC UINTN ExitScreen();

// DrawHeader
//
// Draw the MemTest86 logo header in the main menu
STATIC VOID DrawHeader();

// DrawSideBar
//
// Draw the sidebar in the main menu
STATIC VOID DrawSideBar();

// DrawFooter
//
// Draw the footer in the main menu
STATIC VOID DrawFooter();

// DrawWindow
//
// Draw the header/footer/sidebar in the main menu
STATIC VOID DrawWindow();

// MainMenu
//
// Main loop to handle user actions in the main menu
STATIC UINT16 MainMenu();

// TestSelectScreen
//
// Handle user actions in the test selection screen
STATIC UINT16 TestSelectScreen(IN UINTN XPos, IN UINTN YPos);

// AddressRangeScreen
//
// Handle user actions in the address range screen
STATIC UINT16 AddressRangeScreen(IN UINTN XPos, IN UINTN YPos);

// CPUSelectScreen
//
// Handle user actions in the CPU selection screen
STATIC UINT16 CPUSelectScreen(IN UINTN XPos, IN UINTN YPos);

// SysInfoScreen
//
// Handle user actions in the system information screen
STATIC UINT16 SysInfoScreen(IN UINTN XPos, IN UINTN YPos);

// SPDInfoScreen
//
// Handle user actions in the SPD info screen
STATIC UINT16 SPDInfoScreen(IN UINTN XPos, IN UINTN YPos);

#ifdef PRO_RELEASE
typedef enum
{
    DIMM_FRONT_SIDE,
    DIMM_BACK_SIDE,
    DIMM_BOTH_SIDES
} DIMM_SIDE;

STATIC VOID DrawDIMMResults(UINTN LeftOffset, IN UINTN TopOffset, int StartIdx, int EndIdx, DIMM_SIDE DimmSide, EG_IMAGE **pImage);

// DIMMResultsScreen
//
// Handle user actions in the DIMM results screen
STATIC VOID DIMMResultsScreen(IN UINTN XPos, IN UINTN YPos);
#endif

// RAMBenchmarkScreen
//
// Handle user actions in the RAM benchmark screen
STATIC UINT16 RAMBenchmarkScreen(IN UINTN XPos, IN UINTN YPos);

// BenchmarkResultsScreen
//
// Handle user actions in the RAM benchmark results screen
STATIC UINT16 BenchmarkResultsScreen(UINTN x, UINTN y);

// PrevBenchmarkResultsScreen
//
// Handle user actions in the previous benchmark results screen
STATIC UINT16 PrevBenchmarkResultsScreen(IN UINTN XPos, IN UINTN YPos);

// GeneralSettingsScreen
//
// Handle user actions in the Settings screen
STATIC UINT16 GeneralSettingsScreen(IN UINTN XPos, IN UINTN YPos);

#ifndef PRO_RELEASE
// UpgradeProScreen
//
// Handle user actions in the Upgrade to Pro screen
STATIC UINT16 UpgradeProScreen(IN UINTN XPos, IN UINTN YPos);
#endif

// AutoSetScreenRes
//
// Automatically set to the most suitable screen resolution
STATIC VOID AutoSetScreenRes();

// SetScreenRes
//
// Set the screen resolution to the specified mode
STATIC VOID SetScreenRes(UINTN Mode);

// EnableGraphicsMode
//
// Enable/disable graphics mode (for EFI firmware using Console Control)
STATIC VOID EnableGraphicsMode(BOOLEAN Enable);

// Sidebar icon text label
const EFI_STRING_ID CACHE_MODE_STR_ID[] = {
    STRING_TOKEN(STR_MENU_DISABLED),
    STRING_TOKEN(STR_MENU_ENABLED),
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    STRING_TOKEN(STR_CACHE_WRITE_COMBINING),
    STRING_TOKEN(STR_CACHE_WRITE_THROUGH),
    STRING_TOKEN(STR_CACHE_WRITE_PROTECT),
    STRING_TOKEN(STR_CACHE_WRITE_BACK)
#endif
};

#define NUM_TEST_SEL_OPTIONS 1

// Test selection screen options
STATIC CHAR16 sTestSelOptText[][32] = {
    L"Number of passes: 4"};

#define NUM_ADDR_RANGE_OPTIONS 3

// Address range screen options
STATIC CHAR16 sAddrRangeText[][64] = {
    L"Lower Limit:                 ",
    L"Upper Limit:                 ",
    L"Reset limits to default      ",
};

UINT64 gAddrLimitLo;     // Lower address limit
UINT64 gAddrLimitHi;     // Upper address limit
UINT64 gMemRemMB;        // Amount of memory (in MB) to leave unallocated
UINT64 gMinMemRangeMB;   // Minimum size of memory range (in MB) to allocate
UINT64 gAddr2ChBits;     // Bit mask of address bits to XOR to determine channel number
UINT64 gAddr2SlBits;     // Bit mask of address bits to XOR to determine slot number
UINT64 gAddr2CSBits;     // Bit mask of address bits to XOR to determine CS number
CHIPMAPCFG *gChipMapCfg; // DRAM chip numbering map
int gNumChipMapCfg;      // Size of gChipMapCfg

#define NUM_CPU_SEL_MODE 4

// CPU selection screen options string ID
const EFI_STRING_ID CPU_SEL_OPTIONS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_CPUSEL_SINGLE),
    STRING_TOKEN(STR_MENU_CPUSEL_PARALLEL),
    STRING_TOKEN(STR_MENU_CPUSEL_RROBIN),
    STRING_TOKEN(STR_MENU_CPUSEL_SEQUENTIAL)};

// CPU selection screen options
STATIC CHAR16 sCPUSelText[][32] = {
    L"Single: CPU # 0       ",
    L"Parallel (All CPUs)   ",
    L"Round Robin (All CPUs)",
    L"Sequential (All CPUs) "};

UINTN gCPUSelMode = (UINTN)CPU_DEFAULT; // CPU selection mode
UINTN gSelCPUNo = 0;                    // Selected CPU number (Single CPU mode)
UINTN gMaxCPUs = DEF_CPUS;

#ifdef PRO_RELEASE
#define NUM_MEM_OPTIONS 5 // Number of options in System information screen
#else
#define NUM_MEM_OPTIONS 3 // Number of options in System information screen
#endif

// System information screen options
STATIC CHAR16 sMemOptText[NUM_MEM_OPTIONS][64] = {
    L"  View detailed RAM (SPD) info",
    L"  View memory usage           ",
    L"  Disable ECC Polling         ",
#ifdef PRO_RELEASE
    L"  Enable ECC Injection        ",
    L"  Disable memory caching      ",
#endif
};

UINTN gCacheMode = CACHEMODE_DEFAULT; // default cache mode
BOOLEAN gECCPoll = TRUE;              // poll for ECC errors?
BOOLEAN gECCInject = FALSE;           // inject ECC errors
BOOLEAN gTSODPoll = TRUE;             // enable/disable poll DIMM TSOD temperatures

#define DIMMS_PER_PAGE 8                         // Maximum number of DIMMS to show on screen
STATIC CHAR16 sRAMInfoText[DIMMS_PER_PAGE][128]; // RAM SPD selection
int gSPDPage = 0;                                // Current SPD page
int gSPDSelIdx = 0;

#ifdef PRO_RELEASE
#define NUM_SPD_OPTIONS 6
#else
#define NUM_SPD_OPTIONS 3
#endif

// SPD info screen options
STATIC CHAR16 sSPDOptText[NUM_SPD_OPTIONS][64] = {
    L"Previous Page",
    L"Next Page",
#ifdef PRO_RELEASE
    L"Minimum SPDs to detect: %d",
    L"Exact SPDs to detect: %s",
    L"Save RAM SPD/SMBIOS info to file",
#endif
    L"Go back"};

// List of all tests and corresponding parameters
TESTCFG gTestList[NUMTEST] = {
    {TRUE, 0, NULL, STRING_TOKEN(STR_TEST_0_NAME), RunAddressWalkingOnesTest, NULL, NULL, NULL, 1, 1, 4, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 1, NULL, STRING_TOKEN(STR_TEST_1_NAME), RunAddressOwnTest, NULL, NULL, NULL, 1, 1, 8, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 2, NULL, STRING_TOKEN(STR_TEST_2_NAME), RunAddressOwnTest, NULL, NULL, NULL, MAX_CPUS, 1, 8, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 3, NULL, STRING_TOKEN(STR_TEST_3_NAME), RunMovingInvTest, NULL, NULL, NULL, MAX_CPUS, 6, 4, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 4, NULL, STRING_TOKEN(STR_TEST_4_NAME), RunMovingInvTest, NULL, NULL, NULL, MAX_CPUS, 3, 4, PAT_WALK01, 0, CACHEMODE_DEFAULT, NULL},
#ifndef TRIAL
    {TRUE, 5, NULL, STRING_TOKEN(STR_TEST_5_NAME), RunMovingInvTest, NULL, NULL, NULL, MAX_CPUS, 30, 4, PAT_RAND, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 6, NULL, STRING_TOKEN(STR_TEST_6_NAME), RunBlockMoveTest, NULL, NULL, NULL, MAX_CPUS, 81, 64, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 7, NULL, STRING_TOKEN(STR_TEST_7_NAME), RunMovingInv32bitPatternTest, NULL, NULL, NULL, MAX_CPUS, 3, 256, PAT_DEF, 0x00000001, CACHEMODE_DEFAULT, NULL},
    {TRUE, 8, NULL, STRING_TOKEN(STR_TEST_8_NAME), RunRandomNumSequenceTest, NULL, NULL, NULL, MAX_CPUS, 24, 4, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 9, NULL, STRING_TOKEN(STR_TEST_9_NAME), RunModulo20Test, NULL, NULL, NULL, MAX_CPUS, 3, 4, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 10, NULL, STRING_TOKEN(STR_TEST_10_NAME), RunBitFadeTest_write, RunBitFadeTest_verify, NULL, NULL, 1, 1, 4, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
#ifdef PRO_RELEASE
    {TRUE, 11, NULL, STRING_TOKEN(STR_TEST_11_NAME), RunRandomNumSequence64Test, NULL, NULL, NULL, MAX_CPUS, 24, 8, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {TRUE, 12, NULL, STRING_TOKEN(STR_TEST_12_NAME), RunRandomNumSequence128Test, NULL, NULL, NULL, MAX_CPUS, 24, 16, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
#endif
    {TRUE, 13, NULL, STRING_TOKEN(STR_TEST_13_NAME), RunHammerTest_write, RunHammerTest_double_sided_hammer_fast, RunHammerTest_double_sided_hammer_slow, RunHammerTest_verify, MAX_CPUS, ROW_INC, 16, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
    {FALSE, 14, NULL, STRING_TOKEN(STR_TEST_14_NAME), RunDMATest, NULL, NULL, NULL, 1, 12, 512, PAT_DEF, 0, CACHEMODE_DEFAULT, NULL},
#endif
};

CHAR16 gTestCfgFile[SHORT_STRING_LEN]; // filename containing a list of custom test configurations
TESTCFG *gCustomTestList = NULL;       // contains list of custom test configurations, if specified
int gNumCustomTests = 0;               // Size of gCustomTestList;
BOOLEAN gUsingCustomTests = FALSE;     // if TRUE, using custom tests defined in test config file
UINTN gNumPasses = DEFAULT_NUM_PASSES; // number of passes of the test sequence to perform
BOOLEAN gPass1Full = FALSE;            // whether to run the full # of iterations for Pass 1

#define NUM_BENCHMARK_PARAMS 3

// RAM benchmark screen parameter string IDs
const EFI_STRING_ID BENCH_PARAMS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_BENCH_BLOCK_RW),
    STRING_TOKEN(STR_MENU_BENCH_TEST_MODE),
    STRING_TOKEN(STR_MENU_BENCH_ACCESS_WIDTH)};

#define NUM_BENCH_CMDS 2

// RAM benchmark screen options
const EFI_STRING_ID BENCH_CMD_STR_ID[] = {
    STRING_TOKEN(STR_MENU_BENCH_START),
    STRING_TOKEN(STR_MENU_BENCH_PREV_RESULTS)};

#define NUM_BENCH_OPTIONS (NUM_BENCHMARK_PARAMS + NUM_BENCH_CMDS)

// RAM benchmark screen read/write parameter options
const EFI_STRING_ID BENCH_RW_OPTIONS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_BENCH_READ),
    STRING_TOKEN(STR_MENU_BENCH_WRITE),
    (EFI_STRING_ID)-1};

// RAM benchmark screen test mode parameter options
const EFI_STRING_ID BENCH_TEST_MODE_OPTIONS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_BENCH_TEST_MODE_STEP),
    STRING_TOKEN(STR_MENU_BENCH_TEST_MODE_BLOCK),
    (EFI_STRING_ID)-1};

// RAM benchmark screen data width parameter options
const EFI_STRING_ID BENCH_DATA_WIDTH_OPTIONS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_BENCH_DATA_WIDTH_8BIT),
    STRING_TOKEN(STR_MENU_BENCH_DATA_WIDTH_16BIT),
    STRING_TOKEN(STR_MENU_BENCH_DATA_WIDTH_32BIT),
    STRING_TOKEN(STR_MENU_BENCH_DATA_WIDTH_64BIT),
    (EFI_STRING_ID)-1};

const EFI_STRING_ID BENCH_DISABLED_OPTIONS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_BENCH_NA),
    (EFI_STRING_ID)-1};

// Maps each parameter to a list of suboptions
STATIC const EFI_STRING_ID *sBenchSubOptions[] = {
    BENCH_RW_OPTIONS_STR_ID,
    BENCH_TEST_MODE_OPTIONS_STR_ID,
    BENCH_DISABLED_OPTIONS_STR_ID,
};

// Keeps track of what option is selected for each parameter
STATIC UINTN sBenchSubOptionsSel[] = {
    0,
    0,
    0};

STATIC CONST BOOLEAN READ_WRITE_MAP[] = {MEM_READ, MEM_WRITE};
STATIC CONST int TEST_MODE_MAP[] = {MEM_STEP, MEM_BLOCK};
STATIC CONST int DATA_WIDTH_MAP[] = {BYTE_BITS, WORD_BITS, DWORD_BITS, QWORD_BITS};
typedef struct
{
    int ReadWrite;
    int TestMode;
    int DataWidth;
    EFI_TIME StartTime;
} BENCH_CONFIG;
STATIC BENCH_CONFIG gBenchConfig = {MEM_READ, MEM_STEP, BYTE_BITS}; // Current RAM benchmark configuration

#ifdef PRO_RELEASE
#define NUM_BENCHRESULTS_OPTIONS 3
#else
#define NUM_BENCHRESULTS_OPTIONS 1
#endif

#define MAX_BENCHRESULTS_SERIES 8

// RAM benchmark results screen options
const EFI_STRING_ID BENCH_RESULTS_OPTIONS_STR_ID[] = {
#ifdef PRO_RELEASE
    STRING_TOKEN(STR_MENU_BENCH_SAVE_CHART),
    STRING_TOKEN(STR_MENU_BENCH_SAVE_REPORT),
#endif
    STRING_TOKEN(STR_MENU_GO_BACK)};

#define MAX_PREVBENCH_RESULTS 16

// RAM previous results
STATIC CHAR16 sPrevAdvResultsText[MAX_PREVBENCH_RESULTS][128];

#define NUM_PREVBENCH_OPTIONS 2

// RAM previous results screen options
const EFI_STRING_ID PREV_BENCH_OPTIONS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_BENCH_GRAPH_RESULTS),
    STRING_TOKEN(STR_MENU_GO_BACK)};

enum
{
    SETTINGS_OPTION_IDX_LANG,
    SETTINGS_OPTION_IDX_RES,
#ifdef PRO_RELEASE
    SETTINGS_OPTION_IDX_SAVECFG,
    SETTINGS_OPTION_IDX_SAVELOC,
#endif
    NUM_SETTINGS
};

// Settings screen options
const EFI_STRING_ID SETTINGS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_SETTINGS_LANG),
    STRING_TOKEN(STR_MENU_SETTINGS_RESOLUTION),
#ifdef PRO_RELEASE
    STRING_TOKEN(STR_MENU_SETTINGS_SAVECFG),
    STRING_TOKEN(STR_MENU_SETTINGS_SAVELOC)
#endif
};

// Settings screen language options
const EFI_STRING_ID SETTINGS_LANG_OPTIONS_STR_ID[] = {
    STRING_TOKEN(STR_MENU_ENGLISH),
    STRING_TOKEN(STR_MENU_FRENCH),
    STRING_TOKEN(STR_MENU_ITALIAN),
    STRING_TOKEN(STR_MENU_SPANISH),
    STRING_TOKEN(STR_MENU_PORTUGESE),
    STRING_TOKEN(STR_MENU_CATALAN),
    STRING_TOKEN(STR_MENU_GERMAN),
    STRING_TOKEN(STR_MENU_CZECH),
    STRING_TOKEN(STR_MENU_POLISH),
    STRING_TOKEN(STR_MENU_RUSSIAN),
    STRING_TOKEN(STR_MENU_JAPANESE),
    STRING_TOKEN(STR_MENU_CHINESE_SIMPLIFIED),
    STRING_TOKEN(STR_MENU_CHINESE_TRADITIONAL),
    (EFI_STRING_ID)-1};

// RunAdvancedMemoryTest
//
// Run the RAM benchmark test using the given parameters, and update the UI accordingly
BOOL RunAdvancedMemoryTest(IN UINTN XPos, IN UINTN YPos, BENCH_CONFIG *Config);

// SetupBenchLineChart
//
// Initialize the line chart given the test mode
VOID SetupBenchLineChart(int iTestMode);

// LoadCurBenchResultsToChart
//
// Plot the results of the current RAM benchmark test to the line chart
void LoadCurBenchResultsToChart(int iSeries, BOOLEAN bAve);

// LoadBenchResultsToChart
//
// Plot the results of the RAM benchmark test from file to the line chart
BOOLEAN LoadBenchResultsToChart(MT_HANDLE FileHandle, int iSeries, BOOLEAN bAve);

// SaveBenchReport
//
// Save the RAM benchmark results to file
VOID EFIAPI SaveBenchReport(MT_HANDLE FileHandle, CHAR16 *ChartFilename);

// InstallSelectedTests
//
// Install all selected memory tests
VOID InstallSelectedTests();

// InstallRowHammerTests
//
// (Not used) For row hammer testing
VOID InstallRowHammerTests();

// static EFI_GUID mFontPackageGuid =
//     {0x78941450, 0x90ab, 0x4fb1, {0xb7, 0x5f, 0x58, 0x92, 0x14, 0xe2, 0x4a, 0xc}};

static EFI_GUID mLocalizationPackageGuid =
    {0xc95520c1, 0x3c5d, 0x4077, {0xaf, 0xe9, 0x1, 0x88, 0x98, 0x1f, 0x30, 0xb}};

// Supported language list
const CHAR8 *mSupportedLangList[] = {
    "en-US",
    "fr-FR",
    "it-IT",
    "es-AR",
    "pt-BR",
    "ca-ES",
    "de-DE",
    "cs-CZ",
    "pl-PL",
    "ru-RU",
    "ja-JP",
    "zh-CN",
    "zh-HK",
    NULL};

// Whether each language is supportd by the font's charset
BOOLEAN mLangFontSupported[] = {
    TRUE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE,
    FALSE};

int gCurLang = 0; // Currently selected language

static EFI_HII_HANDLE gStringPackHandle = NULL; // String Package for storing localization strings

static EFI_HII_STRING_PROTOCOL *gHiiString = NULL; // Hii String protcol to retrieve strings from string packs

#define MAX_STRING_OFFSETS 1000

// For when Hii String is not available, need to parse the string package manually
UINT32 **gStringPkgOff = NULL; // Offsets for each string
UINT16 gNumStringPkg = 0;      // Number of string packages

// header of font file to install for language support
typedef struct
{
    CHAR8 Signature[4];
    UINT16 MajorRev;
    UINT16 MinorRev;
    UINT16 NumberOfNarrowGlyphs;
    UINT16 NumberOfWideGlyphs;
    UINT8 Hash[32];
} FontHeader;

#define FONT_SIGNATURE "PASS"
#define FONT_MAJOR_REV 1
#define FONT_MINOR_REV 0

// InitFont
//
// Install the specified font file
EFI_STATUS
InitFont(
    CHAR16 *FontFile);

// InitHexFont
//
// Install the specified .hex font file
EFI_STATUS
InitHexFont(
    CHAR16 *FontFile);

// CheckFontLanguageSupport
//
// Check whether the language is supported by the font
BOOL CheckFontLanguageSupport();

// InitLocalizationStrings
//
// Setup the localization strings
VOID
    InitLocalizationStrings(
        VOID);

// CleanupLocalizationStrings
//
// Cleanup the localization strings
VOID
    CleanupLocalizationStrings(
        VOID);

CHAR16 gConfigFile[SHORT_STRING_LEN]; // filename of the current configuration file

UINTN gReportNumErr = DEFAULT_REPORT_NUM_ERRORS;    // Number of the most recent errors to include in the report
UINTN gReportNumWarn = DEFAULT_REPORT_NUM_WARNINGS; // Number of the most recent warrnings to include in the report
BOOLEAN gDisableSlowHammering = FALSE;              // For debugging. Set to TRUE to only hammer at max rate.
BOOLEAN gHammerRandom = TRUE;                       // Whether to use a random pattern or a set pattern
UINT32 gHammerPattern = 0;                          // If gHammerandom == FALSE, use this set pattern
UINTN gHammerMode = HAMMER_DOUBLE;                  // Whether to use single-side or double-sided hammering
UINT64 gHammerStep = ROW_INC;                       // The step size to use when hammering addresses
UINTN gAutoMode = AUTOMODE_OFF;                     // run the tests, save the log and reboot
BOOLEAN gAutoReport = TRUE;                         // if TRUE, save report automatically (default)
UINTN gAutoReportFmt = AUTOREPORTFMT_HTML;          // The report format when saving automatically
BOOLEAN gAutoPromptFail = FALSE;                    // if TRUE, display FAIL message box on errors even if gAutoReport==AUTOMODE_ON
CHAR16 gTCPServerIP[SHORT_STRING_LEN];              // The address for the remote management console server
UINT32 gTCPServerPort = 80;
CHAR16 gTCPRequestLocation[LONG_STRING_LEN]; // The route location for console message handler. Default /mgtconsolemsghandler.php
CHAR16 gTCPClientIP[SHORT_STRING_LEN];
CHAR16 gTCPGatewayIP[SHORT_STRING_LEN]; // Ip address of the default route for the current network
BOOLEAN gSkipSplash = FALSE;            // if TRUE, the 10 second splashscreen is skipped
UINTN gSkipDecode = SKIPDECODE_NEVER;   // Policy for whether to skip the decode results screen. See SKIPDECODE_*
UINTN gExitMode = EXITMODE_PROMPT;      // 0 - reboot on exit; 1 - shutdown on exit; 2 - exit MemTest86 and return control to UEFI BIOS
BOOLEAN gDisableSPD = FALSE;            // if TRUE, disable SPD collection
UINTN gMinSPDs = 0;                     // minimum number of detected DIMM SPDs before starting the test
INTN gExactSPDs[MAX_NUM_SPDS];          // exact number of detected DIMM SPDs before starting the test. This variable overwrites gMinSPDs
INT64 gExactSPDSize;                    // exact total size of detected DIMM SPDs before starting the test.
BOOLEAN gCheckMemSPDSize = FALSE;       // if TRUE, check to see if the system memory size is consistent with the total memory capacity of all SPDs detected
CHAR16 gSPDManuf[SHORT_STRING_LEN];     // substring to match the JEDEC manufacturer in the SPD before starting the test
CHAR16 gSPDPartNo[MODULE_PARTNO_LEN];   // substring to match the part number in the SPD before starting the test
BOOLEAN gSameSPDPartNo = FALSE;         // if TRUE, check to see if all SPD part numbers match
BOOLEAN gSPDMatch = FALSE;              // if TRUE, check to see if SPD data match the values defined in the SPD.spd file
int gFailedSPDMatchIdx = -1;            // if gSPDMatch is TRUE and SPD check has failed, contains the failed SPD DIMM #
int gFailedSPDMatchByte = -1;           // if gSPDMatch is TRUE and SPD check has failed, contains the failed SPD byte #
UINT8 gFailedSPDActualValue = 0;        // if gSPDMatch is TRUE and SPD check has failed, contains the failed SPD byte value
UINT8 gFailedSPDExpectedValue = 0;      // if gSPDMatch is TRUE and SPD check has failed, contains the expected SPD byte value
CHECKMEMSPEED *gCheckMemSpeed;          // Zero or more checks on active memory speed
int gNumCheckMemSpeed;                  // Size of gCheckMemSpeed
BOOLEAN gCheckSPDSMBIOS = FALSE;        // if TRUE, check if SPD and SMBIOS values are consistent
INTN gSPDReportByteLo = 0;              // Lower byte of the range of the SPD raw bytes to display in the report
INTN gSPDReportByteHi = -1;             // Upper byte of the range of the SPD raw bytes to display in the report
BOOLEAN gSPDReportExtSN = FALSE;        // If TRUE, display the extended 18-digital serial number in the report
UINTN gBgColour = EFI_BLACK;            // background colour in console mode
UINTN gConsoleMode = 0;                 // Console mode (ie. console resolution)
UINTN gBitFadeSecs = DEF_SLEEP_SECS;    // Sleep interval for Bit Fade Test
UINTN gMaxErrCount = DEFAULT_MAXERRCOUNT;
BOOLEAN gTriggerOnErr = FALSE;               // if TRUE, show the address of the error structure before the start of the test
UINTN gReportPrefix = REPORT_PREFIX_DEFAULT; // 0 - default, 1 - Prefix filename with smbios baseboard serial number, 2 - Prefix filename with smbios system information serial number
CHAR16 gReportPrepend[MAX_PREPEND_LEN];      // Prepend report filename with custom string

BOOLEAN gConsoleOnly = FALSE;

// console fg/bg colours
static EG_PIXEL mEfiColors[16] = {
    {0x00, 0x00, 0x00, 0x00},
    {0x98, 0x00, 0x00, 0x00},
    {0x00, 0x98, 0x00, 0x00},
    {0x98, 0x98, 0x00, 0x00},
    {0x00, 0x00, 0x98, 0x00},
    {0x98, 0x00, 0x98, 0x00},
    {0x00, 0x98, 0x98, 0x00},
    {0x98, 0x98, 0x98, 0x00},
    {0x10, 0x10, 0x10, 0x00},
    {0xff, 0x10, 0x10, 0x00},
    {0x10, 0xff, 0x10, 0x00},
    {0xff, 0xff, 0x10, 0x00},
    {0x10, 0x10, 0xff, 0x00},
    {0xf0, 0x10, 0xff, 0x00},
    {0x10, 0xff, 0xff, 0x00},
    {0xff, 0xff, 0xff, 0x00}};

#define WATCHDOG_CODE 0x506173734d61726bULL
struct PAT_UI {
	UINT32 CurPattern[4];
	UINT32 NewPattern[4];
	UINT8 CurSize;
	UINT8 NewSize;
};
extern struct PAT_UI mPatternUI;
CONST UINT8  Aes128CbcKey[] = {
  0xc2, 0x86, 0x69, 0x6d, 0x88, 0x7c, 0x9a, 0xa0, 0x61, 0x1b, 0xbb, 0x3e, 0x20, 0x25, 0xa4, 0x5a
};

CONST UINT8  Aes128CbcIvec[] = {
  0x56, 0x2e, 0x17, 0x99, 0x6d, 0x09, 0x3d, 0x28, 0xdd, 0xb3, 0xba, 0x69, 0x5a, 0x2e, 0x6f, 0x58
};
VOID *gAesContext;

// #define AES_ENABLE

//OKN_20240829_yjb >>
EFI_STATUS
SmbiosGetString(
  IN      CHAR8                   *OptionalStrStart,
  IN      UINT8                   Index,
  OUT     CHAR8                   **String
  )
{
  UINTN          StrSize;

  if (Index == 0) {
    return EFI_INVALID_PARAMETER;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    return EFI_INVALID_PARAMETER;
  }
  *String = OptionalStrStart;

  return EFI_SUCCESS;
}

// extern UINT8 mSpdTableDDR[8][2][1024];
extern UINT8 mSpdTableDDR[8][2][2][1024];

extern VOID EFIAPI UnlockAllMemRanges(VOID);
extern VOID EFIAPI LockAllMemRanges(VOID);
VOID JsonHandler(cJSON *Tree)
{
    EFI_STATUS Status = 0;
    
    cJSON *Cmd = cJSON_GetObjectItemCaseSensitive(Tree, "CMD");
    if (!Cmd || Cmd->type != cJSON_String)
        return;
    if (AsciiStrnCmp("testStatus", Cmd->valuestring, 10) == 0) {
        cJSON *ErrorInfo = cJSON_AddArrayToObject(Tree, "ERRORINFO");
        cJSON_AddNumberToObject(Tree, "DataMissCompare", MtSupportGetNumErrors());
        cJSON_AddNumberToObject(Tree, "CorrError", MtSupportGetNumCorrECCErrors());
        cJSON_AddNumberToObject(Tree, "UncorrError", MtSupportGetNumUncorrECCErrors());
        // cJSON_AddNumberToObject(Tree, "UnknownError", MtSupportGetNumUnknownSlotErrors());
#if 1
        DIMM_ADDRESS_DETAIL *Item = NULL;
        // UINT8 Dimm = 0;


        // static UINTN addddd = 0x12345678;
        // if (gOknDimmErrorQueue.Head == gOknDimmErrorQueue.Tail) {
        //     DIMM_ADDRESS_DETAIL NewItem = {addddd++, 0, 0, 0, 0, 0, 0, 0, 0, 0x10, 0x20, 2};
        //     EnqueueError(&gOknDimmErrorQueue, &NewItem);
        // }


        for (UINT8 i = 0; i < 4; i++) {
            Item = DequeueError(&gOknDimmErrorQueue);
            if (!Item) {
                break;
            }
            cJSON *Detail =  cJSON_CreateObject();
            // Dimm = Item->SocketId * 8 + Item->MemCtrlId * 2 + Item->ChannelId;
            AsciiSPrint(gBuffer, BUF_SIZE, "%llx", Item->Address);
            cJSON_AddStringToObject(Detail, "Address", gBuffer);
            cJSON_AddNumberToObject(Detail, "Type", Item->Type);
            cJSON_AddNumberToObject(Detail, "Socket", Item->SocketId);
            cJSON_AddNumberToObject(Detail, "Channel", Item->MemCtrlId * 2 + Item->ChannelId);
            cJSON_AddNumberToObject(Detail, "Dimm", 0);
            cJSON_AddNumberToObject(Detail, "Rank", Item->RankId);
            cJSON_AddNumberToObject(Detail, "SubCh", Item->SubChId);
            cJSON_AddNumberToObject(Detail, "BG", Item->BankGroup);
            cJSON_AddNumberToObject(Detail, "Bank", Item->Bank);
            cJSON_AddNumberToObject(Detail, "Row", Item->Row);
            cJSON_AddNumberToObject(Detail, "Col", Item->Column);
            cJSON_AddItemToArray(ErrorInfo, Detail);
        }
#else
        int ChipWidth = 0;
        int NumRanks = 0;
        int NumChips = 0;
        for (UINT8 i = 0; i < g_numSMBIOSMem; i++) {
            cJSON *SlotErr = cJSON_CreateArray();
            cJSON_AddItemToArray(ErrorInfo, SlotErr);
            if (g_SMBIOSMemDevices[i].smb17.Size != 0) {
                ChipWidth = MtSupportGetChipWidth(i);
                NumRanks = MtSupportGetNumRanks(i);
                NumChips = NumRanks * 64 / ChipWidth;
                for (int chip = 0; chip < NumChips; chip++)
                {
                    int ChipErrs = MtSupportGetNumSlotChipErrors(i, chip);
                    if (ChipErrs > 0)
                    {
                        cJSON *ChipErr = cJSON_CreateObject();
                        MtSupportGetChipLabel(chip, g_wszBuffer, BUF_SIZE);
                        UnicodeStrToAsciiStrS(g_wszBuffer, gBuffer, BUF_SIZE);
                        cJSON_AddStringToObject(ChipErr, "Chip", gBuffer);
                        cJSON_AddNumberToObject(ChipErr, "ErrorCount", ChipErrs);
                        cJSON_AddItemToArray(SlotErr, ChipErr);
                        // TotalChipErrs += ChipErrs;
                    }
                }
            }
        }
#endif
        GetStringById(STRING_TOKEN(gCustomTestList[gOknMT86TestID].NameStrId), g_wszBuffer, BUF_SIZE);
        UnicodeStrToAsciiStrS(g_wszBuffer, gBuffer, BUF_SIZE);
        cJSON_AddStringToObject(Tree, "NAME", gBuffer);
        cJSON_AddNumberToObject(Tree, "ID", gOknMT86TestID + 47);
        switch (mPatternUI.NewSize)
        {
        case 4:
            AsciiSPrint(gBuffer, BUF_SIZE, "0x%08X", mPatternUI.NewPattern[0]);
            break;
        case 8:
            AsciiSPrint(gBuffer, BUF_SIZE, "0x%016lX", *((UINT64 *)mPatternUI.NewPattern));
            break;
        case 16:
            AsciiSPrint(gBuffer, BUF_SIZE, "0x%08X%08X", mPatternUI.NewPattern[0], mPatternUI.NewPattern[1]);
            break;
        }
        cJSON_AddStringToObject(Tree, "PATTERN", gBuffer);
        cJSON_AddNumberToObject(Tree, "PROGRESS", gOknLastPercent);
        // cJSON_AddNumberToObject(Tree, "MEMSTART", gStartAddr);
        // cJSON_AddNumberToObject(Tree, "MEMEND", gEndAddr);
        cJSON_AddNumberToObject(Tree, "STATUS", gOknTestStatus);
    } else if (AsciiStrnCmp("testStart", Cmd->valuestring, 9) == 0) {
        gOknLastPercent = 0;
        InitErrorQueue(&gOknDimmErrorQueue);
        gOknTestPause = FALSE;

        cJSON *Id = cJSON_GetObjectItemCaseSensitive(Tree, "ID");
        if (Id == NULL || Id->type != cJSON_Number || Id->valueu64 > gNumCustomTests + 47 || Id->valueu64 < 47) {
            return;
        }
        for (UINT8 i = 0; i < gNumCustomTests; i++) {
            gCustomTestList[i].Enabled = FALSE;
        }
        gCustomTestList[Id->valueu64 - 47].Enabled = TRUE;
        gOknMT86TestID = (INT8)(Id->valueu64 - 47);
        gNumPasses = 1;
        gOknTestStart = TRUE;
        gOknTestStatus = 1;

        cJSON_AddBoolToObject(Tree, "SUCCESS", gOknTestStart);
        cJSON *SlotInfo = cJSON_AddArrayToObject(Tree, "SLOTINFO");

        UINT8 Online = 0;
        INT32 RamTemp = 0;
        int index = 0;
        MtSupportGetTSODInfo(&RamTemp, 0);
        UINT8 _socket = 0, _channel = 0, socket = 0, channel = 0;
        for (UINT8 i = 0; i < g_numSMBIOSMem; i++) {
            socket = i / 8;
            channel = i % 8;
            _socket = channel % 2;
            _channel = ((socket << 3) + channel) / 2;
            cJSON *Info = cJSON_CreateObject();
            if (mSpdTableDDR[_channel][_socket][0] != 0) {
                Online = 1;
                RamTemp = g_MemTSODInfo[index++].temperature;
            } else {
                Online = 0;
                RamTemp = 0;
            }
            cJSON_AddNumberToObject(Info, "ONLINE", Online);
            cJSON_AddNumberToObject(Info, "RAMTEMP", RamTemp);
            cJSON_AddItemToArray(SlotInfo, Info);
        }
        cJSON_AddNumberToObject(Tree, "STATUS", gOknTestStart);
        
    } else if (AsciiStrnCmp("testStop", Cmd->valuestring, 8) == 0) {
        cJSON_AddBoolToObject(Tree, "SUCCESS", TRUE);
        MtSupportAbortTesting();
        gOknTestStart = FALSE;   // fix duplicate test
        gOknTestStatus = 2;
        // gTestStop = TRUE;
    } else if (AsciiStrnCmp("areyouok", Cmd->valuestring, 8) == 0) {
        cJSON_AddBoolToObject(Tree, "SUCCESS", true);
        // 关键：MAC/IP 来自“当前接收该包的 NIC”
        if (gOknJsonCtxSocket != NULL) {
          AsciiSPrint(gBuffer, BUF_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x",
                      gOknJsonCtxSocket->NicMac[0], gOknJsonCtxSocket->NicMac[1],
                      gOknJsonCtxSocket->NicMac[2], gOknJsonCtxSocket->NicMac[3],
                      gOknJsonCtxSocket->NicMac[4], gOknJsonCtxSocket->NicMac[5]);
          cJSON_AddStringToObject(Tree, "MAC", gBuffer);
      
          if (gOknJsonCtxSocket->NicIpValid) {
            AsciiSPrint(gBuffer, BUF_SIZE, "%d.%d.%d.%d",
                        gOknJsonCtxSocket->NicIp.Addr[0], gOknJsonCtxSocket->NicIp.Addr[1],
                        gOknJsonCtxSocket->NicIp.Addr[2], gOknJsonCtxSocket->NicIp.Addr[3]);
            cJSON_AddStringToObject(Tree, "IP", gBuffer);
          }
        } else {
          // 兜底：至少别填错
          cJSON_AddStringToObject(Tree, "MAC", "00:00:00:00:00:00");
          cJSON_AddStringToObject(Tree, "IP",  "0.0.0.0");
        }
      
        AsciiSPrint(gBuffer, BUF_SIZE, "mt86-v%d.%d", PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR);
        cJSON_AddStringToObject(Tree, "FW", gBuffer);
    } else if (AsciiStrnCmp("connect", Cmd->valuestring, 7) == 0) {
        cJSON_AddNumberToObject(Tree, "RESULT", g_numTSODModules);
        //OKN_20240829_yjb >>
        EFI_SMBIOS_HANDLE                     SmbiosHandle;
        EFI_SMBIOS_PROTOCOL                   *Smbios;
        SMBIOS_TABLE_TYPE4                    *SmbiosType4Record;
        EFI_SMBIOS_TYPE                       SmbiosType;

        SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
        SmbiosType = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
        Status = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
        if (Status == 0) {
            while (1) {
                Status = Smbios->GetNext(Smbios, &SmbiosHandle, &SmbiosType, (EFI_SMBIOS_TABLE_HEADER **)&SmbiosType4Record, NULL);
                if (EFI_ERROR(Status)) {
                    break;
                }
                CHAR8 *CpuStr, *SocketStr;
                Status = SmbiosGetString((CHAR8*)((UINT8*)SmbiosType4Record + SmbiosType4Record->Hdr.Length), SmbiosType4Record->Socket, &SocketStr);
                Status = SmbiosGetString((CHAR8*)((UINT8*)SmbiosType4Record + SmbiosType4Record->Hdr.Length), SmbiosType4Record->ProcessorVersion, &CpuStr);
                cJSON_AddStringToObject(Tree, SocketStr, CpuStr);
            }
        }
        //OKN_20240829_yjb <<
        cJSON *SlotInfo = cJSON_AddArrayToObject(Tree, "SLOTINFO");
        UINT8 Online = 0;
        INT32 RamTemp = 0;
        int ChipWidth = 0;
        int NumRanks = 0;
        int NumChips = 0;
        int index = 0;
        UINT8 _socket = 0, _channel = 0, socket = 0, channel = 0;
        MtSupportGetTSODInfo(&RamTemp, 0);
#ifndef SEAVO
        for (UINT8 i = 0; i < g_numSMBIOSMem; i++) {
            socket = i / 8;
            channel = i % 8;
            _socket = channel % 2;
            _channel = ((socket << 3) + channel) / 2;
#else 
        for (UINT8 i = 0; i < 8; i++) {    
            _channel = i;
            UINT8 dimm = 0;
            _socket = 0;
#endif
            cJSON *Info = cJSON_CreateObject();
#ifndef SEAVO
            if (mSpdTableDDR[_channel][_socket][0] != 0) {
#else
            if (mSpdTableDDR[_channel][_socket][dimm][0] != 0) {
#endif
                Online = 1;
                RamTemp = g_MemTSODInfo[index++].temperature;
                // GetMemInfoSpd(i, &ChipWidth, &NumRanks);
                // ChipWidth = MtSupportGetChipWidth(i);
                // NumRanks = MtSupportGetNumRanks(i);
                // NumChips = NumRanks * 64 / ChipWidth;
                cJSON_AddNumberToObject(Info, "CHIPWITH", ChipWidth);
                cJSON_AddNumberToObject(Info, "RANKS", NumRanks);
                cJSON_AddNumberToObject(Info, "CHIPS", NumChips);
            } else {
                Online = 0;
                RamTemp = 0;
            }
            cJSON_AddNumberToObject(Info, "ONLINE", Online);
            cJSON *Reason = cJSON_AddArrayToObject(Info, "REASON");
            GetMapOutReason(socket, channel, Reason);
            cJSON_AddNumberToObject(Info, "RAMTEMP", RamTemp);
            cJSON_AddItemToArray(SlotInfo, Info);
        }
    } else if (AsciiStrnCmp("readSPD", Cmd->valuestring, 7) == 0) {
        readSPD(Tree);
    } else if (AsciiStrnCmp("testConfigGet", Cmd->valuestring, 13) == 0) {
        testConfigGet(Tree);
    } else if (AsciiStrnCmp("testConfigSet", Cmd->valuestring, 13) == 0) {
        testConfigSet(Tree);
    } else if (AsciiStrnCmp("testConfigActive", Cmd->valuestring, 16) == 0) {
        testConfigActive(Tree);
    } else if (AsciiStrnCmp("regRead", Cmd->valuestring, 7) == 0) {
        regRead(Tree);
    } else if (AsciiStrnCmp("reset", Cmd->valuestring, 5) == 0) {
        cJSON *Type = cJSON_GetObjectItemCaseSensitive(Tree, "TYPE");
        gOknTestReset = (UINT8)Type->valueu64;
    } else if (AsciiStrnCmp("amtStart", Cmd->valuestring, 8) == 0) {
        amtControl(Tree, TRUE);
    } else if (AsciiStrnCmp("amtStop", Cmd->valuestring, 7) == 0) {
        amtControl(Tree, FALSE);
    } else if (AsciiStrnCmp("rmtStart", Cmd->valuestring, 8) == 0) {
        rmtControl(Tree, TRUE);
    } else if (AsciiStrnCmp("rmtStop", Cmd->valuestring, 7) == 0) {
        rmtControl(Tree, FALSE);
    } else if (AsciiStrnCmp("ecsStatus", Cmd->valuestring, 9) == 0) {
        readEcsReg(Tree);
    } else if (AsciiStrnCmp("trainMsg", Cmd->valuestring, 8) == 0) {
        trainMsgCtrl(Tree);
    } else if (AsciiStrnCmp("cpuSelect", Cmd->valuestring, 9) == 0) {
        cpuSelect(Tree);
    } else if (AsciiStrnCmp("spdPrint", Cmd->valuestring, 8) == 0) {
        spdPrint(Tree);
    } else if (AsciiStrnCmp("injectPpr", Cmd->valuestring, 9) == 0) {
        injectPpr(Tree);
    }
}

VOID EFIAPI Udp4ReceiveHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  UDP4_SOCKET           *Socket;
  EFI_UDP4_RECEIVE_DATA *RxData;
  EFI_STATUS            Status;

  Socket = (UDP4_SOCKET *)Context;
  if (Socket == NULL || Socket->Udp4 == NULL) {
    return;
  }

  RxData = Socket->TokenReceive.Packet.RxData;

  // 1) Token 完成但没有数据：按 UEFI UDP4 约定, 重新投递 Receive()
  if (NULL == RxData) {
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

  // 2) 若 Receive 被 Abort：回收 RxData 并退出
  if (EFI_ABORTED == Socket->TokenReceive.Status) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    return;
  }

  // 3) 空包：回收并重新投递
  if (RxData->DataLength == 0 || RxData->FragmentCount == 0 ||
      RxData->FragmentTable[0].FragmentBuffer == NULL ||
      RxData->FragmentTable[0].FragmentLength == 0) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

  // 4) 首包绑定：选择第一个收到包的 NIC, 其它 RX socket 全部禁用
  if (gOknUdpRxActiveSocket == NULL) {
    gOknUdpRxActiveSocket   = Socket;
    gOknUdpRxActiveSbHandle = Socket->ServiceBindingHandle;

    for (UINTN i = 0; i < gOknUdpRxSocketCount; i++) {
      if (gOknUdpRxSockets[i] != NULL && gOknUdpRxSockets[i] != Socket) {
        DisableUdpRxSocket(gOknUdpRxSockets[i]);
      }
    }
  }

  // 5) 非选中 NIC 上来的包直接丢弃（但要回收 RxData）
  if (Socket != gOknUdpRxActiveSocket) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    return;
  }

  // 6) 懒创建 TX socket：只创建一次, 绑定到选中的 SB handle
  //    注意：NotifyTransmit 改为 Udp4TxFreeHandler, 用于释放本次发送的内存
  if (gOknUdpSocketTransmit == NULL && gOknUdpRxActiveSbHandle != NULL) {
    EFI_UDP4_CONFIG_DATA TxCfg = {
      TRUE,   // AcceptBroadcast
      FALSE,  // AcceptPromiscuous
      FALSE,  // AcceptAnyPort
      TRUE,   // AllowDuplicatePort
      0,      // TypeOfService
      16,     // TimeToLive
      TRUE,   // DoNotFragment
      0,      // ReceiveTimeout
      0,      // TransmitTimeout
      TRUE,   // UseDefaultAddress
      {{0, 0, 0, 0}},  // StationAddress
      {{0, 0, 0, 0}},  // SubnetMask
      5566,            // StationPort
      {{0, 0, 0, 0}},  // RemoteAddress (unused when using UdpSessionData)
      0,               // RemotePort    (unused when using UdpSessionData)
    };

    EFI_STATUS TxStatus;

    TxStatus = CreateUdp4SocketByServiceBindingHandle(
                 gOknUdpRxActiveSbHandle,
                 &TxCfg,
                 (EFI_EVENT_NOTIFY)Udp4NullHandler,   // Tx socket 不需要 Receive 回调
                 (EFI_EVENT_NOTIFY)Udp4TxFreeHandler, // TX 完成释放资源: Udp4TxFreeHandler
                 &gOknUdpSocketTransmit);

    if (EFI_ERROR(TxStatus)) {
      Print(L"[UDP] Create TX socket failed: %r\n", TxStatus);
      gOknUdpSocketTransmit = NULL;
    } else {
      // 初始化 TX 内存追踪字段（需要你在 UDP4_SOCKET 结构体中加入这些字段）
      gOknUdpSocketTransmit->TxPayload    = NULL;
      gOknUdpSocketTransmit->TxSession    = NULL;
      gOknUdpSocketTransmit->TxInProgress = FALSE;
    }
  }

  if (gOknUdpSocketTransmit == NULL) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

  // 7) 若上一次发送还未完成（Token 仍挂起）, 为避免覆盖 Token/内存, 直接丢弃本次回复
  if (gOknUdpSocketTransmit->TxInProgress) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

#ifndef AES_ENABLE
  // 8) 解析 JSON
  cJSON *Tree = cJSON_ParseWithLength(
                  (CONST CHAR8 *)RxData->FragmentTable[0].FragmentBuffer,
                  RxData->FragmentTable[0].FragmentLength);

  if (Tree != NULL) {
    // 9) 设置 JsonHandler 上下文：确保 areyouok 填的是“当前 NIC”的 MAC/IP
    gOknJsonCtxSocket = Socket;
    JsonHandler(Tree);
    gOknJsonCtxSocket = NULL;

    // 10) 生成响应 JSON（cJSON_PrintUnformatted 返回需要 cJSON_free 的内存）
    CHAR8 *JsonStr = cJSON_PrintUnformatted(Tree);
    if (JsonStr != NULL) {
      UINT32 JsonStrLen = (UINT32)AsciiStrLen(JsonStr);

      // 关键：Transmit 是异步的, 不能直接把 JsonStr 指针塞给 TxData 再立刻释放
      // 所以这里复制一份到 Pool, TX 完成回调里释放
      VOID *Payload = AllocateCopyPool(JsonStrLen, JsonStr);
      cJSON_free(JsonStr);

      if (Payload != NULL && JsonStrLen > 0) {
        EFI_UDP4_SESSION_DATA *Sess = AllocateZeroPool(sizeof(EFI_UDP4_SESSION_DATA));
        if (Sess != NULL) {
          Sess->DestinationAddress = RxData->UdpSession.SourceAddress;
          Sess->DestinationPort    = RxData->UdpSession.SourcePort;
          Sess->SourcePort         = 5566;
          // 可选：如果你在 Socket 里保存了 DHCP IP, 可把源地址也填上
          // Sess->SourceAddress = Socket->NicIp;

          EFI_UDP4_TRANSMIT_DATA *TxData = gOknUdpSocketTransmit->TokenTransmit.Packet.TxData;
          ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
          TxData->UdpSessionData = Sess;
          TxData->DataLength     = JsonStrLen;
          TxData->FragmentCount  = 1;
          TxData->FragmentTable[0].FragmentLength = JsonStrLen;
          TxData->FragmentTable[0].FragmentBuffer = Payload;

          // 记录待释放资源, TX 完成回调释放
          gOknUdpSocketTransmit->TxPayload    = Payload;
          gOknUdpSocketTransmit->TxSession    = Sess;
          gOknUdpSocketTransmit->TxInProgress = TRUE;

          Status = gOknUdpSocketTransmit->Udp4->Transmit(gOknUdpSocketTransmit->Udp4, &gOknUdpSocketTransmit->TokenTransmit);
          if (EFI_ERROR(Status)) {
            // 发送失败：立即释放并复位
            gOknUdpSocketTransmit->TxInProgress = FALSE;
            if (gOknUdpSocketTransmit->TxPayload) { FreePool(gOknUdpSocketTransmit->TxPayload); gOknUdpSocketTransmit->TxPayload = NULL; }
            if (gOknUdpSocketTransmit->TxSession) { FreePool(gOknUdpSocketTransmit->TxSession); gOknUdpSocketTransmit->TxSession = NULL; }
          }
        } else {
          FreePool(Payload);
        }
      } else {
        if (Payload) FreePool(Payload);
      }
    }

    cJSON_Delete(Tree);
  }
#else
  UINT32 InputSize = ALIGN_VALUE(RxData->FragmentTable[0].FragmentLength, 16);
  ZeroMem(g_wszBuffer, InputSize);
  CopyMem(g_wszBuffer, RxData->FragmentTable[0].FragmentBuffer, RxData->FragmentTable[0].FragmentLength);
  BOOLEAN Ok = AesCbcDecrypt(gAesContext, (UINT8 *)g_wszBuffer, InputSize, Aes128CbcIvec, (UINT8 *)gBuffer);
  if (Ok) {
    cJSON *Tree = NULL;
    Tree = cJSON_ParseWithLength(gBuffer, InputSize);
    if (Tree) {
        JsonHandler(Tree);
        CHAR8 *JsonStr = cJSON_PrintUnformatted(Tree);
        UINT32 JsonStrLen = (UINT32)AsciiStrLen(JsonStr);
        InputSize = ALIGN_VALUE(JsonStrLen, 16);
        ZeroMem(g_wszBuffer, InputSize);
        CopyMem(g_wszBuffer, JsonStr, JsonStrLen);
        Ok = AesCbcEncrypt(gAesContext, (UINT8 *)g_wszBuffer, InputSize, Aes128CbcIvec, (UINT8 *)gBuffer);
        if (Ok) {
            EFI_UDP4_TRANSMIT_DATA *TxData = gOknUdpSocketTransmit->TokenTransmit.Packet.TxData;
            ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
            TxData->DataLength = InputSize;
            TxData->FragmentCount = 1;
            TxData->FragmentTable[0].FragmentLength = InputSize;
            TxData->FragmentTable[0].FragmentBuffer = gBuffer;
            gOknUdpSocketTransmit->Udp4->Transmit(gOknUdpSocketTransmit->Udp4, &gOknUdpSocketTransmit->TokenTransmit);
        }
        cJSON_Delete(Tree);
    }
  }
#endif

  // 11) 回收 RxData 并重新投递 Receive
  gBS->SignalEvent(RxData->RecycleSignal);
  Socket->TokenReceive.Packet.RxData = NULL;
  Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
  return;
}

VOID EFIAPI Udp4NullHandler(IN EFI_EVENT  Event,  IN VOID *Context)
{

}
/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
UefiMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    MT_HANDLE CfgFileHandle = NULL;
    MT_HANDLE BLFileHandle = NULL;
    EFI_TPL OldTPL = TPL_APPLICATION;
    EFI_SIMPLE_TEXT_OUTPUT_MODE SavedConsoleMode;
    UINT32 MaxGraphicsMode = 0;
    BOOLEAN PrevGraphicsEn;
    UINTN Select = ID_BUTTON_SYSINFO;

    CHAR16 *ImageFilePath;
    CHAR16 *Temp;
    CHAR16 TempBuf[128];
    int i;

    MTCFG *MTConfig = NULL;
    UINTN NumConfig = 0;
    UINTN ConfigTimeout = 0;
    INTN DefaultCfgIdx = 0;
    INTN CfgIdx = 0;
    BLACKLIST Blacklist;

    EFI_HANDLE *SNHandleList = NULL;
    UINTN NumSNHandles = 0;

#ifdef SHELLAPP
    LIST_ENTRY *Package;
    CHAR16 *ProblemParam;

    Package = NULL;
    ProblemParam = NULL;
#endif

    gRT->GetTime(&gBootTime, NULL);

    Status = EFI_SUCCESS;

    ImageFilePath = NULL;
    Temp = NULL;

    //
    // Save the current console cursor position and attributes
    //
    CopyMem(&SavedConsoleMode, gST->ConOut->Mode, sizeof(SavedConsoleMode));
    gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &ConWidth, &ConHeight);
    gST->ConOut->SetMode(gST->ConOut, 0); // Set to 80 x 25
    gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &ConWidth, &ConHeight);
    gConsoleMode = gST->ConOut->Mode->Mode;
    MaxUIWidth = ConWidth > 80 || ConWidth == 0 ? 80 : ConWidth;

    gST->ConOut->EnableCursor(gST->ConOut, FALSE);

    gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&SelfLoadedImage);

    EnableGraphicsMode(FALSE);

#ifdef SHELLAPP // not used
    //
    // Try to find the old shell. If found, we will need to load some drivers
    //
    if (ShellFindSE2(gImageHandle) == EFI_SUCCESS)
    {
        //
        // Locate Hii Database protocol
        //
        Status = LoadDriver(L"HiiDatabase.efi");

        //
        // Locate DevicePathToText protocol
        //
        Status = LoadDriver(L"DevicePathDxe.efi");

        //
        // Locate UnicodeCollation2 protocol
        //
        Status = LoadDriver(L"Uc2OnUcThunk.efi");

        if (mEfiShellEnvironment2 != NULL)
        {
            gBS->CloseProtocol(mEfiShellEnvironment2Handle == NULL ? gImageHandle : mEfiShellEnvironment2Handle,
                               &gEfiShellEnvironment2Guid,
                               gImageHandle,
                               NULL);
            mEfiShellEnvironment2 = NULL;
            mEfiShellEnvironment2Handle = NULL;
        }
    }

    //
    // initialize the shell lib (we must be in non-auto-init...)
    //
    if (SelfLoadedImage->ParentHandle != NULL && ShellInitialize() == EFI_SUCCESS)
    {
        //
        // parse the command line
        //

        if (ShellCommandLineParse(ParamList, &Package, &ProblemParam, TRUE) == EFI_SUCCESS)
        {
            //
            // Initialize SilentMode and RecursiveMode
            //
            gDebugMode = ShellCommandLineGetFlag(Package, L"-d");

            //
            // free the command line package
            //
            ShellCommandLineFreeVarList(Package);

            //  Log MemTest86 version
#ifdef MDE_CPU_X64
            AsciiSPrint(gBuffer, BUF_SIZE, "%s %s Build: %s (64-bit)", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
#elif defined(MDE_CPU_IA32)
            AsciiSPrint(gBuffer, BUF_SIZE, "%s %s Build: %s (32-bit)", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
#endif

            MtSupportDebugWriteLine(gBuffer);
        }
    }
#endif

    if (SelfLoadedImage)
    {
        // Check if we are booting from PXE
        Status = gBS->HandleProtocol(
            SelfLoadedImage->DeviceHandle,
            &gEfiPxeBaseCodeProtocolGuid,
            (VOID **)&PXEProtocol);

        if (Status == EFI_SUCCESS)
        {
            // We are booting from PXE so saved the necessary info
#ifdef PXEBOOT
            SetMem(&ServerIP, sizeof(ServerIP), 0);
            if (PXEProtocol->Mode->UsingIpv6)
            {
                EFI_INPUT_KEY key;

                SetMem(&key, sizeof(key), 0);

                MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                              L"DHCPv6 is not supported in this version of MemTest86",
                              NULL);

                gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
                return EFI_SUCCESS;
            }
            else
            {
                EFI_PXE_BASE_CODE_PACKET *Packet = NULL;

                // Check for command line arguments
                CHAR16 *CommandLineArgs = (CHAR16 *)SelfLoadedImage->LoadOptions;
                CHAR16 *TftpRootHead = NULL;
                CHAR16 *TftpRootTail = NULL;

                if ((TftpRootHead = StrStr(CommandLineArgs, L"TFTPROOT=")) != NULL) // Look for 'TFTPROOT' command line argument
                {
                    // Look for the value of the parameter
                    CHAR16 *TftpRootValue = TftpRootHead + StrLen(L"TFTPROOT=");
                    CHAR16 *Separator = L" ";

                    // Check if the value is enclosed in double-quotes
                    if (TftpRootValue[0] == L'\"')
                    {
                        Separator = L"\"";
                        TftpRootValue++;
                    }

                    // Look for the end of the value string
                    TftpRootTail = StrStr(TftpRootValue, Separator);
                    if (TftpRootTail == NULL)
                    {
                        TftpRootTail = CommandLineArgs + StrLen(CommandLineArgs);
                    }

                    // Save the TFTP directory path
                    if (TftpRootTail > TftpRootValue)
                        StrnCpyS(gTftpRoot, ARRAY_SIZE(gTftpRoot), TftpRootValue, (UINTN)(TftpRootTail - TftpRootValue));
                }

                // Save the Server's IP address
                if (PXEProtocol->Mode->PxeReplyReceived) // PXE Reply
                    Packet = &PXEProtocol->Mode->PxeReply;
                else if (PXEProtocol->Mode->ProxyOfferReceived) // DHCP Proxy Offer
                    Packet = &PXEProtocol->Mode->ProxyOffer;
                else if (PXEProtocol->Mode->DhcpAckReceived) // DHCP Ack
                    Packet = &PXEProtocol->Mode->DhcpAck;

                if (Packet->Dhcpv4.BootpSiAddr[0] == 0 &&
                    Packet->Dhcpv4.BootpSiAddr[1] == 0 &&
                    Packet->Dhcpv4.BootpSiAddr[2] == 0 &&
                    Packet->Dhcpv4.BootpSiAddr[3] == 0)
                {
                    EFI_INPUT_KEY key;

                    SetMem(&key, sizeof(key), 0);

                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                                  L"Warning - TFTP server IP not found",
                                  NULL);
                }
                else
                {
                    CopyMem(
                        &ServerIP.v4,
                        Packet->Dhcpv4.BootpSiAddr,
                        sizeof(EFI_IPv4_ADDRESS));
                }

                // Save the client's IP address
                if (Packet->Dhcpv4.BootpCiAddr[0] != 0 ||
                    Packet->Dhcpv4.BootpCiAddr[1] != 0 ||
                    Packet->Dhcpv4.BootpCiAddr[2] != 0 ||
                    Packet->Dhcpv4.BootpCiAddr[3] != 0)
                {
                    CopyMem(
                        &ClientIP.v4,
                        Packet->Dhcpv4.BootpCiAddr,
                        sizeof(EFI_IPv4_ADDRESS));
                }
                else if (Packet->Dhcpv4.BootpYiAddr[0] != 0 ||
                         Packet->Dhcpv4.BootpYiAddr[1] != 0 ||
                         Packet->Dhcpv4.BootpYiAddr[2] != 0 ||
                         Packet->Dhcpv4.BootpYiAddr[3] != 0)
                {
                    CopyMem(
                        &ClientIP.v4,
                        Packet->Dhcpv4.BootpYiAddr,
                        sizeof(EFI_IPv4_ADDRESS));
                }
#if 0
                Print(L"TFTPROOT=%s\n", CommandLineArgs);
                Print(L"ServerIP=%d.%d.%d.%d\n", ServerIP.v4.Addr[0], ServerIP.v4.Addr[1], ServerIP.v4.Addr[2], ServerIP.v4.Addr[3]);
                Print(L"ClientIP=%d.%d.%d.%d\n", ClientIP.v4.Addr[0], ClientIP.v4.Addr[1], ClientIP.v4.Addr[2], ClientIP.v4.Addr[3]);

                Print(L"PXEProtocol->Mode->ProxyOffer.Dhcpv4.BootpBootFile=%a\n", PXEProtocol->Mode->ProxyOffer.Dhcpv4.BootpBootFile);
                Print(L"PXEProtocol->Mode->ProxyOffer.Dhcpv4.BootpSiAddr=%d.%d.%d.%d\n", PXEProtocol->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[0], PXEProtocol->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[1], PXEProtocol->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[2], PXEProtocol->Mode->ProxyOffer.Dhcpv4.BootpSiAddr[3]);
                Print(L"PXEProtocol->Mode->PxeDiscover.Dhcpv4.BootpBootFile=%a\n", PXEProtocol->Mode->PxeDiscover.Dhcpv4.BootpBootFile);
                Print(L"PXEProtocol->Mode->PxeDiscover.Dhcpv4.BootpSiAddr=%d.%d.%d.%d\n", PXEProtocol->Mode->PxeDiscover.Dhcpv4.BootpSiAddr[0], PXEProtocol->Mode->PxeDiscover.Dhcpv4.BootpSiAddr[1], PXEProtocol->Mode->PxeDiscover.Dhcpv4.BootpSiAddr[2], PXEProtocol->Mode->PxeDiscover.Dhcpv4.BootpSiAddr[3]);
                Print(L"PXEProtocol->Mode->PxeReply.Dhcpv4.BootpBootFile=%a\n", PXEProtocol->Mode->PxeReply.Dhcpv4.BootpBootFile);
                Print(L"PXEProtocol->Mode->PxeReply.Dhcpv4.BootpSiAddr==%d.%d.%d.%d\n", PXEProtocol->Mode->PxeReply.Dhcpv4.BootpSiAddr[0], PXEProtocol->Mode->PxeReply.Dhcpv4.BootpSiAddr[1], PXEProtocol->Mode->PxeReply.Dhcpv4.BootpSiAddr[2], PXEProtocol->Mode->PxeReply.Dhcpv4.BootpSiAddr[3]);
                Print(L"PXEProtocol->Mode->DhcpAck.Dhcpv4.BootpBootFile=%a\n", PXEProtocol->Mode->DhcpAck.Dhcpv4.BootpBootFile);
                Print(L"PXEProtocol->Mode->DhcpAck.Dhcpv4.BootpSiAddr=%d.%d.%d.%d\n", PXEProtocol->Mode->DhcpAck.Dhcpv4.BootpSiAddr[0], PXEProtocol->Mode->DhcpAck.Dhcpv4.BootpSiAddr[1], PXEProtocol->Mode->DhcpAck.Dhcpv4.BootpSiAddr[2], PXEProtocol->Mode->DhcpAck.Dhcpv4.BootpSiAddr[3]);

				{
                    UINTN EventIndex;
                    EFI_INPUT_KEY       key;

                    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);

                    gST->ConIn->ReadKeyStroke(gST->ConIn, &key);

                }
#endif
                CopyMem(&gTFTPServerIp, &ServerIP, sizeof(gTFTPServerIp));
                // CopyMem(&ClientMAC.Addr, Packet->Dhcpv4.BootpHwAddr, Packet->Dhcpv4.BootpHwAddrLen);
            }

#else
            EFI_INPUT_KEY key;

            SetMem(&key, sizeof(key), 0);

            MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                          L"PXE Boot is not supported in this version of MemTest86",
                          NULL);

            gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
            return EFI_SUCCESS;
#endif
        }
        else
        {
            //
            // Get the application path and folder
            //
            SelfRootDir = EfiLibOpenRoot(SelfLoadedImage->DeviceHandle);

            ImageFilePath = ConvertDevicePathToText(SelfLoadedImage->FilePath, FALSE, FALSE);

            Temp = FindPath(ImageFilePath);
            SelfDirPath = SplitDeviceString(Temp);

            FreePool(Temp);
            FreePool(ImageFilePath);

            if (SelfDirPath[0])
            {
                if (SelfRootDir)
                    SelfRootDir->Open(SelfRootDir, &SelfDir, SelfDirPath, EFI_FILE_MODE_READ, 0);
                if (SelfDir == NULL)
                    SelfDir = SelfRootDir;
            }
            else
                SelfDir = SelfRootDir;

            MtSupportDebugWriteLine("Attempting to retrieve the root directory of the data partition.");

            //
            // Get the root directory of the FAT file system on the 1st partition
            //
            Status = gBS->HandleProtocol(
                SelfLoadedImage->DeviceHandle,
                &gEfiDevicePathProtocolGuid,
                (VOID **)&SelfDevicePath);
            if (EFI_ERROR(Status))
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "Could not get DevicePath protocol: %r", Status);
                MtSupportDebugWriteLine(gBuffer);
            }

            Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &NumFSHandles, &FSHandleList);
            if (EFI_ERROR(Status))
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "Could not get list of handles that support SimpleFileSystem protocol: %r", Status);
                MtSupportDebugWriteLine(gBuffer);
            }

            AsciiSPrint(gBuffer, BUF_SIZE, "Found %d handles that supported SimpleFileSystem", NumFSHandles);
            MtSupportDebugWriteLine(gBuffer);

            for (i = 0; i < (int)NumFSHandles && DataEfiDir == NULL; i++)
            {
                EFI_DEVICE_PATH_PROTOCOL *DevicePath;

                AsciiSPrint(gBuffer, BUF_SIZE, "Checking handle %d (%x)", i, FSHandleList[i]);
                MtSupportDebugWriteLine(gBuffer);

                // Ignore own partition
                if (FSHandleList[i] == SelfLoadedImage->DeviceHandle)
                {
                    MtSupportDebugWriteLine("Handle is own partition");
                    continue;
                }

                Status = gBS->HandleProtocol(
                    FSHandleList[i],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&DevicePath);

                if (Status == EFI_SUCCESS)
                {
                    // Compare the current device path with the EFI system partition's device path
                    EFI_DEVICE_PATH_PROTOCOL *SelfDevPathNode = SelfDevicePath;
                    EFI_DEVICE_PATH_PROTOCOL *DevPathNode = DevicePath;
                    while (!IsDevicePathEnd(DevPathNode) && !IsDevicePathEnd(SelfDevPathNode))
                    {
                        /*
                        AsciiSPrint(gBuffer, BUF_SIZE, "DevPathNode->Type = %x, DevPathNode->SubType = %x ", DevPathNode->Type, DevPathNode->SubType );
                        MtSupportDebugWriteLine(gBuffer);

                        AsciiSPrint(gBuffer, BUF_SIZE, "SelfDevicePath->Type = %x, SelfDevicePath->SubType = %x ", SelfDevPathNode->Type, SelfDevPathNode->SubType );
                        MtSupportDebugWriteLine(gBuffer);
                        */

                        if (DevPathNode->Type != SelfDevPathNode->Type ||
                            DevPathNode->SubType != SelfDevPathNode->SubType)
                        {
                            // MtSupportDebugWriteLine("Handle does not mach");
                            break;
                        }

                        if (DevPathNode->Type == MEDIA_DEVICE_PATH &&
                            DevPathNode->SubType == MEDIA_HARDDRIVE_DP)
                        {
                            AsciiSPrint(gBuffer, BUF_SIZE, "HD node found: MBR type(%x), Signature type(%x), Part #(%d), Start LBA(%ld), Num Sectors(%ld)",
                                        ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->MBRType,
                                        ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->SignatureType,
                                        ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionNumber,
                                        ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionStart,
                                        ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionSize);

                            MtSupportDebugWriteLine(gBuffer);

                            AsciiSPrint(gBuffer, BUF_SIZE, "Self node: MBR type(%x), Signature type(%x), Part #(%d), Start LBA(%ld), Num Sectors(%ld)",
                                        ((HARDDRIVE_DEVICE_PATH *)SelfDevPathNode)->MBRType,
                                        ((HARDDRIVE_DEVICE_PATH *)SelfDevPathNode)->SignatureType,
                                        ((HARDDRIVE_DEVICE_PATH *)SelfDevPathNode)->PartitionNumber,
                                        ((HARDDRIVE_DEVICE_PATH *)SelfDevPathNode)->PartitionStart,
                                        ((HARDDRIVE_DEVICE_PATH *)SelfDevPathNode)->PartitionSize);

                            MtSupportDebugWriteLine(gBuffer);

                            if (((HARDDRIVE_DEVICE_PATH *)DevPathNode)->MBRType == PARTITION_TYPE_GPT &&
                                ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionNumber == 1 &&
                                CompareGuid((const GUID *)(((HARDDRIVE_DEVICE_PATH *)DevPathNode)->Signature), &DATA_PART_GUID))
                            {
                                EFI_FILE *DataRootDir;
                                MtSupportDebugWriteLine("Found handle of data partition device. Attempting to open handle to data partition root directory");

                                DataRootDir = EfiLibOpenRoot(FSHandleList[i]);
                                if (DataRootDir == NULL)
                                {
                                    MtSupportDebugWriteLine("Could not open root directory.");
                                }
                                else
                                {
                                    MtSupportDebugWriteLine("Handle to data partition root directory successfully opened.");
                                    MtSupportDebugWriteLine("** Log file shall be continued in MemTest86.log file on the data partition **");
                                    Status = DataRootDir->Open(DataRootDir, &DataEfiDir, L"EFI\\BOOT", EFI_FILE_MODE_READ, 0);
                                    if (EFI_ERROR(Status))
                                    {
                                        MtSupportDebugWriteLine("Could not open EFI\\BOOT directory of data partition. Continue logging on own partition.");
                                        DataEfiDir = NULL;
                                    }
                                    else
                                    {
                                        MtSupportDebugWriteLine("Successfully opened EFI\\BOOT directory of data partition");
                                    }
                                    DataRootDir->Close(DataRootDir);
                                }
                                break;
                            }
                        }

                        //
                        // Next device path node
                        //
                        DevPathNode = NextDevicePathNode(DevPathNode);
                        SelfDevPathNode = NextDevicePathNode(SelfDevPathNode);
                    }
                }
                else
                {
                    AsciiSPrint(gBuffer, BUF_SIZE, "Could not get DevicePath protocol: %r", Status);
                    MtSupportDebugWriteLine(gBuffer);
                }
            }

            if (DMATestPart == NULL)
            {
                UINTN NumBDHandles;
                EFI_HANDLE *BDHandleList;

                Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &NumBDHandles, &BDHandleList);
                if (EFI_ERROR(Status))
                {
                    AsciiSPrint(gBuffer, BUF_SIZE, "Could not get list of handles that support Block I/O protocol: %r", Status);
                    MtSupportDebugWriteLine(gBuffer);
                }

                AsciiSPrint(gBuffer, BUF_SIZE, "Found %d handles that supported Block I/O protocol", NumBDHandles);
                MtSupportDebugWriteLine(gBuffer);

                for (i = 0; i < (int)NumBDHandles; i++)
                {
                    EFI_DEVICE_PATH_PROTOCOL *DevicePath;

                    Status = gBS->HandleProtocol(
                        BDHandleList[i],
                        &gEfiDevicePathProtocolGuid,
                        (VOID **)&DevicePath);

                    if (Status == EFI_SUCCESS)
                    {
                        EFI_DEVICE_PATH_PROTOCOL *DevPathNode = DevicePath;
                        while (!IsDevicePathEnd(DevPathNode))
                        {
                            if (DevPathNode->Type == MEDIA_DEVICE_PATH &&
                                DevPathNode->SubType == MEDIA_HARDDRIVE_DP)
                            {
                                AsciiSPrint(gBuffer, BUF_SIZE, "HD node found: MBR type(%x), Signature type(%x), Signature(%g), Part #(%d), Start LBA(%ld), Num Sectors(%ld)",
                                            ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->MBRType,
                                            ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->SignatureType,
                                            ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->Signature,
                                            ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionNumber,
                                            ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionStart,
                                            ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionSize);

                                MtSupportDebugWriteLine(gBuffer);

                                if (((HARDDRIVE_DEVICE_PATH *)DevPathNode)->MBRType == PARTITION_TYPE_GPT &&
                                    ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionNumber == 3 &&
                                    ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionStart == 0x100000 &&
                                    ((HARDDRIVE_DEVICE_PATH *)DevPathNode)->PartitionSize == 1048543 &&
                                    CompareGuid((const GUID *)(((HARDDRIVE_DEVICE_PATH *)DevPathNode)->Signature), &DMA_TEST_PART_GUID))
                                {
                                    MtSupportDebugWriteLine("Found DMA Test Partition");

                                    Status = gBS->HandleProtocol(
                                        BDHandleList[i],
                                        &gEfiBlockIoProtocolGuid,
                                        (VOID **)&DMATestPart);

                                    if (Status == EFI_SUCCESS)
                                    {
                                        EFI_PARTITION_INFO_PROTOCOL *PartInfo;

                                        MtSupportDebugWriteLine("Successfully obtained Block I/O protocol");

                                        Status = gBS->HandleProtocol(
                                            BDHandleList[i],
                                            &gEfiPartitionInfoProtocolGuid,
                                            (VOID **)&PartInfo);

                                        if (Status == EFI_SUCCESS)
                                        {
                                            UINT32 Index;
                                            UINT8 *InfoPtr;

                                            // const EFI_GUID EFI_PART_TYPE_BASIC_DATA_PART_GUID = { 0xEBD0A0A2, 0xB9E5, 0x4433, { 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7 } };

                                            MtSupportDebugWriteLine("Successfully obtained Partition Info protocol");

                                            AsciiFPrint(DEBUG_FILE_HANDLE, "Revision:  0x%x", PartInfo->Revision);
                                            AsciiFPrint(DEBUG_FILE_HANDLE, "Type:      0x%x", PartInfo->Type);
                                            AsciiFPrint(DEBUG_FILE_HANDLE, "System:    0x%x", PartInfo->System);

                                            switch (PartInfo->Type)
                                            {
                                            case PARTITION_TYPE_OTHER:
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "Other - Should be all 0s:");
                                                for (Index = 0, InfoPtr = (UINT8 *)&PartInfo->Info; Index < sizeof(PartInfo->Info); Index++, InfoPtr++)
                                                {
                                                    AsciiFPrint(DEBUG_FILE_HANDLE, "Other - Index:\t %d, Content:\t ", Index);
                                                    AsciiFPrint(DEBUG_FILE_HANDLE, "0x%02x", *InfoPtr);
                                                }
                                                break;

                                            case PARTITION_TYPE_MBR:
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - BootIndicator:  0x%x", PartInfo->Info.Mbr.BootIndicator);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - StartHead:      0x%x", PartInfo->Info.Mbr.StartHead);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - StartSector:    0x%x", PartInfo->Info.Mbr.StartSector);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - StartTrack:     0x%x", PartInfo->Info.Mbr.StartTrack);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - OSIndicator:    0x%x", PartInfo->Info.Mbr.OSIndicator);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - EndHead:        0x%x", PartInfo->Info.Mbr.EndHead);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - EndSector:      0x%x", PartInfo->Info.Mbr.EndSector);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - EndTrack:       0x%x", PartInfo->Info.Mbr.EndTrack);

                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - StartingLBA:    0x%08x", *((UINT32 *)PartInfo->Info.Mbr.StartingLBA));
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "MBR - SizeInLBA:      0x%08x", *((UINT32 *)PartInfo->Info.Mbr.SizeInLBA));
                                                break;

                                            case PARTITION_TYPE_GPT:
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "GPT - PartitionTypeGUID:    %g", &PartInfo->Info.Gpt.PartitionTypeGUID);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "GPT - UniquePartitionGUID:  %g", &PartInfo->Info.Gpt.UniquePartitionGUID);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "GPT - StartingLBA:          0x%p", PartInfo->Info.Gpt.StartingLBA);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "GPT - EndingLBA:            0x%p", PartInfo->Info.Gpt.EndingLBA);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "GPT - Attributes:           0x%p", PartInfo->Info.Gpt.Attributes);
                                                AsciiFPrint(DEBUG_FILE_HANDLE, "GPT - PartitionName:        %s", PartInfo->Info.Gpt.PartitionName);
                                                break;

                                            default:
                                                ASSERT(FALSE);
                                                break;
                                            }
                                        }

                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Media ID:      0x%08x", DMATestPart->Media->MediaId);
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Removable:     %s", DMATestPart->Media->RemovableMedia ? L"yes" : L"no");
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Media Present: %s", DMATestPart->Media->MediaPresent ? L"yes" : L"no");
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Logical Part:  %s", DMATestPart->Media->LogicalPartition ? L"yes" : L"no");
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Read-only:     %s", DMATestPart->Media->ReadOnly ? L"yes" : L"no");
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Write cache:   %s", DMATestPart->Media->WriteCaching ? L"yes" : L"no");
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Block size:    %d", DMATestPart->Media->BlockSize);
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Align:         %x", DMATestPart->Media->IoAlign);
                                        AsciiFPrint(DEBUG_FILE_HANDLE, "Last block:    0x%p", DMATestPart->Media->LastBlock);
                                        if (DMATestPart->Revision >= EFI_BLOCK_IO_PROTOCOL_REVISION2)
                                        {
                                            AsciiFPrint(DEBUG_FILE_HANDLE, "Lowest LBA:            0x%p", DMATestPart->Media->LowestAlignedLba);
                                            AsciiFPrint(DEBUG_FILE_HANDLE, "Log blks per phys blk: %d", DMATestPart->Media->LogicalBlocksPerPhysicalBlock);
                                            AsciiFPrint(DEBUG_FILE_HANDLE, "Optimal transfer len:  %d", DMATestPart->Media->OptimalTransferLengthGranularity);
                                        }

                                        if (DMATestPart->Media->ReadOnly)
                                        {
                                            AsciiFPrint(DEBUG_FILE_HANDLE, "DMA test shall be disabled as partition is read-only");
                                            DMATestPart = NULL;
                                        }
                                        break;
                                    }
                                }
                            }

                            //
                            // Next device path node
                            //
                            DevPathNode = NextDevicePathNode(DevPathNode);
                        }
                    }

                    if (DMATestPart != NULL)
                        break;
                }

                if (BDHandleList)
                    FreePool(BDHandleList);
            }
        }
    }

    for (i = 0; i < (int)NumFSHandles; i++)
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSys = NULL;
        EFI_DEVICE_PATH_PROTOCOL *DevicePath = DevicePathFromHandle(FSHandleList[i]);

        CHAR16 *DevicePathStr = ConvertDevicePathToText(DevicePath, TRUE, FALSE);

        if (FSHandleList[i] == SelfLoadedImage->DeviceHandle &&
            StrStr(DevicePathStr, L"VirtualDisk") != NULL)
            IsVirtualDisk = TRUE;

        AsciiFPrint(DEBUG_FILE_HANDLE, "[FS%d]", i);
        AsciiFPrint(DEBUG_FILE_HANDLE, "%s", DevicePathStr);

        FreePool(DevicePathStr);

        Status = gBS->HandleProtocol(
            FSHandleList[i],
            &gEfiSimpleFileSystemProtocolGuid,
            (VOID **)&FileSys);

        if (FileSys)
        {
            EFI_FILE_PROTOCOL *RootFs = NULL;
            Status = FileSys->OpenVolume(FileSys, &RootFs);
            if (RootFs)
            {
                EFI_FILE_SYSTEM_INFO *VolumeInfo = NULL;
                UINTN Size = 0;

                Status = RootFs->GetInfo(RootFs, &gEfiFileSystemInfoGuid, &Size, NULL);
                if (Status == EFI_BUFFER_TOO_SMALL)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "File system info buffer size: %d", Size);
                    //
                    // Get volume information of file system
                    //
                    VolumeInfo = (EFI_FILE_SYSTEM_INFO *)AllocateZeroPool(Size);
                    if (VolumeInfo)
                    {
                        Status = RootFs->GetInfo(RootFs, &gEfiFileSystemInfoGuid, &Size, VolumeInfo);
                        if (Status == EFI_SUCCESS)
                        {
                            UINT64 FreeSpaceMB = DivU64x32(VolumeInfo->FreeSpace, 1048576);
                            UINT64 VolumeMB = DivU64x32(VolumeInfo->VolumeSize, 1048576);
                            UINT64 PctFree = VolumeInfo->VolumeSize > 0 ? DivU64x64Remainder(DivU64x32(VolumeInfo->FreeSpace, 100), VolumeInfo->VolumeSize, NULL) : 0;

                            AsciiFPrint(DEBUG_FILE_HANDLE, "Label: \"%s\", Mode: %s, Free space: %ld MB / %ld MB (%d%%)", VolumeInfo->VolumeLabel, VolumeInfo->ReadOnly ? L"RO" : L"RW", FreeSpaceMB, VolumeMB, PctFree);
                        }
                        else
                            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to get file system info: %r", Status);
                        FreePool(VolumeInfo);
                    }
                }
                else
                    AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to get file system info: %r", Status);

                RootFs->Close(RootFs);
            }
            else
                AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to open volume: %r", Status);
        }
        else
            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to get file system protocol: %r", Status);
    }

    //
    // turn off the watchdog timer
    //
    Status = gBS->SetWatchdogTimer(0, WATCHDOG_CODE, 0, NULL); // The firmware reserves codes 0x0000 to 0xFFFF
    AsciiSPrint(gBuffer, BUF_SIZE, "Disabling watchdog timer (Result: %r)", Status);
    MtSupportDebugWriteLine(gBuffer);

    if (Status != EFI_SUCCESS)
    {
        EFI_INPUT_KEY key;

        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"Error disabling watchdog timer (Error: %r)", Status);

        SetMem(&key, sizeof(key), 0);
        MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                      Status,
                      L"Your system may reset unexpectedly.",
                      NULL);
    }

    if (SelfLoadedImage)
    {
        Status = gBS->HandleProtocol(
            SelfLoadedImage->DeviceHandle,
            &gEfiSimpleNetworkProtocolGuid,
            (VOID **)&SimpleNetwork);
        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Found Simple Network protocol from image device handle");

            AsciiFPrint(DEBUG_FILE_HANDLE, "HW addr size: %d", SimpleNetwork->Mode->HwAddressSize);

            AsciiFPrint(DEBUG_FILE_HANDLE, "CurrentAddress: %02x:%02x:%02x:%02x:%02x:%02x", SimpleNetwork->Mode->CurrentAddress.Addr[0], SimpleNetwork->Mode->CurrentAddress.Addr[1], SimpleNetwork->Mode->CurrentAddress.Addr[2], SimpleNetwork->Mode->CurrentAddress.Addr[3], SimpleNetwork->Mode->CurrentAddress.Addr[4], SimpleNetwork->Mode->CurrentAddress.Addr[5]);

            AsciiFPrint(DEBUG_FILE_HANDLE, "PermanentAddress: %02x:%02x:%02x:%02x:%02x:%02x", SimpleNetwork->Mode->PermanentAddress.Addr[0], SimpleNetwork->Mode->PermanentAddress.Addr[1], SimpleNetwork->Mode->PermanentAddress.Addr[2], SimpleNetwork->Mode->PermanentAddress.Addr[3], SimpleNetwork->Mode->PermanentAddress.Addr[4], SimpleNetwork->Mode->PermanentAddress.Addr[5]);

            CopyMem(&ClientMAC, &SimpleNetwork->Mode->PermanentAddress, sizeof(ClientMAC));
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Could not find Simple Network protocol from image device handle");
        }
    }

    EFI_MAC_ADDRESS ZeroMAC;
    SetMem(&ZeroMAC, sizeof(ZeroMAC), 0);

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleNetworkProtocolGuid, NULL, &NumSNHandles, &SNHandleList);
    if (EFI_ERROR(Status))
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not get list of handles that support Simple Network protocol: %r", Status);
    }

    AsciiFPrint(DEBUG_FILE_HANDLE, "Found %d handles that supported Simple Network", NumSNHandles);

    for (i = 0; i < (int)NumSNHandles; i++)
    {
        EFI_SIMPLE_NETWORK_PROTOCOL *SNProt = NULL;
        EFI_GUID **ProtList = NULL;
        UINTN NumProt = 0;

        AsciiFPrint(DEBUG_FILE_HANDLE, "Handle %d - %p", i, SNHandleList[i]);

        Status = gBS->HandleProtocol(
            SNHandleList[i],
            &gEfiSimpleNetworkProtocolGuid,
            (VOID **)&SNProt);
        if (Status == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] State: %d", i, SNProt->Mode->State);
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] HW addr size: %d", i, SNProt->Mode->HwAddressSize);
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] CurrentAddress: %02x:%02x:%02x:%02x:%02x:%02x", i, SNProt->Mode->CurrentAddress.Addr[0], SNProt->Mode->CurrentAddress.Addr[1], SNProt->Mode->CurrentAddress.Addr[2], SNProt->Mode->CurrentAddress.Addr[3], SNProt->Mode->CurrentAddress.Addr[4], SNProt->Mode->CurrentAddress.Addr[5]);
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] PermanentAddress: %02x:%02x:%02x:%02x:%02x:%02x", i, SNProt->Mode->PermanentAddress.Addr[0], SNProt->Mode->PermanentAddress.Addr[1], SNProt->Mode->PermanentAddress.Addr[2], SNProt->Mode->PermanentAddress.Addr[3], SNProt->Mode->PermanentAddress.Addr[4], SNProt->Mode->PermanentAddress.Addr[5]);
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] IfType: %d", i, SNProt->Mode->IfType);
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] MacAddressChangeable: %d", i, SNProt->Mode->MacAddressChangeable);
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] MediaPresentSupported: %d", i, SNProt->Mode->MediaPresentSupported);
            AsciiFPrint(DEBUG_FILE_HANDLE, "[%d] MediaPresent: %d", i, SNProt->Mode->MediaPresent);

            // If we haven't found a MAC address, get the first available
            if (CompareMem(&ZeroMAC, &ClientMAC, sizeof(ClientMAC)) == 0)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Found first handle supporting Simple Network protocol");

                AsciiFPrint(DEBUG_FILE_HANDLE, "HW addr size: %d", SNProt->Mode->HwAddressSize);

                AsciiFPrint(DEBUG_FILE_HANDLE, "CurrentAddress: %02x:%02x:%02x:%02x:%02x:%02x", SNProt->Mode->CurrentAddress.Addr[0], SNProt->Mode->CurrentAddress.Addr[1], SNProt->Mode->CurrentAddress.Addr[2], SNProt->Mode->CurrentAddress.Addr[3], SNProt->Mode->CurrentAddress.Addr[4], SNProt->Mode->CurrentAddress.Addr[5]);

                AsciiFPrint(DEBUG_FILE_HANDLE, "PermanentAddress: %02x:%02x:%02x:%02x:%02x:%02x", SNProt->Mode->PermanentAddress.Addr[0], SNProt->Mode->PermanentAddress.Addr[1], SNProt->Mode->PermanentAddress.Addr[2], SNProt->Mode->PermanentAddress.Addr[3], SNProt->Mode->PermanentAddress.Addr[4], SNProt->Mode->PermanentAddress.Addr[5]);

                CopyMem(&ClientMAC, &SNProt->Mode->PermanentAddress, sizeof(ClientMAC));
            }
        }

        Status = gBS->ProtocolsPerHandle(SNHandleList[i], &ProtList, &NumProt);
        if (Status == EFI_SUCCESS)
        {
            for (UINTN j = 0; j < NumProt; j++)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "{%d} %g", j, ProtList[j]);
            }
            FreePool(ProtList);
        }
    }

    if (SNHandleList)
        FreePool(SNHandleList);

#ifdef SITE_EDITION
    CheckTcp4Support();
#endif

    MtSupportDebugWriteLine("=============================================");

    // Log MemTest86 version
    AsciiSPrint(gBuffer, BUF_SIZE, "%s %s Build: %s (%d-bit)", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD, sizeof(void *) * 8);
    MtSupportDebugWriteLine(gBuffer);
    MtSupportDebugWriteLine("=============================================");

    if (IsVirtualDisk)
        MtSupportDebugWriteLine("Running from virtual disk");

    // Get BIOS info from SMBIOS
    GetBiosInformationStruct(NULL, 0, &gBIOSInfo);

    AsciiSPrint(gBuffer, BUF_SIZE, "SMBIOS BIOS INFO Vendor: \"%a\", Version: \"%a\", Release Date: \"%a\"",
                gBIOSInfo.szVendor,
                gBIOSInfo.szBIOSVersion,
                gBIOSInfo.szBIOSReleaseDate);
    MtSupportDebugWriteLine(gBuffer);

    // Get system info from SMBIOS
    GetSystemInformationStruct(NULL, 0, &gSystemInfo);

    AsciiSPrint(gBuffer, BUF_SIZE, "SMBIOS SYSTEM INFO Manufacturer: \"%a\", Product: \"%a\", Version: \"%a\", S/N: \"%a\", SKU: \"%a\", Family: \"%a\"",
                gSystemInfo.szManufacturer,
                gSystemInfo.szProductName,
                gSystemInfo.szVersion,
                gSystemInfo.szSerialNumber,
                gSystemInfo.szSKUNumber,
                gSystemInfo.szFamily);
    MtSupportDebugWriteLine(gBuffer);

    // Get baseboard info from SMBIOS
    GetBaseboardInformationStruct(NULL, 0, &gBaseBoardInfo);

    AsciiSPrint(gBuffer, BUF_SIZE, "SMBIOS BASEBOARD INFO Manufacturer: \"%a\", Product: \"%a\", Version: \"%a\", S/N: \"%a\", AssetTag: \"%a\", LocationInChassis: \"%a\"",
                gBaseBoardInfo.szManufacturer,
                gBaseBoardInfo.szProductName,
                gBaseBoardInfo.szVersion,
                gBaseBoardInfo.szSerialNumber,
                gBaseBoardInfo.szAssetTag,
                gBaseBoardInfo.szLocationInChassis);
    MtSupportDebugWriteLine(gBuffer);

    AsciiSPrint(gBuffer, BUF_SIZE, "EFI Specifications: %d.%02d",
                (gRT->Hdr.Revision >> 16) & 0x0000FFFF,
                gRT->Hdr.Revision & 0x0000FFFF);
    MtSupportDebugWriteLine(gBuffer);

    // If system is Apple or LENOVO, need to use deprecated Console Control protocol to switch between console/graphics mode
    // gConCtrlWorkaround = AsciiStrStr(gBaseBoardInfo.szManufacturer,"Apple") != NULL || AsciiStrStr(gBaseBoardInfo.szManufacturer,"LENOVO") != NULL;

    // Read blacklist.cfg file
    SetMem(&Blacklist, sizeof(Blacklist), 0);
    MtSupportOpenFile(&BLFileHandle, BLACKLIST_FILENAME, EFI_FILE_MODE_READ, 0);
    if (BLFileHandle)
    {
        MtSupportDebugWriteLine("Found blacklist file");
        MtSupportReadBlacklist(BLFileHandle, &Blacklist);
        MtSupportCloseFile(BLFileHandle);
    }

    // Detect if baseboard is in the blacklist
    for (i = 0; i < Blacklist.iBlacklistLen; i++)
    {
        BOOLEAN bBaseboardMatch = FALSE;
        BOOLEAN bBIOSMatch = FALSE;

        if (Blacklist.pBlacklist[i].PartialMatch)
            bBaseboardMatch = AsciiStrnCmp(gBaseBoardInfo.szProductName, Blacklist.pBlacklist[i].szBaseboardName, AsciiStrLen(Blacklist.pBlacklist[i].szBaseboardName)) == 0;
        else
            bBaseboardMatch = AsciiStriCmp(gBaseBoardInfo.szProductName, Blacklist.pBlacklist[i].szBaseboardName) == 0;

        bBIOSMatch = Blacklist.pBlacklist[i].szBIOSVersion[0] == L'\0' || AsciiStrCmp(gBIOSInfo.szBIOSVersion, Blacklist.pBlacklist[i].szBIOSVersion) < 0;

        if (bBaseboardMatch && bBIOSMatch)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Detected blacklisted baseboard (Product Name: \"%a\", BIOS version: \"%a\")", gBaseBoardInfo.szProductName, gBIOSInfo.szBIOSVersion);
            MtSupportDebugWriteLine(gBuffer);

            gRestrictFlags = Blacklist.pBlacklist[i].RestrictFlags;

            if (gRestrictFlags & RESTRICT_FLAG_STARTUP)
            {
                EFI_INPUT_KEY key;

                MtSupportDebugWriteLine("MemTest86 has detected that an older baseboard firmware is being used and may experience known issues such as missing graphics");

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                              L"**WARNING** MemTest86 has detected an older baseboard firmware that has   ",
                              L"            known issues that prevent MemTest86 from functioning properly.",
                              L"                                                                          ",
                              L" If you wish to continue, press (y). Otherwise, press any key to restart  ",
                              NULL);
                if (key.UnicodeChar != 'y')
                {
                    gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
                    return EFI_SUCCESS;
                }
                gST->ConOut->ClearScreen(gST->ConOut);
            }

            if (gRestrictFlags & RESTRICT_FLAG_MP)
            {
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that has known multiprocessor issues. The MP test shall not be run and the default mode shall be set to single CPU");
            }

            if (gRestrictFlags & RESTRICT_FLAG_MPDISABLE)
            {
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that has known multiprocessor issues. Multiprocessor support shall be completely disabled");
            }

            if (gRestrictFlags & RESTRICT_FLAG_CONCTRLDISABLE)
            {
                gConCtrlWorkaround = FALSE;
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that requires console control to be disabled");
            }

            if (gRestrictFlags & RESTRICT_FLAG_FIXED_SCREENRES)
            {
                gFixedScreenRes = TRUE;
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that requires the screen resolution to be fixed");
            }

            if (gRestrictFlags & RESTRICT_FLAG_RESTRICT_ADDR)
            {
                minaddr = RESTRICT_ADDR_LOW_LIMIT;
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that requires the lower address limit to be set");
            }

            if (gRestrictFlags & RESTRICT_FLAG_TEST12_SINGLECPU)
            {
                gTest12SingleCPU = TRUE;
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that requires Test 12 to run in SINGLE CPU Mode");
            }

            if (gRestrictFlags & RESTRICT_FLAG_LANGDISABLE)
            {
                gDisableFontInstall = TRUE;
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that requires language support and font installation to be disabled");
            }

            if (gRestrictFlags & RESTRICT_FLAG_CPUINFO)
            {
                gDisableCPUInfo = TRUE;
                MtSupportDebugWriteLine("MemTest86 has detected a baseboard that requires CPU info collection to be disabled");
            }

            break;
        }
    }

    // We don't need the blacklist anymore so let's free it from memory
    if (Blacklist.iBlacklistSize > 0)
    {
        FreePool(Blacklist.pBlacklist);
        SetMem(&Blacklist, sizeof(Blacklist), 0);
    }

    if (gConCtrlWorkaround)
        MtSupportDebugWriteLine("Console Control protocol workaround enabled");
    else
        MtSupportDebugWriteLine("Console Control protocol workaround disabled");

    AsciiSPrint(gBuffer, BUF_SIZE, "Number of console modes: %d", SavedConsoleMode.MaxMode);
    MtSupportDebugWriteLine(gBuffer);

    for (i = 0; i < SavedConsoleMode.MaxMode; i++)
    {
        UINTN Columns = 0, Rows = 0;
        gST->ConOut->QueryMode(gST->ConOut, (UINTN)i, &Columns, &Rows);

        AsciiSPrint(gBuffer, BUF_SIZE, "Mode %d: %d x %d", i, Columns, Rows);
        MtSupportDebugWriteLine(gBuffer);
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "Console attribute: %d", SavedConsoleMode.Attribute);
    MtSupportDebugWriteLine(gBuffer);

    MtSupportDebugWriteLine("Initializing localization strings");
    InitLocalizationStrings();

    if (gDisableFontInstall == FALSE)
    {
        if (!CheckFontLanguageSupport())
        {
            // Intialize font
            MtSupportDebugWriteLine("Font does not support all languages. Installing unicode font...");
            Status = InitFont(UNICODE_FONT_FILENAME);
            if (Status != EFI_SUCCESS)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "Could not install font (%r). Some languages may not be available", Status);
                MtSupportDebugWriteLine(gBuffer);
            }
            CheckFontLanguageSupport();
        }
    }

    // Check iMS support
    if (iMSIsAvailable())
    {
        IMS_TO_BIOS_HOB_ST iMSToBiosHob;

        UINT8 SocketId = 0xff;
        UINT8 MemoryControllerId = 0xff;
        UINT8 ChannelId = 0xff;
        UINT8 DimmSlot = 0xff;
        UINT8 DimmRank = 0xff;

        MtSupportDebugWriteLine("iMS support is available");
        MtSupportPrintIMSDefectList(DEBUG_FILE_HANDLE);

        gRunFromBIOS = iMSIsRunFromBios();

        AsciiFPrint(DEBUG_FILE_HANDLE, "Running MemTest86 in iMS firmware: %s", gRunFromBIOS ? L"yes" : L"no");

        if (iMSGetVersionFromHob(&iMSToBiosHob) == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "  IMS Version: %d.%d (%d [%s])",
                        (iMSToBiosHob.iMSVersion >> 8) & 0xFF, // iMS version high
                        (iMSToBiosHob.iMSVersion) & 0xFF,
                        iMSToBiosHob.iMSFreeVersion,
                        iMSToBiosHob.iMSFreeVersion ? L"Free" : L"Paid");

            AsciiFPrint(DEBUG_FILE_HANDLE, "  Module Build Date: %4x/%02x/%02x",
                        (iMSToBiosHob.iMSModuleBuildDate >> 16) & 0xFFFF, // year
                        (iMSToBiosHob.iMSModuleBuildDate >> 8) & 0xFF,    // month
                        (iMSToBiosHob.iMSModuleBuildDate) & 0xFF);        // day

            AsciiFPrint(DEBUG_FILE_HANDLE, "  DIMM ID Err: %d",
                        iMSToBiosHob.DimmIdErr);

            AsciiFPrint(DEBUG_FILE_HANDLE, "  BIOS Area Err: %d",
                        iMSToBiosHob.BiosAreaErr);

            AsciiFPrint(DEBUG_FILE_HANDLE, "  Mem Err Full: %d",
                        iMSToBiosHob.MemErrFull);
        }
        else
            MtSupportDebugWriteLine("Error getting IMS Version from HOB");

        if ((Status = iMSSystemAddressToDimmAddress(0, &SocketId, &MemoryControllerId, &ChannelId, &DimmSlot, &DimmRank)) == EFI_SUCCESS)
            AsciiFPrint(DEBUG_FILE_HANDLE, "iMSSystemAddressToDimmAddress is supported: 0x0 -> {Socket: %d, Ctrl: %d, ChId: %d, DimmSlot: %d, DimmRank: %d}", SocketId, MemoryControllerId, ChannelId, DimmSlot, DimmRank);
        else
            AsciiFPrint(DEBUG_FILE_HANDLE, "iMSSystemAddressToDimmAddress is not supported: %r", Status);
    }
    else
    {
        MtSupportDebugWriteLine("iMS support is NOT available");
#if 0
        Status = MtSupportCreateIMSDefectList();
        AsciiSPrint(gBuffer, BUF_SIZE, "Create IMS defect list returned: %r", Status);
        MtSupportDebugWriteLine(gBuffer);
#endif
    }

    // initialize libeg
    MtSupportDebugWriteLine("Initializing screen for graphics");
    egInitScreen();

    // Get previous graphics mode
    PrevGraphicsEn = gConCtrlWorkaround ? egIsGraphicsModeEnabled() : FALSE;

    MaxGraphicsMode = egGetNumModes();
    AsciiSPrint(gBuffer, BUF_SIZE, "Number of graphics modes: %d", MaxGraphicsMode);
    MtSupportDebugWriteLine(gBuffer);

    // Determine screen mode, width and height
    egGetScreenSize(&ScreenWidth, &ScreenHeight);

    for (i = 0; i < (int)MaxGraphicsMode; i++)
    {
        UINTN Width = (UINTN)i;
        UINTN Height = 0;
        egGetResFromMode(&Width, &Height);

        if (Width == ScreenWidth && Height == ScreenHeight)
        {
            ScreenMode = i;
            AsciiSPrint(gBuffer, BUF_SIZE, "Mode %d: %d x %d [Current]", i, Width, Height);
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Mode %d: %d x %d", i, Width, Height);
        }
        MtSupportDebugWriteLine(gBuffer);
    }

    // Retrieve HW details
    Print(GetStringById(STRING_TOKEN(STR_INIT_HW), TempBuf, sizeof(TempBuf)));
    init();

#ifndef TRIAL
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    gSIMDSupported = cpu_id.fid.bits.sse2 != 0;
    gNTLoadSupported = cpu_id.fid.bits.sse4_1 != 0;
#elif defined(MDE_CPU_AARCH64)
    gSIMDSupported = TRUE;
    gNTLoadSupported = TRUE;
#endif
#ifdef PRO_RELEASE
#if 0 // SSE is no longer enabled during startup
    else
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Enabling SSE instructions (Cr0=0x%08X, Cr4=0x%08X)", AsmReadCr0(), AsmReadCr4());
        MtSupportDebugWriteLine(gBuffer);

        // Enable SSE for all processors
        {
            EFI_EVENT           APEvent[MAX_CPUS];
            UINTN               TotalProcs = MPSupportGetNumProcessors();
            UINTN               TotalEnabledProcs = 0;
            BOOLEAN             bAPFinished = TRUE;
            UINTN               ProcNum;
            int                 RetryCount = 0;

            SetMem(APEvent, sizeof(APEvent), 0);
            for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
            {
                if (MPSupportIsProcEnabled(ProcNum))
                    TotalEnabledProcs++;
            }

            AsciiSPrint(gBuffer, BUF_SIZE, "Enabling SSE instructions for %d processors", TotalEnabledProcs);
            MtSupportDebugWriteLine(gBuffer);

            for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
            {
                if (ProcNum != MPSupportGetBspId() &&
                    MPSupportIsProcEnabled(ProcNum))
                {
                    EFI_STATUS Status;
                    Status = MPSupportEnableSSEAP(ProcNum, &APEvent[ProcNum]);
                    if (EFI_ERROR(Status))
                    {
                        AsciiSPrint(gBuffer, BUF_SIZE, "Could not enable SSE instructions on AP#%d (%r). Resetting...", ProcNum, Status);
                        MtSupportDebugWriteLine(gBuffer);

                        // Try disable/enabling
                        MPSupportResetProc(ProcNum);

                        Status = MPSupportEnableSSEAP(ProcNum, &APEvent[ProcNum]);
                        if (EFI_ERROR(Status))
                        {
                            AsciiSPrint(gBuffer, BUF_SIZE, "Unable to enable SSE instructions on AP#%d (%r)", ProcNum, Status);
                            MtSupportDebugWriteLine(gBuffer);
                        }

                    }
                }
            }

            MtSupportDebugWriteLine("Enabling SSE instructions on BSP");

            EnableSSE();

            AsciiSPrint(gBuffer, BUF_SIZE, "Waiting to finish enabling SSE instructions for %d processors", TotalEnabledProcs - 1);
            MtSupportDebugWriteLine(gBuffer);
            do {
                bAPFinished = TRUE;

                for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
                {
                    if (ProcNum != MPSupportGetBspId())
                    {
                        if (APEvent[ProcNum])
                        {
                            EFI_STATUS Status;
                            Status = gBS->CheckEvent(APEvent[ProcNum]);

                            if (Status == EFI_NOT_READY) // CPU finished event has not been signalled
                            {
                                bAPFinished = FALSE;
                            }
                            else if (Status == EFI_SUCCESS) // CPU finished event has been signalled
                            {
                                gBS->CloseEvent(APEvent[ProcNum]);
                                APEvent[ProcNum] = NULL;
                            }
                        }
                    }
                }
                RetryCount++;
            } while (bAPFinished == FALSE && RetryCount < 10);

            // Clean up events
            for (ProcNum = 0; ProcNum < TotalProcs; ProcNum++)
            {
                if (APEvent[ProcNum])
                {
                    AsciiSPrint(gBuffer, BUF_SIZE, "Proc %d has not finished enabling SSE instructions", ProcNum);
                    MtSupportDebugWriteLine(gBuffer);

                    gBS->CloseEvent(APEvent[ProcNum]);
                    APEvent[ProcNum] = NULL;
                }
            }
    }
    }
#endif
#endif
#endif

    NumConfig = MAX_NUM_CONFIG;
    MTConfig = (MTCFG *)AllocateZeroPool(sizeof(*MTConfig) * NumConfig);
    if (MTConfig == NULL)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to allocate memory for configuration parameters");
        NumConfig = 0;
    }

    MtSupportSetDefaultConfig();

    // Read configuration file
    // 1) Look for <SMBIOS-baseboard-product>-mt86.cfg
    // 2) Look for <Memory-size-in-GB>GB-mt86.cfg
    // 3) Look for mt86.cfg
#ifdef PRO_RELEASE
    UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%a-%s", gBaseBoardInfo.szProductName, CONFIG_FILENAME);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Attempting to open configuration file: %s", TempBuf);
    MtSupportOpenFile(&CfgFileHandle, TempBuf, EFI_FILE_MODE_READ, 0);
    if (CfgFileHandle == NULL)
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%ldGB-%s", MtSupportMemSizeGB(), CONFIG_FILENAME);

        AsciiFPrint(DEBUG_FILE_HANDLE, "Attempting to open configuration file: %s", TempBuf);
        MtSupportOpenFile(&CfgFileHandle, TempBuf, EFI_FILE_MODE_READ, 0);
        if (CfgFileHandle == NULL)
        {
            StrCpyS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), CONFIG_FILENAME);
            AsciiFPrint(DEBUG_FILE_HANDLE, "Attempting to open configuration file: %s", TempBuf);
            MtSupportOpenFile(&CfgFileHandle, TempBuf, EFI_FILE_MODE_READ, 0);
        }
    }
#endif
    if (CfgFileHandle)
    {
        StrCpyS(gConfigFile, sizeof(gConfigFile) / sizeof(gConfigFile[0]), TempBuf);
        AsciiFPrint(DEBUG_FILE_HANDLE, "Found configuration file: %s", gConfigFile);
        if ((Status = MtSupportReadConfig(CfgFileHandle, MTConfig, &NumConfig, &DefaultCfgIdx, &ConfigTimeout)) == EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Successfully read configuration file");
        }
        else
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Error reading configuration file: %r", Status);
            StrCpyS(gConfigFile, sizeof(gConfigFile) / sizeof(gConfigFile[0]), CONFIG_FILENAME);
            CfgIdx = 0;
            NumConfig = 1;
            MtSupportInitConfig(&MTConfig[CfgIdx]); // Set configuration to default
        }

        MtSupportCloseFile(CfgFileHandle);
    }
    else
    {
        StrCpyS(gConfigFile, sizeof(gConfigFile) / sizeof(gConfigFile[0]), CONFIG_FILENAME);
        CfgIdx = 0;
        NumConfig = 1;
        MtSupportInitConfig(&MTConfig[CfgIdx]); // Set configuration to default
    }

    if (NumConfig < MAX_NUM_CONFIG)
    {
        UINTN OldSize = sizeof(*MTConfig) * MAX_NUM_CONFIG;
        UINTN NewSize = sizeof(*MTConfig) * NumConfig;
        // Reduce the size of buffer
        MTCFG *NewMTConfig = ReallocatePool(OldSize, NewSize, MTConfig);
        if (NewMTConfig != NULL)
        {
            MTConfig = NewMTConfig;
        }
    }

    // If multiple configurations, prompt user to select one
    if (NumConfig > 1)
    {
        Print(L"\n");
        Print(GetStringById(STRING_TOKEN(STR_SELECT_CONFIG), TempBuf, sizeof(TempBuf)));
        for (i = 0; i < (int)NumConfig; i++)
            Print(L"%c %d) %s\n", i == DefaultCfgIdx ? L'*' : L' ', i + 1, MTConfig[i].Name);

        Print(L"\n");
        UINTN Row = gST->ConOut->Mode->CursorRow;
        Print(GetStringById(STRING_TOKEN(STR_DEFAULT_CONFIG), TempBuf, sizeof(TempBuf)));
        Print(L"\n");

        Print(L"\n> ");

        UINT64 StartTime = MtSupportReadTSC();

        CfgIdx = DefaultCfgIdx;
        while (TRUE)
        {

            if (ConfigTimeout > 0)
            {
                // Print timeout countdown
                UINT64 ConfigTimeoutMs = ConfigTimeout * 1000;
                UINT64 ElapsedTimeMs = DivU64x64Remainder(MtSupportReadTSC() - StartTime, clks_msec, NULL);

                ConsolePrintXY(0, Row, GetStringById(STRING_TOKEN(STR_DEFAULT_CONFIG), TempBuf, sizeof(TempBuf)));
                Print(GetStringById(STRING_TOKEN(STR_CONFIG_COUNTDOWN), TempBuf, sizeof(TempBuf)), DivU64x32(ConfigTimeoutMs - ElapsedTimeMs, 1000), ModU64x32(ConfigTimeoutMs - ElapsedTimeMs, 1000) / 100);
                int NumSpaces = (int)MaxUIWidth - 2 - gST->ConOut->Mode->CursorColumn;
                if (NumSpaces > 0)
                {
                    CHAR16 TextBuf[128];
                    SetMem16(TextBuf, NumSpaces * sizeof(TextBuf[0]), L' ');
                    TextBuf[NumSpaces] = L'\0';
                    Print(TextBuf);
                }
                Print(L"\n");

                // Check key press
                EFI_INPUT_KEY Key;
                SetMem(&Key, sizeof(Key), 0);

                if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
                {
                    if (Key.UnicodeChar >= L'1' && Key.UnicodeChar < '1' + NumConfig)
                    {
                        CfgIdx = Key.UnicodeChar - L'1';
                        Print(L"\n> %d\n", CfgIdx + 1);
                        break;
                    }
                }

                // Check timeout
                if (ElapsedTimeMs > ConfigTimeoutMs)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "Configuration selection timer expired");
                    break;
                }

                gBS->Stall(1000);
            }
            else
            {
                CHAR16 Key = MtUiWaitForKey(0, NULL);
                if (Key >= L'1' && Key < '1' + NumConfig)
                {
                    CfgIdx = Key - L'1';
                    Print(L"%d\n", CfgIdx + 1);
                    break;
                }
            }
        }

        MtSupportSaveLastConfigSelected(CfgIdx + 1);
    }

    if (MTConfig[CfgIdx].Name[0] != L'\0')
        UnicodeSPrint(gConfigFile, sizeof(gConfigFile), L"%s.cfg", MTConfig[CfgIdx].Name);

    MtSupportParseCommandArgs((CHAR16 *)SelfLoadedImage->LoadOptions, SelfLoadedImage->LoadOptionsSize, &MTConfig[CfgIdx]);

    if (MTConfig[CfgIdx].TPL != gTPL)
    {
        OldTPL = gBS->RaiseTPL(MTConfig[CfgIdx].TPL);
    }

    // Initialize Multiprocessor support library
    if (MTConfig[CfgIdx].DisableMP == FALSE && (gRestrictFlags & RESTRICT_FLAG_MPDISABLE) == 0) // If 'DISABLEMP' config file parameter specified or the board is blacklisted, don't initialize MPSupport
        gMPSupported = MPSupportInit(MTConfig[CfgIdx].MaxCPUs) == EFI_SUCCESS;

    if (gMPSupported && (gRestrictFlags & RESTRICT_FLAG_MP) == 0) // Run the MP test only if MP is supported and the baseboard is not blacklisted
    {
        Print(GetStringById(STRING_TOKEN(STR_INIT_TESTMP), TempBuf, sizeof(TempBuf)));
        MtSupportDebugWriteLine("Testing MP support");
        if (MPSupportTestMPServices())
        {
            MtSupportDebugWriteLine("MP test passed. Setting default CPU mode to PARALLEL");
            if (MTConfig[CfgIdx].CPUSelMode == CPU_DEFAULT) // If no CPU mode is specified in the config file, set default mode to Parallel
                MTConfig[CfgIdx].CPUSelMode = CPU_PARALLEL;
        }
        else
        {
            MtSupportDebugWriteLine("MP test failed. Setting default CPU mode to SINGLE");
            if (MTConfig[CfgIdx].CPUSelMode == CPU_DEFAULT) // If no CPU mode is specified in the config file, set default mode to Single
                MTConfig[CfgIdx].CPUSelMode = CPU_SINGLE;
        }
    }
    else
    {
        if (MTConfig[CfgIdx].CPUSelMode == CPU_DEFAULT) // If no CPU mode is specified in the config file, set default mode to Single
            MTConfig[CfgIdx].CPUSelMode = CPU_SINGLE;
    }

    Print(GetStringById(STRING_TOKEN(STR_INIT_MEMCTRL), TempBuf, sizeof(TempBuf)));
    MtSupportDebugWriteLine("Getting memory controller info");
    find_mem_controller();

    Print(GetStringById(STRING_TOKEN(STR_INIT_CONFIG), TempBuf, sizeof(TempBuf)));

    MtSupportSetDefaultTestConfig(&MTConfig[CfgIdx], &gCustomTestList, &gNumCustomTests);
    if (MTConfig[CfgIdx].TestCfgFile[0] != L'\0')
    {
        MT_HANDLE FileHandle = NULL;
        MtSupportOpenFile(&FileHandle, MTConfig[CfgIdx].TestCfgFile, EFI_FILE_MODE_READ, 0);
        if (FileHandle)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Found test config file: %s", MTConfig[CfgIdx].TestCfgFile);
            gUsingCustomTests = MtSupportReadTestConfig(FileHandle, &gCustomTestList, &gNumCustomTests) == EFI_SUCCESS;
            MtSupportCloseFile(FileHandle);
        }
    }

    MtSupportDebugWriteLine("Applying configurations");
    MtSupportApplyConfig(&MTConfig[CfgIdx]);
    MtSupportDebugWriteLine("Applying configurations complete");

    if (MTConfig)
        FreePool(MTConfig);
    MTConfig = NULL;

    Print(GetStringById(STRING_TOKEN(STR_INIT_ANY), TempBuf, sizeof(TempBuf)), L"SPD");
    MtSupportDebugWriteLine("Getting memory SPD info");
    mem_spd_info();

    Print(GetStringById(STRING_TOKEN(STR_INIT_ANY), TempBuf, sizeof(TempBuf)), L"TSOD");
    MtSupportDebugWriteLine("Getting memory TSOD info");
    mem_temp_info();

    Print(GetStringById(STRING_TOKEN(STR_INIT_ANY), TempBuf, sizeof(TempBuf)), L"SMBIOS");
    MtSupportDebugWriteLine("Getting memory SMBIOS info");
    smbios_mem_info();

    map_spd_to_smbios();

#ifdef SITE_EDITION
    Print(GetStringById(STRING_TOKEN(STR_INIT_PXE), TempBuf, sizeof(TempBuf)));

    // Sync RTC with PXE server
    if (gRTCSync)
    {
        MT_HANDLE FileHandle = NULL;

        MtSupportOpenFile(&FileHandle, TIMEOFDAY_FILENAME, EFI_FILE_MODE_READ, 0);
        if (FileHandle)
        {
            EFI_TIME NowTime;
            SetMem(&NowTime, sizeof(NowTime), 0);
            if (MtSupportReadTimeOfDay(FileHandle, &NowTime))
                gRT->SetTime(&NowTime);

            MtSupportCloseFile(FileHandle);
        }
    }
#endif

    // Connect to managment console
    MtSupportPMPConnect();
    gAesContext = AllocatePool(AesGetContextSize());
    AesInit(gAesContext, Aes128CbcKey, 128);
    EFI_UDP4_CONFIG_DATA RxConfigData = {
                          TRUE,   // AcceptBroadcast
                          FALSE,  // AcceptPromiscuous
                          FALSE,  // AcceptAnyPort
                          TRUE,   // AllowDuplicatePort
                          0,      // TypeOfService
                          16,     // TimeToLive
                          TRUE,   // DoNotFragment
                          0,      // ReceiveTimeout
                          0,      // TransmitTimeout
                          TRUE,   // UseDefaultAddress
                          {{0, 0, 0, 0}},  // StationAddress
                          {{0, 0, 0, 0}},  // SubnetMask
                          5566,            // StationPort
                          {{0, 0, 0, 0}},  // RemoteAddress
                          0,               // RemotePort
     };
 
    // Listen on all NICs. The first NIC that receives a packet becomes the bound interface.
	DumpNetProtoCounts();
    gBS->Stall(4000 * 1000); // 1s, 防止一闪而过
    ConnectAllSnpControllers();
    DumpNetProtoCounts();
    gBS->Stall(4000 * 1000); // 1s

    Status = StartUdp4ReceiveOnAllNics(&RxConfigData);
	Print(L"[UDP] StartUdp4ReceiveOnAllNics: %r, RxSockCnt=%u\n", Status, gOknUdpRxSocketCount);
    gBS->Stall(4 * 1000 * 1000);
    if (false == EFI_ERROR(Status)) {
		gOKnSkipWaiting = FALSE;
		MtSupportDebugWriteLine("Waiting for UDP NIC binding (first packet)...");
		Print(L"Waiting for UDP NIC binding (first packet)...\n");
		(VOID)WaitForUdpNicBind(0);
    } 
	else {
		// 保持原始语义：UDP 不可用才 skip waiting
		// Preserve original semantics: only skip waiting when UDP init fails.
		gOKnSkipWaiting = TRUE;
    }

    // TX socket will be created lazily in Udp4ReceiveHandler() after NIC binding.
    gOknUdpSocketTransmit = NULL;

    if (gOKnSkipWaiting)
        gAutoMode = TRUE;

    Print(GetStringById(STRING_TOKEN(STR_INIT_DISPLAY), TempBuf, sizeof(TempBuf)));

    AsciiSPrint(gBuffer, BUF_SIZE, "Console size = %d x %d", ConWidth, ConHeight);
    MtSupportDebugWriteLine(gBuffer);

    gST->ConOut->ClearScreen(gST->ConOut);

    MtSupportDebugWriteLine("Checking for graphics mode support");

    if (!egHasGraphicsMode())
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Warning - Graphics mode not supported. Setting to Console mode.");
        MtSupportDebugWriteLine(gBuffer);

        gConsoleOnly = TRUE;
    }
    else
    {
        MtSupportDebugWriteLine("Graphics mode available");
    }

    if (gAutoMode) // If AUTOMODE is set, don't show splash screen and start immediately
        Select = ID_BUTTON_START;

    if (gConsoleOnly == FALSE)
    {
        EnableGraphicsMode(TRUE);
        AutoSetScreenRes();

        AsciiSPrint(gBuffer, BUF_SIZE, "Screen size = %d x %d", ScreenWidth, ScreenHeight);
        MtSupportDebugWriteLine(gBuffer);

        GetTextInfo(L"A", &gCharWidth, &gTextHeight);
        AsciiSPrint(gBuffer, BUF_SIZE, "Char width=%d height=%d", gCharWidth, gTextHeight);
        MtSupportDebugWriteLine(gBuffer);

        if (gTextHeight < EFI_GLYPH_HEIGHT)
            gTextHeight = EFI_GLYPH_HEIGHT;
        if (gCharWidth < EFI_GLYPH_WIDTH)
            gTextHeight = EFI_GLYPH_WIDTH;

        gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(SavedConsoleMode.Attribute & 0x0F, gBgColour & 0x07));

        MtSupportDebugWriteLine("Loading images");

        HeaderLogo = egDecodePNG(mt86text_png_bin, sizeof(mt86text_png_bin), 128, TRUE);
        if (HeaderLogo == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load MemTest86 header image");
            MtSupportDebugWriteLine(gBuffer);
        }

        for (i = 0; i < NUM_SIDEBAR_BUTTONS; i++)
        {
            SidebarButtons[i].Id = (UINT16)i;
            SidebarButtons[i].Caption = STRING_TOKEN(SIDEBAR_BTN_CAPTION_STR_ID[i]);
            SidebarButtons[i].Image = egDecodePNG(SIDEBAR_BTN_BIN[i], SIDEBAR_BTN_BIN_SIZE[i], 128, TRUE);
            if (SidebarButtons[i].Image == NULL)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "Could not load sidebar button %d", i);
                MtSupportDebugWriteLine(gBuffer);

                SidebarButtons[i].ScreenPos.Height = 48;
                SidebarButtons[i].ScreenPos.Width = 48;
                SidebarButtons[i].ScreenPos.XPos = 20; // i * ScreenWidth / NUM_SIDEBAR_BUTTONS + SidebarButtons[i].Image->Width/2;
                SidebarButtons[i].ScreenPos.YPos = i * (SidebarButtons[i].ScreenPos.Height + 20) + 80;
            }
            else
            {
                SidebarButtons[i].ScreenPos.Height = SidebarButtons[i].Image->Height;
                SidebarButtons[i].ScreenPos.Width = SidebarButtons[i].Image->Width;
                SidebarButtons[i].ScreenPos.XPos = 20; // i * ScreenWidth / NUM_SIDEBAR_BUTTONS + SidebarButtons[i].Image->Width/2;
                SidebarButtons[i].ScreenPos.YPos = i * (SidebarButtons[i].ScreenPos.Height + 20) + 80;
            }
        }

        SelectionOverlay = egDecodePNG(select_png_bin, sizeof(select_png_bin), 128, TRUE);
        if (SelectionOverlay == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load selection overlay image");
            MtSupportDebugWriteLine(gBuffer);
        }

        SysInfoCPU = egDecodePNG(cputext_png_bin, sizeof(cputext_png_bin), 128, TRUE);
        if (SysInfoCPU == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load CPU text header image");
            MtSupportDebugWriteLine(gBuffer);
        }

        SysInfoMem = egDecodePNG(memtext_png_bin, sizeof(memtext_png_bin), 128, TRUE);
        if (SysInfoMem == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load Memory text header image");
            MtSupportDebugWriteLine(gBuffer);
        }

        DimmIcon = egDecodePNG(dimm_png_bin, sizeof(dimm_png_bin), 128, TRUE);
        if (DimmIcon == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load DIMM icon image");
            MtSupportDebugWriteLine(gBuffer);
        }

        NoDimmIcon = egDecodePNG(dimm_none_png_bin, sizeof(dimm_none_png_bin), 128, TRUE);
        if (NoDimmIcon == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load DIMM (none) icon image");
            MtSupportDebugWriteLine(gBuffer);
        }

#ifndef PRO_RELEASE
        OKBullet = egDecodePNG(ok_png_bin, sizeof(ok_png_bin), 128, TRUE);
        if (OKBullet == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load OK bullet image");
            MtSupportDebugWriteLine(gBuffer);
        }

        if (IsVirtualDisk && AsciiStrStr(gBaseBoardInfo.szManufacturer, "ASUSTeK") != NULL)
            QRCode = egDecodePNG(qrcode_asus_png_bin, sizeof(qrcode_asus_png_bin), 128, TRUE);
        else
            QRCode = egDecodePNG(qrcode_png_bin, sizeof(qrcode_png_bin), 128, TRUE);
        if (QRCode == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load QR code image");
            MtSupportDebugWriteLine(gBuffer);
        }
#endif

        //
        // Show welcome screen
        //
        if (gAutoMode) // If AUTOMODE is set, don't show splash screen and start immediately
            Select = ID_BUTTON_START;
        else if (gSkipSplash) // If SKIPSPLASH is set, don't show splash screen and go to the main menu immediately
            Select = ID_BUTTON_SYSINFO;
        else
            Select = ShowWelcomeScreen();
        // egSetGraphicsModeEnabled(FALSE);

        if (Select == ID_BUTTON_EXIT)
            goto Exit;
    }
    MtRangesConstructor();
    LockAllMemRanges();
    do
    {
        EFI_STATUS TestStatus;
        BOOLEAN ExactSPDFail = FALSE;
        BOOLEAN MinSPDFail = FALSE;
        BOOLEAN ExactSPDSizeFail = FALSE;
        BOOLEAN CheckMemSPDSizeFail = FALSE;
        BOOLEAN SPDManufFail = FALSE;
        BOOLEAN SPDPartNoFail = FALSE;
        BOOLEAN SameSPDPartNoFail = FALSE;
        BOOLEAN SPDMatchFail = FALSE;
        BOOLEAN InvalidSPDFile = FALSE;
        BOOLEAN CheckMemSpeedFail = FALSE;
        BOOLEAN CheckSPDSMBIOSFail = FALSE;
        CHAR16 szCfgMemSpeed[SHORT_STRING_LEN];
        CHAR16 szFailedMemSpeed[SHORT_STRING_LEN];
        BOOLEAN PretestCheck = TRUE;
        INT64 MemSizeMB = (INT64)MtSupportMemSizeMB();
        INT64 TotalSPDSizeMB = 0;
        CHAR16 szFailedSPDManuf[SHORT_STRING_LEN];
        CHAR16 szFailedSPDPartNo[MODULE_PARTNO_LEN];
        int iFailedSameSPDPartNoIdx = -1;
        int iFailedSPDSMBIOSIdx = -1;
        int iFailedSPDMatchIdx = -1;
        int iFailedSPDMatchByte = -1;
        UINT8 iFailedSPDActualValue = 0;
        UINT8 iFailedSPDExpectedValue = 0;
        // clang-format off
        const CHAR16 *BLOCK_SPD_FAIL[] = {
            L"███████ ██████  ██████      ███████  █████  ██ ██     ",
            L"██      ██   ██ ██   ██     ██      ██   ██ ██ ██     ",
            L"███████ ██████  ██   ██     █████   ███████ ██ ██     ",
            L"     ██ ██      ██   ██     ██      ██   ██ ██ ██     ",
            L"███████ ██      ██████      ██      ██   ██ ██ ███████"};
		// clang-format on
        gConsoleOnly = TRUE;
        if (TRUE)
        {
            UINT16 Cmd;

            if (gConsoleOnly)
                Cmd = MtSupportTestConfig();
            else
            {
                EnableGraphicsMode(TRUE);
                Cmd = MainMenu();
            }

            if (Cmd == ID_BUTTON_EXIT)
                goto Exit;
        }

        // Config
        MtSupportUninstallAllTests();
        InstallSelectedTests();

        ASSERT(MtSupportNumTests() > 0);

        EnableGraphicsMode(FALSE);

        // Perform any pre-test checks
        szFailedSPDManuf[0] = L'\0';
        szFailedSPDPartNo[0] = L'\0';

        if (gExactSPDs[0] >= 0)
        {
            ExactSPDFail = TRUE;
            for (i = 0; i < MAX_NUM_SPDS; i++)
            {
                if (gExactSPDs[i] == g_numMemModules)
                    ExactSPDFail = FALSE;
            }
        }
        else if ((UINTN)g_numMemModules < gMinSPDs) // Check minimum number of detected SPDs
            MinSPDFail = TRUE;

        for (i = 0; i < g_numMemModules; i++)
        {
            TotalSPDSizeMB += g_MemoryInfo[i].size;
        }

        if (gExactSPDSize >= 0)
        {
            INT64 ErrorMargin100 = MultS64x64(gExactSPDSize, MEM_SPD_MARGIN_OF_ERROR);
            if (MultS64x64(ABS(TotalSPDSizeMB - gExactSPDSize), 100) > ErrorMargin100)
                ExactSPDSizeFail = TRUE;

            AsciiFPrint(DEBUG_FILE_HANDLE, "Checking total size of detected SPDs (%ldMB) matches specified size (%ldMB) [Margin of error: %ld.%ldMB]", TotalSPDSizeMB, gExactSPDSize, DivU64x32(ErrorMargin100, 1000), ModU64x32(ErrorMargin100, 1000));
        }

        if (gCheckMemSPDSize != FALSE)
        {
            INT64 ErrorMargin100 = MultS64x64(MemSizeMB, MEM_SPD_MARGIN_OF_ERROR);
            if (MultS64x64(ABS(TotalSPDSizeMB - MemSizeMB), 100) > ErrorMargin100)
                CheckMemSPDSizeFail = TRUE;

            AsciiFPrint(DEBUG_FILE_HANDLE, "Checking total size of detected SPDs (%ldMB) against system memory size (%ldMB) [Margin of error: %ld.%ldMB]", TotalSPDSizeMB, MemSizeMB, DivU64x32(ErrorMargin100, 1000), ModU64x32(ErrorMargin100, 1000));
        }

        if (gCheckSPDSMBIOS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Checking SPD data (%d modules detected) is consistent with SMBIOS data (%d memory devices)", g_numMemModules, g_numSMBIOSModules);
            for (i = 0; i < g_numMemModules; i++)
            {
                if (g_MemoryInfo[i].smbiosInd < 0)
                {
                    CheckSPDSMBIOSFail = TRUE;
                    if (iFailedSPDSMBIOSIdx < 0)
                        iFailedSPDSMBIOSIdx = i;
                    AsciiFPrint(DEBUG_FILE_HANDLE, "SPD %d did not match any SMBIOS memory devices", i);
                }
                else
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "SPD %d matches SMBIOS %d", i, g_MemoryInfo[i].smbiosInd);                    
                }
            }
        }

        if (gSPDManuf[0] || gSPDPartNo[0] || gSameSPDPartNo) // Check SPD JEDEC manufacturer and part number
        {
            for (i = 0; i < g_numMemModules; i++)
            {
                if (gSPDManuf[0])
                {
                    if (StrStr(g_MemoryInfo[i].jedecManuf, gSPDManuf) == NULL)
                    {
                        SPDManufFail = TRUE;
                        StrCpyS(szFailedSPDManuf, sizeof(szFailedSPDManuf) / sizeof(szFailedSPDManuf[0]), g_MemoryInfo[i].jedecManuf);
                        break;
                    }
                }

                if (gSPDPartNo[0])
                {
                    CHAR16 szModulePartNo[MODULE_PARTNO_LEN];
                    AsciiStrToUnicodeStrS(g_MemoryInfo[i].modulePartNo, szModulePartNo, ARRAY_SIZE(szModulePartNo));

                    if (StrStr(szModulePartNo, gSPDPartNo) == NULL)
                    {
                        SPDPartNoFail = TRUE;
                        StrCpyS(szFailedSPDPartNo, sizeof(szFailedSPDPartNo) / sizeof(szFailedSPDPartNo[0]), szModulePartNo);
                        break;
                    }
                }

                if (gSameSPDPartNo)
                {
                    int j;
                    for (j = i + 1; j < g_numMemModules; j++)
                    {
                        if (AsciiStrCmp(g_MemoryInfo[i].modulePartNo, g_MemoryInfo[j].modulePartNo) != 0)
                        {
                            SameSPDPartNoFail = TRUE;
                            iFailedSameSPDPartNoIdx = j;
                            break;
                        }
                    }
                    break;
                }
            }
        }

        if (gSPDMatch) // Check SPD data
        {
            if (MtSupportIsValidSPDFile(SPD_FILENAME))
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Checking %d DIMM SPDs against %s file", g_numMemModules, SPD_FILENAME);
                if (g_numMemModules == 0)
                    SPDMatchFail = TRUE;
                else if (MtSupportCheckSPD(SPD_FILENAME, &iFailedSPDMatchIdx, &iFailedSPDMatchByte, &iFailedSPDActualValue, &iFailedSPDExpectedValue) == FALSE)
                    SPDMatchFail = TRUE;
            }
            else
            {
                InvalidSPDFile = TRUE;
            }
        }

        CheckMemSpeedFail = MtSupportCheckMemSpeed(szCfgMemSpeed, sizeof(szCfgMemSpeed), szFailedMemSpeed, sizeof(szFailedMemSpeed)) == FALSE;

        PretestCheck = (ExactSPDFail || MinSPDFail || ExactSPDSizeFail || CheckMemSPDSizeFail || SPDManufFail || SPDPartNoFail || SameSPDPartNoFail || SPDMatchFail || InvalidSPDFile || CheckMemSpeedFail || CheckSPDSMBIOSFail) == FALSE;

        if (PretestCheck)
        {
            // Execute tests
            TestStatus = MtSupportRunAllTests();

            EFI_TIME StartTime;
            CHAR16 Filename[128];
            CHAR16 Prefix[128];

            EFI_INPUT_KEY key;

            int TestResult = MtSupportGetTestResult();

            MtUiGenerateBeep(3);

            SetMem(&key, sizeof(key), 0);

            if (gSPDMatch) // Check SPD data
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Recollecting SPD info for SPDMATCH");

                // Cleanup memory info and recollect
                if (g_MemoryInfo)
                    FreePool(g_MemoryInfo);
                g_MemoryInfo = NULL;

                g_numMemModules = 0;
                g_maxMemModules = 0;

                mem_spd_info();
                map_spd_to_smbios();

                AsciiFPrint(DEBUG_FILE_HANDLE, "Checking %d DIMM SPDs against %s file", g_numMemModules, SPD_FILENAME);

                gFailedSPDMatchIdx = -1;
                gFailedSPDMatchByte = -1;
                gFailedSPDActualValue = 0;
                gFailedSPDExpectedValue = 0;
                MtSupportCheckSPD(SPD_FILENAME, &gFailedSPDMatchIdx, &gFailedSPDMatchByte, &gFailedSPDActualValue, &gFailedSPDExpectedValue);
            }

            // Status = MtSupportPMPTestResult();
            // if (Status != EFI_SUCCESS)
            // {
            //     if (gAutoMode != AUTOMODE_ON)
            //     {
            //         CHAR16 TempBuf2[128];
            //         UnicodeSPrint(TempBuf2, sizeof(TempBuf2), GetStringById(STRING_TOKEN(STR_ERROR_PMP), TempBuf, sizeof(TempBuf)), Status);

            //         SetMem(&key, sizeof(key), 0);

            //         MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
            //                       TempBuf2,
            //                       NULL);
            //     }
            // }

            MtSupportGetTestStartTime(&StartTime);

            Prefix[0] = L'\0';
            MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
            UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.html", Prefix, REPORT_BASE_FILENAME, MtSupportGetTimestampStr(&StartTime));

            // If automode is enabled, save report
            // if (gAutoMode != AUTOMODE_OFF && gAutoReport)
            // {
            //     MT_HANDLE FileHandle = NULL;

            //     UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.%s", Prefix, REPORT_BASE_FILENAME, MtSupportGetTimestampStr(&StartTime), gAutoReportFmt == AUTOREPORTFMT_HTML ? L"html" : L"bin");

            //     AsciiFPrint(DEBUG_FILE_HANDLE, "Saving test report to %s", Filename);
            //     Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
            //     if (FileHandle)
            //     {
            //         AsciiFPrint(DEBUG_FILE_HANDLE, "Test report file successfully created");
            //         if (gAutoReportFmt == AUTOREPORTFMT_HTML)
            //             MtSupportSaveHTMLCertificate(FileHandle);
            //         else
            //             MtSupportSaveBinaryReport(FileHandle);

            //         Status = MtSupportCloseFile(FileHandle);
            //     }

            //     if (Status == EFI_SUCCESS)
            //         AsciiFPrint(DEBUG_FILE_HANDLE, "Test report was successfully saved");
            //     else
            //     {
            //         AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to save test report: %r", Status);
            //         if (gAutoMode != AUTOMODE_ON)
            //         {
            //             gST->ConOut->ClearScreen(gST->ConOut);

            //             SetMem(&key, sizeof(key), 0);

            //             MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
            //                           GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), TempBuf, sizeof(TempBuf)),
            //                           NULL);
            //         }
            //     }
            // }
            if (!gOKnSkipWaiting) {
                UINTN EventIndex;
                gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
            }

            // If AUTOMODE=1 is not set, display test complete message
            if (gAutoMode != AUTOMODE_ON || (TestResult == RESULT_FAIL && gAutoPromptFail))
            {
                CHAR16 InfoLine[128];
				// clang-format off
                const CHAR16 *BLOCK_PASS[] = {
                    L"██████   █████  ███████ ███████",
                    L"██   ██ ██   ██ ██      ██     ",
                    L"██████  ███████ ███████ ███████",
                    L"██      ██   ██      ██      ██",
                    L"██      ██   ██ ███████ ███████"};

                const CHAR16 *BLOCK_FAIL[] = {
                    L"███████  █████  ██ ██     ",
                    L"██      ██   ██ ██ ██     ",
                    L"█████   ███████ ██ ██     ",
                    L"██      ██   ██ ██ ██     ",
                    L"██      ██   ██ ██ ███████"};

                const CHAR16 *BLOCK_INCOMPLETE[] = {
                    L"██  ███   ██  █████  █████  ███    ███ ██████  ██     ██████ ██████ ██████",
                    L"██  ████  ██ ██     ██   ██ ████  ████ ██   ██ ██     ██       ██   ██    ",
                    L"██  ██ ██ ██ ██     ██   ██ ██ ████ ██ ██████  ██     ████     ██   █████ ",
                    L"██  ██  ████ ██     ██   ██ ██  ██  ██ ██      ██     ██       ██   ██    ",
                    L"██  ██   ███  █████  █████  ██      ██ ██      ██████ ██████   ██   ██████"};
                // clang-format on
                UINTN Attribute = gST->ConOut->Mode->Attribute;

                const CHAR16 **BLOCK_RESULT = NULL;
                switch (TestResult)
                {
                case RESULT_FAIL:
                    BLOCK_RESULT = BLOCK_FAIL;
                    break;
                case RESULT_PASS:
                    BLOCK_RESULT = BLOCK_PASS;
                    break;
                case RESULT_INCOMPLETE_PASS:
                case RESULT_INCOMPLETE_FAIL:
                default:
                    BLOCK_RESULT = BLOCK_INCOMPLETE;
                    break;
                }

                AsciiFPrint(DEBUG_FILE_HANDLE, "Test result: %s (Errors: %d)",
                            MtSupportGetTestResultStr(TestResult, 0, TempBuf, sizeof(TempBuf)),
                            MtSupportGetNumErrors());

                if (TestResult == RESULT_PASS)
                {
                    if (MtSupportGetNumWarns() > 0)
                    {
                        Attribute = EFI_TEXT_ATTR(EFI_YELLOW, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));
                        GetStringById(STRING_TOKEN(STR_WARNING_HAMMER), InfoLine, sizeof(InfoLine));
                    }
                    else if (MtSupportGetNumCorrECCErrors() + MtSupportGetNumUncorrECCErrors() > 0)
                    {
                        Attribute = EFI_TEXT_ATTR(EFI_YELLOW, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));
                        GetStringById(STRING_TOKEN(STR_WARNING_ECC_ERRORS), InfoLine, sizeof(InfoLine));
                    }
                    else
                    {
                        Attribute = EFI_TEXT_ATTR(EFI_GREEN, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));
                        InfoLine[0] = L'\0';
                    }
                }
                else if (TestResult == RESULT_FAIL)
                {
                    Attribute = EFI_TEXT_ATTR(EFI_LIGHTRED, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));
                    if (MtSupportGetNumErrors() >= gMaxErrCount) // Aborted due to too many errors
                    {
                        GetStringById(STRING_TOKEN(STR_ERROR_MAXERRORS), InfoLine, sizeof(InfoLine));
                    }
                    else
                        InfoLine[0] = L'\0';
                }
                else if (TestResult == RESULT_INCOMPLETE_PASS || TestResult == RESULT_INCOMPLETE_FAIL)
                {
                    Attribute = EFI_TEXT_ATTR(EFI_YELLOW, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));
                    UnicodeSPrint(InfoLine, sizeof(InfoLine), GetStringById(STRING_TOKEN(STR_NOTE_INCOMPLETE), TempBuf, sizeof(TempBuf)), MtSupportGetNumErrors());
                }

                SetMem(&key, sizeof(key), 0);
                if (InfoLine[0] != L'\0')
                {
                    MtCreatePopUpAtPos(Attribute, 7, -1, &key,
                                       L"",
                                       BLOCK_RESULT[0],
                                       BLOCK_RESULT[1],
                                       BLOCK_RESULT[2],
                                       BLOCK_RESULT[3],
                                       BLOCK_RESULT[4],
                                       L"",
                                       L"--------------",
                                       InfoLine,
                                       GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)),
                                       NULL);
                }
                else
                {
                    MtCreatePopUpAtPos(Attribute, 7, -1, &key,
                                       L"",
                                       BLOCK_RESULT[0],
                                       BLOCK_RESULT[1],
                                       BLOCK_RESULT[2],
                                       BLOCK_RESULT[3],
                                       BLOCK_RESULT[4],
                                       L"",
                                       L"--------------",
                                       GetStringById(STRING_TOKEN(STR_TEST_COMPLETE), TempBuf, sizeof(TempBuf)),
                                       NULL);
                }
#ifdef PRO_RELEASE
                if (MtSupportSkipDIMMResults() == FALSE)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "Supported motherboard (\"%a\") and chipset (\"%a\") detected. Display individual DIMM results.", gBaseBoardInfo.szProductName, get_mem_ctrl_name());

                    EnableGraphicsMode(TRUE);
                    DIMMResultsScreen(0, 0);
                    EnableGraphicsMode(FALSE);
                }
#endif
                // Display test summary
                AsciiFPrint(DEBUG_FILE_HANDLE, "Display test result summary");
                MtSupportDisplayErrorSummary();
            }

            // If automode is disabled, prompt to save report and report errors to IMS
            if (gAutoMode == AUTOMODE_OFF)
            {
                SetMem(&key, sizeof(key), 0);

                // Prompt user to save report to file
                if (!gRunFromBIOS)
                {
                    Print(GetStringById(STRING_TOKEN(STR_SAVE_REPORT), TempBuf, sizeof(TempBuf)), Filename);

                    key.UnicodeChar = MtUiWaitForKey(0, NULL);
                }

                if (!gRunFromBIOS && (key.UnicodeChar == 'y' || key.UnicodeChar == 'h' || key.UnicodeChar == 'b')) // Save report
                {
                    MT_HANDLE FileHandle = NULL;

                    UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.%s", Prefix, REPORT_BASE_FILENAME, MtSupportGetTimestampStr(&StartTime), key.UnicodeChar == 'b' ? L"bin" : L"html");

                    AsciiFPrint(DEBUG_FILE_HANDLE, "Saving test report to %s", Filename);
                    Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                    if (FileHandle)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "Test report file successfully created");
                        if (key.UnicodeChar == 'y')
                            MtSupportSaveHTMLCertificate(FileHandle);
                        else if (key.UnicodeChar == 'b')
                            MtSupportSaveBinaryReport(FileHandle);

                        Status = MtSupportCloseFile(FileHandle);
                    }

                    if (Status == EFI_SUCCESS)
                        AsciiFPrint(DEBUG_FILE_HANDLE, "Test report was successfully saved");
                    else
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to save test report: %r", Status);

                        gST->ConOut->ClearScreen(gST->ConOut);

                        SetMem(&key, sizeof(key), 0);

                        MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                                      GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), TempBuf, sizeof(TempBuf)),
                                      NULL);
                    }
                }

                if (iMSIsAvailable() && MtSupportGetNumErrors() > 0)
                {
                    Print(GetStringById(STRING_TOKEN(STR_REPORT_IMS), TempBuf, sizeof(TempBuf)), gReportNumErr);

                    SetMem(&key, sizeof(key), 0);

                    key.UnicodeChar = MtUiWaitForKey(0, NULL);

                    if (key.UnicodeChar == 'y')
                    {
                        Status = MtSupportReportErrorAddrIMS();

                        if (Status != EFI_SUCCESS)
                        {
                            CHAR16 TempBuf2[128];
                            AsciiFPrint(DEBUG_FILE_HANDLE, "Error reporting error addresses to iMS: %r", Status);

                            gST->ConOut->ClearScreen(gST->ConOut);

                            UnicodeSPrint(TempBuf2, sizeof(TempBuf2), GetStringById(STRING_TOKEN(STR_ERROR_REPORT_IMS), TempBuf, sizeof(TempBuf)), Status);

                            SetMem(&key, sizeof(key), 0);
                            MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                                          TempBuf2,
                                          NULL);
                        }
                        else
                        {
                            MtSupportPrintIMSDefectList(NULL);
                        }
                        MtSupportPrintIMSDefectList(DEBUG_FILE_HANDLE);
                    }
                }
            }

            if (gSPDMatch && gFailedSPDMatchIdx >= 0)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "Post-test check failed. The SPD bytes for module %d, byte %d (0x%02X) does not match the %s file, byte %d (0x%02X)", gFailedSPDMatchIdx, gFailedSPDMatchByte, gFailedSPDActualValue, SPD_FILENAME, gFailedSPDMatchByte, gFailedSPDExpectedValue);
                MtSupportDebugWriteLine(gBuffer);

                if (gAutoMode != AUTOMODE_ON)
                {
                    CHAR16 ErrorMsg[128];
                    CHAR16 ErrorMsg2[128];

                    UINTN Attribute = EFI_TEXT_ATTR(EFI_LIGHTRED, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));

                    UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_MATCH_1), TempBuf, sizeof(TempBuf)), gFailedSPDMatchIdx, gFailedSPDMatchByte, gFailedSPDActualValue);
                    UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SPD_MATCH_2), TempBuf, sizeof(TempBuf)), SPD_FILENAME, gFailedSPDMatchByte, gFailedSPDExpectedValue);

                    SetMem(&key, sizeof(key), 0);
                    MtCreatePopUp(Attribute, &key,
                                  L"",
                                  BLOCK_SPD_FAIL[0],
                                  BLOCK_SPD_FAIL[1],
                                  BLOCK_SPD_FAIL[2],
                                  BLOCK_SPD_FAIL[3],
                                  BLOCK_SPD_FAIL[4],
                                  L"",
                                  L"--------------",
                                  ErrorMsg,
                                  ErrorMsg2,
                                  L"",
                                  GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)),
                                  NULL);
                }
            }
            else if (gSPDMatch && g_numMemModules == 0)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "Post-test check failed. No SPD modules were detected for SPD matching");
                MtSupportDebugWriteLine(gBuffer);

                if (gAutoMode != AUTOMODE_ON)
                {
                    CHAR16 ErrorMsg[128];

                    UINTN Attribute = EFI_TEXT_ATTR(EFI_LIGHTRED, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));

                    UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_NO_SPD), TempBuf, sizeof(TempBuf)));

                    SetMem(&key, sizeof(key), 0);
                    MtCreatePopUp(Attribute, &key,
                                  L"",
                                  BLOCK_SPD_FAIL[0],
                                  BLOCK_SPD_FAIL[1],
                                  BLOCK_SPD_FAIL[2],
                                  BLOCK_SPD_FAIL[3],
                                  BLOCK_SPD_FAIL[4],
                                  L"",
                                  L"--------------",
                                  ErrorMsg,
                                  L"",
                                  GetStringById(STRING_TOKEN(STR_ANY_KEY), TempBuf, sizeof(TempBuf)),
                                  NULL);
                }
            }
        }
        else
        {
			// clang-format off
            const CHAR16 *BLOCK_SIZE_FAIL[] = {
                L"███████ ██ ███████ ███████     ███████  █████  ██ ██     ",
                L"██      ██    ███  ██          ██      ██   ██ ██ ██     ",
                L"███████ ██   ███   █████       █████   ███████ ██ ██     ",
                L"     ██ ██  ███    ██          ██      ██   ██ ██ ██     ",
                L"███████ ██ ███████ ███████     ██      ██   ██ ██ ███████"};
            // clang-format on
            UINTN Attribute = EFI_TEXT_ATTR(EFI_LIGHTRED, ((gST->ConOut->Mode->Attribute & (BIT4 | BIT5 | BIT6)) >> 4));

            if (ExactSPDFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[80];
                CHAR16 ErrorMsg2[80];
                CHAR16 ErrorMsg3[80];
                CHAR16 ErrorMsg4[80];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The number of detected RAM SPDs (%d) is not equal to the number required (%d)", g_numMemModules, gExactSPDs[0]);
                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_1), TempBuf, sizeof(TempBuf)), g_numMemModules);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_2), TempBuf, sizeof(TempBuf)), gExactSPDs[0], L"EXACTSPDS");
                UnicodeSPrint(ErrorMsg3, sizeof(ErrorMsg3), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_3), TempBuf, sizeof(TempBuf)), L"EXACTSPDS");
                UnicodeSPrint(ErrorMsg4, sizeof(ErrorMsg4), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_4), TempBuf, sizeof(TempBuf)));

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              ErrorMsg3,
                              ErrorMsg4,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (MinSPDFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[80];
                CHAR16 ErrorMsg2[80];
                CHAR16 ErrorMsg3[80];
                CHAR16 ErrorMsg4[80];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The number of detected RAM SPDs (%d) is less than the minimum required (%d)", g_numMemModules, gMinSPDs);
                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_1), TempBuf, sizeof(TempBuf)), g_numMemModules);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_2), TempBuf, sizeof(TempBuf)), gMinSPDs, L"MINSPDS");
                UnicodeSPrint(ErrorMsg3, sizeof(ErrorMsg3), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_3), TempBuf, sizeof(TempBuf)), L"MINSPDS");
                UnicodeSPrint(ErrorMsg4, sizeof(ErrorMsg4), GetStringById(STRING_TOKEN(STR_ERROR_NUM_SPD_4), TempBuf, sizeof(TempBuf)));

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              ErrorMsg3,
                              ErrorMsg4,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (ExactSPDSizeFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[80];
                CHAR16 ErrorMsg2[80];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The total capacity of detected SPDs (%ldMB) is not consistent with the specified size (%ldMB)", TotalSPDSizeMB, gExactSPDSize);
                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_SIZE_1), TempBuf, sizeof(TempBuf)), TotalSPDSizeMB);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SPD_SIZE_EXACT), TempBuf, sizeof(TempBuf)), gExactSPDSize);

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SIZE_FAIL[0],
                              BLOCK_SIZE_FAIL[1],
                              BLOCK_SIZE_FAIL[2],
                              BLOCK_SIZE_FAIL[3],
                              BLOCK_SIZE_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (CheckMemSPDSizeFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[80];
                CHAR16 ErrorMsg2[80];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The total capacity of detected SPDs (%ldMB) is not consistent with the system memory size (%ldMB)", TotalSPDSizeMB, MemSizeMB);
                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_SIZE_1), TempBuf, sizeof(TempBuf)), TotalSPDSizeMB);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SPD_SIZE_SYSTEM), TempBuf, sizeof(TempBuf)), MemSizeMB);

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SIZE_FAIL[0],
                              BLOCK_SIZE_FAIL[1],
                              BLOCK_SIZE_FAIL[2],
                              BLOCK_SIZE_FAIL[3],
                              BLOCK_SIZE_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (SPDManufFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];
                CHAR16 ErrorMsg2[128];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The JEDEC manufacturer within the SPD (\"%s\") does not match the substring (\"%s\")", szFailedSPDManuf, gSPDManuf);
                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_MANUF_1), TempBuf, sizeof(TempBuf)), szFailedSPDManuf);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SPD_MANUF_2), TempBuf, sizeof(TempBuf)), gSPDManuf);

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (SPDPartNoFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];
                CHAR16 ErrorMsg2[128];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The RAM part number within the SPD (\"%s\") does not match the substring (\"%s\")", szFailedSPDPartNo, gSPDPartNo);
                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_PARTNO_1), TempBuf, sizeof(TempBuf)), szFailedSPDPartNo);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SPD_PARTNO_2), TempBuf, sizeof(TempBuf)), gSPDPartNo);

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (SameSPDPartNoFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];
                CHAR16 ErrorMsg2[128];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The RAM part number for module %d (\"%a\") does not match module %d (\"%a\")", 0, g_MemoryInfo[0].modulePartNo, iFailedSameSPDPartNoIdx, g_MemoryInfo[iFailedSameSPDPartNoIdx].modulePartNo);
                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SAME_SPD_PARTNO_1), TempBuf, sizeof(TempBuf)), 0, g_MemoryInfo[0].modulePartNo);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SAME_SPD_PARTNO_2), TempBuf, sizeof(TempBuf)), iFailedSameSPDPartNoIdx, g_MemoryInfo[iFailedSameSPDPartNoIdx].modulePartNo);

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (CheckSPDSMBIOSFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];
                CHAR16 ErrorMsg2[128];

                AsciiFPrint(DEBUG_FILE_HANDLE, "Test aborted. SPD %d (%08X) does not match any SMBIOS serial numbers", iFailedSPDSMBIOSIdx, g_MemoryInfo[iFailedSPDSMBIOSIdx].moduleSerialNo);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_SMBIOS_SN_1), TempBuf, sizeof(TempBuf)), iFailedSPDSMBIOSIdx, g_MemoryInfo[iFailedSPDSMBIOSIdx].moduleSerialNo);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SPD_SMBIOS_SN_2), TempBuf, sizeof(TempBuf)));

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);

            }
            else if (SPDMatchFail && iFailedSPDMatchIdx >= 0)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];
                CHAR16 ErrorMsg2[128];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. The SPD bytes for module %d, byte %d (0x%02X) does not match the %s file, byte %d (0x%02X)", iFailedSPDMatchIdx, iFailedSPDMatchByte, iFailedSPDActualValue, SPD_FILENAME, iFailedSPDMatchByte, iFailedSPDExpectedValue);

                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_MATCH_1), TempBuf, sizeof(TempBuf)), iFailedSPDMatchIdx, iFailedSPDMatchByte, iFailedSPDActualValue);
                UnicodeSPrint(ErrorMsg2, sizeof(ErrorMsg2), GetStringById(STRING_TOKEN(STR_ERROR_SPD_MATCH_2), TempBuf, sizeof(TempBuf)), SPD_FILENAME, iFailedSPDMatchByte, iFailedSPDExpectedValue);

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              ErrorMsg2,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (InvalidSPDFile)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. SPD file (%s) does not exist or is invalid", SPD_FILENAME);

                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_SPD_FILE), TempBuf, sizeof(TempBuf)), SPD_FILENAME);

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (SPDMatchFail && g_numMemModules == 0)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. No SPD modules were detected for SPD matching");

                MtSupportDebugWriteLine(gBuffer);

                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_NO_SPD), TempBuf, sizeof(TempBuf)));

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
            else if (CheckMemSpeedFail)
            {
                EFI_INPUT_KEY key;

                CHAR16 ErrorMsg[128];

                AsciiSPrint(gBuffer, BUF_SIZE, "Test aborted. Memory speed check failed");

                MtSupportDebugWriteLine(gBuffer);

                if (g_numMemModules == 0)
                {
                    UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_MEM_SPEED_NO_SPD), TempBuf, sizeof(TempBuf)));
                    szFailedMemSpeed[0] = L'\0';
                }
                else
                {
                    UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_MEM_SPEED), TempBuf, sizeof(TempBuf)), szCfgMemSpeed);
                }

                SetMem(&key, sizeof(key), 0);
                MtCreatePopUp(Attribute, &key,
                              L"",
                              BLOCK_SPD_FAIL[0],
                              BLOCK_SPD_FAIL[1],
                              BLOCK_SPD_FAIL[2],
                              BLOCK_SPD_FAIL[3],
                              BLOCK_SPD_FAIL[4],
                              L"",
                              L"--------------",
                              ErrorMsg,
                              szFailedMemSpeed,
                              L"",
                              GetStringById(STRING_TOKEN(STR_BACK_TO_MENU), TempBuf, sizeof(TempBuf)),
                              NULL);
            }
        }

        // If AUTOMODE is enabled, we are done
        gOknTestStart = FALSE;     // fix duplicate test
        gOknTestStatus = ((gOknTestStatus == 2) ? 2: 0);
        if (gAutoMode == AUTOMODE_ON) {
            if (!gOKnSkipWaiting)
                break;
        }
        gST->ConOut->ClearScreen(gST->ConOut);

        Select = ID_BUTTON_SYSINFO;
    } while (1);
    UnlockAllMemRanges();
    MtRangesDeconstructor();
    gST->ConOut->ClearScreen(gST->ConOut);

Exit:
    MtSupportUninstallAllTests();

    if (gExitMode == EXITMODE_PROMPT)
        gExitMode = ExitScreen();

    for (i = 0; i < NUM_SIDEBAR_BUTTONS; i++)
    {
        if (SidebarButtons[i].Image)
            egFreeImage(SidebarButtons[i].Image);
    }
    if (gAesContext)
        FreePool(gAesContext);

    if (SysInfoCPU)
        egFreeImage(SysInfoCPU);

    if (SysInfoMem)
        egFreeImage(SysInfoMem);

    if (HeaderLogo)
        egFreeImage(HeaderLogo);

    if (SelectionOverlay)
        egFreeImage(SelectionOverlay);

    if (DimmIcon)
        egFreeImage(DimmIcon);

    if (NoDimmIcon)
        egFreeImage(NoDimmIcon);

    if (OKBullet)
        egFreeImage(OKBullet);

    if (QRCode)
        egFreeImage(QRCode);

    CleanupLocalizationStrings();

    if (g_MemoryInfo)
        FreePool(g_MemoryInfo);

    if (g_SMBIOSMemDevices)
        FreePool(g_SMBIOSMemDevices);

    if (g_MemTSODInfo)
        FreePool(g_MemTSODInfo);

    if (g_TSODController)
        FreePool(g_TSODController);

    free_mem_controller();

    if (gCheckMemSpeed)
        FreePool(gCheckMemSpeed);

    if (gCustomTestList)
    {
        for (i = 0; i < gNumCustomTests; i++)
        {
            if (gCustomTestList[i].NameStr)
                FreePool(gCustomTestList[i].NameStr);
        }
        FreePool(gCustomTestList);
    }

    if (gChipMapCfg)
        FreePool(gChipMapCfg);

    if (FSHandleList)
        FreePool(FSHandleList);

    if (SaveDir)
        SaveDir->Close(SaveDir);

    if (DataEfiDir)
        DataEfiDir->Close(DataEfiDir);

    if (SelfDirPath)
        FreePool(SelfDirPath);

    if (SelfDir != SelfRootDir && SelfDir)
        SelfDir->Close(SelfDir);

    if (SelfRootDir)
        SelfRootDir->Close(SelfRootDir);

    //
    // Restore the cursor visibility, position, and attributes
    //
    gST->ConOut->EnableCursor(gST->ConOut, SavedConsoleMode.CursorVisible);
    gST->ConOut->SetCursorPosition(gST->ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
    gST->ConOut->SetAttribute(gST->ConOut, SavedConsoleMode.Attribute);

    // Restore previous graphics mode
    if (gConCtrlWorkaround)
        egSetGraphicsModeEnabled(PrevGraphicsEn);

    if (OldTPL != gTPL)
    {
        gBS->RestoreTPL(OldTPL);
    }

    if (gExitMode == EXITMODE_REBOOT)
        gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
    else if (gExitMode == EXITMODE_SHUTDOWN)
        gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);

	PrintAndStop();
	
    return EFI_SUCCESS;
}

#ifdef SHELLAPP // not used
/**
  Helper function to find ShellEnvironment2 for constructor.

  @param[in] ImageHandle    A copy of the calling image's handle.

  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
ShellFindSE2(
    IN EFI_HANDLE ImageHandle)
{
    EFI_STATUS Status;
    EFI_HANDLE *Buffer;
    UINTN BufferSize;
    UINTN HandleIndex;

    BufferSize = 0;
    Buffer = NULL;
    Status = gBS->OpenProtocol(ImageHandle,
                               &gEfiShellEnvironment2Guid,
                               (VOID **)&mEfiShellEnvironment2,
                               ImageHandle,
                               NULL,
                               EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    //
    // look for the mEfiShellEnvironment2 protocol at a higher level
    //
    if (EFI_ERROR(Status) || !(CompareGuid(&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid)))
    {
        //
        // figure out how big of a buffer we need.
        //
        Status = gBS->LocateHandle(ByProtocol,
                                   &gEfiShellEnvironment2Guid,
                                   NULL, // ignored for ByProtocol
                                   &BufferSize,
                                   Buffer);
        //
        // maybe it's not there???
        //
        if (Status == EFI_BUFFER_TOO_SMALL)
        {
            Buffer = (EFI_HANDLE *)AllocateZeroPool(BufferSize);
            if (Buffer == NULL)
            {
                return (EFI_OUT_OF_RESOURCES);
            }
            Status = gBS->LocateHandle(ByProtocol,
                                       &gEfiShellEnvironment2Guid,
                                       NULL, // ignored for ByProtocol
                                       &BufferSize,
                                       Buffer);
        }
        if (!EFI_ERROR(Status) && Buffer != NULL)
        {
            //
            // now parse the list of returned handles
            //
            Status = EFI_NOT_FOUND;
            for (HandleIndex = 0; HandleIndex < (BufferSize / sizeof(Buffer[0])); HandleIndex++)
            {
                Status = gBS->OpenProtocol(Buffer[HandleIndex],
                                           &gEfiShellEnvironment2Guid,
                                           (VOID **)&mEfiShellEnvironment2,
                                           ImageHandle,
                                           NULL,
                                           EFI_OPEN_PROTOCOL_GET_PROTOCOL);
                if (CompareGuid(&mEfiShellEnvironment2->SESGuid, &gEfiShellEnvironment2ExtGuid))
                {
                    mEfiShellEnvironment2Handle = Buffer[HandleIndex];
                    Status = EFI_SUCCESS;
                    break;
                }
            }
        }
    }
    if (Buffer != NULL)
    {
        FreePool(Buffer);
    }
    return (Status);
}

// This function was from from the BdsLib implementation in
// IntelFrameworkModulePkg\Library\GenericBdsLib\BdsConnect.c
// function name: BdsLibConnectAllEfi
/**
  This function will connect all current system handles recursively. The
  connection will finish until every handle's child handle created if it have.

  @retval EFI_SUCCESS           All handles and it's child handle have been
                                connected
  @retval EFI_STATUS            Return the status of gBS->LocateHandleBuffer().

**/
EFI_STATUS
EFIAPI
ConnectAllEfi(
    VOID)
{
    EFI_STATUS Status;
    UINTN HandleCount;
    EFI_HANDLE *HandleBuffer;
    UINTN Index;

    Status = gBS->LocateHandleBuffer(
        AllHandles,
        NULL,
        NULL,
        &HandleCount,
        &HandleBuffer);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    for (Index = 0; Index < HandleCount; Index++)
    {
        Status = gBS->ConnectController(HandleBuffer[Index], NULL, NULL, TRUE);
    }

    if (HandleBuffer != NULL)
    {
        FreePool(HandleBuffer);
    }

    return EFI_SUCCESS;
}

EFI_STATUS
LoadDriver(
    IN CHAR16 *FileName)
{
    EFI_HANDLE ImageHandle;
    EFI_STATUS Status;
    EFI_DEVICE_PATH_PROTOCOL *NodePath;
    EFI_DEVICE_PATH_PROTOCOL *FilePath;
    EFI_LOADED_IMAGE_PROTOCOL *ImageInfo;
#if 0
    EFI_IMAGE_DOS_HEADER      DosHeader;
    EFI_IMAGE_FILE_HEADER     ImageHeader;
    EFI_IMAGE_OPTIONAL_HEADER OptionalHeader;
#endif
    EFI_LIST_ENTRY FileList;
    SHELL_FILE_ARG *Arg;
    LIST_ENTRY *Link;

    InitializeListHead(&FileList);
    Status = mEfiShellEnvironment2->FileMetaArg(FileName, &FileList);

    if (EFI_ERROR(Status))
    {
        return Status;
    }

    Link = FileList.ForwardLink;

    Arg = CR(Link, SHELL_FILE_ARG, Link, SHELL_FILE_ARG_SIGNATURE);

    NodePath = FileDevicePath(NULL, Arg->FileName);
    FilePath = AppendDevicePath(Arg->ParentDevicePath, NodePath);
    if (NodePath)
    {
        FreePool(NodePath);
        NodePath = NULL;
    }

    if (!FilePath)
    {
        return EFI_OUT_OF_RESOURCES;
    }

#if 0
    //
    // Check whether Image is valid to be loaded.
    //
    Status = LibGetImageHeader(
        FilePath,
        &DosHeader,
        &ImageHeader,
        &OptionalHeader
    );
    if (!EFI_ERROR(Status) && !EFI_IMAGE_MACHINE_TYPE_SUPPORTED(ImageHeader.Machine) &&
        (OptionalHeader.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER ||
            OptionalHeader.Subsystem == EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER)) {
        FreePool(FilePath);
        PrintToken(
            STRING_TOKEN(STR_LOAD_IMAGE_TYPE_UNSUPPORTED),
            HiiLoadHandle,
            LibGetMachineTypeString(ImageHeader.Machine),
            LibGetMachineTypeString(EFI_IMAGE_MACHINE_TYPE)
        );
        return EFI_INVALID_PARAMETER;
    }
#endif

    Status = gBS->LoadImage(
        FALSE,
        gImageHandle,
        FilePath,
        NULL,
        0,
        &ImageHandle);
    FreePool(FilePath);

    if (EFI_ERROR(Status))
    {
        if (Status == EFI_SECURITY_VIOLATION)
        {
            gBS->UnloadImage(ImageHandle);
        }
        return EFI_INVALID_PARAMETER;
    }
    //
    // Verify the image is a driver ?
    //
    Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID *)&ImageInfo);

    if (EFI_ERROR(Status) ||
        (ImageInfo->ImageCodeType != EfiBootServicesCode && ImageInfo->ImageCodeType != EfiRuntimeServicesCode))
    {

        gBS->Exit(ImageHandle, EFI_INVALID_PARAMETER, 0, NULL);
        return EFI_INVALID_PARAMETER;
    }

    //
    // Start the image
    //
    Status = gBS->StartImage(ImageHandle, NULL, NULL);

    ConnectAllEfi();

    return Status;
}
#endif

EFI_STATUS WaitForKeyboardMouseInput()
{
    UINTN EventIdx;
    EFI_STATUS Status;
    EFI_EVENT inEvents[2];
    inEvents[0] = gST->ConIn->WaitForKey;
    inEvents[1] = GetMouseEvent();

    // Wait on keyboard/mouse input
    if ((Status = gBS->WaitForEvent(2, inEvents, &EventIdx)) != EFI_SUCCESS)
        AsciiFPrint(DEBUG_FILE_HANDLE, "WaitForEvent returned error: %r", Status);

    return Status;
}

// Returns the directory portion of a pathname. For instance,
// if FullPath is 'EFI\foo\bar.efi', this function returns the
// string 'EFI\foo'. The calling function is responsible for
// freeing the returned string's memory.
CHAR16 *FindPath(IN CHAR16 *FullPath)
{
    UINTN i, LastBackslash = 0;
    CHAR16 *PathOnly = NULL;

    if (FullPath != NULL)
    {
        for (i = 0; i < StrLen(FullPath); i++)
        {
            if (FullPath[i] == '\\')
                LastBackslash = i;
        } // for
        PathOnly = EfiStrDuplicate(FullPath);
        PathOnly[LastBackslash] = 0;
    } // if
    return (PathOnly);
}

// Converts forward slashes to backslashes, removes duplicate slashes, and
// removes slashes from both the start and end of the pathname.
// Necessary because some (buggy?) EFI implementations produce "\/" strings
// in pathnames, because some user inputs can produce duplicate directory
// separators, and because we want consistent start and end slashes for
// directory comparisons. A special case: If the PathName refers to root,
// return "/", since some firmware implementations flake out if this
// isn't present.
VOID CleanUpPathNameSlashes(IN OUT CHAR16 *PathName)
{
    CHAR16 *NewName;
    UINTN i, FinalChar = 0;
    BOOLEAN LastWasSlash = FALSE;

    NewName = AllocateZeroPool(sizeof(CHAR16) * (StrLen(PathName) + 2));
    if (NewName != NULL)
    {
        for (i = 0; i < StrLen(PathName); i++)
        {
            if ((PathName[i] == L'/') || (PathName[i] == L'\\'))
            {
                if ((!LastWasSlash) && (FinalChar != 0))
                    NewName[FinalChar++] = L'\\';
                LastWasSlash = TRUE;
            }
            else
            {
                NewName[FinalChar++] = PathName[i];
                LastWasSlash = FALSE;
            } // if/else
        } // for
        NewName[FinalChar] = 0;
        if ((FinalChar > 0) && (NewName[FinalChar - 1] == L'\\'))
            NewName[--FinalChar] = 0;
        if (FinalChar == 0)
        {
            NewName[0] = L'\\';
            NewName[1] = 0;
        }
        // Copy the transformed name back....
        StrCpyS(PathName, StrLen(PathName) + 1, NewName);
        FreePool(NewName);
    } // if allocation OK
}

// Splits an EFI device path into device and filename components. For instance, if InString is
// PciRoot(0x0)/Pci(0x1f,0x2)/Ata(Secondary,Master,0x0)/HD(2,GPT,8314ae90-ada3-48e9-9c3b-09a88f80d921,0x96028,0xfa000)/\bzImage-3.5.1.efi,
// this function will truncate that input to
// PciRoot(0x0)/Pci(0x1f,0x2)/Ata(Secondary,Master,0x0)/HD(2,GPT,8314ae90-ada3-48e9-9c3b-09a88f80d921,0x96028,0xfa000)
// and return bzImage-3.5.1.efi as its return value.
// It does this by searching for the last ")" character in InString, copying everything
// after that string (after some cleanup) as the return value, and truncating the original
// input value.
// If InString contains no ")" character, this function leaves the original input string
// unmodified and also returns that string. If InString is NULL, this function returns NULL.
static CHAR16 *SplitDeviceString(IN OUT CHAR16 *InString)
{
    INTN i;
    CHAR16 *FileName = NULL;
    BOOLEAN Found = FALSE;

    if (InString != NULL)
    {
        i = StrLen(InString) - 1;
        while ((i >= 0) && (!Found))
        {
            if (InString[i] == L')')
            {
                Found = TRUE;
                FileName = EfiStrDuplicate(&InString[i + 1]);
                CleanUpPathNameSlashes(FileName);
                InString[i + 1] = '\0';
            } // if
            i--;
        } // while
        if (FileName == NULL)
            FileName = EfiStrDuplicate(InString);
    } // if
    return FileName;
}

BOOL CheckFontLanguageSupport()
{
    EFI_STATUS Status;
    EFI_HII_FONT_PROTOCOL *HiiFont = NULL;
    BOOLEAN AllLangSupport = TRUE;
    int i = 0;

    // Get Hii Font protocol
    Status = gBS->LocateProtocol(&gEfiHiiFontProtocolGuid, NULL, (VOID **)&HiiFont);
    if (EFI_ERROR(Status))
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate HiiFont protocol: %r", Status);
        return FALSE;
    }
#if 0
    AsciiFPrint(DEBUG_FILE_HANDLE, "Getting glyph for <space> character (%d)...", L' ');

    {
        EFI_IMAGE_OUTPUT      *Blt = NULL;
        Status = HiiFont->GetGlyph(
            HiiFont,
            L' ',
            NULL,
            &Blt,
            NULL
        );

        if (Status != EFI_SUCCESS) {
            AsciiFPrint(DEBUG_FILE_HANDLE, "GetGlyph failed for <space> character 0x%04X (%r)", L' ', Status);
        }

        if (Blt != NULL) {
            UINTN SpaceWidth, SpaceHeight;
            AsciiFPrint(DEBUG_FILE_HANDLE, "Space glyph width=%d height=%d", Blt->Width, Blt->Height);

            GetTextInfo(L" ", &SpaceWidth, &SpaceHeight);
            AsciiFPrint(DEBUG_FILE_HANDLE, "Space text width=%d height=%d", SpaceWidth, SpaceHeight);

            FreePool(Blt);
            Blt = NULL;
        }
    }
#endif

    // Go through all languages and see if the font is able to display all characters in the sample text
    while (mSupportedLangList[i] != NULL)
    {
        UINT16 Count;
        EFI_IMAGE_OUTPUT *Blt;
        CHAR16 TestStr[128];
        UINTN TestStrSize = sizeof(TestStr);
        BOOLEAN LangSupported = TRUE;

        Blt = NULL;
        Count = 0;

        SetMem(TestStr, sizeof(TestStr), 0);
        if (gHiiString)
        {
            Status = gHiiString->GetString(gHiiString, mSupportedLangList[i], gStringPackHandle, STRING_TOKEN(STR_SAMPLE_TEXT), TestStr, &TestStrSize, NULL);

            if (Status != EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "GetString failed for language \"%a\": %r", mSupportedLangList[i], Status);
                LangSupported = FALSE;
                AllLangSupport = FALSE;
                TestStrSize = 0;
            }
        }
        else
        {
            StrCpyS(TestStr, sizeof(TestStr) / sizeof(TestStr[0]), (CHAR16 *)(STRING_ARRAY_NAME + gStringPkgOff[i][STR_SAMPLE_TEXT]));
            TestStrSize = StrLen(TestStr) * sizeof(TestStr[0]);
        }

        if (TestStrSize >= sizeof(TestStr))
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "** WARNING ** TestStrSize (%d) >= sizeof(TestStr) (%d)", TestStrSize, sizeof(TestStr));
        }

        while (TestStr[Count] != 0 && Count < sizeof(TestStr) / sizeof(TestStr[0]) && Count < TestStrSize / sizeof(TestStr[0]))
        {
            Status = HiiFont->GetGlyph(
                HiiFont,
                TestStr[Count],
                NULL,
                &Blt,
                NULL);
            if (Blt != NULL)
            {
                FreePool(Blt);
                Blt = NULL;
            }

            if (Status != EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "GetGlyph failed for character 0x%04X (%r)", TestStr[Count], Status);
                LangSupported = FALSE;
                AllLangSupport = FALSE;
                break;
            }
            Count++;
        }
        mLangFontSupported[i] = LangSupported;
        AsciiFPrint(DEBUG_FILE_HANDLE, "Language %a %s", mSupportedLangList[i], LangSupported ? L"is supported" : L"is not supported");
        i++;
    }
    return AllLangSupport;
}

#if 0
EFI_STATUS
InitHexFont(
    CHAR16* FontFile
)
{
    EFI_HII_HANDLE  HiiHandle = NULL;
    EFI_STATUS      Status = EFI_SUCCESS;
    MT_HANDLE       FileHandle = NULL;

    EFI_HII_DATABASE_PROTOCOL* HiiDatabase = NULL;
    EFI_HII_SIMPLE_FONT_PACKAGE_HDR* SimplifiedFont = NULL;
    UINT32                               PackageLength;
    UINT8* Package = NULL;
    UINT8* Location;
    FontHeader                           Header;
    EFI_NARROW_GLYPH* NarrowArray = NULL;
    EFI_WIDE_GLYPH* WideArray = NULL;
    UINT8                                Hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX                           ctx256;

    SetMem(&Header, sizeof(Header), 0);

    // Check if HiiDatabase protocol is available. We can only install a new font via HiiDatabase
    Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID**)&HiiDatabase);
    if (HiiDatabase == NULL)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Could not locate HiiDatabase protocol: %r", Status);
        return Status;
    }

    MtSupportOpenFile(&FileHandle, FontFile, EFI_FILE_MODE_READ, 0);
    if (FileHandle == NULL)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Unable to open %s: %r", FontFile, Status);
        return Status;
    }

    // Reading font data in .hex format
    {
        CHAR16      ReadLine[128];
        UINTN       Size = 128;
        BOOLEAN     Ascii;
        NarrowArray = (EFI_NARROW_GLYPH*)AllocateZeroPool(sizeof(EFI_NARROW_GLYPH) * 0x10000);
        WideArray = (EFI_WIDE_GLYPH*)AllocateZeroPool(sizeof(EFI_WIDE_GLYPH) * 0x10000);

        for (; !MtSupportEof(FileHandle); Size = 1024) {
            UINT16 cp;
            CHAR16* LinePtr = ReadLine;
            SetMem(ReadLine, sizeof(ReadLine), 0);

            Status = MtSupportReadLine(FileHandle, ReadLine, &Size, TRUE, &Ascii);

            //
            // ignore too small of buffer...
            //
            if (Status == EFI_BUFFER_TOO_SMALL) {
                Status = EFI_SUCCESS;
            }
            if (EFI_ERROR(Status)) {
                goto Error;
                break;
            }

            cp = (UINT16)StrHexToUintn(LinePtr);

            if (StrLen(LinePtr + 5) <= 32)
            {
                int i;
                NarrowArray[Header.NumberOfNarrowGlyphs].UnicodeWeight = (CHAR16)cp;
                NarrowArray[Header.NumberOfNarrowGlyphs].Attributes = 0;

                for (i = 0; i < 2; i++)
                {
                    CHAR16 szGlyph[32];
                    UINT64 glyph;

                    SetMem(szGlyph, sizeof(szGlyph), 0);
                    StrnCpyS(szGlyph, ARRAY_SIZE(szGlyph), LinePtr + 5 + i * 16, 16);
                    glyph = StrHexToUint64(szGlyph);

                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8] = (UINT8)(RShiftU64(glyph, 56) & 0xFF);
                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8 + 1] = (UINT8)(RShiftU64(glyph, 48) & 0xFF);
                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8 + 2] = (UINT8)(RShiftU64(glyph, 40) & 0xFF);
                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8 + 3] = (UINT8)(RShiftU64(glyph, 32) & 0xFF);
                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8 + 4] = (UINT8)(RShiftU64(glyph, 24) & 0xFF);
                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8 + 5] = (UINT8)(RShiftU64(glyph, 16) & 0xFF);
                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8 + 6] = (UINT8)(RShiftU64(glyph, 8) & 0xFF);
                    NarrowArray[Header.NumberOfNarrowGlyphs].GlyphCol1[i * 8 + 7] = (UINT8)(glyph & 0xFF);
                }

                Header.NumberOfNarrowGlyphs++;;


            }
            else
            {
                int i;
                WideArray[Header.NumberOfWideGlyphs].UnicodeWeight = (CHAR16)cp;
                WideArray[Header.NumberOfWideGlyphs].Attributes = EFI_GLYPH_WIDE;

                for (i = 0; i < 4; i++)
                {
                    CHAR16 szGlyph[32];
                    UINT64 glyph;

                    SetMem(szGlyph, sizeof(szGlyph), 0);
                    StrnCpy(szGlyph, ARRAY_SIZE(szGlyph), LinePtr + 5 + i * 16, 16);
                    glyph = StrHexToUint64(szGlyph);

                    WideArray[Header.NumberOfWideGlyphs].GlyphCol1[i * 4] = (UINT8)(RShiftU64(glyph, 56) & 0xFF);
                    WideArray[Header.NumberOfWideGlyphs].GlyphCol2[i * 4] = (UINT8)(RShiftU64(glyph, 48) & 0xFF);
                    WideArray[Header.NumberOfWideGlyphs].GlyphCol1[i * 4 + 1] = (UINT8)(RShiftU64(glyph, 40) & 0xFF);
                    WideArray[Header.NumberOfWideGlyphs].GlyphCol2[i * 4 + 1] = (UINT8)(RShiftU64(glyph, 32) & 0xFF);
                    WideArray[Header.NumberOfWideGlyphs].GlyphCol1[i * 4 + 2] = (UINT8)(RShiftU64(glyph, 24) & 0xFF);
                    WideArray[Header.NumberOfWideGlyphs].GlyphCol2[i * 4 + 2] = (UINT8)(RShiftU64(glyph, 16) & 0xFF);
                    WideArray[Header.NumberOfWideGlyphs].GlyphCol1[i * 4 + 3] = (UINT8)(RShiftU64(glyph, 8) & 0xFF);
                    WideArray[Header.NumberOfWideGlyphs].GlyphCol2[i * 4 + 3] = (UINT8)(glyph & 0xFF);
                }
                Header.NumberOfWideGlyphs++;
            }
        }
    }

    // Install font package
    PackageLength = 4
        + sizeof(EFI_HII_SIMPLE_FONT_PACKAGE_HDR)
        + (Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH))
        + (Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));

    Package = (UINT8*)AllocateZeroPool(PackageLength);

    WriteUnaligned32((UINT32*)Package, PackageLength);
    SimplifiedFont = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR*)(Package + 4);
    SimplifiedFont->Header.Length = (UINT32)(PackageLength - 4);
    SimplifiedFont->Header.Type = EFI_HII_PACKAGE_SIMPLE_FONTS;
    SimplifiedFont->NumberOfNarrowGlyphs = Header.NumberOfNarrowGlyphs;
    SimplifiedFont->NumberOfWideGlyphs = Header.NumberOfWideGlyphs;

    Location = (UINT8*)(&SimplifiedFont->NumberOfWideGlyphs + 1);
    CopyMem(Location, NarrowArray, Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH));

    Location += Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH);
    CopyMem(Location, WideArray, Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));

    // Write font data to file on disk
    {
        EFI_FILE_HANDLE GlyphHandle = NULL;
        FontHeader HeaderOut;
        UINTN BufSize;

        if (DataEfiDir)
            Status = DataEfiDir->Open(DataEfiDir, &GlyphHandle, L"unifont-13.0.05.new.bin", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
        else
            Status = SelfDir->Open(SelfDir, &GlyphHandle, L"unifont-13.0.05.new.bin", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

        CopyMem(HeaderOut.Signature, FONT_SIGNATURE, sizeof(HeaderOut.Signature));
        HeaderOut.MajorRev = FONT_MAJOR_REV;
        HeaderOut.MinorRev = FONT_MINOR_REV;
        HeaderOut.NumberOfNarrowGlyphs = Header.NumberOfNarrowGlyphs;
        HeaderOut.NumberOfWideGlyphs = Header.NumberOfWideGlyphs;

        SetMem(Hash, sizeof(Hash), 0);

        SHA256_Init(&ctx256);
        SHA256_Update(&ctx256, (unsigned char*)NarrowArray, Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH));
        SHA256_Update(&ctx256, (unsigned char*)WideArray, Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));
        SHA256_Final(Hash, &ctx256);
        CopyMem(HeaderOut.Hash, Hash, sizeof(HeaderOut.Hash));

        BufSize = sizeof(HeaderOut);
        GlyphHandle->Write(GlyphHandle, &BufSize, &HeaderOut);

        BufSize = HeaderOut.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH);
        GlyphHandle->Write(GlyphHandle, &BufSize, NarrowArray);

        BufSize = HeaderOut.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH);
        GlyphHandle->Write(GlyphHandle, &BufSize, WideArray);

        GlyphHandle->Close(GlyphHandle);
    }

    HiiHandle = HiiAddPackages(&mFontPackageGuid, gImageHandle, Package, NULL);
    if (HiiHandle == NULL)
    {
        Status = EFI_INVALID_PARAMETER;
        MtSupportDebugWriteLine("Could not add font - HiiAddPackages failed");
    }
Error:
    if (Package)
        FreePool(Package);
    if (NarrowArray)
        FreePool(NarrowArray);
    if (WideArray)
        FreePool(WideArray);
    MtSupportCloseFile(FileHandle);
#if 0
    // Reading font data from .pf2 file (incomplete)
    grub_font_t font = NULL;
    grub_font_loader_init();
    font = grub_font_load(L"unicode.pf2");
    if (font == NULL)
        Print(L"grub_font_load failed\n");
    else
    {
        EFI_HII_SIMPLE_FONT_PACKAGE_HDR* SimplifiedFont = NULL;
        UINT32                               PackageLength;
        UINT8* Package;
        UINT8* Location;
        EFI_NARROW_GLYPH* NarrowArray = (EFI_NARROW_GLYPH*)AllocateZeroPool(sizeof(EFI_NARROW_GLYPH) * 0x10000);
        EFI_WIDE_GLYPH* WideArray = (EFI_WIDE_GLYPH*)AllocateZeroPool(sizeof(EFI_WIDE_GLYPH) * 0x10000);
        UINT16 NumberOfNarrowGlyphs = 0;
        UINT16 NumberOfWideGlyphs = 0;

        UINT32 cp;

        Print(L"grub_font_load success\n");
        for (cp = 0; cp < 0x10000; cp++)
        {
            struct grub_font_glyph* glyph = grub_font_get_glyph(font, cp);

            if (glyph)
            {
                Print(L"glyph[%d] width=%d height=%d offset_x=%d offset_y=%d device_width=%d\n", cp, glyph->width, glyph->height, glyph->offset_x, glyph->offset_y, glyph->device_width);
                if (glyph->device_width <= 8)
                {
                    int col;

                    NarrowArray[NumberOfNarrowGlyphs].UnicodeWeight = (CHAR16)cp;
                    NarrowArray[NumberOfNarrowGlyphs].Attributes = 0;
                    for (col = 0; col < glyph->height; col++)
                    {
                        glyph->bitmap[col * glyph->width / 8] << col * glyph->width % 8
                            NarrowArray[NumberOfNarrowGlyphs].GlyphCol1[col + glyph->offset_y] = glyph->bitmap[col] >> glyph->offset_x
                            glyph->
                            NarrowArray[NumberOfNarrowGlyphs].GlyphCol1[col + glyph->offset_y] = glyph->bitmap[col] >> glyph->offset_x
                    }
                    CopyMem(NarrowArray[NumberOfNarrowGlyphs].GlyphCol1, glyph->bitmap, (glyph->width * glyph->height + 7) / 8);
                    NumberOfNarrowGlyphs++;
                }
                else
                {
                    int col;

                    WideArray[NumberOfWideGlyphs].UnicodeWeight = (CHAR16)cp;
                    WideArray[NumberOfWideGlyphs].Attributes = EFI_GLYPH_WIDE;
                    for (col = 0; col < (glyph->width * glyph->height + 7) / 8; col++)
                    {
                        if (col % 2 == 0)
                            WideArray[NumberOfWideGlyphs].GlyphCol1[col / 2] = glyph->bitmap[col];
                        else
                            WideArray[NumberOfWideGlyphs].GlyphCol2[col / 2] = glyph->bitmap[col];
                    }
                    NumberOfWideGlyphs++;
                }
            }
        }

        Print(L"Number of narrow glyphs = %d\n", NumberOfNarrowGlyphs);
        Print(L"Number of wide glyphs = %d\n", NumberOfWideGlyphs);

        PackageLength = 4
            + sizeof(EFI_HII_SIMPLE_FONT_PACKAGE_HDR)
            + (NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH))
            + (NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));

        Package = (UINT8*)AllocateZeroPool(PackageLength);

        WriteUnaligned32((UINT32*)Package, PackageLength);
        SimplifiedFont = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR*)(Package + 4);
        SimplifiedFont->Header.Length = (UINT32)(PackageLength - 4);
        SimplifiedFont->Header.Type = EFI_HII_PACKAGE_SIMPLE_FONTS;
        SimplifiedFont->NumberOfNarrowGlyphs = NumberOfNarrowGlyphs;
        SimplifiedFont->NumberOfWideGlyphs = NumberOfWideGlyphs;

        Location = (UINT8*)(&SimplifiedFont->NumberOfWideGlyphs + 1);
        CopyMem(Location, NarrowArray, NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH));

        Location += NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH);
        CopyMem(Location, WideArray, NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));

        HiiHandle = HiiAddPackages(&mFontPackageGuid, gImageHandle, Package, NULL);

        FreePool(Package);
        FreePool(NarrowArray);
        FreePool(WideArray);
    }
#endif
    return Status;
}
#endif

EFI_STATUS
InitFont(
    CHAR16 *FontFile)
{
#if 0
    EFI_HII_HANDLE HiiHandle = NULL;
    EFI_STATUS Status = EFI_SUCCESS;
    MT_HANDLE FileHandle = NULL;

    EFI_HII_DATABASE_PROTOCOL *HiiDatabase = NULL;
    EFI_HII_SIMPLE_FONT_PACKAGE_HDR *SimplifiedFont = NULL;
    UINT32 PackageLength;
    UINT8 *Package = NULL;
    UINT8 *Location;
    FontHeader Header;
    EFI_NARROW_GLYPH *NarrowArray = NULL;
    EFI_WIDE_GLYPH *WideArray = NULL;
    UINTN BufSize;
    UINT64 FileSize = 0;
    UINT8 Hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx256;

    SetMem(&Header, sizeof(Header), 0);

    // Check if HiiDatabase protocol is available. We can only install a new font via HiiDatabase
    Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&HiiDatabase);
    if (HiiDatabase == NULL)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Could not locate HiiDatabase protocol: %r", Status);
        return Status;
    }

    MtSupportOpenFile(&FileHandle, FontFile, EFI_FILE_MODE_READ, 0);
    if (FileHandle == NULL)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Unable to open %s: %r", FontFile, Status);
        return Status;
    }

    // Read font file header
    BufSize = sizeof(Header);
    // Status = FileHandle->Read(FileHandle, &BufSize, &Header);
    MtSupportReadFile(FileHandle, &BufSize, &Header);
    if (Status != EFI_SUCCESS || BufSize == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Read header failed: %r (BufSize=%d)", Status, BufSize);
        goto Error;
    }
    if (CompareMem(&Header.Signature, FONT_SIGNATURE, sizeof(Header.Signature)) != 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Invalid signature");
        goto Error;
    }
    if (Header.NumberOfNarrowGlyphs == 0 || Header.NumberOfWideGlyphs == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Invalid glyph info");
        goto Error;
    }

    AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Font header version: %d.%d", Header.MajorRev, Header.MajorRev);
    AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Number of narrow glyphs: %d", Header.NumberOfNarrowGlyphs);
    AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Number of wide glyphs: %d", Header.NumberOfWideGlyphs);

    Status = MtSupportGetFileSize(FileHandle, &FileSize);
    if (Status != EFI_SUCCESS)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Unable to get file size: %r ", Status);
        goto Error;
    }

    // Check valid file size
    if (FileSize !=
        (sizeof(Header) +
         Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH) +
         Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH)))
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Invalid font file");
        goto Error;
    }

    BufSize = Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH);
    NarrowArray = (EFI_NARROW_GLYPH *)AllocateZeroPool(BufSize);
    Status = MtSupportReadFile(FileHandle, &BufSize, NarrowArray);
    if (Status != EFI_SUCCESS || BufSize == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Read glyphs failed: %r (BufSize=%d)", Status, BufSize);
        goto Error;
    }

    BufSize = Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH);
    WideArray = (EFI_WIDE_GLYPH *)AllocateZeroPool(BufSize);
    MtSupportReadFile(FileHandle, &BufSize, WideArray);
    if (Status != EFI_SUCCESS || BufSize == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Read glyphs failed: %r (BufSize=%d)", Status, BufSize);
        goto Error;
    }

    // Check hash
    SetMem(Hash, sizeof(Hash), 0);

    SHA256_Init(&ctx256);
    SHA256_Update(&ctx256, (unsigned char *)NarrowArray, Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH));
    SHA256_Update(&ctx256, (unsigned char *)WideArray, Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));
    SHA256_Final(Hash, &ctx256);

    if (CompareMem(Hash, Header.Hash, SHA256_DIGEST_LENGTH) != 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "InitFont - Integrity check failed");
        goto Error;
    }

    // Install font package
    PackageLength = 4 + sizeof(EFI_HII_SIMPLE_FONT_PACKAGE_HDR) + (Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH)) + (Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));

    Package = (UINT8 *)AllocateZeroPool(PackageLength);

    WriteUnaligned32((UINT32 *)Package, PackageLength);
    SimplifiedFont = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR *)(Package + 4);
    SimplifiedFont->Header.Length = (UINT32)(PackageLength - 4);
    SimplifiedFont->Header.Type = EFI_HII_PACKAGE_SIMPLE_FONTS;
    SimplifiedFont->NumberOfNarrowGlyphs = Header.NumberOfNarrowGlyphs;
    SimplifiedFont->NumberOfWideGlyphs = Header.NumberOfWideGlyphs;

    Location = (UINT8 *)(&SimplifiedFont->NumberOfWideGlyphs + 1);
    CopyMem(Location, NarrowArray, Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH));

    Location += Header.NumberOfNarrowGlyphs * sizeof(EFI_NARROW_GLYPH);
    CopyMem(Location, WideArray, Header.NumberOfWideGlyphs * sizeof(EFI_WIDE_GLYPH));

    HiiHandle = HiiAddPackages(&mFontPackageGuid, gImageHandle, Package, NULL);
    if (HiiHandle == NULL)
    {
        Status = EFI_INVALID_PARAMETER;
        MtSupportDebugWriteLine("Could not add font - HiiAddPackages failed");
    }
Error:
    if (Package)
        FreePool(Package);
    if (NarrowArray)
        FreePool(NarrowArray);
    if (WideArray)
        FreePool(WideArray);
    MtSupportCloseFile(FileHandle);

    return Status;
#endif
    return 0;
}

/**

  Initialize string packages in HII database.

**/
VOID InitLocalizationStrings(
    VOID)
{
    EFI_HII_DATABASE_PROTOCOL *HiiDatabase = NULL;
    EFI_STATUS Status;

    Status = gBS->LocateProtocol(&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **)&HiiDatabase);
    if (HiiDatabase != NULL)
    {
        //
        // Initialize strings to HII database
        //
        gStringPackHandle = HiiAddPackages(
            &mLocalizationPackageGuid,
            NULL,
            STRING_ARRAY_NAME,
            NULL);

        if (gStringPackHandle == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to initialize localization string pack");
        }
        else
        {
            //
            // Retrieve the pointer to the UEFI HII String Protocol
            //
            Status = gBS->LocateProtocol(&gEfiHiiStringProtocolGuid, NULL, (VOID **)&gHiiString);
            if (Status != EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate HiiString protocol: %r", Status);
            }
        }
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Could not locate HiiDatabase protocol: %r", Status);
    }

    // If no Hii String protocol available, we need to parse the localization srings manually
    if (gHiiString == NULL)
    {
        UINT32 PkgListSize = 0;
        EFI_HII_STRING_PACKAGE_HDR *HiiStringPkg = NULL;
        int PkgIdx = 0;

        // First 4 bytes contain the size of the package list array
        PkgListSize = ReadUnaligned32((UINT32 *)STRING_ARRAY_NAME);

        // Read each string package (one per language) and count the number of packages (should match the number of languagse)
        HiiStringPkg = (EFI_HII_STRING_PACKAGE_HDR *)(STRING_ARRAY_NAME + sizeof(UINT32));
        while ((UINT8 *)HiiStringPkg < STRING_ARRAY_NAME + PkgListSize)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Found %a package (Length: %d)", HiiStringPkg->Language, HiiStringPkg->Header.Length);
            HiiStringPkg = (EFI_HII_STRING_PACKAGE_HDR *)((UINT8 *)HiiStringPkg + HiiStringPkg->Header.Length);
            gNumStringPkg++;
        }

        // Allocate an array of offsets for each language
        gStringPkgOff = AllocateZeroPool(sizeof(UINT32 *) * gNumStringPkg);

        // Go through each package and record the offsets of each string
        HiiStringPkg = (EFI_HII_STRING_PACKAGE_HDR *)(STRING_ARRAY_NAME + sizeof(UINT32));
        while (PkgIdx < gNumStringPkg)
        {
            // String packages not necessarily in order. Find the index of the current language
            int LangIdx = -1;
            for (int i = 0; i < sizeof(mLangFontSupported) / sizeof(mLangFontSupported[0]); i++)
            {
                if (AsciiStrCmp(HiiStringPkg->Language, mSupportedLangList[i]) == 0)
                {
                    LangIdx = i;
                    break;
                }
            }

            if (LangIdx >= 0)
            {
                // Strings should start after the header
                EFI_HII_STRING_BLOCK *HiiStringBlk = (EFI_HII_STRING_BLOCK *)((UINT8 *)HiiStringPkg + HiiStringPkg->StringInfoOffset);
                UINT32 StringId = 1;
                UINT32 StringPkgOffSize = MAX_STRING_OFFSETS;

                // Allocate an array of offsets
                gStringPkgOff[LangIdx] = AllocateZeroPool(sizeof(UINT32) * StringPkgOffSize);
                if (gStringPkgOff[LangIdx] == NULL)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[%a] Error allocating memory", mSupportedLangList[LangIdx]);
                    break;
                }

                while (HiiStringBlk->BlockType != EFI_HII_SIBT_END)
                {
                    switch (HiiStringBlk->BlockType)
                    {
                    case EFI_HII_SIBT_STRING_UCS2: // String block
                    {
                        if (StringId < StringPkgOffSize)
                        {
                            EFI_HII_SIBT_STRING_UCS2_BLOCK *Ucs2Block = (EFI_HII_SIBT_STRING_UCS2_BLOCK *)HiiStringBlk;
                            gStringPkgOff[LangIdx][StringId] = (UINT32)((UINT8 *)Ucs2Block->StringText - (UINT8 *)STRING_ARRAY_NAME);
                            HiiStringBlk = (EFI_HII_STRING_BLOCK *)(Ucs2Block->StringText + StrLen(Ucs2Block->StringText) + 1);
                            StringId++;
                        }
                    }
                    break;
                    case EFI_HII_SIBT_SKIP1: // Skip block
                    {
                        EFI_HII_SIBT_SKIP1_BLOCK *SkipBlock = (EFI_HII_SIBT_SKIP1_BLOCK *)HiiStringBlk;
                        int i;

                        AsciiFPrint(DEBUG_FILE_HANDLE, "[%a][%d] Found skip block 1: %d", mSupportedLangList[LangIdx], StringId, SkipBlock->SkipCount);

                        for (i = 0; i < SkipBlock->SkipCount; i++)
                        {
                            if (StringId < StringPkgOffSize)
                            {
                                // Set the offset equal to the offset of the default language (English)
                                gStringPkgOff[LangIdx][StringId] = gStringPkgOff[0][StringId];
                                StringId++;
                            }
                        }

                        HiiStringBlk = (EFI_HII_STRING_BLOCK *)(SkipBlock + 1);
                    }
                    break;

                    case EFI_HII_SIBT_SKIP2: // Skip block
                    {
                        EFI_HII_SIBT_SKIP2_BLOCK *SkipBlock = (EFI_HII_SIBT_SKIP2_BLOCK *)HiiStringBlk;
                        int i;

                        AsciiFPrint(DEBUG_FILE_HANDLE, "[%a][%d] Found skip block 2: %d", mSupportedLangList[LangIdx], StringId, SkipBlock->SkipCount);

                        for (i = 0; i < SkipBlock->SkipCount; i++)
                        {
                            if (StringId < StringPkgOffSize)
                            {
                                // Set the offset equal to the offset of the default language (English)
                                gStringPkgOff[LangIdx][StringId] = gStringPkgOff[0][StringId];
                                StringId++;
                            }
                        }

                        HiiStringBlk = (EFI_HII_STRING_BLOCK *)(SkipBlock + 1);
                    }
                    break;

                    default:
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "Unexpected block type: %x", HiiStringBlk->BlockType);
                    }
                    break;
                    }

                    if (StringId >= StringPkgOffSize)
                    {
                        UINT32 NewStringPkgOffSize = StringPkgOffSize * 2;

                        AsciiFPrint(DEBUG_FILE_HANDLE, "[%a] Increasing string offset size: %d -> %d", mSupportedLangList[LangIdx], StringPkgOffSize, NewStringPkgOffSize);

                        void *NewStringPkgOff = ReallocatePool(sizeof(UINT32) * StringPkgOffSize, sizeof(UINT32) * NewStringPkgOffSize, gStringPkgOff[LangIdx]);
                        if (NewStringPkgOff != NULL)
                        {
                            gStringPkgOff[LangIdx] = (UINT32 *)NewStringPkgOff;
                            StringPkgOffSize = NewStringPkgOffSize;
                        }
                        else
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "[%a] Error re-allocating memory", mSupportedLangList[LangIdx]);
                        }
                    }
                }

                // Reduce the size of offset array
                if (StringId < StringPkgOffSize)
                {
                    UINT32 NewStringPkgOffSize = StringId;

                    AsciiFPrint(DEBUG_FILE_HANDLE, "[%a] Decreasing string offset size: %d -> %d", mSupportedLangList[LangIdx], StringPkgOffSize, NewStringPkgOffSize);

                    void *NewStringPkgOff = ReallocatePool(sizeof(UINT32) * StringPkgOffSize, sizeof(UINT32) * NewStringPkgOffSize, gStringPkgOff[LangIdx]);
                    if (NewStringPkgOff != NULL)
                    {
                        gStringPkgOff[LangIdx] = (UINT32 *)NewStringPkgOff;
                        StringPkgOffSize = NewStringPkgOffSize;
                    }
                    else
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[%a] Error re-allocating memory", mSupportedLangList[LangIdx]);
                    }
                }
                AsciiFPrint(DEBUG_FILE_HANDLE, "[%a] Found %d strings", mSupportedLangList[LangIdx], StringId);
            }
            else
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Unexpected language found: %a", HiiStringPkg->Language);
            }

            // Move to the next package
            HiiStringPkg = (EFI_HII_STRING_PACKAGE_HDR *)((UINT8 *)HiiStringPkg + HiiStringPkg->Header.Length);
            PkgIdx++;
        }

        AsciiFPrint(DEBUG_FILE_HANDLE, "Finished parsing localization strings");
    }
}

VOID CleanupLocalizationStrings(
    VOID)
{
    if (gStringPackHandle)
        HiiRemovePackages(gStringPackHandle);
    gStringPackHandle = NULL;

    if (gStringPkgOff)
    {
        int i;
        for (i = 0; i < gNumStringPkg; i++)
        {
            if (gStringPkgOff[i])
                FreePool(gStringPkgOff[i]);
        }
        FreePool(gStringPkgOff);
    }
    gStringPkgOff = NULL;
}

/**
  Get string by string id from HII Interface


  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  L""       If error occurs.

**/
CHAR16 *
GetStringById(
    IN EFI_STRING_ID Id,
    OUT CHAR16 *Str,
    OUT UINTN StrSize)
{
    return GetStringFromPkg(gCurLang, Id, Str, StrSize);
}

/**
  Get string by string id from HII Interface


  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  L""       If error occurs.

**/
CHAR16 *
GetStringFromPkg(
    IN int LangId,
    IN EFI_STRING_ID StringId,
    OUT CHAR16 *Str,
    OUT UINTN StrSize)
{
    if (gHiiString)
    {
        //
        // Retrieve the string from the string package
        //
        EFI_STATUS Status = gHiiString->GetString(
            gHiiString,
            mSupportedLangList[LangId],
            gStringPackHandle,
            StringId,
            Str,
            &StrSize,
            NULL);
        if (Status != EFI_SUCCESS)
        {
            // Try getting the english string
            Status = gHiiString->GetString(
                gHiiString,
                mSupportedLangList[0],
                gStringPackHandle,
                StringId,
                Str,
                &StrSize,
                NULL);
        }

        if (Status != EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "GetString returned %r", Status);
            return L"";
        }
    }
    else
    {
        if (LangId >= gNumStringPkg)
            return L"";

        if (((StrLen((CHAR16 *)(STRING_ARRAY_NAME + gStringPkgOff[LangId][StringId])) + 1) * sizeof(CHAR16)) > StrSize)
            return L"";

        StrCpyS(Str, StrSize / sizeof(CHAR16), (CHAR16 *)(STRING_ARRAY_NAME + gStringPkgOff[LangId][StringId]));
    }
    return Str;
}

/* init.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */

/*
 * Initialize test, setup screen and find out how much memory there is.
 */
void init(void)
{
    CHAR16 TempBuf[128];

    /* Get the cpu and cache information */
    Print(GetStringById(STRING_TOKEN(STR_INIT_CPUID), TempBuf, sizeof(TempBuf)));
    MtSupportDebugWriteLine("Getting CPUID");
    get_cpuid(&cpu_id);
    PopulateCPUIDInfo(&cpu_id, &gCPUInfo);
    Print(GetStringById(STRING_TOKEN(STR_INIT_CACHE), TempBuf, sizeof(TempBuf)));
    MtSupportDebugWriteLine("Getting cache size");
    get_cache_size();
    Print(GetStringById(STRING_TOKEN(STR_INIT_SPEED), TempBuf, sizeof(TempBuf)));
    MtSupportDebugWriteLine("Measuring CPU/cache/mem speed");
    if (!cpu_cache_speed())
    {
        EFI_INPUT_KEY Key;
        MtCreatePopUp(gST->ConOut->Mode->Attribute, &Key,
                      L" **FATAL ERROR** ",
                      L" Failed to measure CPU cycle counter frequency. ",
                      L" Please send the log file to help@passmark.com. ",
                      L" Press any key to restart the system.           ",
                      NULL);
        gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
    }
    Print(GetStringById(STRING_TOKEN(STR_INIT_MSR), TempBuf, sizeof(TempBuf)));
    if (gDisableCPUInfo)
    {
        MtSupportDebugWriteLine("Skip retrieving CPU MSR data - CPU info collection disabled");
        cpu_msr.raw_freq_cpu = (int)cpu_speed;
    }
    else if (AsciiStrCmp(gBaseBoardInfo.szProductName, "VirtualBox") != 0)
    {
        MtSupportDebugWriteLine("Retrieving CPU MSR data");
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
        GetMSRData(&gCPUInfo, &cpu_msr);
#elif defined(MDE_CPU_AARCH64)
        AsciiFPrint(DEBUG_FILE_HANDLE, "MIDR: %016lx", cpu_id.midr.raw);
        AsciiFPrint(DEBUG_FILE_HANDLE, "MPIDR: %016lx", cpu_id.mpidr.raw);

        cpu_msr.raw_freq_cpu = cpu_speed;
#endif
    }
    else
    {
        MtSupportDebugWriteLine("Skip retrieving CPU MSR data - VirtualBox virtual machine detected");
        cpu_msr.raw_freq_cpu = (int)cpu_speed;
    }
    Print(GetStringById(STRING_TOKEN(STR_INIT_MEM), TempBuf, sizeof(TempBuf)));
    MtSupportDebugWriteLine("Getting memory size");
    display_memmap(DEBUG_FILE_HANDLE);
    mem_size();

#if 0
    if (l1_cache == 0) { l1_cache = 66; }
    if (l2_cache == 0) { l1_cache = 666; }
#endif

#if 0
    if (get_mem_ctrl_mode() != ECC_NONE && get_mem_ctrl_mode() != ECC_UNKNOWN)
    {
        EFI_CPU_ARCH_PROTOCOL     *mCpu;
        EFI_STATUS Status;

        //
        // Find the CPU architectural protocol.
        //
        Status = gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
        if (Status == EFI_SUCCESS)
        {
            Status = mCpu->RegisterInterruptHandler(mCpu, EXCEPT_IA32_NMI, NULL);
            if (Status == EFI_SUCCESS)
            {
                MtSupportDebugWriteLine("NMI interrupt handler successfully disabled");
            }
            else if (Status == EFI_INVALID_PARAMETER)
            {
                MtSupportDebugWriteLine("NMI interrupt handler is already disabled");
            }
            else
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "RegisterInterruptHandler failed - %r", Status);
                MtSupportDebugWriteLine(gBuffer);
            }

            if (AsciiStrCmp(get_mem_ctrl_name(), "Intel Sandy Bridge-E") == 0)
            {
                UINTN Cr4 = AsmReadCr4();
                if (((IA32_CR4 *)&Cr4)->Bits.MCE == 0)
                {
                    MtSupportDebugWriteLine("** Warning ** Cr4.MCE is disabled");
                }
                else
                {
                    MtSupportDebugWriteLine("Cr4.MCE is enabled");
                }

                MtSupportDebugWriteLine("ECC is enabled on Sandy Bridge-E. Disabling MCE interrupt handler.");

                Status = mCpu->RegisterInterruptHandler(mCpu, EXCEPT_IA32_MACHINE_CHECK, NULL);
                if (Status == EFI_SUCCESS)
                {
                    MtSupportDebugWriteLine("MCE interrupt handler successfully disabled");
                }
                else if (Status == EFI_INVALID_PARAMETER)
                {
                    MtSupportDebugWriteLine("MCE interrupt handler is already disabled");
                }
                else
                {
                    AsciiSPrint(gBuffer, BUF_SIZE, "RegisterInterruptHandler failed - %r", Status);
                    MtSupportDebugWriteLine(gBuffer);
                }
            }
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "LocateProtocol failed - %r", Status);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
#endif
}

/* Get cache sizes for most AMD and Intel CPUs, exceptions for old CPUs are
 * handled in CPU detection */
void get_cache_size()
{
    AsciiSPrint(gBuffer, BUF_SIZE, "get_cache_size - Vendor ID: %a Brand ID: %a", cpu_id.vend_id.char_array, cpu_id.brand_id.char_array);
    MtSupportDebugWriteLine(gBuffer);
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    GetCacheInfo(&gCPUInfo);
#elif defined(MDE_CPU_AARCH64)
    gCPUInfo.L1_instruction_caches_per_package = cpu_id.l1i.partitions;
    gCPUInfo.L1_instruction_cache_size = cpu_id.l1i.size / SIZE_1KB;

    gCPUInfo.L1_data_caches_per_package = cpu_id.l1d.partitions;
    gCPUInfo.L1_data_cache_size = cpu_id.l1d.size / SIZE_1KB;

    gCPUInfo.L2_caches_per_package = cpu_id.l2.partitions;
    gCPUInfo.L2_cache_size = cpu_id.l2.size / SIZE_1KB;

    gCPUInfo.L3_caches_per_package = cpu_id.l3.partitions;
    gCPUInfo.L3_cache_size = cpu_id.l3.size / SIZE_1KB;
#endif

    if (gCPUInfo.L1_instruction_caches_per_package > 0)
        AsciiSPrint(gBuffer, BUF_SIZE, "L1 instruction cache size: %d x %dKB", gCPUInfo.L1_instruction_caches_per_package, gCPUInfo.L1_instruction_cache_size);
    else
        AsciiSPrint(gBuffer, BUF_SIZE, "L1 instruction cache size: %dKB", gCPUInfo.L1_instruction_cache_size);
    MtSupportDebugWriteLine(gBuffer);

    if (gCPUInfo.L1_data_caches_per_package > 0)
        AsciiSPrint(gBuffer, BUF_SIZE, "L1 data cache size: %d x %dKB", gCPUInfo.L1_data_caches_per_package, gCPUInfo.L1_data_cache_size);
    else
        AsciiSPrint(gBuffer, BUF_SIZE, "L1 data cache size: %dKB", gCPUInfo.L1_data_cache_size);
    MtSupportDebugWriteLine(gBuffer);

    if (gCPUInfo.L2_caches_per_package > 0)
        AsciiSPrint(gBuffer, BUF_SIZE, "L2 cache size: %d x %dKB", gCPUInfo.L2_caches_per_package, gCPUInfo.L2_cache_size);
    else
        AsciiSPrint(gBuffer, BUF_SIZE, "L2 cache size: %dKB", gCPUInfo.L2_cache_size);
    MtSupportDebugWriteLine(gBuffer);

    if (gCPUInfo.L3_caches_per_package > 0)
        AsciiSPrint(gBuffer, BUF_SIZE, "L3 cache size: %d x %dKB", gCPUInfo.L3_caches_per_package, gCPUInfo.L3_cache_size);
    else
        AsciiSPrint(gBuffer, BUF_SIZE, "L3 cache size: %dKB", gCPUInfo.L3_cache_size);
    MtSupportDebugWriteLine(gBuffer);
}

/* Measure and display CPU and cache sizes and speeds */
int cpu_cache_speed()
{
#define MIN_SAMPLES 3
#define MAX_SAMPLES 10
    const int TOLERANCE = 500;
    int speed[MIN_SAMPLES];
    int iRetries = 0;
    int speedtotal = 0;
    int speedall = 0;
    int i;
    BOOLEAN usehpet = false;

    SetMem(speed, sizeof(speed), 0);
#if 0
    btrace(0, __LINE__, "cache_spd ", 1, l1_cache, l2_cache);
#endif

    AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed - freq = %ld", GetPerformanceCounterProperties(NULL, NULL));

    AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed - counter = %ld", GetPerformanceCounter());

    AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed - tsc = %ld", MtSupportReadTSC());

#if defined(MDE_CPU_AARCH64)
    EnableARMv8Cntr();
#endif

    // Use HPET if enabled
    usehpet = hpet_init() == 0;

    /* Turn on cache */
    MtSupportEnableCache(TRUE);

    /* Print CPU speed */
    do
    {
        iRetries++;

        // Keep track of the last 3 samples
        speed[2] = speed[1];
        speed[1] = speed[0];

        speed[0] = usehpet ? cpuspeed_hpet() : cpuspeed();

        if (usehpet && speed[0] == -1)
        {
            usehpet = FALSE;
            speed[0] = cpuspeed();
        }

        AsciiSPrint(gBuffer, BUF_SIZE, "cpu_cache_speed - (Attempt %d) clock cycle (ms): %d", iRetries, speed[0]);
        MtSupportDebugWriteLine(gBuffer);

        // Keep track of the sum of all samples
        speedall += speed[0];

        // Determine the variance of the last 3 samples
        speedtotal = 0;
        for (i = 0; i < MIN_SAMPLES; i++)
            speedtotal += speed[i];

    } while ((iRetries < MIN_SAMPLES) ||  // Make sure we have enough samples
             ((iRetries < MAX_SAMPLES) && // Make sure the last 3 samples are within the tolerance range
              ((ABS(3 * speed[0] - speedtotal) > 3 * TOLERANCE) ||
               (ABS(3 * speed[1] - speedtotal) > 3 * TOLERANCE) ||
               (ABS(3 * speed[2] - speedtotal) > 3 * TOLERANCE))));

    if (iRetries >= MAX_SAMPLES)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "cpu_cache_speed - Using average of last %d samples (%d)", iRetries, speedall / iRetries);
        MtSupportDebugWriteLine(gBuffer);
        if (speedall <= 0)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "**Fatal Error** Unable to measure CPU cycle counter (ie. RDTSC) frequency");
            MtSupportDebugWriteLine(gBuffer);
            return 0;
        }
        cpu_speed = clks_msec = speedall / iRetries;
    }
    else
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "cpu_cache_speed - Using average of last %d samples (%d, %d, %d)", MIN_SAMPLES, speed[0], speed[1], speed[2]);
        MtSupportDebugWriteLine(gBuffer);
        if (speedtotal <= 0)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "**Fatal Error** Unable to measure CPU cycle counter (ie. RDTSC) frequency");
            MtSupportDebugWriteLine(gBuffer);
            return 0;
        }
        cpu_speed = clks_msec = speedtotal / MIN_SAMPLES;
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "cpu_cache_speed - CPU speed: %3d.%1dMHz", cpu_speed / 1000, (cpu_speed / 100) % 10);
    MtSupportDebugWriteLine(gBuffer);

    /* Print out L1 cache info */
    /* To measure L1 cache speed we use a block size that is 1/4th */
    /* of the total L1 cache size since half of it is for instructions */
    if (gCPUInfo.L1_data_cache_size)
    {
        EFI_STATUS Status;
        EFI_PHYSICAL_ADDRESS Start = 0;
        Status = gBS->AllocatePages(
            AllocateAnyPages,
            EfiBootServicesData,
            (UINTN)(EFI_SIZE_TO_PAGES(gCPUInfo.L1_data_cache_size * SIZE_1KB)),
            &Start);

        int iter = 10000000 / gCPUInfo.L1_data_cache_size;
        AsciiFPrint(DEBUG_FILE_HANDLE, "cpu_cache_speed - measuring L1 cache speed at 0x%lx (size: %d KB, iter: %d)", Start, gCPUInfo.L1_data_cache_size, iter);

        if ((l1_speed = memspeed((UINTN)Start, (gCPUInfo.L1_data_cache_size / 2) * SIZE_1KB, iter)) != 0)
        {
        }

        AsciiFPrint(DEBUG_FILE_HANDLE, "cpu_cache_speed - L1 cache speed: %d MB/s", l1_speed);

        gBS->FreePages(Start, (UINTN)(EFI_SIZE_TO_PAGES(gCPUInfo.L1_data_cache_size * SIZE_1KB)));
    }

    /* Print out L2 cache info */
    if (gCPUInfo.L2_cache_size)
    {
        EFI_STATUS Status;
        EFI_PHYSICAL_ADDRESS Start = 0;
        Status = gBS->AllocatePages(
            AllocateAnyPages,
            EfiBootServicesData,
            (UINTN)(EFI_SIZE_TO_PAGES(gCPUInfo.L2_cache_size * SIZE_1KB)),
            &Start);

        int iter = 10000000 / gCPUInfo.L2_cache_size;
        AsciiFPrint(DEBUG_FILE_HANDLE, "cpu_cache_speed - measuring L2 cache speed at 0x%lx (size: %d KB, iter: %d)", Start, gCPUInfo.L2_cache_size, iter);

        if ((l2_speed = memspeed((UINTN)Start, (gCPUInfo.L2_cache_size / 2) * SIZE_1KB, iter)) != 0)
        {
        }

        AsciiFPrint(DEBUG_FILE_HANDLE, "cpu_cache_speed - L2 cache speed: %d MB/s", l2_speed);

        gBS->FreePages(Start, (UINTN)(EFI_SIZE_TO_PAGES(gCPUInfo.L2_cache_size * SIZE_1KB)));
    }
    /* Print out L3 cache info */

    if (gCPUInfo.L3_cache_size)
    {
        EFI_STATUS Status;
        EFI_PHYSICAL_ADDRESS Start = 0;
        Status = gBS->AllocatePages(
            AllocateAnyPages,
            EfiBootServicesData,
            (UINTN)(EFI_SIZE_TO_PAGES(gCPUInfo.L3_cache_size * SIZE_1KB)),
            &Start);

        int iter = 10000000 / gCPUInfo.L3_cache_size;
        AsciiFPrint(DEBUG_FILE_HANDLE, "cpu_cache_speed - measuring L3 cache speed at 0x%lx (size: %d KB, iter: %d)", Start, gCPUInfo.L3_cache_size, iter);

        if ((l3_speed = memspeed((UINTN)Start, (gCPUInfo.L3_cache_size / 2) * SIZE_1KB, iter)) != 0)
        {
        }

        AsciiFPrint(DEBUG_FILE_HANDLE, "cpu_cache_speed - L3 cache speed: %d MB/s", l3_speed);

        gBS->FreePages(Start, (UINTN)(EFI_SIZE_TO_PAGES(gCPUInfo.L3_cache_size * SIZE_1KB)));
    }

    if ((mem_speed = get_mem_speed()) != 0)
    {
    }

    mem_latency = test_mem_latency(500, MEM_LATENCY_RANDOMRANGE);

    AsciiSPrint(gBuffer, BUF_SIZE, "cpu_cache_speed - Memory latency: %d.%03d ns", mem_latency / 1000, mem_latency % 1000);
    MtSupportDebugWriteLine(gBuffer);

    return 1;
}

#if defined(__GNUC__)
#if defined(MDE_CPU_X64)
static inline void __movsd(unsigned long *dst, unsigned long *src, UINTN wlen)
{
    asm __volatile__(
        "mov %0,%%rsi\n\t"
        "mov %1,%%rdi\n\t"
        "mov %2,%%rcx\n\t"
        "cld\n\t"
        "rep\n\t"
        "movsl\n\t" ::"g"(src),
        "g"(dst), "g"(wlen)
        : "rsi", "rdi", "rcx");
}
#elif defined(MDE_CPU_AARCH64)
static __inline void aligned_block_copy_ldpstp_x_aarch64(
    IN VOID *Destination,
    IN VOID *Source,
    IN UINTN Count)
{
    __asm__ __volatile__(
        "mov         x0, %0"
        "\n\t"
        "mov         x1, %1"
        "\n\t"
        "mov         x2, %2"
        "\n\t"
        "cbz         x2, end"
        "\n\t"
        "loop:"
        "\n\t"
        "ldp         q0, q1, [x0, #(0 * 32)]"
        "\n\t"
        "ldp         q2, q3, [x0, #(1 * 32)]"
        "\n\t"
        "add         x0, x0, #64"
        "\n\t"
        "stp         q0, q1, [x1, #(0 * 32)]"
        "\n\t"
        "stp         q2, q3, [x1, #(1 * 32)]"
        "\n\t"
        "add         x1, x1, #64"
        "\n\t"
        "subs        x2, x2, #64"
        "\n\t"
        "bgt         loop"
        "\n\t"
        "end:"
        "\n\t"
        :
        : "r"(Source), "r"(Destination), "r"(Count)
        : "x0", "x1", "x2", "q0", "q1", "q2", "q3", "memory");
}
#endif
#elif defined(_MSC_VER)
extern void __movsd(
    VOID *Dest,
    VOID *Source,
    UINTN Count);
#endif

DISABLE_OPTIMISATIONS()
/* Measure cache speed by copying a block of memory. */
/* Returned value is kbytes/second */
int memspeed(UINTN src, UINT32 len, int iter)
{
    int i;
    UINTN dst, wlen;
    UINT64 st;
    UINT64 end;
    UINT64 cal;

    if (/* cpu_id.fid.bits.rdtsc == 0 || */ clks_msec == (UINT32)-1 || len == 0)
    {
        return (-1);
    }

    dst = src + len;
    wlen = len / 4; /* Length is bytes */

    /* Calibrate the overhead with a zero word copy */
    st = MtSupportReadTSC();

    for (i = 0; i < iter; i++)
    {
#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
        asm __volatile__(
            "movl %0,%%esi\n\t"
            "movl %1,%%edi\n\t"
            "movl %2,%%ecx\n\t"
            "cld\n\t"
            "rep\n\t"
            "movsl\n\t" ::"g"(src),
            "g"(dst), "g"(0)
            : "esi", "edi", "ecx");
#elif defined(MDE_CPU_X64)
        __movsd((VOID *)dst, (VOID *)src, 0);
#elif defined(MDE_CPU_AARCH64)
        aligned_block_copy_ldpstp_x_aarch64((VOID *)dst, (VOID *)src, 0);
#endif
#elif defined(_MSC_VER)
#if defined(MDE_CPU_IA32)
        _asm {
            mov esi, src
            mov edi, dst
            mov ecx, 0
            cld
            rep movsd
        }
#elif defined(MDE_CPU_X64)
        __movsd((VOID *)dst, (VOID *)src, 0);
#endif
#endif
    }

    cal = MtSupportReadTSC();

    /* Compute the overhead time */
    cal -= st;

    /* Now measure the speed */
    /* Do the first copy to prime the cache */
#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
    asm __volatile__(
        "movl %0,%%esi\n\t"
        "movl %1,%%edi\n\t"
        "movl %2,%%ecx\n\t"
        "cld\n\t"
        "rep\n\t"
        "movsl\n\t" ::"g"(src),
        "g"(dst), "g"(wlen)
        : "esi", "edi", "ecx");
#elif defined(MDE_CPU_X64)
    __movsd((VOID *)dst, (VOID *)src, wlen);
#elif defined(MDE_CPU_AARCH64)
    aligned_block_copy_ldpstp_x_aarch64((VOID *)dst, (VOID *)src, len);
#endif
#elif defined(_MSC_VER)
#if defined(MDE_CPU_IA32)
    _asm {
        mov esi, src
        mov edi, dst
        mov ecx, wlen
        cld
        rep movsd
    }
#elif defined(MDE_CPU_X64)
    __movsd((VOID *)dst, (VOID *)src, wlen);
#endif
#endif

    st = MtSupportReadTSC();

    for (i = 0; i < iter; i++)
    {
#ifdef __GNUC__
#if defined(MDE_CPU_IA32)
        asm __volatile__(
            "movl %0,%%esi\n\t"
            "movl %1,%%edi\n\t"
            "movl %2,%%ecx\n\t"
            "cld\n\t"
            "rep\n\t"
            "movsl\n\t" ::"g"(src),
            "g"(dst), "g"(wlen)
            : "esi", "edi", "ecx");
#elif defined(MDE_CPU_X64)
        __movsd((VOID *)dst, (VOID *)src, wlen);
#elif defined(MDE_CPU_AARCH64)
        aligned_block_copy_ldpstp_x_aarch64((VOID *)dst, (VOID *)src, len);
#endif
#elif defined(_MSC_VER)
#if defined(MDE_CPU_IA32)
        _asm {
            mov esi, src
            mov edi, dst
            mov ecx, wlen

            cld
            rep movsd
        }
#elif defined(MDE_CPU_X64)
        __movsd((VOID *)dst, (VOID *)src, wlen);
#endif
#endif
    }
    end = MtSupportReadTSC();

    /* Compute the elapsed time */
    end -= st;

    /* Subtract the overhead time */
    end -= cal;

    AsciiFPrint(DEBUG_FILE_HANDLE, "memspeed - elapsed time = %ld ms", DivU64x64Remainder(end, clks_msec, NULL));

    /* Calculate final memspeed
     *
     *  Takes the number of clock cycles elapsed performing move operation
     *  and converts to MB/s
     */
#if defined(_MSC_VER)
    UINT32 end_low = end & 0xffffffff;
    UINT32 end_high = RShiftU64(end, 32) & 0xffffffff;

    /* We have to fit the result into 32 bits so we dos some complicated
     * combinations to keep from over or underflow. Don't dink with unless
     * you know what you are doing. */
    end_low = end_low >> 4;
    end_low = ((end_high & 0xf) << 28) | end_low;

    /* Convert to clocks/KB */
    end_low /= (len / 1024);
    end_low *= 2048;
    end_low /= iter;
    end_low = (UINT32)(clks_msec * 256 / end_low);
    return (end_low);
#elif defined(__GNUC__)
    float res = (float)end;
    res /= 2.0;
    res /= (float)len;
    res *= 1024.0;
    res /= (float)iter;
    res = (float)clks_msec / res;
    return ((long)res);
#endif
}

/*
    test_mem_latency (float *test_res, int TestDuration, MEM_LATENCY_MODE mode)

    Description:
        Tests the latency of the memory by measuiring how many random reads
        it can do over the test duration.

        Based on sample code sent by Intel (from ronen.zohar@intel.com)

    Returns:
        The actual duration of the test in ms.
        Result in reads/second placed into test_res
*/
int test_mem_latency(int TestDuration, MEM_LATENCY_MODE mode)
{
    // Linked list structure for test
#define LINKED_LIST_ITEM_SIZE 64
#pragma pack(1)
    typedef struct LinkedListItem_s
    {
        struct LinkedListItem_s *NextPtr; // NextPtr Must be start of struct for assembler code to work
        int OrderInd;

        // Make sure 32 and 64 bit structs are the same size
        char Padding[LINKED_LIST_ITEM_SIZE - (sizeof(struct LinkedListItem_s *) + sizeof(int))];
    } LinkedListItem;
#pragma pack()
    const int BufferSize = SIZE_128MB; // 128MB
    const int BufferItemCount = BufferSize / sizeof(LinkedListItem);
    const UINTN ReadsPerLoop = BufferItemCount;
    const int RangeSize = SIZE_64KB / sizeof(LinkedListItem); // 64kb ranges

    UINT64 st;
    UINT64 end;

    UINT64 Timer_Diff;

    int LoopsCompleted;
    LinkedListItem *Buffer;

    int i;

    AsciiFPrint(DEBUG_FILE_HANDLE, "memory latency - allocate memory");

    // Allocate the memory buffer
    Buffer = (LinkedListItem *)AllocatePool(BufferSize);

    // Fail with message if malloc failed
    if (Buffer == NULL)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "memory latency - allocate memory failed");
        return 0;
    }

    // Create a linear ordering for the list
    for (i = 0; i < BufferItemCount; i++)
        Buffer[i].OrderInd = i;

    if (MEM_LATENCY_RANDOM == mode)
    {
        // Scramble the list (fully random rather than random range)
        // This method suffers from TLB lookup misses and grows progressivley worse with buffer size
        rand_init(1);
        rand_seed(37, 0);

        for (i = 0; i < BufferItemCount; i++)
        {
            // For every index swap it with another random index
            // Need double rand as single is only a 15-bit number
            int randIndex = ((rand(0) * rand(0)) % (BufferItemCount - 1)) + 1;
            int temp = Buffer[i].OrderInd;
            Buffer[i].OrderInd = Buffer[randIndex].OrderInd;
            Buffer[randIndex].OrderInd = temp;
        }
        rand_cleanup();
    }
    else if (MEM_LATENCY_RANDOMRANGE == mode)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "memory latency - randomize range");

        // Random range. Randomize order within blocks of 'RangeSize' instead of randomizing the entire
        // linked list. Removes effect of TLB lookup misses on benchmark.
        rand_init(1);
        rand_seed(37, 0);
        for (i = 0; i < BufferItemCount; i += RangeSize)
        {
            LinkedListItem *RangeStartPtr = Buffer + i;
            int ThisRangeSize = RangeSize;
            int j;
            if (i + RangeSize > BufferItemCount)
                ThisRangeSize = BufferItemCount - i;

            // Randomize all values in this range
            for (j = 0; j < ThisRangeSize; j++)
            {
                // For every index swap it with another random index
                int randIndex = rand(0) % ThisRangeSize;
                int temp = RangeStartPtr[j].OrderInd;
                RangeStartPtr[j].OrderInd = RangeStartPtr[randIndex].OrderInd;
                RangeStartPtr[randIndex].OrderInd = temp;
            }
        }
        rand_cleanup();
    }

    AsciiFPrint(DEBUG_FILE_HANDLE, "memory latency - build linked list");

    // ---Build the links for the linked list---
    // I have no idea how this works, based on intels code sample.
    // Also initially I thought we could just build a linked list of just nextPtr's but
    // that turned out to create a short circuited linked list when randomizing the ptr's so I had
    // to create order indeces and then set up the links like this as it was done in the sample code.
    for (i = 0; i < BufferItemCount - 1; i++)
        Buffer[Buffer[i].OrderInd].NextPtr = &Buffer[Buffer[i + 1].OrderInd];
    Buffer[Buffer[BufferItemCount - 1].OrderInd].NextPtr = &Buffer[Buffer[0].OrderInd];

#if 0
    // Debug code, check that the actual list length matches BufferItemCount
    LinkedListItem* CurrentItem = Buffer;
    int listLength = 0;
    do
    {
        listLength++;
        CurrentItem = CurrentItem->NextPtr;
    } while (CurrentItem != Buffer);
#endif

    AsciiFPrint(DEBUG_FILE_HANDLE, "memory latency - start test");

    // Start clock & set loop counter to 0
    st = MtSupportReadTSC();
    LoopsCompleted = 0;
    do
    {

#ifdef __GNUC__
#ifdef MDE_CPU_IA32
        __asm__ __volatile__(
            // Initialise values
            "mov %0,%%eax"
            "\n\t" // Buffer
            "mov %1,%%ebx"
            "\n\t" // ReadsPerLoop

            "read_random_dword_loop:"
            "\n\t" // do
            "mov (%%eax),%%eax"
            "\n\t" //  NextReadLocation = *NextReadLocation; //Read a DWORD
            "dec %%ebx"
            "\n\t" //  Counter--;
            "jnz read_random_dword_loop"
            "\n\t" //} while (Counter != 0);
            :
            : "r"(Buffer), "r"(ReadsPerLoop)
            : "eax", "ebx", "memory");
#elif defined(MDE_CPU_X64)
        __asm__ __volatile__(
            // Initialise values
            "mov %0,%%rax"
            "\n\t" // Buffer
            "mov %1,%%rbx"
            "\n\t" // ReadsPerLoop

            "read_random_dword_loop:"
            "\n\t" // do
            "mov (%%rax),%%rax"
            "\n\t" //  NextReadLocation = *NextReadLocation; //Read a DWORD
            "dec %%rbx"
            "\n\t" //  Counter--;
            "jnz read_random_dword_loop"
            "\n\t" //} while (Counter != 0);
            :
            : "g"(Buffer), "g"(ReadsPerLoop)
            : "rax", "rbx", "memory");
#elif defined(MDE_CPU_AARCH64)
        __asm__ __volatile__(
            "mov         x0, %0"
            "\n\t" // Buffer
            "mov         x1, %1"
            "\n\t" // ReadsPerLoop
            "read_random_dword_loop :"
            "\n\t" // do
            "ldr         x0, [x0]"
            "\n\t" //  NextReadLocation = *NextReadLocation; //Read a DWORD
            "subs        x1, x1, #1"
            "\n\t" //  Counter--
            "bgt         read_random_dword_loop "
            "\n\t" //} while (Counter != 0);
            :
            : "r"(Buffer), "r"(ReadsPerLoop)
            : "x0", "x1", "memory");
#endif
#elif defined(_MSC_VER)
#ifdef MDE_CPU_IA32
        _asm
            {
            push eax // Start address
            push ebx // Number of loops

                        // Initialise values
            mov eax, Buffer
            mov ebx, ReadsPerLoop

            read_random_dword_loop : // do 
            mov eax, [eax] //  NextReadLocation = *NextReadLocation; //Read a DWORD
                dec ebx //  Counter--;
                jnz read_random_dword_loop //} while (Counter != 0); 

                pop ebx
                pop eax
            }
#elif defined(MDE_CPU_X64)
        tests_memASM_latency(Buffer, ReadsPerLoop);
#endif
#endif
        // Update counter
        LoopsCompleted++;

        /* Compute the elapsed time */
        end = MtSupportReadTSC();
        end -= st;
        Timer_Diff = DivU64x64Remainder(end, clks_msec, NULL);

    } while (Timer_Diff < TestDuration);

    AsciiFPrint(DEBUG_FILE_HANDLE, "memory latency - test completed (loops=%d, rpl=%d)", LoopsCompleted, ReadsPerLoop);

    // Release the buffer
    FreePool(Buffer);

    return (int)DivU64x32(MultU64x32(Timer_Diff, 1000000000), (UINT32)(LoopsCompleted * ReadsPerLoop));
}
ENABLE_OPTIMISATIONS()

int get_mem_speed()
{
    int i;
    UINT32 speed = 0;
    UINT32 len;
    int ncpus = 1;

    EFI_STATUS Status;
    EFI_PHYSICAL_ADDRESS Start = 0;

#if 0
    /* Turn off cache */
    AsmDisableCache();
#endif

    /* Determine memory speed.  To find the memory speed we use
     * A block size that is the sum of all the L1, L2 & L3 caches
     * in all cpus * 8 */
    i = (gCPUInfo.L3_cache_size + gCPUInfo.L2_cache_size * ncpus + (gCPUInfo.L1_instruction_cache_size + gCPUInfo.L1_data_cache_size) * ncpus) * 8;

    if (i == 0) //
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "get_mem_speed - **warning** cache size is 0, cannot measure memory speed");
        MtSupportDebugWriteLine(gBuffer);
        return 0;
    }
#if 0
    /* Make sure that we have enough memory to do the test */
    /* If not use all we have */
    if ((UINTN)(1 + (i * 2)) > (lim_upper >> 10)) {
        i = ((lim_upper >> 10) - 1) / 2;
    }
#endif

    /* Divide up the memory block among the CPUs */
    len = i * SIZE_1KB / ncpus;

    Status = gBS->AllocatePages(
        AllocateAnyPages,
        EfiBootServicesData,
        (UINTN)(EFI_SIZE_TO_PAGES(len)),
        &Start);

    int iter = 5000000 / (len / SIZE_1KB);
    AsciiFPrint(DEBUG_FILE_HANDLE, "get_mem_speed - measuring mem speed at 0x%lx (size: %d KB, iter: %d)", Start, len / SIZE_1KB, iter);

    speed = memspeed((UINTN)Start, len / 2, 35);

    AsciiFPrint(DEBUG_FILE_HANDLE, "get_mem_speed - mem speed: %d MB/s", speed);

    gBS->FreePages(Start, (UINTN)(EFI_SIZE_TO_PAGES(len)));

#if 0
    /* Turn on cache */
    AsmEnableCache();
#endif

    return speed;
#if 0
    start = STEST_ADDR + (len * me);
    btrace(me, __LINE__, "mem_speed ", 1, start, len);

    barrier();
    spd[me] = memspeed(start, len, 35);
    barrier();
    if (me == 0) {
        for (i = 0; i < ncpus; i++) {
            speed += spd[i];
        }
        cprint(5, 16, "       MB/s");
        dprint(5, 16, speed, 6, 0);
    }
#endif
}

void mem_size(void)
{
    EFI_STATUS Status;
    UINTN NumberOfEntries;
    UINTN Loop;
    UINTN Size;
    EFI_MEMORY_DESCRIPTOR *MemoryMap;
    EFI_MEMORY_DESCRIPTOR *MapEntry;
    UINTN MapKey;
    UINTN DescSize;
    UINT32 DescVersion;
    UINT64 Available;

    Size = 0;
    Status = gBS->GetMemoryMap(&Size, NULL, NULL, NULL, NULL);
    ASSERT(Status == EFI_BUFFER_TOO_SMALL);

    Size = Size * 2; // Double the size just in case it changes

    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)AllocatePool(Size);
    if (MemoryMap == NULL)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "mem_size - Unable to allocate mem pool for memory map (%d bytes)", Size);
        MtSupportDebugWriteLine(gBuffer);
        return;
    }

    Status = gBS->GetMemoryMap(&Size, MemoryMap, &MapKey, &DescSize, &DescVersion);
    if (EFI_ERROR(Status))
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "mem_size - GetMemoryMap failed (%d)", Status);
        MtSupportDebugWriteLine(gBuffer);
        FreePool(MemoryMap);
        return;
    }

    if (DescVersion != EFI_MEMORY_DESCRIPTOR_VERSION)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "mem_size - Memory descriptor version incorrect (%d)", DescVersion);
        MtSupportDebugWriteLine(gBuffer);
        FreePool(MemoryMap);
        return;
    }

    NumberOfEntries = Size / DescSize;

    AsciiSPrint(gBuffer, BUF_SIZE, "mem_size - Number of entries: %d", NumberOfEntries);
    MtSupportDebugWriteLine(gBuffer);

    for (
        MapEntry = MemoryMap, Loop = 0, Available = 0;
        Loop < NumberOfEntries;
        MapEntry = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)MapEntry + DescSize),
       Loop++)
    {
        UINT64 PhysicalEnd = MapEntry->PhysicalStart + MultU64x32(MapEntry->NumberOfPages, EFI_PAGE_SIZE);
        if (PhysicalEnd > maxaddr)
            maxaddr = PhysicalEnd;

        if (MapEntry->Type == EfiLoaderCode ||
            MapEntry->Type == EfiLoaderData ||
            MapEntry->Type == EfiBootServicesCode ||
            MapEntry->Type == EfiBootServicesData ||
            MapEntry->Type == EfiRuntimeServicesCode ||
            MapEntry->Type == EfiRuntimeServicesData ||
            MapEntry->Type == EfiConventionalMemory ||
            MapEntry->Type == EfiUnusableMemory ||
            MapEntry->Type == EfiACPIReclaimMemory)
            Available += MapEntry->NumberOfPages;
    }
    FreePool(MemoryMap);

    memsize = MultU64x32(Available, EFI_PAGE_SIZE);

    AsciiSPrint(gBuffer, BUF_SIZE, "mem_size - Max address: 0x%lx", maxaddr);
    MtSupportDebugWriteLine(gBuffer);

#ifdef MDE_CPU_IA32
    if (maxaddr > 0x100000000ull)
    {
        maxaddr = 0x100000000ull;
        MtSupportDebugWriteLine("mem_size - Max address is greater than 0x100000000 in 32-bit mode. Setting maximum to 0x100000000");
    }
#endif

    AsciiSPrint(gBuffer, BUF_SIZE, "mem_size - Total memory size (%ld bytes)", memsize);
    MtSupportDebugWriteLine(gBuffer);
}

void expand_spd_info_array()
{
    const int SPDINFO_ARRAY_INCR = 16;
    int newMaxMemModules = g_maxMemModules + SPDINFO_ARRAY_INCR;
    void *NewMemoryInfo = NULL;

    AsciiSPrint(gBuffer, BUF_SIZE, "Expanding SPD info array (%d => %d)", g_maxMemModules, newMaxMemModules);
    MtSupportDebugWriteLine(gBuffer);

    NewMemoryInfo = ReallocatePool(sizeof(SPDINFO) * g_maxMemModules, sizeof(SPDINFO) * newMaxMemModules, g_MemoryInfo);

    AsciiSPrint(gBuffer, BUF_SIZE, "MemoryInfo(0x%p, %d bytes)", NewMemoryInfo, sizeof(SPDINFO) * newMaxMemModules);
    MtSupportDebugWriteLine(gBuffer);

    if (NewMemoryInfo == NULL)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Failed to re-allocate memory for SPD info (old size=%d, new size=%d)", g_maxMemModules, newMaxMemModules);
        MtSupportDebugWriteLine(gBuffer);
    }
    else
    {
        g_MemoryInfo = NewMemoryInfo;
        g_maxMemModules = newMaxMemModules;
    }
}

void expand_tsod_info_array()
{
    const int TSODINFO_ARRAY_INCR = 16;
    int newMaxTSODModules = g_maxTSODModules + TSODINFO_ARRAY_INCR;
    void *NewTSODInfo = NULL;

    AsciiSPrint(gBuffer, BUF_SIZE, "Expanding TSOD info array (%d => %d)", g_maxTSODModules, newMaxTSODModules);
    MtSupportDebugWriteLine(gBuffer);

    NewTSODInfo = ReallocatePool(sizeof(TSODINFO) * g_maxTSODModules, sizeof(TSODINFO) * newMaxTSODModules, g_MemTSODInfo);

    if (NewTSODInfo == NULL)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Failed to re-allocate memory for TSOD info (old size=%d, new size=%d)", g_maxTSODModules, newMaxTSODModules);
        MtSupportDebugWriteLine(gBuffer);
    }
    else
    {
        g_MemTSODInfo = NewTSODInfo;
        g_maxTSODModules = newMaxTSODModules;
    }
}

void mem_spd_info(void)
{
    PCIINFO *SMBusController = NULL;
    int numSMBCtrl = MAX_SMBUS_CTRL;
    BOOLEAN bSMBusMUX = FALSE;
    int i;

    UINTN BufSize = MAX_SMBUS_CTRL * sizeof(SMBusController[0]);

    if (gDisableSPD)
    {
        MtSupportDebugWriteLine("SPD collection disabled. Skipping...");
        return;
    }

    SMBusController = AllocateZeroPool(BufSize);
    if (SMBusController == NULL)
    {
        MtSupportDebugWriteLine("ERROR: Failed to allocate memory for SMBus controllers...");
        return;
    }

    // Scan_PCI(PCI_A_SCAN, NULL);

    MtSupportDebugWriteLine("Attempting to enable any disabled SMBus controllers...");

    // Try to enable any SMBUS devices first
    if (Scan_PCI(PCI_A_ENABLESMBUS, NULL, NULL))
    {
        MtSupportDebugWriteLine("Disabled SMBus controller was enabled");
    }

    // Look for SMBus
    if (Scan_PCI(PCI_A_SMBUS, SMBusController, &numSMBCtrl) == FALSE)
    {
        MtSupportDebugWriteLine("Could not find SMBus controller.");
#if 0
        // If not found, try to enable
        if (Scan_PCI(PCI_A_ENABLESMBUS, NULL, NULL) == FALSE)
        {
            MtSupportDebugWriteLine("Could not enable SMBus controller.");
        }
        else
        {
            // Look for SMBus again
            numSMBCtrl = MAX_SMBUS_CTRL;
            Scan_PCI(PCI_A_SMBUS, SMBusController, &numSMBCtrl);
        }
#endif
    }

    AsciiFPrint(DEBUG_FILE_HANDLE, "Found %d SMBus controllers", numSMBCtrl);

#if 0
    // if (Scan_PCI(PCI_A_SMBUS, &SMBusContoller) == FALSE)
    if (Find_PCI_SMBus(&SMBusController) == FALSE)
    {
        //If we couldn't find the SMBUS, then maybe BIOS has not enabled it - lets enable it
        // Scan_PCI(PCI_A_ENABLESMBUS, NULL);
        Enable_PCI_SMBus();

        //Scan the PCI bus again and find the SMBus controller; OK this could be more efficient - but is depends what you want to do
        // Scan_PCI(PCI_A_SMBUS, &SMBusContoller);
        Find_PCI_SMBus(&SMBusController);
    }
#endif

    if (AsciiStrCmp(gBaseBoardInfo.szProductName, "X8DTU-6+") == 0)
    {
        MtSupportDebugWriteLine("Supermicro X8DTU-6+ baseboard found. SMBus MUX present.");
        bSMBusMUX = TRUE;
    }
    else if (AsciiStrCmp(gBaseBoardInfo.szProductName, "X8DTU-LN4+") == 0)
    {
        MtSupportDebugWriteLine("Supermicro X8DTU-LN4+ baseboard found. SMBus MUX present.");
        bSMBusMUX = TRUE;
    }
    else if (AsciiStrCmp(gBaseBoardInfo.szProductName, "X8DTN") == 0)
    {
        MtSupportDebugWriteLine("Supermicro X8DTN baseboard found. SMBus MUX present.");
        bSMBusMUX = TRUE;
    }
    else if (AsciiStrCmp(gBaseBoardInfo.szProductName, "Z8P(N)E-D12(X)") == 0)
    {
        MtSupportDebugWriteLine("ASUS Z8P(N)E-D12(X) baseboard found. SMBus MUX present.");
        bSMBusMUX = TRUE;
    }

    // If we have a valid SMBus address in the 64KB I/O space then search for all slave devices on the SMBus
    for (i = 0; i < numSMBCtrl; i++)
    {
        if (SMBusController[i].dwDeviceType == PCI_T_SMBUS)
        {
            int numModules = 0;

            if (SMBusController[i].dwSMB_Address > 0 && SMBusController[i].dwSMB_Address < 0xFFFF)
            {
                AsciiSPrint(gBuffer, BUF_SIZE, "[%s %s Bus:%.2X Dev:%.2X Fun:%.2X Add:%.4X] Looking for SPD", SMBusController[i].tszVendor_Name, SMBusController[i].tszDevice_Name, SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun, SMBusController[i].dwSMB_Address);
                MtSupportDebugWriteLine(gBuffer);

                switch (SMBusController[i].smbDriver)
                {
                case SMBUS_NVIDIA: // NVIDIA
                    numModules = smbGetSPDNvidia((WORD)SMBusController[i].dwSMB_Address);
                    if (SMBusController[i].dwSMB_Address2 > 0 && SMBusController[i].dwSMB_Address2 < 0xFFFF)
                    {
                        numModules += smbGetSPDNvidia((WORD)SMBusController[i].dwSMB_Address2);
                    }
                    break;
                case SMBUS_INTEL_5000:
                case SMBUS_INTEL_801: // Intel
                    Enable_Intel801_SMBus(&SMBusController[i]);

                    if (bSMBusMUX && SMBusController[i].dwGPIO_Address > 0)
                    {
                        WORD seg;
                        for (seg = 0; seg < 4; seg++)
                        {
                            Intel801SelectSMBusSeg((WORD)SMBusController[i].dwGPIO_Address, seg);
                            numModules = smbGetSPDIntel801((WORD)SMBusController[i].dwSMB_Address);
                        }
                    }
                    else
                        numModules = smbGetSPDIntel801((WORD)SMBusController[i].dwSMB_Address);
                    break;
                case SMBUS_INTEL_801_SPD5:
                    Enable_Intel801_SMBus(&SMBusController[i]);

                    // Can support either DDR4 or DDR5 RAM. Try DDR4 first then DDR5
                    numModules = smbGetSPDIntel801((WORD)SMBusController[i].dwSMB_Address);
                    if (numModules == 0)
                        numModules = smbGetSPD5Intel801(SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun, (WORD)SMBusController[i].dwSMB_Address);
                    break;

                case SMBUS_PIIX4: // ATI
                    // Can support either DDR4 or DDR5 RAM. Try DDR4 first then DDR5
                    numModules = smbGetSPDPIIX4(SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun, (WORD)SMBusController[i].dwSMB_Address, SMBusController[i].byRev);
                    if (numModules == 0)
                        numModules = smbGetSPD5PIIX4(SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun, (WORD)SMBusController[i].dwSMB_Address, SMBusController[i].byRev);

                    break;
                case SMBUS_VIA: // VIA
                    numModules = smbGetSPDVia((WORD)SMBusController[i].dwSMB_Address);
                    break;
                case SMBUS_AMD756: // AMD
                    numModules = smbGetSPDAmd756((WORD)SMBusController[i].dwSMB_Address);
                    break;
                case SMBUS_SIS96X: // SIS 96X
                    numModules = smbGetSPDSiS96x((WORD)SMBusController[i].dwSMB_Address);
                    break;
                case SMBUS_SIS968: // SIS 968
                    numModules = smbGetSPDSiS968((WORD)SMBusController[i].dwSMB_Address);
                    break;
                case SMBUS_SIS630: // SIS 630/730/964
                    numModules = smbGetSPDSiS630((WORD)SMBusController[i].dwSMB_Address);
                    break;
                case SMBUS_ALI1563: // ALI1563
                    numModules = smbGetSPDAli1563((WORD)SMBusController[i].dwSMB_Address);
                    break;
                default:
                    break;
                }
            }

            // If failed, see if we can use PCI
            if (numModules <= 0)
            {
                switch (SMBusController[i].smbDriver)
                {
                case SMBUS_INTEL_5100: // Intel
                    AsciiSPrint(gBuffer, BUF_SIZE, "[%s %s Bus:%.2X Dev:%.2X Fun:%.2X] Looking for SPD", SMBusController[i].tszVendor_Name, SMBusController[i].tszDevice_Name, SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun);
                    MtSupportDebugWriteLine(gBuffer);
                    numModules = smbGetSPDIntel5100_PCI(SMBusController[i].SMB_Address_PCI.dwBus, SMBusController[i].SMB_Address_PCI.dwDev, SMBusController[i].SMB_Address_PCI.dwFun, SMBusController[i].SMB_Address_PCI.dwReg);
                    break;
                case SMBUS_INTEL_PCU:
                    AsciiSPrint(gBuffer, BUF_SIZE, "[%s %s Bus:%.2X Dev:%.2X Fun:%.2X] Looking for SPD", SMBusController[i].tszVendor_Name, SMBusController[i].tszDevice_Name, SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun);
                    MtSupportDebugWriteLine(gBuffer);
                    numModules = smbGetSPDIntelPCU(SMBusController[i].SMB_Address_PCI.dwBus, SMBusController[i].SMB_Address_PCI.dwDev, SMBusController[i].SMB_Address_PCI.dwFun);
                    break;
                case SMBUS_INTEL_PCU_IL:
                    AsciiSPrint(gBuffer, BUF_SIZE, "[%s %s Bus:%.2X Dev:%.2X Fun:%.2X] Looking for SPD", SMBusController[i].tszVendor_Name, SMBusController[i].tszDevice_Name, SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun);
                    MtSupportDebugWriteLine(gBuffer);
                    numModules = smbGetSPDIntelPCU_IL(SMBusController[i].SMB_Address_PCI.dwBus, SMBusController[i].SMB_Address_PCI.dwDev, SMBusController[i].SMB_Address_PCI.dwFun);
                    break;

                default:
                    break;
                }
            }

            // If failed, see if we can use MMIO
            if (numModules <= 0 && SMBusController[i].SMB_Address_MMIO > 0)
            {
                switch (SMBusController[i].smbDriver)
                {
                case SMBUS_INTEL_801: // Intel
                    AsciiSPrint(gBuffer, BUF_SIZE, "[%s %s Bus:%.2X Dev:%.2X Fun:%.2X] Looking for SPD", SMBusController[i].tszVendor_Name, SMBusController[i].tszDevice_Name, SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun);
                    MtSupportDebugWriteLine(gBuffer);
                    numModules = smbGetSPDIntel801_MMIO((PBYTE)SMBusController[i].SMB_Address_MMIO);
                    break;

                case SMBUS_INTEL_X79: // Intel
                    AsciiSPrint(gBuffer, BUF_SIZE, "[%s %s Bus:%.2X Dev:%.2X Fun:%.2X] Looking for SPD", SMBusController[i].tszVendor_Name, SMBusController[i].tszDevice_Name, SMBusController[i].byBus, SMBusController[i].byDev, SMBusController[i].byFun);
                    MtSupportDebugWriteLine(gBuffer);
                    numModules = smbGetSPDIntelX79_MMIO(SMBusController[i].SMB_Address_PCI.dwBus, SMBusController[i].SMB_Address_PCI.dwDev, SMBusController[i].SMB_Address_PCI.dwFun, (PBYTE)SMBusController[i].MMCFGBase, (PBYTE)SMBusController[i].SMB_Address_MMIO);
                    break;
                default:
                    break;
                }
            }

            if (numModules <= 0)
            {
                switch (SMBusController[i].smbDriver)
                {
                case SMBUS_INTEL_5000: // Intel
                    numModules = smbGetSPDIntel5000_PCI();
                    break;
                default:
                    break;
                }
            }
        }
    }

#ifdef DDR5_DEBUG
    smbGetSPD5Intel801(0, 0, 0, 0);
#endif

    if (g_numMemModules == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to detect SPD modules");

        if (g_MemoryInfo)
            FreePool(g_MemoryInfo);
        g_MemoryInfo = NULL;
        g_maxMemModules = 0;
    }

    if (SMBusController)
        FreePool(SMBusController);

    MtSupportDebugWriteLine("Enumerating PCI bus...");
    // Enumerate the PCI bus
    Scan_PCI(PCI_A_SCAN, NULL, NULL);
}

void mem_temp_info(void)
{
    if (gTSODPoll == FALSE)
    {
        MtSupportDebugWriteLine("TSOD collection disabled. Skipping...");
        return;
    }

    UINTN BufSize = MAX_TSOD_CTRL * sizeof(g_TSODController[0]);
    g_TSODController = AllocateZeroPool(BufSize);
    if (g_TSODController == NULL)
    {
        MtSupportDebugWriteLine("ERROR: Failed to allocate memory for TSOD controllers...");
        return;
    }

    g_numTSODCtrl = MAX_TSOD_CTRL;
    // Scan_PCI(PCI_A_SCAN, NULL);

    // Scan the PCI bus and find the SMBus controller
    MtSupportDebugWriteLine("Attempting to enable any disabled TSOD controllers...");

    // Try to enable any SMBUS devices first
    if (Scan_PCI(PCI_A_ENABLESMBUS, NULL, NULL))
    {
        MtSupportDebugWriteLine("Disabled TSOD controller was enabled");
    }

    // Look for SMBus
    if (Scan_PCI(PCI_A_TSOD, g_TSODController, &g_numTSODCtrl) == FALSE)
    {
        MtSupportDebugWriteLine("Could not find TSOD controller.");
    }

    if (g_numTSODCtrl == 0)
    {
        if (g_TSODController)
            FreePool(g_TSODController);
        g_TSODController = NULL;
    }
    else
    {
        if (g_numTSODCtrl < MAX_TSOD_CTRL)
        {
            // Reduce the size of buffer
            PCIINFO *NewTSODController = ReallocatePool(BufSize, g_numTSODCtrl * sizeof(g_TSODController[0]), g_TSODController);
            if (NewTSODController != NULL)
            {
                g_TSODController = NewTSODController;
            }
        }

        // Attempt to get RAM temperature
        int RAMTemp = MAX_TSOD_TEMP;
        int NumTSOD = MtSupportGetTSODInfo(&RAMTemp, 1);

        // Check if valid
        if (NumTSOD <= 0 || ABS(RAMTemp) >= MAX_TSOD_TEMP)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to detect TSOD modules");

            // We couldn't get temperatures. Free memory
            if (g_MemTSODInfo)
                FreePool(g_MemTSODInfo);
            g_MemTSODInfo = NULL;
            g_numTSODModules = 0;
            g_maxTSODModules = 0;

            if (g_TSODController)
                FreePool(g_TSODController);
            g_TSODController = NULL;
            g_numTSODCtrl = 0;
        }
    }
}

void smbios_mem_info(void)
{
    MtSupportDebugWriteLine("Getting SMBIOS Memory Device info...");
    g_numSMBIOSMem = GetNumSMBIOSStruct(E_MEMORY_DEVICE);
    AsciiSPrint(gBuffer, BUF_SIZE, "Found %d Memory Devices", g_numSMBIOSMem);
    MtSupportDebugWriteLine(gBuffer);
    if (g_numSMBIOSMem <= 0)
        return;

    g_SMBIOSMemDevices = (SMBIOS_MEMORY_DEVICE *)AllocateZeroPool(sizeof(SMBIOS_MEMORY_DEVICE) * g_numSMBIOSMem);
    if (g_SMBIOSMemDevices == NULL)
    {
        g_numSMBIOSMem = 0;
        g_numSMBIOSModules = 0;
        MtSupportDebugWriteLine("Failed to allocate memory for SMBIOS memory device info");
        return;
    }

    for (int iModule = 0, SMBInd = 0; iModule < g_numSMBIOSMem; iModule++)
    {
        UINT32 family = CPUID_FAMILY((&cpu_id));
        UINT32 model = CPUID_MODEL((&cpu_id));

        // Workaround for ASUS AM4 motherboards
        // See https://dlcdnets.asus.com/pub/ASUS/mb/SocketAM4/ROG_STRIX_B550-F_GAMING_WIFI_II/E18592_ROG_STRIX_B550-F_GAMING_WI-FI_II_UM_WEB.pdf?model=ROG%20STRIX%20B550-F%20GAMING%20WIFI%20II
        if ((((family == 0x17 &&
               model >= 0x71 && model <= 0x7f) ||
              (family == 0x19 &&
               model >= 0x21 && model <= 0x2f)) && // AMD Ryzen Zen 2
             AsciiStrStr(gBaseBoardInfo.szManufacturer, "ASUSTeK") != NULL) ||
            // Workaround for Gigabyte/MSI Z890 motherboards
            (SysInfo_IsArrowLake(&gCPUInfo) &&
             (AsciiStrStr(gBaseBoardInfo.szManufacturer, "Gigabyte") != NULL ||
              AsciiStrStr(gBaseBoardInfo.szManufacturer, "Micro-Star International") != NULL ||
              AsciiStrStr(gBaseBoardInfo.szManufacturer, "ASRock") != NULL ||
              AsciiStrStr(gBaseBoardInfo.szManufacturer, "ASUSTeK") != NULL ||
              AsciiStrStr(gBaseBoardInfo.szManufacturer, "BIOSTAR") != NULL ||
              AsciiStrStr(gBaseBoardInfo.szManufacturer, "Intel") != NULL)))
        {
            const int DIMM_MAP[] = {2, 3, 0, 1};
            if (g_numSMBIOSMem == ARRAY_SIZE(DIMM_MAP))
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Map %d to SMBIOS index %d (MB=\"%a\")", iModule, DIMM_MAP[iModule], gBaseBoardInfo.szProductName);
                SMBInd = DIMM_MAP[iModule];
            }
        }

        GetMemoryDeviceStruct(NULL, (UINT16)SMBInd, &g_SMBIOSMemDevices[iModule]);

#if 0 // For Intel Arrow Lake Reference Board - no longer needed
      // Workaround for Arrow Lake 
        if (SysInfo_IsArrowLake(&cpu_id))
        {
            if (AsciiStrStr(gBaseBoardInfo.szProductName, "MTL-S UDIMM 2DPC EVCRB") != NULL ||
                AsciiStrStr(gBaseBoardInfo.szProductName, "MTL-S UDIMM 1DPC EVCRB") != NULL)
            {
                // An additional SMBIOS entry is included for every DIMM installed in an 'A' slot.
                // For example, a 2-up A1-A2 configuration will show 6 SMBIOS entries
                // but a 2-up B1-B2 configuration will show 4 SMBIOS entries. 
                if (g_numSMBIOSMem >= 4)
                {
                    g_numSMBIOSMem = 4;
                }
                else
                {
                    g_numSMBIOSMem = 2;
                }
                int DimmsPerChannel = g_numSMBIOSMem >> 1;
                
                int Ch = iModule / DimmsPerChannel;
                int Dimm = iModule % DimmsPerChannel;
                
                CHAR8 szExpDeviceLocator[SMB_STRINGLEN+1];
                AsciiSPrint(szExpDeviceLocator, sizeof(szExpDeviceLocator), "Controller0-Channel%c-DIMM%d", L'A' + Ch, Dimm);                    
                if (AsciiStrStr(g_SMBIOSMemDevices[iModule].szDeviceLocator, szExpDeviceLocator) == NULL)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[Slot %d] Expected DeviceLocator \"%a\", got \"%a\"", iModule, szExpDeviceLocator, g_SMBIOSMemDevices[iModule].szDeviceLocator);
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[Slot %d] Insert empty \"%a\" SMBIOS entry", iModule, szExpDeviceLocator);

                    SetMem(&g_SMBIOSMemDevices[iModule], sizeof(g_SMBIOSMemDevices[iModule]), 0);
                    AsciiStrCpyS(g_SMBIOSMemDevices[iModule].szDeviceLocator, ARRAY_SIZE(g_SMBIOSMemDevices[iModule].szDeviceLocator), szExpDeviceLocator);
                    g_SMBIOSMemDevices[iModule].smb17.MemoryType = MemoryTypeUnknown;

                    // Retrieve SMBIOS entry again on next iteration
                    if (SMBInd > 0) SMBInd--;
                }
            }
        }
#endif
        AsciiSPrint(gBuffer, BUF_SIZE, "[Slot %d] TotalWidth: %d, DataWidth: %d, Size: %d, FormFactor: %d, DeviceSet: %d, MemoryType: %d, Speed: %d, Attributes: %d, ExtendedSize: %d, ConfiguredMemoryClockSpeed: %d, MinimumVoltage: %d, MaximumVoltage: %d, ConfiguredVoltage: %d",
                    iModule,
                    g_SMBIOSMemDevices[iModule].smb17.TotalWidth,
                    g_SMBIOSMemDevices[iModule].smb17.DataWidth,
                    g_SMBIOSMemDevices[iModule].smb17.Size,
                    g_SMBIOSMemDevices[iModule].smb17.FormFactor,
                    g_SMBIOSMemDevices[iModule].smb17.DeviceSet,
                    g_SMBIOSMemDevices[iModule].smb17.MemoryType,
                    g_SMBIOSMemDevices[iModule].smb17.Speed,
                    g_SMBIOSMemDevices[iModule].smb17.Attributes,
                    g_SMBIOSMemDevices[iModule].smb17.ExtendedSize,
                    g_SMBIOSMemDevices[iModule].smb17.ConfiguredMemoryClockSpeed,
                    g_SMBIOSMemDevices[iModule].smb17.MinimumVoltage,
                    g_SMBIOSMemDevices[iModule].smb17.MaximumVoltage,
                    g_SMBIOSMemDevices[iModule].smb17.ConfiguredVoltage);
        MtSupportDebugWriteLine(gBuffer);

        AsciiSPrint(gBuffer, BUF_SIZE, "[Slot %d] DeviceLocator: %a, BankLocator: %a, Manufacturer: %a, S/N: %a, AssetTag: %a, PartNumber: %a",
                    iModule,
                    g_SMBIOSMemDevices[iModule].szDeviceLocator,
                    g_SMBIOSMemDevices[iModule].szBankLocator,
                    g_SMBIOSMemDevices[iModule].szManufacturer,
                    g_SMBIOSMemDevices[iModule].szSerialNumber,
                    g_SMBIOSMemDevices[iModule].szAssetTag,
                    g_SMBIOSMemDevices[iModule].szPartNumber);
        MtSupportDebugWriteLine(gBuffer);

        // Check if empty slot
        if (g_SMBIOSMemDevices[iModule].smb17.Size != 0)
            g_numSMBIOSModules++;

        SMBInd++;
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "Number of non-empty SMBIOS memory devices found: %d",
                g_numSMBIOSModules);
    MtSupportDebugWriteLine(gBuffer);
}

void map_spd_to_smbios(void)
{
    // Map SPD to SMBIOS
    BOOLEAN *SMBIOSFound = NULL;

    if (g_numSMBIOSMem > 0)
        SMBIOSFound = AllocateZeroPool(g_numSMBIOSMem * sizeof(SMBIOSFound[0]));

    for (int i = 0; i < g_numMemModules; i++)
    {
        g_MemoryInfo[i].smbiosInd = -1;
        for (int j = 0; j < g_numSMBIOSMem; j++)
        {
            if (SMBIOSFound[j] == FALSE && g_SMBIOSMemDevices[j].smb17.Size != 0)
            {
                CHAR8 *EndPtr = NULL;
                UINTN SMBSerial;
                AsciiFPrint(DEBUG_FILE_HANDLE, "[SPD %d] Comparing SMBIOS S/N (%a) with SPD S/N (%08x)", i, g_SMBIOSMemDevices[j].szSerialNumber, g_MemoryInfo[i].moduleSerialNo);

                if (AsciiStrHexToUintnS(g_SMBIOSMemDevices[j].szSerialNumber, &EndPtr, &SMBSerial) == EFI_SUCCESS)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[SPD %d] SMBIOS S/N = %08x", i, SMBSerial);
                    if ((UINT32)SMBSerial == (UINT32)g_MemoryInfo[i].moduleSerialNo)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "[SPD %d] Found match with SMBIOS %d", i, j);

                        g_MemoryInfo[i].smbiosInd = j;
                        SMBIOSFound[j] = TRUE;
                        break;
                    }
                }
            }
        }

        // We couldn't match SMBIOS S/N with SPD S/N (possibly due to missing S/N in SMBIOS)
        if (g_MemoryInfo[i].smbiosInd < 0)
            AsciiFPrint(DEBUG_FILE_HANDLE, "[SPD %d] Could not find match for SPD S/N (%08x).", i, g_MemoryInfo[i].moduleSerialNo);
    }

    if (SMBIOSFound)
        FreePool(SMBIOSFound);
}

/* #define TICKS 5 * 11832 (count = 6376)*/
/* #define TICKS (65536 - 12752) */
#define TICKS 59659 /* 50 ms */

DISABLE_OPTIMISATIONS() // Optimisation must be turned off
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
/* Returns CPU clock in khz */
static int cpuspeed(void)
{
    int loops;
    UINT64 st;
    UINT64 end;

    if (cpu_id.fid.bits.rdtsc == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed - rdtsc instruction supported: %d", cpu_id.fid.bits.rdtsc);
        return (-1);
    }

    /* Setup timer */
    IoWrite8(0x61, (IoRead8(0x61) & ~0x02) | 0x01);
    IoWrite8(0x43, 0xb0);
    IoWrite8(0x42, TICKS & 0xff);
    IoWrite8(0x42, TICKS >> 8);

    st = MtSupportReadTSC();

    loops = 0;
    do
    {
        loops++;
    } while ((IoRead8(0x61) & 0x20) == 0);

    end = MtSupportReadTSC();

    AsciiSPrint(gBuffer, BUF_SIZE, "cpuspeed - start ticks: %ld, end ticks: %ld (difference: %ld)", st, end, end - st);
    MtSupportDebugWriteLine(gBuffer);

    end -= st;

    /* Make sure we have a credible result */
    if (loops < 4 || end < 50000)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "cpuspeed - invalid result obtained (loops: %d)", loops);
        MtSupportDebugWriteLine(gBuffer);
        return (-1);
    }

    return ((UINT32)end / 50);
}
#elif defined(MDE_CPU_AARCH64)

#include <Library/ArmGenericTimerCounterLib.h>

static int cpuspeed(void)
{
    UINT64 CounterStart, CounterEnd;
    UINTN CntFreq;
    int loops = 0;
    UINT64 st;
    UINT64 end;

    CntFreq = GetPerformanceCounterProperties(NULL, NULL);

    // Find the number of rdtsc ticks within 50ms
    CounterStart = GetPerformanceCounter();
    CounterEnd = CounterStart + DivU64x32(MultU64x32(CntFreq, 50), 1000); // We loop for 50ms (end tick = start tick + 50ms / period)

    st = MtSupportReadTSC();

    do
    {
        loops++;
    } while (GetPerformanceCounter() < CounterEnd);

    end = MtSupportReadTSC();

    AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed - counter start: %ld, counter end: %ld", CounterStart, CounterEnd);

    AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed - start TSC: %ld, end TSC: %ld (difference: %ld)", st, end, end - st);
    end -= st;

    /* Make sure we have a credible result */
    if (loops < 4 || end < 50000)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed_cntpct - invalid result obtained (loops: %d)", loops);
        return (-1);
    }

    return ((UINT32)end / 50);
}
#endif
ENABLE_OPTIMISATIONS()

#define HPET_GENERAL_CFG_OFF 0x10
#define HPET_MAIN_COUNTER_VAL_OFF 0xF0
#define HPET_MAX_PERIOD 0x05F5E100

int hpet_init(void)
{
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    UINTN Index;

    for (Index = 0; Index < gST->NumberOfTableEntries; Index++)
    {
        if ((CompareGuid(&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpiTableGuid)) ||
            (CompareGuid(&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi20TableGuid)))
        {
            // Look for RSDP (Root System Description Pointer) in the ACPI programming interface.
            if (AsciiStrnCmp("RSD PTR ", (CHAR8 *)(gST->ConfigurationTable[Index].VendorTable), 8) == 0)
            {
                EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *Rsdp = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)gST->ConfigurationTable[Index].VendorTable;
                EFI_ACPI_SDT_HEADER *Xsdt, *Entry;
                UINT32 EntryCount;
                UINT64 *EntryPtr;
                UINT32 Index2;

                AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - Found RSDP. Version: %d ", (int)(Rsdp->Revision));
                MtSupportDebugWriteLine(gBuffer);

                if (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION)
                {
                    Xsdt = (EFI_ACPI_SDT_HEADER *)(UINTN)(Rsdp->XsdtAddress);
                }
                else
                {
                    MtSupportDebugWriteLine("ERROR: No XSDT table found.");
                    return -1;
                }

                if (AsciiStrnCmp("XSDT", (CHAR8 *)(&Xsdt->Signature), 4))
                {
                    MtSupportDebugWriteLine("ERROR: Invalid XSDT table found.");
                    return -1;
                }

                EntryCount = (Xsdt->Length - sizeof(EFI_ACPI_SDT_HEADER)) / sizeof(UINT64);
                AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - Found XSDT. Entry Count: %d", EntryCount);
                MtSupportDebugWriteLine(gBuffer);

                EntryPtr = (UINT64 *)(Xsdt + 1);
                for (Index2 = 0; Index2 < EntryCount; Index2++, EntryPtr++)
                {
                    CHAR8 SigStr[5];

                    SetMem(SigStr, sizeof(SigStr), 0);
                    Entry = (EFI_ACPI_SDT_HEADER *)((UINTN)(*EntryPtr));

                    AsciiStrnCpyS(SigStr, ARRAY_SIZE(SigStr), (const CHAR8 *)&Entry->Signature, 4);
                    AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - Found ACPI table: %a  Version: %d", SigStr, (int)(Entry->Revision));
                    MtSupportDebugWriteLine(gBuffer);

                    // Look for HPET table
                    if (AsciiStrnCmp("HPET", (CHAR8 *)(&Entry->Signature), 4) == 0)
                    {
                        EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER *Hpet = NULL;

                        UINT64 GenCapReg = 0;
                        UINT64 GenCfgReg = 0;

                        // Get the HPET base address
                        Hpet = (EFI_ACPI_HIGH_PRECISION_EVENT_TIMER_TABLE_HEADER *)Entry;
                        sHPETBase = (UINTN)Hpet->BaseAddressLower32Bit.Address;

                        AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - HPET base address: 0x%lx", sHPETBase);
                        MtSupportDebugWriteLine(gBuffer);

                        if (sHPETBase == 0)
                            return -1;

                        // Read the HPET general capabilities register
                        GenCapReg = MmioRead64(sHPETBase);

                        AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - HPET gen cap: 0x%lx (Period: %dfs)", GenCapReg, (UINT32)(RShiftU64(GenCapReg, 32) & 0xFFFFFFFF));
                        MtSupportDebugWriteLine(gBuffer);

                        // Get the timer period
                        sPeriodfs = (RShiftU64(GenCapReg, 32) & 0x0FFFFFFFF);

                        if (sPeriodfs == 0 || sPeriodfs > HPET_MAX_PERIOD)
                        {
                            MtSupportDebugWriteLine("hpet_init - period is invalid");
                            return -1;
                        }

                        // Check if HPET is enabled
                        GenCfgReg = MmioRead64(sHPETBase + HPET_GENERAL_CFG_OFF);

                        AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - HPET gen cfg: 0x%lx", GenCfgReg);
                        MtSupportDebugWriteLine(gBuffer);

                        if ((GenCfgReg & 0x1) == 0)
                        {
                            MtSupportDebugWriteLine("hpet_init - HPET is disabled");
                            return -1;
                        }

                        AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - main counter value (1): %ld", MmioRead64(sHPETBase + HPET_MAIN_COUNTER_VAL_OFF));
                        MtSupportDebugWriteLine(gBuffer);

                        AsciiSPrint(gBuffer, BUF_SIZE, "hpet_init - main counter value (2): %ld", MmioRead64(sHPETBase + HPET_MAIN_COUNTER_VAL_OFF));
                        MtSupportDebugWriteLine(gBuffer);

                        return 0;
                    }
                }
            }
        }
    }
#endif
    return -1;
}

DISABLE_OPTIMISATIONS() // Optimisation must be turned off
int cpuspeed_hpet(void)
{
    if (sHPETBase == 0)
        return -1;

    if (sPeriodfs == 0 || sPeriodfs > HPET_MAX_PERIOD)
        return -1;

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    UINT64 CounterStart, CounterEnd;

    int loops;
    UINT64 st;
    UINT64 end;

    if (cpu_id.fid.bits.rdtsc == 0)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "cpuspeed_hpet - rdtsc instruction supported: %d", cpu_id.fid.bits.rdtsc);
        return (-1);
    }

    st = MtSupportReadTSC();

    // Find the number of rdtsc ticks within 50ms
    CounterStart = MmioRead64(sHPETBase + 0x0F0);
    CounterEnd = CounterStart + DivU64x32(MultU64x32(1000000000000ULL, 50), (UINT32)sPeriodfs); // We loop for 50ms (end tick = start tick + 50ms / period)

    loops = 0;
    do
    {
        loops++;
    } while (MmioRead64(sHPETBase + 0x0F0) < CounterEnd);

    end = MtSupportReadTSC();

    AsciiSPrint(gBuffer, BUF_SIZE, "cpuspeed_hpet - HPET counter start: %ld, HPET counter end: %ld", CounterStart, CounterEnd);
    MtSupportDebugWriteLine(gBuffer);

    AsciiSPrint(gBuffer, BUF_SIZE, "cpuspeed_hpet - start TSC: %ld, end TSC: %ld (difference: %ld)", st, end, end - st);
    MtSupportDebugWriteLine(gBuffer);

    end -= st;

    /* Make sure we have a credible result */
    if (loops < 4 || end < 50000)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "cpuspeed_hpet - invalid result obtained (loops: %d)", loops);
        MtSupportDebugWriteLine(gBuffer);
        return (-1);
    }

    return ((UINT32)end / 50);
#else
    return -1;
#endif
}
ENABLE_OPTIMISATIONS()

BOOLEAN EnableSSE()
{
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
    UINTN Cr0;
    UINTN Cr4;

    Cr0 = AsmReadCr0();
    if (((IA32_CR0 *)&Cr0)->Bits.EM == 1 ||
        ((IA32_CR0 *)&Cr0)->Bits.MP == 0 ||
        ((IA32_CR0 *)&Cr0)->Bits.TS == 1)
    {
        ((IA32_CR0 *)&Cr0)->Bits.EM = 0;
        ((IA32_CR0 *)&Cr0)->Bits.MP = 1;
        ((IA32_CR0 *)&Cr0)->Bits.TS = 0;
        AsmWriteCr0(Cr0);

        Cr0 = AsmReadCr0();
        if (((IA32_CR0 *)&Cr0)->Bits.EM == 1 ||
            ((IA32_CR0 *)&Cr0)->Bits.MP == 0 ||
            ((IA32_CR0 *)&Cr0)->Bits.TS == 1)
            return FALSE;
    }

    Cr4 = AsmReadCr4();
    if (((IA32_CR4 *)&Cr4)->Bits.OSFXSR == 0 ||
        ((IA32_CR4 *)&Cr4)->Bits.OSXMMEXCPT == 0)
    {
        ((IA32_CR4 *)&Cr4)->Bits.OSFXSR = 1;
        ((IA32_CR4 *)&Cr4)->Bits.OSXMMEXCPT = 1;
        AsmWriteCr4(Cr4);

        Cr4 = AsmReadCr4();
        if (((IA32_CR4 *)&Cr4)->Bits.OSFXSR == 0 ||
            ((IA32_CR4 *)&Cr4)->Bits.OSXMMEXCPT == 0)
            return FALSE;
    }
#endif
    return TRUE;
}

VOID BltImageCompositeBadge(IN EG_IMAGE *BaseImage, IN EG_IMAGE *TopImage, IN EG_IMAGE *BadgeImage, IN UINTN XPos, IN UINTN YPos)
{
    UINTN TotalWidth, TotalHeight, CompWidth, CompHeight, OffsetX, OffsetY;
    EG_IMAGE *CompImage;

    if (BaseImage == NULL || TopImage == NULL)
        return;

    CompImage = egCreateFilledImage(BaseImage->Width, BaseImage->Height, TRUE, &BG_COLOUR);
    egComposeImage(CompImage, BaseImage, 0, 0);
    TotalWidth = BaseImage->Width;
    TotalHeight = BaseImage->Height;

    // place the top image
    CompWidth = TopImage->Width;
    if (CompWidth > TotalWidth)
        CompWidth = TotalWidth;
    OffsetX = (TotalWidth - CompWidth) >> 1;
    CompHeight = TopImage->Height;
    if (CompHeight > TotalHeight)
        CompHeight = TotalHeight;
    OffsetY = (TotalHeight - CompHeight) >> 1;
    egComposeImage(CompImage, TopImage, OffsetX, OffsetY);

    // place the badge image
    if (BadgeImage != NULL && (BadgeImage->Width + 8) < CompWidth && (BadgeImage->Height + 8) < CompHeight)
    {
        OffsetX += CompWidth - 8 - BadgeImage->Width;
        OffsetY += CompHeight - 8 - BadgeImage->Height;
        egComposeImage(CompImage, BadgeImage, OffsetX, OffsetY);
    }

    // blit to screen and clean up
    egDrawImage(CompImage, NULL, XPos, YPos);
    egFreeImage(CompImage);
    //    GraphicsScreenDirty = TRUE;
}

UINT16 ShowWelcomeScreen()
{
    EG_RECT BannerPos;
    BOOLEAN HaveUserInput = FALSE;
    MOUSE_EVENT mouseEv;
    int PrevSelBtnIdx = -1, SelBtnIdx = -1;
    UINT64 MenuStartTime;
    UINT16 Return = ID_BUTTON_START;
    CHAR16 TempBuf[128];
    int i;
    BUTTON WelcomeButtons[NUM_WELCOME_BUTTONS]; // Splashscreen buttons

    MtSupportDebugWriteLine("Show splash screen");

    // Load images
    EG_IMAGE *Banner = egDecodePNG(mt86logo_png_bin, sizeof(mt86logo_png_bin), 128, TRUE);
    if (Banner == NULL)
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Could not load MemTest86 logo image");
        MtSupportDebugWriteLine(gBuffer);
    }

    for (i = 0; i < NUM_WELCOME_BUTTONS; i++)
    {
        WelcomeButtons[i].Caption = STRING_TOKEN(WELCOME_BTN_CAPTION_STR_ID[i]);
        WelcomeButtons[i].Image = egDecodePNG(WELCOME_BTN_BIN[i], WELCOME_BTN_BIN_SIZE[i], 128, TRUE);
        if (WelcomeButtons[i].Image == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load splash screen icon %d", i);
            MtSupportDebugWriteLine(gBuffer);
        }
    }
    WelcomeButtons[0].Id = ID_BUTTON_EXIT;
    WelcomeButtons[1].Id = ID_BUTTON_SYSINFO;

    egClearScreen(&BG_COLOUR);

    if (Banner)
    {
        // Draw banner
        BannerPos.XPos = (ScreenWidth - Banner->Width) / 2;
        BannerPos.YPos = (ScreenHeight - Banner->Height) / 2;
        BannerPos.Width = Banner->Width;
        BannerPos.Height = Banner->Height;

        egDrawImage(Banner, &BG_COLOUR, BannerPos.XPos, BannerPos.YPos);

        if (WelcomeButtons[0].Image && WelcomeButtons[1].Image && SelectionOverlay)
        {
            // Draw buttons
            for (i = 0; i < NUM_WELCOME_BUTTONS; i++)
            {
                WelcomeButtons[i].ScreenPos.Height = WelcomeButtons[i].Image->Height;
                WelcomeButtons[i].ScreenPos.Width = WelcomeButtons[i].Image->Width;

                WelcomeButtons[i].ScreenPos.XPos = i == 0 ? BannerPos.XPos : BannerPos.XPos + BannerPos.Width - WelcomeButtons[i].Image->Width;
                WelcomeButtons[i].ScreenPos.YPos = BannerPos.YPos + BannerPos.Height + 5;

                egDrawImage(WelcomeButtons[i].Image, &BG_COLOUR, WelcomeButtons[i].ScreenPos.XPos, WelcomeButtons[i].ScreenPos.YPos);
                PrintXYAlign(WelcomeButtons[i].ScreenPos.XPos + WelcomeButtons[i].ScreenPos.Width / 2, WelcomeButtons[i].ScreenPos.YPos + WelcomeButtons[i].ScreenPos.Height + 5, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(WelcomeButtons[i].Caption, TempBuf, sizeof(TempBuf)));
            }

            PrintXYAlign(ScreenWidth / 2, 5 + gTextHeight + 2, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_SPLASH_INSTR1), TempBuf, sizeof(TempBuf)));
            PrintXYAlign(ScreenWidth / 2, 5 + 2 * (gTextHeight + 2), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_SPLASH_INSTR2), TempBuf, sizeof(TempBuf)));
        }
    }

    if (MouseBirth() != EFI_SUCCESS)
        MtSupportDebugWriteLine("Warning - no mouse detected");

    MenuStartTime = MtSupportReadTSC();

    AsciiFPrint(DEBUG_FILE_HANDLE, "Splash screen start time: %ld (clk=%d, ConIn=%p, WaitForKey=%p)", MenuStartTime, clks_msec, gST->ConIn, gST->ConIn ? gST->ConIn->WaitForKey : 0);

    while (TRUE)
    {
        UINTN curX, curY;
        mouseEv = NoEvents;

        // Only check for mouse and keyboard events if our images loaded correctly
        if (Banner && WelcomeButtons[0].Image && WelcomeButtons[1].Image && SelectionOverlay)
        {
            EFI_INPUT_KEY Key;
            SetMem(&Key, sizeof(Key), 0);

            if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Splash screen key press detected: %d (%d)", Key.UnicodeChar, Key.ScanCode);
                if (Key.ScanCode == 0x4 || Key.ScanCode == 0x3)
                {
                    if (Key.ScanCode == 0x4)
                        SelBtnIdx = SelBtnIdx <= 0 ? NUM_WELCOME_BUTTONS - 1 : SelBtnIdx - 1;
                    else if (Key.ScanCode == 0x3)
                        SelBtnIdx = SelBtnIdx < 0 ? 0 : (SelBtnIdx + 1) % NUM_WELCOME_BUTTONS;
                }
                else if (Key.UnicodeChar == 13)
                {
                    if (SelBtnIdx >= 0 && SelBtnIdx < NUM_WELCOME_BUTTONS)
                    {
                        Return = (UINT16)WelcomeButtons[SelBtnIdx].Id;
                        goto MenuExit;
                    }
                }

                if (HaveUserInput == FALSE)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "Splash screen user input detected - stopping timer");
                    HaveUserInput = TRUE;

                    ClearTextXYAlign(ScreenWidth / 2, 5, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_SPLASH_COUNTER), TempBuf, sizeof(TempBuf)), DivU64x32(COUNTDOWN_TIME_MS, 1000), ModU64x32(COUNTDOWN_TIME_MS, 1000) / 100);
                }
            }

            mouseEv = WaitForMouseEvent(&curX, &curY, 10);
            if (mouseEv != NoEvents && HaveUserInput == FALSE)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Splash screen mouse event detected: %d (x=%d, y=%d)", mouseEv, curX, curY);
                HaveUserInput = TRUE;

                ClearTextXYAlign(ScreenWidth / 2, 5, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_SPLASH_COUNTER), TempBuf, sizeof(TempBuf)), DivU64x32(COUNTDOWN_TIME_MS, 1000), ModU64x32(COUNTDOWN_TIME_MS, 1000) / 100);
            }
        }

        // If no mouse events have been detected, decrement the countdown timer
        if (HaveUserInput == FALSE)
        {
            UINT64 ElapsedTimeMs;
            ElapsedTimeMs = DivU64x64Remainder(MtSupportReadTSC() - MenuStartTime, clks_msec, NULL);

            if (ElapsedTimeMs > COUNTDOWN_TIME_MS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Splash screen countdown timer expired");
                Return = ID_BUTTON_START;
                goto MenuExit;
            }

            PrintXYAlign(ScreenWidth / 2, 5, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_SPLASH_COUNTER), TempBuf, sizeof(TempBuf)), DivU64x32(COUNTDOWN_TIME_MS - ElapsedTimeMs, 1000), ModU64x32(COUNTDOWN_TIME_MS - ElapsedTimeMs, 1000) / 100);
        }

        switch (mouseEv)
        {
        case MouseMove:
        {
            HidePointer();

            SelBtnIdx = -1;

            // Check if mouse over button
            for (i = 0; i < NUM_WELCOME_BUTTONS; i++)
            {
                if (MouseInRect(&WelcomeButtons[i].ScreenPos))
                {
                    SelBtnIdx = i;
                    break;
                }
            }

            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            // Check if mouse over button
            for (i = 0; i < NUM_WELCOME_BUTTONS; i++)
            {
                if (MouseInRect(&WelcomeButtons[i].ScreenPos))
                {
                    Return = WelcomeButtons[i].Id;
                    goto MenuExit;
                }
            }
        }
        break;

        default:
            break;
        }

        if (PrevSelBtnIdx != SelBtnIdx)
        {
            HidePointer();
            // Clear old selection
            if (PrevSelBtnIdx >= 0 && PrevSelBtnIdx < NUM_WELCOME_BUTTONS)
                egDrawImage(WelcomeButtons[PrevSelBtnIdx].Image, &BG_COLOUR, WelcomeButtons[PrevSelBtnIdx].ScreenPos.XPos, WelcomeButtons[PrevSelBtnIdx].ScreenPos.YPos);

            // Set new selection
            if (SelBtnIdx >= 0 && SelBtnIdx < NUM_WELCOME_BUTTONS)
                BltImageCompositeBadge(WelcomeButtons[SelBtnIdx].Image, SelectionOverlay, NULL, WelcomeButtons[SelBtnIdx].ScreenPos.XPos, WelcomeButtons[SelBtnIdx].ScreenPos.YPos);

            PrevSelBtnIdx = SelBtnIdx;
            DrawPointer();
        }

        gBS->Stall(1000);
    }

MenuExit:
    KillMouse();
    egClearScreen(&BG_COLOUR);

    for (i = 0; i < NUM_WELCOME_BUTTONS; i++)
    {
        if (WelcomeButtons[i].Image)
            egFreeImage(WelcomeButtons[i].Image);
    }

    if (Banner)
        egFreeImage(Banner);

    AsciiFPrint(DEBUG_FILE_HANDLE, "Exit splash screen");

    return Return;
}

UINTN ExitScreen()
{
    BOOLEAN HaveUserInput = FALSE;
    MOUSE_EVENT mouseEv;
    int PrevSelBtnIdx = -1, SelBtnIdx = -1;
    UINT64 MenuStartTime;
    UINT16 Return = EXITMODE_REBOOT;
    CHAR16 TempBuf[128];
    BUTTON ExitButtons[NUM_EXIT_BUTTONS];
    int i;

    if (gConsoleOnly)
        return EXITMODE_EXIT;

    egClearScreen(&BG_COLOUR);

    for (i = 0; i < NUM_EXIT_BUTTONS; i++)
    {
        ExitButtons[i].Caption = STRING_TOKEN(EXIT_BTN_CAPTION_STR_ID[i]);
        ExitButtons[i].Image = egDecodePNG(EXIT_BTN_BIN[i], EXIT_BTN_BIN_SIZE[i], 128, TRUE);
        if (ExitButtons[i].Image != NULL)
        {
            ExitButtons[i].ScreenPos.Height = ExitButtons[i].Image->Height;
            ExitButtons[i].ScreenPos.Width = ExitButtons[i].Image->Width;

            if (i == 0)
                ExitButtons[i].ScreenPos.XPos = ScreenWidth / 2 - ExitButtons[i].ScreenPos.Width * 3;
            else if (i == 1)
                ExitButtons[i].ScreenPos.XPos = (ScreenWidth - ExitButtons[i].ScreenPos.Width) / 2;
            else
                ExitButtons[i].ScreenPos.XPos = ScreenWidth / 2 + ExitButtons[i].ScreenPos.Width * 2;

            ExitButtons[i].ScreenPos.YPos = (ScreenHeight - ExitButtons[i].ScreenPos.Height) / 2;

            egDrawImage(ExitButtons[i].Image, &BG_COLOUR, ExitButtons[i].ScreenPos.XPos, ExitButtons[i].ScreenPos.YPos);
            PrintXYAlign(ExitButtons[i].ScreenPos.XPos + ExitButtons[i].ScreenPos.Width / 2, ExitButtons[i].ScreenPos.YPos + ExitButtons[i].ScreenPos.Height + 5, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(ExitButtons[i].Caption, TempBuf, sizeof(TempBuf)));
        }
        else
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load exit icon %d", i);
            MtSupportDebugWriteLine(gBuffer);
        }
    }

    ExitButtons[0].Id = EXITMODE_REBOOT;
    ExitButtons[1].Id = EXITMODE_SHUTDOWN;
    ExitButtons[2].Id = EXITMODE_EXIT;

    if (MouseBirth() != EFI_SUCCESS)
        MtSupportDebugWriteLine("Warning - no mouse detected");

    MenuStartTime = MtSupportReadTSC();
    while (TRUE)
    {
        UINTN curX, curY;
        mouseEv = NoEvents;

        // Only check for mouse and keyboard events if our images loaded correctly
        if (ExitButtons[0].Image && ExitButtons[1].Image && ExitButtons[2].Image && SelectionOverlay)
        {
            EFI_INPUT_KEY Key;
            SetMem(&Key, sizeof(Key), 0);

            if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
            {
                if (Key.ScanCode == 0x4 || Key.ScanCode == 0x3)
                {
                    if (Key.ScanCode == 0x4)
                        SelBtnIdx = SelBtnIdx <= 0 ? NUM_EXIT_BUTTONS - 1 : SelBtnIdx - 1;
                    else if (Key.ScanCode == 0x3)
                        SelBtnIdx = SelBtnIdx < 0 ? 0 : (SelBtnIdx + 1) % NUM_EXIT_BUTTONS;
                }
                else if (Key.UnicodeChar == 13)
                {
                    if (SelBtnIdx >= 0 && SelBtnIdx < NUM_EXIT_BUTTONS)
                    {
                        Return = (UINT16)ExitButtons[SelBtnIdx].Id;
                        goto MenuExit;
                    }
                }

                if (HaveUserInput == FALSE)
                {
                    HaveUserInput = TRUE;

                    ClearTextXYAlign(ScreenWidth / 2, ScreenHeight / 3, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_EXIT_COUNTER), TempBuf, sizeof(TempBuf)), DivU64x32(COUNTDOWN_TIME_MS, 1000), ModU64x32(COUNTDOWN_TIME_MS, 1000) / 100);
                }
            }
            mouseEv = WaitForMouseEvent(&curX, &curY, 10);
            if (mouseEv != NoEvents && HaveUserInput == FALSE)
            {
                HaveUserInput = TRUE;

                ClearTextXYAlign(ScreenWidth / 2, ScreenHeight / 3, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_EXIT_COUNTER), TempBuf, sizeof(TempBuf)), DivU64x32(COUNTDOWN_TIME_MS, 1000), ModU64x32(COUNTDOWN_TIME_MS, 1000) / 100);
            }
        }

        // If no mouse events have been detected, decrement the countdown timer
        if (HaveUserInput == FALSE)
        {
            UINT64 ElapsedTimeMs;
            ElapsedTimeMs = DivU64x64Remainder(MtSupportReadTSC() - MenuStartTime, clks_msec, NULL);

            if (ElapsedTimeMs > COUNTDOWN_TIME_MS)
                goto MenuExit;

            PrintXYAlign(ScreenWidth / 2, ScreenHeight / 3, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_EXIT_COUNTER), TempBuf, sizeof(TempBuf)), DivU64x32(COUNTDOWN_TIME_MS - ElapsedTimeMs, 1000), ModU64x32(COUNTDOWN_TIME_MS - ElapsedTimeMs, 1000) / 100);
        }

        switch (mouseEv)
        {
        case MouseMove:
        {
            HidePointer();

            SelBtnIdx = -1;

            // Check if mouse over button
            for (i = 0; i < NUM_EXIT_BUTTONS; i++)
            {
                if (MouseInRect(&ExitButtons[i].ScreenPos))
                {
                    SelBtnIdx = i;
                    break;
                }
            }

            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            // Check if mouse over button
            for (i = 0; i < NUM_EXIT_BUTTONS; i++)
            {
                if (MouseInRect(&ExitButtons[i].ScreenPos))
                {
                    Return = ExitButtons[i].Id;
                    goto MenuExit;
                }
            }
        }
        break;

        default:
            break;
        }

        if (PrevSelBtnIdx != SelBtnIdx)
        {
            HidePointer();

            // Clear old selection
            if (PrevSelBtnIdx >= 0 && PrevSelBtnIdx < NUM_EXIT_BUTTONS)
                egDrawImage(ExitButtons[PrevSelBtnIdx].Image, &BG_COLOUR, ExitButtons[PrevSelBtnIdx].ScreenPos.XPos, ExitButtons[PrevSelBtnIdx].ScreenPos.YPos);

            if (SelBtnIdx >= 0 && SelBtnIdx < NUM_EXIT_BUTTONS)
                BltImageCompositeBadge(ExitButtons[SelBtnIdx].Image, SelectionOverlay, NULL, ExitButtons[SelBtnIdx].ScreenPos.XPos, ExitButtons[SelBtnIdx].ScreenPos.YPos);

            PrevSelBtnIdx = SelBtnIdx;
            DrawPointer();
        }

        gBS->Stall(1000);
    }

MenuExit:
    KillMouse();
    egClearScreen(&BG_COLOUR);

    for (i = 0; i < NUM_EXIT_BUTTONS; i++)
    {
        if (ExitButtons[i].Image)
            egFreeImage(ExitButtons[i].Image);
    }

    return Return;
}

int MouseMoveSideBar()
{
    int i;
    static UINT sPrevSelBtnId = (UINT)-1;

    UINT SelBtnId = (UINT)-1;

    // Check if mouse over button
    for (i = 0; i < NUM_SIDEBAR_BUTTONS; i++)
    {
        if (MouseInRect(&SidebarButtons[i].ScreenPos))
        {
            SelBtnId = SidebarButtons[i].Id;
            break;
        }
    }

    if (sPrevSelBtnId == SelBtnId)
        return SelBtnId;

    HidePointer();

    // Un-select previous selection
    if (sPrevSelBtnId >= 0 && sPrevSelBtnId < NUM_SIDEBAR_BUTTONS)
    {
        egDrawImage(SidebarButtons[sPrevSelBtnId].Image, &BG_COLOUR, SidebarButtons[sPrevSelBtnId].ScreenPos.XPos, SidebarButtons[sPrevSelBtnId].ScreenPos.YPos);
    }

    // Check if mouse over button
    if (SelBtnId >= 0 && SelBtnId < NUM_SIDEBAR_BUTTONS)
    {
        BltImageCompositeBadge(SidebarButtons[SelBtnId].Image, SelectionOverlay, NULL, SidebarButtons[SelBtnId].ScreenPos.XPos, SidebarButtons[SelBtnId].ScreenPos.YPos);
    }

    DrawPointer();

    sPrevSelBtnId = SelBtnId;
    return sPrevSelBtnId;
}

int MouseClickSideBar()
{
    int i;

    // Check if mouse over button
    for (i = 0; i < NUM_SIDEBAR_BUTTONS; i++)
    {
        if (MouseInRect(&SidebarButtons[i].ScreenPos))
        {
            return SidebarButtons[i].Id;
        }
    }
    return -1;
}

int KeyPressSideBar(EFI_INPUT_KEY *Key)
{
    switch (Key->UnicodeChar)
    {
    case L'i':
        return ID_BUTTON_SYSINFO;
    case L't':
        return ID_BUTTON_TESTSELECT;
    case L'c':
        return ID_BUTTON_CPUSELECT;
    case L'a':
        return ID_BUTTON_MEMADDR;
    case L's':
        return ID_BUTTON_START;
    case L'r':
        return ID_BUTTON_BENCHMARK;
    case 'e':
        return ID_BUTTON_SETTINGS;
#ifndef PRO_RELEASE
    case 'u':
        return ID_BUTTON_UPGRADE;
#endif
    case L'x':
        return ID_BUTTON_EXIT;
    default:
        break;
    }

    switch (Key->ScanCode)
    {
    case 0x16: // F12
    {
        MT_HANDLE FileHandle = NULL;
        EFI_STATUS Status;
        EFI_TIME Time;
        CHAR16 Filename[128];

        gRT->GetTime(&Time, NULL);

        UnicodeSPrint(Filename, sizeof(Filename), L"%s-%s.bmp", SCREENSHOT_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

        AsciiFPrint(DEBUG_FILE_HANDLE, "Saving screenshot to %s", Filename);
        Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
        if (FileHandle == NULL)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to open file \"%s\" for writing: %r", Filename, Status);
        }
        else
        {
            EG_PIXEL WHITE_PIXEL = {255, 255, 255, 0};
            EG_IMAGE *pScreenImg = egCopyScreen();

            egSaveImage(FileHandle, pScreenImg);
            MtSupportCloseFile(FileHandle);
            FileHandle = NULL;
            AsciiFPrint(DEBUG_FILE_HANDLE, "Screenshot successfully saved to \"%s\"", Filename);

            // Flash to indicate screen capture
            egClearScreen(&WHITE_PIXEL);
            gBS->Stall(100000);
            egDrawImage(pScreenImg, NULL, 0, 0);

            egFreeImage(pScreenImg);
        }
    }
    break;

    default:
        break;
    }
    return -1;
}

STATIC VOID DrawHeader()
{
    CHAR16 TempBuf[128];

    // Draw header
    if (HeaderLogo)
    {
        egDrawImage(HeaderLogo, &BG_COLOUR, 5, 5);
    }

    PrintXYAlign(ScreenWidth - 5, 5, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, GetStringById(STRING_TOKEN(STR_SCREENSHOT), TempBuf, sizeof(TempBuf)));
}

STATIC VOID DrawFooter()
{
    PrintXYAlign(5, WindowHeight - gTextHeight, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, PROGRAM_NAME L" " PROGRAM_VERSION);
}

STATIC VOID DrawSideBar()
{
    int i;
    CHAR16 TempBuf[128];

    for (i = 0; i < NUM_SIDEBAR_BUTTONS; i++)
    {
        // Draw sidebar
        if (SidebarButtons[i].Image)
        {
            // Draw buttons
            egDrawImage(SidebarButtons[i].Image, &BG_COLOUR, SidebarButtons[i].ScreenPos.XPos, SidebarButtons[i].ScreenPos.YPos);
        }
        PrintXYAlign(SidebarButtons[i].ScreenPos.XPos + SidebarButtons[i].ScreenPos.Width + 5, SidebarButtons[i].ScreenPos.YPos + ((SidebarButtons[i].ScreenPos.Height - gTextHeight) >> 1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(SidebarButtons[i].Caption, TempBuf, sizeof(TempBuf)));
    }
}

STATIC VOID DrawWindow()
{
    UINTN TextWidth = 0;
    CHAR16 TempBuf[128];
    int i;

    egClearScreen(&BG_COLOUR);
    DrawHeader();
    DrawFooter();
    DrawSideBar();

    for (i = 0; i < NUM_SIDEBAR_BUTTONS; i++)
    {
        UINTN LineWidth = 0;
        GetTextInfo(GetStringById(SidebarButtons[i].Caption, TempBuf, sizeof(TempBuf)), &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    gMainFrameTop = HeaderLogo ? HeaderLogo->Height + 5 : 64 + 5;
    gMainFrameBottom = ScreenHeight;

    gMainFrameLeft = SidebarButtons[0].ScreenPos.XPos + SidebarButtons[0].ScreenPos.Width + TextWidth + 5;
    gMainFrameRight = ScreenWidth;
}

STATIC VOID SetScreenRes(UINTN Mode)
{
    UINTN NewWidth = (UINTN)Mode;
    UINTN NewHeight = 0;
    egGetResFromMode(&NewWidth, &NewHeight);

    if (Mode == ScreenMode && NewWidth == ScreenWidth && NewHeight == ScreenHeight)
        return;

    AsciiSPrint(gBuffer, BUF_SIZE, "Setting graphics mode to: %d [%d x %d]", Mode, NewWidth, NewHeight);
    MtSupportDebugWriteLine(gBuffer);

    NewWidth = (UINTN)Mode;
    NewHeight = 0;
    if (egSetScreenSize(&NewWidth, &NewHeight))
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Successfully set graphics mode to: %d", Mode);
        MtSupportDebugWriteLine(gBuffer);

        ScreenMode = Mode;
        egGetScreenSize(&ScreenWidth, &ScreenHeight);

        AsciiSPrint(gBuffer, BUF_SIZE, "New screen size = %d x %d", ScreenWidth, ScreenHeight);
        MtSupportDebugWriteLine(gBuffer);
    }
    else
    {
        AsciiSPrint(gBuffer, BUF_SIZE, "Failed to set graphics mode to: %d", Mode);
        MtSupportDebugWriteLine(gBuffer);
    }
}

STATIC VOID AutoSetScreenRes()
{
    if (ScreenWidth < MIN_SCREEN_WIDTH || ScreenWidth > MAX_SCREEN_WIDTH || ScreenHeight < MIN_SCREEN_HEIGHT || ScreenHeight > MAX_SCREEN_HEIGHT)
    {
        if (ScreenWidth < MIN_SCREEN_WIDTH || ScreenHeight < MIN_SCREEN_HEIGHT)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Screen resolution is too low (%d x %d). Attempting to set new screen size.", ScreenWidth, ScreenHeight);
        }
        else if (ScreenWidth > MAX_SCREEN_WIDTH || ScreenHeight > MAX_SCREEN_HEIGHT)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Screen resolution is too high (%d x %d). Attempting to set new screen size.", ScreenWidth, ScreenHeight);
        }

        MtSupportDebugWriteLine(gBuffer);

        if (!gFixedScreenRes)
        {
            UINT32 MaxGraphicsMode = egGetNumModes();
            INT32 BestMode = -1;
            UINTN BestWidth = 0;
            UINTN BestHeight = 0;
            UINT32 Mode;

            // Find the best graphics mode
            for (Mode = 0; Mode < MaxGraphicsMode; Mode++)
            {
                UINTN Width = (UINTN)Mode;
                UINTN Height = 0;
                egGetResFromMode(&Width, &Height);

                if (Width >= MIN_SCREEN_WIDTH && Width <= MAX_SCREEN_WIDTH &&
                    Height >= MIN_SCREEN_HEIGHT && Height <= MAX_SCREEN_HEIGHT)
                {
                    AsciiSPrint(gBuffer, BUF_SIZE, "Found eligible graphics mode: %d [%d x %d]", Mode, Width, Height);
                    MtSupportDebugWriteLine(gBuffer);

                    if (BestMode < 0 ||
                        (Width < BestWidth &&
                         Height < BestHeight))
                    {
                        BestMode = Mode;
                        BestWidth = Width;
                        BestHeight = Height;
                    }
                }
            }

            if (BestMode >= 0 && BestMode < (INT32)MaxGraphicsMode)
            {
                SetScreenRes(BestMode);
            }
        }
        else
        {
            MtSupportDebugWriteLine("Screen size not set due to blacklisted baseboard");
        }
    }
}

STATIC VOID EnableGraphicsMode(BOOLEAN Enable)
{
    if (Enable)
    {
        MtSupportDebugWriteLine("Enabling graphics mode");

        gST->ConOut->ClearScreen(gST->ConOut);
        if (gConCtrlWorkaround)
            egSetGraphicsModeEnabled(Enable);
        egClearScreen(&BG_COLOUR);

        MtSupportDebugWriteLine("Get screen size");

        egGetScreenSize(&ScreenWidth, &ScreenHeight);

        AsciiSPrint(gBuffer, BUF_SIZE, "Current screen size: %d x %d", ScreenWidth, ScreenHeight);
        MtSupportDebugWriteLine(gBuffer);

        SetScreenRes(ScreenMode);
    }
    else
    {
        egClearScreen(&BG_COLOUR);
        if (gConCtrlWorkaround)
            egSetGraphicsModeEnabled(Enable);
        gST->ConOut->ClearScreen(gST->ConOut);
        gST->ConOut->EnableCursor(gST->ConOut, FALSE);
    }
}
UINT16 MainMenu()
{
    UINT16 Cmd = ID_BUTTON_SYSINFO;
    while (TRUE) {
        if (gOKnSkipWaiting) {
            if (gOknTestStart) {
                Cmd = ID_BUTTON_START;
                gOknTestStart = FALSE;
                gOknTestStatus = 1;
                break;
            }
            if (gOknTestStatus == 0) {

            }
            if (gOknTestReset == 0 || gOknTestReset == 1 || gOknTestReset == 2) {
                gRT->ResetSystem(gOknTestReset, 0, 0, NULL);
            }
        }
        gBS->CheckEvent(gST->ConIn->WaitForKey);
    }
    return Cmd;

    DrawWindow();

    if (MouseBirth() != EFI_SUCCESS)
        MtSupportDebugWriteLine("Warning - no mouse detected");

    HidePointer();

    while (TRUE)
    {
        UINT16 NewCmd = ID_BUTTON_SYSINFO;

        if (Cmd == ID_BUTTON_SPDINFO)
            egClearScreenArea(&LINE_COLOUR_PIXEL, SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.XPos, SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.YPos + SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.Height + 5, gMainFrameLeft - SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.XPos, 1);
        else if (Cmd == ID_BUTTON_BENCHRESULTS || Cmd == ID_BUTTON_PREVBENCHRESULTS)
            egClearScreenArea(&LINE_COLOUR_PIXEL, SidebarButtons[ID_BUTTON_BENCHMARK].ScreenPos.XPos, SidebarButtons[ID_BUTTON_BENCHMARK].ScreenPos.YPos + SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.Height + 5, gMainFrameLeft - SidebarButtons[ID_BUTTON_BENCHMARK].ScreenPos.XPos, 1);
        else
            egClearScreenArea(&LINE_COLOUR_PIXEL, SidebarButtons[Cmd].ScreenPos.XPos, SidebarButtons[Cmd].ScreenPos.YPos + SidebarButtons[Cmd].ScreenPos.Height + 5, gMainFrameLeft - SidebarButtons[Cmd].ScreenPos.XPos, 1);

        DrawPointer();

        if (Cmd == ID_BUTTON_SYSINFO)
            NewCmd = SysInfoScreen(gMainFrameLeft, gMainFrameTop); // Cmd = MainConfigScreen();
        else if (Cmd == ID_BUTTON_SPDINFO)
            NewCmd = SPDInfoScreen(gMainFrameLeft, gMainFrameTop);
        else if (Cmd == ID_BUTTON_MEMADDR)
            NewCmd = AddressRangeScreen(gMainFrameLeft, gMainFrameTop);
        else if (Cmd == ID_BUTTON_TESTSELECT)
            NewCmd = TestSelectScreen(gMainFrameLeft, gMainFrameTop);
        else if (Cmd == ID_BUTTON_CPUSELECT)
            NewCmd = CPUSelectScreen(gMainFrameLeft, gMainFrameTop);
        else if (Cmd == ID_BUTTON_BENCHMARK)
            NewCmd = RAMBenchmarkScreen(gMainFrameLeft, gMainFrameTop);
        else if (Cmd == ID_BUTTON_BENCHRESULTS)
            NewCmd = BenchmarkResultsScreen(gMainFrameLeft, gMainFrameTop);
        else if (Cmd == ID_BUTTON_PREVBENCHRESULTS)
            NewCmd = PrevBenchmarkResultsScreen(gMainFrameLeft, gMainFrameTop);
        else if (Cmd == ID_BUTTON_SETTINGS)
            NewCmd = GeneralSettingsScreen(gMainFrameLeft, gMainFrameTop);
#ifndef PRO_RELEASE
        else if (Cmd == ID_BUTTON_UPGRADE)
            NewCmd = UpgradeProScreen(gMainFrameLeft, gMainFrameTop);
#endif
        else if (Cmd == ID_BUTTON_START || Cmd == ID_BUTTON_EXIT)
            break;

        HidePointer();

        if (Cmd == ID_BUTTON_SPDINFO)
            egClearScreenArea(&BG_COLOUR, SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.XPos, SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.YPos + SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.Height + 5, gMainFrameLeft - SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.XPos, 1);
        else if (Cmd == ID_BUTTON_BENCHRESULTS || Cmd == ID_BUTTON_PREVBENCHRESULTS)
            egClearScreenArea(&BG_COLOUR, SidebarButtons[ID_BUTTON_BENCHMARK].ScreenPos.XPos, SidebarButtons[ID_BUTTON_BENCHMARK].ScreenPos.YPos + SidebarButtons[ID_BUTTON_SYSINFO].ScreenPos.Height + 5, gMainFrameLeft - SidebarButtons[ID_BUTTON_BENCHMARK].ScreenPos.XPos, 1);
        else
            egClearScreenArea(&BG_COLOUR, SidebarButtons[Cmd].ScreenPos.XPos, SidebarButtons[Cmd].ScreenPos.YPos + SidebarButtons[Cmd].ScreenPos.Height + 5, gMainFrameLeft - SidebarButtons[Cmd].ScreenPos.XPos, 1);

        egClearScreenArea(&BG_COLOUR, gMainFrameLeft, gMainFrameTop, gMainFrameRight - gMainFrameLeft, gMainFrameBottom - gMainFrameTop);
        Cmd = NewCmd;
    }

    KillMouse();
    egClearScreen(&BG_COLOUR);
    return Cmd;
}

#define MAIN_FRAME_LEFT_OFF 15
#define MAIN_FRAME_TOP_OFF 0
#define LINE_SPACING (gTextHeight + 7)
#define LINENO_YPOS(lineno) (TopOffset + (LINE_SPACING * (lineno)))

UINT16
TestSelectScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    UINTN NumTestsSel = 0;
    MOUSE_EVENT mouseEv;
    EG_RECT TestRect;
    EG_RECT OptionsRect;
    int NumOpts = 0;
    CHAR16 TempBuf[128];

    HidePointer();

    // Print instructions at the top
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(0), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_TEST_INSTR1), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_TEST_INSTR2), TempBuf, sizeof(TempBuf)));

    // Find the longest text in the test list
    for (i = 0; i < gNumCustomTests; i++)
    {
        UINTN LineWidth = 0;
        GetTextInfo(gCustomTestList[i].NameStr ? gCustomTestList[i].NameStr : GetStringById(gCustomTestList[i].NameStrId, TempBuf, sizeof(TempBuf)), &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    // Print the test list
    for (i = 0; i < gNumCustomTests; i++)
    {
        DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(4 + i), gCustomTestList[i].NameStr ? gCustomTestList[i].NameStr : GetStringById(gCustomTestList[i].NameStrId, TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

        if (gCustomTestList[i].Enabled)
        {
            PrintXYAlign(LeftOffset, LINENO_YPOS(4 + i), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"*");
            NumTestsSel++;
        }
        else
            PrintXYAlign(LeftOffset, LINENO_YPOS(4 + i), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L" ");
    }

    // Set the rectangle of the test list for mouse over detection
    TestRect.XPos = LeftOffset + gCharWidth; // (ScreenWidth - TextWidth) >> 1;
    TestRect.YPos = LINENO_YPOS(4);
    TestRect.Width = TextWidth;
    TestRect.Height = gNumCustomTests * LINE_SPACING;

    NumOpts = gNumCustomTests + 1;

    // Test selection options
    UnicodeSPrint(sTestSelOptText[0], sizeof(sTestSelOptText[0]), GetStringById(STRING_TOKEN(STR_MENU_TEST_PASSES), TempBuf, sizeof(TempBuf)), gNumPasses);

    GetTextInfo(sTestSelOptText[0], &TextWidth, NULL);

    OptionsRect.XPos = LeftOffset;
    OptionsRect.YPos = LINENO_YPOS(4 + gNumCustomTests + 1);
    OptionsRect.Width = TextWidth;
    OptionsRect.Height = LINE_SPACING;

    DisplayMenuOption(LeftOffset, OptionsRect.YPos, sTestSelOptText[0], &DEFAULT_TEXT_PIXEL, SelIdx >= gNumCustomTests, TextWidth);

    DrawPointer();

    // Main loop to handle user input
    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NumOpts - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % NumOpts;
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0 && SelIdx < NumOpts)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();
            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&TestRect))
            {
                SelIdx = (int)((curY - TestRect.YPos) / LINE_SPACING);
            }
            else if (MouseInRect(&OptionsRect))
            {
                SelIdx = gNumCustomTests;
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            if (SelIdx >= 0 && SelIdx < NumOpts)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();

            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NumOpts)
            {
                if (PrevSelIdx < gNumCustomTests)
                    DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(4 + PrevSelIdx), gCustomTestList[PrevSelIdx].NameStr ? gCustomTestList[PrevSelIdx].NameStr : GetStringById(gCustomTestList[PrevSelIdx].NameStrId, TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, FALSE, TestRect.Width);
                else
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + gNumCustomTests + 1), sTestSelOptText[0], &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect.Width);
            }

            // Set selection for new index
            if (SelIdx >= 0 && SelIdx < NumOpts)
            {
                if (SelIdx < gNumCustomTests)
                    DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(4 + SelIdx), gCustomTestList[SelIdx].NameStr ? gCustomTestList[SelIdx].NameStr : GetStringById(gCustomTestList[SelIdx].NameStrId, TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, TRUE, TestRect.Width);
                else
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + gNumCustomTests + 1), sTestSelOptText[0], &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect.Width);
            }

            PrevSelIdx = SelIdx;
            DrawPointer();
        }

        if (bOptSelected)
        {
            if (SelIdx < gNumCustomTests) // User selected/deselected a test
            {
                HidePointer();
                if (NumTestsSel == 1 && gCustomTestList[SelIdx].Enabled) // If user is deselecting the only test selected, display error
                {
                    PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_SELECTED_TESTS), TempBuf, sizeof(TempBuf)));
                }
                else
                {
                    if (gCustomTestList[SelIdx].NameStrId == STRING_TOKEN(STR_TEST_12_NAME) && !gSIMDSupported) // If user is selecting the 128-bit test and SIMD is not supported, display error
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_TEST_NO_SIMD), TempBuf, sizeof(TempBuf)));
                    }
                    else if (gCustomTestList[SelIdx].NameStrId == STRING_TOKEN(STR_TEST_14_NAME) && DMATestPart == NULL) // If user is selecting the DMA test and DMA Test partition is not found, display error
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_DMA_TEST), TempBuf, sizeof(TempBuf)));
                    }
                    else
                    {
                        // Select/Deselect the test and update the screen
                        NumTestsSel = gCustomTestList[SelIdx].Enabled ? NumTestsSel - 1 : NumTestsSel + 1;
                        gCustomTestList[SelIdx].Enabled = !gCustomTestList[SelIdx].Enabled;

                        ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_TEST_NO_SIMD), TempBuf, sizeof(TempBuf)));
                        if (gCustomTestList[SelIdx].Enabled)
                            PrintXYAlign(LeftOffset, LINENO_YPOS(4 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"*");
                        else
                            PrintXYAlign(LeftOffset, LINENO_YPOS(4 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L" ");
                    }
                }
                DrawPointer();
            }
            else // USer is attempting to change the # of passes
            {
                CHAR16 ErrorMsg[128];
                UINTN TextHeight;
                UINT64 NumPasses;

                ErrorMsg[0] = L'\0';

                HidePointer();

                // Get the new # of passes
                XPos = LeftOffset + OptionsRect.Width + 10;
                PrintXYAlign(XPos, LINENO_YPOS(4 + gNumCustomTests + 1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));

                GetTextInfo(GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)), &TextWidth, &TextHeight);

                while (TRUE)
                {
                    if (getval(XPos + TextWidth, LINENO_YPOS(4 + gNumCustomTests + 1), 3, 1, &NumPasses) == 0)
                    {
                        if (NumPasses > 0)
                        {
#ifndef PRO_RELEASE // Free version
                            if (NumPasses > DEFAULT_NUM_PASSES)
                            {
                                ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), NULL, ALIGN_CENTRE, ErrorMsg);

                                GetStringById(STRING_TOKEN(STR_ERROR_NUM_PASSES_FREE), TempBuf, sizeof(TempBuf));
                                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), TempBuf, DEFAULT_NUM_PASSES);

                                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, ErrorMsg);
                            }
                            else
                            {
                                gNumPasses = (UINTN)NumPasses;
                                break;
                            }
#else
                            gNumPasses = (UINTN)NumPasses;
                            break;
#endif
                        }
                        else
                        {
                            ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), NULL, ALIGN_CENTRE, ErrorMsg);
                            PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_NUM_PASSES), ErrorMsg, sizeof(ErrorMsg)));
                        }
                    }
                    else
                        break;
                }

                ClearTextXYAlign(XPos, LINENO_YPOS(4 + gNumCustomTests + 1), NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));

                ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(4 + gNumCustomTests + 3), NULL, ALIGN_CENTRE, ErrorMsg);

                UnicodeSPrint(sTestSelOptText[0], sizeof(sTestSelOptText[0]), GetStringById(STRING_TOKEN(STR_MENU_TEST_PASSES), TempBuf, sizeof(TempBuf)), gNumPasses);
                DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + gNumCustomTests + 1), sTestSelOptText[0], &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect.Width);

                DrawPointer();
            }
        }
    }
}

UINT16
SysInfoScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN ValueXPos;
    UINTN TextWidth = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    CHAR16 TextBuf[LONG_STRING_LEN];
    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[32];
    CHAR16 ErrorMsg[128];
    MOUSE_EVENT mouseEv;

    EG_RECT MemRect;
    int NumMemOpts = NUM_MEM_OPTIONS;
    int TotalOpts;
    EG_RECT OptionsRect;

    MSR_TEMP CPUTemp;

    HidePointer();

    // Draw the CPU header
    if (SysInfoCPU)
        egDrawImage(SysInfoCPU, &BG_COLOUR, LeftOffset, LINENO_YPOS(1));
    else
        PrintXYAlign(LeftOffset, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"[ CPU ]");

    // CPU Type
    cpu_type(TextBuf, sizeof(TextBuf));
    PrintXYAlign(LeftOffset, LINENO_YPOS(3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TYPE), TempBuf, sizeof(TempBuf)));

    ValueXPos = gCharWidth * 25;

    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // CPU Clock
    getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TextBuf, sizeof(TextBuf));

    PrintXYAlign(LeftOffset, LINENO_YPOS(4), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_SPEED), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(4), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // CPU Temperature
    SetMem(&CPUTemp, sizeof(CPUTemp), 0);

    if (GetCPUTemp(&gCPUInfo, MPSupportGetBspId(), &CPUTemp) == EFI_SUCCESS)
    {
        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dC", CPUTemp.iTemperature);
        PrintXYAlign(LeftOffset, LINENO_YPOS(5), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TEMP), TempBuf, sizeof(TempBuf)));
        PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(5), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);
    }
    else
    {
        PrintXYAlign(LeftOffset, LINENO_YPOS(5), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_TEMP), TempBuf, sizeof(TempBuf)));
        PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(5), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
    }

    // # Logical Processors
    if (MPSupportGetNumProcessors() == MPSupportGetNumEnabledProcessors())
        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d", MPSupportGetNumProcessors());
    else
        UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CPU_AVAIL), TempBuf, sizeof(TempBuf)), MPSupportGetNumProcessors(), MPSupportGetNumEnabledProcessors());

    PrintXYAlign(LeftOffset, LINENO_YPOS(6), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_NUM_CPU), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(6), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // L1 Cache
    if (gCPUInfo.L1_data_caches_per_package > 0)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d x %dK (%d MB/s)", gCPUInfo.L1_data_caches_per_package, gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
    else
        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dK (%d MB/s)", gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size, l1_speed);
    PrintXYAlign(LeftOffset, LINENO_YPOS(7), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L1_CACHE), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(7), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // L2 Cache
    if (gCPUInfo.L2_caches_per_package > 0)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d x %dK (%d MB/s)", gCPUInfo.L2_caches_per_package, gCPUInfo.L2_cache_size, l2_speed);
    else
        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dK (%d MB/s)", gCPUInfo.L2_cache_size, l2_speed);
    PrintXYAlign(LeftOffset, LINENO_YPOS(8), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L2_CACHE), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(8), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // L3 Cache
    if (gCPUInfo.L3_cache_size)
    {
        if (gCPUInfo.L3_caches_per_package > 0)
            UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d x %dK (%d MB/s)", gCPUInfo.L3_caches_per_package, gCPUInfo.L3_cache_size, l3_speed);
        else
            UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%dK (%d MB/s)", gCPUInfo.L3_cache_size, l3_speed);
        PrintXYAlign(LeftOffset, LINENO_YPOS(9), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)));
        PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(9), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);
    }
    else
    {
        PrintXYAlign(LeftOffset, LINENO_YPOS(9), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)));
        PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(9), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
    }

    // Memory Header
    if (SysInfoMem)
        egDrawImage(SysInfoMem, &BG_COLOUR, LeftOffset, LINENO_YPOS(10));
    else
        PrintXYAlign(LeftOffset, LINENO_YPOS(10), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"[ Memory ]");

    // Total Physical Memory
    UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%ldM (%d MB/s)", MtSupportMemSizeMB(), mem_speed);
    PrintXYAlign(LeftOffset, LINENO_YPOS(12), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_TOTAL_MEM), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(12), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // Memory latency
    UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d.%03d ns", mem_latency / 1000, mem_latency % 1000);
    PrintXYAlign(LeftOffset, LINENO_YPOS(13), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_MEM_LATENCY), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(13), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // RAM Configuration
    getMemConfigStr(TextBuf, sizeof(TextBuf));
    PrintXYAlign(LeftOffset, LINENO_YPOS(14), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_REPORT_RAM_CONFIG), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(14), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // ECC Enabled
    MtSupportGetECCFeatures(TextBuf, sizeof(TextBuf));
    PrintXYAlign(LeftOffset, LINENO_YPOS(15), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_ENABLED), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(15), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

#if 0
    // iMS available
    if (iMSIsAvailable())
    {
        IMS_TO_BIOS_HOB_ST iMSToBiosHob;
        SetMem(&iMSToBiosHob, sizeof(iMSToBiosHob), 0);

        if (iMSGetVersionFromHob(&iMSToBiosHob) == EFI_SUCCESS)
        {
            UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d.%d (%s)",
                (iMSToBiosHob.iMSVersion >> 8) & 0xFF,          //iMS version high
                (iMSToBiosHob.iMSVersion) & 0xFF,
                iMSToBiosHob.iMSFreeVersion ? L"Free" : L"Paid");
        }
        else
            GetStringById(STRING_TOKEN(STR_MENU_YES), TextBuf, sizeof(TextBuf));
    }
    else
        GetStringById(STRING_TOKEN(STR_MENU_NA), TextBuf, sizeof(TextBuf));

    PrintXYAlign(LeftOffset, LINENO_YPOS(16), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s:", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_IMS_AVAILABLE), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(16), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);
#endif

    // System Information options
    NumMemOpts = NUM_MEM_OPTIONS;

    StrCpyS(sMemOptText[0], 64, GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_SPD), TempBuf, sizeof(TempBuf)));
    StrCpyS(sMemOptText[1], 64, GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_MEMUSAGE), TempBuf, sizeof(TempBuf)));
    UnicodeSPrint(sMemOptText[2], sizeof(sMemOptText[2]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_POLL), TempBuf, sizeof(TempBuf)), gECCPoll ? GetStringById(STRING_TOKEN(STR_MENU_ENABLED), TempBuf2, sizeof(TempBuf2)) : GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf2, sizeof(TempBuf2)));
#ifdef PRO_RELEASE
    UnicodeSPrint(sMemOptText[3], sizeof(sMemOptText[3]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_INJECT), TempBuf, sizeof(TempBuf)), gECCInject ? GetStringById(STRING_TOKEN(STR_MENU_ENABLED), TempBuf2, sizeof(TempBuf2)) : GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf2, sizeof(TempBuf2)));
    UnicodeSPrint(sMemOptText[4], sizeof(sMemOptText[4]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CACHE), TempBuf, sizeof(TempBuf)), GetStringById(CACHE_MODE_STR_ID[gCacheMode], TempBuf2, sizeof(TempBuf2)));
#endif
    // Get the longest text width of all the options
    for (i = 0; i < NumMemOpts; i++)
    {
        UINTN LineWidth = 0;
        GetTextInfo(sMemOptText[i], &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    // Display the options
    for (i = 0; i < NumMemOpts; i++)
        DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + i), sMemOptText[i], &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

    // Prepare the options rectangle for mouse handling
    MemRect.XPos = LeftOffset;
    MemRect.YPos = LINENO_YPOS(17);
    MemRect.Width = TextWidth;
    MemRect.Height = NumMemOpts * LINE_SPACING;

    TotalOpts = NumMemOpts;

#ifdef PRO_RELEASE
    TotalOpts = NumMemOpts + 1;
    GetTextInfo(GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_SAVE_SUMMARY), TempBuf, sizeof(TempBuf)), &TextWidth, NULL);

    // Save system information summary option
    DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + NumMemOpts), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_SAVE_SUMMARY), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, SelIdx == NumMemOpts, TextWidth);

    OptionsRect.XPos = LeftOffset;
    OptionsRect.YPos = LINENO_YPOS(17 + NumMemOpts);
    OptionsRect.Width = TextWidth;
    OptionsRect.Height = LINE_SPACING;

    PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(24), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_INSTR), TempBuf, sizeof(TempBuf)));
#else
    SetMem(&OptionsRect, sizeof(OptionsRect), 0);
#endif
    DrawPointer();

    SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? TotalOpts - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % TotalOpts;
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();
            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&MemRect))
            {
                SelIdx = (int)((curY - MemRect.YPos) / LINE_SPACING);
            }
            else if (MouseInRect(&OptionsRect))
            {
                SelIdx = NumMemOpts;
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            // Check if mouse over button
            if (SelIdx >= 0)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();

            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < TotalOpts)
            {
                if (PrevSelIdx < NumMemOpts)
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + PrevSelIdx), sMemOptText[PrevSelIdx], &DEFAULT_TEXT_PIXEL, FALSE, MemRect.Width);
                else
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + NumMemOpts), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_SAVE_SUMMARY), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect.Width);
            }

            if (SelIdx >= 0 && SelIdx < TotalOpts)
            {
                // Set selection for new index
                if (SelIdx < NumMemOpts)
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + SelIdx), sMemOptText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, MemRect.Width);
                else
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + NumMemOpts), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_SAVE_SUMMARY), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect.Width);
            }

            PrevSelIdx = SelIdx;

            DrawPointer();
        }

        if (bOptSelected)
        {
            HidePointer();

            // Clear status message
            ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), NULL, ALIGN_CENTRE, ErrorMsg);
            ErrorMsg[0] = L'\0';
            if (SelIdx < NumMemOpts)
            {
                if (SelIdx == 0) // View detailed RAM (SPD) info
                {
                    gSPDPage = 0;
                    gSPDSelIdx = 0;
                    return ID_BUTTON_SPDINFO;
                }
                else if (SelIdx == 1) // View memory usage
                {
                    EnableGraphicsMode(FALSE);
                    display_memmap(NULL);
                    EnableGraphicsMode(TRUE);

                    DrawWindow();
                    return ID_BUTTON_SYSINFO;
                }
                else if (SelIdx == 2) // ECC polling: Enabled/Disabled
                {
                    if (MtSupportECCEnabled())
                    {
                        gECCPoll = !gECCPoll;

                        UnicodeSPrint(sMemOptText[SelIdx], sizeof(sMemOptText[SelIdx]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_POLL), TempBuf, sizeof(TempBuf)), gECCPoll ? GetStringById(STRING_TOKEN(STR_MENU_ENABLED), TempBuf2, sizeof(TempBuf2)) : GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf2, sizeof(TempBuf2)));

                        // Set selection for new index
                        DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + SelIdx), sMemOptText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, MemRect.Width);

                        // PrintXYAlign(LeftOffset + ValueXPos, LINENO_YPOS(14), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s          ", gECCPoll ? GetStringById(STRING_TOKEN (STR_MENU_ENABLED), TempBuf, sizeof(TempBuf)) : GetStringById(STRING_TOKEN (STR_MENU_DISABLED), TempBuf, sizeof(TempBuf)));
                    }
                    else
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_ECC_UNSUPPORTED), ErrorMsg, sizeof(ErrorMsg)));
                    }
                }
                else if (SelIdx == 3) // ECC injection: Enabled/Disabled
                {
#ifdef PRO_RELEASE
                    if (MtSupportECCEnabled() &&
                        (AsciiStrCmp(get_mem_ctrl_name(), "AMD 15h") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "AMD 16h") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "AMD Steppe Eagle") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "AMD 15h (30h-3fh)") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "AMD Merlin Falcon (60h-6fh)") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Nehalem") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Lynnfield") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Westmere") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel E3-1200v3") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel E3-1200") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel E3-1200v2") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Ivy Bridge") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Broadwell-H") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Skylake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Atom C2000 SoC Transaction Router") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Apollo Lake SoC") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Kaby Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Coffee Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Comet Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Ice Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Rocket Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Tiger Lake H") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Tiger Lake") == 0 ||
                         AsciiStrStr(get_mem_ctrl_name(), "Intel Alder Lake") != NULL ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Raptor Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Meteor Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Arrow Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Lunar Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Elkhart Lake") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Broadwell-DE (IMC 0)") == 0 ||
                         AsciiStrCmp(get_mem_ctrl_name(), "Intel Sandy Bridge-E") == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(), "AMD Ryzen (", AsciiStrLen("AMD Ryzen (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(), "AMD Ryzen Zen 2 (", AsciiStrLen("AMD Ryzen Zen 2 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(), "AMD Ryzen Zen 3 (", AsciiStrLen("AMD Ryzen Zen 3 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(), "AMD Ryzen Zen 4 (", AsciiStrLen("AMD Ryzen Zen 4 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(), "AMD Ryzen Zen 5 (", AsciiStrLen("AMD Ryzen Zen 5 (")) == 0 ||
                         AsciiStrnCmp(get_mem_ctrl_name(), "Hygon Dhyana", AsciiStrLen("Hygon Dhyana")) == 0))
                    {
                        gECCInject = !gECCInject;

                        UnicodeSPrint(sMemOptText[SelIdx], sizeof(sMemOptText[SelIdx]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_INJECT), TempBuf, sizeof(TempBuf)), gECCInject ? GetStringById(STRING_TOKEN(STR_MENU_ENABLED), TempBuf2, sizeof(TempBuf2)) : GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf2, sizeof(TempBuf2)));

                        // Set selection for new index
                        DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + SelIdx), sMemOptText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, MemRect.Width);
                    }
                    else // ECC injection unsupported
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_ECC_INJ_UNSUPPORTED), ErrorMsg, sizeof(ErrorMsg)));
                    }
#else // Pro version only
                    PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_PRO_ONLY), ErrorMsg, sizeof(ErrorMsg)));
#endif
                }
                else if (SelIdx == 4) // Memory caching: Enabled/Disabled
                {
#ifdef PRO_RELEASE
                    int CacheMode = -1;

                    XPos = LeftOffset + OptionsRect.Width + 10;

                    CHAR16 **CacheModeOptions = AllocateStringTable(CACHE_MODE_STR_ID, ARRAY_SIZE(CACHE_MODE_STR_ID));

                    CacheMode = getoption(XPos, MemRect.YPos, CacheModeOptions, ARRAY_SIZE(CACHE_MODE_STR_ID));

                    FreeStringTable(CacheModeOptions, ARRAY_SIZE(CACHE_MODE_STR_ID));

                    if (CacheMode >= CACHEMODE_MIN && CacheMode <= CACHEMODE_MAX)
                    {
                        gCacheMode = CacheMode;

                        UnicodeSPrint(sMemOptText[SelIdx], sizeof(sMemOptText[SelIdx]), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_CACHE), TempBuf, sizeof(TempBuf)), GetStringById(CACHE_MODE_STR_ID[gCacheMode], TempBuf2, sizeof(TempBuf2)));

                        // Set selection for new index
                        DisplayMenuOption(LeftOffset, LINENO_YPOS(17 + SelIdx), sMemOptText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, MemRect.Width);
                    }
#else // Pro version only
                    PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_PRO_ONLY), ErrorMsg, sizeof(ErrorMsg)));
#endif
                }
            }
            else // Save system information summary
            {
#if 0
#ifdef PRO_RELEASE
                MT_HANDLE FileHandle = NULL;
                EFI_STATUS Status;

                EFI_TIME Time;
                CHAR16 Filename[128];
                CHAR16 Prefix[128];

                gRT->GetTime(&Time, NULL);

                Prefix[0] = L'\0';
                MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
                UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.txt", Prefix, SYSINFO_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                if (FileHandle == NULL)
                {
                    PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), ErrorMsg, sizeof(ErrorMsg)));
                }
                else
                {
                    UINTN BOMSize = 2;

                    // Write UTF16LE BOM
                    MtSupportWriteFile(FileHandle, &BOMSize, (void *)BOM_UTF16LE);

                    // Program Summary Header
                    FPrint(FileHandle, L"%s %s Build: %s\r\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
                    FPrint(FileHandle, L"PassMark Software\r\nwww.passmark.com\r\n\r\n");

                    // Hardware Info
                    FPrint(FileHandle, L"%s\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SYSINFO), TempBuf, sizeof(TempBuf)));
                    FPrint(FileHandle, L"=============================================\r\n");
                    MtSupportOutputSysInfo(FileHandle);

                    MtSupportCloseFile(FileHandle);

                    UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_SAVED), TempBuf, sizeof(TempBuf)), Filename);
                    PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, ErrorMsg);
                }
#else // Pro version only
                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(23), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_PRO_ONLY), ErrorMsg, sizeof(ErrorMsg)));
#endif
#endif
            }
            DrawPointer();
        }
    }
}

UINT16
SPDInfoScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;
    int PrevSelIdx = gSPDSelIdx;
    int SelIdx = gSPDSelIdx;
    MOUSE_EVENT mouseEv;
    CHAR16 TempBuf[128];
    CHAR16 ErrorMsg[128];

    EG_RECT DIMMRect[DIMMS_PER_PAGE];
    EG_RECT OptionsRect[NUM_SPD_OPTIONS];

    const int MAX_MODULE_STR_LEN = 40;

    enum
    {
        DIMM_NONE = -1,
        DIMM_SPD_UNMATCHED = -2,
    };

    int SPDMatchInd[DIMMS_PER_PAGE];

    // Max 8 DIMMs per page. Find out number of DIMMs in current page
    int NumModulesPage = (gSPDPage + 1) * DIMMS_PER_PAGE < g_numSMBIOSMem ? DIMMS_PER_PAGE : g_numSMBIOSMem - (gSPDPage)*DIMMS_PER_PAGE;

    HidePointer();

    // Print instructions on top
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(0), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_SPD_INSTR1), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_SPD_INSTR2), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(2), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_SPD_PAGE), TempBuf, sizeof(TempBuf)), gSPDPage * DIMMS_PER_PAGE + 1, gSPDPage * DIMMS_PER_PAGE + NumModulesPage, g_numSMBIOSMem);

    // Get the RAM summary string for DIMMs in current page
    for (i = 0; i < NumModulesPage; i++)
    {
        UINTN LineWidth;
        CHAR16 szwRAMSummary[128];
        int dimmidx = gSPDPage * DIMMS_PER_PAGE + i;

        if (g_SMBIOSMemDevices[dimmidx].smb17.Size != 0)
        {
            int FoundIdx;
            SPDMatchInd[i] = DIMM_SPD_UNMATCHED;
            for (FoundIdx = 0; FoundIdx < g_numMemModules; FoundIdx++)
            {
                if (g_MemoryInfo[FoundIdx].smbiosInd == dimmidx)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[Slot %d] Found match with SPD%d", dimmidx, FoundIdx);
                    SPDMatchInd[i] = FoundIdx;
                    break;
                }
            }

            // We couldn't match SMBIOS S/N with SPD S/N (possibly due to missing S/N in SMBIOS)
            // Just assign by order
            if (SPDMatchInd[i] == DIMM_SPD_UNMATCHED)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "[Slot %d] Could not find match for SMBIOS S/N (%a).", dimmidx, g_SMBIOSMemDevices[dimmidx].szSerialNumber);
                int j;
                int NonEmptyIdx = 0;
                for (j = 0; j < dimmidx; j++)
                {
                    if (g_SMBIOSMemDevices[j].smb17.Size != 0)
                        NonEmptyIdx++;
                }
                if (NonEmptyIdx < g_numMemModules && g_MemoryInfo[NonEmptyIdx].smbiosInd < 0)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "[Slot %d] Assume match with unmatched SPD%d S/N (%08x).", dimmidx, NonEmptyIdx, g_MemoryInfo[NonEmptyIdx].moduleSerialNo);
                    SPDMatchInd[i] = NonEmptyIdx;
                }
            }

            if (SPDMatchInd[i] >= 0)
                UnicodeSPrint(szwRAMSummary, sizeof(szwRAMSummary), L"%s %a", g_MemoryInfo[SPDMatchInd[i]].jedecManuf, g_MemoryInfo[SPDMatchInd[i]].modulePartNo);
            else
                UnicodeSPrint(szwRAMSummary, sizeof(szwRAMSummary), L"%a %a", g_SMBIOSMemDevices[dimmidx].szManufacturer, g_SMBIOSMemDevices[dimmidx].szPartNumber);
            // getSMBIOSRAMSummaryLine(&g_SMBIOSMemDevices[dimmidx], szwRAMSummary, sizeof(szwRAMSummary), TRUE);

            UnicodeSPrint(sRAMInfoText[i], sizeof(sRAMInfoText[i]), L"[%d] %s", dimmidx + 1, szwRAMSummary);
        }
        else
        {
            SPDMatchInd[i] = DIMM_NONE;
            UnicodeSPrint(sRAMInfoText[i], sizeof(sRAMInfoText[i]), L"[%d] { %s }", dimmidx + 1, GetStringById(STRING_TOKEN(STR_REPORT_EMPTY), TempBuf, sizeof(TempBuf)));
        }

        sRAMInfoText[i][MAX_MODULE_STR_LEN] = L'\0';
        GetTextInfo(sRAMInfoText[i], &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    // Display the DIMMs
    for (i = 0; i < NumModulesPage; i++)
    {
        UINTN DIMMXPos = i % 2 == 0 ? LeftOffset + 50 : (WindowWidth + LeftOffset) / 2;
        UINTN DIMMYPos = LINENO_YPOS(3 + (i / 2) * 5);

        egDrawImage(SPDMatchInd[i] == DIMM_NONE ? NoDimmIcon : DimmIcon, &BG_COLOUR, DIMMXPos, DIMMYPos);
        DisplayMenuOption(DIMMXPos, DIMMYPos + 3 * LINE_SPACING, sRAMInfoText[i], &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

        // DIMM selection rectangle for mouse handling
        DIMMRect[i].XPos = DIMMXPos;
        DIMMRect[i].YPos = DIMMYPos + 3 * LINE_SPACING;
        DIMMRect[i].Width = TextWidth;
        DIMMRect[i].Height = LINE_SPACING;

        if (SPDMatchInd[i] != DIMM_NONE)
        {
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL FOUND_TEXT_PIXEL = {0, 128, 0, 0};
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL NOT_FOUND_TEXT_PIXEL = {0, 0, 128, 0};

            PrintXYAlign(DIMMXPos, DIMMYPos + 4 * LINE_SPACING, SPDMatchInd[i] >= 0 ? &FOUND_TEXT_PIXEL : &NOT_FOUND_TEXT_PIXEL, NULL, ALIGN_LEFT, SPDMatchInd[i] >= 0 ? L"SPD detected" : L"SPD not detected");
        }
    }

    // SPD info options
    GetStringById(STRING_TOKEN(STR_MENU_PREV_PAGE), sSPDOptText[0], sizeof(sSPDOptText[0]));
    GetStringById(STRING_TOKEN(STR_MENU_NEXT_PAGE), sSPDOptText[1], sizeof(sSPDOptText[1]));
#ifdef PRO_RELEASE
    CHAR16 TempBuf2[128];
    if (gExactSPDs[0] >= 0) // Enabled if >= 0
    {
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", gExactSPDs[0]);

        for (i = 1; i < MAX_NUM_SPDS; i++)
        {
            if (gExactSPDs[i] < 0)
                break;

            CHAR16 NumBuf[16];
            UnicodeSPrint(NumBuf, sizeof(NumBuf), L",%d", gExactSPDs[i]);
            StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), NumBuf);
        }
    }
    else
        GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf, sizeof(TempBuf));
    UnicodeSPrint(sSPDOptText[3], sizeof(sSPDOptText[3]), GetStringById(STRING_TOKEN(STR_MENU_EXACTSPDS), TempBuf2, sizeof(TempBuf2)), TempBuf);
    if (gExactSPDs[0] < 0 && gMinSPDs > 0) // Enable if EXACTSPDS is disabled and MINSPDS > 0
        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", gMinSPDs);
    else
        GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf, sizeof(TempBuf));
    UnicodeSPrint(sSPDOptText[2], sizeof(sSPDOptText[2]), GetStringById(STRING_TOKEN(STR_MENU_MINSPDS), TempBuf2, sizeof(TempBuf2)), TempBuf);
    GetStringById(STRING_TOKEN(STR_MENU_SPD_SAVE), sSPDOptText[4], sizeof(sSPDOptText[4]));
#endif
    GetStringById(STRING_TOKEN(STR_MENU_GO_BACK), sSPDOptText[NUM_SPD_OPTIONS - 1], sizeof(sSPDOptText[NUM_SPD_OPTIONS - 1]));

    TextWidth = 0;
    for (i = 0; i < NUM_SPD_OPTIONS; i++)
    {
        UINTN LineWidth;

        GetTextInfo(sSPDOptText[i], &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    for (i = 0; i < NUM_SPD_OPTIONS; i++)
    {
        OptionsRect[i].XPos = i % 2 == 0 ? LeftOffset : (WindowWidth + LeftOffset) / 2;
        OptionsRect[i].YPos = LINENO_YPOS(23 + i / 2);
        OptionsRect[i].Width = TextWidth;
        OptionsRect[i].Height = LINE_SPACING;
        DisplayMenuOption(OptionsRect[i].XPos, OptionsRect[i].YPos, sSPDOptText[i], &DEFAULT_TEXT_PIXEL, SelIdx == NumModulesPage + i, TextWidth);
    }

    DrawPointer();

    SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NumModulesPage + NUM_SPD_OPTIONS - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % (NumModulesPage + NUM_SPD_OPTIONS);
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0)
                    bOptSelected = TRUE;
            }
            else if (Key.ScanCode == 9 || Key.ScanCode == 10)
            {
                int NewModulesPage;
                if (Key.ScanCode == 9 && gSPDPage > 0)
                    gSPDPage--;
                else if (Key.ScanCode == 10 && g_numSMBIOSMem > (gSPDPage + 1) * DIMMS_PER_PAGE)
                    gSPDPage++;

                NewModulesPage = (gSPDPage + 1) * DIMMS_PER_PAGE < g_numSMBIOSMem ? DIMMS_PER_PAGE : g_numSMBIOSMem - (gSPDPage)*DIMMS_PER_PAGE;
                gSPDSelIdx = NewModulesPage + Key.ScanCode == 9 ? 0 : 1;

                return ID_BUTTON_SPDINFO;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();
            SelIdx = -1;

            // Check if mouse over button
            for (i = 0; i < NumModulesPage && SelIdx < 0; i++)
            {
                if (MouseInRect(&DIMMRect[i]))
                {
                    SelIdx = i;
                    break;
                }
            }

            for (i = 0; i < NUM_SPD_OPTIONS && SelIdx < 0; i++)
            {
                if (MouseInRect(&OptionsRect[i]))
                {
                    int SPDOptIdx = i;
                    SelIdx = NumModulesPage + SPDOptIdx;
                    break;
                }
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            // Check if mouse over button
            if (SelIdx >= 0)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();

            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NumModulesPage + NUM_SPD_OPTIONS)
            {
                if (PrevSelIdx < NumModulesPage)
                    DisplayMenuOption(DIMMRect[PrevSelIdx].XPos, DIMMRect[PrevSelIdx].YPos, sRAMInfoText[PrevSelIdx], &DEFAULT_TEXT_PIXEL, FALSE, DIMMRect[PrevSelIdx].Width);
                else
                {
                    int SPDOptIdx = PrevSelIdx - NumModulesPage;
                    DisplayMenuOption(OptionsRect[SPDOptIdx].XPos, OptionsRect[SPDOptIdx].YPos, sSPDOptText[SPDOptIdx], &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect[SPDOptIdx].Width);
                }
            }

            if (SelIdx >= 0 && SelIdx < NumModulesPage + NUM_SPD_OPTIONS)
            {
                // Set selection for new index
                if (SelIdx < NumModulesPage)
                    DisplayMenuOption(DIMMRect[SelIdx].XPos, DIMMRect[SelIdx].YPos, sRAMInfoText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, DIMMRect[SelIdx].Width);
                else
                {
                    int SPDOptIdx = SelIdx - NumModulesPage;
                    DisplayMenuOption(OptionsRect[SPDOptIdx].XPos, OptionsRect[SPDOptIdx].YPos, sSPDOptText[SPDOptIdx], &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect[SPDOptIdx].Width);
                }
            }

            PrevSelIdx = SelIdx;

            DrawPointer();
        }

        if (bOptSelected)
        {
            HidePointer();

            // Clear status message
            ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(26), NULL, ALIGN_CENTRE, ErrorMsg);

            DrawPointer();

            ErrorMsg[0] = L'\0';
            if (SelIdx < NumModulesPage) // Use selected a DIMM module
            {
                EnableGraphicsMode(FALSE);
                output_smbios_info(NULL, gSPDPage * DIMMS_PER_PAGE + SelIdx);
                if (SPDMatchInd[SelIdx] >= 0)
                {
                    output_spd_info(NULL, SPDMatchInd[SelIdx]);
                }
                EnableGraphicsMode(TRUE);

                DrawWindow();
                gSPDSelIdx = SelIdx;
                return ID_BUTTON_SPDINFO;
            }
            else // SPD options
            {
                int SPDOptIdx = SelIdx - NumModulesPage;

                switch (SPDOptIdx)
                {
                case 0:
                case 1:
                {
                    int NewModulesPage;

                    if (SPDOptIdx == 0 && gSPDPage > 0)
                        gSPDPage--;
                    else if (SPDOptIdx == 1 && g_numSMBIOSMem > (gSPDPage + 1) * DIMMS_PER_PAGE)
                        gSPDPage++;

                    NewModulesPage = (gSPDPage + 1) * DIMMS_PER_PAGE < g_numSMBIOSMem ? DIMMS_PER_PAGE : g_numSMBIOSMem - (gSPDPage)*DIMMS_PER_PAGE;
                    gSPDSelIdx = NewModulesPage + SPDOptIdx;
                    return ID_BUTTON_SPDINFO;
                }
                break;
#ifdef PRO_RELEASE
                case 4: // Save detailed RAM SPD info to file
                {
                    MT_HANDLE FileHandle = NULL;
                    EFI_STATUS Status;
                    EFI_TIME Time;
                    CHAR16 Filename[128];
                    CHAR16 Prefix[128];

                    HidePointer();

                    gRT->GetTime(&Time, NULL);

                    Prefix[0] = L'\0';
                    MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
                    UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.txt", Prefix, RAMINFO_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                    Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                    if (FileHandle == NULL)
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(26), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), ErrorMsg, sizeof(ErrorMsg)));
                    }
                    else
                    {
                        UINTN BOMSize = 2;

                        // Write UTF16LE BOM
                        MtSupportWriteFile(FileHandle, &BOMSize, (void *)BOM_UTF16LE);

                        // Program Summary Header
                        FPrint(FileHandle, L"%s %s Build: %s\r\n", PROGRAM_NAME, PROGRAM_VERSION, PROGRAM_BUILD);
                        FPrint(FileHandle, L"PassMark Software\r\nwww.passmark.com\r\n\r\n");

                        // Memory Summary
                        FPrint(FileHandle, L"%s:\r\n", GetStringById(STRING_TOKEN(STR_REPORT_MEM_SUMMARY), TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L" %s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM_SLOTS), TempBuf, sizeof(TempBuf)), g_numSMBIOSMem);
                        FPrint(FileHandle, L" %s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_DIMM), TempBuf, sizeof(TempBuf)), g_numSMBIOSModules);
                        FPrint(FileHandle, L" %s: %d\r\n", GetStringById(STRING_TOKEN(STR_REPORT_NUM_SPD), TempBuf, sizeof(TempBuf)), g_numMemModules);
                        FPrint(FileHandle, L" %s: %4dM\r\n\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_TOTAL_MEM), TempBuf, sizeof(TempBuf)), MtSupportMemSizeMB());

                        FPrint(FileHandle, L"%s:\r\n", GetStringById(STRING_TOKEN(STR_REPORT_RAM_CONFIG), TempBuf, sizeof(TempBuf)));

                        CHAR16 szwXferSpeed[32];
                        CHAR16 szwChannelMode[32];
                        CHAR16 szwTiming[32];
                        CHAR16 szwVoltage[32];
                        CHAR16 szwECC[32];

                        getMemConfig(szwXferSpeed, szwChannelMode, szwTiming, szwVoltage, szwECC, sizeof(szwXferSpeed));
                        FPrint(FileHandle, L" %s: %s\r\n", GetStringById(STRING_TOKEN(STR_XFER_SPEED), TempBuf, sizeof(TempBuf)), szwXferSpeed[0] ? szwXferSpeed : L"N/A");
                        FPrint(FileHandle, L" %s: %s\r\n", GetStringById(STRING_TOKEN(STR_CHANNEL_MODE), TempBuf, sizeof(TempBuf)), szwChannelMode[0] ? szwChannelMode : L"N/A");
                        FPrint(FileHandle, L" %s: %s\r\n", GetStringById(STRING_TOKEN(STR_TIMINGS), TempBuf, sizeof(TempBuf)), szwTiming[0] ? szwTiming : L"N/A");
                        FPrint(FileHandle, L" %s: %s\r\n", GetStringById(STRING_TOKEN(STR_VOLTAGE), TempBuf, sizeof(TempBuf)), szwVoltage[0] ? szwVoltage : L"N/A");
                        FPrint(FileHandle, L" %s: %s\r\n", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_ECC_ENABLED), TempBuf, sizeof(TempBuf)), szwECC);

                        FPrint(FileHandle, L"\r\n%s:\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SPD_DETAILS), TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"--------------\r\n");
                        for (i = 0; i < g_numMemModules; i++)
                        {
                            FPrint(FileHandle, L"\r\nSPD #: %d\r\n", g_MemoryInfo[i].dimmNum);
                            FPrint(FileHandle, L"==============\r\n");
                            output_spd_info(FileHandle, i);
                        }
                        if (g_numMemModules == 0)
                            FPrint(FileHandle, L"%s\r\n", GetStringById(STRING_TOKEN(STR_ERROR_SPD), TempBuf, sizeof(TempBuf)));

                        FPrint(FileHandle, L"\r\n%s:\r\n", GetStringById(STRING_TOKEN(STR_REPORT_SMBIOS_DETAILS), TempBuf, sizeof(TempBuf)));
                        FPrint(FileHandle, L"--------------\r\n");
                        for (i = 0; i < g_numSMBIOSMem; i++)
                        {
                            FPrint(FileHandle, L"\r\nMemory Device #: %d\r\n", i);
                            FPrint(FileHandle, L"==============\r\n");
                            if (g_SMBIOSMemDevices[i].smb17.Size != 0)
                                output_smbios_info(FileHandle, i);
                            else
                                FPrint(FileHandle, L"Empty slot\r\n");
                        }
                        MtSupportCloseFile(FileHandle);

                        UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_MENU_SPD_SAVED), TempBuf, sizeof(TempBuf)), Filename);
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(26), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, ErrorMsg);
                    }

                    DrawPointer();
                }
                break;

                case 2: // Minimum SPDs
                {
                    UINTN TextHeight;
                    UINT64 MinSPDs;

                    HidePointer();

                    // Get the new # of passes
                    XPos = OptionsRect[SPDOptIdx].XPos + OptionsRect[SPDOptIdx].Width + 10;
                    PrintXYAlign(XPos, OptionsRect[SPDOptIdx].YPos, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));

                    GetTextInfo(GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)), &TextWidth, &TextHeight);

                    while (TRUE)
                    {
                        if (getval(XPos + TextWidth, OptionsRect[SPDOptIdx].YPos, 2, 1, &MinSPDs) == 0)
                        {
                            gMinSPDs = (UINTN)MinSPDs;
                        }
                        break;
                    }

                    ClearTextXYAlign(XPos, OptionsRect[SPDOptIdx].YPos, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));

                    if (gExactSPDs[0] < 0 && gMinSPDs > 0) // Enable if EXACTSPDS is disabled and MINSPDS > 0
                        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", gMinSPDs);
                    else
                        GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf, sizeof(TempBuf));
                    UnicodeSPrint(sSPDOptText[SPDOptIdx], sizeof(sSPDOptText[SPDOptIdx]), GetStringById(STRING_TOKEN(STR_MENU_MINSPDS), TempBuf2, sizeof(TempBuf2)), TempBuf);

                    DisplayMenuOption(OptionsRect[SPDOptIdx].XPos, OptionsRect[SPDOptIdx].YPos, sSPDOptText[SPDOptIdx], &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect[SPDOptIdx].Width);

                    DrawPointer();
                }
                break;

                case 3: // Exact SPDs
                {
                    UINTN TextHeight;
                    UINT64 ExactSPDs;

                    HidePointer();

                    // Get the new # of passes
                    XPos = OptionsRect[SPDOptIdx].XPos + OptionsRect[SPDOptIdx].Width + 10;
                    PrintXYAlign(XPos, OptionsRect[SPDOptIdx].YPos, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));

                    GetTextInfo(GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)), &TextWidth, &TextHeight);

                    while (TRUE)
                    {
                        for (i = 0; i < MAX_NUM_SPDS; i++)
                            gExactSPDs[i] = -1;

                        if (getval(XPos + TextWidth, OptionsRect[SPDOptIdx].YPos, 2, 1, &ExactSPDs) == 0)
                        {
                            gExactSPDs[0] = (INTN)ExactSPDs;
                        }
                        break;
                    }

                    ClearTextXYAlign(XPos, OptionsRect[SPDOptIdx].YPos, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));

                    if (gExactSPDs[0] >= 0) // Enabled if >= 0
                    {
                        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", gExactSPDs[0]);

                        for (i = 1; i < MAX_NUM_SPDS; i++)
                        {
                            if (gExactSPDs[i] < 0)
                                break;

                            CHAR16 NumBuf[16];
                            UnicodeSPrint(NumBuf, sizeof(NumBuf), L",%d", gExactSPDs[i]);
                            StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), NumBuf);
                        }
                    }
                    else
                        GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf, sizeof(TempBuf));
                    UnicodeSPrint(sSPDOptText[SPDOptIdx], sizeof(sSPDOptText[SPDOptIdx]), GetStringById(STRING_TOKEN(STR_MENU_EXACTSPDS), TempBuf2, sizeof(TempBuf2)), TempBuf);
                    if (gExactSPDs[0] < 0 && gMinSPDs > 0) // Enable if EXACTSPDS is disabled and MINSPDS > 0
                        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d", gMinSPDs);
                    else
                        GetStringById(STRING_TOKEN(STR_MENU_DISABLED), TempBuf, sizeof(TempBuf));
                    UnicodeSPrint(sSPDOptText[SPDOptIdx - 1], sizeof(sSPDOptText[SPDOptIdx - 1]), GetStringById(STRING_TOKEN(STR_MENU_MINSPDS), TempBuf2, sizeof(TempBuf2)), TempBuf);

                    DisplayMenuOption(OptionsRect[SPDOptIdx - 1].XPos, OptionsRect[SPDOptIdx - 1].YPos, sSPDOptText[SPDOptIdx - 1], &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect[SPDOptIdx - 1].Width);
                    DisplayMenuOption(OptionsRect[SPDOptIdx].XPos, OptionsRect[SPDOptIdx].YPos, sSPDOptText[SPDOptIdx], &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect[SPDOptIdx].Width);

                    DrawPointer();
                }
                break;
#endif
                default:
                {
                    return ID_BUTTON_SYSINFO;
                }
                break;
                }
            }
        }
    }
}

#ifdef PRO_RELEASE
VOID DrawDIMMResults(UINTN LeftOffset, IN UINTN TopOffset, int StartIdx, int EndIdx, DIMM_SIDE DimmSide, EG_IMAGE **pImage)
{
    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[64];
    int i;
    int NumDimms = EndIdx - StartIdx + 1;
    EG_IMAGE *Output = NULL;

    const UINTN MIN_WIDTH = 640;
    const UINTN DIMM_ICON_WIDTH = 100;
    const UINTN DIMM_ICON_HEIGHT = 350;
    const UINTN DIMM_HORIZ_MARGIN = 10;
    const UINTN DIMM_VERT_MARGIN = 20;

    int NumSides = DimmSide == DIMM_BOTH_SIDES ? 2 : 1;

    // Draw to bitmap image instead of screen
    if (pImage)
    {
        UINTN ImageWidth = LeftOffset + 20 + NumDimms * (DIMM_ICON_WIDTH * 2 + DIMM_HORIZ_MARGIN);
        ImageWidth = ImageWidth < MIN_WIDTH ? MIN_WIDTH : ImageWidth;
        UINTN ImageHeight = LINENO_YPOS(10) + NumSides * (DIMM_ICON_HEIGHT + DIMM_VERT_MARGIN);
        Output = egCreateImage(ImageWidth, ImageHeight, FALSE);
        if (Output == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not create bitmap image");
            MtSupportDebugWriteLine(gBuffer);
            return;
        }
        egFillArea(Output, &BG_COLOUR, 0, 0, ImageWidth, ImageHeight);
    }

    ImagePrintXYAlign(Output, LeftOffset, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"DIMM %d - %d Test Results (%d slots total)", StartIdx + 1, EndIdx + 1, g_numSMBIOSMem);

    ImagePrintXYAlign(Output, LeftOffset, LINENO_YPOS(3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, GetStringById(STRING_TOKEN(STR_SLOT), TempBuf, sizeof(TempBuf)));
    ImagePrintXYAlign(Output, LeftOffset, LINENO_YPOS(4), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, GetStringById(STRING_TOKEN(STR_RESULT), TempBuf, sizeof(TempBuf)));

    ImagePrintXYAlign(Output, LeftOffset, LINENO_YPOS(6), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, GetStringById(STRING_TOKEN(STR_REPORT_VENDOR), TempBuf, sizeof(TempBuf)));
    ImagePrintXYAlign(Output, LeftOffset, LINENO_YPOS(7), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, GetStringById(STRING_TOKEN(STR_PART_NO), TempBuf, sizeof(TempBuf)));
    ImagePrintXYAlign(Output, LeftOffset, LINENO_YPOS(8), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, GetStringById(STRING_TOKEN(STR_SERIAL_NO), TempBuf, sizeof(TempBuf)));
    ImagePrintXYAlign(Output, LeftOffset, LINENO_YPOS(9), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, GetStringById(STRING_TOKEN(STR_SIZE), TempBuf, sizeof(TempBuf)));

    // Display the DIMMs
    for (i = 0; i < NumDimms; i++)
    {
        int dimmidx = StartIdx + i;
        BOOLEAN Occupied = g_SMBIOSMemDevices[dimmidx].smb17.Size != 0;

#ifdef MEM_DECODE_DEBUG
        UINT8 FormFactor = ChipMapFormFactorDimm;
        UINT8 MemoryType = 0x22;
#else
        UINT8 FormFactor = MtSupportGetFormFactor(dimmidx);
        UINT8 MemoryType = g_SMBIOSMemDevices[dimmidx].smb17.MemoryType;

#endif
        unsigned char const *png_bin = dimm_vert_png_bin;
        UINTN png_bin_len = sizeof(dimm_vert_png_bin);

        // Load DIMM icon based on form factor
        switch (FormFactor)
        {
        case ChipMapFormFactorSodimm:
        case ChipMapFormFactorCSodimm:
            png_bin = Occupied ? sodimm_png_bin : sodimm_none_png_bin;
            png_bin_len = Occupied ? sizeof(sodimm_png_bin) : sizeof(sodimm_none_png_bin);
            break;
        default:
            png_bin = Occupied ? dimm_vert_png_bin : dimm_none_vert_png_bin;
            png_bin_len = Occupied ? sizeof(dimm_vert_png_bin) : sizeof(dimm_none_vert_png_bin);
            break;
        }

        EG_IMAGE *DimmImage = egDecodePNG(png_bin, png_bin_len, 128, TRUE);
        if (DimmImage == NULL)
        {
            AsciiSPrint(gBuffer, BUF_SIZE, "Could not load DIMM vertical icon image");
            MtSupportDebugWriteLine(gBuffer);
        }

        UINTN DIMMXPos = LeftOffset + 20 + i * (DIMM_ICON_WIDTH * 2 + DIMM_HORIZ_MARGIN);

        EFI_GRAPHICS_OUTPUT_BLT_PIXEL TEXT_COLOUR = {192, 192, 192, 0};
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL PASS_COLOUR = {0, 128, 0, 0};
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL INCOMPLETE_COLOUR = {0, 215, 255, 0};
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL FAIL_COLOUR = {0, 0, 192, 0};

        int ChipWidth = MtSupportGetChipWidth(dimmidx);
        int NumRanks = MtSupportGetNumRanks(dimmidx);
        int DimmResult = MtSupportGetDimmTestResult(dimmidx);
        int NumErrs = MtSupportGetNumSlotErrors(dimmidx);

        ImagePrintXYAlign(Output, DIMMXPos, LINENO_YPOS(3), &TEXT_COLOUR, NULL, ALIGN_LEFT, L"%s", MtSupportGetSlotLabel(dimmidx, TRUE, TempBuf, sizeof(TempBuf)));

        if (Occupied)
        {
            UINT64 DimmSizeMB = 0;
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL *RESULT_COLOUR = &INCOMPLETE_COLOUR;

            if (DimmResult == RESULT_PASS)
                RESULT_COLOUR = &PASS_COLOUR;
            else if (DimmResult == RESULT_FAIL)
                RESULT_COLOUR = &FAIL_COLOUR;
            else
                RESULT_COLOUR = &INCOMPLETE_COLOUR;

            ImagePrintXYAlign(Output, DIMMXPos, LINENO_YPOS(4), RESULT_COLOUR, NULL, ALIGN_LEFT, L"%s",
                              MtSupportGetTestResultStr(DimmResult, gCurLang, TempBuf, sizeof(TempBuf)));
            ImagePrintXYAlign(Output, DIMMXPos, LINENO_YPOS(5), RESULT_COLOUR, NULL, ALIGN_LEFT, L"(%d %s)",
                              NumErrs,
                              GetStringById(STRING_TOKEN(STR_ERRORS), TempBuf2, sizeof(TempBuf2)));

            ImagePrintXYAlign(Output, DIMMXPos, LINENO_YPOS(6), &TEXT_COLOUR, NULL, ALIGN_LEFT, L"%a", g_SMBIOSMemDevices[dimmidx].szManufacturer);
            ImagePrintXYAlign(Output, DIMMXPos, LINENO_YPOS(7), &TEXT_COLOUR, NULL, ALIGN_LEFT, L"%a", g_SMBIOSMemDevices[dimmidx].szPartNumber);
            ImagePrintXYAlign(Output, DIMMXPos, LINENO_YPOS(8), &TEXT_COLOUR, NULL, ALIGN_LEFT, L"%a", g_SMBIOSMemDevices[dimmidx].szSerialNumber);

            if (g_SMBIOSMemDevices[dimmidx].smb17.Size == 0x7fff) // Need to use Extended Size field
            {
                DimmSizeMB = g_SMBIOSMemDevices[dimmidx].smb17.ExtendedSize;
            }
            else
            {
                if (BitExtract(g_SMBIOSMemDevices[dimmidx].smb17.Size, 15, 15) == 0)                           // MB - bit 15 is the unit MB or KB
                    DimmSizeMB = g_SMBIOSMemDevices[dimmidx].smb17.Size;                                       // express in B //BIT601000.0012 overflow corrected
                else                                                                                           // KB
                    DimmSizeMB = DivU64x32(MultU64x32(g_SMBIOSMemDevices[dimmidx].smb17.Size, 1024), 1048576); // express in B
            }
            ImagePrintXYAlign(Output, DIMMXPos, LINENO_YPOS(9), &TEXT_COLOUR, NULL, ALIGN_LEFT, L"%ld MB", DimmSizeMB);
        }
        else
        {
            ImagePrintXYAlign(Output, DIMMXPos + DimmImage->Width / 2, LINENO_YPOS(4), &TEXT_COLOUR, NULL, ALIGN_CENTRE, L"-");
            ImagePrintXYAlign(Output, DIMMXPos + DimmImage->Width / 2, LINENO_YPOS(6), &TEXT_COLOUR, NULL, ALIGN_CENTRE, L"-");
            ImagePrintXYAlign(Output, DIMMXPos + DimmImage->Width / 2, LINENO_YPOS(7), &TEXT_COLOUR, NULL, ALIGN_CENTRE, L"-");
            ImagePrintXYAlign(Output, DIMMXPos + DimmImage->Width / 2, LINENO_YPOS(8), &TEXT_COLOUR, NULL, ALIGN_CENTRE, L"-");
        }

        // For screen display, only show one side.
        // For bitmap file, show both sides
        DIMM_SIDE CurSide = DimmSide == DIMM_BOTH_SIDES ? DIMM_FRONT_SIDE : DimmSide;
        for (int k = 0; k < NumSides; k++)
        {
            UINTN DIMMYPos = LINENO_YPOS(10) + k * (DIMM_ICON_HEIGHT + 20) + (DIMM_ICON_HEIGHT - DimmImage->Height) / 2;

            if (i == 0)
            {
                // Display "front" or "back" side vertical text
                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%s", GetStringById(CurSide == DIMM_FRONT_SIDE ? STRING_TOKEN(STR_FRONT_SIDE) : STRING_TOKEN(STR_BACK_SIDE), TempBuf2, sizeof(TempBuf2)));

                UINTN TextWidth = 0, TextHeight = 0;
                egMeasureText(TempBuf, &TextWidth, &TextHeight);

                // Render text to image and rotate
                EG_IMAGE *TextBufferXY = egCreateImage(TextWidth, gTextHeight, FALSE);
                if (TextBufferXY)
                {
                    egFillArea(TextBufferXY, &BG_COLOUR, 0, 0, TextWidth, gTextHeight);
                    ImagePrintXYAlign(TextBufferXY, 0, 0, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TempBuf);

                    EG_IMAGE *VTextBufferXY = egRotateImage(TextBufferXY, ROTATE_90);
                    if (VTextBufferXY)
                    {
                        UINTN XPos = LeftOffset - VTextBufferXY->Width;
                        UINTN YPos = DIMMYPos + (DimmImage->Height - VTextBufferXY->Height) / 2;
                        if (pImage)
                            egComposeImage(Output, VTextBufferXY, XPos, YPos);
                        else
                            egDrawImage(VTextBufferXY, &BG_COLOUR, XPos, YPos);
                        egFreeImage(VTextBufferXY);
                    }

                    egFreeImage(TextBufferXY);
                    TextBufferXY = NULL;
                }
            }

            // Flip the DIMM icon if back side
            EG_IMAGE *CurImg = NULL;
            if (CurSide == DIMM_FRONT_SIDE)
            {
                CurImg = egCopyImage(DimmImage);
            }
            else
            {
                CurImg = egFlipImage(DimmImage, FLIP_VERT);
            }

            if (pImage)
                egComposeImage(Output, CurImg, DIMMXPos, DIMMYPos);
            else
                egDrawImage(CurImg, &BG_COLOUR, DIMMXPos, DIMMYPos);

            if (FormFactor == ChipMapFormFactorSodimm || FormFactor == ChipMapFormFactorCSodimm)
            {
                const int ERRORS_OFFSET = 100;

                // Number of chips to draw depend on back/front side, single/dual-rank
                int NumChips = 0;
                if (Occupied)
                {
                    if (MemoryType == 0x22 || MemoryType == 0x23) // DDR5
                        NumChips = (CurSide == DIMM_FRONT_SIDE || NumRanks >= 2) ? 64 / ChipWidth : 0;
                    else
                    {
                        if (ChipWidth <= 8)
                            NumChips = 32 * NumRanks / ChipWidth;
                        else
                            NumChips = (CurSide == DIMM_FRONT_SIDE || NumRanks >= 2) ? 64 / ChipWidth : 0;
                    }
                }

                AsciiFPrint(DEBUG_FILE_HANDLE, "[SO-DIMM %d] Errs=%d Ranks=%d Side=%d Chips=%d", dimmidx, NumErrs, NumRanks, CurSide, NumChips);

                if (NumChips > 0)
                    ImagePrintXYAlign(Output, DIMMXPos + CurImg->Width + ERRORS_OFFSET, DIMMYPos, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, L"%s", GetStringById(STRING_TOKEN(STR_ERRORS), TempBuf, sizeof(TempBuf)));

                for (int j = 0; j < NumChips; j++)
                {
                    int chip = CurSide == DIMM_FRONT_SIDE ? j : NumChips + (NumChips - 1 - j);
                    int ChipErrs = MtSupportGetNumSlotChipErrors(dimmidx, chip);
                    UINTN ChipXPos = 0;
                    UINTN ChipYPos = DIMMYPos;

                    const int CHIP_HEIGHT = 28;
                    const int CHIP_WIDTH = 33;
                    const int CHIP_SPACING = 5;
                    const int CHIP_MARGIN = 20;
                    const int CHIP_SEPARATION = 85;

                    // Calculate the position to draw each chip
                    if (NumChips == 8)
                    {
                        if (CurSide == DIMM_FRONT_SIDE)
                        {
                            ChipXPos = DIMMXPos + CHIP_SPACING;
                            if (NumRanks == 1)
                            {
                                if (chip % 2 == 0)
                                    ChipXPos += CHIP_WIDTH + CHIP_SPACING;
                            }
                            else
                            {
                                if (chip == 0 || chip == 2 || chip == 5 || chip == 7)
                                    ChipXPos += CHIP_WIDTH + CHIP_SPACING;
                            }
                        }
                        else
                        {
                            ChipXPos = DIMMXPos + CurImg->Width - CHIP_SPACING - CHIP_WIDTH;
                            if (chip == 8 || chip == 10 || chip == 13 || chip == 15)
                                ChipXPos -= CHIP_WIDTH + CHIP_SPACING;
                        }
                        ChipYPos = DIMMYPos + CurImg->Height - CHIP_MARGIN - (CHIP_HEIGHT + CHIP_SPACING) * ((j >> 1) + 1);
                    }
                    else if (NumChips == 4)
                    {
                        if (CurSide == DIMM_FRONT_SIDE)
                        {
                            ChipXPos = DIMMXPos + CHIP_SPACING + CHIP_WIDTH / 2;
                        }
                        else
                        {
                            ChipXPos = DIMMXPos + CurImg->Width - CHIP_SPACING - CHIP_WIDTH - CHIP_WIDTH / 2;
                        }
                        ChipYPos = DIMMYPos + CurImg->Height - CHIP_MARGIN - (CHIP_HEIGHT + CHIP_SPACING) * (j + 1);
                    }
                    else
                        continue;

                    // Separation between in the middle
                    if (j >= (NumChips >> 1))
                        ChipYPos -= CHIP_SEPARATION;

                    // Draw the chip
                    egFillArea(Output, &BG_COLOUR, ChipXPos, ChipYPos, CHIP_WIDTH, CHIP_HEIGHT);

                    // Print the chip name
                    CHAR16 ChipStr[8];
                    ImagePrintXYAlign(Output, ChipXPos + CHIP_WIDTH / 2, ChipYPos + (CHIP_HEIGHT - gTextHeight) / 2, ChipErrs > 0 ? &FAIL_COLOUR : &TEXT_COLOUR, NULL, ALIGN_CENTRE, L"%s", MtSupportGetChipLabel(chip, ChipStr, sizeof(ChipStr)));

                    // Display per-chip errors if Site Edition
#ifdef SITE_EDITION
                    UINTN ErrXPos = DIMMXPos + CurImg->Width + ERRORS_OFFSET;
                    UINTN ErrYPos = ChipYPos + (CHIP_HEIGHT - gTextHeight) / 2;
                    if (NumChips == 8)
                    {
                        if (CurSide == DIMM_FRONT_SIDE)
                        {
                            if (NumRanks == 1)
                            {
                                if (chip % 2 == 0)
                                    ErrXPos = DIMMXPos + CurImg->Width + ERRORS_OFFSET;
                                else
                                    ErrXPos = DIMMXPos + CurImg->Width + ERRORS_OFFSET / 2;
                            }
                            else
                            {
                                if (chip == 0 || chip == 2 || chip == 5 || chip == 7)
                                    ErrXPos = DIMMXPos + CurImg->Width + ERRORS_OFFSET;
                                else
                                    ErrXPos = DIMMXPos + CurImg->Width + ERRORS_OFFSET / 2;
                            }
                        }
                        else
                        {
                            if (chip == 8 || chip == 10 || chip == 13 || chip == 15)
                                ErrXPos = DIMMXPos + CurImg->Width + ERRORS_OFFSET / 2;
                            else
                                ErrXPos = DIMMXPos + CurImg->Width + ERRORS_OFFSET;
                        }
                    }
                    ImagePrintXYAlign(Output, ErrXPos, ErrYPos, ChipErrs > 0 ? &FAIL_COLOUR : &TEXT_COLOUR, NULL, ALIGN_RIGHT, L"%d", ChipErrs);
#else
                    if (j == NumChips - 1)
                    {
                        EFI_GRAPHICS_OUTPUT_BLT_PIXEL INFO_COLOUR = {128, 128, 128, 0};
                        ImagePrintXYAlign(Output, DIMMXPos + CurImg->Width + ERRORS_OFFSET, ChipYPos + (CHIP_HEIGHT - gTextHeight) / 2, &INFO_COLOUR, NULL, ALIGN_RIGHT, L"%s", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
                    }
#endif
                }
            }
            else
            {
                const int ERRORS_OFFSET = 75;

                // Number of chips to draw depend on back/front side, single/dual-rank
                int NumChips = (Occupied && (CurSide == DIMM_FRONT_SIDE || NumRanks >= 2)) ? 64 / ChipWidth : 0;

                AsciiFPrint(DEBUG_FILE_HANDLE, "[DIMM %d] Errs=%d Ranks=%d Side=%d Chips=%d", dimmidx, NumErrs, NumRanks, CurSide, NumChips);

                if (NumChips > 0)
                    ImagePrintXYAlign(Output, DIMMXPos + CurImg->Width + ERRORS_OFFSET, DIMMYPos, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_RIGHT, L"%s", GetStringById(STRING_TOKEN(STR_ERRORS), TempBuf, sizeof(TempBuf)));

                for (int j = 0; j < NumChips; j++)
                {
                    int chip = CurSide == DIMM_FRONT_SIDE ? j : NumChips + (NumChips - 1 - j);
                    int ChipErrs = MtSupportGetNumSlotChipErrors(dimmidx, chip);

                    const int CHIP_HEIGHT = 28;
                    const int CHIP_WIDTH = 40;
                    const int CHIP_SPACING = 5;
                    const int CHIP_XMARGIN = 10;
                    const int CHIP_YMARGIN = 35;
                    const int CHIP_SEPARATION = 10;

                    UINTN ChipXPos = CurSide == DIMM_FRONT_SIDE ? DIMMXPos + CHIP_XMARGIN : DIMMXPos + CurImg->Width - CHIP_XMARGIN - CHIP_WIDTH;
                    UINTN ChipYPos = DIMMYPos;

                    if (NumChips == 8)
                    {
                        ChipYPos = DIMMYPos + CurImg->Height - CHIP_YMARGIN - (CHIP_HEIGHT + CHIP_SPACING) * (j + 1);
                        if (j >= (NumChips >> 1))
                            ChipYPos -= CHIP_SEPARATION;
                    }
                    else if (NumChips == 4)
                    {
                        ChipYPos = DIMMYPos + CurImg->Height - CHIP_YMARGIN - (CHIP_HEIGHT + CHIP_SPACING) * (j + 2);
                        if (j >= (NumChips >> 1))
                            ChipYPos -= CHIP_SEPARATION + (CHIP_HEIGHT + CHIP_SPACING) * 2;
                    }
                    else
                        continue;

                    egFillArea(Output, &BG_COLOUR, ChipXPos, ChipYPos, CHIP_WIDTH, CHIP_HEIGHT);

                    CHAR16 ChipStr[8];
                    ImagePrintXYAlign(Output, ChipXPos + CHIP_WIDTH / 2, ChipYPos + (CHIP_HEIGHT - gTextHeight) / 2, ChipErrs > 0 ? &FAIL_COLOUR : &TEXT_COLOUR, NULL, ALIGN_CENTRE, L"%s", MtSupportGetChipLabel(chip, ChipStr, sizeof(ChipStr)));
#ifdef SITE_EDITION
                    ImagePrintXYAlign(Output, DIMMXPos + CurImg->Width + ERRORS_OFFSET, ChipYPos + (CHIP_HEIGHT - gTextHeight) / 2, ChipErrs > 0 ? &FAIL_COLOUR : &TEXT_COLOUR, NULL, ALIGN_RIGHT, L"%d", ChipErrs);
#else
                    if (j == NumChips - 1)
                    {
                        EFI_GRAPHICS_OUTPUT_BLT_PIXEL INFO_COLOUR = {128, 128, 128, 0};
                        ImagePrintXYAlign(Output, DIMMXPos + CurImg->Width + ERRORS_OFFSET, ChipYPos + (CHIP_HEIGHT - gTextHeight) / 2, &INFO_COLOUR, NULL, ALIGN_RIGHT, L"%s*", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
                    }
#endif
                }
            }
            if (CurImg)
                egFreeImage(CurImg);
            CurSide = DIMM_BACK_SIDE;
        }

        if (DimmImage)
            egFreeImage(DimmImage);
    }
#ifndef SITE_EDITION
    if (Output == NULL)
    {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL INFO_COLOUR = {128, 128, 128, 0};
        ImagePrintXYAlign(Output, (LeftOffset + WindowWidth) / 2, LINENO_YPOS(10) + DIMM_ICON_HEIGHT, &INFO_COLOUR, NULL, ALIGN_CENTRE, L"*%s", GetStringById(STRING_TOKEN(STR_CHIP_SITE_ONLY), TempBuf, sizeof(TempBuf)));
    }
#endif

    if (pImage)
        *pImage = Output;
}

VOID DIMMResultsScreen(IN UINTN XPos, IN UINTN YPos)
{
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF + 100;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    CHAR16 TempBuf[128];
    CHAR16 TempBuf2[64];
    CHAR16 ErrorMsg[128];
    int i;

#define NUM_OPTIONS 5
    CHAR16 OptText[NUM_OPTIONS][48];
    EG_RECT OptionsRect[NUM_OPTIONS];

    const int NUM_DIMMS = 4;
    int DIMMPage = 0;
    DIMM_SIDE DimmSide = DIMM_FRONT_SIDE;
    BOOLEAN Finished = FALSE;

    GetStringById(STRING_TOKEN(STR_CONTINUE), OptText[0], sizeof(OptText[0]));
    UnicodeSPrint(OptText[1], sizeof(OptText[1]), GetStringById(STRING_TOKEN(STR_SHOW), TempBuf, sizeof(TempBuf)), GetStringById(DimmSide == DIMM_FRONT_SIDE ? STRING_TOKEN(STR_BACK_SIDE) : STRING_TOKEN(STR_FRONT_SIDE), TempBuf2, sizeof(TempBuf2)));
    GetStringById(STRING_TOKEN(STR_MENU_PREV_PAGE), OptText[2], sizeof(OptText[2]));
    GetStringById(STRING_TOKEN(STR_MENU_NEXT_PAGE), OptText[3], sizeof(OptText[3]));
    GetStringById(STRING_TOKEN(STR_SAVE_DIMM_RESULTS), OptText[4], sizeof(OptText[4]));

    EFI_GRAPHICS_OUTPUT_BLT_PIXEL FAIL_COLOUR = {0, 0, 192, 0};

    TextWidth = 0;
    for (i = 0; i < NUM_OPTIONS; i++)
    {
        UINTN LineWidth;

        GetTextInfo(OptText[i], &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    while (!Finished)
    {
        // Max 4 DIMMs per page. Find out number of DIMMs in current page
        int NumModulesPage = (DIMMPage + 1) * NUM_DIMMS < g_numSMBIOSMem ? NUM_DIMMS : g_numSMBIOSMem - (DIMMPage)*NUM_DIMMS;

        egClearScreen(&BG_COLOUR);

        UnicodeSPrint(OptText[1], sizeof(OptText[1]), GetStringById(STRING_TOKEN(STR_SHOW), TempBuf, sizeof(TempBuf)), GetStringById(DimmSide == DIMM_FRONT_SIDE ? STRING_TOKEN(STR_BACK_SIDE) : STRING_TOKEN(STR_FRONT_SIDE), TempBuf2, sizeof(TempBuf2)));

        for (i = 0; i < NUM_OPTIONS; i++)
        {
            OptionsRect[i].XPos = LeftOffset;
            OptionsRect[i].YPos = LINENO_YPOS(24 + i);
            OptionsRect[i].Width = TextWidth;
            OptionsRect[i].Height = LINE_SPACING;
            DisplayMenuOption(OptionsRect[i].XPos, OptionsRect[i].YPos, OptText[i], &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);
        }

        DrawDIMMResults(LeftOffset, TopOffset, DIMMPage * NUM_DIMMS, DIMMPage * NUM_DIMMS + NumModulesPage - 1, DimmSide, NULL);

        if (MtSupportGetNumUnknownSlotErrors() > 0)
        {
            PrintXYAlign(LeftOffset + 241 + 100, LINENO_YPOS(24 + 2), &FAIL_COLOUR, NULL, ALIGN_LEFT, L"%s: %d", GetStringById(STRING_TOKEN(STR_UNDECODED_ERR), TempBuf, sizeof(TempBuf)), MtSupportGetNumUnknownSlotErrors());
        }

        while (TRUE)
        {
            EFI_INPUT_KEY Key;
            BOOL bOptSelected = FALSE;

            SetMem(&Key, sizeof(Key), 0);

            MtUiWaitForKey(0, &Key);

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NUM_OPTIONS - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % NUM_OPTIONS;
            }
            else if (Key.ScanCode == 0x17 /* Esc */)
            {
                Finished = TRUE;
                break;
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0)
                    bOptSelected = TRUE;
            }

            if (PrevSelIdx != SelIdx)
            {
                // Clear selection for old index
                if (PrevSelIdx >= 0 && PrevSelIdx < NUM_OPTIONS)
                {
                    DisplayMenuOption(OptionsRect[PrevSelIdx].XPos, OptionsRect[PrevSelIdx].YPos, OptText[PrevSelIdx], &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect[PrevSelIdx].Width);
                }

                if (SelIdx >= 0 && SelIdx < NUM_OPTIONS)
                {
                    // Set selection for new index
                    DisplayMenuOption(OptionsRect[SelIdx].XPos, OptionsRect[SelIdx].YPos, OptText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect[SelIdx].Width);
                }

                PrevSelIdx = SelIdx;
            }

            if (bOptSelected)
            {
                // Clear status message
                ClearTextXYAlign(LeftOffset + TextWidth + 50, LINENO_YPOS(28), NULL, ALIGN_LEFT, ErrorMsg);

                ErrorMsg[0] = L'\0';

                if (SelIdx == 0)
                {
                    Finished = TRUE;
                    break;
                }
                else if (SelIdx == 1)
                {
                    DimmSide = DimmSide == DIMM_FRONT_SIDE ? DIMM_BACK_SIDE : DIMM_FRONT_SIDE;
                    break;
                }
                else if (SelIdx == 2 || SelIdx == 3)
                {
                    if (SelIdx == 2 && DIMMPage > 0)
                        DIMMPage--;
                    else if (SelIdx == 3 && g_numSMBIOSMem > (DIMMPage + 1) * NUM_DIMMS)
                        DIMMPage++;
                    break;
                }
                else if (SelIdx == 4)
                {
                    CHAR16 Filename[128];
                    CHAR16 Prefix[128];
                    MT_HANDLE FileHandle = NULL;
                    EFI_STATUS Status;
                    EFI_TIME Time;

                    gRT->GetTime(&Time, NULL);

                    Prefix[0] = L'\0';
                    MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
                    UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.bmp", Prefix, DIMM_RESULTS_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                    AsciiFPrint(DEBUG_FILE_HANDLE, "Saving screenshot to %s", Filename);
                    Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                    if (FileHandle == NULL)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to open file \"%s\" for writing: %r", Filename, Status);
                        PrintXYAlign(LeftOffset + TextWidth + 50, LINENO_YPOS(28), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), ErrorMsg, sizeof(ErrorMsg)));
                    }
                    else
                    {
                        EG_IMAGE *Image = NULL;
                        DrawDIMMResults(LeftOffset, TopOffset, 0, g_numSMBIOSMem - 1, DIMM_BOTH_SIDES, &Image);
                        if (Image != NULL)
                        {
                            egSaveImage(FileHandle, Image);
                            MtSupportCloseFile(FileHandle);
                            FileHandle = NULL;
                            AsciiFPrint(DEBUG_FILE_HANDLE, "Screenshot successfully saved to \"%s\"", Filename);

                            egFreeImage(Image);
                            UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_DIMM_RESULTS_SAVED), TempBuf, sizeof(TempBuf)), Filename);
                            PrintXYAlign(LeftOffset + TextWidth + 50, LINENO_YPOS(28), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, ErrorMsg);
                        }
                        else
                        {
                            AsciiFPrint(DEBUG_FILE_HANDLE, "Failed to create DIMM results bitmap");
                        }
                    }
                }
            }
        }
    }
}
#endif

#define CINBUF 18
static int getval(UINTN x, UINTN y, int len, int decimalonly, UINT64 *val)
{
    int ret = 0;
    UINT64 tempval;
    int done;
    int i, n;
    int base;
    int shift;
    CHAR16 buf[CINBUF + 1];
    EFI_INPUT_KEY Key;

    // Clear buffer
    SetMem(buf, sizeof(buf), 0);
    for (i = 0; i < len; i++)
    {
        buf[i] = L' ';
    }
    PrintXYAlign(x, y, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, buf);

    done = 0;
    n = 0;
    base = 10;
    while (!done)
    {

        SetMem(&Key, sizeof(Key), 0);

        /* Read a new character and process it */
        MtUiWaitForKey(0, &Key);

        switch (Key.UnicodeChar)
        {
        case 13: /* CR */
            /* If something has been entered we are done */
            if (n)
                done = 1;
            break;

        case 8: /* BS */
            if (n > 0)
            {
                n -= 1;
                buf[n] = L' ';
            }
            break;

        case 0x0:
        {
            switch (Key.ScanCode)
            {
            case 0x26: /* ^L/L - redraw the display */
                break;
            case 0x17: /* Esc */
                ret = 1;
                goto Exit;
            }
        }
        break;

        case L'p': /* p */
            if (decimalonly == 0 && n < len)
                buf[n] = L'p';
            break;
        case L'g': /* g */
            if (decimalonly == 0 && n < len)
                buf[n] = L'g';
            break;
        case L'm': /* m */
            if (decimalonly == 0 && n < len)
                buf[n] = L'm';
            break;
        case L'k': /* k */
            if (decimalonly == 0 && n < len)
                buf[n] = L'k';
            break;
        case L'x': /* x */
            /* Only allow 'x' after an initial 0 */
            if (decimalonly == 0 && n == 1 && (buf[0] == L'0'))
            {
                if (n < len)
                    buf[n] = L'x';
            }
            break;

            /* Don't allow entering a number not in our current base */
        case L'0':
            if (base >= 1)
                if (n < len)
                    buf[n] = L'0';
            break;
        case L'1':
            if (base >= 2)
                if (n < len)
                    buf[n] = L'1';
            break;
        case L'2':
            if (base >= 3)
                if (n < len)
                    buf[n] = L'2';
            break;
        case L'3':
            if (base >= 4)
                if (n < len)
                    buf[n] = L'3';
            break;
        case L'4':
            if (base >= 5)
                if (n < len)
                    buf[n] = L'4';
            break;
        case L'5':
            if (base >= 6)
                if (n < len)
                    buf[n] = L'5';
            break;
        case L'6':
            if (base >= 7)
                if (n < len)
                    buf[n] = L'6';
            break;
        case L'7':
            if (base >= 8)
                if (n < len)
                    buf[n] = L'7';
            break;
        case L'8':
            if (base >= 9)
                if (n < len)
                    buf[n] = L'8';
            break;
        case L'9':
            if (base >= 10)
                if (n < len)
                    buf[n] = L'9';
            break;
        case L'a':
            if (base >= 11)
                if (n < len)
                    buf[n] = L'a';
            break;
        case L'b':
            if (base >= 12)
                if (n < len)
                    buf[n] = L'b';
            break;
        case L'c':
            if (base >= 13)
                if (n < len)
                    buf[n] = L'c';
            break;
        case L'd':
            if (base >= 14)
                if (n < len)
                    buf[n] = L'd';
            break;
        case L'e':
            if (base >= 15)
                if (n < len)
                    buf[n] = L'e';
            break;
        case L'f':
            if (base >= 16)
                if (n < len)
                    buf[n] = L'f';
            break;
        default:
            break;
        }
        /* Don't allow anything to be entered after a suffix */
        if (n > 0 && ((buf[n - 1] == L'p') || (buf[n - 1] == L'g') ||
                      (buf[n - 1] == L'm') || (buf[n - 1] == L'k')))
        {
            buf[n] = L' ';
        }
        /* If we have entered a character increment n */
        if (buf[n] != L' ')
        {
            n++;
        }
        buf[n] = L' ';
        /* Print the current number */
        buf[len] = L'\0';
        PrintXYAlign(x, y, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, buf);

        /* Find the base we are entering numbers in */
        base = 10;
        if ((buf[0] == L'0') && (buf[1] == L'x'))
        {
            base = 16;
        }
#if 0
        else if (buf[0] == '0') {
            base = 8;
        }
#endif
    }
    /* Compute our current shift */
    shift = 0;
    switch (buf[n - 1])
    {
    case L'g': /* gig */
        shift = 30;
        break;
    case L'm': /* meg */
        shift = 20;
        break;
    case L'p': /* page */
        shift = 12;
        break;
    case L'k': /* kilo */
        shift = 10;
        break;
    }

    /* Compute our current value */
    if (base == 16)
        tempval = StrHexToUint64(buf);
    else
        tempval = StrDecimalToUint64(buf);

    if (shift > 0)
    {
        /*
        if (shift >= 32) {
            val = 0xffffffff;
        } else {
            val <<= shift;
        }
        */
        tempval = LShiftU64(tempval, shift);
    }
    else
    {
        /*
        if (-shift >= 32) {
            val = 0;
        }
        else {
            val >>= -shift;
        }
        */
        tempval = RShiftU64(tempval, -shift);
    }

    *val = tempval;
Exit:
    // Option has been selected, clear from screen
    for (i = 0; i < len; i++)
        buf[i] = L' ';
    buf[sizeof(buf) / sizeof(buf[0]) - 1] = L'\0';
    PrintXYAlign(x, y, &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, buf);
    return ret;
}

UINT16
AddressRangeScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    CHAR16 TempBuf[128];

    MOUSE_EVENT mouseEv;
    EG_RECT SelRect;

    HidePointer();

    // Print instructinos on top
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(0), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INSTR1), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INSTR2), TempBuf, sizeof(TempBuf)));

    // Print maximum system address
    PrintXYAlign(LeftOffset, LINENO_YPOS(3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_MAXADDR), TempBuf, sizeof(TempBuf)), maxaddr);

    // Address Range options
    UnicodeSPrint(sAddrRangeText[0], sizeof(sAddrRangeText[0]), GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_LOWER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitLo);
    UnicodeSPrint(sAddrRangeText[1], sizeof(sAddrRangeText[1]), GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_UPPER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitHi);
    UnicodeSPrint(sAddrRangeText[2], sizeof(sAddrRangeText[2]), GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_DEFAULT), TempBuf, sizeof(TempBuf)));

    // Get the longest text width of all the options
    for (i = 0; i < NUM_ADDR_RANGE_OPTIONS; i++)
    {
        UINTN LineWidth = 0;
        GetTextInfo(sAddrRangeText[i], &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    // Display address range options
    for (i = 0; i < NUM_ADDR_RANGE_OPTIONS; i++)
    {
        DisplayMenuOption(LeftOffset, LINENO_YPOS(9 + i), sAddrRangeText[i], &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);
    }

    // Address range options rectangle for mouse handling
    SelRect.XPos = LeftOffset;
    SelRect.YPos = LINENO_YPOS(9);
    SelRect.Width = TextWidth;
    SelRect.Height = NUM_ADDR_RANGE_OPTIONS * LINE_SPACING;

    DrawPointer();

    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NUM_ADDR_RANGE_OPTIONS - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % NUM_ADDR_RANGE_OPTIONS;
            }
            else if (Key.UnicodeChar == 13)
            {

                if (SelIdx >= 0)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();
            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&SelRect))
            {
                SelIdx = (int)((curY - SelRect.YPos) / LINE_SPACING);
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            // Check if mouse over button
            if (SelIdx >= 0)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();

            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NUM_ADDR_RANGE_OPTIONS)
                DisplayMenuOption(LeftOffset, LINENO_YPOS(9 + PrevSelIdx), sAddrRangeText[PrevSelIdx], &DEFAULT_TEXT_PIXEL, FALSE, SelRect.Width);

            // Set selection for new index
            if (SelIdx >= 0 && SelIdx < NUM_ADDR_RANGE_OPTIONS)
                DisplayMenuOption(LeftOffset, LINENO_YPOS(9 + SelIdx), sAddrRangeText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, SelRect.Width);

            PrevSelIdx = SelIdx;

            DrawPointer();
        }

        if (bOptSelected)
        {
            if (SelIdx == 0 || SelIdx == 1) // Lower Limit/Upper Limit
            {
                UINTN TextHeight;
                UINT64 Addr;
                CHAR16 ErrorMsg[128];

                HidePointer();

                XPos = LeftOffset + SelRect.Width + 10;
                PrintXYAlign(XPos, LINENO_YPOS(9 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));
                GetTextInfo(GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)), &TextWidth, &TextHeight);

                // Display insructions on address limit input
                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(5), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INPUT_INSTR1), TempBuf, sizeof(TempBuf)));
                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(6), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INPUT_INSTR2), TempBuf, sizeof(TempBuf)));
                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(7), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INPUT_INSTR3), TempBuf, sizeof(TempBuf)));

                SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
                while (TRUE)
                {
                    if (getval(XPos + TextWidth, LINENO_YPOS(9 + SelIdx), CINBUF, 0, &Addr) == 0)
                    {
                        Addr &= ~0x0FFF;
                        if (SelIdx == 0) // Lower Limit
                        {
                            if (Addr < gAddrLimitHi) // Check if lower limit is lower than upper limit
                            {
                                gAddrLimitLo = Addr;
                                UnicodeSPrint(sAddrRangeText[0], sizeof(sAddrRangeText[0]), GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_LOWER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitLo);
                                break;
                            }
                            else
                            {
                                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(13), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_BAD_LOWER_ADDR), ErrorMsg, sizeof(ErrorMsg)));
                            }
                        }
                        else if (SelIdx == 1) // Upper Limit
                        {
                            if (Addr > gAddrLimitLo && Addr <= maxaddr) // Check if upper limit is greater than lower limit, and less than the max system address
                            {
                                gAddrLimitHi = Addr;
                                UnicodeSPrint(sAddrRangeText[1], sizeof(sAddrRangeText[1]), GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_UPPER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitHi);
                                break;
                            }
                            else if (Addr <= gAddrLimitLo)
                            {
                                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(13), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_BAD_UPPER_ADDR), ErrorMsg, sizeof(ErrorMsg)));
                            }
                            else if (Addr > maxaddr)
                            {
                                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(13), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_INVALID_ADDR), ErrorMsg, sizeof(ErrorMsg)));
                            }
                        }
                    }
                    else
                        break;
                }

                ClearTextXYAlign(XPos, LINENO_YPOS(9 + SelIdx), NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_NEW_VALUE), TempBuf, sizeof(TempBuf)));
                ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(13), NULL, ALIGN_CENTRE, ErrorMsg);

                // Clear instructions
                ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(5), NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INPUT_INSTR1), TempBuf, sizeof(TempBuf)));
                ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(6), NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INPUT_INSTR2), TempBuf, sizeof(TempBuf)));
                ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(7), NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_INPUT_INSTR3), TempBuf, sizeof(TempBuf)));

                // Set selection for new index
                DisplayMenuOption(LeftOffset, LINENO_YPOS(9 + SelIdx), sAddrRangeText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, SelRect.Width);

                DrawPointer();
            }
            else if (SelIdx == 2) // Reset limits to default
            {
                HidePointer();
                gAddrLimitLo = minaddr;
                gAddrLimitHi = maxaddr;
                UnicodeSPrint(sAddrRangeText[0], sizeof(sAddrRangeText[0]), GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_LOWER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitLo);
                UnicodeSPrint(sAddrRangeText[1], sizeof(sAddrRangeText[1]), GetStringById(STRING_TOKEN(STR_MENU_MEMRANGE_UPPER_LIMIT), TempBuf, sizeof(TempBuf)), gAddrLimitHi);
                PrintXYAlign(LeftOffset, LINENO_YPOS(9), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, sAddrRangeText[0]);
                PrintXYAlign(LeftOffset, LINENO_YPOS(10), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, sAddrRangeText[1]);
                DrawPointer();
            }
        }
    }
}

UINT16 CPUSelectScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    MOUSE_EVENT mouseEv;
    EG_RECT TestRect;
    CHAR16 TextBuf[LONG_STRING_LEN];
    CHAR16 TempBuf[128];
    CHAR16 ErrorMsg[128];

    HidePointer();

    // Print instructions on top
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(0), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_INSTR1), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_INSTR2), TempBuf, sizeof(TempBuf)));

    // If multiprocessing is not support, display warning
    if (!gMPSupported)
        PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_WARN_MULTIPROCESSOR), TempBuf, sizeof(TempBuf)));
    else if (gRestrictFlags & RESTRICT_FLAG_MP)
        PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(3), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_WARN_MP_BLACKLIST), TempBuf, sizeof(TempBuf)));

    // Display all available processors that can be used for testing
    UnicodeSPrint(TextBuf, sizeof(TextBuf), GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_AVAIL_CPUS), TempBuf, sizeof(TempBuf)));
    if (MPSupportGetMaxProcessors() > 16)
        UnicodeSPrint(TextBuf, sizeof(TextBuf), L"0 - %d ", (int)(MPSupportGetMaxProcessors() - 1));
    else
    {
        for (i = 0; i < (int)MPSupportGetMaxProcessors(); i++)
        {
            if (MPSupportIsProcEnabled(i))
            {
                UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d ", i);
                StrCatS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), TempBuf);
            }
        }
    }

    PrintXYAlign(LeftOffset, LINENO_YPOS(4), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TextBuf);

    // Get the longest text width of all the options
    for (i = 0; i < NUM_CPU_SEL_MODE; i++)
    {
        UINTN LineWidth = 0;
        if (i == 0)
            UnicodeSPrint(sCPUSelText[0], sizeof(sCPUSelText[0]), GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_SINGLE), TempBuf, sizeof(TempBuf)), gSelCPUNo);
        else
            UnicodeSPrint(sCPUSelText[i], sizeof(sCPUSelText[i]), GetStringById(CPU_SEL_OPTIONS_STR_ID[i], TempBuf, sizeof(TempBuf)));

        GetTextInfo(sCPUSelText[i], &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    // CPU Selection options
    for (i = 0; i < NUM_CPU_SEL_MODE; i++)
    {
        DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(6 + i), sCPUSelText[i], &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

        if (gCPUSelMode == (UINTN)i)
            PrintXYAlign(LeftOffset, LINENO_YPOS(6 + i), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"*");
        else
            PrintXYAlign(LeftOffset, LINENO_YPOS(6 + i), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L" ");
    }

    // CPU selection options rectangle for mouse handling
    TestRect.XPos = LeftOffset + gCharWidth; // (ScreenWidth - TextWidth) >> 1;
    TestRect.YPos = LINENO_YPOS(6);
    TestRect.Width = TextWidth;
    TestRect.Height = NUM_CPU_SEL_MODE * LINE_SPACING;

    DrawPointer();

    SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NUM_CPU_SEL_MODE - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % NUM_CPU_SEL_MODE;
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0 && SelIdx < NUM_CPU_SEL_MODE)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();

            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&TestRect))
            {
                SelIdx = (int)((curY - TestRect.YPos) / LINE_SPACING);
            }

            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            // Check if mouse over button
            if (SelIdx >= 0 && SelIdx < NUM_CPU_SEL_MODE)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();

            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NUM_CPU_SEL_MODE)
                DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(6 + PrevSelIdx), sCPUSelText[PrevSelIdx], &DEFAULT_TEXT_PIXEL, FALSE, TestRect.Width);

            // Set selection for new index
            if (SelIdx >= 0 && SelIdx < NUM_CPU_SEL_MODE)
                DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(6 + SelIdx), sCPUSelText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, TestRect.Width);

            PrevSelIdx = SelIdx;

            DrawPointer();
        }

        if (bOptSelected)
        {
            HidePointer();
            ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(12), NULL, ALIGN_CENTRE, ErrorMsg);
            ErrorMsg[0] = L'\0';
            if (SelIdx == 0) // Single CPU
            {
                UINTN TextHeight;
                UINT64 CPUNo;

                // Prompt the user to input new CPU #
                XPos = LeftOffset + TestRect.Width + 10;
                PrintXYAlign(XPos, LINENO_YPOS(6 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_NEW_CPU), TempBuf, sizeof(TempBuf)));
                GetTextInfo(GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_NEW_CPU), TempBuf, sizeof(TempBuf)), &TextWidth, &TextHeight);

                while (TRUE)
                {
                    if (getval(XPos + TextWidth, LINENO_YPOS(6 + SelIdx), 2, 1, &CPUNo) == 0)
                    {
                        if ((UINTN)CPUNo < MPSupportGetMaxProcessors() && MPSupportIsProcEnabled((UINTN)CPUNo)) // Is CPU valid?
                        {
                            PrintXYAlign(LeftOffset, LINENO_YPOS(6 + gCPUSelMode), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L" ");

                            gCPUSelMode = (UINTN)SelIdx;
                            gSelCPUNo = (UINTN)CPUNo;

                            PrintXYAlign(LeftOffset, LINENO_YPOS(6 + gCPUSelMode), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"*");

                            UnicodeSPrint(sCPUSelText[0], sizeof(sCPUSelText[0]), GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_SINGLE), TempBuf, sizeof(TempBuf)), gSelCPUNo);
                            DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(6), sCPUSelText[0], &DEFAULT_TEXT_PIXEL, TRUE, TestRect.Width);
                            break;
                        }
                        else
                        {
                            PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(12), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_INVALID_CPUS), ErrorMsg, sizeof(ErrorMsg)));
                        }
                    }
                    else
                        break;
                }

                ClearTextXYAlign(XPos, LINENO_YPOS(6 + SelIdx), NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(STR_MENU_CPUSEL_NEW_CPU), TempBuf, sizeof(TempBuf)));
                ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(12), NULL, ALIGN_CENTRE, ErrorMsg);
            }
            else // All other CPU selection modes
            {
                if (!gMPSupported) // If multiprocessing not supported, display error message
                {
                    PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(12), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_MULTI_CPU), ErrorMsg, sizeof(ErrorMsg)));
                }
                else
                {
                    PrintXYAlign(LeftOffset, LINENO_YPOS(6 + gCPUSelMode), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L" ");

                    gCPUSelMode = (UINTN)SelIdx;
                    gSelCPUNo = 0;

                    PrintXYAlign(LeftOffset, LINENO_YPOS(6 + gCPUSelMode), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"*");
                }
            }
            DrawPointer();
        }
    }
}

VOID FreeStringTable(CHAR16 **StringTable, int NumOpts)
{
    int i;

    if (StringTable == NULL)
        return;

    for (i = 0; i < NumOpts; i++)
    {
        if (StringTable[i])
            FreePool(StringTable[i]);
    }
    FreePool(StringTable);
}

CHAR16 **AllocateStringTable(const EFI_STRING_ID StringIdTable[], int NumOpts)
{
    int i;
    CHAR16 **StrTbl = AllocateZeroPool(NumOpts * sizeof(CHAR16 *));

    if (StrTbl == NULL)
        return NULL;

    for (i = 0; i < NumOpts; i++)
    {
        CHAR16 TempBuf[128];
        TempBuf[0] = L'\0';
        GetStringById(STRING_TOKEN(StringIdTable[i]), TempBuf, sizeof(TempBuf));
        StrTbl[i] = AllocateZeroPool((StrLen(TempBuf) + 1) * sizeof(CHAR16));
        if (StrTbl[i] == NULL)
        {
            FreeStringTable(StrTbl, NumOpts);
            return NULL;
        }
        StrCpyS(StrTbl[i], (StrLen(TempBuf) + 1), TempBuf);
    }
    return StrTbl;
}

int getoption(UINTN x, UINTN y, CHAR16 *OptionText[], int numoptions)
{
    int i;
    UINTN TextWidth = 0;
    UINTN TextLen = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    EG_RECT OptionsRect;

    // Get the text width of the longest option
    for (i = 0; i < numoptions; i++)
    {
        UINTN StringWidth;
        UINTN StringLen;

        GetTextInfo(OptionText[i], &StringWidth, NULL);
        if (StringWidth > TextWidth)
            TextWidth = StringWidth;

        StringLen = StrLen(OptionText[i]);
        if (StringLen > TextLen)
            TextLen = StringLen;
    }

    // Display the list of options
    for (i = 0; i < numoptions; i++)
        DisplayMenuOption(x, y + LINE_SPACING * i, OptionText[i], &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

    OptionsRect.XPos = x;
    OptionsRect.YPos = y;
    OptionsRect.Width = TextWidth;
    OptionsRect.Height = numoptions * LINE_SPACING;

    while (TRUE)
    {
        UINTN EventIdx;
        if (gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIdx) == EFI_SUCCESS)
        {
            EFI_INPUT_KEY Key;
            SetMem(&Key, sizeof(Key), 0);
            gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? numoptions - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % numoptions;

                if (PrevSelIdx != SelIdx)
                {
                    // Clear selection for old index
                    if (PrevSelIdx >= 0 && PrevSelIdx < numoptions)
                        DisplayMenuOption(x, y + LINE_SPACING * PrevSelIdx, OptionText[PrevSelIdx], &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect.Width);

                    // Set selection for new index
                    if (SelIdx >= 0 && SelIdx < numoptions)
                        DisplayMenuOption(x, y + LINE_SPACING * SelIdx, OptionText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect.Width);

                    PrevSelIdx = SelIdx;
                }
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0 && SelIdx < numoptions)
                    break;
            }
            else if (Key.ScanCode == 0x17 /* Esc */)
            {
                SelIdx = -1;
                break;
            }
        }
    }

    // We are finished, erase the area
    for (i = 0; i < numoptions; i++)
        egFillArea(NULL, &BG_COLOUR, x, y + LINE_SPACING * i, OptionsRect.Width, gTextHeight);

    return SelIdx;
}

UINT16 RAMBenchmarkScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;

    int PrevSelIdx = 0, SelIdx = 0;
    MOUSE_EVENT mouseEv;
    EG_RECT ParamsRect;
    EG_RECT CmdsRect;
    CHAR16 TempBuf[128];

    HidePointer();

    // Print instructions on top
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(0), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_BENCH_INSTR1), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_BENCH_INSTR2), TempBuf, sizeof(TempBuf)));

    // Display status
    PrintXYAlign(LeftOffset + 50, LINENO_YPOS(13), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s: ", GetStringById(STRING_TOKEN(STR_MENU_BENCH_AVAIL_MEM), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + 50 + gCharWidth * 25, LINENO_YPOS(13), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%ld Bytes", (UINT64)memsize);
    PrintXYAlign(LeftOffset + 50, LINENO_YPOS(14), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s: ", GetStringById(STRING_TOKEN(STR_BLOCK_SIZE), TempBuf, sizeof(TempBuf)));
    PrintXYAlign(LeftOffset + 20, LINENO_YPOS(17), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%s: ", GetStringById(STRING_TOKEN(STR_MENU_BENCH_PROGRESS), TempBuf, sizeof(TempBuf)));

    // Initialise the progress bar
    DrawProgressBar(&PROGBAR_COLOUR_PIXEL, (EG_PIXEL *)&DEFAULT_TEXT_PIXEL, LeftOffset + 20, LINENO_YPOS(18), PROG_BAR_WIDTH, LINE_SPACING, 0);

    // RAM Benchmark parameters
    for (i = 0; i < NUM_BENCHMARK_PARAMS; i++)
    {
        UINTN LineWidth = 0;
        GetTextInfo(GetStringById(STRING_TOKEN(BENCH_PARAMS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    for (i = 0; i < NUM_BENCHMARK_PARAMS; i++)
    {
        DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + i), GetStringById(STRING_TOKEN(BENCH_PARAMS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

        if (sBenchSubOptions[i])
            PrintXYAlign(LeftOffset + TextWidth, LINENO_YPOS(4 + i), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(sBenchSubOptions[i][sBenchSubOptionsSel[i]]), TempBuf, sizeof(TempBuf)));
    }

    // Display parameters
    ParamsRect.XPos = LeftOffset; // (ScreenWidth - TextWidth) >> 1;
    ParamsRect.YPos = LINENO_YPOS(4);
    ParamsRect.Width = TextWidth;
    ParamsRect.Height = NUM_BENCHMARK_PARAMS * LINE_SPACING;

    // RAM Benchmark commands
    TextWidth = 0;
    for (i = 0; i < NUM_BENCH_CMDS; i++)
    {
        UINTN LineWidth = 0;
        GetTextInfo(GetStringById(STRING_TOKEN(BENCH_CMD_STR_ID[i]), TempBuf, sizeof(TempBuf)), &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    for (i = 0; i < NUM_BENCH_CMDS; i++)
        DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + NUM_BENCHMARK_PARAMS + 1 + i), GetStringById(STRING_TOKEN(BENCH_CMD_STR_ID[i]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, SelIdx == NUM_BENCHMARK_PARAMS + i, TextWidth);

    // Display commands
    CmdsRect.XPos = LeftOffset; // (ScreenWidth - TextWidth) >> 1;
    CmdsRect.YPos = LINENO_YPOS(4 + NUM_BENCHMARK_PARAMS + 1);
    CmdsRect.Width = TextWidth;
    CmdsRect.Height = NUM_BENCH_CMDS * LINE_SPACING;

    DrawPointer();

    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NUM_BENCH_OPTIONS - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % NUM_BENCH_OPTIONS;
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0 && SelIdx < NUM_BENCH_OPTIONS)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();
            HidePointer();

            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&ParamsRect))
            {
                SelIdx = (int)((curY - ParamsRect.YPos) / LINE_SPACING);
            }
            else if (MouseInRect(&CmdsRect))
            {
                int CmdIdx = (int)((curY - CmdsRect.YPos) / LINE_SPACING);
                SelIdx = NUM_BENCHMARK_PARAMS + CmdIdx;
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            // Check if mouse over button
            if (SelIdx >= 0 && SelIdx < NUM_BENCH_OPTIONS)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();

            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NUM_BENCH_OPTIONS)
            {
                if (PrevSelIdx < NUM_BENCHMARK_PARAMS)
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + PrevSelIdx), GetStringById(STRING_TOKEN(BENCH_PARAMS_STR_ID[PrevSelIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, FALSE, ParamsRect.Width);
                else
                {
                    int CmdIdx = PrevSelIdx - NUM_BENCHMARK_PARAMS;
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + NUM_BENCHMARK_PARAMS + 1 + CmdIdx), GetStringById(STRING_TOKEN(BENCH_CMD_STR_ID[CmdIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, FALSE, CmdsRect.Width);
                }
            }

            // Set selection for new index
            if (SelIdx >= 0 && SelIdx < NUM_BENCH_OPTIONS)
            {
                if (SelIdx < NUM_BENCHMARK_PARAMS)
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + SelIdx), GetStringById(STRING_TOKEN(BENCH_PARAMS_STR_ID[SelIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, TRUE, ParamsRect.Width);
                else
                {
                    int CmdIdx = SelIdx - NUM_BENCHMARK_PARAMS;
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + NUM_BENCHMARK_PARAMS + 1 + CmdIdx), GetStringById(STRING_TOKEN(BENCH_CMD_STR_ID[CmdIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, TRUE, CmdsRect.Width);
                }
            }

            PrevSelIdx = SelIdx;
            DrawPointer();
        }

        if (bOptSelected)
        {
            HidePointer();

            if (SelIdx >= 0 && SelIdx < NUM_BENCH_OPTIONS)
            {
                if (SelIdx < NUM_BENCHMARK_PARAMS) // User wants to change parameters
                {
                    // Parameter selected, display list of suboptions for selected parameter
                    int SubIdx;
                    int NumOpts = 0;
                    UINTN MaxWidth = 0;
                    UINTN OptXPos = 0;

                    // Get maximum width of parameter suboptions
                    for (i = 0; i < NUM_BENCHMARK_PARAMS; i++)
                    {
                        int j;
                        for (j = 0; sBenchSubOptions[i][j] != (EFI_STRING_ID)-1; j++)
                        {
                            UINTN StringWidth = 0;
                            GetTextInfo(GetStringById(sBenchSubOptions[i][j], TempBuf, sizeof(TempBuf)), &StringWidth, NULL);

                            if (StringWidth > MaxWidth)
                                MaxWidth = StringWidth;
                        }
                    }

                    OptXPos = LeftOffset + ParamsRect.Width + MaxWidth + 5;

                    // Get the number of suboptions for current parameter
                    while (sBenchSubOptions[SelIdx][NumOpts] != (EFI_STRING_ID)-1)
                        NumOpts++;

                    CHAR16 **BenchOptions = AllocateStringTable(sBenchSubOptions[SelIdx], NumOpts);

                    SubIdx = getoption(OptXPos, LINENO_YPOS(4 + SelIdx), BenchOptions, NumOpts);

                    FreeStringTable(BenchOptions, NumOpts);

                    if (SubIdx >= 0) // If suboption selected, update UI
                    {
                        // Erase the previous suboption
                        ClearTextXYAlign(LeftOffset + ParamsRect.Width, LINENO_YPOS(4 + SelIdx), NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(sBenchSubOptions[SelIdx][sBenchSubOptionsSel[SelIdx]]), TempBuf, sizeof(TempBuf)));
                        sBenchSubOptionsSel[SelIdx] = SubIdx;
                        if (SelIdx == 1) // Test mode changed
                        {
                            ClearTextXYAlign(LeftOffset + ParamsRect.Width, LINENO_YPOS(4 + 2), NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(sBenchSubOptions[2][sBenchSubOptionsSel[2]]), TempBuf, sizeof(TempBuf)));
                            if (SubIdx == 0) // If test mode is MEM_STEP, disable 'Data Width' parameter
                            {
                                sBenchSubOptions[2] = BENCH_DISABLED_OPTIONS_STR_ID;
                                sBenchSubOptionsSel[2] = 0;
                            }
                            else if (SubIdx == 1) // If test mode is MEM_BLOCK, enable 'Data Width' parameter and set to 64-bits as default
                            {
                                sBenchSubOptions[2] = BENCH_DATA_WIDTH_OPTIONS_STR_ID;
                                sBenchSubOptionsSel[2] = 3; // 64-bits
                            }

                            PrintXYAlign(LeftOffset + ParamsRect.Width, LINENO_YPOS(4 + 2), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(sBenchSubOptions[2][sBenchSubOptionsSel[2]]), TempBuf, sizeof(TempBuf)));
                        }
                    }
                    // Set selection for new index
                    PrintXYAlign(LeftOffset + ParamsRect.Width, LINENO_YPOS(4 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(sBenchSubOptions[SelIdx][sBenchSubOptionsSel[SelIdx]]), TempBuf, sizeof(TempBuf)));
                }
                else // Benchmark commands
                {
                    int CmdIdx = SelIdx - NUM_BENCHMARK_PARAMS;
                    if (CmdIdx == 0) // Start
                    {
                        gBenchConfig.ReadWrite = READ_WRITE_MAP[sBenchSubOptionsSel[0]];
                        gBenchConfig.TestMode = TEST_MODE_MAP[sBenchSubOptionsSel[1]];
                        gBenchConfig.DataWidth = DATA_WIDTH_MAP[sBenchSubOptionsSel[2]];
                        gRT->GetTime(&gBenchConfig.StartTime, NULL);

                        if (RunAdvancedMemoryTest(XPos, YPos, &gBenchConfig))
                        {
                            PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(20), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_BENCH_VIEW_RESULTS), TempBuf, sizeof(TempBuf)));

                            MtUiWaitForKey(0, NULL);

                            SetupBenchLineChart(gBenchConfig.TestMode);
                            LoadCurBenchResultsToChart(0, FALSE);
                            LineChart_SetNumSer(1);
                            return ID_BUTTON_BENCHRESULTS;
                        }
                    }
                    else if (CmdIdx == 1) // previous past results
                    {
                        gBenchConfig.TestMode = TEST_MODE_MAP[sBenchSubOptionsSel[1]];
                        return ID_BUTTON_PREVBENCHRESULTS;
                    }
                }
            }

            DrawPointer();
        }
    }
}

UINT16 BenchmarkResultsScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    MOUSE_EVENT mouseEv;
    EG_RECT OptionsRect;
    UINT16 RetScreen = ID_BUTTON_BENCHMARK;
    CHAR16 TempBuf[128];
    CHAR16 ErrorMsg[128];

    HidePointer();

    // Display chart
    LineChart_SetChartWidth((int)(WindowWidth - XPos) * 3 / 5);
    LineChart_SetChartHeight((int)(WindowHeight - YPos) / 2);
    LineChart_DisplayChart(NULL, (int)(XPos + 20), (int)(YPos + 20));

    for (i = 0; i < NUM_BENCHRESULTS_OPTIONS; i++)
    {
        UINTN LineWidth;

        GetTextInfo(GetStringById(STRING_TOKEN(BENCH_RESULTS_OPTIONS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    for (i = 0; i < NUM_BENCHRESULTS_OPTIONS; i++)
        DisplayMenuOption(LeftOffset, LINENO_YPOS(21 + i), GetStringById(STRING_TOKEN(BENCH_RESULTS_OPTIONS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

    // Display options
    OptionsRect.XPos = LeftOffset + 5; // (ScreenWidth - TextWidth) >> 1;
    OptionsRect.YPos = LINENO_YPOS(21);
    OptionsRect.Width = TextWidth;
    OptionsRect.Height = NUM_BENCHRESULTS_OPTIONS * LINE_SPACING;

    DrawPointer();

    SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
            {
                RetScreen = (UINT16)Ret;
                goto Cleanup;
            }

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NUM_BENCHRESULTS_OPTIONS - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % NUM_BENCHRESULTS_OPTIONS;
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0 && SelIdx < NUM_BENCHRESULTS_OPTIONS)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();
            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&OptionsRect))
            {
                SelIdx = (int)((curY - OptionsRect.YPos) / LINE_SPACING);
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
            {
                RetScreen = (UINT16)Ret;
                goto Cleanup;
            }

            // Check if mouse over button
            if (SelIdx >= 0 && SelIdx < NUM_BENCHRESULTS_OPTIONS)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();

            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NUM_BENCHRESULTS_OPTIONS)
                DisplayMenuOption(LeftOffset, LINENO_YPOS(21 + PrevSelIdx), GetStringById(STRING_TOKEN(BENCH_RESULTS_OPTIONS_STR_ID[PrevSelIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect.Width);

            // Set selection for new index
            if (SelIdx >= 0 && SelIdx < NUM_BENCHRESULTS_OPTIONS)
                DisplayMenuOption(LeftOffset, LINENO_YPOS(21 + SelIdx), GetStringById(STRING_TOKEN(BENCH_RESULTS_OPTIONS_STR_ID[SelIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect.Width);

            PrevSelIdx = SelIdx;
            DrawPointer();
        }

        if (bOptSelected)
        {
            HidePointer();

            // Clear status message
            ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), NULL, ALIGN_CENTRE, ErrorMsg);
            ErrorMsg[0] = L'\0';

            if (SelIdx >= 0 && SelIdx < NUM_BENCHRESULTS_OPTIONS)
            {
                switch (SelIdx)
                {
#ifdef PRO_RELEASE
                case 0: // Save chart as BMP
                {
                    MT_HANDLE FileHandle = NULL;
                    EFI_STATUS Status;

                    EFI_TIME Time;
                    CHAR16 Filename[128];
                    CHAR16 Prefix[128];

                    gRT->GetTime(&Time, NULL);

                    Prefix[0] = L'\0';
                    MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
                    UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.bmp", Prefix, BENCH_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                    Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                    if (FileHandle == NULL)
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), ErrorMsg, sizeof(ErrorMsg)));
                    }
                    else
                    {
                        EG_IMAGE *Image = egCreateImage(LineChart_GetFullChartWidth(), LineChart_GetFullChartHeight(), FALSE);
                        if (Image == NULL)
                        {
                            PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_CHART_IMAGE), ErrorMsg, sizeof(ErrorMsg)));
                        }
                        else
                        {
                            EG_PIXEL WhiteColour = {255, 255, 255, 255};

                            LineChart_SetBackgroundColour(&WhiteColour, 0, FALSE);
                            LineChart_DisplayChart(Image, 0, 0);
                            egSaveImage(FileHandle, Image);
                            egFreeImage(Image);
                            // egSaveScreenImage(FileHandle, XPos + 40, YPos + 40, LineChart_GetFullChartWidth() + 20, LineChart_GetFullChartHeight() + 20);

                            UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_MENU_BENCH_CHART_SAVED), TempBuf, sizeof(TempBuf)), Filename);
                            PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, ErrorMsg);
                        }

                        MtSupportCloseFile(FileHandle);
                        FileHandle = NULL;
                    }
                }
                break;
                case 1: // Save report as HTML
                {
                    MT_HANDLE FileHandle = NULL;
                    EFI_STATUS Status;

                    EFI_TIME Time;
                    CHAR16 Filename[128];
                    CHAR16 ChartFilename[128];
                    CHAR16 Prefix[128];

                    gRT->GetTime(&Time, NULL);

                    Prefix[0] = L'\0';
                    MtSupportGetReportPrefix(Prefix, sizeof(Prefix));
                    UnicodeSPrint(ChartFilename, sizeof(ChartFilename), L"%s%s-%s.bmp", Prefix, BENCH_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                    Status = MtSupportOpenFile(&FileHandle, ChartFilename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                    if (FileHandle == NULL)
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), ErrorMsg, sizeof(ErrorMsg)));
                    }
                    else
                    {
                        EG_IMAGE *Image = egCreateImage(LineChart_GetFullChartWidth(), LineChart_GetFullChartHeight(), FALSE);
                        if (Image == NULL)
                        {
                            PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_CHART_IMAGE), ErrorMsg, sizeof(ErrorMsg)));
                        }
                        else
                        {
                            EG_PIXEL WhiteColour = {255, 255, 255, 255};
                            LineChart_SetBackgroundColour(&WhiteColour, 0, FALSE);
                            LineChart_DisplayChart(Image, 0, 0);
                            egSaveImage(FileHandle, Image);
                            egFreeImage(Image);
                            // egSaveScreenImage(FileHandle, XPos + 40, YPos + 40, LineChart_GetFullChartWidth() + 20, LineChart_GetFullChartHeight() + 20);
                        }
                        MtSupportCloseFile(FileHandle);
                        FileHandle = NULL;
                    }

                    // Now save the report
                    UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.html", Prefix, BENCH_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

                    Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

                    if (FileHandle == NULL)
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), ErrorMsg, sizeof(ErrorMsg)));
                    }
                    else
                    {
                        SaveBenchReport(FileHandle, ChartFilename);
                        MtSupportCloseFile(FileHandle);

                        UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_MENU_BENCH_REPORT_SAVED), TempBuf, sizeof(TempBuf)), Filename);
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, ErrorMsg);
                    }
                }
                break;
#endif
                default: // Go back
                {
                    RetScreen = ID_BUTTON_BENCHMARK;
                    goto Cleanup;
                }
                break;
                }
            }

            DrawPointer();
        }
    }
Cleanup:
    LineChart_Destroy();
    return RetScreen;
}

UINT16
PrevBenchmarkResultsScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;
    int PrevSelIdx = 0, SelIdx = 0;
    MOUSE_EVENT mouseEv;

    BOOLEAN ResultSelected[MAX_PREVBENCH_RESULTS];
    CHAR16 ResultFilename[MAX_PREVBENCH_RESULTS][64];

    int NumPrevResults = 0;
    int NumSelResults = 0;

    EG_RECT ResultsRect;
    EG_RECT OptionsRect;
    CHAR16 TempBuf[128];
    CHAR16 ErrorMsg[128];
    SetMem(ResultSelected, sizeof(ResultSelected), 0);

    HidePointer();

    // Print instructions on top
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(0), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_RESULTS_INSTR1), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_RESULTS_INSTR2), TempBuf, sizeof(TempBuf)));

    // Display prev. RAM benchmark results
    {
        MT_HANDLE DirHandle = NULL;
        EFI_STATUS Status = EFI_NOT_FOUND;

        AsciiFPrint(DEBUG_FILE_HANDLE, "Searching for previous benchmark results...");

        // Open the benchmark directory
        Status = MtSupportOpenFile(&DirHandle, BENCH_DIR, EFI_FILE_MODE_READ, 0);
        if (Status != EFI_SUCCESS)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Error opening Benchmark directory: %r", Status);
        }

#if 0
        else if (PXEProtocol)
        {
            UINT64 ReadSize = 256;
            UINT8 DirBuf[256];
            CHAR8 BenchDir[32];
            UnicodeStrToAsciiStr(BENCH_DIR, BenchDir);
            Status = PXEProtocol->Mtftp(PXEProtocol, EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY, DirBuf, FALSE, &ReadSize, NULL, &ServerIP, (UINT8 *)BenchDir, NULL, FALSE);
            Print(L"EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY returned: %r", Status);

            if (Status == EFI_SUCCESS)
            {
                CHAR8   tmpMsg[VLONG_STRING_LEN];
                AsciiStrCpyS(tmpMsg, sizeof(tmpMsg) / sizeof(tmpMsg[0]), "");

                for (i = 0; i < ReadSize; i++)
                {
                    CHAR8 strSPDBytes[8];
                    AsciiSPrint(strSPDBytes, sizeof(strSPDBytes), "%02X ", DirBuf[i]);
                    AsciiStrCatS(tmpMsg, sizeof(tmpMsg) / sizeof(tmpMsg[0]), strSPDBytes);

                    if ((i % 16 == 15) || (i == ReadSize - 1))
                    {
                        MtSupportDebugWriteLine(tmpMsg);
                        AsciiStrCpyS(tmpMsg, sizeof(tmpMsg) / sizeof(tmpMsg[0]), "");
                    }
                }
            }
        }
#endif

        if (DirHandle != NULL)
        {
            // Look for all .ptx files
            EFI_FILE_HANDLE DirEfiHandle = NULL;
            EFI_FILE_INFO *DirEntry = NULL;

            MtSupportGetEfiFileHandle(DirHandle, &DirEfiHandle);

            Status = FileHandleFindFirstFile(DirEfiHandle, &DirEntry);
            if (Status == EFI_SUCCESS)
            {
                for (BOOLEAN NoFile = FALSE; NoFile == FALSE; Status = FileHandleFindNextFile(DirEfiHandle, DirEntry, &NoFile))
                {
                    CHAR16 Filename[128];
                    MT_HANDLE FileHandle = NULL;

                    AsciiFPrint(DEBUG_FILE_HANDLE, "Found dir entry: %s (last=%d)", DirEntry->FileName, NoFile);
                    if (Status != EFI_SUCCESS)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "FileHandleFindNextFile returned error: %r. Freeing memory...", Status);
                        if (DirEntry)
                            FreePool(DirEntry);
                        break;
                    }

                    if ((DirEntry->Attribute & EFI_FILE_DIRECTORY) != 0)
                        continue;

                    if (StrStr(DirEntry->FileName, L".ptx") == NULL)
                        continue;

                    AsciiFPrint(DEBUG_FILE_HANDLE, "Found benchmark results: %s", DirEntry->FileName);

                    if (NumPrevResults >= MAX_PREVBENCH_RESULTS)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "Ignoring \"%s\" - max results exceeded (%d)", DirEntry->FileName, NumPrevResults);
                        continue;
                    }

                    // Open ptx file
                    UnicodeSPrint(Filename, sizeof(Filename), L"%s\\%s", BENCH_DIR, DirEntry->FileName);
                    Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);

                    if (Status != EFI_SUCCESS)
                    {
                        AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to open %s: %r", Filename, Status);
                    }

                    if (FileHandle != NULL)
                    {
#if 0
                        TestResultsMetaInfo TestInfo;
                        MTVersion VersionInfo;
                        MEM_RESULTS Header;

                        AsciiFPrint(DEBUG_FILE_HANDLE, "Parsing benchmark results: %s", Filename);
                        // Try to load the ptx file
                        if (LoadBenchResults(FileHandle, &TestInfo, &VersionInfo, &Header, NULL, NULL, NULL) && Header.iTestMode == gBenchConfig.TestMode)
                        {
                            EFI_TIME TestDate;
                            CHAR16 szAccessSize[16];
                            CHAR16 szRAMType[32];
                            CHAR16 TempBuf2[32];
                            UINTN LineWidth;

                            // Get the date of the results
                            TimestampToEFITime(TestInfo.i64TestStartTime, &TestDate);

                            // Get the access zsize
                            UnicodeSPrint(szAccessSize, sizeof(szAccessSize), L", %dbit", Header.dwAccessDataSize);

                            // Get the RAM info
                            SetMem(szRAMType, sizeof(szRAMType), 0);
                            if (Header.szRAMType[0] != L'\0')
                                StrCpyS(szRAMType, sizeof(szRAMType) / sizeof(szRAMType[0]), Header.szRAMType);
                            else
                                UnicodeSPrint(szRAMType, sizeof(szRAMType), L"%ldMB", DivU64x32(Header.TotalPhys, 1048576));

                            // Create the summary string for the benchmark results
                            UnicodeSPrint(sPrevAdvResultsText[NumPrevResults], sizeof(sPrevAdvResultsText[NumPrevResults]), L"%d-%02d-%02d %02d:%02d:%02d / %s / %s%s / %s",
                                          TestDate.Year, TestDate.Month, TestDate.Day, TestDate.Hour, TestDate.Minute, TestDate.Second,
                                          Header.iTestMode == MEM_STEP ? GetStringById(STRING_TOKEN(STR_STEP_SIZE), TempBuf, sizeof(TempBuf)) : GetStringById(STRING_TOKEN(STR_BLOCK_SIZE), TempBuf, sizeof(TempBuf)),
                                          Header.bRead ? GetStringById(STRING_TOKEN(STR_READ), TempBuf2, sizeof(TempBuf2)) : GetStringById(STRING_TOKEN(STR_WRITE), TempBuf2, sizeof(TempBuf2)),
                                          Header.iTestMode == MEM_BLOCK ? szAccessSize : L"",
                                          szRAMType);

                            GetTextInfo(sPrevAdvResultsText[NumPrevResults], &LineWidth, NULL);
                            if (LineWidth > TextWidth)
                                TextWidth = LineWidth;

                            StrCpyS(ResultFilename[NumPrevResults], 64, DirEntry->FileName);
                            NumPrevResults++;

                            AsciiFPrint(DEBUG_FILE_HANDLE, "Parsing benchmark results successful. Adding %s to list.", DirEntry->FileName);
                        }
#endif
                        MtSupportCloseFile(FileHandle);
                    }
                }
            }

            Status = MtSupportCloseFile(DirHandle);
            AsciiFPrint(DEBUG_FILE_HANDLE, "Traversing directory \"%s\" complete", BENCH_DIR);
        }
    }

    if (NumPrevResults == 0) // No results found
    {
        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(5), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_NO_PREV_RESULTS), TempBuf, sizeof(TempBuf)));
    }

    // Display the results
    for (i = 0; i < NumPrevResults; i++)
        DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(4 + i), sPrevAdvResultsText[i], &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

    // Results rectangle for mouse handling
    ResultsRect.XPos = LeftOffset + gCharWidth;
    ResultsRect.YPos = LINENO_YPOS(4);
    ResultsRect.Width = TextWidth;
    ResultsRect.Height = NumPrevResults * LINE_SPACING;

    // Previous benchmark options
    TextWidth = 0;
    for (i = 0; i < NUM_PREVBENCH_OPTIONS; i++)
    {
        UINTN LineWidth;

        GetTextInfo(GetStringById(STRING_TOKEN(PREV_BENCH_OPTIONS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    for (i = 0; i < NUM_PREVBENCH_OPTIONS; i++)
        DisplayMenuOption(LeftOffset, LINENO_YPOS(21 + i), GetStringById(STRING_TOKEN(PREV_BENCH_OPTIONS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, SelIdx == NumPrevResults + i, TextWidth);

    // Previous benchmark options rectangle for mouse handling
    OptionsRect.XPos = LeftOffset;
    OptionsRect.YPos = LINENO_YPOS(21);
    OptionsRect.Width = TextWidth;
    OptionsRect.Height = NUM_PREVBENCH_OPTIONS * LINE_SPACING;

    DrawPointer();

    SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NumPrevResults + NUM_PREVBENCH_OPTIONS - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % (NumPrevResults + NUM_PREVBENCH_OPTIONS);
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();
            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&ResultsRect))
            {
                SelIdx = (int)((curY - ResultsRect.YPos) / LINE_SPACING);
            }
            else if (MouseInRect(&OptionsRect))
            {
                int OptIdx = (int)((curY - OptionsRect.YPos) / LINE_SPACING);
                SelIdx = NumPrevResults + OptIdx;
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            // Check if mouse over button
            if (SelIdx >= 0)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();
            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NumPrevResults + NUM_PREVBENCH_OPTIONS)
            {
                if (PrevSelIdx < NumPrevResults)
                    DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(4 + PrevSelIdx), sPrevAdvResultsText[PrevSelIdx], &DEFAULT_TEXT_PIXEL, FALSE, ResultsRect.Width);
                else
                {
                    int OptIdx = PrevSelIdx - NumPrevResults;
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(21 + OptIdx), GetStringById(STRING_TOKEN(PREV_BENCH_OPTIONS_STR_ID[OptIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, FALSE, OptionsRect.Width);
                }
            }

            // Set selection for new index
            if (SelIdx >= 0 && SelIdx < NumPrevResults + NUM_PREVBENCH_OPTIONS)
            {
                if (SelIdx < NumPrevResults)
                    DisplayMenuOption(LeftOffset + gCharWidth, LINENO_YPOS(4 + SelIdx), sPrevAdvResultsText[SelIdx], &DEFAULT_TEXT_PIXEL, TRUE, ResultsRect.Width);
                else
                {
                    int OptIdx = SelIdx - NumPrevResults;
                    DisplayMenuOption(LeftOffset, LINENO_YPOS(21 + OptIdx), GetStringById(STRING_TOKEN(PREV_BENCH_OPTIONS_STR_ID[OptIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, TRUE, OptionsRect.Width);
                }
            }

            PrevSelIdx = SelIdx;
            DrawPointer();
        }

        if (bOptSelected)
        {
            HidePointer();

            // Clear status message
            ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), NULL, ALIGN_CENTRE, ErrorMsg);
            ErrorMsg[0] = L'\0';
            if (SelIdx < NumPrevResults) // Benchmark result selected
            {
                NumSelResults = ResultSelected[SelIdx] ? NumSelResults - 1 : NumSelResults + 1;
                ResultSelected[SelIdx] = !ResultSelected[SelIdx];

                if (ResultSelected[SelIdx])
                    PrintXYAlign(LeftOffset, LINENO_YPOS(4 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"*");
                else
                    PrintXYAlign(LeftOffset, LINENO_YPOS(4 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L" ");
            }
            else // Benchmark result option selected
            {
                int OptIdx = SelIdx - NumPrevResults;

                if (OptIdx == 0) // Graph selected results
                {
                    if (NumSelResults == 0) // If no results selected, display error
                    {
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_SELECTED_RESULTS), ErrorMsg, sizeof(ErrorMsg)));
                    }
                    else if (NumSelResults > MAX_BENCHRESULTS_SERIES) // If more than max results selected, display error
                    {
                        UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_ERROR_MAX_RESULTS), TempBuf, sizeof(TempBuf)), MAX_BENCHRESULTS_SERIES);
                        PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, ErrorMsg);
                    }
                    else
                    {
                        int Series = 0;
                        SetupBenchLineChart(gBenchConfig.TestMode);

                        // Load all selected benchmarks and graph to chart
                        for (i = 0; i < NumPrevResults; i++)
                        {
                            if (ResultSelected[i])
                            {
                                CHAR16 Filename[128];
                                MT_HANDLE FileHandle = NULL;
                                EFI_STATUS Status = EFI_NOT_FOUND;

                                // Open ptx file
                                UnicodeSPrint(Filename, sizeof(Filename), L"%s\\%s", BENCH_DIR, ResultFilename[i]);

                                AsciiFPrint(DEBUG_FILE_HANDLE, "Opening file: %s", Filename);
                                Status = MtSupportOpenFile(&FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);

                                if (Status != EFI_SUCCESS)
                                {
                                    AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to open %s: %r", Filename, Status);
                                }

                                if (FileHandle != NULL)
                                {
                                    AsciiFPrint(DEBUG_FILE_HANDLE, "Loading benchmark results to chart: %s", ResultFilename[i]);

                                    if (LoadBenchResultsToChart(FileHandle, Series, FALSE))
                                        Series++;

                                    MtSupportCloseFile(FileHandle);
                                }
                            }
                        }
                        LineChart_SetNumSer(Series);

                        return ID_BUTTON_BENCHRESULTS;
                    }
                }
                else // Glo back
                {
                    return ID_BUTTON_BENCHMARK;
                }
            }
            DrawPointer();
        }
    }
}

UINT16 GeneralSettingsScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    UINTN TextWidth = 0;

    int PrevSelIdx = 0, SelIdx = 0;
    MOUSE_EVENT mouseEv;
    EG_RECT SettingsRect;
    CHAR16 TempBuf[128];
    CHAR16 ErrorMsg[128];
    HidePointer();

    // Print instructions at top
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(0), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_SETTINGS_INSTR1), TempBuf, sizeof(TempBuf)));
    PrintXYAlign((WindowWidth + LeftOffset) / 2, LINENO_YPOS(1), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_MENU_SETTINGS_INSTR2), TempBuf, sizeof(TempBuf)));

    for (i = 0; i < NUM_SETTINGS; i++)
    {
        UINTN LineWidth;

        GetTextInfo(GetStringById(STRING_TOKEN(SETTINGS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &LineWidth, NULL);
        if (LineWidth > TextWidth)
            TextWidth = LineWidth;
    }

    // Display all settings, and current value for each setting
    for (i = 0; i < NUM_SETTINGS; i++)
    {
        DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + i), GetStringById(STRING_TOKEN(SETTINGS_STR_ID[i]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, SelIdx == i, TextWidth);

        if (i == SETTINGS_OPTION_IDX_LANG)
            PrintXYAlign(LeftOffset + TextWidth, LINENO_YPOS(4 + i), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(SETTINGS_LANG_OPTIONS_STR_ID[gCurLang]), TempBuf, sizeof(TempBuf)));
        else if (i == SETTINGS_OPTION_IDX_RES)
        {
            UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d x %d", ScreenWidth, ScreenHeight);
            PrintXYAlign(LeftOffset + TextWidth, LINENO_YPOS(4 + i), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, TempBuf);
        }
    }

    // Settings rectangle for mouse handlign
    SettingsRect.XPos = LeftOffset; // (ScreenWidth - TextWidth) >> 1;
    SettingsRect.YPos = LINENO_YPOS(4);
    SettingsRect.Width = TextWidth;
    SettingsRect.Height = NUM_SETTINGS * LINE_SPACING;

    DrawPointer();

    SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
    while (TRUE)
    {
        BOOL bOptSelected = FALSE;
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;

            if (Key.ScanCode == 1 || Key.ScanCode == 2)
            {
                if (Key.ScanCode == 1)
                    SelIdx = SelIdx <= 0 ? NUM_SETTINGS - 1 : SelIdx - 1;
                else if (Key.ScanCode == 2)
                    SelIdx = SelIdx < 0 ? 0 : (SelIdx + 1) % NUM_SETTINGS;
            }
            else if (Key.UnicodeChar == 13)
            {
                if (SelIdx >= 0 && SelIdx < NUM_SETTINGS)
                    bOptSelected = TRUE;
            }
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();

            HidePointer();
            SelIdx = -1;

            // Check if mouse over button
            if (MouseInRect(&SettingsRect))
            {
                SelIdx = (int)((curY - SettingsRect.YPos) / LINE_SPACING);
            }
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;

            // Check if mouse over button
            if (SelIdx >= 0 && SelIdx < NUM_SETTINGS)
                bOptSelected = TRUE;
        }
        break;

        default:
            break;
        }

        if (PrevSelIdx != SelIdx)
        {
            HidePointer();
            // Clear selection for old index
            if (PrevSelIdx >= 0 && PrevSelIdx < NUM_SETTINGS)
                DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + PrevSelIdx), GetStringById(STRING_TOKEN(SETTINGS_STR_ID[PrevSelIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, FALSE, SettingsRect.Width);

            // Set selection for new index
            if (SelIdx >= 0 && SelIdx < NUM_SETTINGS)
                DisplayMenuOption(LeftOffset, LINENO_YPOS(4 + SelIdx), GetStringById(STRING_TOKEN(SETTINGS_STR_ID[SelIdx]), TempBuf, sizeof(TempBuf)), &DEFAULT_TEXT_PIXEL, TRUE, SettingsRect.Width);

            PrevSelIdx = SelIdx;
            DrawPointer();
        }

        if (bOptSelected)
        {
            HidePointer();

            ClearTextXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(12), NULL, ALIGN_CENTRE, ErrorMsg);
            ErrorMsg[0] = L'\0';
            if (SelIdx >= 0 && SelIdx < NUM_SETTINGS)
            {
                switch (SelIdx)
                {
                case SETTINGS_OPTION_IDX_LANG: // Language
                {
                    // Setting selected, display list of suboptions for selected setting

                    int SubIdx;
                    int NumOpts = 0;
                    UINTN MaxWidth = 0;

                    for (i = 0; SETTINGS_LANG_OPTIONS_STR_ID[i] != (EFI_STRING_ID)-1; i++)
                    {
                        UINTN StringWidth = 0;
                        GetTextInfo(GetStringById(SETTINGS_LANG_OPTIONS_STR_ID[i], TempBuf, sizeof(TempBuf)), &StringWidth, NULL);

                        if (StringWidth > MaxWidth)
                            MaxWidth = StringWidth;
                    }

                    XPos = LeftOffset + SettingsRect.Width + MaxWidth + 5;

                    while (SETTINGS_LANG_OPTIONS_STR_ID[NumOpts] != (EFI_STRING_ID)-1 && mLangFontSupported[NumOpts]) // Display only the languages that are supported by the font
                        NumOpts++;

                    CHAR16 **LangOptions = AllocateStringTable(SETTINGS_LANG_OPTIONS_STR_ID, NumOpts);

                    SubIdx = getoption(XPos, LINENO_YPOS(4 + SelIdx), LangOptions, NumOpts);

                    FreeStringTable(LangOptions, NumOpts);

                    if (SubIdx >= 0) // User selected a new suboption
                    {

                        if (SelIdx == 0) // Language
                        {
                            if (mLangFontSupported[SubIdx] == FALSE) // If language is unsupported by font, display error
                            {
                                PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(12), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_LANG_UNSUPPORTED), ErrorMsg, sizeof(ErrorMsg)));
                            }
                            else // New language selected, redraw screen
                            {
                                gCurLang = SubIdx;
                                DrawWindow();
                                return ID_BUTTON_SETTINGS;
                            }
                        }
                    }
                    // Set selection for new index
                    PrintXYAlign(LeftOffset + SettingsRect.Width, LINENO_YPOS(4 + SelIdx), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STRING_TOKEN(SETTINGS_LANG_OPTIONS_STR_ID[gCurLang]), TempBuf, sizeof(TempBuf)));
                }
                break;

                case SETTINGS_OPTION_IDX_RES: // Resolution
                {
                    // Setting selected, display list of suboptions for selected setting
                    UINT32 MaxGraphicsMode = egGetNumModes();

                    int SubIdx;
                    UINTN MaxWidth = 0;

                    CHAR16 **ResOptions = AllocateZeroPool(MaxGraphicsMode * sizeof(CHAR16 *));
                    for (i = 0; i < (int)MaxGraphicsMode; i++)
                    {
                        UINTN StringWidth = 0;
                        UINTN Width = (UINTN)i;
                        UINTN Height = 0;
                        egGetResFromMode(&Width, &Height);

                        UnicodeSPrint(TempBuf, sizeof(TempBuf), L"%d x %d", Width, Height);
                        ResOptions[i] = AllocateZeroPool((StrLen(TempBuf) + 1) * sizeof(CHAR16));
                        StrCpyS(ResOptions[i], StrLen(TempBuf) + 1, TempBuf);

                        GetTextInfo(TempBuf, &StringWidth, NULL);

                        if (StringWidth > MaxWidth)
                            MaxWidth = StringWidth;
                    }

                    XPos = LeftOffset + SettingsRect.Width + MaxWidth + 5;

                    SubIdx = getoption(XPos, LINENO_YPOS(4 + SelIdx), ResOptions, MaxGraphicsMode);

                    FreeStringTable(ResOptions, MaxGraphicsMode);

                    if (SubIdx >= 0) // User selected a new suboption
                    {
                        if (ScreenMode != (UINTN)SubIdx)
                        {
                            SetScreenRes((UINTN)SubIdx);
                            KillMouse();
                            MouseBirth();
                            DrawWindow();
                            return ID_BUTTON_SETTINGS;
                        }
                    }
                }
                break;
#ifdef PRO_RELEASE
                case SETTINGS_OPTION_IDX_SAVECFG: // Save config to file
                {
                    EnableGraphicsMode(FALSE);

                    EFI_INPUT_KEY key;
                    SetMem(&key, sizeof(key), 0);
                    MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                                  L"**WARNING** This will overwrite the following config file:        ",
                                  L"                                                                  ",
                                  gConfigFile,
                                  L"                                                                  ",
                                  L" Press (o) to overwrite. Otherwise, press any other key to cancel.",
                                  NULL);

                    gST->ConOut->ClearScreen(gST->ConOut);

                    if (key.UnicodeChar == 'o')
                    {
                        MT_HANDLE FileHandle = NULL;
                        EFI_STATUS Status;

                        Status = MtSupportOpenFile(&FileHandle, gConfigFile, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                        if (FileHandle == NULL)
                        {
                            SetMem(&key, sizeof(key), 0);
                            MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                                          GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), ErrorMsg, sizeof(ErrorMsg)),
                                          NULL);
                        }
                        else
                        {
                            Status = MtSupportDeleteFile(FileHandle); // Delete closes the handle
                            if (Status != EFI_SUCCESS)
                            {
                                AsciiFPrint(DEBUG_FILE_HANDLE, "Error deleting config file (%r)", Status);
                            }

                            FileHandle = NULL;
                            Status = MtSupportOpenFile(&FileHandle, gConfigFile, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
                            if (FileHandle != NULL)
                            {
                                MtSupportWriteConfig(FileHandle);

                                MtSupportCloseFile(FileHandle);

                                UnicodeSPrint(ErrorMsg, sizeof(ErrorMsg), GetStringById(STRING_TOKEN(STR_MENU_CONFIG_SAVED), TempBuf, sizeof(TempBuf)), gConfigFile);

                                SetMem(&key, sizeof(key), 0);
                                MtCreatePopUp(gST->ConOut->Mode->Attribute, &key,
                                              ErrorMsg,
                                              NULL);
                            }
                            else
                            {
                                AsciiFPrint(DEBUG_FILE_HANDLE, "Error creating config file (%r)", Status);
                            }
                        }
                    }

                    EnableGraphicsMode(TRUE);

                    DrawWindow();
                    return ID_BUTTON_SETTINGS;
                }
                break;

                case SETTINGS_OPTION_IDX_SAVELOC: // Change save location
                {
                    EnableGraphicsMode(FALSE);

                    change_save_location();

                    EnableGraphicsMode(TRUE);

                    DrawWindow();
                    return ID_BUTTON_SETTINGS;
                }
                break;
#endif
                }
            }

            DrawPointer();
        }
    }
}

#ifndef PRO_RELEASE
UINT16 UpgradeProScreen(IN UINTN XPos, IN UINTN YPos)
{
    int i;
    const UINTN UPGRADE_FRAME_MARGIN = 100;
    const UINTN UPGRADE_FRAME_TOP_OFF = 0;

    UINTN LeftOffset = XPos + UPGRADE_FRAME_MARGIN;
    UINTN TopOffset = YPos + UPGRADE_FRAME_TOP_OFF;

    int LineNo = 0;
    MOUSE_EVENT mouseEv;
    CHAR16 TempBuf[128];
    CHAR16 ErrorMsg[128];
    HidePointer();

    const CHAR16 *PRO_FEATURES[] = {
        L"DDR / EPP / XMP SPD details",
        L"Customizable Reports",
        L"ECC error injection (Some CPU models)",
        L"Disable CPU cache option",
        L"Configuration file",
        L"Full test automation (via configuration file)",
        L"New 64-bit / 128-bit SIMD tests",
        L"Saving RAM benchmark reports to disk",
        L"Network (PXE) Boot",
        L"Phone, E-mail, Forum Support, plus free upgrades for 12 months after purchase"};

    // Print instructions at top
    PrintXYAlign(LeftOffset + UPGRADE_FRAME_MARGIN, LINENO_YPOS(LineNo), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"Additional Pro Features");

    LineNo += 2;

    for (i = 0; i < sizeof(PRO_FEATURES) / sizeof(PRO_FEATURES[0]); i++)
    {
        egDrawImage(OKBullet, &BG_COLOUR, LeftOffset - 15 - OKBullet->Width, LINENO_YPOS(LineNo));

        PrintXYAlign(LeftOffset, LINENO_YPOS(LineNo), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, PRO_FEATURES[i]);
        LineNo += 2;
    }

    PrintXYAlign(LeftOffset + UPGRADE_FRAME_MARGIN, LINENO_YPOS(LineNo), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, GetStringById(STR_UPGRADE_PRO, TempBuf, sizeof(TempBuf)));

    egDrawImage(QRCode, &BG_COLOUR, WindowWidth - UPGRADE_FRAME_MARGIN - QRCode->Width, LINENO_YPOS(LineNo));

    DrawPointer();

    SetMem(ErrorMsg, sizeof(ErrorMsg), 0);
    while (TRUE)
    {
        UINTN curX, curY;
        EFI_INPUT_KEY Key;
        SetMem(&Key, sizeof(Key), 0);

        mouseEv = NoEvents;

        WaitForKeyboardMouseInput();

        if (gST->ConIn->ReadKeyStroke(gST->ConIn, &Key) == EFI_SUCCESS)
        {
            int Ret;

            Ret = KeyPressSideBar(&Key);
            if (Ret >= 0)
                return (UINT16)Ret;
        }
        mouseEv = WaitForMouseEvent(&curX, &curY, 10);

        switch (mouseEv)
        {
        case MouseMove:
        {
            MouseMoveSideBar();
            HidePointer();
            DrawPointer();
        }
        break;

        case DoubleClick:
        case LeftClick:
        {
            int Ret;

            Ret = MouseClickSideBar();
            if (Ret >= 0)
                return (UINT16)Ret;
        }
        break;

        default:
            break;
        }
    }
}
#endif

VOID InstallSelectedTests()
{
    int i;
    for (i = 0; i < gNumCustomTests; i++)
    {
        if (gCustomTestList[i].Enabled)
        {
            if (gCustomTestList[i].RunMemTest2 == NULL)
                MtSupportInstallMemoryRangeTest(
                    gCustomTestList[i].TestNo,
                    gCustomTestList[i].NameStr,
                    gCustomTestList[i].NameStrId,
                    gCustomTestList[i].RunMemTest1,
                    gCustomTestList[i].MaxCPUs,
                    gCustomTestList[i].Iterations,
                    gCustomTestList[i].Align,
                    gCustomTestList[i].Pattern,
                    gCustomTestList[i].PatternUser,
                    gCustomTestList[i].CacheMode,
                    gCustomTestList[i].Context);
#ifndef TRIAL
            else if (gCustomTestList[i].RunMemTest3 == NULL)
                MtSupportInstallBitFadeTest(
                    gCustomTestList[i].TestNo,
                    gCustomTestList[i].NameStr,
                    gCustomTestList[i].NameStrId,
                    gCustomTestList[i].RunMemTest1,
                    gCustomTestList[i].RunMemTest2,
                    gCustomTestList[i].MaxCPUs,
                    gCustomTestList[i].Iterations,
                    gCustomTestList[i].Align,
                    gCustomTestList[i].Pattern,
                    gCustomTestList[i].PatternUser,
                    gCustomTestList[i].CacheMode,
                    gCustomTestList[i].Context);
            else
            {
                MtSupportInstallHammerTest(
                    gCustomTestList[i].TestNo,
                    gCustomTestList[i].NameStr,
                    gCustomTestList[i].NameStrId,
                    gCustomTestList[i].RunMemTest1,
                    gCustomTestList[i].RunMemTest2,
                    gCustomTestList[i].RunMemTest3,
                    gCustomTestList[i].RunMemTest4,
                    gCustomTestList[i].MaxCPUs,
                    gCustomTestList[i].Iterations,
                    gCustomTestList[i].Align,
                    gCustomTestList[i].Pattern,
                    gCustomTestList[i].PatternUser,
                    gCustomTestList[i].CacheMode,
                    gCustomTestList[i].Context);
            }
#endif
        }
    }
}

VOID InstallRowHammerTests() // Not used
{
#ifndef TRIAL
    // Original
    MtSupportInstallHammerTest(
        0,
        NULL,
        STRING_TOKEN(STR_TEST_13_NAME),
        (TEST_MEM_RANGE)RunHammerTest_write,
        (TEST_MEM_RANGE)RunHammerTest_hammer_fast,
        (TEST_MEM_RANGE)RunHammerTest_hammer_slow,
        (TEST_MEM_RANGE)RunHammerTest_verify,
        MAX_CPUS,
        1,
        16,
        PAT_DEF,
        0,
        CACHEMODE_DEFAULT,
        NULL);

    // Double-sided
    MtSupportInstallHammerTest(
        1,
        NULL,
        STRING_TOKEN(STR_TEST_13_DBL_NAME),
        (TEST_MEM_RANGE)RunHammerTest_write,
        (TEST_MEM_RANGE)RunHammerTest_double_sided_hammer_fast,
        (TEST_MEM_RANGE)RunHammerTest_double_sided_hammer_slow,
        (TEST_MEM_RANGE)RunHammerTest_verify,
        MAX_CPUS,
        1,
        16,
        PAT_DEF,
        0,
        CACHEMODE_DEFAULT,
        NULL);

    // Random pattern
    MtSupportInstallHammerTest(
        2,
        NULL,
        STRING_TOKEN(STR_TEST_13_RND_NAME),
        (TEST_MEM_RANGE)RunHammerTest_write,
        (TEST_MEM_RANGE)RunHammerTest_hammer_fast,
        (TEST_MEM_RANGE)RunHammerTest_hammer_slow,
        (TEST_MEM_RANGE)RunHammerTest_verify,
        MAX_CPUS,
        1,
        16,
        PAT_DEF,
        0,
        CACHEMODE_DEFAULT,
        (void *)(UINTN)0x50415353);

    // Multi-threaded hammering
    MtSupportInstallHammerTest_multithreaded(
        3,
        NULL,
        STRING_TOKEN(STR_TEST_13_MT_NAME),
        (TEST_MEM_RANGE)RunHammerTest_write,
        (TEST_MEM_RANGE)RunHammerTest_hammer_fast,
        (TEST_MEM_RANGE)RunHammerTest_hammer_slow,
        (TEST_MEM_RANGE)RunHammerTest_verify,
        MAX_CPUS,
        1,
        16,
        PAT_DEF,
        0,
        CACHEMODE_DEFAULT,
        NULL);
#endif
}

BOOL RunAdvancedMemoryTest(IN UINTN XPos, IN UINTN YPos, BENCH_CONFIG *Config)
{
    int i;
    UINTN LeftOffset = XPos + MAIN_FRAME_LEFT_OFF;
    UINTN TopOffset = YPos + MAIN_FRAME_TOP_OFF;
    int nNumSteps = 0;
    CHAR16 szProgressLabel[256];
    CHAR16 TempBuf[128];

    SetReadWrite(Config->ReadWrite == MEM_READ);
    SetTestMode(Config->TestMode);
    SetDataSize(Config->DataWidth);

    // Reset the progress
    ClearTextXYAlign(LeftOffset + 20, LINENO_YPOS(17), NULL, ALIGN_LEFT, L"%s: ", GetStringById(STRING_TOKEN(STR_MENU_BENCH_PROGRESS), TempBuf, sizeof(TempBuf)));
    if (MEM_STEP == Config->TestMode)
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Running MEM_STEP test (%s)", Config->ReadWrite == MEM_READ ? L"Read" : L"Write");

        // Initialise the the test
        nNumSteps = NewStepTest();

        AsciiFPrint(DEBUG_FILE_HANDLE, "Number of memory steps: %d (Block size: %ld Bytes)", nNumSteps, (UINT64)GetArraySize());

        // Memory allocation failed
        if (nNumSteps == 0)
        {
            PrintXYAlign(LeftOffset + 20 + PROG_BAR_WIDTH / 2, LINENO_YPOS(17), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_ALLOC), TempBuf, sizeof(TempBuf)));
            return FALSE;
        }

        PrintXYAlign(LeftOffset + 50 + gCharWidth * 25, LINENO_YPOS(14), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%ld Bytes", (UINT64)GetArraySize());

        // Initialise the progress bar
        DrawProgressBar(&PROGBAR_COLOUR_PIXEL, (EG_PIXEL *)&DEFAULT_TEXT_PIXEL, LeftOffset + 20, LINENO_YPOS(18), PROG_BAR_WIDTH, LINE_SPACING, 0);

        // Run the main test loop
        for (i = 0; i < nNumSteps; i++)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Step %d of %d: Step size %ld bytes", i + 1, nNumSteps, (UINT64)GetCurrentStepSize());

            // Update UI
            UnicodeSPrint(szProgressLabel, sizeof(szProgressLabel), GetStringById(STRING_TOKEN(STR_BENCH_PROGRESS_STEP), TempBuf, sizeof(TempBuf)), i + 1, nNumSteps, (UINT64)GetCurrentStepSize());

            PrintXYAlign(LeftOffset + 20 + PROG_BAR_WIDTH / 2, LINENO_YPOS(17), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, szProgressLabel);

            DrawProgressBar(&PROGBAR_COLOUR_PIXEL, (EG_PIXEL *)&DEFAULT_TEXT_PIXEL, LeftOffset + 20, LINENO_YPOS(18), PROG_BAR_WIDTH, LINE_SPACING, i * 100 / nNumSteps);

            if (!Step())
            {
                SetTestState(ERR_TEST);
                break;
            }

            if (gBS->CheckEvent(gST->ConIn->WaitForKey) == EFI_SUCCESS) // Check for abort
            {
                EFI_INPUT_KEY Key;
                SetMem(&Key, sizeof(Key), 0);

                gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);

                if (Key.ScanCode == 0x17 /* Esc */)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "MEM_STEP test cancelled by user");
                    SetTestState(USER_STOPPED);
                    break;
                }
            }
        }

        SetAveSpeed(DivU64x32(GetTotalKBPerSec(), nNumSteps));
        CleanupTest();
    }
    else
    {
        AsciiFPrint(DEBUG_FILE_HANDLE, "Running MEM_BLOCK test (%s, %d-bit)", Config->ReadWrite == MEM_READ ? L"Read" : L"Write", Config->DataWidth);

        // Find out how many steps
        nNumSteps = NewBlockTest();

        AsciiFPrint(DEBUG_FILE_HANDLE, "Number of blocks: %d", nNumSteps);

        // Initialise the progress bar
        DrawProgressBar(&PROGBAR_COLOUR_PIXEL, (EG_PIXEL *)&DEFAULT_TEXT_PIXEL, LeftOffset + 20, LINENO_YPOS(18), PROG_BAR_WIDTH, LINE_SPACING, 0);

        for (i = 0; i < nNumSteps; i++)
        {
            AsciiFPrint(DEBUG_FILE_HANDLE, "Step %d of %d: Block Size: %ld, Data Size %ld Bits", i + 1, nNumSteps, (UINT64)GetBlockSize(), (UINT64)GetDataSize());

            ClearTextXYAlign(LeftOffset + 50 + gCharWidth * 25, LINENO_YPOS(14), NULL, ALIGN_LEFT, L"% 16ld Bytes", (UINT64)GetBlockSize());
            PrintXYAlign(LeftOffset + 50 + gCharWidth * 25, LINENO_YPOS(14), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_LEFT, L"%ld Bytes", (UINT64)GetBlockSize());

            // Update UI
            UnicodeSPrint(szProgressLabel, sizeof(szProgressLabel), GetStringById(STRING_TOKEN(STR_BENCH_PROGRESS_BLOCK), TempBuf, sizeof(TempBuf)), i + 1, nNumSteps, (UINT64)GetDataSize());

            PrintXYAlign(LeftOffset + 20 + PROG_BAR_WIDTH / 2, LINENO_YPOS(17), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, szProgressLabel);

            DrawProgressBar(&PROGBAR_COLOUR_PIXEL, (EG_PIXEL *)&DEFAULT_TEXT_PIXEL, LeftOffset + 20, LINENO_YPOS(18), PROG_BAR_WIDTH, LINE_SPACING, i * 100 / nNumSteps);

            if (!StepBlock())
            {
                SetTestState(ERR_TEST);
                break;
            }

            if (gBS->CheckEvent(gST->ConIn->WaitForKey) == EFI_SUCCESS)
            {
                EFI_INPUT_KEY Key;
                SetMem(&Key, sizeof(Key), 0);

                gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);

                if (Key.ScanCode == 0x17 /* Esc */)
                {
                    AsciiFPrint(DEBUG_FILE_HANDLE, "MEM_BLOCK test cancelled by user");
                    SetTestState(USER_STOPPED);
                    break;
                }
            }
        }
        SetAveSpeed(DivU64x32(GetTotalKBPerSec(), nNumSteps));

        // Did the test finish successfully?
        if (i == nNumSteps)
        {
            SetTestState(COMPLETE);
            SetResultsAvailable(TRUE);
        }
    }

    // Erase progress text
    ClearTextXYAlign(LeftOffset + 20 + PROG_BAR_WIDTH / 2, LINENO_YPOS(17), NULL, ALIGN_CENTRE, szProgressLabel);

    // Examine the result and take the appropriate action
    switch (GetTestState())
    {
    case NOT_STARTED:
    case RUNNING:
        // Display unknown error message
        PrintXYAlign(LeftOffset + 20 + PROG_BAR_WIDTH / 2, LINENO_YPOS(17), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_UNKNOWN), TempBuf, sizeof(TempBuf)));
        break;

    case COMPLETE:
    {
        EFI_FILE_HANDLE DirHandle = NULL;
        EFI_FILE_HANDLE FileHandle = NULL;
        MT_HANDLE DirMTHandle = NULL;
        EFI_STATUS Status = EFI_NOT_FOUND;

        EFI_TIME Time;
        CHAR16 Filename[128];
        CHAR16 Prefix[128];

        // Test has completed successfully
        PrintXYAlign(LeftOffset + 20 + PROG_BAR_WIDTH / 2, LINENO_YPOS(17), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_BENCH_TEST_COMPLETE), TempBuf, sizeof(TempBuf)));

        DrawProgressBar(&PROGBAR_COLOUR_PIXEL, (EG_PIXEL *)&DEFAULT_TEXT_PIXEL, LeftOffset + 20, LINENO_YPOS(18), PROG_BAR_WIDTH, LINE_SPACING, 100);
        // Write the stats to disk

        gRT->GetTime(&Time, NULL);

        Prefix[0] = L'\0';
        MtSupportGetReportPrefix(Prefix, sizeof(Prefix));

        // Create benchmark directory, if not exist
        Status = MtSupportOpenFile(&DirMTHandle, BENCH_DIR, EFI_FILE_MODE_READ, 0);
        if (DirMTHandle == NULL)
        {
            Status = MtSupportOpenFile(&DirMTHandle, BENCH_DIR, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, EFI_FILE_DIRECTORY);
            if (Status == EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Created directory \"%s\"", BENCH_DIR);
            }
        }
        if (DirMTHandle != NULL)
        {
            MtSupportGetEfiFileHandle(DirMTHandle, &DirHandle);
        }

        UnicodeSPrint(Filename, sizeof(Filename), L"%s%s-%s.ptx", Prefix, BENCH_BASE_FILENAME, MtSupportGetTimestampStr(&Time));

        AsciiFPrint(DEBUG_FILE_HANDLE, "Test has completed successfully. Saving results to %s", Filename);
        if (DirHandle)
        {
            Status = DirHandle->Open(DirHandle, &FileHandle, Filename, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
            if (Status != EFI_SUCCESS)
            {
                AsciiFPrint(DEBUG_FILE_HANDLE, "Unable to open %s for writing (%r)", Filename, Status);
            }
        }

        if (FileHandle == NULL)
        {
            PrintXYAlign((LeftOffset + WindowWidth) / 2, LINENO_YPOS(25), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_ERROR_FILE_OPEN), TempBuf, sizeof(TempBuf)));
        }
        else
        {
            // Save the benchmark results to disk
            MT_HANDLE FileMTHandle = NULL;
            TestResultsMetaInfo TestInfo;
            MEM_RESULTS Header;
            UINT64 StepSizeValues[32];
            UINT64 KBPerSecValues[32];

            // TestType
            enum
            {
                MAIN_ALL,
                ADVANCED_DISK_READ,
                ADVANCED_DISK_WRITE,
                ADVANCED_DISK_SUPER,
                ADVANCED_CD_READ,
                ADVANCED_CD_BURN,
                ADVANCED_MEMORY_STEP,
                ADVANCED_MEMORY_BLOCK,
                ADVANCED_NETWORK_SERVER,
                ADVANCED_NETWORK_CLIENT,
                ADVANCED_MULTIPLE_PROCESS,

            };

            SetMem(&TestInfo, sizeof(TestInfo), 0);
            SetMem(&Header, sizeof(Header), 0);

            // Test metadata
            TestInfo.iTestType = Config->TestMode == MEM_STEP ? ADVANCED_MEMORY_STEP : ADVANCED_MEMORY_BLOCK;
            TestInfo.i64TestStartTime = MultU64x32(10000000000ul, Config->StartTime.Year);
            TestInfo.i64TestStartTime += MultU64x32(100000000, Config->StartTime.Month);
            TestInfo.i64TestStartTime += MultU64x32(1000000, Config->StartTime.Day);
            TestInfo.i64TestStartTime += MultU64x32(10000, Config->StartTime.Hour);
            TestInfo.i64TestStartTime += MultU64x32(100, Config->StartTime.Minute);
            TestInfo.i64TestStartTime += Config->StartTime.Second;

            TestInfo.uiNumCPU = (UINT32)MPSupportGetNumProcessors();
            cpu_type(TestInfo.szCPUType, sizeof(TestInfo.szCPUType));

            getCPUSpeedLine(cpu_msr.raw_freq_cpu, cpu_msr.flCPUSpeedTurbo, TestInfo.szCPUSpeed, sizeof(TestInfo.szCPUSpeed));

            Header.TotalPhys = memsize;
            Header.iTestMode = Config->TestMode;
            Header.fAveSpeed = GetAveSpeed();
            if (g_numMemModules > 0) // Save SPD information if available
            {
                CHAR16 szwRAMType[32];
                CHAR16 szwXferSpeed[32];
                CHAR16 szwMaxBandwidth[32];
                CHAR16 szwVoltage[32];

                BOOLEAN ECCEn = FALSE;

                StrCpyS(Header.szModuleManuf, sizeof(Header.szModuleManuf) / sizeof(Header.szModuleManuf[0]), g_MemoryInfo[0].jedecManuf);
                AsciiStrToUnicodeStrS(g_MemoryInfo[0].modulePartNo, Header.szModulePartNo, ARRAY_SIZE(Header.szModulePartNo));

                getMaxSPDInfoStr(&g_MemoryInfo[0], szwRAMType, szwXferSpeed, szwMaxBandwidth, Header.szRAMTiming, szwVoltage, sizeof(szwRAMType));
                ECCEn = (g_MemoryInfo[0].ecc != FALSE) && MtSupportECCEnabled();
                UnicodeSPrint(Header.szRAMType, sizeof(Header.szRAMType), L"%s %s %s%sMT/s", szwMaxBandwidth, szwRAMType, ECCEn ? L"ECC " : L"", szwXferSpeed);
            }
            else if (g_numSMBIOSModules > 0) // Otherwise save RAM info from SMBIOS
            {
                CHAR16 szwRAMType[32];
                CHAR16 szwXferSpeed[32];
                CHAR16 szwMaxBandwidth[32];

                for (i = 0; i < g_numSMBIOSMem; i++)
                {
                    if (g_SMBIOSMemDevices[i].smb17.Size != 0)
                    {
                        AsciiStrToUnicodeStrS(g_SMBIOSMemDevices[i].szManufacturer, Header.szModuleManuf, ARRAY_SIZE(Header.szModuleManuf));
                        AsciiStrToUnicodeStrS(g_SMBIOSMemDevices[i].szPartNumber, Header.szModulePartNo, ARRAY_SIZE(Header.szModulePartNo));

                        getSMBIOSMemoryType(&g_SMBIOSMemDevices[i], szwRAMType, szwXferSpeed, szwMaxBandwidth, sizeof(szwRAMType));
                        UnicodeSPrint(Header.szRAMType, sizeof(Header.szRAMType), L"%s %s %s%sMT/s", szwMaxBandwidth, szwRAMType, MtSupportECCEnabled() ? L"ECC " : L"", szwXferSpeed);
                        break;
                    }
                }
            }

            Header.bRead = Config->ReadWrite == MEM_READ;

            // Step size data
            Header.ArraySize = Config->TestMode == MEM_STEP ? GetArraySize() : 0;
            // Block size data
            Header.MinBlock = Config->TestMode == MEM_BLOCK ? MIN_BLOCK_SIZE : 0;
            Header.MaxBlock = Config->TestMode == MEM_BLOCK ? GetMaxBlockSize() : 0;
            Header.dwAccessDataSize = Config->TestMode == MEM_BLOCK ? GetDataSize() : 0;

            for (i = 0; i < (int)GetNumSamples(); i++)
            {
                MEMORY_STATS Sample;
                GetSample(i, &Sample);
                StepSizeValues[i] = Sample.StepSize;
                KBPerSecValues[i] = Sample.dKBPerSecond;
            }

            MtSupportEfiFileHandleToMtHandle(FileHandle, &FileMTHandle);

            // if (SaveBenchResults(FileMTHandle, &TestInfo, &Header, StepSizeValues, KBPerSecValues, (int)GetNumSamples()))
            //     AsciiFPrint(DEBUG_FILE_HANDLE, "Successfully saved benchmark results");
            // else
            //     AsciiFPrint(DEBUG_FILE_HANDLE, "Error saving benchmark results");

            MtSupportCloseFile(FileMTHandle);
            MtSupportCloseFile(DirMTHandle);
        }
        return TRUE;
    }
    break;

    case USER_STOPPED:
        // Cancelled by user
        PrintXYAlign(LeftOffset + 20 + PROG_BAR_WIDTH / 2, LINENO_YPOS(17), &DEFAULT_TEXT_PIXEL, NULL, ALIGN_CENTRE, GetStringById(STRING_TOKEN(STR_BENCH_TEST_STOPPED), TempBuf, sizeof(TempBuf)));

        DrawProgressBar(&PROGBAR_COLOUR_PIXEL, (EG_PIXEL *)&DEFAULT_TEXT_PIXEL, LeftOffset + 20, LINENO_YPOS(18), PROG_BAR_WIDTH, LINE_SPACING, 0);
        break;

    case ERR_TEST:
        break;
    default:
        break;
    }

    return FALSE;
}

VOID SetupBenchLineChart(int iTestMode)
{
    CHAR16 TempBuf[128];

    // Init graph
    LineChart_Destroy();
    LineChart_Init();

    LineChart_SetBackgroundColour(&BG_COLOUR, NULL, FALSE);

    LineChart_SetAltBGOverlay(FALSE);
    LineChart_ShowGrid(TRUE);
    LineChart_SetGridThickness(1);

    if (iTestMode == MEM_STEP)
    {
        LineChart_SetChartTitle(GetStringById(STRING_TOKEN(STR_BENCH_CHART_TITLE_STEP), TempBuf, sizeof(TempBuf)));
        LineChart_SetXAxisText(GetStringById(STRING_TOKEN(STR_STEP_SIZE), TempBuf, sizeof(TempBuf)));
    }
    else
    {
        LineChart_SetChartTitle(GetStringById(STRING_TOKEN(STR_BENCH_CHART_TITLE_BLOCK), TempBuf, sizeof(TempBuf)));
        LineChart_SetXAxisText(GetStringById(STRING_TOKEN(STR_BLOCK_SIZE), TempBuf, sizeof(TempBuf)));
    }

    LineChart_SetDiscrete();
}

void LoadCurBenchResultsToChart(int iSeries, BOOLEAN bAve)
{
    UINT64 flMBPerSec = 0;
    UINT64 flAveMBPerSec = 0;
    UINT64 flTotalMBPerSec = 0;

    UINT64 StepSizeTemp = 0;
    UINTN StepSize = 0;
    CHAR16 szTextBuf[64];
    CHAR16 szAccessSize[16];
    CHAR16 TempBuf[128];

    int i;
    for (i = 0; i < (int)GetNumSamples(); i++)
    {
        MEMORY_STATS Sample;
        GetSample(i, &Sample);

        // Get stat in MB/s
        flMBPerSec = DivU64x32(Sample.dKBPerSecond, 1024);

        // Calculate ave MB/s
        flTotalMBPerSec += flMBPerSec;
        flAveMBPerSec = DivU64x32(flTotalMBPerSec, i + 1);

        // Step size
        StepSizeTemp = Sample.StepSize;
        StepSize = (UINTN)StepSizeTemp;

        bAve ? LineChart_SetCatValue(iSeries, i, flAveMBPerSec) : LineChart_SetCatValue(iSeries, i, flMBPerSec);

        if (GetTestMode() == MEM_STEP)
        {
            if (StepSize >= 1024)
                UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ldKB", (UINT64)StepSize / 1024);
            else
                UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ld", (UINT64)StepSize);
        }
        else
        {
            if (StepSize >= 1048576)
                UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ldMB", (UINT64)StepSize / 1048576);
            else
                UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ldKB", (UINT64)StepSize / 1024);
        }
        LineChart_SetCatTitle(i, szTextBuf);
    }
    LineChart_SetNumCat(iSeries, (int)GetNumSamples());

    // Print title
    UnicodeSPrint(szAccessSize, sizeof(szAccessSize), L", %dbit", GetDataSize());

    UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%s%s", GetReadWrite() ? GetStringById(STRING_TOKEN(STR_READ), TempBuf, sizeof(TempBuf)) : GetStringById(STRING_TOKEN(STR_WRITE), TempBuf, sizeof(TempBuf)), (GetTestMode() == MEM_BLOCK) ? szAccessSize : L"");
    LineChart_SetSerTitle(iSeries, szTextBuf);

    // Print series label
    if (g_numMemModules > 0) // Use SPD info if available
    {
        CHAR16 szwRAMSummary[65];
        CHAR16 *pszRAMTypeTail;

        getSPDSummaryLine(&g_MemoryInfo[0], szwRAMSummary, sizeof(szwRAMSummary));
        if ((pszRAMTypeTail = StrStr(szwRAMSummary, L"/")) != NULL)
        {
            SetMem(szTextBuf, sizeof(szTextBuf), 0);
            StrnCpyS(szTextBuf, ARRAY_SIZE(szTextBuf), szwRAMSummary, (UINTN)(pszRAMTypeTail - szwRAMSummary));
            LineChart_SetSerSubtitle(iSeries, szTextBuf);
        }
    }
    else if (g_numSMBIOSModules > 0) // Use SMBIOS info if available
    {
        CHAR16 szwRAMSummary[65];
        CHAR16 *pszRAMTypeTail;

        for (i = 0; i < g_numSMBIOSMem; i++)
        {
            if (g_SMBIOSMemDevices[i].smb17.Size != 0)
            {
                getSMBIOSRAMSummaryLine(&g_SMBIOSMemDevices[i], szwRAMSummary, sizeof(szwRAMSummary));
                if ((pszRAMTypeTail = StrStr(szwRAMSummary, L"/")) != NULL)
                {
                    SetMem(szTextBuf, sizeof(szTextBuf), 0);
                    StrnCpyS(szTextBuf, ARRAY_SIZE(szTextBuf), szwRAMSummary, (UINTN)(pszRAMTypeTail - szwRAMSummary));
                    LineChart_SetSerSubtitle(iSeries, szTextBuf);
                }
                break;
            }
        }
    }
    else // Otherwise, just use the RAM size
    {
        CHAR16 szRAMSize[32];
        UnicodeSPrint(szRAMSize, sizeof(szRAMSize), L"%ldMB", MtSupportMemSizeMB());
        LineChart_SetSerSubtitle(iSeries, szRAMSize);
    }

    if (bAve)
    {
        LineChart_ShowDataPoints(iSeries, FALSE);
    }
    else
    {

        LineChart_ShowDataPoints(iSeries, TRUE);
    }
}

BOOLEAN LoadBenchResultsToChart(MT_HANDLE FileHandle, int iSeries, BOOLEAN bAve)
{
#if 0
    TestResultsMetaInfo TestInfo;
    MTVersion VersionInfo;
    MEM_RESULTS Header;

    UINT64 flMBPerSec = 0;
    UINT64 flAveMBPerSec = 0;
    UINT64 flTotalMBPerSec = 0;

    UINT64 StepSizeValues[32];
    UINT64 KBPerSecValues[32];
    int NumSamples = 32;

    UINTN StepSize = 0;
    CHAR16 szTextBuf[64];
    CHAR16 szAccessSize[16];
    CHAR16 TempBuf[128];

    int k;

    if (LoadBenchResults(FileHandle, &TestInfo, &VersionInfo, &Header, StepSizeValues, KBPerSecValues, &NumSamples))
    {
        for (k = 0; k < NumSamples; k++)
        {
            // get MB/s
            flMBPerSec = DivU64x32(KBPerSecValues[k], 1024);

            // Calculate average MB/s
            flTotalMBPerSec += flMBPerSec;
            flAveMBPerSec = DivU64x32(flTotalMBPerSec, k + 1);

            StepSize = (UINTN)StepSizeValues[k];

            bAve ? LineChart_SetCatValue(iSeries, k, flAveMBPerSec) : LineChart_SetCatValue(iSeries, k, flMBPerSec);

            if (Header.iTestMode == MEM_STEP)
            {
                if (StepSize >= 1024)
                    UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ldKB", (UINT64)StepSize / 1024);
                else
                    UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ld", (UINT64)StepSize);
            }
            else
            {
                if (StepSize >= 1048576)
                    UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ldMB", (UINT64)StepSize / 1048576);
                else
                    UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ldKB", (UINT64)StepSize / 1024);
            }
            LineChart_SetCatTitle(k, szTextBuf);
        }

        LineChart_SetNumCat(iSeries, (int)NumSamples);

        // Print chart title
        UnicodeSPrint(szAccessSize, sizeof(szAccessSize), L", %dbit", Header.dwAccessDataSize);

        UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%s%s", Header.bRead ? GetStringById(STRING_TOKEN(STR_READ), TempBuf, sizeof(TempBuf)) : GetStringById(STRING_TOKEN(STR_WRITE), TempBuf, sizeof(TempBuf)), (Header.iTestMode == MEM_BLOCK) ? szAccessSize : L"");
        LineChart_SetSerTitle(iSeries, szTextBuf);

        // Get the series label from the results header
        if (Header.szRAMType[0] != L'\0')
            LineChart_SetSerSubtitle(iSeries, Header.szRAMType);
        else
        {
            UnicodeSPrint(szTextBuf, sizeof(szTextBuf), L"%ldMB", DivU64x32(Header.TotalPhys, 1048576));
            LineChart_SetSerSubtitle(iSeries, szTextBuf);
        }

        if (bAve)
        {
            LineChart_ShowDataPoints(iSeries, FALSE);
        }
        else
        {

            LineChart_ShowDataPoints(iSeries, TRUE);
        }
        return TRUE;
    }
#endif
    return FALSE;
}

VOID
    EFIAPI
    SaveBenchReport(MT_HANDLE FileHandle, CHAR16 *ChartFilename)
{
    EFI_TIME ReportTime;
    int i;
    CHAR16 TempBuf[128];

    MtSupportWriteReportHeader(FileHandle);

    // Write out the heading
    FPrint(FileHandle, L"<h2>%s</h2>\n", GetStringById(STRING_TOKEN(STR_REPORT_SUMMARY), TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"<div class=\"summary\">\n");

    // Customer summary table
    FPrint(FileHandle, L"<table>\n");

    gRT->GetTime(&ReportTime, NULL);

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_DATE), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\">%d-%02d-%02d %02d:%02d:%02d</td>\n",
           ReportTime.Year, ReportTime.Month, ReportTime.Day, ReportTime.Hour, ReportTime.Minute, ReportTime.Second);
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_GENERATED_BY), TempBuf, sizeof(TempBuf)));
#ifdef MDE_CPU_X64
    FPrint(FileHandle, L"<td width=\"65%%\">%s %s (64-bit)</td>\n", PROGRAM_NAME, PROGRAM_VERSION);
#elif defined(MDE_CPU_IA32)
    FPrint(FileHandle, L"<td width=\"65%%\">%s %s (32-bit)</td>\n", PROGRAM_NAME, PROGRAM_VERSION); // BIT6010120009
#endif
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"<tr>\n");
    FPrint(FileHandle, L"<td width=\"35%%\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_TYPE), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"<td width=\"65%%\">%s</td>\n", GetStringById(STRING_TOKEN(STR_REPORT_TYPE_BENCHMARK), TempBuf, sizeof(TempBuf)));
    FPrint(FileHandle, L"</tr>\n");

    FPrint(FileHandle, L"</table>\n");

    FPrint(FileHandle, L"</div>\n");

    // Write Sys Info
    MtSupportWriteReportSysInfo(FileHandle);

    // Start a new table
    FPrint(FileHandle, L"<h2>%s</h2>\n", GetStringById(STRING_TOKEN(STR_REPORT_RESULT_SUMMARY), TempBuf, sizeof(TempBuf)));

    FPrint(FileHandle, L"<div class=\"results\">\n");

    FPrint(FileHandle, L"<p>\n");
    FPrint(FileHandle, L"<img src=\"%s\">\n", ChartFilename);
    FPrint(FileHandle, L"</p>\n");

    // Start results table
    FPrint(FileHandle, L"<table>\n");

    // Table 2 headings
    FPrint(FileHandle, L"<tr>\n");

    FPrint(FileHandle, L"<td width=\"20%%\" class=\"header\">%s</td>\n", gBenchConfig.TestMode == MEM_STEP ? GetStringById(STRING_TOKEN(STR_STEP_SIZE), TempBuf, sizeof(TempBuf)) : GetStringById(STRING_TOKEN(STR_BLOCK_SIZE), TempBuf, sizeof(TempBuf)));
    for (i = 0; i < LineChart_GetNumSer(); i++)
    {
        FPrint(FileHandle, L"<td class=\"header\">%s / %s (MB/s)</td>\n", LineChart_GetSerTitle(i), LineChart_GetSerSubtitle(i));
    }
    FPrint(FileHandle, L"</tr>\n");

    for (i = 0; i < LineChart_GetMaxNumCat(); i++)
    {
        int j;

        FPrint(FileHandle, L"<tr>\n");
        FPrint(FileHandle, L"<td width=\"20%%\" class=\"value\">%s</td>\n", LineChart_GetCatTitle(i));
        for (j = 0; j < LineChart_GetNumSer(); j++)
            FPrint(FileHandle, L"<td class=\"altvalue\">%ld</td>\n", LineChart_GetCatValue(j, i));
        FPrint(FileHandle, L"</tr>\n");
    }

    // End the table
    FPrint(FileHandle, L"</table>\n");

    FPrint(FileHandle, L"</div>\n");

    MtSupportWriteReportFooter(FileHandle);
}

/****************** OKN  ******************/
STATIC VOID PollAllUdpSockets(VOID)
{
  for (UINTN i = 0; i < gOknUdpRxSocketCount; ++i) {
    if (gOknUdpRxSockets[i] != NULL && gOknUdpRxSockets[i]->Udp4 != NULL) {
      gOknUdpRxSockets[i]->Udp4->Poll(gOknUdpRxSockets[i]->Udp4);
    }
  }

  if (gOknUdpSocketTransmit != NULL && gOknUdpSocketTransmit->Udp4 != NULL) {
    gOknUdpSocketTransmit->Udp4->Poll(gOknUdpSocketTransmit->Udp4);
  }
}

STATIC EFI_STATUS WaitForUdpNicBind(UINTN TimeoutMs)
{
  UINTN Elapsed = 0;
  UINTN Tick = 0;

  UDP_LOG("WaitForUdpNicBind() start, timeout=%u ms", TimeoutMs);

  while (NULL == gOknUdpRxActiveSocket) {
    PollAllUdpSockets();
    gBS->Stall(10 * 1000);

    Elapsed += 10;
    Tick++;

    if (0 == (Tick % 100)) { // 约 1 秒
      UDP_LOG("Waiting bind... Elapsed=%u ms Active=%p", Elapsed, gOknUdpRxActiveSocket);
    }

    if (TimeoutMs != 0 && Elapsed >= TimeoutMs) {
      UDP_LOG("WaitForUdpNicBind() TIMEOUT");
      return EFI_TIMEOUT;
    }
  }

  UDP_LOG("WaitForUdpNicBind() DONE, Active=%p", gOknUdpRxActiveSocket);
  return EFI_SUCCESS;
}

STATIC UINTN CountHandlesByProtocol(IN EFI_GUID *Guid)
{
  EFI_STATUS Status;
  EFI_HANDLE *Handles = NULL;
  UINTN      Count = 0;

  Status = gBS->LocateHandleBuffer(ByProtocol, Guid, NULL, &Count, &Handles);
  if (!EFI_ERROR(Status) && Handles != NULL) {
    FreePool(Handles);
  } 
  else {
    Count = 0;
  }
  return Count;
}

STATIC VOID DumpNetProtoCounts(VOID)
{
  EFI_GUID Dhcp4SbGuid = EFI_DHCP4_SERVICE_BINDING_PROTOCOL_GUID; // [NET] DHCP4 SB
  Print(L"[NET] SNP   handles         : %u\n", (UINT32)CountHandlesByProtocol(&gEfiSimpleNetworkProtocolGuid));
  Print(L"[NET] MNP   Service Binding : %u\n", (UINT32)CountHandlesByProtocol(&gEfiManagedNetworkServiceBindingProtocolGuid));
  Print(L"[NET] IP4   Service Binding : %u\n", (UINT32)CountHandlesByProtocol(&gEfiIp4ServiceBindingProtocolGuid));
  Print(L"[NET] UDP4  Service Binding : %u\n", (UINT32)CountHandlesByProtocol(&gEfiUdp4ServiceBindingProtocolGuid));
  Print(L"[NET] DHCP4 Service Binding : %u\n", (UINT32)CountHandlesByProtocol(&Dhcp4SbGuid));

  return;
}

STATIC VOID ConnectAllSnpControllers(VOID)
{
  EFI_STATUS Status;
  EFI_HANDLE *SnpHandles = NULL;
  UINTN      Count = 0;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleNetworkProtocolGuid, NULL, &Count, &SnpHandles);
  Print(L"[NET] Locate SNP: %r, Count=%u\n", Status, (UINT32)Count);

  if (EFI_ERROR(Status) || 0 == Count) {
    return;
  }

  for (UINTN i = 0; i < Count; i++) {
    EFI_STATUS S = gBS->ConnectController(SnpHandles[i], NULL, NULL, TRUE);
    Print(L"[NET] ConnectController(SNP[%u]): %r\n", (UINT32)i, S);
  }

  FreePool(SnpHandles);
  SnpHandles = NULL;

  return;
}

STATIC BOOLEAN IsZeroIp4(IN EFI_IPv4_ADDRESS *A)
{
  return (A->Addr[0] == 0 && A->Addr[1] == 0 && A->Addr[2] == 0 && A->Addr[3] == 0);
}

STATIC EFI_STATUS EnsureDhcpIp4Ready(EFI_HANDLE Handle, UINTN TimeoutMs, EFI_IPv4_ADDRESS *OutIp)
{
  EFI_STATUS                       Status;
  EFI_IP4_CONFIG2_PROTOCOL         *Ip4Cfg2 = NULL;
  EFI_IP4_CONFIG2_POLICY           Policy;
  EFI_IP4_CONFIG2_INTERFACE_INFO   *IfInfo = NULL;
  UINTN                            IfInfoSize = 0;

  Status = gBS->HandleProtocol(Handle, &gEfiIp4Config2ProtocolGuid, (VOID **)&Ip4Cfg2);
  if (EFI_ERROR(Status) || NULL == Ip4Cfg2) {
    Print(L"  IP4Config2 not found: %r\n", Status);
    return Status;
  }

  // 1) 设为 DHCP
  Policy = Ip4Config2PolicyDhcp;
  Status = Ip4Cfg2->SetData(Ip4Cfg2, Ip4Config2DataTypePolicy, sizeof(Policy), &Policy);
  Print(L"  IP4Config2 Set Policy DHCP: %r\n", Status);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // 2) 轮询等待 DHCP 生效（InterfaceInfo.StationAddress != 0）
  UINTN Waited = 0;
  while (Waited < TimeoutMs) {
    IfInfo = NULL;
    IfInfoSize = 0;

    Status = Ip4Cfg2->GetData(Ip4Cfg2, Ip4Config2DataTypeInterfaceInfo, &IfInfoSize, IfInfo);
    if (EFI_BUFFER_TOO_SMALL == Status && IfInfoSize > 0) {
      IfInfo = AllocateZeroPool(IfInfoSize);
      if (NULL == IfInfo) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = Ip4Cfg2->GetData(Ip4Cfg2, Ip4Config2DataTypeInterfaceInfo, &IfInfoSize, IfInfo);
      if (!EFI_ERROR(Status) && IfInfo != NULL && !IsZeroIp4(&IfInfo->StationAddress)) {
        *OutIp = IfInfo->StationAddress;

        Print(L"  DHCP Ready: %d.%d.%d.%d / %d.%d.%d.%d\n",
              IfInfo->StationAddress.Addr[0], IfInfo->StationAddress.Addr[1],
              IfInfo->StationAddress.Addr[2], IfInfo->StationAddress.Addr[3],
              IfInfo->SubnetMask.Addr[0], IfInfo->SubnetMask.Addr[1],
              IfInfo->SubnetMask.Addr[2], IfInfo->SubnetMask.Addr[3]);
        FreePool(IfInfo);
        return EFI_SUCCESS;
      }

      if (IfInfo) {
        FreePool(IfInfo);
      }
    }

    gBS->Stall(100 * 1000); // 100ms
    Waited += 100;
  }

  Print(L"  DHCP timeout (%u ms)\n", (UINT32)TimeoutMs);
  return EFI_TIMEOUT;
}
