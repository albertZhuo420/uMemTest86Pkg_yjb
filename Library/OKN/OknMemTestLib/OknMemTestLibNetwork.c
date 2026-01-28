/**
 * @file
 *
 */

#include <Library/OKN/OknDDR4SpdLib/OknDDR4SpdLib.h>
#include <Library/OKN/OknMemTestLib/OknMemTestLib.h>
#include <Protocol/Smbios.h>

/**
 * UDP4 Rx (Udp4ReceiveHandler)
 *    ├─ cJSON_ParseWithLength(udp_payload)  -> Tree
 *    ├─ JsonHandler(Tree)                  -> 在 Tree 里加返回字段 / 改全局状态
 *    ├─ cJSON_PrintUnformatted(Tree)       -> JSON response string
 *    └─ UDP4 Tx (gSocketTransmit->Transmit) 回发给上位机
 */

#define OKN_BUF_SIZE 1024
char gOknAsciiSPrintBuffer[OKN_BUF_SIZE];

// extern UINT8 mSpdTableDDR[8][2][1024];
extern UINT8 mSpdTableDDR[8][2][2][1024];

extern VOID EFIAPI UnlockAllMemRanges(VOID);
extern VOID EFIAPI LockAllMemRanges(VOID);

VOID JsonHandler(cJSON *Tree)
{
  EFI_STATUS Status = 0;

  cJSON *Cmd = cJSON_GetObjectItemCaseSensitive(Tree, "CMD");
  if (!Cmd || Cmd->type != cJSON_String) {
    return;
  }
  if (AsciiStrnCmp("testStatus", Cmd->valuestring, 10) == 0) {
    cJSON *ErrorInfo = cJSON_AddArrayToObject(Tree, "ERRORINFO");
    cJSON_AddNumberToObject(Tree, "TID", gTestTid);
    cJSON_AddNumberToObject(Tree, "DataMissCompare", MtSupportGetNumErrors());
    cJSON_AddNumberToObject(Tree, "CorrError", MtSupportGetNumCorrECCErrors());
    cJSON_AddNumberToObject(Tree, "UncorrError", MtSupportGetNumUncorrECCErrors());
    // cJSON_AddNumberToObject(Tree, "UnknownError", MtSupportGetNumUnknownSlotErrors());
    DIMM_ADDRESS_DETAIL *Item = NULL;

    for (UINT8 i = 0; i < 4; i++) {
      Item = DequeueError(&gDimmErrorQueue);
      if (!Item) {
        break;
      }
      cJSON *Detail = cJSON_CreateObject();
      cJSON_AddNumberToObject(Detail, "Socket", Item->SocketId);
      cJSON_AddNumberToObject(Detail, "MemCtrl", Item->MemCtrlId);
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

    GetStringById(STRING_TOKEN(gCustomTestList[gTestNum].NameStrId), g_wszBuffer, BUF_SIZE);
    UnicodeStrToAsciiStrS(g_wszBuffer, gBuffer, BUF_SIZE);
    cJSON_AddStringToObject(Tree, "NAME", gBuffer);
    cJSON_AddNumberToObject(Tree, "ID", gTestNum + 47);
    switch (mPatternUI.NewSize) {
      case 4: AsciiSPrint(gBuffer, BUF_SIZE, "0x%08X", mPatternUI.NewPattern[0]); break;
      case 8: AsciiSPrint(gBuffer, BUF_SIZE, "0x%016lX", *((UINT64 *)mPatternUI.NewPattern)); break;
      case 16: AsciiSPrint(gBuffer, BUF_SIZE, "0x%08X%08X", mPatternUI.NewPattern[0], mPatternUI.NewPattern[1]); break;
    }
    cJSON_AddStringToObject(Tree, "PATTERN", gBuffer);
    cJSON_AddNumberToObject(Tree, "PROGRESS", gLastPercent);
    // cJSON_AddNumberToObject(Tree, "MEMSTART", gStartAddr);
    // cJSON_AddNumberToObject(Tree, "MEMEND", gEndAddr);
    cJSON_AddNumberToObject(Tree, "STATUS", gTestStatus);
  }
  else if (AsciiStrnCmp("testStart", Cmd->valuestring, 9) == 0) {
    gLastPercent = 0;
    InitErrorQueue(&gDimmErrorQueue);
    gTestPause = FALSE;
    cJSON *Tid = cJSON_GetObjectItemCaseSensitive(Tree, "TID");
    if (Tid) {
      gTestTid = (UINT8)Tid->valueu64;
    }
    cJSON *Id = cJSON_GetObjectItemCaseSensitive(Tree, "ID");
    if (Id == NULL || Id->type != cJSON_Number || Id->valueu64 > gNumCustomTests + 47 || Id->valueu64 < 47) {
      return;
    }
    for (UINT8 i = 0; i < gNumCustomTests; i++) {
      gCustomTestList[i].Enabled = FALSE;
    }
    gCustomTestList[Id->valueu64 - 47].Enabled = TRUE;
    gTestNum                                   = (INT8)(Id->valueu64 - 47);
    gNumPasses                                 = 1;
    gTestStart                                 = TRUE;
    gTestStatus                                = 1;

    cJSON_AddBoolToObject(Tree, "SUCCESS", gTestStart);
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
    cJSON_AddNumberToObject(Tree, "STATUS", gTestStart);
  }
  else if (AsciiStrnCmp("testStop", Cmd->valuestring, 8) == 0) {
    cJSON_AddBoolToObject(Tree, "SUCCESS", TRUE);
    MtSupportAbortTesting();
    gTestStart  = FALSE;  // fix duplicate test
    gTestStatus = 2;
    // gTestStop = TRUE;
  }
  else if (AsciiStrnCmp("areyouok", Cmd->valuestring, 8) == 0) {
    cJSON_AddBoolToObject(Tree, "SUCCESS", true);
    // 关键：MAC/IP 来自“当前接收该包的 NIC”
    if (gJsonCtxSocket != NULL) {
      AsciiSPrint(gBuffer,
                  BUF_SIZE,
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
    gTestReset  = (UINT8)Type->valueu64;
  }
  else if (AsciiStrnCmp("amtStart", Cmd->valuestring, 8) == 0) {
    amtControl(Tree, TRUE);
  }
  return;
}

/**
 * 处理 HW_INFO 命令(原先的 connect命令)
 */
EFI_STATUS OknMT_ProcessJsonCmd_HwInfo(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree)
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
        cJSON_AddStringToObject(pJsTree, pCpuID, pCpuVerInfo);
      }
    }
  } while (0);

  // 3. SLOT_CNT
  cJSON_AddNumberToObject(pJsTree, "SMBIOS_MEM_DEV_CNT", g_numSMBIOSMem);

  // 3. 获取DIMM信息
  cJSON *DimmInfoArray = cJSON_AddArrayToObject(pJsTree, "DIMM_INFO");
  for (UINT8 i = 0; i < g_numSMBIOSMem; i++) {
    cJSON *Info = cJSON_CreateObject();

    UINT8 Socket  = i / 8;
    UINT8 Channel = i % 8;
    UINT8 Dimm    = 0;  // 单通道设计, 只有Dimm 0

    BOOLEAN                  RamPresent    = FALSE;
    UINT8                    Online        = 0;
    INT32                    RamTemp0      = OKN_MAGIC_ERR_NUM;
    INT32                    RamTemp1      = 0;
    INT32                    HubTemp       = 0;
    DIMM_RANK_MAP_OUT_REASON MapOutReason  = DimmRankMapOutMax;
    INT32                    SdramDevWidth = OKN_MAGIC_ERR_NUM;  // SDRAM Device Width
    INT32                    PkgRanksCnt   = OKN_MAGIC_ERR_NUM;  // Number of Package Ranks per DIMM
    INT32                    SdramPkgCnt   = OKN_MAGIC_ERR_NUM;  // SDRAM Chip Count, 不含ECC
    INT32                    EccPkgCnt     = OKN_MAGIC_ERR_NUM;  // ECC Chip Count

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
          Print(L"[OKN_UEFI_ERR] [%s] SpdRead() failed for Skt %d Ch %d Dimm %d: %r\n",
                __func__,
                Socket,
                Channel,
                Dimm,
                Status);
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
