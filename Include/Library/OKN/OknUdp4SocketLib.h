#ifndef _OKN_UDP4_SOCKET_LIB_H_
#define _OKN_UDP4_SOCKET_LIB_H_

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Dhcp4.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/Udp4.h>
#include <Protocol/UsbIo.h>

#define MAX_UDP4_FRAGMENT_LENGTH SIZE_2KB

#define MAX_UDP4_RX_SOCKETS 16

#define OKN_STATION_UDP_PORT 9527

typedef struct {
  // 归属关系：这个 UDP4 child 属于哪个 UDP4 ServiceBinding(也就是哪张 NIC)
  EFI_HANDLE                    ServiceBindingHandle;
  EFI_SERVICE_BINDING_PROTOCOL *ServiceBinding;

  // child + protocol
  EFI_HANDLE         ChildHandle;
  EFI_UDP4_PROTOCOL *Udp4Proto;

  // config + tokens
  EFI_UDP4_CONFIG_DATA      ConfigData;
  EFI_UDP4_COMPLETION_TOKEN TokenReceive;
  EFI_UDP4_COMPLETION_TOKEN TokenTransmit;

  // NIC 身份(用于 JSON 回包映射)
  UINT8            NicMac[6];
  EFI_IPv4_ADDRESS NicIp;
  BOOLEAN          NicIpValid;

  // TX 完成回调释放内存
  VOID                  *TxPayload;
  EFI_UDP4_SESSION_DATA *TxSession;
  BOOLEAN                TxInProgress;
} OKN_UDP4_ENDPOINT;

extern UINTN              gOknUdpEdpOnlineCnt;
extern OKN_UDP4_ENDPOINT *gOknJsonCtxUdpEdp;

/**
 * 下面的四个函数必须要有 EFIAPI, 包括函数的实现, 这是一个天坑
 */
RETURN_STATUS EFIAPI OknUdp4SocketLibConstructor(VOID);
VOID EFIAPI          OknUdp4ReceiveHandler(IN EFI_EVENT Event, IN VOID *Context);
VOID EFIAPI          OknUdp4TxFreeHandler(IN EFI_EVENT Event, IN VOID *Context);
VOID EFIAPI          OknUdp4NullHandler(IN EFI_EVENT Event, IN VOID *Context);

EFI_STATUS OknStartUdp4ReceiveOnAllNics(IN EFI_UDP4_CONFIG_DATA *RxCfg);
EFI_STATUS OknWaitForUdpNicBind(UINTN TimeoutMs);
VOID       OknDumpNetProtoCounts(VOID);
VOID       OknConnectAllSnpControllers(VOID);

#endif  // _OKN_UDP4_SOCKET_LIB_H_
