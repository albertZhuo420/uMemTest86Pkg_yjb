#ifndef _OKN_MEM_TEST_UTILS_LIB_H_
#define _OKN_MEM_TEST_UTILS_LIB_H_

#include <Library/OKN/OknMemTestProtocol.h>  // 放在 最上方

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>        // ZeroMem
#include <Library/MemoryAllocationLib.h>  // AllocateZeroPool/FreePool
#include <Library/OKN/PortingLibs/cJSONLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/UefiLib.h>  // Print
#include <uMemTest86.h>

#define OKN_MT86

#define OKN_MAX_DIMM_CNT 8

#define OKN_MAGIC_NUMBER 9527
#define OKN_BUF_SIZE     2048

#define OKN_STRING_EQUAL 0

#define OknDebugPrintAndStop()                                                                                         \
  do {                                                                                                                 \
    UINTN         EventIndex_tmp;                                                                                      \
    EFI_INPUT_KEY key;                                                                                                 \
    Print(L"[%a:%d] ...\r\n", OknMT_GetFileBaseName(__FILE__), __LINE__);                                              \
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex_tmp);                                                    \
    gST->ConIn->ReadKeyStroke(gST->ConIn, &key);                                                                       \
  } while (0)

#define OknWaitForKeyForever()                                                                                         \
  do {                                                                                                                 \
    UINTN         EventIndex_tmp;                                                                                      \
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex_tmp);                                                    \
  } while (TRUE)

/**
 * 后期可以添加idx, 能不能连的上, 连不上就说明有遗漏
 */
typedef struct {
#define OKN_MAX_ADDRESS_RECORD_SIZE 32

  SPIN_LOCK Lock;
  BOOLEAN   LockInitialized;

  OKN_DIMM_ADDRESS_DETAIL AddrBuffer[OKN_MAX_ADDRESS_RECORD_SIZE];
  UINTN                        Head;
  UINTN                        Tail;
} DIMM_ERROR_QUEUE;

typedef enum {
  OKN_TST_Finish  = 0,
  OKN_TST_Testing = 1,
  OKN_TST_Aborted = 2,
  OKN_TST_Unknown = 0xFF
} OKN_TEST_STATUS_TYPE;

/// @brief OKN_EFI_RESET_TYPE 来自 MdePkg/Include/Uefi/UefiMultiPhase.h: EFI_RESET_TYPE
typedef enum {
  ///
  /// Used to induce a system-wide reset. This sets all circuitry within the
  /// system to its initial state.  This type of reset is asynchronous to system
  /// operation and operates withgout regard to cycle boundaries.  EfiColdReset
  /// is tantamount to a system power cycle.
  ///
  Okn_EfiResetCold,
  ///
  /// Used to induce a system-wide initialization. The processors are set to their
  /// initial state, and pending cycles are not corrupted.  If the system does
  /// not support this reset type, then an EfiResetCold must be performed.
  ///
  Okn_EfiResetWarm,
  ///
  /// Used to induce an entry into a power state equivalent to the ACPI G2/S5 or G3
  /// state.  If the system does not support this reset type, then when the system
  /// is rebooted, it should exhibit the EfiResetCold attributes.
  ///
  Okn_EfiResetShutdown,
  ///
  /// Used to induce a system-wide reset. The exact type of the reset is defined by
  /// the EFI_GUID that follows the Null-terminated Unicode string passed into
  /// ResetData. If the platform does not recognize the EFI_GUID in ResetData the
  /// platform must pick a supported reset type to perform. The platform may
  /// optionally log the parameters from any non-normal reset that occurs.
  ///
  Okn_EfiResetPlatformSpecific,
  ///
  /// Okn_EfiResetNone 是一个无效的TYPE, 用于初始化
  ///
  Okn_EfiResetNone
} OKN_EFI_RESET_TYPE;

extern UINTN                gOknLastPercent;
extern BOOLEAN              gOKnSkipWaiting;
extern BOOLEAN              gOknTestStart;
extern BOOLEAN              gOknTestPause;
extern OKN_EFI_RESET_TYPE   gOknTestReset;
extern OKN_TEST_STATUS_TYPE gOknTestStatus;  // 0:end 1:testing 2:abort
extern INT8                 gOknMT86TestID;  // 这个是MT86真实的TestID [0 ... 15]

extern DIMM_ERROR_QUEUE          gOknDimmErrorQueue;
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
extern int      g_numTSODModules;
extern int      g_numSMBIOSMem;   // Num of SMBIOS memory devices, 在smbios_mem_info()中被赋值
extern int      gNumCustomTests;  // Size of gCustomTestList;
extern TESTCFG *gCustomTestList;  // contains list of custom test configurations, if specified
extern UINTN    gNumPasses;       // number of passes of the test sequence to perform

typedef struct {
  OKN_MEMORY_TEST_PROTOCOL *Proto;      // 可为 NULL（某些命令不需要）
  cJSON                    *Tree;       // IN/OUT：请求树 & 回包树（你现在就是这么用的）
  OKN_EFI_RESET_TYPE        ResetType;  // ResetSystem 成功后填
  BOOLEAN                   ResetReq;   // 是否请求重启
} OKN_MT_CMD_CTX;

typedef EFI_STATUS (*OKN_MT_CMD_HANDLER)(OKN_MT_CMD_CTX *Ctx);

typedef struct {
  CONST CHAR8       *CmdName;
  OKN_MT_CMD_HANDLER Handler;
} OKN_MT_CMD_DISPATCH;

EFI_STATUS OknMT_InitProtocol(VOID);
EFI_STATUS OknMT_SetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);
EFI_STATUS OknMT_GetMemConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
EFI_STATUS OknMT_GetMemConfigReal(IN OKN_MEMORY_TEST_PROTOCOL *pProto, OUT cJSON *pJsTree);
EFI_STATUS OknMT_ReadSpdToJson(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN OUT cJSON *pJsTree);
EFI_STATUS OknMT_SetAmtConfig(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree);
EFI_STATUS OknMT_IsDimmPresent(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree, OUT BOOLEAN *pPresent);
EFI_STATUS OknMT_GetDimmTemperature(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN CONST cJSON *pJsTree, OUT UINT8 *pTSensor0);
EFI_STATUS OknMT_GetDimmRankMapOutReason(IN OKN_MEMORY_TEST_PROTOCOL  *pProto,
                                         IN CONST cJSON               *pJsTree,
                                         OUT DIMM_RANK_MAP_OUT_REASON *pReason);
EFI_STATUS OknMT_TranslatedAddressFromSystemToDimm(IN OKN_MEMORY_TEST_PROTOCOL      *pProto,
                                                   IN UINTN                          SystemAddress,
                                                   OUT OKN_DIMM_ADDRESS_DETAIL_BASE *pTranslatedAddress);
/**
 * 下面是一些辅助工具函数
 */
EFI_STATUS   OknMT_LocateProtocol(IN INTN                        RequestedIndex,
                                  OUT OKN_MEMORY_TEST_PROTOCOL **pProto,
                                  OUT EFI_HANDLE                *pChosenHandle,
                                  OUT UINTN                     *TotalHandles);
VOID         OknMT_PrintMemCfg(IN CONST OKN_MEMORY_CONFIGURATION *Cfg);
VOID         OknMT_PrintHandleSummary(IN EFI_HANDLE Handle);
EFI_STATUS   OknMT_SetMemCfgToJson(IN CONST OKN_MEMORY_CONFIGURATION *pCfg, OUT cJSON *pJsTree);
EFI_STATUS   OknMT_GetMemCfgFromJson(IN CONST cJSON *pJsTree, OUT OKN_MEMORY_CONFIGURATION *pCfg);
EFI_STATUS   OknMT_GetMemConfigFunc(IN OKN_MEMORY_TEST_PROTOCOL *pProto, IN BOOLEAN Real, OUT cJSON *pJsTree);
EFI_STATUS   OknMT_GetSocketChannelDImmPtrsFromJson(IN OKN_MEMORY_TEST_PROTOCOL *pProto,
                                                    IN CONST cJSON              *pJsTree,
                                                    OUT cJSON                   **ppSocket,
                                                    OUT cJSON                   **ppChannel,
                                                    OUT cJSON                   **ppDimm);
CONST CHAR8 *OknMT_GetFileBaseName(IN CONST CHAR8 *Path);

EFI_STATUS
OknMT_DispatchJsonCmd(IN OKN_MEMORY_TEST_PROTOCOL *Proto, IN OUT cJSON *Tree, OUT OKN_EFI_RESET_TYPE *OutResetType);
/**
 * OknMemTestLibJson.c
 */
EFI_STATUS OknMT_JsonGetU64(IN CONST cJSON *Item, OUT UINT64 *Val);
EFI_STATUS OknMT_JsonGetU32FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT32 *Val);
EFI_STATUS OknMT_JsonGetU16FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT16 *Val);
EFI_STATUS OknMT_JsonGetU8FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT8 *Val);
EFI_STATUS OknMT_JsonGetU8FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT8 *Val);
EFI_STATUS OknMT_JsonGetU16FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT16 *Val);
EFI_STATUS OknMT_JsonGetU32FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT32 *Val);

EFI_STATUS OknMT_JsonSetBool(IN cJSON *Obj, IN CONST CHAR8 *Key, IN BOOLEAN Val);
EFI_STATUS OknMT_JsonSetString(IN cJSON *Obj, IN CONST CHAR8 *Key, IN CONST CHAR8 *Val);
EFI_STATUS OknMT_JsonSetNumber(IN cJSON *Obj, IN CONST CHAR8 *Key, IN INTN Val);

/**
 * OknMemTestLibDimmErrQueue.c
 */
VOID       OknMT_InitErrorQueue(DIMM_ERROR_QUEUE *Queue);
BOOLEAN    OknMT_IsErrorQueueEmpty(DIMM_ERROR_QUEUE *Queue);
BOOLEAN    OknMT_IsErrorQueueFull(DIMM_ERROR_QUEUE *Queue);
VOID       OknMT_EnqueueError(DIMM_ERROR_QUEUE *Queue, OKN_DIMM_ADDRESS_DETAIL *Item);
EFI_STATUS OknMT_DequeueErrorCopy(IN OUT DIMM_ERROR_QUEUE *Queue, OUT OKN_DIMM_ADDRESS_DETAIL *OutItem);

#endif  // _OKN_MEM_TEST_UTILS_LIB_H_
