#ifndef _OKN_MEM_TEST_PROTOCOL_H_
#define _OKN_MEM_TEST_PROTOCOL_H_

#include <Uefi.h>

#define OKN_MEMORY_TEST_PROTOCOL_GUID {0x03793b65, 0x5388, 0x479d, {0xa4, 0x4d, 0xec, 0xa5, 0xf3, 0x00, 0x92, 0x2d}}

extern EFI_GUID gOknMemTestProtocolGuid;

typedef enum {
  DimmRankMapOutUnknown           = 0,
  DimmRankMapOutMemDecode         = 1,
  DimmRankMapOutPopPorViolation   = 2,
  DimmRankMapOutRankDisabled      = 3,
  DimmRankMapOutAdvMemTestFailure = 4,
  DimmRankMapOutMax
} DIMM_RANK_MAP_OUT_REASON;

#pragma pack(1)

typedef struct {
  UINT16 DdrFreqLimit;
  UINT8  tCAS;
  UINT8  tRCD;
  UINT8  tRP;
  UINT16 tRAS;
  UINT8  tCWL;
  UINT8  tRTP;
  UINT8  tWR;
  UINT8  tRRD;
  UINT8  tFAW;
  UINT16 tREFI;
  UINT8  tWTR;
  UINT16 tRFC1;
  UINT16 tRC;
  UINT8  tCCD;
  UINT8  CommandTiming;  // 杨工说只能是1
  // Voltages
  UINT16 DfxPmicVdd;
  UINT16 DfxPmicVddQ;
  UINT16 DfxPmicVpp;
  // MISC
  UINT8 EnableRMT;
  UINT8 EnforcePopulationPor;
  UINT8 ErrorCheckScrub;  // ECS unsupported in DDR4
  UINT8 PprType;
  // ECC
  UINT8 DirectoryModeEn;  // SocketMplinkConfiguration
  UINT8 DdrEccEnable;
  UINT8 DdrEccCheck;
} OKN_MEMORY_CONFIGURATION;

typedef struct {
#define OKN_AMT_OPT_COUNT 20
//
// AMT_OPT_ARR[i] 内层数组下标映射
//
#define AMT_OPT_IDX_COND     0
#define AMT_OPT_IDX_VDD      1
#define AMT_OPT_IDX_VDDQ     2
#define AMT_OPT_IDX_TWR      3
#define AMT_OPT_IDX_TREFI    4
#define AMT_OPT_IDX_PAUSE    5
#define AMT_OPT_IDX_BGINTLV  6
#define AMT_OPT_IDX_ADDRMODE 7
#define AMT_OPT_IDX_PATTERN  8
#define AMT_INNER_MIN_ITEMS  9

  UINT8  AdvMemTestCondition;  // ——  0=disable(无意义) 1=Auto(默认, 使用BIOS默认参数测试) 2=Manual(覆盖BIOS设置值)
  UINT16 PmicVddLevel;         // ——  DDR4无效
  UINT16 PmicVddQLevel;        // ——  DDR4无效
  UINT8  TwrValue;             // ——  默认10(10-26), units = tCK
  UINT16 TrefiValue;           // ——  默认15600(3900-32767), units = nsec
  UINT32 AdvMemTestCondPause;  // ——  受Condition影响, Condition=2, 生效为设置值; Condition=1, 生效为预设值
  UINT8  BgInterleave;         // ——  每项测试默认都是2, 可取值1或者2
  UINT8  AddressMode;          // ——  取值0或者1(0=FAST_X  1=FAST_Y)
  UINT64 Pattern;              // ——  UINT64值即可(从basePatternQW换算为最终的Pattern)
} OKN_AMT_OPT;

typedef struct {
  UINT32      AdvMemTestOptions;  // —— 参考下述每个Bit定义
  UINT16      AdvMemTestLoops;    // —— 默认为1, 范围0-0xFF
  UINT8       AdvMemTestPpr;      // —— 0=disable 1=enable(默认), 属于AMT大测试的一个功能之一
  OKN_AMT_OPT AmtOptArr[OKN_AMT_OPT_COUNT];
} OKN_AMT_CONFIGURATION;

typedef struct {
  UINT8  Socket;
  UINT8  MemCtrl;
  UINT8  Channel;
  UINT8  SubCh;
  UINT8  Rank;
  UINT8  SubRank;
  UINT32 NibbleMask;
  UINT8  Bank;
  UINT32 Row;
} OKN_PPR_INTERFACE;

typedef enum {
  DIMM_ERROR_TYPE_UNKOWN,
  DIMM_ERROR_TYPE_DATA,
  DIMM_ERROR_TYPE_ECC_CORRECTED,
  DIMM_ERROR_TYPE_ECC_UNCORRECTED
} OKN_DIMM_ERROR_TYPE;

typedef struct {
  UINT8  SocketId;   // 0..1
  UINT8  MemCtrlId;  // 0..3
  UINT8  ChannelId;  // 0..1
  UINT8  DimmSlot;   // 0..1
  UINT8  RankId;     // 0..1
  UINT8  SubChId;    // 0..1 always 0 in DDR4
  UINT8  BankGroup;  // 0..7
  UINT8  Bank;       // 0..3
  UINT32 Row;
  UINT32 Column;
} OKN_DIMM_ADDRESS_DETAIL;

typedef struct {
  OKN_DIMM_ADDRESS_DETAIL AddrDetail;
  OKN_DIMM_ERROR_TYPE     Type;
} OKN_DIMM_ADDRESS_DETAIL_PLUS;

#pragma pack()

typedef EFI_STATUS(EFIAPI *OKN_IS_DIMM_PRESENT)(IN UINT8     Socket,
                                                IN UINT8     Channel,
                                                IN UINT8     Dimm,
                                                OUT BOOLEAN *pPresent);
typedef EFI_STATUS(EFIAPI *OKN_GET_MEM_CONFIG)(OUT OKN_MEMORY_CONFIGURATION *Config);
typedef EFI_STATUS(EFIAPI *OKN_GET_MEM_CONFIG_REAL)(OUT OKN_MEMORY_CONFIGURATION *Config);
typedef EFI_STATUS(EFIAPI *OKN_SET_MEM_CONFIG)(IN CONST OKN_MEMORY_CONFIGURATION *Config);
typedef EFI_STATUS(EFIAPI *OKN_GET_AMT_CONFIG)(OUT OKN_AMT_CONFIGURATION *Config);
typedef EFI_STATUS(EFIAPI *OKN_SET_AMT_CONFIG)(IN CONST OKN_AMT_CONFIGURATION *Config);
typedef EFI_STATUS(EFIAPI *OKN_GET_MEM_DISABLED_REASON)(IN UINT8                      Socket,
                                                        IN UINT8                      Channel,
                                                        IN UINT8                      Dimm,
                                                        OUT DIMM_RANK_MAP_OUT_REASON *Reason);
typedef EFI_STATUS(EFIAPI *OKN_GET_DIMM_TEMP)(IN UINT8   Socket,
                                              IN UINT8   Channel,
                                              IN UINT8   Dimm,
                                              OUT UINT8 *Ts0,
                                              OUT UINT8 *Ts1,
                                              OUT UINT8 *Hub);
// EntryNum Max Value is 5.
typedef EFI_STATUS(EFIAPI *OKN_INJECT_PPR)(IN UINT8 EntryNum, IN OKN_PPR_INTERFACE *PprEntry);
typedef EFI_STATUS(
    EFIAPI *OKN_SPD_READ)(IN UINT8 Socket, IN UINT8 Channel, IN UINT8 Dimm, OUT UINT8 *SpdData, OUT INTN *DataLen);
typedef EFI_STATUS(EFIAPI *OKN_SPD_WRITE)(IN UINT8 Socket, IN UINT8 Channel, IN UINT8 Dimm, IN CONST UINT8 *SpdData);
/**
 * 这个函数其实就是egs repo中的: EFI_STATUS SysToDimm(UINTN Address, OKN_DIMM_ADDRESS_DETAIL *DimmAddress);
 */
typedef EFI_STATUS(EFIAPI *OKN_ADDRESS_SYSTEM_TO_DIMM)(IN UINTN                     SystemAddress,
                                                       OUT OKN_DIMM_ADDRESS_DETAIL *TranslatedAddress);

typedef struct {
  UINT32                      Revision;          // XX
  OKN_IS_DIMM_PRESENT         IsDimmPresent;     // Done
  OKN_GET_MEM_CONFIG          GetMemConfig;      // Done
  OKN_GET_MEM_CONFIG_REAL     GetMemConfigReal;  // Done
  OKN_SET_MEM_CONFIG          SetMemConfig;      // Done
  OKN_GET_AMT_CONFIG          GetAmtConfig;  // 需要针对具体的AMT测试项来实现这个函数? 该函数接口应该需要加一个传入参数
  OKN_SET_AMT_CONFIG          SetAmtConfig;  // Done
  OKN_GET_MEM_DISABLED_REASON GetDisReason;  // Done DIMM_RANK_MAP_OUT_REASON
  OKN_GET_DIMM_TEMP           GetDimmTemp;   // Done
  OKN_INJECT_PPR              InjectPpr;     // XX
  OKN_SPD_READ                SpdRead;       // Done
  OKN_SPD_WRITE               SpdWrite;      // XX
  OKN_ADDRESS_SYSTEM_TO_DIMM  AddrSys2Dimm;  // Done
} OKN_MEMORY_TEST_PROTOCOL;

#endif  // _OKN_MEM_TEST_PROTOCOL_H_
