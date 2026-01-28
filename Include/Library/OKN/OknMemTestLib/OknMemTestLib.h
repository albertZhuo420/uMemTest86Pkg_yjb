#ifndef _OKN_MEM_TEST_UTILS_LIB_H_
#define _OKN_MEM_TEST_UTILS_LIB_H_

#include <Library/OKN/OknMemTestProtocol/Protocol/OknMemTestProtocol.h>  // 放在 最上方

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>        // ZeroMem
#include <Library/MemoryAllocationLib.h>  // AllocateZeroPool/FreePool
#include <Library/OKN/OknMemTestLib/OknMemTestLib.h>
#include <Library/OKN/PortingLibs/cJSONLib.h>
#include <Library/UefiLib.h>  // Print

#define OKN_MAGIC_NUMBER 9527

/**
 * 这些函数要不要加EFIAPI? 以后再进一步研究吧
 */
extern DIMM_ERROR_QUEUE          gDimmErrorQueue;
extern OKN_MEMORY_TEST_PROTOCOL *gOknMtProtoPtr;
extern EFI_HANDLE                gOknChosenHandle;

/**
 * uMemTest86Pkg的全局变量
 */
/**
 * 探测到的 TSOD 模块数量, DIMM 上温度传感器/TSOD hub 可读到的条目数
 * uMemTest86Pkg/SupportLib.c->MtSupportGetTSODInfo()->switch (g_TSODController[i].tsodDriver)分支中
 * 的函数通过 g_numTSODModules++ 来统计探测到的 TSOD 模块数量;
 * 也就是说: TSOD 的探测是通过扫描 g_TSODController[] 上的 SMBus/TSOD 控制器, 然后调用 smbGetTSOD...()
 * 系列函数, 最终把结果写入全局:
 *   - g_numTSODModules: 总数;
 *   - g_MemTSODInfo[]: 每个模块的 channel/slot/temperature/raw...
 */
extern int g_numTSODModules;
extern int g_numSMBIOSMem;  // Num of SMBIOS memory devices, 在smbios_mem_info()中被赋值

EFI_STATUS OknMT_InitProtocol(VOID);
EFI_STATUS OknMT_ConfigSet(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);
EFI_STATUS OknMT_ConfigGet(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
EFI_STATUS OknMT_ConfigActive(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
EFI_STATUS OknMT_ReadSpdToJson(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree);
EFI_STATUS OknMT_SetAmtConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);
EFI_STATUS OknMT_IsDimmPresent(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree, OUT BOOLEAN *pPresent);
EFI_STATUS OknMT_GetDimmTemperature(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree, OUT UINT8 *pTSensor0);
EFI_STATUS OknMT_GetDimmRankMapOutReason(IN OKN_MEMORY_TEST_PROTOCOL  *pProto,
                                         IN CONST cJSON               *pJsTree,
                                         OUT DIMM_RANK_MAP_OUT_REASON *pReason);
EFI_STATUS OknMT_TranslatedAddressFromSystemToDimm(IN OKN_MEMORY_TEST_PROTOCOL *pProto,
                                                   IN UINTN                     SystemAddress,
                                                   OUT DIMM_ADDRESS_DETAIL     *pTranslatedAddress);
/**
 * 下面是一些辅助工具函数
 */
EFI_STATUS OknMT_LocateProtocol(IN INTN                        RequestedIndex,
                                OUT OKN_MEMORY_TEST_PROTOCOL **pProto,
                                OUT EFI_HANDLE                *pChosenHandle,
                                OUT UINTN                     *TotalHandles);
VOID       OknMT_PrintMemCfg(IN CONST OKN_MEMORY_CONFIGURATION *Cfg);
VOID       OknMT_PrintHandleSummary(IN EFI_HANDLE Handle);
EFI_STATUS OknMT_SetMemCfgToJson(IN CONST OKN_MEMORY_CONFIGURATION *pCfg, OUT cJSON *pJsTree);
EFI_STATUS OknMT_GetMemCfgTFromJson(IN CONST cJSON *pJsTree, OUT OKN_MEMORY_CONFIGURATION *pCfg);
EFI_STATUS OknMT_ConfigGetFunc(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN BOOLEAN Real, OUT cJSON *pJsTree);
EFI_STATUS OknMT_GetSocketChannelDImmPtrsFromJson(IN OKN_MEMORY_TEST_PROTOCOL *pProto,
                                                  IN CONST cJSON              *pJsTree,
                                                  OUT cJSON                   *pSocket,
                                                  OUT cJSON                   *pChannel,
                                                  OUT cJSON                   *pDimm);

/**
 * OknMemTestLibJson.c
 */
EFI_STATUS JsonGetU64(IN CONST cJSON *Item, OUT UINT64 *Val);
EFI_STATUS JsonGetU32FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT32 *Val);
EFI_STATUS JsonGetU16FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT16 *Val);
EFI_STATUS JsonGetU8FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT8 *Val);
EFI_STATUS JsonGetU16FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT16 *Val);
EFI_STATUS JsonGetU32FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT32 *Val);

#endif  // _OKN_MEM_TEST_UTILS_LIB_H_
