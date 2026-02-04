#include <Library/OKN/OknMemTestLib.h>
#include <Library/OKN/OknUdp4SocketLib.h>
#include <Library/OKN/PortingLibs/cJSONLib.h>

#define OKN_UDP_DBG

/**
 * 以太网常见 MTU=1500(不含二层头), IPv4 + UDP 头共 28 字节: 
 * 最大不分片 UDP payload ≈ 1500 - 20 - 8 = 1472
 * 再留一点余量(VLAN tag、隧道、不同驱动/栈差异、某些环境 MTU 不是 1500), 工程上就常取 1400 或 1200. 
 * 所以 1400 的意义是: 尽量保证"不走 IP 分片", 提高稳定性. 
 */
#define OKN_UDP_SAFE_PAYLOAD 1400

UDP4_SOCKET            *gOknUdpSocketTransmit = NULL;
UDP4_SOCKET            *gOknUdpRxSockets[MAX_UDP4_RX_SOCKETS];
UINTN                   gOknUdpRxSocketCount    = 0;
UDP4_SOCKET            *gOknUdpRxActiveSocket   = NULL;
EFI_HANDLE              gOknUdpRxActiveSbHandle = NULL;
UDP4_SOCKET            *gOknJsonCtxSocket       = NULL;
STATIC volatile BOOLEAN gOknUdpNicBound         = FALSE;  // ?

/**
 * 下面这两个变量是不是可以删除?
 */
static EFI_HANDLE sDefaultUdp4SbHandle = NULL;
static CHAR8      sStartOfBuffer[MAX_UDP4_FRAGMENT_LENGTH];

STATIC VOID       OknPrintMac(IN CONST EFI_MAC_ADDRESS *Mac, IN UINT32 HwAddrSize);
STATIC VOID       OknPrintIpv4A(IN CONST EFI_IPv4_ADDRESS Ip);
STATIC VOID       OknDumpAsciiN(IN CONST VOID *Buf, IN UINTN Len, IN UINTN Max);
STATIC VOID       OknDisableUdpRxSocket(IN UDP4_SOCKET *Sock);
STATIC UINTN      OknCountHandlesByProtocol(IN EFI_GUID *Guid);
STATIC VOID       OknPollAllUdpSockets(VOID);
STATIC BOOLEAN    OknIsZeroIp4(IN EFI_IPv4_ADDRESS *A);
STATIC EFI_STATUS OknEnsureDhcpIp4Ready(EFI_HANDLE Handle, UINTN TimeoutMs, EFI_IPv4_ADDRESS *OutIp);

RETURN_STATUS EFIAPI OknUdp4SocketLibConstructor(VOID)
{
  EFI_STATUS           Status;
  UINTN                HandleNum;
  EFI_HANDLE          *Udp4Handles;
  EFI_USB_IO_PROTOCOL *UsbIo;

  sDefaultUdp4SbHandle = NULL;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUdp4ServiceBindingProtocolGuid, NULL, &HandleNum, &Udp4Handles);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] LocateHandleBuffer status: %r\n", Status);
    return Status;
  }

  for (UINTN i = 0; i < HandleNum; i++) {
    UsbIo = NULL;
    // 跳过USB网卡
    if (FALSE == EFI_ERROR(gBS->HandleProtocol(Udp4Handles[i], &gEfiUsbIoProtocolGuid, (VOID **)&UsbIo))) {
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

VOID EFIAPI OknUdp4ReceiveHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  EFI_STATUS             Status;
  UDP4_SOCKET           *Socket;
  EFI_UDP4_RECEIVE_DATA *RxData;

  /**
   * EFI_UDP4_CONFIG_DATA.DoNotFragment=TRUE 会在 IPv4 包上设置 DF 标志.
   * 当 UDP payload(含 IP/UDP 头)超过 MTU 时, 网络栈必须分片(IP 层分片); 但 DF=1
   * 时禁止分片, 发送要么失败、要么直接不出线. 表现为: Qt/Python已经发送了命令, MT86也显示收到了这个命令,
   * 但是MT86就是没有任何回复, 在HW_INFO这个命令的时候就有这个问题, 因为当时的g_numSMBIOSMem是32,
   * 导致HW_INFO的回复保温很大, 然后添加打印调式发现TX一直失败, OknUdp4TxFreeHandler一直没被调用;
   * 因对端能收到请求, 但你这端永远收不到回包; 抓包看到只有请求; TX 完成回调不触发.
   *
   * 当 datagram 太大时, 能不能发、怎么处理取决于:
   *  - IP 层是否允许分片(表现在用 DoNotFragment 控 DF)
   *  - 路径 MTU(PMTU)发现是否工作
   *  - UDP4/IP4 驱动/协议实现是否稳定支持分片
   *  - 交换机/链路是否会丢分片(很常见)
   *  - 所以"默认分包多大"没有一个固定值; 核心约束就是: 不能超过链路 MTU 所允许的单包大小, 超过就需要
   *    IP分片(DF=FALSE)或直接失败(DF=TRUE).
   */
  EFI_UDP4_CONFIG_DATA TxCfg = {
      TRUE,                  // AcceptBroadcast
      FALSE,                 // AcceptPromiscuous
      FALSE,                 // AcceptAnyPort
      TRUE,                  // AllowDuplicatePort
      0,                     // TypeOfService
      16,                    // TimeToLive
      FALSE,                 // DoNotFragment  (Tx 可能很大, 这样就必须允许分片, 这样避免TX失败)
      0,                     // ReceiveTimeout
      0,                     // TransmitTimeout
      TRUE,                  // UseDefaultAddress
      {{0, 0, 0, 0}},        // StationAddress
      {{0, 0, 0, 0}},        // SubnetMask
      OKN_STATION_UDP_PORT,  // StationPort
      {{0, 0, 0, 0}},        // RemoteAddress (unused when using UdpSessionData)
      0,                     // RemotePort    (unused when using UdpSessionData)
  };

  Socket = (UDP4_SOCKET *)Context;
  if (NULL == Socket || NULL == Socket->Udp4) {
    Print(L"[OKN_UDP_ERROR] Socket or Socket->Udp4 is NULL.\n");
    return;
  }

  RxData = Socket->TokenReceive.Packet.RxData;

  // 1) Token 完成但没有数据: 按 UEFI UDP4 约定, 重新投递 Receive()
  if (NULL == RxData) {
    Print(L"[OKN_UDP_WARNING] RxData is NULL, Status=%r\n", Socket->TokenReceive.Status);
    Socket->TokenReceive.Packet.RxData = NULL;
    Status                             = Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    Print(L"[OKN_UDP_DEBUG] resubmit Receive: %r\n", Status);
    return;
  }

  // 2) 若 Receive 被 Abort: 回收 RxData 并退出
  if (EFI_ABORTED == Socket->TokenReceive.Status) {
    Print(L"[OKN_UDP_WARNING] Socket TokenReceive Status is EFI_ABORTED\n");
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    return;
  }

  // 3) 空包: 回收并重新投递
  if (0 == RxData->DataLength || 0 == RxData->FragmentCount || NULL == RxData->FragmentTable[0].FragmentBuffer ||
      0 == RxData->FragmentTable[0].FragmentLength) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

  // 4) 首包绑定: 选择第一个收到包的 NIC, 其它 RX socket 全部禁用
  if (NULL == gOknUdpRxActiveSocket) {
    gOknUdpRxActiveSocket   = Socket;
    gOknUdpRxActiveSbHandle = Socket->ServiceBindingHandle;

    for (UINTN i = 0; i < gOknUdpRxSocketCount; i++) {
      if (gOknUdpRxSockets[i] != NULL && gOknUdpRxSockets[i] != Socket) {
        OknDisableUdpRxSocket(gOknUdpRxSockets[i]);
      }
    }
  }

  // 5) 非选中 NIC 上来的包直接丢弃(但要回收 RxData)
  if (Socket != gOknUdpRxActiveSocket) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    return;
  }

  // 6) 懒创建 TX socket: 只创建一次, 绑定到选中的 SB handle
  //    注意: NotifyTransmit 改为 OknUdp4TxFreeHandler, 用于释放本次发送的内存
  if (NULL == gOknUdpSocketTransmit && gOknUdpRxActiveSbHandle != NULL) {


    EFI_STATUS TxStatus;

    TxStatus = OknCreateUdp4SocketByServiceBindingHandle(gOknUdpRxActiveSbHandle,
                                                         &TxCfg,
                                                         OknUdp4NullHandler,    // Tx socket 不需要 Receive 回调
                                                         OknUdp4TxFreeHandler,  // TX 完成释放资源
                                                         &gOknUdpSocketTransmit);

    if (TRUE == EFI_ERROR(TxStatus)) {
      Print(L"[OKN_UDP_ERROR] Create TX socket failed: %r\n", TxStatus);
      gOknUdpSocketTransmit = NULL;
    }
    else {
      // 初始化 TX 内存追踪字段(需要你在 UDP4_SOCKET 结构体中加入这些字段)
      gOknUdpSocketTransmit->TxPayload    = NULL;
      gOknUdpSocketTransmit->TxSession    = NULL;
      gOknUdpSocketTransmit->TxInProgress = FALSE;
    }
  }

  if (NULL == gOknUdpSocketTransmit) {
    Print(L"[OKN_UDP_ERROR] Create TX socket Success, but is NULL??!!\n");
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

  // 7) 若上一次发送还未完成(Token 仍挂起), 为避免覆盖 Token/内存, 直接丢弃本次回复
  if (gOknUdpSocketTransmit->TxInProgress) {
    gBS->SignalEvent(RxData->RecycleSignal);
    Socket->TokenReceive.Packet.RxData = NULL;
    Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
    return;
  }

#ifdef OKN_UDP_DBG
  /**
   * 在收到包后、Parse 之前打印: 来源 IP/端口 + payload 长度 + 前 64 字符
   */
  Print(L"[OKN_RX] from ");
  OknPrintIpv4A(RxData->UdpSession.SourceAddress);
  Print(L":%u  len=%u  frag0=%u  head=\"",
        (UINTN)RxData->UdpSession.SourcePort,
        (UINTN)RxData->DataLength,
        (UINTN)RxData->FragmentTable[0].FragmentLength);

  Print(L"\"\n");
  OknDumpAsciiN(RxData->FragmentTable[0].FragmentBuffer, RxData->FragmentTable[0].FragmentLength, 64);
#endif  // OKN_UDP_DBG

  // 8) 解析 JSON
  cJSON *Tree = cJSON_ParseWithLength((CONST CHAR8 *)RxData->FragmentTable[0].FragmentBuffer,
                                      RxData->FragmentTable[0].FragmentLength);

  if (Tree != NULL) {
    // 9) 设置 JsonHandler 上下文: 确保 areyouok 填的是"当前 NIC"的 MAC/IP
    gOknJsonCtxSocket = Socket;
    OknMT_DispatchJsonCmd(gOknMtProtoPtr, Tree, &gOknTestReset);
    gOknJsonCtxSocket = NULL;

    // 10) 生成响应 JSON(cJSON_PrintUnformatted 返回需要 cJSON_free 的内存)
    CHAR8 *JsonStr = cJSON_PrintUnformatted(Tree);
    if (JsonStr != NULL) {
      UINT32 JsonStrLen = (UINT32)AsciiStrLen(JsonStr);  // 注意 坑位: JsonStrLen长度是了少了'\0';

#ifdef OKN_UDP_DBG
      Print(L"[OKN_TX_PRE] resp_len=%u\n", (UINTN)JsonStrLen);
      if (JsonStrLen > OKN_UDP_SAFE_PAYLOAD) {
        Print(L"[OKN_TX_WARN] resp too big (%u), DF=%d may fail\n", (UINTN)JsonStrLen, TxCfg.DoNotFragment);
      }
#endif  // OKN_UDP_DBG

      /**
       * 关键: Transmit 是异步的, 不能直接把 JsonStr 指针塞给 TxData 再立刻释放, 所以这里复制一份到 Pool, TX
       * 完成回调里释放;
       * +1 是因为JsonStrLen是纯字符, 末尾是没有\0的, 如果不+1, 下面的应用就很危险:
       * Print(L"%a", Payload) / 传给需要 NUL 结尾的 API就会越界读;
       */
      VOID *Payload = AllocateZeroPool(JsonStrLen + 1);  // 自动补零

      cJSON_free(JsonStr);

      if (Payload != NULL && JsonStrLen > 0) {
        CopyMem(Payload, JsonStr, JsonStrLen);  // 注意: TxData->FragmentLength 仍然是 JsonStrLen(不带 '\0')

        EFI_UDP4_SESSION_DATA *Sess = AllocateZeroPool(sizeof(EFI_UDP4_SESSION_DATA));
        if (Sess != NULL) {
          Sess->DestinationAddress = RxData->UdpSession.SourceAddress;
          Sess->DestinationPort    = RxData->UdpSession.SourcePort;
          Sess->SourceAddress      = Socket->NicIp;
          Sess->SourcePort         = OKN_STATION_UDP_PORT;

#ifdef OKN_UDP_DBG
          Print(L"[OKN_TX_SESS] dst=");
          OknPrintIpv4A(Sess->DestinationAddress);
          Print(L":%u  src=", (UINTN)Sess->DestinationPort);
          OknPrintIpv4A(Sess->SourceAddress);
          Print(L":%u\n", (UINTN)Sess->SourcePort);
#endif  // OKN_UDP_DBG

          EFI_UDP4_TRANSMIT_DATA *TxData = gOknUdpSocketTransmit->TokenTransmit.Packet.TxData;
          ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
          TxData->UdpSessionData                  = Sess;
          TxData->DataLength                      = JsonStrLen;
          TxData->FragmentCount                   = 1;
          TxData->FragmentTable[0].FragmentLength = JsonStrLen; // 这个是不包含\0的, 加了也不会被发送
          TxData->FragmentTable[0].FragmentBuffer = Payload;

          // 记录待释放资源, TX 完成回调释放
          gOknUdpSocketTransmit->TxPayload    = Payload;
          gOknUdpSocketTransmit->TxSession    = Sess;
          gOknUdpSocketTransmit->TxInProgress = TRUE;

          Status =
              gOknUdpSocketTransmit->Udp4->Transmit(gOknUdpSocketTransmit->Udp4, &gOknUdpSocketTransmit->TokenTransmit);
          Print(L"[OKN_TX] Transmit() -> %r\n", Status);
          if (TRUE == EFI_ERROR(Status)) {
            Print(L"[OKN_TX_ERR] TxData=%p Payload=%p Sess=%p\n",
                  gOknUdpSocketTransmit->TokenTransmit.Packet.TxData,
                  Payload,
                  Sess);
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

      // 在最后释放 JsonStr;
      cJSON_free(JsonStr);
    }  // if (JsonStr != NULL)

    cJSON_Delete(Tree);
  }

  // 11) 回收 RxData 并重新投递 Receive
  gBS->SignalEvent(RxData->RecycleSignal);
  Socket->TokenReceive.Packet.RxData = NULL;
  Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);

  return;
}

// TX 完成后释放本次发送申请的内存, 避免泄漏
VOID EFIAPI OknUdp4TxFreeHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  UDP4_SOCKET *Sock = (UDP4_SOCKET *)Context;
  if (NULL == Sock) {
    return;
  }

#ifdef OKN_UDP_DBG
  Print(L"[OKN_TX_DONE] status=%r payload=%p sess=%p\n", Sock->TokenTransmit.Status, Sock->TxPayload, Sock->TxSession);
#endif  // OKN_UDP_DBG

  // 释放本次发送的 Payload(我们在 ReceiveHandler 里用 AllocateCopyPool 分配)
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

VOID EFIAPI OknUdp4NullHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  return;
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
      NULL == Socket) {
    Print(L"[OKN_UDP_ERROR] INVALID PARAMETER, Udp4ServiceBindingHandle/ConfigData/NotifyReceive/NotifyTransmit is "
          L"NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  *Socket     = NULL;
  Udp4Service = NULL;

  Status = gBS->HandleProtocol(Udp4ServiceBindingHandle, &gEfiUdp4ServiceBindingProtocolGuid, (VOID **)&Udp4Service);
  if (TRUE == EFI_ERROR(Status) || NULL == Udp4Service) {
    Print(L"[OKN_UDP_ERROR] Queries a Udp4Service handle failed: %r\n", Status);
    return EFI_NOT_FOUND;
  }

  *Socket = AllocateZeroPool(sizeof(UDP4_SOCKET));
  if (NULL == *Socket) {
    Print(L"[OKN_UDP_ERROR] AllocateZeroPool() for UDP4_SOCKET failed\n");
    return EFI_OUT_OF_RESOURCES;
  }

  (*Socket)->ServiceBindingHandle = Udp4ServiceBindingHandle;
  (*Socket)->ServiceBinding       = Udp4Service;

  Status = Udp4Service->CreateChild(Udp4Service, &(*Socket)->ChildHandle);

  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4Service CreateChild failed: %r\n", Status);
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
  if (TRUE == EFI_ERROR(Status) || NULL == (*Socket)->Udp4) {
    Print(L"[OKN_UDP_ERROR] Open Udp4 Protocol failed: %r\n", Status);
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  (*Socket)->ConfigData = *ConfigData;
  Status                = (*Socket)->Udp4->Configure((*Socket)->Udp4, ConfigData);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4 Configure failed: %r\n", Status);
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
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4 CreateEvent for NotifyReceive failed: %r\n", Status);
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  (*Socket)->TokenTransmit.Packet.TxData = AllocateZeroPool(sizeof(EFI_UDP4_TRANSMIT_DATA));
  if (NULL == (*Socket)->TokenTransmit.Packet.TxData) {
    Print(L"[OKN_UDP_ERROR] AllocateZeroPool for TxData failed: %r\n", Status);
    gBS->CloseEvent((*Socket)->TokenReceive.Event);
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyTransmit, *Socket, &(*Socket)->TokenTransmit.Event);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4 CreateEvent for NotifyTransmit failed: %r\n", Status);
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
 * 每个 NIC 创建一个 RX socket(多个)
 */
EFI_STATUS OknStartUdp4ReceiveOnAllNics(IN EFI_UDP4_CONFIG_DATA *RxCfg)
{
  EFI_STATUS                   Status;
  EFI_HANDLE                  *Handles;
  UINTN                        HandleNum;
  EFI_USB_IO_PROTOCOL         *UsbIo;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;

  Print(L"[OKN_UDP] Enter OknStartUdp4ReceiveOnAllNics()\n");

  if (RxCfg == NULL) {
    Print(L"RxCfg == NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  Handles              = NULL;
  HandleNum            = 0;
  gOknUdpRxSocketCount = 0;

  Print(L"[OKN_UDP] LocateHandleBuffer(Udp4ServiceBinding) ...\n");
  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUdp4ServiceBindingProtocolGuid, NULL, &HandleNum, &Handles);
  Print(L"[OKN_UDP] LocateHandleBuffer: %r, HandleNum=%u\n", Status, (UINT32)HandleNum);
  if (EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] LocateHandleBuffer() failed\n");
    return Status;
  }

  Print(L"[OKN_UDP] RxCfg: UseDefault=%u AcceptBroadcast=%u AllowDupPort=%u StationPort=%u RemotePort=%u\n",
        (UINT32)RxCfg->UseDefaultAddress,
        (UINT32)RxCfg->AcceptBroadcast,
        (UINT32)RxCfg->AllowDuplicatePort,
        (UINT32)RxCfg->StationPort,
        (UINT32)RxCfg->RemotePort);

  for (UINTN i = 0; i < HandleNum && gOknUdpRxSocketCount < MAX_UDP4_RX_SOCKETS; i++) {
    Print(L"[OKN_UDP] SB[%u] Handle=%p\n", (UINT32)i, Handles[i]);

    // 跳过 USB NIC, 不跳过是不是也行??
    UsbIo  = NULL;
    Status = gBS->HandleProtocol(Handles[i], &gEfiUsbIoProtocolGuid, (VOID **)&UsbIo);
    if (!EFI_ERROR(Status)) {
      Print(L"[OKN_UDP]   Skip: USB NIC\n");
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
      Print(L"[OKN_UDP]   SNP MediaPresent=%u State=%u HwAddrSize=%u\n",
            (UINT32)Snp->Mode->MediaPresent,
            (UINT32)Snp->Mode->State,
            (UINT32)Snp->Mode->HwAddressSize);

      if (Snp->Mode->HwAddressSize >= 6) {
        Print(L"[OKN_UDP]   MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
              Snp->Mode->CurrentAddress.Addr[0],
              Snp->Mode->CurrentAddress.Addr[1],
              Snp->Mode->CurrentAddress.Addr[2],
              Snp->Mode->CurrentAddress.Addr[3],
              Snp->Mode->CurrentAddress.Addr[4],
              Snp->Mode->CurrentAddress.Addr[5]);
      }

      if (0 == Snp->Mode->MediaPresent) {
        Print(L"[OKN_UDP]   Warn: Skip for MediaPresent=0\n");
        continue;
      }
    }
    else {
      Print(L"[OKN_UDP]   SNP not found or no Mode (Status=%r)\n", Status);
      // 不强制 continue: 信步说有的平台 UDP4 SB handle 上不一定挂 SNP
    }

    // 先跑 DHCP, 让 UseDefaultAddress=TRUE 有 mapping
    DhcpStatus = OknEnsureDhcpIp4Ready(Handles[i], DhcpTimeoutMs, &Ip);
    if (TRUE == EFI_ERROR(DhcpStatus)) {
      Print(L"[OKN_UDP]   DHCP not ready: %r\n", DhcpStatus);
      if (RxCfg->UseDefaultAddress) {
        Print(L"[OKN_UDP]   Skip: UseDefaultAddress=TRUE requires DHCP mapping\n");
        continue;
      }
    }

    // 创建 socket
    UDP4_SOCKET *Sock = NULL;
    Status =
        OknCreateUdp4SocketByServiceBindingHandle(Handles[i], RxCfg, OknUdp4ReceiveHandler, OknUdp4NullHandler, &Sock);
    Print(L"[OKN_UDP]   CreateSock: %r, Sock=%p\n", Status, Sock);
    if (TRUE == EFI_ERROR(Status) || Sock == NULL) {
      Print(L"[OKN_UDP_ERROR]  Skip: CreateSock failed\n");
      continue;
    }

    // 保存 NIC 身份到 Sock(关键)
    do {
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
    } while (0);

    Sock->TokenReceive.Packet.RxData = NULL;
    Status                           = Sock->Udp4->Receive(Sock->Udp4, &Sock->TokenReceive);
    Print(L"[OKN_UDP]   Sock->Udp4->Receive(submit): %r\n", Status);
    if (TRUE == EFI_ERROR(Status)) {
      Print(L"[OKN_UDP_ERROR]  Receive() FAILED on SB handle %p: %r\n", Handles[i], Status);
      OknCloseUdp4Socket(Sock);
      continue;
    }

    gOknUdpRxSockets[gOknUdpRxSocketCount++] = Sock;
    Print(L"[OKN_UDP]   Added RX socket. Count=%u\n", (UINT32)gOknUdpRxSocketCount);
  }  // for() 结束

  if (Handles != NULL) {
    FreePool(Handles);
  }

  Print(L"[OKN_UDP] Exit OknStartUdp4ReceiveOnAllNics(): gOknUdpRxSocketCount=%u\n", (UINT32)gOknUdpRxSocketCount);
  gBS->Stall(2 * 1000 * 1000);
  return (gOknUdpRxSocketCount > 0) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

EFI_STATUS OknWaitForUdpNicBind(UINTN TimeoutMs)
{
  UINTN Elapsed = 0;
  UINTN Tick    = 0;

  Print(L"[OKN_UDP] OknWaitForUdpNicBind() start, timeout=%u ms\n", TimeoutMs);

  while (NULL == gOknUdpRxActiveSocket) {
    OknPollAllUdpSockets();
    gBS->Stall(10 * 1000);

    Elapsed += 10;
    Tick++;

    if (0 == (Tick % 100)) {  // 约 1 秒
      //   Print(L"[OKN_UDP] Waiting bind... Elapsed=%u ms Active=%p", Elapsed, gOknUdpRxActiveSocket);
    }

    if (TimeoutMs != 0 && Elapsed >= TimeoutMs) {
      Print(L"[OKN_UDP_WARNING] OknWaitForUdpNicBind() TIMEOUT\n");
      return EFI_TIMEOUT;
    }
  }

  Print(L"[OKN_UDP] Wait for Udp Nic bind DONE, Active=%p\n", gOknUdpRxActiveSocket);
  return EFI_SUCCESS;
}

VOID OknDumpNetProtoCounts(VOID)
{
  // clang-format off
  EFI_GUID Dhcp4SbGuid = EFI_DHCP4_SERVICE_BINDING_PROTOCOL_GUID;  // [NET] DHCP4 SB
  Print(L"[OKN_NET] SNP   handles         : %u\n", (UINT32)OknCountHandlesByProtocol(&gEfiSimpleNetworkProtocolGuid));
  Print(L"[OKN_NET] MNP   Service Binding : %u\n", (UINT32)OknCountHandlesByProtocol(&gEfiManagedNetworkServiceBindingProtocolGuid));
  Print(L"[OKN_NET] IP4   Service Binding : %u\n", (UINT32)OknCountHandlesByProtocol(&gEfiIp4ServiceBindingProtocolGuid));
  Print(L"[OKN_NET] UDP4  Service Binding : %u\n", (UINT32)OknCountHandlesByProtocol(&gEfiUdp4ServiceBindingProtocolGuid));
  Print(L"[OKN_NET] DHCP4 Service Binding : %u\n", (UINT32)OknCountHandlesByProtocol(&Dhcp4SbGuid));
  // clang-format on
  return;
}

VOID OknConnectAllSnpControllers(VOID)
{
  EFI_STATUS  Status;
  EFI_HANDLE *SnpHandles = NULL;
  UINTN       Count      = 0;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleNetworkProtocolGuid, NULL, &Count, &SnpHandles);
  Print(L"[OKN_NET] Locate SNP: %r, Count=%u\n", Status, (UINT32)Count);  // [NET] Locate SNP: Success, Count=6

  if (EFI_ERROR(Status) || 0 == Count) {
    return;
  }

  for (UINTN i = 0; i < Count; i++) {
    EFI_STATUS S = gBS->ConnectController(SnpHandles[i], NULL, NULL, TRUE);
    Print(L"[OKN_NET] ConnectController(SNP[%u]): %r\n", (UINT32)i, S);
  }

  FreePool(SnpHandles);
  SnpHandles = NULL;

  return;
}

/**
 * 这个函数暂时不知道有啥用, 待删除
 */
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

STATIC VOID OknPrintIpv4A(IN CONST EFI_IPv4_ADDRESS Ip)
{
  Print(L"%d.%d.%d.%d", Ip.Addr[0], Ip.Addr[1], Ip.Addr[2], Ip.Addr[3]);
}

STATIC VOID OknDumpAsciiN(IN CONST VOID *Buf, IN UINTN Len, IN UINTN Max)
{
  CONST UINT8 *p = (CONST UINT8 *)Buf;
  UINTN        n = (Len < Max) ? Len : Max;
  for (UINTN i = 0; i < n; i++) {
    UINT8 c = p[i];
    if (c >= 0x20 && c <= 0x7E) {
      Print(L"%c", c);
    }
    else {
      Print(L".");
    }
  }
}

STATIC VOID OknDisableUdpRxSocket(IN UDP4_SOCKET *Sock)
{
  if (NULL == Sock || NULL == Sock->Udp4) {
    return;
  }
  // Cancel the pending receive token to stop further callbacks.
  Sock->Udp4->Cancel(Sock->Udp4, &Sock->TokenReceive);
}

STATIC UINTN OknCountHandlesByProtocol(IN EFI_GUID *Guid)
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

STATIC VOID OknPollAllUdpSockets(VOID)
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

STATIC BOOLEAN OknIsZeroIp4(IN EFI_IPv4_ADDRESS *A)
{
  // UEFI 环境里 ASSERT 的收益很高; 能把"上游传错指针"这种问题提前暴露, 而不是拖到很后面才炸
  ASSERT(A != NULL);
  return (A->Addr[0] == 0 && A->Addr[1] == 0 && A->Addr[2] == 0 && A->Addr[3] == 0);
}

STATIC EFI_STATUS OknEnsureDhcpIp4Ready(EFI_HANDLE Handle, UINTN TimeoutMs, EFI_IPv4_ADDRESS *OutIp)
{
  EFI_STATUS                      Status;
  EFI_IP4_CONFIG2_PROTOCOL       *Ip4Cfg2 = NULL;
  EFI_IP4_CONFIG2_POLICY          Policy;
  EFI_IP4_CONFIG2_INTERFACE_INFO *IfInfo     = NULL;
  UINTN                           IfInfoSize = 0;

  Status = gBS->HandleProtocol(Handle, &gEfiIp4Config2ProtocolGuid, (VOID **)&Ip4Cfg2);
  if (TRUE == EFI_ERROR(Status) || NULL == Ip4Cfg2) {
    Print(L"[OKN_UDP_ERROR]   IP4Config2 not found: %r\n", Status);
    return Status;
  }

  // 1) 设为 DHCP
  Policy = Ip4Config2PolicyDhcp;
  Status = Ip4Cfg2->SetData(Ip4Cfg2, Ip4Config2DataTypePolicy, sizeof(Policy), &Policy);
  if (FALSE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP]   IP4Config2 Set Policy DHCP: %r\n", Status);
  }
  else {
    Print(L"[OKN_UDP_ERROR]  IP4Config2 Set Policy DHCP: %r\n", Status);
    return Status;
  }

  // 2) 轮询等待 DHCP 生效(InterfaceInfo.StationAddress != 0)
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
      if (FALSE == EFI_ERROR(Status) && IfInfo != NULL && !OknIsZeroIp4(&IfInfo->StationAddress)) {
        *OutIp = IfInfo->StationAddress;

        Print(L"[OKN_UDP]   DHCP Ready: %d.%d.%d.%d / %d.%d.%d.%d\n",
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

      if (IfInfo != NULL) {
        FreePool(IfInfo);
      }
    }

    gBS->Stall(100 * 1000);  // 100ms
    Waited += 100;
  }

  Print(L"[OKN_UDP_WARNING]  DHCP timeout (%u ms)\n", (UINT32)TimeoutMs);
  return EFI_TIMEOUT;
}
