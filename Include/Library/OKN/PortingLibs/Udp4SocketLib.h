#ifndef __UDP4_SOCKET_LIB_H__
#define __UDP4_SOCKET_LIB_H__

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>

#include <Protocol/ServiceBinding.h>
#include <Protocol/Udp4.h>

#define MAX_UDP4_FRAGMENT_LENGTH    SIZE_2KB

#define MAX_UDP4_RX_SOCKETS 16

extern UDP4_SOCKET *gOknUdpSocketTransmit = NULL;
extern UDP4_SOCKET *gUdpRxSockets[MAX_UDP4_RX_SOCKETS];
extern UINTN        gUdpRxSocketCount    = 0;
extern UDP4_SOCKET *gUdpRxActiveSocket   = NULL;
extern EFI_HANDLE   gUdpRxActiveSbHandle = NULL;
extern UDP4_SOCKET *gJsonCtxSocket       = NULL;

typedef struct {
  // 归属关系：这个 UDP4 child 属于哪个 UDP4 ServiceBinding(也就是哪张 NIC)
  EFI_HANDLE                    ServiceBindingHandle;
  EFI_SERVICE_BINDING_PROTOCOL *ServiceBinding;

  // child + protocol
  EFI_HANDLE                ChildHandle;
  EFI_UDP4_PROTOCOL        *Udp4;

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
} UDP4_SOCKET;

EFI_STATUS EFIAPI CreateUdp4SocketByServiceBindingHandle(EFI_HANDLE Udp4ServiceBindingHandle, EFI_UDP4_CONFIG_DATA *ConfigData, EFI_EVENT_NOTIFY NotifyReceive, EFI_EVENT_NOTIFY NotifyTransmit, UDP4_SOCKET **Socket);
EFI_STATUS EFIAPI CreateUdp4Socket(EFI_UDP4_CONFIG_DATA *ConfigData, EFI_EVENT_NOTIFY NotifyReceive, EFI_EVENT_NOTIFY NotifyTransmit, UDP4_SOCKET **Socket);
EFI_STATUS EFIAPI CloseUdp4Socket(UDP4_SOCKET *Socket);
EFI_STATUS EFIAPI AsciiUdp4Write(UDP4_SOCKET *Socket, CONST CHAR8 *FormatString, ...);

#endif