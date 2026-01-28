/**
 * @file
 * 
 */

#include <Library/OKN/OknMemTestLib/OknMemTestLib.h>

EFI_STATUS JsonGetU64(IN CONST cJSON *Item, OUT UINT64 *Val)
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

EFI_STATUS JsonGetU32FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT32 *Val)
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

EFI_STATUS JsonGetU16FromObject(IN CONST cJSON *Obj, IN CONST CHAR8 *Key, OUT UINT16 *Val)
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

EFI_STATUS JsonGetU8FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT8 *Val)
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

EFI_STATUS JsonGetU16FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT16 *Val)
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

EFI_STATUS JsonGetU32FromArray(IN CONST cJSON *Arr, IN INTN Index, OUT UINT32 *Val)
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
