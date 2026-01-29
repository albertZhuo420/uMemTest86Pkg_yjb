#include <Library/OKN/PortingLibs/Udp4SocketLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleNetwork.h>
#include <Protocol/Udp4.h>
#include <Protocol/UsbIo.h>

#include <Library/PrintLib.h>
#include <Library/UefiLib.h>

UDP4_SOCKET *gOknUdpSocketTransmit = NULL;
UDP4_SOCKET *gUdpRxSockets[MAX_UDP4_RX_SOCKETS];
UINTN        gUdpRxSocketCount    = 0;
UDP4_SOCKET *gUdpRxActiveSocket   = NULL;
EFI_HANDLE   gUdpRxActiveSbHandle = NULL;
UDP4_SOCKET *gJsonCtxSocket = NULL;

static EFI_HANDLE sDefaultUdp4SbHandle = NULL;
static CHAR8      sStartOfBuffer[MAX_UDP4_FRAGMENT_LENGTH];



STATIC VOID PrintMac(IN CONST EFI_MAC_ADDRESS *Mac, IN UINT32 HwAddrSize)
{
  for (UINT32 i = 0; i < HwAddrSize; i++) {
    Print(L"%02x", (UINTN)Mac->Addr[i]);
    if (i + 1 < HwAddrSize)
      Print(L":");
  }
}

EFI_STATUS EFIAPI CreateUdp4SocketByServiceBindingHandle(IN EFI_HANDLE            Udp4ServiceBindingHandle,
                                                         IN EFI_UDP4_CONFIG_DATA *ConfigData,
                                                         IN EFI_EVENT_NOTIFY      NotifyReceive,
                                                         IN EFI_EVENT_NOTIFY      NotifyTransmit,
                                                         OUT UDP4_SOCKET        **Socket)
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL *Udp4Service;

  if (NULL == Udp4ServiceBindingHandle || NULL == ConfigData || NULL == NotifyReceive || NULL == NotifyTransmit ||
      Socket) {
    return EFI_INVALID_PARAMETER;
  }

  *Socket     = NULL;
  Udp4Service = NULL;

  Status = gBS->HandleProtocol(Udp4ServiceBindingHandle, &gEfiUdp4ServiceBindingProtocolGuid, (VOID **)&Udp4Service);
  if (EFI_ERROR(Status) || NULL == Udp4Service) {
    return EFI_NOT_FOUND;
  }

  *Socket = AllocateZeroPool(sizeof(UDP4_SOCKET));
  if (NULL == *Socket) {
    return EFI_OUT_OF_RESOURCES;
  }

  (*Socket)->ServiceBindingHandle = Udp4ServiceBindingHandle;
  (*Socket)->ServiceBinding       = Udp4Service;

  Status = Udp4Service->CreateChild(Udp4Service, &(*Socket)->ChildHandle);

  if (EFI_ERROR(Status)) {
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  Status = gBS->OpenProtocol((*Socket)->ChildHandle,
                             &gEfiUdp4ProtocolGuid,
                             (VOID **)&(*Socket)->Udp4,
                             gImageHandle,
                             NULL,
                             EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if (EFI_ERROR(Status) || NULL == (*Socket)->Udp4) {
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  (*Socket)->ConfigData = *ConfigData;
  Status                = (*Socket)->Udp4->Configure((*Socket)->Udp4, ConfigData);
  if (EFI_ERROR(Status)) {
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  // IMPORTANT:
  // For EFI_UDP4_PROTOCOL.Receive(), the caller must submit the token with
  // Token.Packet.RxData. The UDP4 implementation will allocate an
  // EFI_UDP4_RECEIVE_DATA buffer and return it via Token.Packet.RxData on
  // completion. The caller must recycle the buffer by signaling
  // RxData->RecycleSignal.
  (*Socket)->TokenReceive.Packet.RxData = NULL;

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyReceive, *Socket, &(*Socket)->TokenReceive.Event);
  if (EFI_ERROR(Status)) {
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  (*Socket)->TokenTransmit.Packet.TxData = AllocateZeroPool(sizeof(EFI_UDP4_TRANSMIT_DATA));
  if ((*Socket)->TokenTransmit.Packet.TxData) {
    gBS->CloseEvent((*Socket)->TokenReceive.Event);
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_CALLBACK, NotifyTransmit, *Socket, &(*Socket)->TokenTransmit.Event);
  if (EFI_ERROR(Status)) {
    FreePool((*Socket)->TokenTransmit.Packet.TxData);
    gBS->CloseEvent((*Socket)->TokenReceive.Event);
    Udp4Service->DestroyChild(Udp4Service, (*Socket)->ChildHandle);
    FreePool(*Socket);
    *Socket = NULL;
    return Status;
  }

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI CreateUdp4Socket(IN EFI_UDP4_CONFIG_DATA *ConfigData,
                                   IN EFI_EVENT_NOTIFY      NotifyReceive,
                                   IN EFI_EVENT_NOTIFY      NotifyTransmit,
                                   OUT UDP4_SOCKET        **Socket)
{
  if (NULL == sDefaultUdp4SbHandle) {
    return EFI_NOT_FOUND;
  }

  return CreateUdp4SocketByServiceBindingHandle(sDefaultUdp4SbHandle,
                                                ConfigData,
                                                NotifyReceive,
                                                NotifyTransmit,
                                                Socket);
}

EFI_STATUS EFIAPI CloseUdp4Socket(IN UDP4_SOCKET *Socket)
{
  EFI_STATUS Status;

  if (NULL == Socket) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;

  if (Socket->Udp4 != NULL) {
    Status = Socket->Udp4->Cancel(Socket->Udp4, NULL);
    Status |= Socket->Udp4->Configure(Socket->Udp4, NULL);
  }

  if (Socket->TokenReceive.Event != NULL) {
    Status |= gBS->CloseEvent(Socket->TokenReceive.Event);
  }
  if (Socket->TokenTransmit.Event != NULL) {
    Status |= gBS->CloseEvent(Socket->TokenTransmit.Event);
  }

  if (Socket->ServiceBinding != NULL && Socket->ChildHandle != NULL) {
    Status |= Socket->ServiceBinding->DestroyChild(Socket->ServiceBinding, Socket->ChildHandle);
  }

  // NOTE: Receive buffers are owned by UDP4 and must be recycled via
  // RxData->RecycleSignal. Do not FreePool() them here.
  if (Socket->TokenTransmit.Packet.TxData != NULL) {
    FreePool(Socket->TokenTransmit.Packet.TxData);
  }

  FreePool(Socket);
  Socket = NULL;
  return Status;
}

EFI_STATUS EFIAPI AsciiUdp4Write(IN UDP4_SOCKET *Socket, IN CONST CHAR8 *FormatString, ...)
{
  VA_LIST Marker;
  UINT32  NumberOfPrinted;

  if (NULL == Socket) {
    return EFI_NOT_FOUND;
  }

  VA_START(Marker, FormatString);
  NumberOfPrinted = (UINT32)AsciiVSPrint(sStartOfBuffer, MAX_UDP4_FRAGMENT_LENGTH, FormatString, Marker);
  VA_END(Marker);

  EFI_UDP4_TRANSMIT_DATA *TxData = Socket->TokenTransmit.Packet.TxData;
  ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
  TxData->DataLength                      = NumberOfPrinted;
  TxData->FragmentCount                   = 1;
  TxData->FragmentTable[0].FragmentLength = NumberOfPrinted;
  TxData->FragmentTable[0].FragmentBuffer = sStartOfBuffer;
  return Socket->Udp4->Transmit(Socket->Udp4, &Socket->TokenTransmit);
}

RETURN_STATUS EFIAPI Udp4SocketLibConstructor(VOID)
{
  EFI_STATUS           Status;
  UINTN                HandleNum;
  EFI_HANDLE          *Udp4Handles;
  EFI_USB_IO_PROTOCOL *UsbIo;

  sDefaultUdp4SbHandle = NULL;

  Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUdp4ServiceBindingProtocolGuid, NULL, &HandleNum, &Udp4Handles);
  if (EFI_ERROR(Status)) {
    Print(L"LocateHandleBuffer status: %r\n", Status);
    return Status;
  }

  for (UINTN i = 0; i < HandleNum; i++) {
    UsbIo = NULL;
    // 跳过USB网卡
    if (!EFI_ERROR(gBS->HandleProtocol(Udp4Handles[i], &gEfiUsbIoProtocolGuid, (VOID **)&UsbIo))) {
      continue;
    }

    sDefaultUdp4SbHandle = Udp4Handles[i];
    break;
  }

  if (Udp4Handles != NULL) {
    FreePool(Udp4Handles);
    Udp4Handles = NULL;
  }

  return (sDefaultUdp4SbHandle != NULL) ? RETURN_SUCCESS : RETURN_NOT_FOUND;
}
