/**
 * @file
 *
 */

#include <Library/OKN/OknMemTestLib.h>

#define OKN_MAX_HANDLES_TO_PRINT 64u

#define PrintAndStop()                                                                                                 \
  do {                                                                                                                 \
    UINTN         EventIndex_tmp;                                                                                      \
    EFI_INPUT_KEY key;                                                                                                 \
    Print(L"[%a:%d] ...\r\n", OknMT_GetFileBaseName(__FILE__), __LINE__);                                              \
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex_tmp);                                                    \
    gST->ConIn->ReadKeyStroke(gST->ConIn, &key);                                                                       \
  } while (0)

EFI_STATUS OknMT_LocateProtocol(IN INTN                        RequestedIndex,
                                OUT OKN_MEMORY_TEST_PROTOCOL **pProto,
                                OUT EFI_HANDLE                *pChosenHandle,
                                OUT UINTN                     *pTotalHandles)
{
  EFI_STATUS  Status;
  EFI_HANDLE *Handles;
  UINTN       Count;
  //   UINTN       i; // PrintHandleSummary() 函数实现了再打开

  if (NULL == pProto || NULL == pChosenHandle || NULL == pTotalHandles) {
    return EFI_INVALID_PARAMETER;
  }

  *pProto        = NULL;
  *pChosenHandle = NULL;
  *pTotalHandles = 0;

  Handles = NULL;
  Count   = 0;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gOknMemTestProtocolGuid, NULL, &Count, &Handles);
  if (TRUE == EFI_ERROR(Status)) {
    return Status;
  }

  *pTotalHandles = Count;

#if 0  // PrintHandleSummary() 函数实现了再打开
  Print(L"Found %u handle(s) with OKN MemTest protocol\n", (UINT32)Count);
  for (i = 0; i < Count && i < OKN_MAX_HANDLES_TO_PRINT; ++i) {
    Print(L"  [%u] ", (UINT32)i);
    PrintHandleSummary(Handles[i]);
    Print(L"\n");
  }
  if (Count > OKN_MAX_HANDLES_TO_PRINT) {
    Print(L"  ... (%u more not shown)\n", (UINT32)(Count - OKN_MAX_HANDLES_TO_PRINT));
  }
#endif

  if (0 == Count) {
    FreePool(Handles);
    return EFI_NOT_FOUND;
  }

  if (RequestedIndex < 0) {
    if (Count > 1) {
      Print(L"WARNING: multiple protocol instances detected; defaulting to 0\n");
    }
    RequestedIndex = 0;
  }

  if ((UINTN)RequestedIndex >= Count) {
    FreePool(Handles);
    return EFI_INVALID_PARAMETER;
  }

  *pChosenHandle = Handles[RequestedIndex];

  Status = gBS->HandleProtocol(*pChosenHandle, &gOknMemTestProtocolGuid, (VOID **)pProto);
  FreePool(Handles);

  return Status;
}

VOID OknMT_PrintHandleSummary(IN EFI_HANDLE Handle)
{
  /**
   * TODO:
   */
  return;
}

VOID OknMT_PrintMemCfg(IN CONST OKN_MEMORY_CONFIGURATION *pCfg)
{
  Print(L"DdrFreqLimit=%u\n", pCfg->DdrFreqLimit);
  Print(L"tCAS=%u tRCD=%u tRP=%u tRAS=%u tCWL=%u\n", pCfg->tCAS, pCfg->tRCD, pCfg->tRP, pCfg->tRAS, pCfg->tCWL);
  Print(L"tRTP=%u tWR=%u tRRD=%u tFAW=%u tREFI=%u\n", pCfg->tRTP, pCfg->tWR, pCfg->tRRD, pCfg->tFAW, pCfg->tREFI);
  Print(L"tWTR=%u tRFC1=%u tRC=%u tCCD=%u\n", pCfg->tWTR, pCfg->tRFC1, pCfg->tRC, pCfg->tCCD);
  Print(L"CommandTiming=%u\n", pCfg->CommandTiming);
  Print(L"PMIC: Vpp=%u Vdd=%u VddQ=%u\n", pCfg->DfxPmicVpp, pCfg->DfxPmicVdd, pCfg->DfxPmicVddQ);
  Print(L"EnableRMT=%u EnforcePopulationPor=%u ErrorCheckScrub=%u\n",
        pCfg->EnableRMT,
        pCfg->EnforcePopulationPor,
        pCfg->ErrorCheckScrub);
  //   Print(L"PprType=%u AdvMemTestPpr=%u\n", pCfg->PprType, pCfg->AdvMemTestPpr);
  Print(L"ECC: DirectoryModeEn=%u DdrEccEnable=%u DdrEccCheck=%u\n",
        pCfg->DirectoryModeEn,
        pCfg->DdrEccEnable,
        pCfg->DdrEccCheck);
  return;
}

EFI_STATUS OknMT_SetMemCfgToJson(IN CONST OKN_MEMORY_CONFIGURATION *pCfg, OUT cJSON *pJsTree)
{
  if (NULL == pCfg || NULL == pJsTree) {
    return EFI_INVALID_PARAMETER;
  }

  GetSetJsonObjInt(pJsTree, "Freq", pCfg->DdrFreqLimit);
  GetSetJsonObjInt(pJsTree, "tCL", pCfg->tCAS);
  GetSetJsonObjInt(pJsTree, "tRCD", pCfg->tRCD);
  GetSetJsonObjInt(pJsTree, "tRP", pCfg->tRP);
  GetSetJsonObjInt(pJsTree, "tRAS", pCfg->tRAS);
  GetSetJsonObjInt(pJsTree, "tCWL", pCfg->tCWL);
  GetSetJsonObjInt(pJsTree, "tRTP", pCfg->tRTP);
  GetSetJsonObjInt(pJsTree, "tWR", pCfg->tWR);
  GetSetJsonObjInt(pJsTree, "tRRD_S", pCfg->tRRD);
  GetSetJsonObjInt(pJsTree, "tFAW", pCfg->tFAW);
  GetSetJsonObjInt(pJsTree, "tREFI", pCfg->tREFI);
  GetSetJsonObjInt(pJsTree, "tWTR_S", pCfg->tWTR);
  GetSetJsonObjInt(pJsTree, "tRFC", pCfg->tRFC1);
  GetSetJsonObjInt(pJsTree, "tRC", pCfg->tRC);
  GetSetJsonObjInt(pJsTree, "tCCD", pCfg->tCCD);
  GetSetJsonObjInt(pJsTree, "CommandRate", pCfg->CommandTiming);
  // Voltages
  GetSetJsonObjInt(pJsTree, "Vdd", pCfg->DfxPmicVdd);
  GetSetJsonObjInt(pJsTree, "Vddq", pCfg->DfxPmicVddQ);
  GetSetJsonObjInt(pJsTree, "Vpp", pCfg->DfxPmicVpp);
  // MISC
  GetSetJsonObjInt(pJsTree, "EnRMT", pCfg->EnableRMT);
  GetSetJsonObjInt(pJsTree, "Por", pCfg->EnforcePopulationPor);
  GetSetJsonObjInt(pJsTree, "Ecs", pCfg->ErrorCheckScrub);  // unsupported in DDR4
  GetSetJsonObjInt(pJsTree, "PprType", pCfg->PprType);
  //   GetSetJsonObjInt(pJsTree, "AmtPpr", pCfg->AdvMemTestPpr);
  // ECC
  GetSetJsonObjInt(pJsTree, "DirectoryModeEn", pCfg->DirectoryModeEn);
  GetSetJsonObjInt(pJsTree, "DdrEccEnable", pCfg->DdrEccEnable);
  GetSetJsonObjInt(pJsTree, "DdrEccCheck", pCfg->DdrEccCheck);

#if 0
    GetSetJsonObjInt(pJsTree, "AmtRetry", pCfg->AdvMemTestRetryAfterRepair);
	GetSetJsonObjInt(pJsTree, "tRRDR", pCfg->tRRDR);
	GetSetJsonObjInt(pJsTree, "tRWDR", pCfg->tRWDR);
	GetSetJsonObjInt(pJsTree, "tWRDR", pCfg->tWRDR);
	GetSetJsonObjInt(pJsTree, "tWWDR", pCfg->tWWDR);
	GetSetJsonObjInt(pJsTree, "tRRSG", pCfg->tRRSG);
	GetSetJsonObjInt(pJsTree, "tRWSG", pCfg->tRWSG);
	GetSetJsonObjInt(pJsTree, "tWRSG", pCfg->tWRSG);
	GetSetJsonObjInt(pJsTree, "tWWSG", pCfg->tWWSG);
	GetSetJsonObjInt(pJsTree, "tRRDD", pCfg->tRRDD);
	GetSetJsonObjInt(pJsTree, "tRWDD", pCfg->tRWDD);
	GetSetJsonObjInt(pJsTree, "tWRDD", pCfg->tWRDD);
	GetSetJsonObjInt(pJsTree, "tWWDD", pCfg->tWWDD);
	GetSetJsonObjInt(pJsTree, "tRRSR", pCfg->tRRSR);
	GetSetJsonObjInt(pJsTree, "tRWSR", pCfg->tRWSR);
	GetSetJsonObjInt(pJsTree, "tWRSR", pCfg->tWRSR);
	GetSetJsonObjInt(pJsTree, "tWWSR", pCfg->tWWSR);
	GetSetJsonObjInt(pJsTree, "tRRDS", pCfg->tRRDS);
	GetSetJsonObjInt(pJsTree, "tRWDS", pCfg->tRWDS);
	GetSetJsonObjInt(pJsTree, "tWRDS", pCfg->tWRDS);
	GetSetJsonObjInt(pJsTree, "tWWDS", pCfg->tWWDS);
#endif

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_GetMemCfgTFromJson(IN CONST cJSON *pJsTree, OUT OKN_MEMORY_CONFIGURATION *pCfg)
{
  if (NULL == pCfg || NULL == pJsTree) {
    return EFI_INVALID_PARAMETER;
  }

#if 1  // 数据 Get 区域 ON
  // clang-format off
  cJSON *Freq_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "Freq");
  cJSON *tCL_pVal         = cJSON_GetObjectItemCaseSensitive(pJsTree, "tCL");
  cJSON *tRCD_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "tRCD");
  cJSON *tRP_pVal         = cJSON_GetObjectItemCaseSensitive(pJsTree, "tRP");
  cJSON *tRAS_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "tRAS");
  cJSON *tCWL_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "tCWL");
  cJSON *tRTP_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "tRTP");
  cJSON *tWR_pVal         = cJSON_GetObjectItemCaseSensitive(pJsTree, "tWR");
  cJSON *tRRD_S_pVal      = cJSON_GetObjectItemCaseSensitive(pJsTree, "tRRD_S");
  cJSON *tFAW_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "tFAW");
  cJSON *tREFI_pVal       = cJSON_GetObjectItemCaseSensitive(pJsTree, "tREFI");
  cJSON *tWTR_S_pVal      = cJSON_GetObjectItemCaseSensitive(pJsTree, "tWTR_S");
  cJSON *tRFC_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "tRFC");
  cJSON *tRC_pVal         = cJSON_GetObjectItemCaseSensitive(pJsTree, "tRC");
  cJSON *tCCD_pVal        = cJSON_GetObjectItemCaseSensitive(pJsTree, "tCCD");
  cJSON *CommandRate_pVal = cJSON_GetObjectItemCaseSensitive(pJsTree, "CommandRate");
  // Voltages
  cJSON *Vdd_pVal  = cJSON_GetObjectItemCaseSensitive(pJsTree, "Vdd");
  cJSON *Vddq_pVal = cJSON_GetObjectItemCaseSensitive(pJsTree, "Vddq");
  cJSON *Vpp_pVal  = cJSON_GetObjectItemCaseSensitive(pJsTree, "Vpp");
  // MISC
  cJSON *EnRMT_pVal   = cJSON_GetObjectItemCaseSensitive(pJsTree, "EnRMT");
  cJSON *Por_pVal     = cJSON_GetObjectItemCaseSensitive(pJsTree, "Por");
  cJSON *Ecs_pVal     = cJSON_GetObjectItemCaseSensitive(pJsTree, "Ecs");  // unsupported in DDR4
  cJSON *PprType_pVal = cJSON_GetObjectItemCaseSensitive(pJsTree, "PprType");
  cJSON *AmtPpr_pVal  = cJSON_GetObjectItemCaseSensitive(pJsTree, "AmtPpr");
  // ECC
  cJSON *DirectoryModeEn_pVal = cJSON_GetObjectItemCaseSensitive(pJsTree, "DirectoryModeEn");
  cJSON *DdrEccEnable_pVal    = cJSON_GetObjectItemCaseSensitive(pJsTree, "DdrEccEnable");
  cJSON *DdrEccCheck_pVal     = cJSON_GetObjectItemCaseSensitive(pJsTree, "DdrEccCheck");
  // clang-format on
#endif  // 数据 Get 区域 OFF

#if 1  // 数据 Set 区域 ON
  // clang-format off
  if ((Freq_pVal != NULL) && (cJSON_Number == Freq_pVal->type)) { pCfg->DdrFreqLimit = (UINT16)Freq_pVal->valueu64;}
  if ((tCL_pVal != NULL) && (cJSON_Number == tCL_pVal->type)) { pCfg->tCAS = (UINT8)tCL_pVal->valueu64;}
  if ((tRCD_pVal != NULL) && (cJSON_Number == tRCD_pVal->type)) { pCfg->tRCD = (UINT8)tRCD_pVal->valueu64;}
  if ((tRP_pVal != NULL) && (cJSON_Number == tRP_pVal->type)) { pCfg->tRCD = (UINT8)tRP_pVal->valueu64;}
  if ((tRAS_pVal != NULL) && (cJSON_Number == tRAS_pVal->type)) { pCfg->tRAS = (UINT8)tRAS_pVal->valueu64;}
  if ((tCWL_pVal != NULL) && (cJSON_Number == tCWL_pVal->type)) { pCfg->tCWL = (UINT8)tCWL_pVal->valueu64;}
  if ((tRTP_pVal != NULL) && (cJSON_Number == tRTP_pVal->type)) { pCfg->tRTP = (UINT8)tRTP_pVal->valueu64;}
  if ((tWR_pVal != NULL) && (cJSON_Number == tWR_pVal->type)) { pCfg->tWR = (UINT8)tWR_pVal->valueu64;}
  if ((tRRD_S_pVal != NULL) && (cJSON_Number == tRRD_S_pVal->type)) { pCfg->tRRD = (UINT8)tRRD_S_pVal->valueu64;}
  if ((tFAW_pVal != NULL) && (cJSON_Number == tFAW_pVal->type)) { pCfg->tFAW = (UINT8)tFAW_pVal->valueu64;}
  if ((tREFI_pVal != NULL) && (cJSON_Number == tREFI_pVal->type)) { pCfg->tREFI = (UINT8)tREFI_pVal->valueu64;}
  if ((tWTR_S_pVal != NULL) && (cJSON_Number == tWTR_S_pVal->type)) { pCfg->tWTR = (UINT8)tWTR_S_pVal->valueu64;}
  if ((tRFC_pVal != NULL) && (cJSON_Number == tRFC_pVal->type)) { pCfg->tRFC1 = (UINT8)tRFC_pVal->valueu64;}
  if ((tRC_pVal != NULL) && (cJSON_Number == tRC_pVal->type)) { pCfg->tRC = (UINT8)tRC_pVal->valueu64;}
  if ((tCCD_pVal != NULL) && (cJSON_Number == tCCD_pVal->type)) { pCfg->tCCD = (UINT8)tCCD_pVal->valueu64;}
  if ((CommandRate_pVal != NULL) && (cJSON_Number == CommandRate_pVal->type)) { pCfg->CommandTiming = (UINT8)CommandRate_pVal->valueu64;}
  // Voltages
  if ((Vdd_pVal != NULL) && (cJSON_Number == Vdd_pVal->type)) { pCfg->DfxPmicVpp = (UINT8)Vdd_pVal->valueu64;}
  if ((Vddq_pVal != NULL) && (cJSON_Number == Vddq_pVal->type)) { pCfg->DfxPmicVddQ = (UINT8)Vddq_pVal->valueu64;}
  if ((Vpp_pVal != NULL) && (cJSON_Number == Vpp_pVal->type)) { pCfg->DfxPmicVpp = (UINT8)Vpp_pVal->valueu64;}
  // MISC
  if ((EnRMT_pVal != NULL)  && (cJSON_Number == EnRMT_pVal->type)) { pCfg->EnableRMT = (UINT8)EnRMT_pVal->valueu64;}
  if ((Por_pVal != NULL)  && (cJSON_Number == Por_pVal->type)) { pCfg->EnforcePopulationPor = (UINT8)Por_pVal->valueu64;}
  if ((Ecs_pVal != NULL)  && (cJSON_Number == Ecs_pVal->type)) { pCfg->ErrorCheckScrub = (UINT8)Ecs_pVal->valueu64;}
  if ((PprType_pVal != NULL)  && (cJSON_Number == PprType_pVal->type)) { pCfg->PprType = (UINT8)PprType_pVal->valueu64;}
//   if ((AmtPpr_pVal != NULL)  && (cJSON_Number == AmtPpr_pVal->type)) { pCfg->AdvMemTestPpr = (UINT8)AmtPpr_pVal->valueu64;}
  // ECC
  if ((DirectoryModeEn_pVal != NULL) && (cJSON_Number == DirectoryModeEn_pVal->type)) { pCfg->DirectoryModeEn = (UINT8)DirectoryModeEn_pVal->valueu64;}
  if ((DdrEccEnable_pVal != NULL) && (cJSON_Number == DdrEccEnable_pVal->type)) { pCfg->DdrEccEnable = (UINT8)DdrEccEnable_pVal->valueu64;}
  if ((DdrEccCheck_pVal != NULL) && (cJSON_Number == DdrEccCheck_pVal->type)) { pCfg->DdrEccCheck = (UINT8)DdrEccCheck_pVal->valueu64;}
  // clang-format on
#endif  // 数据 Set 区域 ON

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_GetMemConfigFunc(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN BOOLEAN Real, OUT cJSON *pJsTree)
{
  EFI_STATUS               Status;
  OKN_MEMORY_CONFIGURATION Cfg;

  if (NULL == pProto) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem(&Cfg, sizeof(Cfg));

  if (Real) {
    if (NULL == pProto->GetMemConfigReal) {
      return EFI_UNSUPPORTED;
    }
    Status = pProto->GetMemConfigReal(&Cfg);
  }
  else {
    if (NULL == pProto->GetMemConfig) {
      return EFI_UNSUPPORTED;
    }
    Status = pProto->GetMemConfig(&Cfg);
  }

  if (EFI_ERROR(Status)) {
    Print(L"GetMemConfig%s failed: %r\n", Real ? L"Real" : L"", Status);
    return Status;
  }

  // PrintMemCfg(&Cfg); // 调试用

  if (FALSE == EFI_ERROR(Status)) {
    Status = OknMT_SetMemCfgToJson(&Cfg, pJsTree);
    if (TRUE == EFI_ERROR(Status)) {
      Print(L"[OKN_UEFI_ERR] OknMT_SetMemCfgToJson failed: %r\n", Status);
    }
  }

  cJSON_AddBoolToObject(pJsTree, "SUCCESS", (FALSE == EFI_ERROR(Status)));

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_GetSocketChannelDImmPtrsFromJson(IN OKN_MEMORY_TEST_PROTOCOL *pProto,
                                                  IN CONST cJSON              *pJsTree,
                                                  OUT cJSON                   *pSocket,
                                                  OUT cJSON                   *pChannel,
                                                  OUT cJSON                   *pDimm)
{
  cJSON *pSocket_tmp  = NULL;
  cJSON *pChannel_tmp = NULL;
  cJSON *pDimm_tmp    = NULL;
  // 1) 参数检查
  if (NULL == pProto || NULL == pJsTree) {
    Print(L"[OKN_UEFI_ERR] Protocol/SpdRead() or JSON tree is NULL\n");
    return EFI_INVALID_PARAMETER;
  }

  // 2) 取 Socket/Channel/Dimm
  pSocket_tmp  = cJSON_GetObjectItemCaseSensitive(pJsTree, "Socket");
  pChannel_tmp = cJSON_GetObjectItemCaseSensitive(pJsTree, "Channel");
  pDimm_tmp    = cJSON_GetObjectItemCaseSensitive(pJsTree, "Dimm");
  if (!cJSON_IsNumber(pSocket_tmp) || !cJSON_IsNumber(pChannel_tmp) || !cJSON_IsNumber(pDimm_tmp)) {
    Print(L"[OKN_UEFI_ERR] Invalid Socket/Channel/Dimm in JSON\n");
    return EFI_INVALID_PARAMETER;
  }

  // 3) 输出结果
  pSocket  = pSocket_tmp;
  pChannel = pChannel_tmp;
  pDimm    = pDimm_tmp;

  return EFI_SUCCESS;
}

CONST CHAR8 *OknMT_GetFileBaseName(IN CONST CHAR8 *Path)
{
  CONST CHAR8 *p    = Path;
  CONST CHAR8 *last = Path;
  while (*p != '\0') {
    if (*p == '/' || *p == '\\') {
      last = p + 1;
    }
    p++;
  }
  return last;
}
