#include <Library/IntelPortingLib.h>

#include "IntelPorting.h"

#include <Guid/SocketMemoryVariable.h>
#include <Guid/SetupVariable.h>
#include <Mem/Library/MemMcIpLib/Include/MemMcRegs.h>

VOID testConfigGet(cJSON *Tree)
{
  EFI_STATUS Status = 0, Status1 = 0;
  UINT32 Attributes, OknVarAttributes;
  UINTN DataSize = sizeof(SOCKET_MEMORY_CONFIGURATION), OknVarSize = sizeof(OKN_AMT_VARIABLE);
  SOCKET_MEMORY_CONFIGURATION Data;
  OKN_AMT_VARIABLE OknAmtVar;
  EFI_GUID OknAmtVariableGuid = OKN_AMT_VARIABLE_GUID;
  Status = gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &Attributes, &DataSize, &Data);
  Status1 = gRT->GetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, &OknVarAttributes, &OknVarSize, &OknAmtVar);

  if (Status1 == EFI_NOT_FOUND) {
    OknVarAttributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    ZeroMem(&OknAmtVar, sizeof(OKN_AMT_VARIABLE));
    Status1 = gRT->SetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, OknVarAttributes, OknVarSize, &OknAmtVar);
  }
  if (!EFI_ERROR (Status)) {
    GetSetJsonObjInt(Tree, "CommandRate", Data.CommandTiming);
    GetSetJsonObjInt(Tree, "Freq",      Data.DdrFreqLimit);
    GetSetJsonObjInt(Tree, "tCL",       Data.tCAS);
    GetSetJsonObjInt(Tree, "tRCD",      Data.tRCD);
    GetSetJsonObjInt(Tree, "tCCD",      Data.tCCD);
    // GetSetJsonObjInt(Tree, "tRP",       (Data.tRP == 0) ? 0 : (Data.tRP - 1));
    // GetSetJsonObjInt(Tree, "tRAS",      (Data.tRAS == 0) ? 0 : (Data.tRAS + 1));
    GetSetJsonObjInt(Tree, "tRP",       Data.tRP);
    GetSetJsonObjInt(Tree, "tRAS",      Data.tRAS);
    GetSetJsonObjInt(Tree, "tWR",       Data.tWR);
    // GetSetJsonObjInt(Tree, "tRFC",      (Data.tRFC1 / 2));
    GetSetJsonObjInt(Tree, "tRFC",      Data.tRFC1);
    GetSetJsonObjInt(Tree, "tRC",       Data.tRC);
    GetSetJsonObjInt(Tree, "Vdd",       Data.DfxPmicVdd);
    GetSetJsonObjInt(Tree, "Vddq",      Data.DfxPmicVddQ);
    GetSetJsonObjInt(Tree, "Vpp",       Data.DfxPmicVpp);
    GetSetJsonObjInt(Tree, "tCWL",      Data.tCWL);
    // GetSetJsonObjInt(Tree, "tRRD_S",    (Data.tRRD == 0) ? 0 : (Data.tRRD - 2));
    GetSetJsonObjInt(Tree, "tRRD_S",    Data.tRRD);
    GetSetJsonObjInt(Tree, "tWTR_S",    Data.tWTR);
    // GetSetJsonObjInt(Tree, "tFAW",      (Data.tFAW == 0) ? 0 : (Data.tFAW - 4));
    GetSetJsonObjInt(Tree, "tFAW",      Data.tFAW);
    // GetSetJsonObjInt(Tree, "tRTP",      Data.tRTP);
    GetSetJsonObjInt(Tree, "tRTP",      (Data.tRTP == 0) ? 0 : (Data.tRTP - 1));
    GetSetJsonObjInt(Tree, "tREFI",     Data.tREFI);
    GetSetJsonObjInt(Tree, "tRRDR",     Data.tRRDR);
    GetSetJsonObjInt(Tree, "tRWDR",     Data.tRWDR);
    GetSetJsonObjInt(Tree, "tWRDR",     Data.tWRDR);
    GetSetJsonObjInt(Tree, "tWWDR",     Data.tWWDR);
    GetSetJsonObjInt(Tree, "tRRSG",     Data.tRRSG);
    GetSetJsonObjInt(Tree, "tRWSG",     Data.tRWSG);
    GetSetJsonObjInt(Tree, "tWRSG",     Data.tWRSG);
    GetSetJsonObjInt(Tree, "tWWSG",     Data.tWWSG);
    GetSetJsonObjInt(Tree, "tRRDD",     Data.tRRDD);
    GetSetJsonObjInt(Tree, "tRWDD",     Data.tRWDD);
    GetSetJsonObjInt(Tree, "tWRDD",     Data.tWRDD);
    GetSetJsonObjInt(Tree, "tWWDD",     Data.tWWDD);
    GetSetJsonObjInt(Tree, "tRRSR",     Data.tRRSR);
    GetSetJsonObjInt(Tree, "tRWSR",     Data.tRWSR);
    GetSetJsonObjInt(Tree, "tWRSR",     Data.tWRSR);
    GetSetJsonObjInt(Tree, "tWWSR",     Data.tWWSR);
    GetSetJsonObjInt(Tree, "tRRDS",     Data.tRRDS);
    GetSetJsonObjInt(Tree, "tRWDS",     Data.tRWDS);
    GetSetJsonObjInt(Tree, "tWRDS",     Data.tWRDS);
    GetSetJsonObjInt(Tree, "tWWDS",     Data.tWWDS);
    GetSetJsonObjInt(Tree, "PprType",   Data.pprType);
    GetSetJsonObjInt(Tree, "AmtPpr",    Data.AdvMemTestPpr);
    GetSetJsonObjInt(Tree, "AmtRetry",  Data.AdvMemTestRetryAfterRepair);
    GetSetJsonObjInt(Tree, "Ecs",       Data.ErrorCheckScrub);
    GetSetJsonObjInt(Tree, "Por",       Data.EnforcePopulationPor);
  }
  if (!EFI_ERROR(Status1)) {
    GetSetJsonObjInt(Tree, "RcdMask",   OknAmtVar.RcdMask);
    GetSetJsonObjInt(Tree, "RW0A",      OknAmtVar.Rw0A);
    GetSetJsonObjInt(Tree, "RW0C",      OknAmtVar.Rw0C);
    GetSetJsonObjInt(Tree, "RW0E",      OknAmtVar.Rw0E);
    GetSetJsonObjInt(Tree, "isTrainingDisableHang", OknAmtVar.HangWhenFail);
  }
  
  cJSON_AddBoolToObject(Tree, "SUCCESS", (!Status) && (!Status1));
  return;
}

VOID testConfigSet(cJSON *Tree)
{
  EFI_STATUS Status, Status1;
  UINT32 Attributes, OknVarAttributes;
  UINTN DataSize = sizeof(SOCKET_MEMORY_CONFIGURATION), OknVarSize = sizeof(OKN_AMT_VARIABLE);
  SOCKET_MEMORY_CONFIGURATION Data;
  OKN_AMT_VARIABLE OknAmtVar;
  EFI_GUID OknAmtVariableGuid = OKN_AMT_VARIABLE_GUID;

  Status = gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &Attributes, &DataSize, &Data);
  Status1 = gRT->GetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, &OknVarAttributes, &OknVarSize, &OknAmtVar);

  if (Status1 == EFI_NOT_FOUND) {
    OknVarAttributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    ZeroMem(&OknAmtVar, sizeof(OKN_AMT_VARIABLE));
    Status1 = gRT->SetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, OknVarAttributes, OknVarSize, &OknAmtVar);
  }
  if (!EFI_ERROR (Status)) {
      Data.XMPMode = XMP_MANUAL;
      cJSON *Freq    = cJSON_GetObjectItemCaseSensitive(Tree, "Freq");
      cJSON *Vdd     = cJSON_GetObjectItemCaseSensitive(Tree, "Vdd");
      cJSON *Vddq    = cJSON_GetObjectItemCaseSensitive(Tree, "Vddq");
      cJSON *Vpp     = cJSON_GetObjectItemCaseSensitive(Tree, "Vpp");
      cJSON *tCL     = cJSON_GetObjectItemCaseSensitive(Tree, "tCL");
      cJSON *tRCD    = cJSON_GetObjectItemCaseSensitive(Tree, "tRCD");
      cJSON *tCCD    = cJSON_GetObjectItemCaseSensitive(Tree, "tCCD");
      cJSON *tRP     = cJSON_GetObjectItemCaseSensitive(Tree, "tRP");
      cJSON *tRAS    = cJSON_GetObjectItemCaseSensitive(Tree, "tRAS");
      cJSON *tWR     = cJSON_GetObjectItemCaseSensitive(Tree, "tWR");
      cJSON *tRFC    = cJSON_GetObjectItemCaseSensitive(Tree, "tRFC");
      cJSON *tRC     = cJSON_GetObjectItemCaseSensitive(Tree, "tRC");
      cJSON *tCWL    = cJSON_GetObjectItemCaseSensitive(Tree, "tCWL");
      cJSON *tRRD    = cJSON_GetObjectItemCaseSensitive(Tree, "tRRD_S");
      cJSON *tWTR    = cJSON_GetObjectItemCaseSensitive(Tree, "tWTR_S");
      cJSON *tFAW    = cJSON_GetObjectItemCaseSensitive(Tree, "tFAW");
      cJSON *tRTP    = cJSON_GetObjectItemCaseSensitive(Tree, "tRTP");
      cJSON *tREFI   = cJSON_GetObjectItemCaseSensitive(Tree, "tREFI");
      cJSON *tRRDR   = cJSON_GetObjectItemCaseSensitive(Tree, "tRRDR");
      cJSON *tRWDR   = cJSON_GetObjectItemCaseSensitive(Tree, "tRWDR");
      cJSON *tWRDR   = cJSON_GetObjectItemCaseSensitive(Tree, "tWRDR");
      cJSON *tWWDR   = cJSON_GetObjectItemCaseSensitive(Tree, "tWWDR");
      cJSON *tRRSG   = cJSON_GetObjectItemCaseSensitive(Tree, "tRRSG");
      cJSON *tRWSG   = cJSON_GetObjectItemCaseSensitive(Tree, "tRWSG");
      cJSON *tWRSG   = cJSON_GetObjectItemCaseSensitive(Tree, "tWRSG");
      cJSON *tWWSG   = cJSON_GetObjectItemCaseSensitive(Tree, "tWWSG");
      cJSON *tRRDD   = cJSON_GetObjectItemCaseSensitive(Tree, "tRRDD");
      cJSON *tRWDD   = cJSON_GetObjectItemCaseSensitive(Tree, "tRWDD");
      cJSON *tWRDD   = cJSON_GetObjectItemCaseSensitive(Tree, "tWRDD");
      cJSON *tWWDD   = cJSON_GetObjectItemCaseSensitive(Tree, "tWWDD");
      cJSON *tRRSR   = cJSON_GetObjectItemCaseSensitive(Tree, "tRRSR");
      cJSON *tRWSR   = cJSON_GetObjectItemCaseSensitive(Tree, "tRWSR");
      cJSON *tWRSR   = cJSON_GetObjectItemCaseSensitive(Tree, "tWRSR");
      cJSON *tWWSR   = cJSON_GetObjectItemCaseSensitive(Tree, "tWWSR");
      cJSON *tRRDS   = cJSON_GetObjectItemCaseSensitive(Tree, "tRRDS");
      cJSON *tRWDS   = cJSON_GetObjectItemCaseSensitive(Tree, "tRWDS");
      cJSON *tWRDS   = cJSON_GetObjectItemCaseSensitive(Tree, "tWRDS");
      cJSON *tWWDS   = cJSON_GetObjectItemCaseSensitive(Tree, "tWWDS");
      cJSON *CommandRate = cJSON_GetObjectItemCaseSensitive(Tree, "CommandRate");
      cJSON *PprType = cJSON_GetObjectItemCaseSensitive(Tree, "PprType");
      cJSON *AmtPpr  = cJSON_GetObjectItemCaseSensitive(Tree, "AmtPpr");
      cJSON *AmtRetry = cJSON_GetObjectItemCaseSensitive(Tree, "AmtRetry");
      cJSON *Ecs   = cJSON_GetObjectItemCaseSensitive(Tree, "Ecs");
      cJSON *Por   = cJSON_GetObjectItemCaseSensitive(Tree, "Por");

      if (Freq && Freq->type == cJSON_Number && Freq->valueu64 < 0x20 && Freq->valueu64 >= 0) {
          Data.DdrFreqLimit = (UINT8)Freq->valueu64;
      }
      if (Vdd && Vdd->type == cJSON_Number) {
          Data.DfxPmicVdd = (UINT16)Vdd->valueu64;
      }
      if (CommandRate && CommandRate->type == cJSON_Number) {
          Data.CommandTiming = (UINT8)CommandRate->valueu64;
      }
      if (Vddq && Vddq->type == cJSON_Number) {
          Data.DfxPmicVddQ = (UINT16)Vddq->valueu64;
      }
      if (Vpp && Vpp->type == cJSON_Number) {
          Data.DfxPmicVpp = (UINT16)Vpp->valueu64;
      }
      if (tCL && tCL->type == cJSON_Number) {
          Data.tCAS = (UINT8)tCL->valueu64;
      }
      if (tRCD && tRCD->type == cJSON_Number) {
          Data.tRCD = (UINT8)tRCD->valueu64;
      }
      if (tCCD && tCCD->type == cJSON_Number) {
          Data.tCCD = (UINT8)tCCD->valueu64;
      }
      if (tRP && tRP->type == cJSON_Number) {
          // Data.tRP = (tRP->valueu64 == 0) ? 0 : ((UINT8)tRP->valueu64 + 1);
          Data.tRP = (UINT8)tRP->valueu64;
      }
      if (tRAS && tRAS->type == cJSON_Number) {
          // Data.tRAS = (tRAS->valueu64 == 0) ? 0 : (UINT8)tRAS->valueu64 - 1;
          Data.tRAS = (UINT8)tRAS->valueu64;
      }
      if (tWR && tWR->type == cJSON_Number) {
          Data.tWR = (UINT8)tWR->valueu64;
      }
      if (tRFC && tRFC->type == cJSON_Number) {
          // Data.tRFC1 = (UINT16)(tRFC->valueu64 * 2);
          Data.tRFC1 = (UINT16)tRFC->valueu64;
      }
      if (tRC && tRC->type == cJSON_Number) {
          Data.tRC = (UINT8)tRC->valueu64;
      }
      if (tCWL && tCWL->type == cJSON_Number) {
          Data.tCWL = (UINT8)tCWL->valueu64;
      }
      if (tRRD && tRRD->type == cJSON_Number) {
          // Data.tRRD = (tRRD->valueu64 == 0) ? 0 : (UINT8)tRRD->valueu64 + 2;
          Data.tRRD = (UINT8)tRRD->valueu64;
      }
      if (tWTR && tWTR->type == cJSON_Number) {
          Data.tWTR = (UINT8)tWTR->valueu64;
      }
      if (tFAW && tFAW->type == cJSON_Number) {
          // Data.tFAW = (tFAW->valueu64 == 0) ? 0 : (UINT8)tFAW->valueu64 + 4;
          Data.tFAW = (UINT8)tFAW->valueu64;
      }
      if (tRTP && tRTP->type == cJSON_Number) {
          // Data.tRTP = (UINT8)tRTP->valueu64;
          Data.tRTP = (tRTP->valueu64 == 0) ? 0 : (UINT8)tRTP->valueu64 + 1;
      }
      if (tREFI && tREFI->type == cJSON_Number) {
          Data.tREFI = (UINT16)tREFI->valueu64;
      }
      if (tRRDR && tRRDR->type == cJSON_Number) { Data.tRRDR = (UINT8)tRRDR->valueu64;}
      if (tRWDR && tRWDR->type == cJSON_Number) { Data.tRWDR = (UINT8)tRWDR->valueu64;}
      if (tWRDR && tWRDR->type == cJSON_Number) { Data.tWRDR = (UINT8)tWRDR->valueu64;}
      if (tWWDR && tWWDR->type == cJSON_Number) { Data.tWWDR = (UINT8)tWWDR->valueu64;}
      if (tRRSG && tRRSG->type == cJSON_Number) { Data.tRRSG = (UINT8)tRRSG->valueu64;}
      if (tRWSG && tRWSG->type == cJSON_Number) { Data.tRWSG = (UINT8)tRWSG->valueu64;}
      if (tWRSG && tWRSG->type == cJSON_Number) { Data.tWRSG = (UINT8)tWRSG->valueu64;}
      if (tWWSG && tWWSG->type == cJSON_Number) { Data.tWWSG = (UINT8)tWWSG->valueu64;}
      if (tRRDD && tRRDD->type == cJSON_Number) { Data.tRRDD = (UINT8)tRRDD->valueu64;}
      if (tRWDD && tRWDD->type == cJSON_Number) { Data.tRWDD = (UINT8)tRWDD->valueu64;}
      if (tWRDD && tWRDD->type == cJSON_Number) { Data.tWRDD = (UINT8)tWRDD->valueu64;}
      if (tWWDD && tWWDD->type == cJSON_Number) { Data.tWWDD = (UINT8)tWWDD->valueu64;}
      if (tRRSR && tRRSR->type == cJSON_Number) { Data.tRRSR = (UINT8)tRRSR->valueu64;}
      if (tRWSR && tRWSR->type == cJSON_Number) { Data.tRWSR = (UINT8)tRWSR->valueu64;}
      if (tWRSR && tWRSR->type == cJSON_Number) { Data.tWRSR = (UINT8)tWRSR->valueu64;}
      if (tWWSR && tWWSR->type == cJSON_Number) { Data.tWWSR = (UINT8)tWWSR->valueu64;}
      if (tRRDS && tRRDS->type == cJSON_Number) { Data.tRRDS = (UINT8)tRRDS->valueu64;}
      if (tRWDS && tRWDS->type == cJSON_Number) { Data.tRWDS = (UINT8)tRWDS->valueu64;}
      if (tWRDS && tWRDS->type == cJSON_Number) { Data.tWRDS = (UINT8)tWRDS->valueu64;}
      if (tWWDS && tWWDS->type == cJSON_Number) { Data.tWWDS = (UINT8)tWWDS->valueu64;}
      if (PprType && PprType->type == cJSON_Number) { Data.pprType = (UINT8)PprType->valueu64;}
      if (AmtPpr && AmtPpr->type == cJSON_Number) { Data.AdvMemTestPpr = (UINT8)AmtPpr->valueu64;}
      if (AmtRetry && AmtRetry->type == cJSON_Number) { Data.AdvMemTestRetryAfterRepair = (UINT8)AmtRetry->valueu64;}
      if (Ecs && Ecs->type == cJSON_Number) { Data.ErrorCheckScrub = (UINT8)Ecs->valueu64;}
      if (Por && Por->type == cJSON_Number) { Data.EnforcePopulationPor = (UINT8)Por->valueu64;}
      Status = gRT->SetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, Attributes, DataSize, &Data);
  }
  if (!EFI_ERROR(Status1)) {
    cJSON *RcdMask = cJSON_GetObjectItemCaseSensitive(Tree, "RcdMask");
    cJSON *RW0A    = cJSON_GetObjectItemCaseSensitive(Tree, "RW0A");
    cJSON *RW0C    = cJSON_GetObjectItemCaseSensitive(Tree, "RW0C");
    cJSON *RW0E    = cJSON_GetObjectItemCaseSensitive(Tree, "RW0E");
    cJSON *HangWhenFail = cJSON_GetObjectItemCaseSensitive(Tree, "isTrainingDisableHang");
    if (RcdMask && RcdMask->type == cJSON_Number) { OknAmtVar.RcdMask = (UINT8)RcdMask->valueu64;}
    if (RW0A && RW0A->type == cJSON_Number) { OknAmtVar.Rw0A = (UINT8)RW0A->valueu64;}
    if (RW0C && RW0C->type == cJSON_Number) { OknAmtVar.Rw0C = (UINT8)RW0C->valueu64;}
    if (RW0E && RW0E->type == cJSON_Number) { OknAmtVar.Rw0E = (UINT8)RW0E->valueu64;}
    if (HangWhenFail && HangWhenFail->type == cJSON_Number) {OknAmtVar.HangWhenFail = (BOOLEAN)HangWhenFail->valueu64;}
    Status1 = gRT->SetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, OknVarAttributes, OknVarSize, &OknAmtVar);
  }
  cJSON_AddBoolToObject(Tree, "SUCCESS", !Status);
}

VOID testConfigActive(cJSON *Tree)
{
  MEMORY_TIMINGS_BANK_TCL_TWL_MCDDC_CTL_STRUCT        MtTclTwl = {0};
  MEMORY_TIMINGS_BANK_TRCD_MCDDC_CTL_STRUCT           MtTrcdRW = {0};
  MEMORY_TIMINGS_BANK_TRP_TRC_TRAS_MCDDC_CTL_STRUCT   MtTrpTrcTras = {0};
  MEMORY_TIMINGS_BANK_TRTP_TWR_MCDDC_CTL_STRUCT       MtTrtpTwr = {0};
  TCRFTP_MCDDC_CTL_STRUCT                             Tcrftp = {0};
  MEMORY_TIMINGS_RANK_TRRD_TFAW_MCDDC_CTL_STRUCT      MtTrrdTfaw = {0};
  MEMORY_TIMINGS_CAS2CAS_DR_MCDDC_CTL_STRUCT          TCas2CasDr = {0};
  MEMORY_TIMINGS_CAS2CAS_DD_MCDDC_CTL_STRUCT          TCas2CasDd = {0};
  MEMORY_TIMINGS_CAS2CAS_SR_MCDDC_CTL_STRUCT          TCas2CasSr = {0};
  MEMORY_TIMINGS_CAS2CAS_SG_MCDDC_CTL_STRUCT          TCas2CasSg = {0};
  MEMORY_TIMINGS_CAS2CAS_DS_MCDDC_CTL_STRUCT          TCas2CasDs = {0};
  MCSCHED_CHKN_BIT2_MCDDC_CTL_SPRD0_STRUCT            McSchedChknBit2 = {0};
  MEMORY_TIMINGS_BANK_TRDA_TWRA_TWRPRE_MCDDC_CTL_STRUCT MtTrdaTwraTwrpre = {0};
  MEMORY_TIMINGS_RANK_TRRD_3DS_MCDDC_CTL_STRUCT       TRrd3ds = {0};
  MEMORY_TIMINGS_ADJ_MCDDC_CTL_STRUCT                 MemTimingsAdj = {0};
  MCSCHED_CHKN_BIT2_MCDDC_CTL_SPRMCC_SPRLCC_EMRXCC_SPRE0_EMRMCC_SPRMCCEEC_STRUCT McSchedChknBit2E0 = {0};
  SYS_SETUP                                           *Setup;
  struct channelNvram                                 (*ChannelNvList)[MAX_CH];
  struct SystemMemoryMapHob                           *SystemMemoryMap;

  UINT32 Attributes;
  UINTN DataSize = sizeof(SOCKET_MEMORY_CONFIGURATION);
  SOCKET_MEMORY_CONFIGURATION Data;
  gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &Attributes, &DataSize, &Data);

  SystemMemoryMap = GetSystemMemoryMapData ();
  cJSON *Socket = cJSON_GetObjectItemCaseSensitive(Tree, "Socket");
  cJSON *Channel = cJSON_GetObjectItemCaseSensitive(Tree, "Channel");
  
  if (Socket && Channel) {
    Setup = GetSysSetupPointer();

    cJSON *Active = cJSON_AddArrayToObject(Tree, "Active");
    ChannelNvList = GetChannelNvList ((UINT8)Socket->valueu64);

    MtTclTwl.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_BANK_TCL_TWL_MCDDC_CTL_REG);
    MtTrcdRW.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_BANK_TRCD_MCDDC_CTL_REG);
    MtTrpTrcTras.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_BANK_TRP_TRC_TRAS_MCDDC_CTL_REG);
    MtTrtpTwr.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_BANK_TRTP_TWR_MCDDC_CTL_REG);
    Tcrftp.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, TCRFTP_MCDDC_CTL_REG);
    MtTrrdTfaw.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_RANK_TRRD_TFAW_MCDDC_CTL_REG);
    TCas2CasSg.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_CAS2CAS_SG_MCDDC_CTL_REG);
    TCas2CasSr.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_CAS2CAS_SR_MCDDC_CTL_REG);
    TCas2CasDr.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_CAS2CAS_DR_MCDDC_CTL_REG);
    TCas2CasDd.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_CAS2CAS_DD_MCDDC_CTL_REG);
    TCas2CasDs.Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_CAS2CAS_DS_MCDDC_CTL_REG);
    McSchedChknBit2.Data = MemReadPciCfgEp ((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MCSCHED_CHKN_BIT2_MCDDC_CTL_REG);
    MtTrdaTwraTwrpre.Data = MemReadPciCfgEp ((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_BANK_TRDA_TWRA_TWRPRE_MCDDC_CTL_REG);
    MemTimingsAdj.Data = MemReadPciCfgEp ((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_ADJ_MCDDC_CTL_REG);
    McSchedChknBit2E0.Data = MemReadPciCfgEp ((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MCSCHED_CHKN_BIT2_MCDDC_CTL_REG);

    cJSON *Detail = cJSON_CreateObject();
    cJSON_AddNumberToObject(Detail, "Freq", SystemMemoryMap->memFreq);
    UINT8 CmdRate = 0;
    if (MemTimingsAdj.Bits.cmd_stretch == 0) {
      CmdRate = 1;
    } else if (MemTimingsAdj.Bits.cmd_stretch == 2) {
      CmdRate = 2;
    }
    cJSON_AddNumberToObject(Detail, "CommandRate", CmdRate);
    cJSON_AddNumberToObject(Detail, "tCL", MtTclTwl.Bits.t_cl);
    cJSON_AddNumberToObject(Detail, "tRCD", MtTrcdRW.Bits.t_rcd_rd);
    cJSON_AddNumberToObject(Detail, "tCCD", (*ChannelNvList)[Channel->valueu64].common.tCCD);
    // cJSON_AddNumberToObject(Detail, "tRP", MtTrpTrcTras.Bits.t_rp);
    // cJSON_AddNumberToObject(Detail, "tRAS", MtTrpTrcTras.Bits.t_ras);
    cJSON_AddNumberToObject(Detail, "tRP", MtTrpTrcTras.Bits.t_rp + CmdRate);
    cJSON_AddNumberToObject(Detail, "tRAS", MtTrpTrcTras.Bits.t_ras - CmdRate);
    cJSON_AddNumberToObject(Detail, "tWR", (*ChannelNvList)[Channel->valueu64].common.nWR);
    // cJSON_AddNumberToObject(Detail, "tRFC", Tcrftp.Bits.t_rfc);
    cJSON_AddNumberToObject(Detail, "tRFC", (*ChannelNvList)[Channel->valueu64].common.nRFC);
    cJSON_AddNumberToObject(Detail, "tRC", MtTrpTrcTras.Bits.t_rc);
    cJSON_AddNumberToObject(Detail, "Vdd", Setup->mem.DfxPmicVdd);
    cJSON_AddNumberToObject(Detail, "Vddq", Setup->mem.DfxPmicVddQ);
    cJSON_AddNumberToObject(Detail, "Vpp", Setup->mem.DfxPmicVpp);
    cJSON_AddNumberToObject(Detail, "tCWL", MtTclTwl.Bits.t_wl);
    // cJSON_AddNumberToObject(Detail, "tRRD_S", MtTrrdTfaw.Bits.t_rrd_s);
    cJSON_AddNumberToObject(Detail, "tRRD_S", MtTrrdTfaw.Bits.t_rrd_s + 2);
    cJSON_AddNumberToObject(Detail, "tWTR_S", (*ChannelNvList)[Channel->valueu64].common.nWTR);
    // cJSON_AddNumberToObject(Detail, "tFAW", MtTrrdTfaw.Bits.t_faw);
    cJSON_AddNumberToObject(Detail, "tFAW", MtTrrdTfaw.Bits.t_faw + 4);
    // cJSON_AddNumberToObject(Detail, "tRTP", MtTrtpTwr.Bits.t_rtp);
    cJSON_AddNumberToObject(Detail, "tRTP", MtTrtpTwr.Bits.t_rtp-1);
    cJSON_AddNumberToObject(Detail, "tREFI", Tcrftp.Bits.t_refi);
    cJSON_AddNumberToObject(Detail, "tRRDR", TCas2CasDr.Bits.t_rrdr);
    cJSON_AddNumberToObject(Detail, "tRWDR", TCas2CasDr.Bits.t_rwdr);
    cJSON_AddNumberToObject(Detail, "tWRDR", TCas2CasDr.Bits.t_wrdr);
    cJSON_AddNumberToObject(Detail, "tWWDR", TCas2CasDr.Bits.t_wwdr);
    cJSON_AddNumberToObject(Detail, "tRRSG", TCas2CasSg.Bits.t_rrsg);
    cJSON_AddNumberToObject(Detail, "tRWSG", TCas2CasSg.Bits.t_rwsg);
    cJSON_AddNumberToObject(Detail, "tWRSG", TCas2CasSg.Bits.t_wrsg);
    cJSON_AddNumberToObject(Detail, "tWWSG", TCas2CasSg.Bits.t_wwsg);
    cJSON_AddNumberToObject(Detail, "tRRDD", TCas2CasDd.Bits.t_rrdd);
    cJSON_AddNumberToObject(Detail, "tRWDD", TCas2CasDd.Bits.t_rwdd);
    cJSON_AddNumberToObject(Detail, "tWRDD", TCas2CasDd.Bits.t_wrdd);
    cJSON_AddNumberToObject(Detail, "tWWDD", TCas2CasDd.Bits.t_wwdd);
    cJSON_AddNumberToObject(Detail, "tRRSR", TCas2CasSr.Bits.t_rrsr);
    cJSON_AddNumberToObject(Detail, "tRWSR", TCas2CasSr.Bits.t_rwsr);
    cJSON_AddNumberToObject(Detail, "tWRSR", TCas2CasSr.Bits.t_wrsr);
    cJSON_AddNumberToObject(Detail, "tWWSR", TCas2CasSr.Bits.t_wwsr);
    cJSON_AddNumberToObject(Detail, "tRRDS", TCas2CasDs.Bits.t_rrds);
    cJSON_AddNumberToObject(Detail, "tRWDS", TCas2CasDs.Bits.t_rwds);
    cJSON_AddNumberToObject(Detail, "tWRDS", TCas2CasDs.Bits.t_wrds);
    cJSON_AddNumberToObject(Detail, "tWWDS", TCas2CasDs.Bits.t_wwds);
    cJSON_AddNumberToObject(Detail, "tRCDWR", MtTrcdRW.Bits.t_rcd_wr);
    cJSON_AddNumberToObject(Detail, "tRCDIMPRD", MtTrcdRW.Bits.t_rcd_imprd);
    cJSON_AddNumberToObject(Detail, "tRCDIMPWR", MtTrcdRW.Bits.t_rcd_impwr);
    cJSON_AddNumberToObject(Detail, "tRDA", MtTrdaTwraTwrpre.Bits.t_rda);
    cJSON_AddNumberToObject(Detail, "tWRA", MtTrdaTwraTwrpre.Bits.t_wra);
    cJSON_AddNumberToObject(Detail, "tWRPRE", MtTrdaTwraTwrpre.Bits.t_wrpre);
    cJSON_AddNumberToObject(Detail, "tWRRDA", MtTrdaTwraTwrpre.Bits.t_wr_rda);
    cJSON_AddNumberToObject(Detail, "tRRDL", MtTrrdTfaw.Bits.t_rrd_l);
    if (Is3dsDimmPresentInChannel ((UINT8)Socket->valueu64, (UINT8)Channel->valueu64) == TRUE) {
      TRrd3ds.Data        = ChRegisterRead (MemTechDdr, (UINT8)Socket->valueu64, (UINT8)Channel->valueu64, MEMORY_TIMINGS_RANK_TRRD_3DS_MCDDC_CTL_REG);
      cJSON_AddNumberToObject(Detail, "tRRDDLR", TRrd3ds.Bits.t_rrd_dlr);
    }
    cJSON_AddNumberToObject(Detail, "tWTRL", (*ChannelNvList)[Channel->valueu64].common.nWTR_L);
    cJSON_AddItemToArray(Active, Detail);
    cJSON_AddBoolToObject(Tree, "SUCCESS", TRUE);
  }
}

VOID regRead(cJSON *Tree)
{
  cJSON *Socket = cJSON_GetObjectItemCaseSensitive(Tree, "Socket");
  cJSON *Channel = cJSON_GetObjectItemCaseSensitive(Tree, "Channel");
  cJSON *OffsetStr = cJSON_GetObjectItemCaseSensitive(Tree, "Offset");

  UINT64 Offset = AsciiStrHexToUint64(OffsetStr->valuestring);

  if (Socket || Channel || OffsetStr) {
    UINT32 Data = MemReadPciCfgEp((UINT8)Socket->valueu64, (UINT8)Channel->valueu64, (UINT32)Offset);
    CHAR8 Buffer[16];
    AsciiSPrint(Buffer, 16, "%x", Data);
    cJSON_AddStringToObject(Tree, "Data", Buffer);
  }
  cJSON_AddBoolToObject(Tree, "SUCCESS", TRUE);
}

VOID amtControl(cJSON *Tree, BOOLEAN Enable)
{
  EFI_STATUS Status = 0, Status1 = 0;
  UINT32 MemConfAttributes, SysConfAttributes, OknVarAttributes;
  EFI_GUID OknAmtVariableGuid = OKN_AMT_VARIABLE_GUID;
  UINTN MemConfSize = sizeof(SOCKET_MEMORY_CONFIGURATION), SysConfSize = sizeof(SYSTEM_CONFIGURATION), OknVarSize = sizeof(OKN_AMT_VARIABLE);
  SOCKET_MEMORY_CONFIGURATION MemConf;
  SYSTEM_CONFIGURATION SysConf;
  OKN_AMT_VARIABLE OknAmtVar;
  Status |= gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &MemConfAttributes, &MemConfSize, &MemConf);
  Status |= gRT->GetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, &SysConfAttributes, &SysConfSize, &SysConf);
  Status1 = gRT->GetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, &OknVarAttributes, &OknVarSize, &OknAmtVar);
  if (Status1 == EFI_NOT_FOUND) {
    OknVarAttributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
    ZeroMem(&OknAmtVar, sizeof(OKN_AMT_VARIABLE));
    Status1 = gRT->SetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, OknVarAttributes, OknVarSize, &OknAmtVar);
  }
  if (Status == EFI_SUCCESS) {
    cJSON *Options = cJSON_GetObjectItemCaseSensitive(Tree, "OPTIONS");
    cJSON *AmtPause = cJSON_GetObjectItemCaseSensitive(Tree, "AmtPause");
    if (AmtPause && AmtPause->type == cJSON_Number) {
      MemConf.AdvMemTestCondition = ADV_MEM_TEST_COND_MANUAL;
      MemConf.AdvMemTestCondPause = (UINT32)AmtPause->valueu64;
    }
    if (Options && Enable) {
      cJSON *Loops = cJSON_GetObjectItemCaseSensitive(Tree, "LOOPS");
      cJSON *AllowCe = cJSON_GetObjectItemCaseSensitive(Tree, "AllowCe");
      cJSON *MaxFailNumPerBank = cJSON_GetObjectItemCaseSensitive(Tree, "MaxFailNumPerBank");
      if (Loops && Loops->valueu64 != 0) {
        MemConf.MemTestLoops = (UINT16)Loops->valueu64;
      }
      if (MaxFailNumPerBank) {
        OknAmtVar.MaxFailNumPerBank = MaxFailNumPerBank->valueu64;
      }
      
      if (AllowCe && AllowCe->type == cJSON_Number) {MemConf.allowCorrectableMemTestError = (UINT8)AllowCe->valueu64;}
      MemConf.AdvMemTestOptions = (UINT32)Options->valueu64;
      // MemConf.AttemptFastBoot = FAST_BOOT_DISABLE;
      // MemConf.AttemptFastBootCold = FAST_BOOT_COLD_DISABLE;
      MemConf.MemTestOnColdFastBoot = MEM_TEST_COLD_FAST_BOOT_ENABLE;
      SysConf.serialDebugMsgLvl = 1;       // STR_SERIAL_DEBUG_MINIMUM

      cJSON *Param = cJSON_GetObjectItemCaseSensitive(Tree, "PARAM");
      if (Param && (Param->type == cJSON_Array)) {
        INT32 TestNum = (INT32)cJSON_GetArraySize(Param);
        if (TestNum > 0) {
          MemConf.AdvMemTestCondition = ADV_MEM_TEST_COND_MANUAL;
        }
        for (INT32 i = 0; i < TestNum; i++) {
          cJSON *TestCond = cJSON_GetArrayItem(Param, i);
          if (TestCond && TestCond->type == cJSON_Array) {
            INT32 ParamNum = (INT32)cJSON_GetArraySize(TestCond);
            if ((ParamNum != 5) && (ParamNum != 10)) {
              Status = EFI_INVALID_PARAMETER;
              break;
            }
            cJSON *TestId = cJSON_GetArrayItem(TestCond, 0);
            cJSON *TestVdd = cJSON_GetArrayItem(TestCond, 1);
            cJSON *TestVddq = cJSON_GetArrayItem(TestCond, 2);
            cJSON *TestTwr = cJSON_GetArrayItem(TestCond, 3);
            cJSON *TestTrefi = cJSON_GetArrayItem(TestCond, 4);
            
            MemConf.OknAmtCondition[TestId->valueu64].PmicVddLevel = (UINT16)TestVdd->valueu64;
            MemConf.OknAmtCondition[TestId->valueu64].PmicVddQLevel = (UINT16)TestVddq->valueu64;
            MemConf.OknAmtCondition[TestId->valueu64].TwrValue = (UINT8)TestTwr->valueu64;
            MemConf.OknAmtCondition[TestId->valueu64].TrefiValue = (UINT16)TestTrefi->valueu64;
            if (ParamNum == 10) {
              cJSON *BitMask = cJSON_GetArrayItem(TestCond, 5);
              cJSON *BgIntlev = cJSON_GetArrayItem(TestCond, 6);
              cJSON *AddMode = cJSON_GetArrayItem(TestCond, 7);
              cJSON *AmtPause2 = cJSON_GetArrayItem(TestCond, 8);
              cJSON *Pat = cJSON_GetArrayItem(TestCond, 9);
              if (BitMask->valueu64) {
                OknAmtVar.Enable = TRUE;
              }
              OknAmtVar.OknAmtCondition2[TestId->valueu64].BitMask = (UINT8)BitMask->valueu64;
              OknAmtVar.OknAmtCondition2[TestId->valueu64].BgInterleave = (UINT8)BgIntlev->valueu64;
              OknAmtVar.OknAmtCondition2[TestId->valueu64].AddressMode = (UINT8)AddMode->valueu64;
              OknAmtVar.OknAmtCondition2[TestId->valueu64].AdvMemTestCondPause = (UINT32)AmtPause2->valueu64;
              OknAmtVar.OknAmtCondition2[TestId->valueu64].Pattern = Pat->valueu64;
            }
          }
        }
      }
    }
    if (!Enable) {
      MemConf.AdvMemTestOptions = 0;
      MemConf.MemTestLoops = 1;
      // MemConf.AttemptFastBoot = FAST_BOOT_ENABLE;
      // MemConf.AttemptFastBootCold = FAST_BOOT_COLD_ENABLE;
      MemConf.MemTestOnColdFastBoot = MEM_TEST_COLD_FAST_BOOT_DISABLE;
      SysConf.serialDebugMsgLvl = 1;
      OknAmtVar.Enable = FALSE;
    }
    
    Status |= gRT->SetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, MemConfAttributes, MemConfSize, &MemConf);
    Status |= gRT->SetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, SysConfAttributes, SysConfSize, &SysConf);
    Status |= gRT->SetVariable(OKN_AMT_VARIABLE_NAME, &OknAmtVariableGuid, OknVarAttributes, OknVarSize, &OknAmtVar);
  }
  cJSON_AddBoolToObject(Tree, "SUCCESS", !Status);
  return;
}

VOID rmtControl(cJSON *Tree, BOOLEAN Enable) {
  EFI_STATUS Status = 0;
  UINT32 MemConfAttributes, SysConfAttributes;
  UINTN MemConfSize = sizeof(SOCKET_MEMORY_CONFIGURATION), SysConfSize = sizeof(SYSTEM_CONFIGURATION);
  SOCKET_MEMORY_CONFIGURATION MemConf;
  SYSTEM_CONFIGURATION SysConf;
  Status |= gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &MemConfAttributes, &MemConfSize, &MemConf);
  Status |= gRT->GetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, &SysConfAttributes, &SysConfSize, &SysConf);
  if (Status == EFI_SUCCESS) {
    if (Enable) {
      MemConf.EnableRMT = BIOS_SSA_RMT_ENABLE;
      SysConf.serialDebugMsgLvl = 2;   // STR_SERIAL_DEBUG_NORMAL
    } else {
      MemConf.EnableRMT = BIOS_SSA_RMT_DISABLE;
      SysConf.serialDebugMsgLvl = 1;   // STR_SERIAL_DEBUG_MINIMUM
    }
    Status |= gRT->SetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, MemConfAttributes, MemConfSize, &MemConf);
    Status |= gRT->SetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, SysConfAttributes, SysConfSize, &SysConf);
  }
  cJSON_AddBoolToObject(Tree, "SUCCESS", !Status);
  return;
}

VOID trainMsgCtrl(cJSON *Tree) {
  EFI_STATUS Status = 0;
  UINT32 SysConfAttributes;
  UINTN SysConfSize = sizeof(SYSTEM_CONFIGURATION);
  SYSTEM_CONFIGURATION SysConf;
  Status = gRT->GetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, &SysConfAttributes, &SysConfSize, &SysConf);
  if (Status == 0) {
    Status = EFI_NOT_FOUND;
    cJSON *Enable = cJSON_GetObjectItemCaseSensitive(Tree, "Enable");
    if (Enable && Enable->type == cJSON_Number) {
      Status = 0;
      if (Enable->valueu64 == 1) {
        SysConf.serialDebugMsgLvlTrainResults = 8;
      } else {
        SysConf.serialDebugMsgLvlTrainResults = 0;
      }
    }
    Status = gRT->SetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, SysConfAttributes, SysConfSize, &SysConf);
  }
  cJSON_AddBoolToObject(Tree, "SUCCESS", !Status);
  return;
}

extern UINT8 mSpdTableDDR[MAX_CH][MAX_DIMM][MAX_SPD_BYTE_DDR];

DIMM_ERROR_QUEUE gOknDimmErrorQueue;

VOID readSPD(cJSON *Tree)
{
  EFI_STATUS Status = 0;
  cJSON *Socket = cJSON_GetObjectItemCaseSensitive(Tree, "Socket");
  cJSON *Channel = cJSON_GetObjectItemCaseSensitive(Tree, "Channel");
  cJSON *Dimm = cJSON_GetObjectItemCaseSensitive(Tree, "Dimm");

  if (Socket && Channel && Dimm) {
    if (!cJSON_GetObjectItemCaseSensitive(Tree, "SPD")) {
      cJSON *Spd = cJSON_AddStringToObject(Tree, "SPD", "");
      UINT8 SocketId = 0, DimmId = 0;
      SocketId = (UINT8)(Channel->valueu64 % 2);
      DimmId = (UINT8)((Socket->valueu64 << 3) + Channel->valueu64) / 2;
	  /**
	   * mSpdTableDDR 是预先存储好的数据
	   */
      if (mSpdTableDDR[DimmId][SocketId][0] != 0) {   //OKN_DBG
      // if (1) {//OKN_DBG
        UINT32 SourceLength = 1024;
        CHAR8 *Destination = NULL;
        UINTN DestinationSize = 0;
        Status = Base64Encode(mSpdTableDDR[DimmId][SocketId], SourceLength, Destination, &DestinationSize);
        // SpdBinBuffer[0x208] = Socket->valueu64 * 8 + Channel->valueu64 + 0x10;
        // Status = Base64Encode(SpdBinBuffer, SourceLength, Destination, &DestinationSize); //OKN_DBG
        if (Status == EFI_BUFFER_TOO_SMALL) {
            Destination = AllocateZeroPool(DestinationSize);
            Status = Base64Encode(mSpdTableDDR[DimmId][SocketId], SourceLength, Destination, &DestinationSize);
            // Status = Base64Encode(SpdBinBuffer, SourceLength, Destination, &DestinationSize); //OKN_DBG
            if (!EFI_ERROR (Status)) {
                cJSON_SetValuestring(Spd, Destination);
            }
            FreePool(Destination);
            Destination = NULL;
            return;
        }
      }
    }
  }
}

VOID cpuSelect(cJSON *Tree)
{
  EFI_STATUS Status = 0;
  UINT32 MemConfAttributes;
  UINTN MemConfSize = sizeof(SOCKET_MEMORY_CONFIGURATION);
  SOCKET_MEMORY_CONFIGURATION MemConf;
  Status |= gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &MemConfAttributes, &MemConfSize, &MemConf);
  if (Status == 0) {
    cJSON *Count = cJSON_GetObjectItemCaseSensitive(Tree, "Count");
    if (Count && Count->type == cJSON_Number) {
      MemConf.AllowedSocketsInParallel = (UINT8)Count->valueu64;
    }
    Status |= gRT->SetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, MemConfAttributes, MemConfSize, &MemConf);
  }
  cJSON_AddBoolToObject(Tree, "SUCCESS", !Status);
  return;
}

VOID spdPrint(cJSON *Tree)
{
  EFI_STATUS Status = 0;
  UINT32 MemConfAttributes, SysConfAttributes;
  UINTN MemConfSize = sizeof(SOCKET_MEMORY_CONFIGURATION), SysConfSize = sizeof(SYSTEM_CONFIGURATION);
  SOCKET_MEMORY_CONFIGURATION MemConf;
  SYSTEM_CONFIGURATION SysConf;
  Status |= gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &MemConfAttributes, &MemConfSize, &MemConf);
  Status |= gRT->GetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, &SysConfAttributes, &SysConfSize, &SysConf);
  if (Status == 0) {
    cJSON *Enable = cJSON_GetObjectItemCaseSensitive(Tree, "ENABLE");
    if (Enable && Enable->type == cJSON_Number) {
      MemConf.SpdPrintEn = (UINT8)Enable->valueu64;
      if (Enable->valueu64) {
        MemConf.AllowedSocketsInParallel = 1;
        MemConf.AttemptFastBoot = FAST_BOOT_DISABLE;
        MemConf.AttemptFastBootCold = FAST_BOOT_COLD_DISABLE;
        MemConf.MemTestOnColdFastBoot = MEM_TEST_COLD_FAST_BOOT_ENABLE;
        SysConf.serialDebugMsgLvl = 1;       // STR_SERIAL_DEBUG_MINIMUM
      } else {
        MemConf.AllowedSocketsInParallel = 0;
        MemConf.AttemptFastBoot = FAST_BOOT_ENABLE;
        MemConf.AttemptFastBootCold = FAST_BOOT_COLD_ENABLE;
        MemConf.MemTestOnColdFastBoot = MEM_TEST_COLD_FAST_BOOT_DISABLE;
        SysConf.serialDebugMsgLvl = 1;
      }
    }
    Status |= gRT->SetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, MemConfAttributes, MemConfSize, &MemConf);
    Status |= gRT->SetVariable(PLATFORM_SETUP_VARIABLE_NAME, &gSetupVariableGuid, SysConfAttributes, SysConfSize, &SysConf);
  }
  cJSON_AddBoolToObject(Tree, "SUCCESS", !Status);
  return;
}

//OKN_20240822_yjb_EgsAddrTrans >>
EFI_STATUS SysToDimm(UINTN Address, OKN_DIMM_ADDRESS_DETAIL *DimmAddress)
{
  EFI_STATUS Status = 0;
  TRANSLATED_ADDRESS TranslatedAddress = {0};

  if (gAddrTrans == NULL) {
    return EFI_UNSUPPORTED;
  }
  Status = gAddrTrans->SystemAddressToDimmAddress(Address, &TranslatedAddress);

  if (Status == 0) {
    DimmAddress->Address = TranslatedAddress.SystemAddress;
    DimmAddress->SocketId = TranslatedAddress.SocketId;
    DimmAddress->MemCtrlId = TranslatedAddress.MemoryControllerId;
    DimmAddress->ChannelId = TranslatedAddress.ChannelId;
    DimmAddress->DimmSlot = TranslatedAddress.DimmSlot;
    DimmAddress->RankId = TranslatedAddress.PhysicalRankId;
    DimmAddress->SubChId = TranslatedAddress.ChipSelect;
    DimmAddress->BankGroup = TranslatedAddress.BankGroup;
    DimmAddress->Bank = TranslatedAddress.Bank;
    DimmAddress->Row = TranslatedAddress.Row;
    DimmAddress->Column = TranslatedAddress.Col;
  }

  return Status;
}

//OKN_20240822_yjb_EgsAddrTrans <<

//OKN_20240827_yjb_ErrQueue >>
VOID InitErrorQueue(DIMM_ERROR_QUEUE *Queue)
{
  for (UINT8 i = 0; i < MAX_ADDRESS_RECORD_SIZE; i++) {
    ZeroMem(&Queue->AddrBuffer[i], sizeof(OKN_DIMM_ADDRESS_DETAIL));
  }
  Queue->Head = 0;
  Queue->Tail = 0;
}
BOOLEAN IsErrorQueueEmpty(DIMM_ERROR_QUEUE *Queue)
{
  return Queue->Head == Queue->Tail;
}
BOOLEAN IsErrorQueueFull(DIMM_ERROR_QUEUE *Queue)
{
  return ((Queue->Tail + 1) % MAX_ADDRESS_RECORD_SIZE) == Queue->Head;
}
VOID EnqueueError(DIMM_ERROR_QUEUE *Queue, OKN_DIMM_ADDRESS_DETAIL *Item)
{
  if (IsErrorQueueFull(Queue)) {
    Queue->Head = (Queue->Head + 1) % MAX_ADDRESS_RECORD_SIZE;
  }
  Queue->AddrBuffer[Queue->Tail] = *Item;
  Queue->Tail = (Queue->Tail + 1) % MAX_ADDRESS_RECORD_SIZE;
}
OKN_DIMM_ADDRESS_DETAIL *DequeueError(DIMM_ERROR_QUEUE *Queue)
{
  if (IsErrorQueueEmpty(Queue)) {
    return NULL;
  }
  OKN_DIMM_ADDRESS_DETAIL *Item = &Queue->AddrBuffer[Queue->Head];
  Queue->Head = (Queue->Head + 1) % MAX_ADDRESS_RECORD_SIZE;
  return Item;
}
//OKN_20240827_yjb_ErrQueue <<

VOID GetMemInfoSpd(UINT8 Dimm, INT32 *ChipWidth, INT32 *Ranks)
{
  UINT8 Socket = Dimm / 8;
  UINT8 Channel = Dimm % 8;
  UINT8 Sid, Cid;
  Sid = Channel % 2;
  Cid = ((Socket << 3) + Channel) >> 1;
  *ChipWidth = 1 << (((mSpdTableDDR[Cid][Sid][6] >> 5) & 0x7) + 2);
  *Ranks = ((mSpdTableDDR[Cid][Sid][234] >> 3) & 0x07) + 1;
}

VOID injectPpr(cJSON *Tree) {
  EFI_STATUS Status = 0;
  cJSON *EntryList = cJSON_GetObjectItemCaseSensitive(Tree, "EntryList");
  cJSON *PprType = cJSON_GetObjectItemCaseSensitive(Tree, "PprType");
  BOOLEAN Success = FALSE;
  if (EntryList && (EntryList->type == cJSON_Array) && PprType && (PprType->type == cJSON_Number)) {
    UINT8 EntryNum = (UINT8)cJSON_GetArraySize(EntryList);
    if ((EntryNum > 0) && (EntryNum <= MAX_OKN_PPR_ENTRY_NUM)) {
      UINT32 Attributes;
      UINTN DataSize = sizeof(SOCKET_MEMORY_CONFIGURATION);
      SOCKET_MEMORY_CONFIGURATION Data;
      Status = gRT->GetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, &Attributes, &DataSize, &Data);
      if (Status == EFI_SUCCESS) {
        Data.OknPprEntryNum = EntryNum;
        Data.pprType = (UINT8)PprType->valueu64;
        for (UINT8 i = 0; i < EntryNum; i++) {
          cJSON *Entry = cJSON_GetArrayItem(EntryList, i);
          if (Entry && (Entry->type == cJSON_Array)) {
            UINT8 EntrySize = (UINT8)cJSON_GetArraySize(Entry);
            if (EntrySize == 9) {       // 9 member variables in OKN_PPR_ENTRY
              Data.OknPprEntry[i].socket = (UINT8)(cJSON_GetArrayItem(Entry, 0)->valueu64);
              Data.OknPprEntry[i].mc = (UINT8)(cJSON_GetArrayItem(Entry, 1)->valueu64);
              Data.OknPprEntry[i].ch = (UINT8)(cJSON_GetArrayItem(Entry, 2)->valueu64);
              Data.OknPprEntry[i].subCh = (UINT8)(cJSON_GetArrayItem(Entry, 3)->valueu64);
              Data.OknPprEntry[i].rank = (UINT8)(cJSON_GetArrayItem(Entry, 4)->valueu64);
              Data.OknPprEntry[i].subRank = (UINT8)(cJSON_GetArrayItem(Entry, 5)->valueu64);
              Data.OknPprEntry[i].nibbleMask = (UINT32)(cJSON_GetArrayItem(Entry, 6)->valueu64);
              Data.OknPprEntry[i].bank = (UINT8)(cJSON_GetArrayItem(Entry, 7)->valueu64);
              Data.OknPprEntry[i].row = (UINT32)(cJSON_GetArrayItem(Entry, 8)->valueu64);
              Status = gRT->SetVariable(SOCKET_MEMORY_CONFIGURATION_NAME, &gEfiSocketMemoryVariableGuid, Attributes, DataSize, &Data);
              if (Status == EFI_SUCCESS) {
                Success = TRUE;
              }
            }
          } // Entry && (Entry->type == cJSON_Array)
        } // for
      } // Status == EFI_SUCCESS
    } // (EntryNum > 0) && (EntryNum <= MAX_OKN_PPR_ENTRY_NUM)
  } // EntryList && (EntryList->type == cJSON_Array)
  cJSON_AddBoolToObject(Tree, "SUCCESS", Success);
}

VOID GetMapOutReason(UINT8 socket, UINT8 ch, cJSON *Reason) {
  DIMM_NVRAM_STRUCT     (*DimmNvList)[MAX_DIMM];

  DimmNvList = GetDimmNvList (socket, ch);
  for (UINT8 Rank = 0; Rank < 2; Rank++) {
    cJSON *r = cJSON_CreateNumber((*DimmNvList)[0].MapOutReason[Rank]);
    cJSON_AddItemToArray(Reason, r);
  }
}

VOID readEcsReg(cJSON *Tree) {
  EFI_STATUS        Status = EFI_NOT_FOUND;
  UINT8             Soc;
  UINT8             Ch;
  UINT8             Rank;
  UINT8             Subch;
  UINT8             Addr;
  UINT8             Die;
  RAS_ECS_READ_RESULT_STRUCT Data;

  cJSON *Slot = cJSON_GetObjectItemCaseSensitive(Tree, "Slot");
  if ((Slot != NULL) && (Slot->type == cJSON_Number)) {
    Status = 0;
    Soc = (UINT8)Slot->valueu64 / 8;
    Ch = (UINT8)Slot->valueu64 % 8;
  } else {
    goto end;
  }
  cJSON *MrRegs = cJSON_AddArrayToObject(Tree, "mrRegs");
  if (IsX4Dimm(Soc, Ch, 0)) {
    Die = 10;
  } else if (IsX8Dimm(Soc, Ch, 0)) {
    Die = 5;
  }

  for (Addr = 14; Addr < 21; Addr++) {
    cJSON *MrReg = cJSON_CreateArray();
    for (Rank = 0; Rank < GetNumberOfRanksOnDimm(Soc, Ch, 0); Rank++) {
      for (Subch = 0; Subch < 2; Subch++) {
        Status = MrReadRas(Soc, Ch, Subch, Rank, Addr, &Data, TRUE);
        if (Status == EFI_SUCCESS) {
          for (UINT8 i = 0; i < Die; i++) {
            cJSON *Value = cJSON_CreateNumber(((UINT8 *)(&Data))[i]);
            cJSON_AddItemToArray(MrReg, Value);
          }
        } else {
          goto end;
        }
      }
    }
    cJSON_AddItemToArray(MrRegs, MrReg);
  }

end:
  cJSON_AddBoolToObject(Tree, "SUCCESS", !Status);
}