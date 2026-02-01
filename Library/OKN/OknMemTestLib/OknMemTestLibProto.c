/**
 * @file
 *
 */

#include <Library/OKN/OknMemTestLib.h>

OKN_MEMORY_TEST_PROTOCOL *gOknMtProtoPtr   = NULL;
EFI_HANDLE                gOknChosenHandle = NULL;

/**
 * @description: 初始化 全局变量 gOknMtProtoPtr 和 gOknChosenHandle
 * @return EFI_STATUS
 */
EFI_STATUS OknMT_InitProtocol(VOID)
{
  INTN       HandleIndex = 0;  // 默认第一个Handle
  EFI_STATUS Status;
  UINTN      HandleCount;

  Status = OknMT_LocateProtocol(HandleIndex, &gOknMtProtoPtr, &gOknChosenHandle, &HandleCount);
  if (TRUE == EFI_ERROR(Status) || NULL == gOknMtProtoPtr) {
    Print(L"Locate OKN MmeTest protocol failed: %r\n", Status);
    Print(L"Hints:\n");
    Print(L"  - Ensure the DXE driver that installs gOknMemTestProtocolGuid is loaded.\n");
    Print(L"  - Ensure the GUID value and protocol struct ABI match between producer and consumer.\n");
    return Status;
  }
  else {
    Print(L"Found %u handle(s) with OKN MemTest protocol\n", (UINT32)HandleCount);
  }

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_IsDimmPresent(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree, OUT BOOLEAN *pPresent)
{
  EFI_STATUS Status = EFI_SUCCESS;

  cJSON *pSocket  = NULL;
  cJSON *pChannel = NULL;
  cJSON *pDimm    = NULL;

  BOOLEAN IsPresent = FALSE;

  // 1) 参数检查
  if (NULL == pProto || NULL == pProto->IsDimmPresent || NULL == pJsTree || NULL == pPresent) {
    Print(L"[OKN_UEFI_ERR] Protocol/IsDimmPresent() or JSON tree or pPresent is NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  // 2) 取 Socket/Channel/Dimm
  Status = OknMT_GetSocketChannelDImmPtrsFromJson(pProto, pJsTree, pSocket, pChannel, pDimm);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] OknMT_GetSocketChannelDImmPtrsFromJson failed: %r\n", Status);
    return Status;
  }

  // 3) 调用 IsDimmPresent
  Status =
      pProto->IsDimmPresent((UINT8)pSocket->valueu64, (UINT8)pChannel->valueu64, (UINT8)pDimm->valueu64, &IsPresent);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] IsDimmPresent() failed: %r\n", Status);
    return Status;
  }

  // 4) 输出结果
  *pPresent = IsPresent;

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_GetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
{
  return OknMT_GetMemConfigFunc(pProto, FALSE, pJsTree);
}

EFI_STATUS OknMT_GetMemConfigReal(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree)
{
  return OknMT_GetMemConfigFunc(pProto, TRUE, pJsTree);
}

EFI_STATUS OknMT_SetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree)
{
  EFI_STATUS Status;

  if (NULL == pProto) {
    return EFI_INVALID_PARAMETER;
  }
  if (NULL == pProto->SetMemConfig) {
    return EFI_UNSUPPORTED;
  }

  OKN_MEMORY_CONFIGURATION Cfg;
  ZeroMem(&Cfg, sizeof(Cfg));

  if (FALSE == EFI_ERROR(Status)) {
    Status = OknMT_GetMemCfgTFromJson(pJsTree, &Cfg);
    if (TRUE == EFI_ERROR(Status)) {
      Print(L"[OKN_UEFI_ERR] OknMT_GetMemCfgTFromJson failed: %r\n", Status);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_SetAmtConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree)
{
  EFI_STATUS            Status;
  OKN_AMT_CONFIGURATION AmtCfg;
  cJSON                *AmtOptArr;
  UINT32                Options;
  UINT16                Loops;
  UINT8                 AmtPpr;
  INTN                  OptCount;

  // 0) 基本检查
  if (NULL == pProto || NULL == pProto->SetAmtConfig || NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] Protocol/SetAmtConfig or JSON is NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem(&AmtCfg, sizeof(AmtCfg));

  // 1) 解析 OPTIONS / LOOPS / PPR
  Status = OknMT_JsonGetU32FromObject(pJsTree, "OPTIONS", &Options);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] Missing/invalid OPTIONS\n");
    return EFI_INVALID_PARAMETER;
  }

  Status = OknMT_JsonGetU16FromObject(pJsTree, "LOOPS", &Loops);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] Missing/invalid LOOPS\n");
    return EFI_INVALID_PARAMETER;
  }
  Status = OknMT_JsonGetU8FromObject(pJsTree, "AMT_PPR", &AmtPpr);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] Missing/invalid AMT_PPR\n");
    return EFI_INVALID_PARAMETER;
  }

  AmtCfg.AdvMemTestOptions = Options;
  AmtCfg.AdvMemTestLoops   = Loops;
  AmtCfg.AdvMemTestPpr     = AmtPpr;

  // 2) 解析 AMT_OPT_ARR
  AmtOptArr = cJSON_GetObjectItemCaseSensitive((cJSON *)pJsTree, "AMT_OPT_ARR");
  if (NULL == AmtOptArr || !cJSON_IsArray(AmtOptArr)) {
    Print(L"[OKN_UEFI_ERR] Missing/invalid AMT_OPT_ARR\n");
    return EFI_INVALID_PARAMETER;
  }

  OptCount = cJSON_GetArraySize(AmtOptArr);
  if (OptCount < 0) {
    Print(L"[OKN_UEFI_ERR] AMT_OPT_ARR size invalid\n");
    return EFI_INVALID_PARAMETER;
  }

  if (OptCount > OKN_AMT_OPT_COUNT) {
    // 你也可以选择只取前 20 个，但这里我按“输入非法”处理，避免静默截断导致误配置
    Print(L"[OKN_UEFI_ERR] AMT_OPT_ARR too large: %d (max %d)\n", OptCount, OKN_AMT_OPT_COUNT);
    return EFI_INVALID_PARAMETER;
  }

  for (INTN i = 0; i < OptCount; ++i) {
    cJSON *One       = cJSON_GetArrayItem(AmtOptArr, i);
    UINT64 Pattern64 = 0;

    if (NULL == One || !cJSON_IsArray(One)) {
      Print(L"[OKN_UEFI_ERR] AMT_OPT_ARR[%d] is not an array\n", i);
      return EFI_INVALID_PARAMETER;
    }

    if (cJSON_GetArraySize(One) < AMT_INNER_MIN_ITEMS) {
      Print(L"[OKN_UEFI_ERR] AMT_OPT_ARR[%d] item count < %d\n", i, AMT_INNER_MIN_ITEMS);
      return EFI_INVALID_PARAMETER;
    }

    // 按映射顺序填充
    // clang-format off
	// AdvMemTestCondition
    Status = OknMT_JsonGetU8FromArray(One, AMT_OPT_IDX_COND, &AmtCfg.AmtOptArr[i].AdvMemTestCondition);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // PmicVddLevel
    Status = OknMT_JsonGetU16FromArray(One, AMT_OPT_IDX_VDD, &AmtCfg.AmtOptArr[i].PmicVddLevel);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // PmicVddQLevel
    Status = OknMT_JsonGetU16FromArray(One, AMT_OPT_IDX_VDDQ, &AmtCfg.AmtOptArr[i].PmicVddQLevel);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // TwrValue
    Status = OknMT_JsonGetU8FromArray(One, AMT_OPT_IDX_TWR, &AmtCfg.AmtOptArr[i].TwrValue);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // TrefiValue
    Status = OknMT_JsonGetU16FromArray(One, AMT_OPT_IDX_TREFI, &AmtCfg.AmtOptArr[i].TrefiValue);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // AdvMemTestCondPause
    Status = OknMT_JsonGetU32FromArray(One, AMT_OPT_IDX_PAUSE, &AmtCfg.AmtOptArr[i].AdvMemTestCondPause);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // BgInterleave
    Status = OknMT_JsonGetU8FromArray(One, AMT_OPT_IDX_BGINTLV, &AmtCfg.AmtOptArr[i].BgInterleave);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // AddressMode
    Status = OknMT_JsonGetU8FromArray(One, AMT_OPT_IDX_ADDRMODE, &AmtCfg.AmtOptArr[i].AddressMode);
    if (TRUE == EFI_ERROR(Status)) { return EFI_INVALID_PARAMETER; }
    // clang-format on

    // Pattern: number 或 string
    Status = OknMT_JsonGetU64(cJSON_GetArrayItem(One, AMT_OPT_IDX_PATTERN), &Pattern64);
    if (TRUE == EFI_ERROR(Status)) {
      Print(L"[OKN_UEFI_ERR] AMT_OPT_ARR[%d] Pattern invalid\n", i);
      return EFI_INVALID_PARAMETER;
    }
    AmtCfg.AmtOptArr[i].Pattern = Pattern64;
  }

  // 3) 下发给 BIOS
  Status = pProto->SetAmtConfig(&AmtCfg);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] SetAmtConfig() failed: %r\n", Status);
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_GetDimmRankMapOutReason(IN OKN_MEMORY_TEST_PROTOCOL  *pProto,
                                         IN CONST cJSON               *pJsTree,
                                         OUT DIMM_RANK_MAP_OUT_REASON *pReason)
{
  EFI_STATUS Status = EFI_SUCCESS;

  cJSON *pSocket  = NULL;
  cJSON *pChannel = NULL;
  cJSON *pDimm    = NULL;

  // 1) 参数检查
  if (NULL == pProto || NULL == pProto->GetDisReason || NULL == pJsTree || NULL == pReason) {
    Print(L"[OKN_UEFI_ERR] [%s] Protocol/GetDisReason() or JSON tree or pReason is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  // 2) 取 Socket/Channel/Dimm
  Status = OknMT_GetSocketChannelDImmPtrsFromJson(pProto, pJsTree, pSocket, pChannel, pDimm);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] OknMT_GetSocketChannelDImmPtrsFromJson failed: %r\n", __func__, Status);
    return Status;
  }

  // 3) 调用 GetDimmTemp
  DIMM_RANK_MAP_OUT_REASON Reason;
  Status = pProto->GetDisReason((UINT8)pSocket->valueu64, (UINT8)pChannel->valueu64, (UINT8)pDimm->valueu64, &Reason);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] GetDisReason() failed: %r\n", __func__, Status);
    return Status;
  }
  *pReason = Reason;

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_GetDimmTemperature(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree, OUT UINT8 *pTSensor0)
{
  EFI_STATUS Status = EFI_SUCCESS;

  cJSON *pSocket  = NULL;
  cJSON *pChannel = NULL;
  cJSON *pDimm    = NULL;

  // 1) 参数检查
  if (NULL == pProto || NULL == pProto->GetDimmTemp || NULL == pJsTree || NULL == pTSensor0) {
    Print(L"[OKN_UEFI_ERR] [%s] Protocol/GetDimmTemp() or JSON tree or pTSensor0 is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  // 2) 取 Socket/Channel/Dimm
  Status = OknMT_GetSocketChannelDImmPtrsFromJson(pProto, pJsTree, pSocket, pChannel, pDimm);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] OknMT_GetSocketChannelDImmPtrsFromJson failed: %r\n", __func__, Status);
    return Status;
  }

  // 3) 调用 GetDimmTemp
  UINT8 Ts0 = 0, Ts1 = 0, Hub = 0;
  Status = pProto->GetDimmTemp((UINT8)pSocket->valueu64,
                               (UINT8)pChannel->valueu64,
                               (UINT8)pDimm->valueu64,
                               &Ts0,
                               &Ts1,
                               &Hub);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] GetDimmTemp() failed: %r\n", __func__, Status);
    return Status;
  }

  *pTSensor0 = Ts0;

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_ReadSpdToJson(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree)
{
  EFI_STATUS Status = EFI_SUCCESS;

  cJSON *Socket  = NULL;
  cJSON *Channel = NULL;
  cJSON *Dimm    = NULL;

  UINT8 SpdDataArr[1024];
  INTN  ReadSpdDataLen = -1;

  // 1) 参数检查
  if (NULL == pProto || NULL == pProto->SpdRead || NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] Protocol/SpdRead() or JSON tree is NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  // 2) 取 Socket/Channel/Dimm
  Status = OknMT_GetSocketChannelDImmPtrsFromJson(pProto, pJsTree, Socket, Channel, Dimm);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] OknMT_GetSocketChannelDImmPtrsFromJson failed: %r\n", Status);
    return Status;
  }

  // 3) 读 SPD
  ZeroMem(SpdDataArr, sizeof(SpdDataArr));
  Status = pProto->SpdRead((UINT8)Socket->valueu64,
                           (UINT8)Channel->valueu64,
                           (UINT8)Dimm->valueu64,
                           SpdDataArr,
                           &ReadSpdDataLen);
  if (EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] SpdRead() failed: %r\n", Status);
    return Status;
  }

  // 3.1) 长度合法性
  if (ReadSpdDataLen <= 0 || ReadSpdDataLen > (INTN)sizeof(SpdDataArr)) {
    Print(L"[OKN_UEFI_ERR] Invalid ReadSpdDataLen = %d\n", ReadSpdDataLen);
    return EFI_DEVICE_ERROR;
  }

  // 4) 写 SPD_LEN
  do {
    cJSON *LenItem = cJSON_GetObjectItemCaseSensitive(pJsTree, "SPD_LEN");
    if (NULL == LenItem) {
      if (NULL == cJSON_AddNumberToObject(pJsTree, "SPD_LEN", (double)ReadSpdDataLen)) {
        Print(L"[OKN_UEFI_ERR] Add SPD_LEN failed\n");
        return EFI_OUT_OF_RESOURCES;
      }
    }
    else {
      // 已存在则尽量更新（如果类型不对就替换）
      if (cJSON_IsNumber(LenItem)) {
        LenItem->valueu64 = (UINT64)ReadSpdDataLen;
      }
      else {
        cJSON *NewLen = cJSON_CreateNumber((double)ReadSpdDataLen);
        if (NULL == NewLen) {
          return EFI_OUT_OF_RESOURCES;
        }
        cJSON_ReplaceItemInObjectCaseSensitive(pJsTree, "SPD_LEN", NewLen);
      }
    }
  } while (0);

  // 5) Base64 编码并写 SPD String
  do {
    CHAR8 *pSpdB64Str = NULL;
    UINTN  B64NeedSz  = 0;
    UINTN  B64OutSz   = 0;

    // 第一次探测所需 buffer 大小
    Status = Base64Encode(SpdDataArr, (UINTN)ReadSpdDataLen, NULL, &B64NeedSz);
    if (Status != EFI_BUFFER_TOO_SMALL || 0 == B64NeedSz) {
      Print(L"[OKN_UEFI_ERR] Base64Encode size query failed: %r, need=%u\n", Status, (UINT32)B64NeedSz);
      return (EFI_SUCCESS == Status) ? EFI_DEVICE_ERROR : Status;
    }

    pSpdB64Str = AllocateZeroPool(B64NeedSz);
    if (NULL == pSpdB64Str) {
      Print(L"[OKN_UEFI_ERR] Allocate Base64 buffer failed, size=%u\n", (UINT32)B64NeedSz);
      return EFI_OUT_OF_RESOURCES;
    }

    B64OutSz = B64NeedSz;
    Status   = Base64Encode(SpdDataArr, (UINTN)ReadSpdDataLen, pSpdB64Str, &B64OutSz);
    if (EFI_ERROR(Status)) {
      Print(L"[OKN_UEFI_ERR] Base64Encode failed: %r\n", Status);
      FreePool(pSpdB64Str);
      return Status;
    }

    // 写入/更新 JSON 的 SPD 字段
    cJSON *SpdItem = cJSON_GetObjectItemCaseSensitive(pJsTree, "SPD");
    if (NULL == SpdItem) {
      if (NULL == cJSON_AddStringToObject(pJsTree, "SPD", pSpdB64Str)) {
        Print(L"[OKN_UEFI_ERR] Add SPD string failed\n");
        FreePool(pSpdB64Str);
        return EFI_OUT_OF_RESOURCES;
      }
    }
    else {
      if (cJSON_IsString(SpdItem)) {
        cJSON_SetValuestring(SpdItem, pSpdB64Str);
      }
      else {
        cJSON *NewSpd = cJSON_CreateString(pSpdB64Str);
        if (NULL == NewSpd) {
          FreePool(pSpdB64Str);
          return EFI_OUT_OF_RESOURCES;
        }
        cJSON_ReplaceItemInObjectCaseSensitive(pJsTree, "SPD", NewSpd);
      }
    }

    FreePool(pSpdB64Str);
  } while (0);

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_TranslatedAddressFromSystemToDimm(IN OKN_MEMORY_TEST_PROTOCOL *pProto,
                                                   IN UINTN                     SystemAddress,
                                                   OUT OKN_DIMM_ADDRESS_DETAIL *pTranslatedAddress)
{
  EFI_STATUS Status = EFI_SUCCESS;

  // 1) 参数检查
  if (NULL == pProto || NULL == pProto->AddrSys2Dimm || NULL == pTranslatedAddress) {
    Print(L"[OKN_UEFI_ERR] [%s] Protocol/AddrSys2Dimm() or pTranslatedAddress is NULL\n", __func__);
    return EFI_INVALID_PARAMETER;
  }

  // 2) 调用 AddrSys2Dimm
  OKN_DIMM_ADDRESS_DETAIL TranslatedAddr;
  ZeroMem(&TranslatedAddr, sizeof(OKN_DIMM_ADDRESS_DETAIL));
  Status = pProto->AddrSys2Dimm(SystemAddress, &TranslatedAddr);
  if (TRUE == EFI_ERROR(Status)) {
    Print(L"[OKN_UEFI_ERR] [%s] AddrSys2Dimm() failed: %r\n", __func__, Status);
    return Status;
  }

  // 3) 输出结果
  CopyMem(pTranslatedAddress, &TranslatedAddr, sizeof(OKN_DIMM_ADDRESS_DETAIL));

  return EFI_SUCCESS;
}
