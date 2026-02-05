#include <Library/OKN/OknMemTestLib.h>
#include <Library/OKN/OknUdp4Lib.h>
#include <Library/OKN/PortingLibs/cJSONLib.h>

/**
 * UDP4 Rx (OknUdp4ReceiveHandler)
 *    ├─ cJSON_ParseWithLength(udp_payload)  -> Tree
 *    ├─ OknMT_DispatchJsonCmd(Tree)                   -> 在 Tree 里加返回字段 / 改全局状态
 *    ├─ cJSON_PrintUnformatted(Tree)        -> JSON response string
 *    └─ UDP4 Tx (sOknUdpXferEdp->Transmit) -> 回发给上位机
 */

#define OKN_UDP_DBG

/**
 * 以太网常见 MTU=1500(不含二层头), IPv4 + UDP 头共 28 字节:
 * 最大不分片 UDP payload ≈ 1500 - 20 - 8 = 1472
 * 再留一点余量(VLAN tag、隧道、不同驱动/栈差异、某些环境 MTU 不是 1500), 工程上就常取 1400 或 1200.
 * 所以 1400 的意义是: 尽量保证"不走 IP 分片", 提高稳定性.
 */
#define OKN_UDP_SAFE_PAYLOAD 1400

OKN_UDP4_ENDPOINT *gOknJsonCtxUdpEdp = NULL;

UINTN sOknUdpEdpOnlineCnt = 0;
/**
 * 保存 `sOknUdpActiveEdpSbEfiHandle` 的意义是:以后要创建 TX child / 其他 child 时,
 * 直接锁定到这张 NIC 的 ServiceBinding.
 *
 * sOknUdpActiveEdpSbEfiHandle 与 sOknUdpActiveEdp 的关系有点父对象与子对象之间的关系
 */
STATIC EFI_HANDLE         sOknUdpActiveEdpSbEfiHandle = NULL;
STATIC OKN_UDP4_ENDPOINT *sOknUdpXferEdp              = NULL;
// sOknUdpActiveEdp用来表示是找到那个可以通信的NIC
STATIC OKN_UDP4_ENDPOINT *sOknUdpActiveEdp = NULL;
STATIC OKN_UDP4_ENDPOINT *sOknUdpEpdArr[MAX_UDP4_RX_SOCKETS];  // 只用于第一次匹配在线的ENDPOINT

STATIC VOID       OknPrintMac(IN CONST EFI_MAC_ADDRESS *Mac, IN UINT32 HwAddrSize);
STATIC VOID       OknPrintIpv4A(IN CONST EFI_IPv4_ADDRESS Ip);
STATIC VOID       OknDumpAsciiN(IN CONST VOID *Buf, IN UINTN Len, IN UINTN Max);
STATIC VOID       OknDisableUdpRxSocket(IN OKN_UDP4_ENDPOINT *UdpEdp);
STATIC UINTN      OknCountHandlesByProtocol(IN EFI_GUID *Guid);
STATIC VOID       OknPollAllUdpSockets(VOID);
STATIC BOOLEAN    OknIsZeroIp4(IN EFI_IPv4_ADDRESS *A);
STATIC EFI_STATUS OknEnsureDhcpIp4Ready(EFI_HANDLE Handle, UINTN TimeoutMs, EFI_IPv4_ADDRESS *OutIp);
STATIC EFI_STATUS OknCreateUdpEndpointByServiceBindingHandle(IN EFI_HANDLE            Udp4ServiceBindingHandle,
                                                             IN EFI_UDP4_CONFIG_DATA *ConfigData,
                                                             IN EFI_EVENT_NOTIFY      NotifyReceive,
                                                             IN EFI_EVENT_NOTIFY      NotifyTransmit,
                                                             OUT OKN_UDP4_ENDPOINT  **UdpEdp);
STATIC EFI_STATUS OknCloseUdp4Socket(IN OKN_UDP4_ENDPOINT *UdpEdp);

RETURN_STATUS EFIAPI OknUdp4SocketLibConstructor(VOID)
{
  EFI_STATUS           Status;
  UINTN                HandleNum;
  EFI_HANDLE          *Udp4Handles;
  EFI_USB_IO_PROTOCOL *UsbIo;

  EFI_HANDLE DefaultUdp4SbHandle = NULL;

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

    DefaultUdp4SbHandle = Udp4Handles[i];
    break;
  }

  if (Udp4Handles != NULL) {
    FreePool(Udp4Handles);
    Udp4Handles = NULL;
  }

  return (DefaultUdp4SbHandle != NULL) ? EFI_SUCCESS : EFI_NOT_FOUND;
}

VOID EFIAPI OknUdp4ReceiveHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  EFI_STATUS             Status;
  OKN_UDP4_ENDPOINT     *OknUdpEdpInstance;
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

  OknUdpEdpInstance = (OKN_UDP4_ENDPOINT *)Context;
  if (NULL == OknUdpEdpInstance || NULL == OknUdpEdpInstance->Udp4Proto) {
    Print(L"[OKN_UDP_ERROR] OknUdpEdpInstance or OknUdpEdpInstance->Udp4Proto is NULL.\n");
    return;
  }

  RxData = OknUdpEdpInstance->TokenReceive.Packet.RxData;

  // 1) Token 完成但没有数据: 按 UEFI UDP4 约定, 重新投递 Receive()
  if (NULL == RxData) {
    Print(L"[OKN_UDP_WARNING] RxData is NULL, Status=%r\n", OknUdpEdpInstance->TokenReceive.Status);
    OknUdpEdpInstance->TokenReceive.Packet.RxData = NULL;
    Status = OknUdpEdpInstance->Udp4Proto->Receive(OknUdpEdpInstance->Udp4Proto, &OknUdpEdpInstance->TokenReceive);
    Print(L"[OKN_UDP_DEBUG] resubmit Receive: %r\n", Status);
    return;
  }

  // 2) 若 Receive 被 Abort: 回收 RxData 并退出
  if (EFI_ABORTED == OknUdpEdpInstance->TokenReceive.Status) {
    Print(L"[OKN_UDP_WARNING] OknUdpEdpInstance TokenReceive Status is EFI_ABORTED\n");
    gBS->SignalEvent(RxData->RecycleSignal);
    OknUdpEdpInstance->TokenReceive.Packet.RxData = NULL;
    return;
  }

  // 3) 空包: 回收并重新投递
  if (0 == RxData->DataLength || 0 == RxData->FragmentCount || NULL == RxData->FragmentTable[0].FragmentBuffer ||
      0 == RxData->FragmentTable[0].FragmentLength) {
    gBS->SignalEvent(RxData->RecycleSignal);
    OknUdpEdpInstance->TokenReceive.Packet.RxData = NULL;
    OknUdpEdpInstance->Udp4Proto->Receive(OknUdpEdpInstance->Udp4Proto, &OknUdpEdpInstance->TokenReceive);
    return;
  }

  // 4) 首包绑定: 选择第一个收到包的 NIC, 其它 RX socket 全部禁用
  if (NULL == sOknUdpActiveEdp) {
    sOknUdpActiveEdp            = OknUdpEdpInstance;
    sOknUdpActiveEdpSbEfiHandle = OknUdpEdpInstance->ServiceBindingHandle;

    for (UINTN i = 0; i < sOknUdpEdpOnlineCnt; i++) {
      if (sOknUdpEpdArr[i] != NULL && sOknUdpEpdArr[i] != OknUdpEdpInstance) {
        OknDisableUdpRxSocket(sOknUdpEpdArr[i]);
      }
    }
  }

  // 5) 非选中 NIC 上来的包直接丢弃(但要回收 RxData)
  if (OknUdpEdpInstance != sOknUdpActiveEdp) {
    gBS->SignalEvent(RxData->RecycleSignal);
    OknUdpEdpInstance->TokenReceive.Packet.RxData = NULL;
    return;
  }

  /**
   * 6) 懒创建 TX socket: 只创建一次, 绑定到选中的 SB handle,
   * 注意: NotifyTransmit 改为 OknUdp4TxFreeHandler, 用于释放本次发送的内存;
   *
   * TX endpoint 还没建(sOknUdpXferEdp == NULL);
   * 但已经选定了用哪张网卡(sOknUdpActiveEdpSbEfiHandle != NULL)
   */
  if (NULL == sOknUdpXferEdp && sOknUdpActiveEdpSbEfiHandle != NULL) {
    EFI_STATUS TxStatus;

    TxStatus = OknCreateUdpEndpointByServiceBindingHandle(sOknUdpActiveEdpSbEfiHandle,
                                                          &TxCfg,
                                                          OknUdp4NullHandler,    // Tx socket 不需要 Receive 回调
                                                          OknUdp4TxFreeHandler,  // TX 完成释放资源
                                                          &sOknUdpXferEdp);
    if (TRUE == EFI_ERROR(TxStatus)) {
      Print(L"[OKN_UDP_ERROR] Create TX socket failed: %r\n", TxStatus);
      sOknUdpXferEdp = NULL;
    }
    else {
      // 初始化 TX 内存追踪字段(需要你在 OKN_UDP4_ENDPOINT 结构体中加入这些字段)
      sOknUdpXferEdp->TxPayload    = NULL;
      sOknUdpXferEdp->TxSession    = NULL;
      sOknUdpXferEdp->TxInProgress = FALSE;
    }
  }

  if (NULL == sOknUdpXferEdp) {
    // 用??!!会有 warning: trigraph ??! ignored, use -trigraphs to enable [-Wtrigraphs]
    Print(L"[OKN_UDP_ERROR] Create TX socket Success, but is NULL ?!\n");
    gBS->SignalEvent(RxData->RecycleSignal);
    OknUdpEdpInstance->TokenReceive.Packet.RxData = NULL;
    OknUdpEdpInstance->Udp4Proto->Receive(OknUdpEdpInstance->Udp4Proto, &OknUdpEdpInstance->TokenReceive);
    return;
  }

  // 7) 若上一次发送还未完成(Token 仍挂起), 为避免覆盖 Token/内存, 直接丢弃本次回复
  if (sOknUdpXferEdp->TxInProgress) {
    gBS->SignalEvent(RxData->RecycleSignal);
    OknUdpEdpInstance->TokenReceive.Packet.RxData = NULL;
    OknUdpEdpInstance->Udp4Proto->Receive(OknUdpEdpInstance->Udp4Proto, &OknUdpEdpInstance->TokenReceive);
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
    gOknJsonCtxUdpEdp = OknUdpEdpInstance;
    OknMT_DispatchJsonCmd(gOknMtProtoPtr, Tree, &gOknTestReset);
    gOknJsonCtxUdpEdp = NULL;

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

      if (Payload != NULL && JsonStrLen > 0) {
        CopyMem(Payload, JsonStr, JsonStrLen);  // 注意: TxData->FragmentLength 仍然是 JsonStrLen(不带 '\0')

        EFI_UDP4_SESSION_DATA *Sess = AllocateZeroPool(sizeof(EFI_UDP4_SESSION_DATA));
        if (Sess != NULL) {
          Sess->DestinationAddress = RxData->UdpSession.SourceAddress;
          Sess->DestinationPort    = RxData->UdpSession.SourcePort;
          Sess->SourceAddress      = OknUdpEdpInstance->NicIp;
          Sess->SourcePort         = OKN_STATION_UDP_PORT;

#ifdef OKN_UDP_DBG
          Print(L"[OKN_TX_SESS] dst=");
          OknPrintIpv4A(Sess->DestinationAddress);
          Print(L":%u  src=", (UINTN)Sess->DestinationPort);
          OknPrintIpv4A(Sess->SourceAddress);
          Print(L":%u\n", (UINTN)Sess->SourcePort);
#endif  // OKN_UDP_DBG

          EFI_UDP4_TRANSMIT_DATA *TxData = sOknUdpXferEdp->TokenTransmit.Packet.TxData;
          ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
          TxData->UdpSessionData                  = Sess;
          TxData->DataLength                      = JsonStrLen;
          TxData->FragmentCount                   = 1;
          TxData->FragmentTable[0].FragmentLength = JsonStrLen;  // 这个是不包含\0的, 加了也不会被发送
          TxData->FragmentTable[0].FragmentBuffer = Payload;

          // 记录待释放资源, TX 完成回调释放
          sOknUdpXferEdp->TxPayload    = Payload;
          sOknUdpXferEdp->TxSession    = Sess;
          sOknUdpXferEdp->TxInProgress = TRUE;

          Status = sOknUdpXferEdp->Udp4Proto->Transmit(sOknUdpXferEdp->Udp4Proto, &sOknUdpXferEdp->TokenTransmit);
          Print(L"[OKN_TX] Transmit() -> %r\n", Status);
          if (TRUE == EFI_ERROR(Status)) {
            Print(L"[OKN_TX_ERR] TxData=%p Payload=%p Sess=%p\n",
                  sOknUdpXferEdp->TokenTransmit.Packet.TxData,
                  Payload,
                  Sess);
            // 发送失败: 立即释放并复位
            sOknUdpXferEdp->TxInProgress = FALSE;
            if (sOknUdpXferEdp->TxPayload) {
              FreePool(sOknUdpXferEdp->TxPayload);
              sOknUdpXferEdp->TxPayload = NULL;
            }
            if (sOknUdpXferEdp->TxSession) {
              FreePool(sOknUdpXferEdp->TxSession);
              sOknUdpXferEdp->TxSession = NULL;
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
  OknUdpEdpInstance->TokenReceive.Packet.RxData = NULL;
  OknUdpEdpInstance->Udp4Proto->Receive(OknUdpEdpInstance->Udp4Proto, &OknUdpEdpInstance->TokenReceive);

  return;
}

// TX 完成后释放本次发送申请的内存, 避免泄漏
VOID EFIAPI OknUdp4TxFreeHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  OKN_UDP4_ENDPOINT *OknUdpEdpInstance = (OKN_UDP4_ENDPOINT *)Context;
  if (NULL == OknUdpEdpInstance) {
    return;
  }

#ifdef OKN_UDP_DBG
  Print(L"[OKN_TX_DONE] status=%r payload=%p sess=%p\n",
        OknUdpEdpInstance->TokenTransmit.Status,
        OknUdpEdpInstance->TxPayload,
        OknUdpEdpInstance->TxSession);
#endif  // OKN_UDP_DBG

  // 释放本次发送的 Payload(我们在 ReceiveHandler 里用 AllocateCopyPool 分配)
  if (OknUdpEdpInstance->TxPayload != NULL) {
    FreePool(OknUdpEdpInstance->TxPayload);
    OknUdpEdpInstance->TxPayload = NULL;
  }

  // 释放本次发送的 SessionData
  if (OknUdpEdpInstance->TxSession != NULL) {
    FreePool(OknUdpEdpInstance->TxSession);
    OknUdpEdpInstance->TxSession = NULL;
  }

  OknUdpEdpInstance->TxInProgress = FALSE;
  return;
}

VOID EFIAPI OknUdp4NullHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  return;
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

  Handles             = NULL;
  HandleNum           = 0;
  sOknUdpEdpOnlineCnt = 0;

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

  for (UINTN i = 0; i < HandleNum && sOknUdpEdpOnlineCnt < MAX_UDP4_RX_SOCKETS; i++) {
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
    OKN_UDP4_ENDPOINT *UdpEdp = NULL;
    Status                    = OknCreateUdpEndpointByServiceBindingHandle(Handles[i],
                                                        RxCfg,
                                                        OknUdp4ReceiveHandler,
                                                        OknUdp4NullHandler,
                                                        &UdpEdp);
    Print(L"[OKN_UDP]   CreateSock: %r, UDP Endpoint=%p\n", Status, UdpEdp);
    if (TRUE == EFI_ERROR(Status) || NULL == UdpEdp) {
      Print(L"[OKN_UDP_ERROR]  Skip: CreateSock failed\n");
      continue;
    }

    // 保存 NIC 身份到 UdpEdp(关键)
    do {
      if (HasSnp && Snp->Mode && Snp->Mode->HwAddressSize >= 6) {
        CopyMem(UdpEdp->NicMac, Snp->Mode->CurrentAddress.Addr, 6);
      }
      else {
        ZeroMem(UdpEdp->NicMac, 6);
      }

      UdpEdp->NicIpValid = !EFI_ERROR(DhcpStatus);
      if (UdpEdp->NicIpValid) {
        UdpEdp->NicIp = Ip;
      }
      else {
        ZeroMem(&UdpEdp->NicIp, sizeof(UdpEdp->NicIp));
      }
    } while (0);

    UdpEdp->TokenReceive.Packet.RxData = NULL;
    Status                             = UdpEdp->Udp4Proto->Receive(UdpEdp->Udp4Proto, &UdpEdp->TokenReceive);
    Print(L"[OKN_UDP]   UdpEdp->Udp4Proto->Receive(submit): %r\n", Status);
    if (TRUE == EFI_ERROR(Status)) {
      Print(L"[OKN_UDP_ERROR]  Receive() FAILED on SB handle %p: %r\n", Handles[i], Status);
      OknCloseUdp4Socket(UdpEdp);
      continue;
    }

    sOknUdpEpdArr[sOknUdpEdpOnlineCnt++] = UdpEdp;
    Print(L"[OKN_UDP]   Added RX socket. Count=%u\n", (UINT32)sOknUdpEdpOnlineCnt);
  }  // for() 结束

  if (Handles != NULL) {
    FreePool(Handles);
  }

  Status = (sOknUdpEdpOnlineCnt > 0) ? EFI_SUCCESS : EFI_NOT_FOUND;
  Print(L"[OKN_UDP] Exit OknStartUdp4ReceiveOnAllNics: %r,  RxSockCnt=%u\n", Status, (UINT32)sOknUdpEdpOnlineCnt);

  return Status;
}

EFI_STATUS OknWaitForUdpNicBind(UINTN TimeoutMs)
{
  UINTN Elapsed = 0;
  UINTN Tick    = 0;

  Print(L"[OKN_UDP] OknWaitForUdpNicBind() start, timeout=%u ms\n", TimeoutMs);

  while (NULL == sOknUdpActiveEdp) {
    OknPollAllUdpSockets();
    gBS->Stall(10 * 1000);

    Elapsed += 10;
    Tick++;

    if (0 == (Tick % 100)) {  // 约 1 秒
      //   Print(L"[OKN_UDP] Waiting bind... Elapsed=%u ms Active=%p", Elapsed, sOknUdpActiveEdp);
    }

    if (TimeoutMs != 0 && Elapsed >= TimeoutMs) {
      Print(L"[OKN_UDP_WARNING] OknWaitForUdpNicBind() TIMEOUT\n");
      return EFI_TIMEOUT;
    }
  }

  Print(L"[OKN_UDP] Wait for Udp Nic bind DONE, Active=%p\n", sOknUdpActiveEdp);
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

STATIC VOID OknDisableUdpRxSocket(IN OKN_UDP4_ENDPOINT *UdpEdp)
{
  if (NULL == UdpEdp || NULL == UdpEdp->Udp4Proto) {
    return;
  }
  // Cancel the pending receive token to stop further callbacks.
  UdpEdp->Udp4Proto->Cancel(UdpEdp->Udp4Proto, &UdpEdp->TokenReceive);
  return;
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
  for (UINTN i = 0; i < sOknUdpEdpOnlineCnt; ++i) {
    if (sOknUdpEpdArr[i] != NULL && sOknUdpEpdArr[i]->Udp4Proto != NULL) {
      sOknUdpEpdArr[i]->Udp4Proto->Poll(sOknUdpEpdArr[i]->Udp4Proto);
    }
  }

  if (sOknUdpXferEdp != NULL && sOknUdpXferEdp->Udp4Proto != NULL) {
    sOknUdpXferEdp->Udp4Proto->Poll(sOknUdpXferEdp->Udp4Proto);
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

STATIC EFI_STATUS OknCreateUdpEndpointByServiceBindingHandle(IN EFI_HANDLE            Udp4ServiceBindingHandle,
                                                             IN EFI_UDP4_CONFIG_DATA *ConfigData,
                                                             IN EFI_EVENT_NOTIFY      NotifyReceive,
                                                             IN EFI_EVENT_NOTIFY      NotifyTransmit,
                                                             OUT OKN_UDP4_ENDPOINT  **UdpEdp)
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL *Udp4Service;

  if (NULL == Udp4ServiceBindingHandle || NULL == ConfigData || NULL == NotifyReceive || NULL == NotifyTransmit ||
      NULL == UdpEdp) {
    Print(L"[OKN_UDP_ERROR] INVALID PARAMETER, Udp4ServiceBindingHandle/ConfigData/NotifyReceive/NotifyTransmit is "
          L"NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  *UdpEdp     = NULL;
  Udp4Service = NULL;

  Status = gBS->HandleProtocol(Udp4ServiceBindingHandle, &gEfiUdp4ServiceBindingProtocolGuid, (VOID **)&Udp4Service);
  if (TRUE == EFI_ERROR(Status) || NULL == Udp4Service) {
    Print(L"[OKN_UDP_ERROR] Queries a Udp4Service handle failed: %r\n", Status);
    return EFI_NOT_FOUND;
  }

  *UdpEdp = AllocateZeroPool(sizeof(OKN_UDP4_ENDPOINT));
  if (NULL == *UdpEdp) {
    Print(L"[OKN_UDP_ERROR] AllocateZeroPool() for OKN_UDP4_ENDPOINT failed\n");
    return EFI_OUT_OF_RESOURCES;
  }

  (*UdpEdp)->ServiceBindingHandle = Udp4ServiceBindingHandle;
  (*UdpEdp)->ServiceBinding       = Udp4Service;

  Status = Udp4Service->CreateChild(Udp4Service, &(*UdpEdp)->ChildHandle);

  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4Service CreateChild failed: %r\n", Status);
    FreePool(*UdpEdp);
    *UdpEdp = NULL;
    return Status;
  }

  Status = gBS->OpenProtocol((*UdpEdp)->ChildHandle,
                             &gEfiUdp4ProtocolGuid,
                             (VOID **)&(*UdpEdp)->Udp4Proto,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (TRUE == EFI_ERROR(Status) || NULL == (*UdpEdp)->Udp4Proto) {
    Print(L"[OKN_UDP_ERROR] Open Udp4 Protocol failed: %r\n", Status);
    Udp4Service->DestroyChild(Udp4Service, (*UdpEdp)->ChildHandle);
    FreePool(*UdpEdp);
    *UdpEdp = NULL;
    return Status;
  }

  (*UdpEdp)->ConfigData = *ConfigData;
  Status                = (*UdpEdp)->Udp4Proto->Configure((*UdpEdp)->Udp4Proto, ConfigData);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4 Configure failed: %r\n", Status);
    Udp4Service->DestroyChild(Udp4Service, (*UdpEdp)->ChildHandle);
    FreePool(*UdpEdp);
    *UdpEdp = NULL;
    return Status;
  }

  // IMPORTANT:
  // For EFI_UDP4_PROTOCOL.Receive(), the caller must submit the token with
  // Token.Packet.RxData. The UDP4 implementation will allocate an
  // EFI_UDP4_RECEIVE_DATA buffer and return it via Token.Packet.RxData on
  // completion. The caller must recycle the buffer by signaling
  // RxData->RecycleSignal.
  (*UdpEdp)->TokenReceive.Packet.RxData = NULL;

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyReceive, *UdpEdp, &(*UdpEdp)->TokenReceive.Event);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4 CreateEvent for NotifyReceive failed: %r\n", Status);
    Udp4Service->DestroyChild(Udp4Service, (*UdpEdp)->ChildHandle);
    FreePool(*UdpEdp);
    *UdpEdp = NULL;
    return Status;
  }

  (*UdpEdp)->TokenTransmit.Packet.TxData = AllocateZeroPool(sizeof(EFI_UDP4_TRANSMIT_DATA));
  if (NULL == (*UdpEdp)->TokenTransmit.Packet.TxData) {
    Print(L"[OKN_UDP_ERROR] AllocateZeroPool for TxData failed: %r\n", Status);
    gBS->CloseEvent((*UdpEdp)->TokenReceive.Event);
    Udp4Service->DestroyChild(Udp4Service, (*UdpEdp)->ChildHandle);
    FreePool(*UdpEdp);
    *UdpEdp = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyTransmit, *UdpEdp, &(*UdpEdp)->TokenTransmit.Event);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UDP_ERROR] Udp4 CreateEvent for NotifyTransmit failed: %r\n", Status);
    FreePool((*UdpEdp)->TokenTransmit.Packet.TxData);
    gBS->CloseEvent((*UdpEdp)->TokenReceive.Event);
    Udp4Service->DestroyChild(Udp4Service, (*UdpEdp)->ChildHandle);
    FreePool(*UdpEdp);
    *UdpEdp = NULL;
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC EFI_STATUS OknCloseUdp4Socket(IN OKN_UDP4_ENDPOINT *UdpEdp)
{
  EFI_STATUS Status;

  if (NULL == UdpEdp) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  if (UdpEdp->Udp4Proto != NULL) {
    Status = UdpEdp->Udp4Proto->Cancel(UdpEdp->Udp4Proto, NULL);
    Status |= UdpEdp->Udp4Proto->Configure(UdpEdp->Udp4Proto, NULL);
  }

  if (UdpEdp->TokenReceive.Event != NULL) {
    Status |= gBS->CloseEvent(UdpEdp->TokenReceive.Event);
  }
  if (UdpEdp->TokenTransmit.Event != NULL) {
    Status |= gBS->CloseEvent(UdpEdp->TokenTransmit.Event);
  }

  if (UdpEdp->ServiceBinding != NULL && UdpEdp->ChildHandle != NULL) {
    Status |= UdpEdp->ServiceBinding->DestroyChild(UdpEdp->ServiceBinding, UdpEdp->ChildHandle);
  }

  // NOTE: Receive buffers are owned by UDP4 and must be recycled via
  // RxData->RecycleSignal. Do not FreePool() them here.
  if (UdpEdp->TokenTransmit.Packet.TxData != NULL) {
    FreePool(UdpEdp->TokenTransmit.Packet.TxData);
  }

  FreePool(UdpEdp);
  return Status;
}
