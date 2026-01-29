#include <Library/OKN/OknMemTestLib/OknMemTestLib.h>

DIMM_ERROR_QUEUE gOknDimmErrorQueue;

STATIC VOID OknMT_EnsureErrorQueueLockInit(IN OUT DIMM_ERROR_QUEUE *Queue)
{
  if (NULL == Queue) {
    return;
  }

  if (!Queue->LockInitialized) {
    InitializeSpinLock(&Queue->Lock);
    Queue->LockInitialized = TRUE;
  }

  return;
}

STATIC BOOLEAN OknMT_IsErrorQueueEmptyNoLock(IN CONST DIMM_ERROR_QUEUE *Queue)
{
  return (Queue->Head == Queue->Tail);
}

STATIC BOOLEAN OknMT_IsErrorQueueFullNoLock(IN CONST DIMM_ERROR_QUEUE *Queue)
{
  return (((Queue->Tail + 1) % OKN_MAX_ADDRESS_RECORD_SIZE) == Queue->Head);
}

VOID OknMT_InitErrorQueue(DIMM_ERROR_QUEUE *Queue)
{
  if (NULL == Queue) {
    return;
  }

  OknMT_EnsureErrorQueueLockInit(Queue);

  AcquireSpinLock(&Queue->Lock);

  for (UINT8 i = 0; i < OKN_MAX_ADDRESS_RECORD_SIZE; i++) {
    ZeroMem(&Queue->AddrBuffer[i], sizeof(DIMM_ADDRESS_DETAIL));
  }
  Queue->Head = 0;
  Queue->Tail = 0;

  ReleaseSpinLock(&Queue->Lock);

  return;
}

BOOLEAN OknMT_IsErrorQueueEmpty(DIMM_ERROR_QUEUE *Queue)
{
  BOOLEAN Empty;

  if (NULL == Queue) {
    return TRUE;  // 安全起见：当成空
  }

  OknMT_EnsureErrorQueueLockInit(Queue);

  AcquireSpinLock(&Queue->Lock);
  Empty = OknMT_IsErrorQueueEmptyNoLock(Queue);
  ReleaseSpinLock(&Queue->Lock);

  return Empty;
}

BOOLEAN OknMT_IsErrorQueueFull(DIMM_ERROR_QUEUE *Queue)
{
  BOOLEAN Full;

  if (NULL == Queue) {
    return TRUE;  // 安全起见：当成满，避免调用方继续写
  }

  OknMT_EnsureErrorQueueLockInit(Queue);

  AcquireSpinLock(&Queue->Lock);
  Full = OknMT_IsErrorQueueFullNoLock(Queue);
  ReleaseSpinLock(&Queue->Lock);

  return Full;
}

VOID OknMT_EnqueueError(DIMM_ERROR_QUEUE *Queue, DIMM_ADDRESS_DETAIL *Item)
{
  if (NULL == Queue || NULL == Item) {
    return;
  }

  OknMT_EnsureErrorQueueLockInit(Queue);

  AcquireSpinLock(&Queue->Lock);

  if (OknMT_IsErrorQueueFullNoLock(Queue)) {
    // 丢弃最旧的一条
    Queue->Head = (Queue->Head + 1) % OKN_MAX_ADDRESS_RECORD_SIZE;
  }

  Queue->AddrBuffer[Queue->Tail] = *Item;
  Queue->Tail                    = (Queue->Tail + 1) % OKN_MAX_ADDRESS_RECORD_SIZE;

  ReleaseSpinLock(&Queue->Lock);

  return;
}

EFI_STATUS OknMT_DequeueErrorCopy(IN OUT DIMM_ERROR_QUEUE *Queue, OUT DIMM_ADDRESS_DETAIL *OutItem)
{
  if (NULL == Queue || NULL == OutItem) {
    return EFI_INVALID_PARAMETER;
  }

  OknMT_EnsureErrorQueueLockInit(Queue);

  AcquireSpinLock(&Queue->Lock);

  if (OknMT_IsErrorQueueEmptyNoLock(Queue)) {
    ReleaseSpinLock(&Queue->Lock);
    return EFI_NOT_FOUND;
  }

  CopyMem(OutItem, &Queue->AddrBuffer[Queue->Head], sizeof(DIMM_ADDRESS_DETAIL));
  Queue->Head = (Queue->Head + 1) % OKN_MAX_ADDRESS_RECORD_SIZE;

  ReleaseSpinLock(&Queue->Lock);

  return EFI_SUCCESS;
}
