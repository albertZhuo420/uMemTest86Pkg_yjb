/**
 * @file
 *
 */

#include <Library/MemTestSupportLib.h>
#include <Library/OKN/OknDdr4SpdLib.h>
#include <Library/OKN/OknMemTestLib.h>
#include <Library/OKN/OknUdp4SocketLib.h>
#include <Protocol/Smbios.h>

#define CMD_ENTRY(_name, _handler) {(_name), (_handler)}

extern struct {
  UINT32 CurPattern[4];
  UINT32 NewPattern[4];
  UINT8  CurSize;
  UINT8  NewSize;
} mPatternUI;  // current pattern | 这个变量的结构体字段不能做任何修改, uMemTest86Pkg/Ui.c文件中

#if 1  // OKN 前置定义区域 ON
/**STATIC函数 前置定义区域1**************************************************************/
STATIC UINTN      OknMT_CmdTableCount(VOID);
STATIC EFI_STATUS OknMT_SmbiosGetOptionalStringByIndex(IN CONST CHAR8   *OptionalStrStart,
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
/**GLOBAL/STATIC变量 前置定义区域1********************************************************/
UINTN                gOknLastPercent;  // 用在uMemTest86Pkg/Ui.c 文件中: mLastPercent;
BOOLEAN              gOKnSkipWaiting = FALSE;
BOOLEAN              gOknTestStart   = FALSE;
BOOLEAN              gOknTestPause   = FALSE;
OKN_EFI_RESET_TYPE   gOknTestReset   = Okn_EfiResetNone;
OKN_TEST_STATUS_TYPE gOknTestStatus  = OKN_TST_Unknown;
INT8                 gOknMT86TestID  = -1;  // 这个是MT86真实的TestID [0 ... 15]

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
/*************************************************************************************************/
#endif  // OKN 前置定义区域 OFF

EFI_STATUS
OknMT_DispatchJsonCmd(IN OKN_MEMORY_TEST_PROTOCOL *Proto, IN OUT cJSON *Tree, OUT OKN_EFI_RESET_TYPE *OutResetType)
{
  CHAR8 AsciiSPrintBuffer[OKN_BUF_SIZE] = {0};

  if (NULL == Tree) {
    return EFI_INVALID_PARAMETER;
  }

  if (OutResetType != NULL) {
    *OutResetType = Okn_EfiResetNone;
  }

  cJSON *Cmd = cJSON_GetObjectItemCaseSensitive(Tree, "CMD");
  if (NULL == Cmd || Cmd->type != cJSON_String || NULL == Cmd->valuestring) {
    // 这里你可以用 OknMT_JsonSetBool/JsonSetString（如果你已有封装）
    OknMT_JsonSetBool(Tree, "SUCCESS", FALSE);
    OknMT_JsonSetString(Tree, "ERROR", "Missing/invalid CMD (string)");
    return EFI_INVALID_PARAMETER;
  }

  OKN_MT_CMD_CTX Ctx;
  ZeroMem(&Ctx, sizeof(Ctx));
  Ctx.Proto    = Proto;
  Ctx.Tree     = Tree;
  Ctx.ResetReq = FALSE;

  CONST CHAR8 *CmdStr = (CONST CHAR8 *)Cmd->valuestring;

  for (UINTN i = 0; i < OknMT_CmdTableCount(); i++) {
    if (OKN_STRING_EQUAL == AsciiStrCmp(CmdStr, gOknCmdTable[i].CmdName)) {
      EFI_STATUS Status = gOknCmdTable[i].Handler(&Ctx);

      // 统一回显 SUCCESS（如果你希望所有命令都带 SUCCESS）
      OknMT_JsonSetBool(Tree, "SUCCESS", EFI_ERROR(Status) ? FALSE : TRUE);

      if (FALSE == EFI_ERROR(Status) && TRUE == Ctx.ResetReq) {
        if (OutResetType != NULL) {
          *OutResetType = Ctx.ResetType;
        }
      }

      return Status;
    }
  }

  OknMT_JsonSetBool(Tree, "SUCCESS", FALSE);
  AsciiSPrint(AsciiSPrintBuffer, OKN_BUF_SIZE, "Unsupported CMD: %s", Cmd->valuestring);
  OknMT_JsonSetString(Tree, "ERROR", AsciiSPrintBuffer);

  return EFI_UNSUPPORTED;
}

#if 1  // STATIC函数实现区域 ON
STATIC UINTN OknMT_CmdTableCount(VOID)
{
  return sizeof(gOknCmdTable) / sizeof(gOknCmdTable[0]);
}

// 从 SMBIOS Optional String Area(以 '\0\0' 结束)中, 按 1-based Index 获取字符串指针.
// Index == 0: SMBIOS 语义为 "Not Specified", 返回 EFI_NOT_FOUND(更合理)
// 成功: *OutStr 指向原始记录内存(不拷贝)
STATIC EFI_STATUS OknMT_SmbiosGetOptionalStringByIndex(IN CONST CHAR8   *OptionalStrStart,
                                                       IN UINT8          Index,
                                                       OUT CONST CHAR8 **OutStr)
{
  if (NULL == OptionalStrStart || NULL == OutStr) {
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
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_SetAmtConfig(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Status(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_MT86Status(Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Abort(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  Cmd_MT86Abort();
  return EFI_SUCCESS;
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_MT86Start(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_MT86Start(Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_Reply9527(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_Reply9527(Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_HwInfo(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_HwInfo(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_ReadSPD(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_ReadSPD(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfig(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_GetMemConfig(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_GetMemConfigReal(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_GetMemConfigReal(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_SetMemConfig(OKN_MT_CMD_CTX *Ctx)
{
  Print(L"| = %a = |\n", __FUNCTION__);
  return Cmd_SetMemConfig(Ctx->Proto, Ctx->Tree);
}

STATIC EFI_STATUS OknMT_ProcessJsonCmd_ResetSystem(OKN_MT_CMD_CTX *Ctx)
{
  EFI_STATUS     Status;
  EFI_RESET_TYPE Rt = EfiResetCold;

  Print(L"| = %a = |\n", __FUNCTION__);

  Status = Cmd_ResetSystem(Ctx->Tree, &Rt);
  if (FALSE == EFI_ERROR(Status)) {
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

  cJSON                  *ErrorInfo = cJSON_AddArrayToObject(pJsTree, "ERRORINFO");
  OKN_DIMM_ADDRESS_DETAIL Item;
  for (UINT8 i = 0; i < 4; i++) {
    ZeroMem(&Item, sizeof(OKN_DIMM_ADDRESS_DETAIL));

    Status = OknMT_DequeueErrorCopy(&gOknDimmErrorQueue, &Item);
    if (EFI_NOT_FOUND == Status) {
      break;
    }
    cJSON *Detail = cJSON_CreateObject();
    cJSON_AddNumberToObject(Detail, "Socket", Item.AddrDetail.SocketId);
    cJSON_AddNumberToObject(Detail, "MemCtrl", Item.AddrDetail.MemCtrlId);
    cJSON_AddNumberToObject(Detail, "Channel", Item.AddrDetail.MemCtrlId * 2 + Item.AddrDetail.ChannelId);
    cJSON_AddNumberToObject(Detail, "Dimm", 0);
    cJSON_AddNumberToObject(Detail, "Rank", Item.AddrDetail.RankId);
    cJSON_AddNumberToObject(Detail, "SubCh", Item.AddrDetail.SubChId);
    cJSON_AddNumberToObject(Detail, "BG", Item.AddrDetail.BankGroup);
    cJSON_AddNumberToObject(Detail, "Bank", Item.AddrDetail.Bank);
    cJSON_AddNumberToObject(Detail, "Row", Item.AddrDetail.Row);
    cJSON_AddNumberToObject(Detail, "Col", Item.AddrDetail.Column);
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
  gOknTestStatus = OKN_TST_Aborted;

  return;
}

/**
 * 处理 testStart 命令 | 改成: MT86_Start
 */
STATIC EFI_STATUS Cmd_MT86Start(IN cJSON *pJsTree)
{
  if (NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  gNumPasses      = 1;
  gOknLastPercent = 0;
  gOknTestPause   = FALSE;

  OknMT_InitErrorQueue(&gOknDimmErrorQueue);

  cJSON *Id = cJSON_GetObjectItemCaseSensitive(pJsTree, "ID");
  if (NULL == Id || Id->type != cJSON_Number || Id->valueu64 > gNumCustomTests + 47 || Id->valueu64 < 47) {
    Print(L"[OKN_UEFI_ERR] [%s] Invalid test ID\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  for (UINT8 i = 0; i < gNumCustomTests; i++) {
    gCustomTestList[i].Enabled = FALSE;
  }
  gCustomTestList[Id->valueu64 - 47].Enabled = TRUE;

  gOknMT86TestID = (INT8)(Id->valueu64 - 47);  // ?
  gOknTestStart  = TRUE;                       // ?
  gOknTestStatus = OKN_TST_Testing;            // ?

  return EFI_SUCCESS;
}

/**
 * 处理 areyouok 命令 | 改成: Reply_9527
 */
STATIC EFI_STATUS Cmd_Reply9527(OUT cJSON *pJsTree)
{
  CHAR8 AsciiSPrintBuffer[OKN_BUF_SIZE] = {0};

  if (NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] [%s] JSON tree is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  // 关键: MAC/IP 来自"当前接收该包的 NIC"
  if (gOknJsonCtxUdpEdp != NULL) {
    AsciiSPrint(AsciiSPrintBuffer,
                OKN_BUF_SIZE,
                "%02x:%02x:%02x:%02x:%02x:%02x",
                gOknJsonCtxUdpEdp->NicMac[0],
                gOknJsonCtxUdpEdp->NicMac[1],
                gOknJsonCtxUdpEdp->NicMac[2],
                gOknJsonCtxUdpEdp->NicMac[3],
                gOknJsonCtxUdpEdp->NicMac[4],
                gOknJsonCtxUdpEdp->NicMac[5]);
    cJSON_AddStringToObject(pJsTree, "MAC", AsciiSPrintBuffer);

    if (gOknJsonCtxUdpEdp->NicIpValid) {
      AsciiSPrint(AsciiSPrintBuffer,
                  OKN_BUF_SIZE,
                  "%d.%d.%d.%d",
                  gOknJsonCtxUdpEdp->NicIp.Addr[0],
                  gOknJsonCtxUdpEdp->NicIp.Addr[1],
                  gOknJsonCtxUdpEdp->NicIp.Addr[2],
                  gOknJsonCtxUdpEdp->NicIp.Addr[3]);
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
        CONST CHAR8 *pCpuID, *pCpuVerInfo;  // 允许改指针指向哪里, 不允许改指针指向的内容
        Status = OknMT_SmbiosGetOptionalStringByIndex(
            (CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
            pSmbiosType4Record->Socket,
            &pCpuID);
        Status = OknMT_SmbiosGetOptionalStringByIndex(
            (CHAR8 *)((UINT8 *)pSmbiosType4Record + pSmbiosType4Record->Hdr.Length),
            pSmbiosType4Record->ProcessorVersion,
            &pCpuVerInfo);
        cJSON_AddStringToObject(pJsTree, pCpuID, pCpuVerInfo);
      }
    }
  } while (0);

  /**
   * 3. SLOT_CNT
   * 通过 E_MEMORY_DEVICE 来获取, 全局搜
   */
  cJSON_AddNumberToObject(pJsTree, "SMBIOS_MEM_DEV_CNT", g_numSMBIOSMem);

  // 4. 获取DIMM信息
  cJSON *DimmInfoArray = cJSON_AddArrayToObject(pJsTree, "DIMM_INFO");
  INT32  NumSMBIOSMem  = (OKN_MAX_DIMM_CNT == g_numSMBIOSMem) ? g_numSMBIOSMem : OKN_MAX_DIMM_CNT;
  for (UINT8 i = 0; i < NumSMBIOSMem; i++) {
    cJSON *Info = cJSON_CreateObject();

    UINT8 Socket  = i / 8;
    UINT8 Channel = i % 8;
    UINT8 Dimm    = 0;  // 单通道设计, 只有Dimm 0

    BOOLEAN                  RamPresent    = FALSE;
    UINT8                    Online        = 0;
    INT32                    RamTemp0      = OKN_MAGIC_NUMBER;
    INT8                     RamTemp1      = 0;
    INT8                     HubTemp       = 0;
    DIMM_RANK_MAP_OUT_REASON MapOutReason  = DimmRankMapOutMax;
    INT32                    SdramDevWidth = OKN_MAGIC_NUMBER;  // SDRAM Device Width
    INT32                    PkgRanksCnt   = OKN_MAGIC_NUMBER;  // Number of Package Ranks per DIMM
    INT32                    SdramPkgCnt   = OKN_MAGIC_NUMBER;  // SDRAM Chip Count, 不含ECC
    INT32                    EccPkgCnt     = OKN_MAGIC_NUMBER;  // ECC Chip Count

    Status = pProto->IsDimmPresent(Socket, Channel, Dimm, &RamPresent);
    Print(L"[OKN_PROTO] N%u:C%u:D%u - IsDimmPresent(): %r\n", Socket, Channel, Dimm, Status);

    if (FALSE == EFI_ERROR(Status)) {  // 在线
      Online = RamPresent ? 1 : 0;
      if (TRUE == RamPresent) {
        // 读取温度
        Status = pProto->GetDimmTemp(Socket, Channel, Dimm, (UINT8 *)&RamTemp0, (UINT8 *)&RamTemp1, (UINT8 *)&HubTemp);
        if (TRUE == EFI_ERROR(Status)) {
          Print(L"[OKN_PROTO_ERROR] N%u:C%u:D%u - GetDimmTemp(): %r\n", Socket, Channel, Dimm, Status);
        }
        // 读取 MAP_OUT_REASON
        Status = pProto->GetDisReason(Socket, Channel, Dimm, &MapOutReason);
        if (TRUE == EFI_ERROR(Status)) {
          Print(L"[OKN_PROTO_ERROR] N%u:C%u:D%u - GetDisReason(): %r\n", Socket, Channel, Dimm, Status);
        }
        // 读取 SPD 数据: SdramDevWidth / PkgRanksCnt / SdramPkgCnt / EccPkgCnt
        UINT8 SpdData[1024] = {0};
        INTN  SpdReadLen    = 0;
        Status              = pProto->SpdRead(Socket, Channel, Dimm, SpdData, &SpdReadLen);
        if (FALSE == EFI_ERROR(Status)) {
          if (DDR4_SPD_LEN == SpdReadLen) {
            // 这里没有检查返回值, 因为我使用了 OKN_MAGIC_ERR_NUM作为错误标志;
            OknDdr4SpdGetSdramDeviceWidthBits(SpdData, DDR4_SPD_LEN, (UINT16 *)&SdramDevWidth);
            OknDdr4SpdGetPackageRanksPerDimm(SpdData, DDR4_SPD_LEN, (UINT8 *)&PkgRanksCnt);
            OknDdr4SpdEstimateTotalDataPackages(SpdData, DDR4_SPD_LEN, (UINT16 *)&SdramPkgCnt);
            OknDdr4SpdEstimateTotalEccPackages(SpdData, DDR4_SPD_LEN, (UINT16 *)&EccPkgCnt);
          }
          else {
            Print(L"[OKN_PROTO_ERROR] N%u:C%u:D%u - SpdRead(): %r\n", Socket, Channel, Dimm, Status);
          }
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

  Status = OknMT_JsonGetU32FromObject(pTree, "TYPE", &TypeU32);
  if (EFI_ERROR(Status)) {
    OknMT_JsonSetString(pTree, "ERROR", "Missing/invalid TYPE (number 0..2)");
    return Status;
  }

  // 只支持 0/1/2，不支持 3（PlatformSpecific）,因为那需要 ResetData GUID
  if (TypeU32 > EfiResetShutdown) {  // EfiResetShutdown == 2
    if (EfiResetPlatformSpecific == TypeU32) {
      OknMT_JsonSetString(pTree,
                          "ERROR",
                          "TYPE=3 (PlatformSpecific) not supported (GUID ResetData disabled by design)");
      return EFI_UNSUPPORTED;
    }
    OknMT_JsonSetString(pTree, "ERROR", "TYPE out of range (0=cold,1=warm,2=shutdown)");
    return EFI_INVALID_PARAMETER;
  }

  *pResetType = (EFI_RESET_TYPE)TypeU32;

  OknMT_JsonSetNumber(pTree, "TYPE", (INTN)TypeU32);  // 规范化回显

  return EFI_SUCCESS;
}
#endif  // STATIC函数实现区域 OFF
