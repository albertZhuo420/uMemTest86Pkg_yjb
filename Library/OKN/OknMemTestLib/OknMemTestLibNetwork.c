/**
 * @file
 *
 */

#include <Library/OKN/OknDdr4SpdLib/OknDdr4SpdLib.h>
#include <Library/OKN/OknMemTestLib/OknMemTestLib.h>
#include <Library/OKN/PortingLibs/Udp4SocketLib.h>
#include <Protocol/Smbios.h>
#include <uMemTest86.h>

/**
 * UDP4 Rx (Udp4ReceiveHandler)
 *    ├─ cJSON_ParseWithLength(udp_payload)  -> Tree
 *    ├─ JsonHandler(Tree)                   -> 在 Tree 里加返回字段 / 改全局状态
 *    ├─ cJSON_PrintUnformatted(Tree)        -> JSON response string
 *    └─ UDP4 Tx (gOknUdpSocketTransmit->Transmit) -> 回发给上位机
 */

UINTN gOknLastPercent;  // 用在uMemTest86Pkg/Ui.c 文件中: mLastPercent;

BOOLEAN              gOKnSkipWaiting = FALSE;
BOOLEAN              gOknTestStart   = FALSE;
BOOLEAN              gOknTestPause   = FALSE;
UINT8                gOknTestReset   = 0xff;
OKN_TEST_STATUS_TYPE gOknTestStatus  = OKN_TST_Unknown;
INT8                 gOknMT86TestID  = -1;               // 这个是MT86真实的TestID [0 ... 15]

CHAR8 gOknAsciiSPrintBuffer[OKN_BUF_SIZE];

VOID JsonHandler(cJSON *Tree)
{
  EFI_STATUS Status = 0;

  cJSON *Cmd = cJSON_GetObjectItemCaseSensitive(Tree, "CMD");
  if (!Cmd || Cmd->type != cJSON_String) {
    return;
  }

  if (AsciiStrnCmp("testStatus", Cmd->valuestring, 10) == 0) {
    cJSON *ErrorInfo = cJSON_AddArrayToObject(Tree, "ERRORINFO");
    cJSON_AddNumberToObject(Tree, "DataMissCompare", MtSupportGetNumErrors());
    cJSON_AddNumberToObject(Tree, "CorrError", MtSupportGetNumCorrECCErrors());
    cJSON_AddNumberToObject(Tree, "UncorrError", MtSupportGetNumUncorrECCErrors());
    // cJSON_AddNumberToObject(Tree, "UnknownError", MtSupportGetNumUnknownSlotErrors());
    DIMM_ADDRESS_DETAIL *Item = NULL;

    for (UINT8 i = 0; i < 4; i++) {
      Item = DequeueError(&gOknDimmErrorQueue);
      if (!Item) {
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
    cJSON_AddStringToObject(Tree, "NAME", AsciiBuf);
    cJSON_AddNumberToObject(Tree, "ID", gOknMT86TestID + 47);
    switch (mPatternUI.NewSize) {
      case 4:  AsciiSPrint(AsciiBuf, OKN_BUF_SIZE, "0x%08X", mPatternUI.NewPattern[0]); break;
      case 8:  AsciiSPrint(AsciiBuf, OKN_BUF_SIZE, "0x%016lX", *((UINT64 *)mPatternUI.NewPattern)); break;
      case 16: AsciiSPrint(AsciiBuf, OKN_BUF_SIZE, "0x%08X%08X", mPatternUI.NewPattern[0], mPatternUI.NewPattern[1]); break;
    }
    cJSON_AddStringToObject(Tree, "PATTERN_NEW", gBuffer);
    cJSON_AddNumberToObject(Tree, "PROGRESS", gOknLastPercent);
    // cJSON_AddNumberToObject(Tree, "MEMSTART", gStartAddr);
    // cJSON_AddNumberToObject(Tree, "MEMEND", gEndAddr);
    cJSON_AddNumberToObject(Tree, "STATUS", gOknTestStatus);
  }
  else if (AsciiStrnCmp("testStart", Cmd->valuestring, 9) == 0) {
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
    gOknMT86TestID                             = (INT8)(Id->valueu64 - 47);
    gNumPasses                                 = 1;
    gOknTestStart                              = TRUE;
    gOknTestStatus                             = 1;

    cJSON_AddBoolToObject(Tree, "SUCCESS", gOknTestStart);
    cJSON *SlotInfo = cJSON_AddArrayToObject(Tree, "SLOTINFO");

    UINT8 Online  = 0;
    INT32 RamTemp = 0;
    int   index   = 0;
    MtSupportGetTSODInfo(&RamTemp, 0);
    UINT8 _socket = 0, _channel = 0, socket = 0, channel = 0;
    for (UINT8 i = 0; i < g_numSMBIOSMem; i++) {
      socket      = i / 8;
      channel     = i % 8;
      _socket     = channel % 2;
      _channel    = ((socket << 3) + channel) / 2;
      cJSON *Info = cJSON_CreateObject();
      if (mSpdTableDDR[_channel][_socket][0] != 0) {
        Online  = 1;
        RamTemp = g_MemTSODInfo[index++].temperature;
      }
      else {
        Online  = 0;
        RamTemp = 0;
      }
      cJSON_AddNumberToObject(Info, "ONLINE", Online);
      cJSON_AddNumberToObject(Info, "RAMTEMP", RamTemp);
      cJSON_AddItemToArray(SlotInfo, Info);
    }
    cJSON_AddNumberToObject(Tree, "STATUS", gOknTestStart);
  }
  else if (AsciiStrnCmp("testStop", Cmd->valuestring, 8) == 0) {
    cJSON_AddBoolToObject(Tree, "SUCCESS", TRUE);
    MtSupportAbortTesting();
    gOknTestStart  = FALSE;  // fix duplicate test
    gOknTestStatus = 2;
    // gTestStop = TRUE;
  }
  else if (AsciiStrnCmp("areyouok", Cmd->valuestring, 8) == 0) {
    cJSON_AddBoolToObject(Tree, "SUCCESS", TRUE);
    // 关键：MAC/IP 来自“当前接收该包的 NIC”
    if (gJsonCtxSocket != NULL) {
      AsciiSPrint(gOknAsciiSPrintBuffer,
                  OKN_BUF_SIZE,
                  "%02x:%02x:%02x:%02x:%02x:%02x",
                  gJsonCtxSocket->NicMac[0],
                  gJsonCtxSocket->NicMac[1],
                  gJsonCtxSocket->NicMac[2],
                  gJsonCtxSocket->NicMac[3],
                  gJsonCtxSocket->NicMac[4],
                  gJsonCtxSocket->NicMac[5]);
      cJSON_AddStringToObject(Tree, "MAC", gBuffer);

      if (gJsonCtxSocket->NicIpValid) {
        AsciiSPrint(gBuffer,
                    BUF_SIZE,
                    "%d.%d.%d.%d",
                    gJsonCtxSocket->NicIp.Addr[0],
                    gJsonCtxSocket->NicIp.Addr[1],
                    gJsonCtxSocket->NicIp.Addr[2],
                    gJsonCtxSocket->NicIp.Addr[3]);
        cJSON_AddStringToObject(Tree, "IP", gBuffer);
      }
    }
    else {
      // 兜底：至少别填错
      cJSON_AddStringToObject(Tree, "MAC", "00:00:00:00:00:00");
      cJSON_AddStringToObject(Tree, "IP", "0.0.0.0");
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "mt86-v%d.%d", PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR);
    cJSON_AddStringToObject(Tree, "FW", gBuffer);
  }
  else if (AsciiStrnCmp("connect", Cmd->valuestring, 7) == 0) {
    // 1. 获取TSOD模块数量, 是整个系统的TSOD模块的数量, 不是单个DIMM的TSOD数量
    MtSupportGetTSODInfo(NULL, 0);  // 只是为了更新 g_numTSODModules, 参数1是NULL的话, 参数2必须是0
    cJSON_AddNumberToObject(Tree, "TSOD_CNT", g_numTSODModules);  // 之前是"RESULT"

    // 2. 获取 CPU 信息
    do {
      EFI_SMBIOS_HANDLE    SmbiosHandle;
      EFI_SMBIOS_PROTOCOL *pSmbios;
      SMBIOS_TABLE_TYPE4  *pSmbiosType4Record;
      EFI_SMBIOS_TYPE      SmbiosType;

      SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
      SmbiosType   = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
      Status       = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&pSmbios);
      if (Status == 0) {
        while (1) {
          Status = pSmbios->GetNext(pSmbios,
                                    &SmbiosHandle,
                                    &SmbiosType,
                                    (EFI_SMBIOS_TABLE_HEADER **)&pSmbiosType4Record,
                                    NULL);
          if (TRUE == EFI_ERROR(Status)) {
            break;
          }
          CHAR8 *pCpuID, *pCpuVerInfo;
          Status = SmbiosGetString((CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
                                   pSmbiosType4Record->Socket,
                                   &pCpuID);
          Status = SmbiosGetString((CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
                                   pSmbiosType4Record->ProcessorVersion,
                                   &pCpuVerInfo);
          cJSON_AddStringToObject(Tree, pCpuID, pCpuVerInfo);
        }
      }
    } while (0);

    // 3. 获取DIMM信息
    cJSON *SlotInfo = cJSON_AddArrayToObject(Tree, "DIMM_INFO");

    UINT8 Online    = 0;
    INT32 RamTemp   = 0;
    int   ChipWidth = 0;
    int   NumRanks  = 0;
    int   NumChips  = 0;
    int   index     = 0;
    UINT8 _socket = 0, _channel = 0, socket = 0, channel = 0;
    MtSupportGetTSODInfo(&RamTemp, 0);
#ifndef SEAVO
    for (UINT8 i = 0; i < g_numSMBIOSMem; i++) {
      socket   = i / 8;
      channel  = i % 8;
      _socket  = channel % 2;
      _channel = ((socket << 3) + channel) / 2;
#else
    for (UINT8 i = 0; i < 8; i++) {
      _channel   = i;
      UINT8 dimm = 0;
      _socket    = 0;
#endif
      cJSON *Info = cJSON_CreateObject();
#ifndef SEAVO
      if (mSpdTableDDR[_channel][_socket][0] != 0) {
#else
      if (mSpdTableDDR[_channel][_socket][dimm][0] != 0) {
#endif
        Online  = 1;
        RamTemp = g_MemTSODInfo[index++].temperature;
        GetMemInfoSpd(i, &ChipWidth, &NumRanks);
        ChipWidth = MtSupportGetChipWidth(i);
        NumRanks  = MtSupportGetNumRanks(i);
        NumChips  = NumRanks * 64 / ChipWidth;
        cJSON_AddNumberToObject(Info, "CHIPWITH", ChipWidth);
        cJSON_AddNumberToObject(Info, "RANKS", NumRanks);
        cJSON_AddNumberToObject(Info, "CHIPS", NumChips);
      }
      else {
        Online  = 0;
        RamTemp = 0;
      }
      cJSON_AddNumberToObject(Info, "ONLINE", Online);
      cJSON *Reason = cJSON_AddArrayToObject(Info, "REASON");
      GetMapOutReason(socket, channel, Reason);
      cJSON_AddNumberToObject(Info, "RAMTEMP", RamTemp);
      cJSON_AddItemToArray(SlotInfo, Info);
    }
  }
  else if (AsciiStrnCmp("readSPD", Cmd->valuestring, 7) == 0) {
    readSPD(Tree);
  }
  else if (AsciiStrnCmp("testConfigGet", Cmd->valuestring, 13) == 0) {
    testConfigGet(Tree);
  }
  else if (AsciiStrnCmp("testConfigSet", Cmd->valuestring, 13) == 0) {
    testConfigSet(Tree);
  }
  else if (AsciiStrnCmp("testConfigActive", Cmd->valuestring, 16) == 0) {
    testConfigActive(Tree);
  }
  else if (AsciiStrnCmp("reset", Cmd->valuestring, 5) == 0) {
    cJSON *Type = cJSON_GetObjectItemCaseSensitive(Tree, "TYPE");
    gOknTestReset  = (UINT8)Type->valueu64;
  }
  else if (AsciiStrnCmp("amtStart", Cmd->valuestring, 8) == 0) {
    amtControl(Tree, TRUE);
  }

  return;
}

/**
 * 处理 amtStart 命令 | 我要改成: SetAmtConfig
 */
EFI_STATUS OknMT_ProcessJsonCmd_SetAmtConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree)
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
 * 处理 testStatus 命令 | 我要改成: MT86_Status
 */
EFI_STATUS OknMT_ProcessJsonCmd_MT86Status(OUT cJSON *pJsTree)
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
 * 处理 testStop 命令 | 我要改成: MT86_Abort
 */
VOID OknMT_ProcessJsonCmd_MT86Abort(VOID)
{
  MtSupportAbortTesting();
  gOknTestStart  = FALSE;  // fix duplicate test
  gOknTestStatus = 2;

  return;
}

/**
 * 处理 testStart 命令 | 我要改成: MT86_Start
 */
EFI_STATUS OknMT_ProcessJsonCmd_MT86Start(IN cJSON *pJsTree)
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
 * 处理 areyouok 命令 | 我要改成: Reply_9527
 */
EFI_STATUS OknMT_ProcessJsonCmd_Reply9527(OUT cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  if (NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  // 关键：MAC/IP 来自“当前接收该包的 NIC”
  if (gJsonCtxSocket != NULL) {
    AsciiSPrint(gOknAsciiSPrintBuffer,
                OKN_BUF_SIZE,
                "%02x:%02x:%02x:%02x:%02x:%02x",
                gJsonCtxSocket->NicMac[0],
                gJsonCtxSocket->NicMac[1],
                gJsonCtxSocket->NicMac[2],
                gJsonCtxSocket->NicMac[3],
                gJsonCtxSocket->NicMac[4],
                gJsonCtxSocket->NicMac[5]);
    cJSON_AddStringToObject(pJsTree, "MAC", gOknAsciiSPrintBuffer);

    if (gJsonCtxSocket->NicIpValid) {
      AsciiSPrint(gOknAsciiSPrintBuffer,
                  OKN_BUF_SIZE,
                  "%d.%d.%d.%d",
                  gJsonCtxSocket->NicIp.Addr[0],
                  gJsonCtxSocket->NicIp.Addr[1],
                  gJsonCtxSocket->NicIp.Addr[2],
                  gJsonCtxSocket->NicIp.Addr[3]);
      cJSON_AddStringToObject(pJsTree, "IP", gOknAsciiSPrintBuffer);
    }
  }
  else {
    // 兜底：至少别填错
    cJSON_AddStringToObject(pJsTree, "MAC", "00:00:00:00:00:00");
    cJSON_AddStringToObject(pJsTree, "IP", "0.0.0.0");
  }

  AsciiSPrint(gOknAsciiSPrintBuffer, OKN_BUF_SIZE, "mt86-v%d.%d", PROGRAM_VERSION_MAJOR, PROGRAM_VERSION_MINOR);
  cJSON_AddStringToObject(pJsTree, "MT86_FW", gOknAsciiSPrintBuffer);

  return EFI_SUCCESS;
}

/**
 * 处理 connect 命令 | 我要改成: HW_Info
 */
EFI_STATUS OknMT_ProcessJsonCmd_HwInfo(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
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
    EFI_SMBIOS_HANDLE    SmbiosHandle;
    EFI_SMBIOS_PROTOCOL *pSmbios;
    SMBIOS_TABLE_TYPE4  *pSmbiosType4Record;
    EFI_SMBIOS_TYPE      SmbiosType;

    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    SmbiosType   = EFI_SMBIOS_TYPE_PROCESSOR_INFORMATION;
    Status       = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID **)&pSmbios);
    if (EFI_SUCCESS == Status) {
      while (1) {
        Status = pSmbios->GetNext(pSmbios,
                                  &SmbiosHandle,
                                  &SmbiosType,
                                  (EFI_SMBIOS_TABLE_HEADER **)&pSmbiosType4Record,
                                  NULL);
        if (TRUE == EFI_ERROR(Status)) {
          break;
        }
        CHAR8 *pCpuID, *pCpuVerInfo;
        Status = SmbiosGetString((CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
                                 pSmbiosType4Record->Socket,
                                 &pCpuID);
        Status = SmbiosGetString((CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
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
    cJSON_AddNumberToObject(Info, "SDRAM_DEV_WIDTH", (UINT32)MapOutReason);
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
EFI_STATUS OknMT_ProcessJsonCmd_ReadSPD(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree)
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
 * 处理 testConfigGet 命令 | 我要改成: GetMemConfig
 */
EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
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
 * 处理 testConfigActive 命令 | 我要改成: GetMemConfigReal
 */
EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfigReal(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
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
 * 处理 testConfigSet 命令 | 我要改成: SetMemConfig
 */
EFI_STATUS OknMT_ProcessJsonCmd_SetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree)
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
 * 处理 reset 命令 | 我要改成: ResetSystem
 * MdePkg/Include/Uefi/UefiMultiPhase.h有定义 EFI_RESET_TYPE;
 */
EFI_STATUS OknMT_ProcessJsonCmd_ResetSystem(IN OUT cJSON *pTree, OUT EFI_RESET_TYPE *pResetType)
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
