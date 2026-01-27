#ifndef _OKN_MEM_TEST_UTILS_LIB_H_
#define _OKN_MEM_TEST_UTILS_LIB_H_

#include <Library/OKN/OknMemTestProtocol/Protocol/OknMemTestProtocol.h>

/**
 * 这些函数要不要加EFIAPI? 以后再进一步研究吧
 */
extern DIMM_ERROR_QUEUE          gDimmErrorQueue;
extern OKN_MEMORY_TEST_PROTOCOL *gOknMtProtoPtr;
extern EFI_HANDLE                gOknChosenHandle;

EFI_STATUS OknMT_InitProtocol(VOID);
EFI_STATUS OknMT_ConfigSet(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);
EFI_STATUS OknMT_ConfigGet(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
EFI_STATUS OknMT_ConfigActive(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
EFI_STATUS OknMT_ReadSpdToJson(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree);
EFI_STATUS OknMT_SetAmtConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);

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

EFI_STATUS JsonGetU64(IN CONST cJSON *Item, OUT UINT64 *Val);
EFI_STATUS JsonGetU32FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT32 *Val);
EFI_STATUS JsonGetU16FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT16 *Val);
EFI_STATUS JsonGetU8FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT8 *Val);
EFI_STATUS JsonGetU16FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT16 *Val);
EFI_STATUS JsonGetU32FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT32 *Val);

#endif  // _OKN_MEM_TEST_UTILS_LIB_H_
