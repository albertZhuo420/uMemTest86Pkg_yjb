/**
 * @file
 *
 */

#include <Library/OKN/OknDdr4SpdLib.h>
#include <Library/OKN/OknMemTestLib.h>
#include <Library/OKN/OknUdp4SocketLib.h>
#include <Protocol/Smbios.h>
#include <uMemTest86.h>

/**
 * UDP4 Rx (Udp4ReceiveHandler)
 *    ├─ cJSON_ParseWithLength(udp_payload)  -> Tree
 *    ├─ JsonHandler(Tree)                   -> 在 Tree 里加返回字段 / 改全局状态
 *    ├─ cJSON_PrintUnformatted(Tree)        -> JSON response string
 *    └─ UDP4 Tx (gOknUdpSocketTransmit->Transmit) -> 回发给上位机
 */

#if 1  // OKN 前置定义区域 ON
/**GLOBAL/STATIC变量 前置定义区域********************************************************/
UINTN                gOknLastPercent;  // 用在uMemTest86Pkg/Ui.c 文件中: mLastPercent;
BOOLEAN              gOKnSkipWaiting = FALSE;
BOOLEAN              gOknTestStart   = FALSE;
BOOLEAN              gOknTestPause   = FALSE;
EFI_RESET_TYPE       gOknTestReset   = EfiResetPlatformSpecific;
OKN_TEST_STATUS_TYPE gOknTestStatus  = OKN_TST_Unknown;
INT8                 gOknMT86TestID  = -1;  // 这个是MT86真实的TestID [0 ... 15]

  #define CMD_ENTRY(_name, _handler) {(_name), (_handler)}

STATIC CONST OKN_MT_CMD_DISPATCH gOknCmdTable[] = {
    // ---- 新命令名（对外只暴露这些）----
    CMD_ENTRY("SetAmtConfig", OknMT_ProcessJsonCmd_SetAmtConfig),
    CMD_ENTRY("MT86_Status", OknMT_ProcessJsonCmd_MT86Status),
    CMD_ENTRY("MT86_Start", OknMT_ProcessJsonCmd_MT86Start),
    CMD_ENTRY("MT86_Abort", OknMT_ProcessJsonCmd_MT86Abort),
    CMD_ENTRY("Reply_9527", OknMT_ProcessJsonCmd_Reply9527),
    CMD_ENTRY("HW_Info", OknMT_ProcessJsonCmd_HwInfo),
    CMD_ENTRY("ReadSPD", OknMT_ProcessJsonCmd_ReadSPD),
    CMD_ENTRY("GetMemConfig", OknMT_ProcessJsonCmd_GetMemConfig),
    CMD_ENTRY("GetMemConfigReal", OknMT_ProcessJsonCmd_GetMemConfigReal),
    CMD_ENTRY("SetMemConfig", OknMT_ProcessJsonCmd_SetMemConfig),
    CMD_ENTRY("ResetSystem", OknMT_ProcessJsonCmd_ResetSystem),
    // ---- 兼容旧命令名（保留一段时间再删除）----
    CMD_ENTRY("amtStart", OknMT_ProcessJsonCmd_SetAmtConfig),
    CMD_ENTRY("testStatus", OknMT_ProcessJsonCmd_MT86Status),
    CMD_ENTRY("testStart", OknMT_ProcessJsonCmd_MT86Start),
    CMD_ENTRY("testStop", OknMT_ProcessJsonCmd_MT86Abort),
    CMD_ENTRY("areyouok", OknMT_ProcessJsonCmd_Reply9527),
    CMD_ENTRY("connect", OknMT_ProcessJsonCmd_HwInfo),
    CMD_ENTRY("readSPD", OknMT_ProcessJsonCmd_ReadSPD),
    CMD_ENTRY("testConfigGet", OknMT_ProcessJsonCmd_GetMemConfig),
    CMD_ENTRY("testConfigActive", OknMT_ProcessJsonCmd_GetMemConfigReal),
    CMD_ENTRY("testConfigSet", OknMT_ProcessJsonCmd_SetMemConfig),
    CMD_ENTRY("reset", OknMT_ProcessJsonCmd_ResetSystem),
};
/**STATIC函数 前置定义区域1**************************************************************/
STATIC UINTN      OknCmdTableCount(VOID);
STATIC EFI_STATUS OknMT_DispatchJsonCmd(IN OKN_MEMORY_TEST_PROTOCOL *Proto,
                                        IN OUT cJSON                *Tree,
                                        OUT EFI_RESET_TYPE          *OutResetType,
                                        OUT BOOLEAN                 *OutNeedReset);
STATIC EFI_STATUS OKnSmbiosGetOptionalStringByIndex(IN CONST CHAR8   *OptionalStrStart,
                                                    IN UINT8          Index,
                                                    OUT CONST CHAR8 **OutStr);
/**STATIC函数 前置定义区域2**************************************************************/
STATIC EFI_STATUS OknMT_ProcessJsonCmd_SetAmtConfig(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Status(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Abort(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Start(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_Reply9527(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_HwInfo(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_ReadSPD(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfig(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfigReal(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_SetMemConfig(OKN_MT_CMD_CTX *Ctx);
STATIC EFI_STATUS OknMT_ProcessJsonCmd_ResetSystem(OKN_MT_CMD_CTX *Ctx);
/**STATIC函数 前置定义区域3**************************************************************/
STATIC EFI_STATUS Cmd_SetAmtConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);
STATIC EFI_STATUS Cmd_MT86Status(OUT cJSON *pJsTree);
STATIC VOID       Cmd_MT86Abort(VOID);
STATIC EFI_STATUS Cmd_MT86Start(IN cJSON *pJsTree);
STATIC EFI_STATUS Cmd_Reply9527(OUT cJSON *pJsTree);
STATIC EFI_STATUS Cmd_HwInfo(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
STATIC EFI_STATUS Cmd_ReadSPD(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree);
STATIC EFI_STATUS Cmd_GetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
STATIC EFI_STATUS Cmd_GetMemConfigReal(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
STATIC EFI_STATUS Cmd_SetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);
STATIC EFI_STATUS Cmd_ResetSystem(IN OUT cJSON *pTree, OUT EFI_RESET_TYPE *pResetType);
/*************************************************************************************************/
#endif  // OKN 前置定义区域 OFF

VOID EFIAPI Udp4ReceiveHandler(IN EFI_EVENT Event, IN VOID *Context)
{
  UDP4_SOCKET           *Socket;
  EFI_UDP4_RECEIVE_DATA *RxData;
  EFI_STATUS             Status;

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
  if (RxData->DataLength == 0 || RxData->FragmentCount == 0 || RxData->FragmentTable[0].FragmentBuffer == NULL ||
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
        TRUE,            // AcceptBroadcast
        FALSE,           // AcceptPromiscuous
        FALSE,           // AcceptAnyPort
        TRUE,            // AllowDuplicatePort
        0,               // TypeOfService
        16,              // TimeToLive
        TRUE,            // DoNotFragment
        0,               // ReceiveTimeout
        0,               // TransmitTimeout
        TRUE,            // UseDefaultAddress
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
        (EFI_EVENT_NOTIFY)Udp4NullHandler,    // Tx socket 不需要 Receive 回调
        (EFI_EVENT_NOTIFY)Udp4TxFreeHandler,  // TX 完成释放资源: Udp4TxFreeHandler
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
            // 发送失败：立即释放并复位
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

VOID EFIAPI Udp4NullHandler(IN EFI_EVENT Event, IN VOID *Context) { }

#if 1  // STATIC函数实现区域 ON
STATIC UINTN OknCmdTableCount(VOID)
{
  return sizeof(gOknCmdTable) / sizeof(gOknCmdTable[0]);
}

STATIC EFI_STATUS OknMT_DispatchJsonCmd(IN OKN_MEMORY_TEST_PROTOCOL *Proto,
                                        IN OUT cJSON                *Tree,
                                        OUT EFI_RESET_TYPE          *OutResetType,
                                        OUT BOOLEAN                 *OutNeedReset)
{
  if (Tree == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (OutNeedReset) {
    *OutNeedReset = FALSE;
  }
  if (OutResetType) {
    *OutResetType = EfiResetCold;
  }

  cJSON *Cmd = cJSON_GetObjectItemCaseSensitive(Tree, "CMD");
  if (Cmd == NULL || Cmd->type != cJSON_String || Cmd->valuestring == NULL) {
    // 这里你可以用 JsonSetBool/JsonSetString（如果你已有封装）
    cJSON_AddBoolToObject(Tree, "SUCCESS", FALSE);
    cJSON_AddStringToObject(Tree, "ERROR", "Missing/invalid CMD (string)");
    return EFI_INVALID_PARAMETER;
  }

  OKN_MT_CMD_CTX Ctx;
  ZeroMem(&Ctx, sizeof(Ctx));
  Ctx.Proto    = Proto;
  Ctx.Tree     = Tree;
  Ctx.ResetReq = FALSE;

  CONST CHAR8 *CmdStr = (CONST CHAR8 *)Cmd->valuestring;

  for (UINTN i = 0; i < OknCmdTableCount(); i++) {
    if (AsciiStrCmp(CmdStr, gOknCmdTable[i].CmdName) == 0) {
      EFI_STATUS Status = gOknCmdTable[i].Handler(&Ctx);

      // 可选：统一回显 SUCCESS（如果你希望所有命令都带 SUCCESS）
      // cJSON_AddBoolToObject(Tree, "SUCCESS", EFI_ERROR(Status) ? FALSE : TRUE);

      if (!EFI_ERROR(Status) && Ctx.ResetReq) {
        if (OutNeedReset)
          *OutNeedReset = TRUE;
        if (OutResetType)
          *OutResetType = Ctx.ResetType;
      }
      return Status;
    }
  }

  cJSON_AddBoolToObject(Tree, "SUCCESS", FALSE);
  cJSON_AddStringToObject(Tree, "ERROR", "Unsupported CMD");

  return EFI_UNSUPPORTED;
}

// 从 SMBIOS Optional String Area(以 '\0\0' 结束)中, 按 1-based Index 获取字符串指针.
// Index == 0: SMBIOS 语义为 "Not Specified", 返回 EFI_NOT_FOUND(更合理)
// 成功: *OutStr 指向原始记录内存(不拷贝)
STATIC EFI_STATUS OKnSmbiosGetOptionalStringByIndex(IN CONST CHAR8   *OptionalStrStart,
                                                    IN UINT8          Index,
                                                    OUT CONST CHAR8 **OutStr)
{
  if (OptionalStrStart == NULL || OutStr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *OutStr = NULL;

  // SMBIOS: 0 means "Not Specified"
  if (Index == 0) {
    return EFI_NOT_FOUND;
  }

  CONST CHAR8 *p = OptionalStrStart;

  // 遍历字符串区: String1\0String2\0...\0\0
  while (*p != '\0') {
    // p 指向当前字符串起始
    Index--;
    if (Index == 0) {
      // 可选: 拒绝空串(一般不需要, 空串也算合法值)
      // if (*p == '\0') return EFI_NOT_FOUND;
      *OutStr = p;
      return EFI_SUCCESS;
    }

    // 跳到下一个字符串起始: 走到 '\0' 后再 +1
    p += AsciiStrLen(p) + 1;
  }

  // 到了 '\0\0' 的第一个 '\0', 说明字符串区结束, Index 仍未耗尽 => 越界
  return EFI_NOT_FOUND;
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_SetAmtConfig(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_SetAmtConfig(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Status(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_MT86Status(Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Abort(OKN_MT_CMD_CTX *Ctx)
{
  Cmd_MT86Abort();
  return EFI_SUCCESS;
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Start(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_MT86Start(Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_Reply9527(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_Reply9527(Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_HwInfo(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_HwInfo(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_ReadSPD(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_ReadSPD(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfig(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_GetMemConfig(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfigReal(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_GetMemConfigReal(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_SetMemConfig(OKN_MT_CMD_CTX *Ctx)
{
  return Cmd_SetMemConfig(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_ResetSystem(OKN_MT_CMD_CTX *Ctx)
{
  EFI_STATUS     Status;
  EFI_RESET_TYPE Rt = EfiResetCold;

  Status = Cmd_ResetSystem(Ctx->Tree, &Rt);
  if (!EFI_ERROR(Status)) {
    Ctx->ResetType = Rt;
    Ctx->ResetReq  = TRUE;
  }
  return Status;
}

/**
 * 处理 amtStart 命令 | 改成: SetAmtConfig
 */
STATIC EFI_STATUS Cmd_SetAmtConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (NULL == pProto || NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] Protocol or JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  Status = OknMT_SetAmtConfig(pProto, pJsTree);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] OknMT_SetAmtConfig() failed: %r\n", __func__, Status);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
 * 处理 testStatus 命令 | 改成: MT86_Status
 */
STATIC EFI_STATUS Cmd_MT86Status(OUT cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }
  // 收集错误数量, 这些都是累加值
  cJSON_AddNumberToObject(pJsTree, "DataMissCompare", MtSupportGetNumErrors());
  cJSON_AddNumberToObject(pJsTree, "CorrError", MtSupportGetNumCorrECCErrors());
  cJSON_AddNumberToObject(pJsTree, "UncorrError", MtSupportGetNumUncorrECCErrors());
  cJSON_AddNumberToObject(pJsTree, "UnknownSlotError", MtSupportGetNumUnknownSlotErrors());

  cJSON              *ErrorInfo = cJSON_AddArrayToObject(pJsTree, "ERRORINFO");
  DIMM_ADDRESS_DETAIL Item;
  for (UINT8 i = 0; i < 4; i++) {
    ZeroMem(&Item, sizeof(DIMM_ADDRESS_DETAIL));

    Status = OknMT_DequeueErrorCopy(&gOknDimmErrorQueue, &Item);
    if (EFI_NOT_FOUND == Status) {
      break;
    }
    cJSON *Detail = cJSON_CreateObject();
    cJSON_AddNumberToObject(Detail, "Socket", Item.SocketId);
    cJSON_AddNumberToObject(Detail, "MemCtrl", Item.MemCtrlId);
    cJSON_AddNumberToObject(Detail, "Channel", Item.MemCtrlId * 2 + Item.ChannelId);
    cJSON_AddNumberToObject(Detail, "Dimm", 0);
    cJSON_AddNumberToObject(Detail, "Rank", Item.RankId);
    cJSON_AddNumberToObject(Detail, "SubCh", Item.SubChId);
    cJSON_AddNumberToObject(Detail, "BG", Item.BankGroup);
    cJSON_AddNumberToObject(Detail, "Bank", Item.Bank);
    cJSON_AddNumberToObject(Detail, "Row", Item.Row);
    cJSON_AddNumberToObject(Detail, "Col", Item.Column);
    cJSON_AddItemToArray(ErrorInfo, Detail);
  }

  CHAR16 UnicodeBuf[OKN_BUF_SIZE] = {0};
  CHAR8  AsciiBuf[OKN_BUF_SIZE]   = {0};
  GetStringById(STRING_TOKEN(gCustomTestList[gOknMT86TestID].NameStrId), UnicodeBuf, BUF_SIZE);
  UnicodeStrToAsciiStrS(UnicodeBuf, AsciiBuf, OKN_BUF_SIZE);
  cJSON_AddStringToObject(pJsTree, "NAME", AsciiBuf);
  cJSON_AddNumberToObject(pJsTree, "ID", gOknMT86TestID + 47);
  switch (mPatternUI.NewSize) {
    case 4: AsciiSPrint(AsciiBuf, OKN_BUF_SIZE, "0x%08X", mPatternUI.NewPattern[0]); break;
    case 8: AsciiSPrint(AsciiBuf, OKN_BUF_SIZE, "0x%016lX", *((UINT64 *)mPatternUI.NewPattern)); break;
    case 16:
      AsciiSPrint(AsciiBuf, OKN_BUF_SIZE, "0x%08X%08X", mPatternUI.NewPattern[0], mPatternUI.NewPattern[1]);
      break;
  }

  cJSON_AddStringToObject(pJsTree, "PATTERN", AsciiBuf);
  cJSON_AddNumberToObject(pJsTree, "PROGRESS", gOknLastPercent);
  cJSON_AddNumberToObject(pJsTree, "STATUS", gOknTestStatus);

  return EFI_SUCCESS;
}

/**
 * 处理 testStop 命令 | 改成: MT86_Abort
 */
STATIC VOID Cmd_MT86Abort(VOID)
{
  MtSupportAbortTesting();
  gOknTestStart  = FALSE;  // fix duplicate test
  gOknTestStatus = 2;

  return;
}

/**
 * 处理 testStart 命令 | 改成: MT86_Start
 */
STATIC EFI_STATUS Cmd_MT86Start(IN cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  gNumPasses      = 1;
  gOknLastPercent = 0;
  gOknTestPause   = FALSE;

  OknMT_InitErrorQueue(&gOknDimmErrorQueue);

  cJSON *Id = cJSON_GetObjectItemCaseSensitive(pJsTree, "ID");
  if (Id == NULL || Id->type != cJSON_Number || Id->valueu64 > gNumCustomTests + 47 || Id->valueu64 < 47) {
    Print(L"[OKN_UEFI_ERR] [%s] Invalid test ID\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  for (UINT8 i = 0; i < gNumCustomTests; i++) {
    gCustomTestList[i].Enabled = FALSE;
  }
  gCustomTestList[Id->valueu64 - 47].Enabled = TRUE;

  gOknMT86TestID = (INT8)(Id->valueu64 - 47);  // ?
  gOknTestStart  = TRUE;                       // ?
  gOknTestStatus = 1;                          // ?

  return EFI_SUCCESS;
}

/**
 * 处理 areyouok 命令 | 改成: Reply_9527
 */
STATIC EFI_STATUS Cmd_Reply9527(OUT cJSON *pJsTree)
{
  EFI_STATUS Status                          = EFI_SUCCESS;
  CHAR8      AsciiSPrintBuffer[OKN_BUF_SIZE] = {0};

  if (NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  // 关键: MAC/IP 来自"当前接收该包的 NIC"
  if (gOknJsonCtxSocket != NULL) {
    AsciiSPrint(AsciiSPrintBuffer,
                OKN_BUF_SIZE,
                "%02x:%02x:%02x:%02x:%02x:%02x",
                gOknJsonCtxSocket->NicMac[0],
                gOknJsonCtxSocket->NicMac[1],
                gOknJsonCtxSocket->NicMac[2],
                gOknJsonCtxSocket->NicMac[3],
                gOknJsonCtxSocket->NicMac[4],
                gOknJsonCtxSocket->NicMac[5]);
    cJSON_AddStringToObject(pJsTree, "MAC", AsciiSPrintBuffer);

    if (gOknJsonCtxSocket->NicIpValid) {
      AsciiSPrint(AsciiSPrintBuffer,
                  OKN_BUF_SIZE,
                  "%d.%d.%d.%d",
                  gOknJsonCtxSocket->NicIp.Addr[0],
                  gOknJsonCtxSocket->NicIp.Addr[1],
                  gOknJsonCtxSocket->NicIp.Addr[2],
                  gOknJsonCtxSocket->NicIp.Addr[3]);
      cJSON_AddStringToObject(pJsTree, "IP", AsciiSPrintBuffer);
    }
  }
  else {
    // 兜底：至少别填错
    cJSON_AddStringToObject(pJsTree, "MAC", "00:00:00:00:00:00");
    cJSON_AddStringToObject(pJsTree, "IP", "0.0.0.0");
  }

  AsciiSPrint(AsciiSPrintBuffer, OKN_BUF_SIZE, "mt86-v%d.%d", PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR);
  cJSON_AddStringToObject(pJsTree, "MT86_FW", AsciiSPrintBuffer);

  return EFI_SUCCESS;
}

/**
 * 处理 connect 命令 | 改成: HW_Info
 */
STATIC EFI_STATUS Cmd_HwInfo(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (NULL == pProto || NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] Protocol or JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  if (NULL == pProto->GetDimmTemp || NULL == pProto->IsDimmPresent || NULL == pProto->SpdRead) {
    Print(L"[OKN_UEFI_ERR] [%s] Protocol function pointer is NULL\n", __func__);
    return EFI_UNSUPPORTED;
  }

  // 1. 获取TSOD模块数量, 是整个系统的TSOD模块的数量, 不是单个DIMM的TSOD数量
  MtSupportGetTSODInfo(NULL, 0);  // 只是为了更新 g_numTSODModules, 参数1是NULL的话, 参数2必须是0
  cJSON_AddNumberToObject(pJsTree, "TSOD_CNT", g_numTSODModules);  // 之前是"RESULT"

  // 2. 获取 CPU 信息
  do {
    EFI_SMBIOS_PROTOCOL *pSmbiosProto;
    EFI_SMBIOS_HANDLE    SmbiosHandle;
    SMBIOS_TABLE_TYPE4  *pSmbiosType4Record;
    EFI_SMBIOS_TYPE      SmbiosType;

    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    SmbiosType   = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
    Status       = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&pSmbiosProto);
    if (EFI_SUCCESS == Status) {
      while (1) {
        Status = pSmbiosProto->GetNext(pSmbiosProto,
                                       &SmbiosHandle,
                                       &SmbiosType,
                                       (EFI_SMBIOS_TABLE_HEADER **)&pSmbiosType4Record,
                                       NULL);
        if (TRUE == EFI_ERROR(Status)) {
          break;
        }
        CHAR8 *pCpuID, *pCpuVerInfo;
        Status =
            OKnSmbiosGetOptionalStringByIndex((CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
                                              pSmbiosType4Record->Socket,
                                              &pCpuID);
        Status =
            OKnSmbiosGetOptionalStringByIndex((CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
                                              pSmbiosType4Record->ProcessorVersion,
                                              &pCpuVerInfo);
        cJSON_AddStringToObject(pJsTree, pCpuID, pCpuVerInfo);
      }
    }
  } while (0);

  // 3. SLOT_CNT
  cJSON_AddNumberToObject(pJsTree, "SMBIOS_MEM_DEV_CNT", g_numSMBIOSMem);

  // 4. 获取DIMM信息
  cJSON *DimmInfoArray = cJSON_AddArrayToObject(pJsTree, "DIMM_INFO");
  for (UINT8 i = 0; i < g_numSMBIOSMem; i++) {
    cJSON *Info = cJSON_CreateObject();

    UINT8 Socket  = i / 8;
    UINT8 Channel = i % 8;
    UINT8 Dimm    = 0;  // 单通道设计, 只有Dimm 0

    BOOLEAN                  RamPresent    = FALSE;
    UINT8                    Online        = 0;
    INT32                    RamTemp0      = OKN_MAGIC_NUMBER;
    INT32                    RamTemp1      = 0;
    INT32                    HubTemp       = 0;
    DIMM_RANK_MAP_OUT_REASON MapOutReason  = DimmRankMapOutMax;
    INT32                    SdramDevWidth = OKN_MAGIC_NUMBER;  // SDRAM Device Width
    INT32                    PkgRanksCnt   = OKN_MAGIC_NUMBER;  // Number of Package Ranks per DIMM
    INT32                    SdramPkgCnt   = OKN_MAGIC_NUMBER;  // SDRAM Chip Count, 不含ECC
    INT32                    EccPkgCnt     = OKN_MAGIC_NUMBER;  // ECC Chip Count

    Status = pProto->IsDimmPresent(Socket, Channel, Dimm, &RamPresent);
    if (FALSE == EFI_ERROR(Status)) {  // 在线
      Online = RamPresent ? 1 : 0;
      if (TRUE == RamPresent) {
        // 读取温度
        Status = pProto->GetDimmTemp(Socket, Channel, Dimm, &RamTemp0, &RamTemp1, &HubTemp);
        // 读取 MAP_OUT_REASON
        Status = pProto->GetDisReason(Socket, Channel, Dimm, &MapOutReason);
        // 读取 SPD 数据: SdramDevWidth / PkgRanksCnt / SdramPkgCnt / EccPkgCnt
        UINT8 SpdData[1024] = {0};
        Status              = pProto->SpdRead(Socket, Channel, Dimm, SpdData, DDR4_SPD_LEN);
        if (FALSE == EFI_ERROR(Status)) {
          // 这里没有检查返回值, 因为我使用了 OKN_MAGIC_ERR_NUM作为错误标志;
          OknDdr4SpdGetSdramDeviceWidthBits(SpdData, DDR4_SPD_LEN, (UINT8 *)&SdramDevWidth);
          OknDdr4SpdGetPackageRanksPerDimm(SpdData, DDR4_SPD_LEN, (UINT8 *)&PkgRanksCnt);
          OknDdr4SpdEstimateTotalDataPackages(SpdData, DDR4_SPD_LEN, (UINT8 *)&SdramPkgCnt);
          OknDdr4SpdEstimateTotalEccPackages(SpdData, DDR4_SPD_LEN, (UINT8 *)&EccPkgCnt);
        }
        else {
          // clang-format off
          Print(L"[OKN_UEFI_ERR] [%s] SpdRead() failed for Skt %d Ch %d Dimm %d: %r\n",
                __func__, Socket, Channel, Dimm, Status);
          // clang-format on
        }
      }  // if (TRUE == RamPresent)
    }  //  if (FALSE == EFI_ERROR(Status))

    cJSON_AddNumberToObject(Info, "ONLINE", Online);
    cJSON_AddNumberToObject(Info, "RAM_TEMP", RamTemp0);
    cJSON_AddNumberToObject(Info, "MAP_OUT_REASON_CODE", (UINT32)MapOutReason);
    cJSON_AddNumberToObject(Info, "SDRAM_DEV_WIDTH", (UINT32)SdramDevWidth);
    cJSON_AddNumberToObject(Info, "PKG_RANK_CNT", (UINT32)PkgRanksCnt);
    cJSON_AddNumberToObject(Info, "SDRAM_PKG_CNT", SdramPkgCnt);
    cJSON_AddNumberToObject(Info, "ECC_PKG_CNT", EccPkgCnt);

    cJSON_AddItemToArray(DimmInfoArray, Info);
  }

  return EFI_SUCCESS;
}

/**
 * 处理 readSPD 命令
 */
STATIC EFI_STATUS Cmd_ReadSPD(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = OknMT_ReadSpdToJson(pProto, pJsTree);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] OknMT_ReadSpdToJson() failed: %r\n", __func__, Status);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
 * 处理 testConfigGet 命令 | 改成: GetMemConfig
 */
STATIC EFI_STATUS Cmd_GetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = OknMT_GetMemConfig(pProto, pJsTree);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] OknMT_GetMemConfig() failed: %r\n", __func__, Status);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
 * 处理 testConfigActive 命令 | 改成: GetMemConfigReal
 */
STATIC EFI_STATUS Cmd_GetMemConfigReal(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = OknMT_GetMemConfigReal(pProto, pJsTree);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] OknMT_GetMemConfigReal() failed: %r\n", __func__, Status);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
 * 处理 testConfigSet 命令 | 改成: SetMemConfig
 */
STATIC EFI_STATUS Cmd_SetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = OknMT_SetMemConfig(pProto, pJsTree);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] OknMT_SetMemConfig() failed: %r\n", __func__, Status);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
 * 处理 reset 命令 | 改成: ResetSystem
 * MdePkg/Include/Uefi/UefiMultiPhase.h有定义 EFI_RESET_TYPE;
 */
STATIC EFI_STATUS Cmd_ResetSystem(IN OUT cJSON *pTree, OUT EFI_RESET_TYPE *pResetType)
{
  EFI_STATUS Status;
  UINT32     TypeU32;

  if (NULL == pTree || NULL == pResetType) {
    Print(L"[OKN_UEFI_ERR] [%s] JSON tree or pResetType is NULL: %r\n", __func__, Status);
    return EFI_INVALID_PARAMETER;
  }

  Status = JsonGetU32FromObject(pTree, "TYPE", &TypeU32);
  if (EFI_ERROR(Status)) {
    JsonSetBool(pTree, "SUCCESS", FALSE);
    JsonSetString(pTree, "ERROR", "Missing/invalid TYPE (number 0..2)");
    return Status;
  }

  // 只支持 0/1/2，不支持 3（PlatformSpecific）,因为那需要 ResetData GUID
  if (TypeU32 > EfiResetShutdown) {  // EfiResetShutdown == 2
    JsonSetBool(pTree, "SUCCESS", FALSE);
    if (EfiResetPlatformSpecific == TypeU32) {
      JsonSetString(pTree, "ERROR", "TYPE=3 (PlatformSpecific) not supported (GUID ResetData disabled by design)");
      return EFI_UNSUPPORTED;
    }
    JsonSetString(pTree, "ERROR", "TYPE out of range (0=cold,1=warm,2=shutdown)");
    return EFI_INVALID_PARAMETER;
  }

  *pResetType = (EFI_RESET_TYPE)TypeU32;

  JsonSetNumber(pTree, "TYPE", (INTN)TypeU32);  // 规范化回显
  JsonSetString(pTree, "ERROR", "");

  return EFI_SUCCESS;
}
#endif  // STATIC函数实现区域 OFF
