#ifndef __INTEL_PORTING_LIB_H__
#define __INTEL_PORTING_LIB_H__

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/cJSONLib.h>
#include <Library/IoLib.h>

#include <Guid/DxeServices.h>

#define MAX_ADDRESS_RECORD_SIZE   32

//OKN_20240822_yjb_EgsAddrTrans >>
typedef enum {
  DIMM_ERROR_TYPE_UNKOWN,
  DIMM_ERROR_TYPE_DATA,
  DIMM_ERROR_TYPE_ECC_CORRECTED,
  DIMM_ERROR_TYPE_ECC_UNCORRECTED
} DIMM_ERROR_TYPE;

typedef struct {
  UINTN     Address;
  UINT8     SocketId;           //0~1
  UINT8     MemCtrlId;          //0~3
  UINT8     ChannelId;          //0~1
  UINT8     DimmSlot;           //always 0
  UINT8     RankId;             //0~1
  UINT8     SubChId;            //0~1
  UINT8     BankGroup;          //0~7
  UINT8     Bank;               //0~3
  UINT32    Row;
  UINT32    Column;
  UINT32    Type;
} DIMM_ADDRESS_DETAIL;
//OKN_20240822_yjb_EgsAddrTrans <<

//OKN_20240827_yjb_ErrQueue >>
typedef struct {
  DIMM_ADDRESS_DETAIL       AddrBuffer[MAX_ADDRESS_RECORD_SIZE];
  UINTN                     Head;
  UINTN                     Tail;
} DIMM_ERROR_QUEUE;
//OKN_20240827_yjb_ErrQueue <<

extern DIMM_ERROR_QUEUE gDimmErrorQueue;

VOID testConfigGet(cJSON *Tree);
VOID testConfigSet(cJSON *Tree);
VOID testConfigActive(cJSON *Tree);
VOID regRead(cJSON *Tree);
VOID amtControl(cJSON *Tree, BOOLEAN Enable);
VOID rmtControl(cJSON *Tree, BOOLEAN Enable);
VOID trainMsgCtrl(cJSON *Tree);
VOID readSPD(cJSON *Tree);
VOID cpuSelect(cJSON *Tree);
VOID spdPrint(cJSON *Tree);
EFI_STATUS SysToDimm(UINTN Address, DIMM_ADDRESS_DETAIL *DimmAddress);      //OKN_20240822_yjb_EgsAddrTrans
//OKN_20240827_yjb_ErrQueue >>
VOID InitErrorQueue(DIMM_ERROR_QUEUE *Queue);
BOOLEAN IsErrorQueueEmpty(DIMM_ERROR_QUEUE *Queue);
BOOLEAN IsErrorQueueFull(DIMM_ERROR_QUEUE *Queue);
VOID EnqueueError(DIMM_ERROR_QUEUE *Queue, DIMM_ADDRESS_DETAIL *Item);
DIMM_ADDRESS_DETAIL *DequeueError(DIMM_ERROR_QUEUE *Queue);
//OKN_20240827_yjb_ErrQueue <<
VOID GetMemInfoSpd(UINT8 Dimm, INT32 *ChipWidth, INT32 *Ranks);
VOID injectPpr(cJSON *Tree);
VOID GetMapOutReason(UINT8 socket, UINT8 ch, cJSON *Reason);
VOID readEcsReg(cJSON *Tree);

#endif