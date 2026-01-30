#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/Udp4.h>
#include <Protocol/UsbIo.h>

#include <Library/OKN/OknUdp4SocketLib.h>

UDP4_SOCKET            *gOknUdpSocketTransmit = NULL;
UDP4_SOCKET            *gOknUdpRxSockets[MAX_UDP4_RX_SOCKETS];
UINTN                   gOknUdpRxSocketCount    = 0;
UDP4_SOCKET            *gOknUdpRxActiveSocket   = NULL;
EFI_HANDLE              gOknUdpRxActiveSbHandle = NULL;
UDP4_SOCKET            *gOknJsonCtxSocket       = NULL;
STATIC volatile BOOLEAN gUdpNicBound            = FALSE;  // ?

static EFI_HANDLE sDefaultUdp4SbHandle = NULL;
static CHAR8      sStartOfBuffer[MAX_UDP4_FRAGMENT_LENGTH];

STATIC VOID OknPrintMac(IN CONST EFI_MAC_ADDRESS *Mac, IN UINT32 HwAddrSize);
STATIC VOID PrintIpv4A(IN CONST EFI_IPv4_ADDRESS *Ip);
STATIC VOID OknDisableUdpRxSocket(IN UDP4_SOCKET *Sock);

EFI_STATUS OknUdp4SocketLibConstructor(VOID)
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

  return (sDefaultUdp4SbHandle != NULL) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

EFI_STATUS OknCreateUdp4SocketByServiceBindingHandle(IN EFI_HANDLE            Udp4ServiceBindingHandle,
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

EFI_STATUS OknCloseUdp4Socket(IN UDP4_SOCKET *Socket)
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

/**
 * 每个 NIC 创建一个 RX socket（多个）
 */
EFI_STATUS OknStartUdp4ReceiveOnAllNics(IN EFI_UDP4_CONFIG_DATA *RxCfg)
{
  EFI_STATUS                   Status;
  EFI_HANDLE                  *Handles;
  UINTN                        HandleNum;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;

  Print(L"Enter OknStartUdp4ReceiveOnAllNics()\n");

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
    DhcpStatus = OknEnsureDhcpIp4Ready(Handles[i], DhcpTimeoutMs, &Ip);
    if (EFI_ERROR(DhcpStatus)) {
      Print(L"  DHCP not ready: %r\n", DhcpStatus);
      if (RxCfg->UseDefaultAddress) {
        Print(L"  Skip: UseDefaultAddress=TRUE requires DHCP mapping\n");
        continue;
      }
    }

    // 创建 socket
    UDP4_SOCKET *Sock = NULL;
    Status            = OknCreateUdp4SocketByServiceBindingHandle(Handles[i],
                                                       RxCfg,
                                                       (EFI_EVENT_NOTIFY)OknUdp4ReceiveHandler,
                                                       (EFI_EVENT_NOTIFY)OknUdp4NullHandler,
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
      OknCloseUdp4Socket(Sock);
      continue;
    }

    gOknUdpRxSockets[gOknUdpRxSocketCount++] = Sock;
    Print(L"  Added RX socket. Count=%u\n", (UINT32)gOknUdpRxSocketCount);
  }  // for() 结束

  if (Handles != NULL) {
    FreePool(Handles);
  }

  Print(L"Exit OknStartUdp4ReceiveOnAllNics(): gOknUdpRxSocketCount=%u\n", (UINT32)gOknUdpRxSocketCount);
  gBS->Stall(2 * 1000 * 1000);
  return (gOknUdpRxSocketCount > 0) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

VOID OknUdp4ReceiveHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  UDP4_SOCKET           *Socket;
  EFI_UDP4_RECEIVE_DATA *RxData;
  EFI_STATUS             Status;

  Socket = (UDP4_SOCKET *)Context;
  if (Socket == NULL || Socket->Udp4 == NULL) {
    return;
  }

  RxData = Socket->TokenReceive.Packet.RxData;

  // 1) Token 完成但没有数据: 按 UEFI UDP4 约定, 重新投递 Receive()
  if (NULL == RxData) {
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

  // 2) 若 Receive 被 Abort: 回收 RxData 并退出
  if (EFI_ABORTED == Socket->TokenReceive.Status) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    return;
  }

  // 3) 空包: 回收并重新投递
  if (RxData->DataLength == 0 || RxData->FragmentCount == 0 || RxData->FragmentTable[0].FragmentBuffer == NULL ||
      RxData->FragmentTable[0].FragmentLength == 0) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

  // 4) 首包绑定: 选择第一个收到包的 NIC, 其它 RX socket 全部禁用
  if (gOknUdpRxActiveSocket == NULL) {
    gOknUdpRxActiveSocket   = Socket;
    gOknUdpRxActiveSbHandle = Socket->ServiceBindingHandle;

    for (UINTN i = 0; i < gOknUdpRxSocketCount; i++) {
      if (gOknUdpRxSockets[i] != NULL && gOknUdpRxSockets[i] != Socket) {
        OknDisableUdpRxSocket(gOknUdpRxSockets[i]);
      }
    }
  }

  // 5) 非选中 NIC 上来的包直接丢弃（但要回收 RxData）
  if (Socket != gOknUdpRxActiveSocket) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    return;
  }

  // 6) 懒创建 TX socket: 只创建一次, 绑定到选中的 SB handle
  //    注意: NotifyTransmit 改为 OknUdp4TxFreeHandler, 用于释放本次发送的内存
  if (gOknUdpSocketTransmit == NULL && gOknUdpRxActiveSbHandle != NULL) {
    EFI_UDP4_CONFIG_DATA TxCfg = {
        TRUE,                  // AcceptBroadcast
        FALSE,                 // AcceptPromiscuous
        FALSE,                 // AcceptAnyPort
        TRUE,                  // AllowDuplicatePort
        0,                     // TypeOfService
        16,                    // TimeToLive
        TRUE,                  // DoNotFragment
        0,                     // ReceiveTimeout
        0,                     // TransmitTimeout
        TRUE,                  // UseDefaultAddress
        {{0, 0, 0, 0}},        // StationAddress
        {{0, 0, 0, 0}},        // SubnetMask
        OKN_STATION_UDP_PORT,  // StationPort
        {{0, 0, 0, 0}},        // RemoteAddress (unused when using UdpSessionData)
        0,                     // RemotePort    (unused when using UdpSessionData)
    };

    EFI_STATUS TxStatus;

    TxStatus = OknCreateUdp4SocketByServiceBindingHandle(
        gOknUdpRxActiveSbHandle,
        &TxCfg,
        (EFI_EVENT_NOTIFY)OknUdp4NullHandler,    // Tx socket 不需要 Receive 回调
        (EFI_EVENT_NOTIFY)OknUdp4TxFreeHandler,  // TX 完成释放资源: OknUdp4TxFreeHandler
        &gOknUdpSocketTransmit);

    if (EFI_ERROR(TxStatus)) {
      Print(L"[UDP] Create TX socket failed: %r\n", TxStatus);
      gOknUdpSocketTransmit = NULL;
    }
    else {
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

  // 8) 解析 JSON
  cJSON *Tree = cJSON_ParseWithLength((CONST CHAR8 *)RxData->FragmentTable[0].FragmentBuffer,
                                      RxData->FragmentTable[0].FragmentLength);

  if (Tree != NULL) {
    // 9) 设置 JsonHandler 上下文: 确保 areyouok 填的是“当前 NIC”的 MAC/IP
    gOknJsonCtxSocket = Socket;
    JsonHandler(Tree);
    gOknJsonCtxSocket = NULL;

    // 10) 生成响应 JSON（cJSON_PrintUnformatted 返回需要 cJSON_free 的内存）
    CHAR8 *JsonStr = cJSON_PrintUnformatted(Tree);
    if (JsonStr != NULL) {
      UINT32 JsonStrLen = (UINT32)AsciiStrLen(JsonStr);

      // 关键: Transmit 是异步的, 不能直接把 JsonStr 指针塞给 TxData 再立刻释放
      // 所以这里复制一份到 Pool, TX 完成回调里释放
      VOID *Payload = AllocateCopyPool(JsonStrLen, JsonStr);
      cJSON_free(JsonStr);

      if (Payload != NULL && JsonStrLen > 0) {
        EFI_UDP4_SESSION_DATA *Sess = AllocateZeroPool(sizeof(EFI_UDP4_SESSION_DATA));
        if (Sess != NULL) {
          Sess->DestinationAddress = RxData->UdpSession.SourceAddress;
          Sess->DestinationPort    = RxData->UdpSession.SourcePort;
          Sess->SourcePort         = 5566;
          // 可选: 如果你在 Socket 里保存了 DHCP IP, 可把源地址也填上
          // Sess->SourceAddress = Socket->NicIp;

          EFI_UDP4_TRANSMIT_DATA *TxData = gOknUdpSocketTransmit->TokenTransmit.Packet.TxData;
          ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
          TxData->UdpSessionData                  = Sess;
          TxData->DataLength                      = JsonStrLen;
          TxData->FragmentCount                   = 1;
          TxData->FragmentTable[0].FragmentLength = JsonStrLen;
          TxData->FragmentTable[0].FragmentBuffer = Payload;

          // 记录待释放资源, TX 完成回调释放
          gOknUdpSocketTransmit->TxPayload    = Payload;
          gOknUdpSocketTransmit->TxSession    = Sess;
          gOknUdpSocketTransmit->TxInProgress = TRUE;

          Status =
              gOknUdpSocketTransmit->Udp4->Transmit(gOknUdpSocketTransmit->Udp4, &gOknUdpSocketTransmit->TokenTransmit);
          if (EFI_ERROR(Status)) {
            // 发送失败: 立即释放并复位
            gOknUdpSocketTransmit->TxInProgress = FALSE;
            if (gOknUdpSocketTransmit->TxPayload) {
              FreePool(gOknUdpSocketTransmit->TxPayload);
              gOknUdpSocketTransmit->TxPayload = NULL;
            }
            if (gOknUdpSocketTransmit->TxSession) {
              FreePool(gOknUdpSocketTransmit->TxSession);
              gOknUdpSocketTransmit->TxSession = NULL;
            }
          }
        }
        else {
          FreePool(Payload);
        }
      }
      else {
        if (Payload)
          FreePool(Payload);
      }
    }

    cJSON_Delete(Tree);
  }

  // 11) 回收 RxData 并重新投递 Receive
  gBS->SignalEvent(RxData->RecycleSignal);
  Socket->TokenReceive.Packet.RxData = NULL;
  Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
  return;
}

VOID OknPollAllUdpSockets(VOID)
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

EFI_STATUS OknWaitForUdpNicBind(UINTN TimeoutMs)
{
  UINTN Elapsed = 0;
  UINTN Tick    = 0;

  UDP_LOG("OknWaitForUdpNicBind() start, timeout=%u ms", TimeoutMs);

  while (NULL == gOknUdpRxActiveSocket) {
    OknPollAllUdpSockets();
    gBS->Stall(10 * 1000);

    Elapsed += 10;
    Tick++;

    if (0 == (Tick % 100)) {  // 约 1 秒
      UDP_LOG("Waiting bind... Elapsed=%u ms Active=%p", Elapsed, gOknUdpRxActiveSocket);
    }

    if (TimeoutMs != 0 && Elapsed >= TimeoutMs) {
      UDP_LOG("OknWaitForUdpNicBind() TIMEOUT");
      return EFI_TIMEOUT;
    }
  }

  UDP_LOG("OknWaitForUdpNicBind() DONE, Active=%p", gOknUdpRxActiveSocket);
  return EFI_SUCCESS;
}

UINTN OknCountHandlesByProtocol(IN EFI_GUID *Guid)
{
  EFI_STATUS  Status;
  EFI_HANDLE *Handles = NULL;
  UINTN       Count   = 0;

  Status = gBS->LocateHandleBuffer(ByProtocol, Guid, NULL, &Count, &Handles);
  if (!EFI_ERROR(Status) && Handles != NULL) {
    FreePool(Handles);
  }
  else {
    Count = 0;
  }
  return Count;
}

VOID OknDumpNetProtoCounts(VOID)
{
  EFI_GUID Dhcp4SbGuid = EFI_DHCP4_SERVICE_BINDING_PROTOCOL_GUID;  // [NET] DHCP4 SB
  Print(L"[NET] SNP   handles         : %u\n", (UINT32)OknCountHandlesByProtocol(&gEfiSimpleNetworkProtocolGuid));
  Print(L"[NET] MNP   Service Binding : %u\n",
        (UINT32)OknCountHandlesByProtocol(&gEfiManagedNetworkServiceBindingProtocolGuid));
  Print(L"[NET] IP4   Service Binding : %u\n", (UINT32)OknCountHandlesByProtocol(&gEfiIp4ServiceBindingProtocolGuid));
  Print(L"[NET] UDP4  Service Binding : %u\n", (UINT32)OknCountHandlesByProtocol(&gEfiUdp4ServiceBindingProtocolGuid));
  Print(L"[NET] DHCP4 Service Binding : %u\n", (UINT32)OknCountHandlesByProtocol(&Dhcp4SbGuid));

  return;
}

VOID OknConnectAllSnpControllers(VOID)
{
  EFI_STATUS  Status;
  EFI_HANDLE *SnpHandles = NULL;
  UINTN       Count      = 0;

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

BOOLEAN OknIsZeroIp4(IN EFI_IPv4_ADDRESS *A)
{
  return (A->Addr[0] == 0 && A->Addr[1] == 0 && A->Addr[2] == 0 && A->Addr[3] == 0);
}

EFI_STATUS OknEnsureDhcpIp4Ready(EFI_HANDLE Handle, UINTN TimeoutMs, EFI_IPv4_ADDRESS *OutIp)
{
  EFI_STATUS                      Status;
  EFI_IP4_CONFIG2_PROTOCOL       *Ip4Cfg2 = NULL;
  EFI_IP4_CONFIG2_POLICY          Policy;
  EFI_IP4_CONFIG2_INTERFACE_INFO *IfInfo     = NULL;
  UINTN                           IfInfoSize = 0;

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
    IfInfo     = NULL;
    IfInfoSize = 0;

    Status = Ip4Cfg2->GetData(Ip4Cfg2, Ip4Config2DataTypeInterfaceInfo, &IfInfoSize, IfInfo);
    if (EFI_BUFFER_TOO_SMALL == Status && IfInfoSize > 0) {
      IfInfo = AllocateZeroPool(IfInfoSize);
      if (NULL == IfInfo) {
        return EFI_OUT_OF_RESOURCES;
      }

      Status = Ip4Cfg2->GetData(Ip4Cfg2, Ip4Config2DataTypeInterfaceInfo, &IfInfoSize, IfInfo);
      if (!EFI_ERROR(Status) && IfInfo != NULL && !OknIsZeroIp4(&IfInfo->StationAddress)) {
        *OutIp = IfInfo->StationAddress;

        Print(L"  DHCP Ready: %d.%d.%d.%d / %d.%d.%d.%d\n",
              IfInfo->StationAddress.Addr[0],
              IfInfo->StationAddress.Addr[1],
              IfInfo->StationAddress.Addr[2],
              IfInfo->StationAddress.Addr[3],
              IfInfo->SubnetMask.Addr[0],
              IfInfo->SubnetMask.Addr[1],
              IfInfo->SubnetMask.Addr[2],
              IfInfo->SubnetMask.Addr[3]);
        FreePool(IfInfo);
        return EFI_SUCCESS;
      }

      if (IfInfo) {
        FreePool(IfInfo);
      }
    }

    gBS->Stall(100 * 1000);  // 100ms
    Waited += 100;
  }

  Print(L"  DHCP timeout (%u ms)\n", (UINT32)TimeoutMs);
  return EFI_TIMEOUT;
}

// TX 完成后释放本次发送申请的内存, 避免泄漏
VOID OknUdp4TxFreeHandler(IN EFI_EVENT Event, IN VOID *Context)
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

VOID OknUdp4NullHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  return;
}

EFI_STATUS OknAsciiUdp4Write(IN UDP4_SOCKET *Socket, IN CONST CHAR8 *FormatString, ...)
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

/**************************************************************************************************/
STATIC VOID OknPrintMac(IN CONST EFI_MAC_ADDRESS *Mac, IN UINT32 HwAddrSize)
{
  for (UINT32 i = 0; i < HwAddrSize; i++) {
    Print(L"%02x", (UINTN)Mac->Addr[i]);
    if (i + 1 < HwAddrSize)
      Print(L":");
  }
}

STATIC VOID PrintIpv4A(IN CONST EFI_IPv4_ADDRESS *Ip)
{
  Print(L"%u.%u.%u.%u", (UINTN)Ip->Addr[0], (UINTN)Ip->Addr[1], (UINTN)Ip->Addr[2], (UINTN)Ip->Addr[3]);
}

STATIC VOID OknDisableUdpRxSocket(IN UDP4_SOCKET *Sock)
{
  if (NULL == Sock || NULL == Sock->Udp4) {
    return;
  }
  // Cancel the pending receive token to stop further callbacks.
  Sock->Udp4->Cancel(Sock->Udp4, &Sock->TokenReceive);
}
