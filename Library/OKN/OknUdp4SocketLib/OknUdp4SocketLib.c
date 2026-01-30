#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/Udp4.h>
#include <Protocol/UsbIo.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

#include <Library/OKN/OknUdp4SocketLib.h>

UDP4_SOCKET *gOknUdpSocketTransmit = NULL;
UDP4_SOCKET *gOknUdpRxSockets[MAX_UDP4_RX_SOCKETS];
UINTN        gOknUdpRxSocketCount    = 0;
UDP4_SOCKET *gOknUdpRxActiveSocket   = NULL;
EFI_HANDLE   gOknUdpRxActiveSbHandle = NULL;
UDP4_SOCKET *gOknJsonCtxSocket       = NULL;

static EFI_HANDLE sDefaultUdp4SbHandle = NULL;
static CHAR8      sStartOfBuffer[MAX_UDP4_FRAGMENT_LENGTH];

STATIC VOID PrintMac(IN CONST EFI_MAC_ADDRESS *Mac, IN UINT32 HwAddrSize)
{
  for (UINT32 i = 0; i < HwAddrSize; i++) {
    Print(L"%02x", (UINTN)Mac->Addr[i]);
    if (i + 1 < HwAddrSize)
      Print(L":");
  }
}

EFI_STATUS EFIAPI CreateUdp4SocketByServiceBindingHandle(IN EFI_HANDLE            Udp4ServiceBindingHandle,
                                                         IN EFI_UDP4_CONFIG_DATA *ConfigData,
                                                         IN EFI_EVENT_NOTIFY      NotifyReceive,
                                                         IN EFI_EVENT_NOTIFY      NotifyTransmit,
                                                         OUT UDP4_SOCKET        **Socket)
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL *Udp4Service;

  if (NULL == Udp4ServiceBindingHandle || NULL == ConfigData || NULL == NotifyReceive || NULL == NotifyTransmit ||
      Socket) {
    return EFI_INVALID_PARAMETER;
  }

  *Socket     = NULL;
  Udp4Service = NULL;

  Status = gBS->HandleProtocol(Udp4ServiceBindingHandle, &gEfiUdp4ServiceBindingProtocolGuid, (VOID **)&Udp4Service);
  if (EFI_ERROR(Status) || NULL == Udp4Service) {
    return EFI_NOT_FOUND;
  }

  *Socket = AllocateZeroPool(sizeof(UDP4_SOCKET));
  if (NULL == *Socket) {
    return EFI_OUT_OF_RESOURCES;
  }

  (*Socket)->ServiceBindingHandle = Udp4ServiceBindingHandle;
  (*Socket)->ServiceBinding       = Udp4Service;

  Status = Udp4Service->CreateChild(Udp4Service, &(*Socket)->ChildHandle);

  if (EFI_ERROR(Status)) {
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  Status = gBS->OpenProtocol((*Socket)->ChildHandle,
                             &gEfiUdp4ProtocolGuid,
                             (VOID **)&(*Socket)->Udp4,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (EFI_ERROR(Status) || NULL == (*Socket)->Udp4) {
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  (*Socket)->ConfigData = *ConfigData;
  Status                = (*Socket)->Udp4->Configure((*Socket)->Udp4, ConfigData);
  if (EFI_ERROR(Status)) {
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  // IMPORTANT:
  // For EFI_UDP4_PROTOCOL.Receive(), the caller must submit the token with
  // Token.Packet.RxData. The UDP4 implementation will allocate an
  // EFI_UDP4_RECEIVE_DATA buffer and return it via Token.Packet.RxData on
  // completion. The caller must recycle the buffer by signaling
  // RxData->RecycleSignal.
  (*Socket)->TokenReceive.Packet.RxData = NULL;

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyReceive, *Socket, &(*Socket)->TokenReceive.Event);
  if (EFI_ERROR(Status)) {
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  (*Socket)->TokenTransmit.Packet.TxData = AllocateZeroPool(sizeof(EFI_UDP4_TRANSMIT_DATA));
  if ((*Socket)->TokenTransmit.Packet.TxData) {
    gBS->CloseEvent((*Socket)->TokenReceive.Event);
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyTransmit, *Socket, &(*Socket)->TokenTransmit.Event);
  if (EFI_ERROR(Status)) {
    FreePool((*Socket)->TokenTransmit.Packet.TxData);
    gBS->CloseEvent((*Socket)->TokenReceive.Event);
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI CloseUdp4Socket(IN UDP4_SOCKET *Socket)
{
  EFI_STATUS Status;

  if (NULL == Socket) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  if (Socket->Udp4 != NULL) {
    Status = Socket->Udp4->Cancel(Socket->Udp4, NULL);
    Status |= Socket->Udp4->Configure(Socket->Udp4, NULL);
  }

  if (Socket->TokenReceive.Event != NULL) {
    Status |= gBS->CloseEvent(Socket->TokenReceive.Event);
  }
  if (Socket->TokenTransmit.Event != NULL) {
    Status |= gBS->CloseEvent(Socket->TokenTransmit.Event);
  }

  if (Socket->ServiceBinding != NULL && Socket->ChildHandle != NULL) {
    Status |= Socket->ServiceBinding->DestroyChild(Socket->ServiceBinding, Socket->ChildHandle);
  }

  // NOTE: Receive buffers are owned by UDP4 and must be recycled via
  // RxData->RecycleSignal. Do not FreePool() them here.
  if (Socket->TokenTransmit.Packet.TxData != NULL) {
    FreePool(Socket->TokenTransmit.Packet.TxData);
  }

  FreePool(Socket);
  Socket = NULL;
  return Status;
}

EFI_STATUS EFIAPI AsciiUdp4Write(IN UDP4_SOCKET *Socket, IN CONST CHAR8 *FormatString, ...)
{
  VA_LIST Marker;
  UINT32  NumberOfPrinted;

  if (NULL == Socket) {
    return EFI_NOT_FOUND;
  }

  VA_START(Marker, FormatString);
  NumberOfPrinted = (UINT32)AsciiVSPrint(sStartOfBuffer, MAX_UDP4_FRAGMENT_LENGTH, FormatString, Marker);
  VA_END(Marker);

  EFI_UDP4_TRANSMIT_DATA *TxData = Socket->TokenTransmit.Packet.TxData;
  ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
  TxData->DataLength                      = NumberOfPrinted;
  TxData->FragmentCount                   = 1;
  TxData->FragmentTable[0].FragmentLength = NumberOfPrinted;
  TxData->FragmentTable[0].FragmentBuffer = sStartOfBuffer;
  return Socket->Udp4->Transmit(Socket->Udp4, &Socket->TokenTransmit);
}

RETURN_STATUS EFIAPI Udp4SocketLibConstructor(VOID)
{
  EFI_STATUS           Status;
  UINTN                HandleNum;
  EFI_HANDLE          *Udp4Handles;
  EFI_USB_IO_PROTOCOL *UsbIo;

  sDefaultUdp4SbHandle = NULL;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUdp4ServiceBindingProtocolGuid, NULL, &HandleNum, &Udp4Handles);
  if (EFI_ERROR(Status)) {
    Print(L"LocateHandleBuffer status: %r\n", Status);
    return Status;
  }

  for (UINTN i = 0; i < HandleNum; i++) {
    UsbIo = NULL;
    // 跳过USB网卡
    if (!EFI_ERROR(gBS->HandleProtocol(Udp4Handles[i], &gEfiUsbIoProtocolGuid, (VOID **)&UsbIo))) {
      continue;
    }

    sDefaultUdp4SbHandle = Udp4Handles[i];
    break;
  }

  if (Udp4Handles != NULL) {
    FreePool(Udp4Handles);
    Udp4Handles = NULL;
  }

  return (sDefaultUdp4SbHandle != NULL) ? RETURN_SUCCESS : RETURN_NOT_FOUND;
}

/**
 * 每个 NIC 创建一个 RX socket（多个）
 */
EFI_STATUS StartUdp4ReceiveOnAllNics(IN EFI_UDP4_CONFIG_DATA *RxCfg)
{
  EFI_STATUS                   Status;
  EFI_HANDLE                  *Handles;
  UINTN                        HandleNum;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;

  Print(L"Enter StartUdp4ReceiveOnAllNics()\n");

  if (RxCfg == NULL) {
    Print(L"RxCfg == NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  Handles              = NULL;
  HandleNum            = 0;
  gOknUdpRxSocketCount = 0;

  Print(L"LocateHandleBuffer(Udp4ServiceBinding) ...\n");
  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUdp4ServiceBindingProtocolGuid, NULL, &HandleNum, &Handles);
  Print(L"LocateHandleBuffer: %r, HandleNum=%u\n", Status, (UINT32)HandleNum);
  if (EFI_ERROR(Status)) {
    Print(L"LocateHandleBuffer() failed\n");
    return Status;
  }

  Print(L"RxCfg: UseDefault=%u AcceptBroadcast=%u AllowDupPort=%u StationPort=%u RemotePort=%u\n",
        (UINT32)RxCfg->UseDefaultAddress,
        (UINT32)RxCfg->AcceptBroadcast,
        (UINT32)RxCfg->AllowDuplicatePort,
        (UINT32)RxCfg->StationPort,
        (UINT32)RxCfg->RemotePort);

  for (UINTN i = 0; i < HandleNum && gOknUdpRxSocketCount < MAX_UDP4_RX_SOCKETS; i++) {
    Print(L"SB[%u] Handle=%p\n", (UINT32)i, Handles[i]);

    // 跳过 USB NIC, 不跳过是不是也行??
    UsbIo  = NULL;
    Status = gBS->HandleProtocol(Handles[i], &gEfiUsbIoProtocolGuid, (VOID **)&UsbIo);
    if (!EFI_ERROR(Status)) {
      Print(L"  Skip: USB NIC\n");
      continue;
    }

    EFI_IPv4_ADDRESS Ip;
    ZeroMem(&Ip, sizeof(Ip));

    EFI_STATUS DhcpStatus;
    UINTN      DhcpTimeoutMs = 15000;
    BOOLEAN    HasSnp        = FALSE;

    // 尽量选择实际有效的链接.
    Snp    = NULL;
    Status = gBS->HandleProtocol(Handles[i], &gEfiSimpleNetworkProtocolGuid, (VOID **)&Snp);
    if (!EFI_ERROR(Status) && Snp != NULL && Snp->Mode != NULL) {
      HasSnp = TRUE;
      Print(L"  SNP MediaPresent=%u State=%u HwAddrSize=%u\n",
            (UINT32)Snp->Mode->MediaPresent,
            (UINT32)Snp->Mode->State,
            (UINT32)Snp->Mode->HwAddressSize);

      if (Snp->Mode->HwAddressSize >= 6) {
        Print(L"  MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
              Snp->Mode->CurrentAddress.Addr[0],
              Snp->Mode->CurrentAddress.Addr[1],
              Snp->Mode->CurrentAddress.Addr[2],
              Snp->Mode->CurrentAddress.Addr[3],
              Snp->Mode->CurrentAddress.Addr[4],
              Snp->Mode->CurrentAddress.Addr[5]);
      }

      if (0 == Snp->Mode->MediaPresent) {
        Print(L"  Warn: Skip for MediaPresent=0\n");
        continue;
      }
    }
    else {
      Print(L"  SNP not found or no Mode (Status=%r)\n", Status);
      // 不强制 continue: 信步说有的平台 UDP4 SB handle 上不一定挂 SNP
    }

    // 先跑 DHCP, 让 UseDefaultAddress=TRUE 有 mapping
    DhcpStatus = EnsureDhcpIp4Ready(Handles[i], DhcpTimeoutMs, &Ip);
    if (EFI_ERROR(DhcpStatus)) {
      Print(L"  DHCP not ready: %r\n", DhcpStatus);
      if (RxCfg->UseDefaultAddress) {
        Print(L"  Skip: UseDefaultAddress=TRUE requires DHCP mapping\n");
        continue;
      }
    }

    // 创建 socket
    UDP4_SOCKET *Sock = NULL;
    Status            = CreateUdp4SocketByServiceBindingHandle(Handles[i],
                                                    RxCfg,
                                                    (EFI_EVENT_NOTIFY)Udp4ReceiveHandler,
                                                    (EFI_EVENT_NOTIFY)Udp4NullHandler,
                                                    &Sock);
    Print(L"  CreateSock: %r, Sock=%p\n", Status, Sock);
    if (EFI_ERROR(Status) || Sock == NULL) {
      Print(L"  Skip: CreateSock failed\n");
      continue;
    }

    // 保存 NIC 身份到 Sock（关键）
    if (HasSnp && Snp->Mode && Snp->Mode->HwAddressSize >= 6) {
      CopyMem(Sock->NicMac, Snp->Mode->CurrentAddress.Addr, 6);
    }
    else {
      ZeroMem(Sock->NicMac, 6);
    }

    Sock->NicIpValid = !EFI_ERROR(DhcpStatus);
    if (Sock->NicIpValid) {
      Sock->NicIp = Ip;
    }
    else {
      ZeroMem(&Sock->NicIp, sizeof(Sock->NicIp));
    }
    Sock->TokenReceive.Packet.RxData = NULL;
    Status                           = Sock->Udp4->Receive(Sock->Udp4, &Sock->TokenReceive);
    Print(L"  Receive(submit): %r\n", Status);
    if (EFI_ERROR(Status)) {
      Print(L"  Receive() FAILED on SB handle %p: %r\n", Handles[i], Status);
      CloseUdp4Socket(Sock);
      continue;
    }

    gOknUdpRxSockets[gOknUdpRxSocketCount++] = Sock;
    Print(L"  Added RX socket. Count=%u\n", (UINT32)gOknUdpRxSocketCount);
  }  // for() 结束

  if (Handles != NULL) {
    FreePool(Handles);
  }

  Print(L"Exit StartUdp4ReceiveOnAllNics(): gOknUdpRxSocketCount=%u\n", (UINT32)gOknUdpRxSocketCount);
  gBS->Stall(2 * 1000 * 1000);
  return (gOknUdpRxSocketCount > 0) ? EFI_SUCCESS : EFI_NOT_FOUND;
}
