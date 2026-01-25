#ifndef __INTEL_PORTING_H__
#define __INTEL_PORTING_H__

#include <MemCommon.h>
#include <Library/SysHostPointerLib.h>
#include <Library/SysHostPointerLib/StaticPointerTableHelper.h>
#include <SysHost.h>
#include <Library/SystemInfoLib.h>
#include <Protocol/SystemInfoVarProtocol.h>
#include <Library/HbmCoreLib.h>
#include <MaxSocket.h>
#include <MaxCore.h>
#include <Library/CsrPseudoOffsetConvertLib.h>
#include <Protocol/CsrPseudoOffsetInfoProtocol.h>
#include <Protocol/SiliconRegAccess.h>
#include <Library/UsraAccessApi.h>
#include <Library/UsraCsrLib.h>
#include <Library/MemoryCoreLib.h>
#include <CpuPciAccess.h>
#include <UncoreCommonIncludes.h>
#include <Library/MemMcIpLib.h>
#include <Library/MemRcLib.h>
#include <Library/ProcMemInitChipLib.h>
#include <Library/CpuAndRevisionLib.h>
#include <Library/RcDebugLib.h>
#include <Library/MemoryServicesLib.h>
#include <Library/MemAccessLib.h>
#include <Library/SpdAccessLib.h>
#include <Protocol/MemoryAddressTranslation.h>      //OKN_20240822_yjb_EgsAddrTrans
#include <Guid/MemoryMapData.h>
#include <Library/MemMapDataLib.h>
#include <Mem/Library/MemMcIpLib/Include/MemMcRegs.h>
#include <Cpu/CpuCoreRegs.h>
#include <Library/MemMcIpRasLib.h>

extern MEMORY_ADDRESS_TRANSLATION_PROTOCOL *gAddrTrans;         //OKN_20240822_yjb_EgsAddrTrans

#endif