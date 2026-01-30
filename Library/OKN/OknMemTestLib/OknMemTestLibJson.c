/**
 * @file
 *
 */

#include <Library/OKN/OknMemTestLib.h>

EFI_STATUS OknMT_JsonGetU64(IN CONST cJSON *Item, OUT UINT64 *Val)
{
  if (NULL == Item || NULL == Val) {
    return EFI_INVALID_PARAMETER;
  }

  if (cJSON_IsNumber((cJSON *)Item)) {
    *Val = (UINT64)Item->valueu64;
    return EFI_SUCCESS;
  }

  if (cJSON_IsString((cJSON *)Item) && Item->valuestring != NULL) {
    // 支持 "0x..." 或十进制字符串
    if ((Item->valuestring[0] == '0') && (Item->valuestring[1] == 'x' || Item->valuestring[1] == 'X')) {
      *Val = AsciiStrHexToUint64(Item->valuestring);
    }
    else {
      *Val = AsciiStrDecimalToUint64(Item->valuestring);
    }
    return EFI_SUCCESS;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS OknMT_JsonGetU32FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT32 *Val)
{
  cJSON *It;
  UINT64 Tmp;

  if (NULL == Obj || NULL == Key || NULL == Val) {
    return EFI_INVALID_PARAMETER;
  }

  It = cJSON_GetObjectItemCaseSensitive((cJSON *)Obj, Key);
  if (NULL == It) {
    return EFI_NOT_FOUND;
  }

  if (!cJSON_IsNumber(It)) {
    return EFI_INVALID_PARAMETER;
  }

  Tmp = (UINT64)It->valueu64;
  if (Tmp > 0xFFFFFFFFULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Val = (UINT32)Tmp;
  return EFI_SUCCESS;
}

EFI_STATUS OknMT_JsonGetU16FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT16 *Val)
{
  cJSON *It;
  UINT64 Tmp;

  if (NULL == Obj || NULL == Key || NULL == Val) {
    return EFI_INVALID_PARAMETER;
  }

  It = cJSON_GetObjectItemCaseSensitive((cJSON *)Obj, Key);
  if (NULL == It) {
    return EFI_NOT_FOUND;
  }

  if (!cJSON_IsNumber(It)) {
    return EFI_INVALID_PARAMETER;
  }

  Tmp = (UINT64)It->valueu64;
  if (Tmp > 0xFFFFULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Val = (UINT16)Tmp;
  return EFI_SUCCESS;
}

EFI_STATUS JsonGetU8FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT8 *Val)
{
  cJSON *It;
  UINT64 Tmp;

  if (NULL == Obj || NULL == Key || NULL == Val) {
    return EFI_INVALID_PARAMETER;
  }

  It = cJSON_GetObjectItemCaseSensitive((cJSON *)Obj, Key);
  if (NULL == It) {
    return EFI_NOT_FOUND;
  }

  if (!cJSON_IsNumber(It)) {
    return EFI_INVALID_PARAMETER;
  }

  Tmp = (UINT64)It->valueu64;
  if (Tmp > 0xFFULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Val = (UINT8)Tmp;
  return EFI_SUCCESS;
}

EFI_STATUS OknMT_JsonGetU8FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT8 *Val)
{
  cJSON *It;
  UINT64 Tmp;

  if (NULL == Arr || NULL == Val) {
    return EFI_INVALID_PARAMETER;
  }

  It = cJSON_GetArrayItem((cJSON *)Arr, Index);
  if (NULL == It) {
    return EFI_NOT_FOUND;
  }

  if (!cJSON_IsNumber(It)) {
    return EFI_INVALID_PARAMETER;
  }

  Tmp = (UINT64)It->valueu64;
  if (Tmp > 0xFFULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Val = (UINT8)Tmp;
  return EFI_SUCCESS;
}

EFI_STATUS OknMT_JsonGetU16FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT16 *Val)
{
  cJSON *It;
  UINT64 Tmp;

  if (NULL == Arr || NULL == Val) {
    return EFI_INVALID_PARAMETER;
  }

  It = cJSON_GetArrayItem((cJSON *)Arr, Index);
  if (NULL == It) {
    return EFI_NOT_FOUND;
  }

  if (!cJSON_IsNumber(It)) {
    return EFI_INVALID_PARAMETER;
  }

  Tmp = (UINT64)It->valueu64;
  if (Tmp > 0xFFFFULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Val = (UINT16)Tmp;
  return EFI_SUCCESS;
}

EFI_STATUS OknMT_JsonGetU32FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT32 *Val)
{
  cJSON *It;
  UINT64 Tmp;

  if (NULL == Arr || NULL == Val) {
    return EFI_INVALID_PARAMETER;
  }

  It = cJSON_GetArrayItem((cJSON *)Arr, Index);
  if (NULL == It) {
    return EFI_NOT_FOUND;
  }

  if (!cJSON_IsNumber(It)) {
    return EFI_INVALID_PARAMETER;
  }

  Tmp = (UINT64)It->valueu64;
  if (Tmp > 0xFFFFFFFFULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Val = (UINT32)Tmp;
  return EFI_SUCCESS;
}

// ---- JSON "add or replace" helpers (avoid duplicate keys) ----

EFI_STATUS OknMT_JsonSetBool(IN cJSON *Obj, IN CONST CHAR8 *Key, IN BOOLEAN Val)
{
  cJSON     *Old;
  cJSON     *NewItem;
  cJSON_bool Ok;

  if (NULL == Obj || NULL == Key) {
    return EFI_INVALID_PARAMETER;
  }

  NewItem = cJSON_CreateBool(Val ? 1 : 0);
  if (NULL == NewItem) {
    return EFI_OUT_OF_RESOURCES;
  }

  Old = cJSON_GetObjectItemCaseSensitive(Obj, Key);
  if (NULL != Old) {
    Ok = cJSON_ReplaceItemInObjectCaseSensitive(Obj, Key, NewItem);
    if (!Ok) {
      cJSON_Delete(NewItem);  // replace failed -> free to avoid leak
      return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
  }

  Ok = cJSON_AddItemToObject(Obj, Key, NewItem);
  if (!Ok) {
    cJSON_Delete(NewItem);  // add failed -> free to avoid leak
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_JsonSetString(IN cJSON *Obj, IN CONST CHAR8 *Key, IN CONST CHAR8 *Val)
{
  cJSON     *Old;
  cJSON     *NewItem;
  cJSON_bool Ok;

  if (NULL == Obj || NULL == Key) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Treat NULL Val as empty string to keep protocol stable
  //
  NewItem = cJSON_CreateString((NULL != Val) ? Val : "");
  if (NULL == NewItem) {
    return EFI_OUT_OF_RESOURCES;
  }

  Old = cJSON_GetObjectItemCaseSensitive(Obj, Key);
  if (NULL != Old) {
    //
    // ReplaceItem takes ownership of NewItem on success
    //
    Ok = cJSON_ReplaceItemInObjectCaseSensitive(Obj, Key, NewItem);
    if (!Ok) {
      cJSON_Delete(NewItem);  // replace failed -> free to avoid leak
      return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
  }

  //
  // AddItem takes ownership of NewItem on success
  //
  Ok = cJSON_AddItemToObject(Obj, Key, NewItem);
  if (!Ok) {
    cJSON_Delete(NewItem);  // add failed -> free to avoid leak
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS OknMT_JsonSetNumber(IN cJSON *Obj, IN CONST CHAR8 *Key, IN INTN Val)
{
  cJSON     *Old;
  cJSON     *NewItem;
  cJSON_bool Ok;

  if (NULL == Obj || NULL == Key) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // cJSON number is double; for large INTN (esp. 64-bit) precision may be lost.
  // Keep as-is to match existing code behavior.
  //
  NewItem = cJSON_CreateNumber((double)Val);
  if (NULL == NewItem) {
    return EFI_OUT_OF_RESOURCES;
  }

  Old = cJSON_GetObjectItemCaseSensitive(Obj, Key);
  if (NULL != Old) {
    Ok = cJSON_ReplaceItemInObjectCaseSensitive(Obj, Key, NewItem);
    if (!Ok) {
      cJSON_Delete(NewItem);
      return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;
  }

  Ok = cJSON_AddItemToObject(Obj, Key, NewItem);
  if (!Ok) {
    cJSON_Delete(NewItem);
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}
