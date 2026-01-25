#include <Library/IntelPortingLib.h>

#include "IntelPorting.h"

//
// Global static pointer table.
// There is not actually much benefit from using the table, it could
// more simply be a set of pointers, but this helps verify that design.
//
STATIC STATIC_POINTER_TABLE  *mStaticPointerTable = NULL;

//
// This GUID is not intended for use by other code
//
STATIC GUID  mStaticPointerTableGuid          = STATIC_POINTER_TABLE_GUID;

/**
  This function verifies that we are compatible with other components.

  It verifies the GUID and size of the HOB that stores the table.
  It fails bluntly with a deadloop.

  @param  None

  @return None

**/
VOID
VerifyStaticPointerTable (
  VOID
  )
{
  EFI_HOB_GUID_TYPE       *StaticPointerTableHob;
  BOOLEAN                 DoesGuidMatch;

  //
  // Verify input
  //
  ASSERT (mStaticPointerTable != NULL);
  if (mStaticPointerTable == NULL) {
    CpuDeadLoop ();
  }

  //
  // Verify that it is a compatible version
  // This is done by verifying GUID and verifying the size of the HOB
  //
  StaticPointerTableHob = (EFI_HOB_GUID_TYPE *) (((UINT8 *) mStaticPointerTable) - sizeof (EFI_HOB_GUID_TYPE));

  ASSERT(StaticPointerTableHob != NULL);
  if (StaticPointerTableHob == NULL) {
    CpuDeadLoop();
  } else {
    //
    // No need to check the size as EFI_PEI_PPI_DESCRIPTOR is different from PEI to DXE
    // Only check GUID
    //
    DoesGuidMatch = CompareGuid (&StaticPointerTableHob->Name, &mStaticPointerTableGuid);

    ASSERT (DoesGuidMatch);
    if (!DoesGuidMatch) {
      CpuDeadLoop ();
    }
  }
}

/**
  This library constructor initializes the global variable instance
  of the static pointer table.

  @param      None

  @return     RETURN_SUCCESS              Library is ready for use

**/
RETURN_STATUS
EFIAPI
StaticPointerTableLibConstructor (
  VOID
  )
{
  VOID        *HobPtr;

  if (mStaticPointerTable == NULL) {
    //
    // Do not create another table if one has already been created
    //
    HobPtr = GetFirstGuidHob (&mStaticPointerTableGuid);
    ASSERT (HobPtr != NULL);
    if (HobPtr == NULL) {
      return EFI_NOT_FOUND;
    }

    //
    // Existing HOB found - in DXE phase, only SptEntries are used
    //
    mStaticPointerTable = GET_GUID_HOB_DATA (HobPtr);
    //
    // Verify compatibility with existing table
    // Since this is a static library, the table may be produced from other
    // components built with other versions of the HOB.
    //
    VerifyStaticPointerTable ();
  }

  return RETURN_SUCCESS;
}

/**
  Gets the SysHost pointer.

  This function abstracts the retrieval of the SysHost pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to SysHost

**/
VOID *
EFIAPI
GetSysHostPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptSysHost];
}

/**
  Sets the SysHost pointer.

  This function abstracts the setting of the SysHost pointer.
  We may have implementation specific way to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  SysHost                 Pointer to be stored

**/
VOID
EFIAPI
SetSysHostPointer (
  IN VOID *SysHost
  )
{
  ASSERT (FALSE);
  return;
}

/**

  This function retrieves the ActmData pointer
  This is designed to be very high performance

  @return Pointer to ActmData

**/
VOID *
EFIAPI
GetActmPointer (
  VOID
  )
{
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptActmData];
}

/**

  This function stores the ActmData pointer
  This is designed to be very high performance

  @param[in] ActmDataPtr    Pointer to ActmData

**/
VOID
EFIAPI
SetActmPointer (
  IN VOID *ActmDataPtr
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the HbmHost pointer.

  This function abstracts the retrieval of the HbmHost pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to HbmHost

**/
VOID *
EFIAPI
GetHbmHostPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptHbmHost];
}

/**
  Sets the HbmHost pointer.

  This function abstracts the setting of the HbmHost pointer.
  We may have implementation specific way to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  HbmHost                 Pointer to be stored

**/
VOID
EFIAPI
SetHbmHostPointer (
  IN VOID *HbmHost
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the CpgcHost pointer.

  This function abstracts the retrieval of the CpgcHost pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to CpgcHost

**/
VOID *
EFIAPI
GetCpgcHostPointer (
  VOID
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Sets the CpgcHost pointer.

  This function abstracts the setting of the CpgcHost pointer.
  We may have implementation specific way to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  CpgcHost                Pointer to be stored

**/
VOID
EFIAPI
SetCpgcHostPointer (
  IN VOID *CpgcHost
  )
{
  ASSERT (FALSE);
}

/**
  Gets the CpuAndRevision pointer.

  This function abstracts the retrieval of the CpuAndRevision pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to CpuAndRevision

**/
VOID *
EFIAPI
GetCpuAndRevisionPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptCpuAndRevision];
}

/**
  Sets the CpuAndRevision pointer.

  This function abstracts the setting of the CpuAndRevision pointer.
  We may have implementation specific way to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  CpuAndRevision          Pointer to be stored

**/
VOID
EFIAPI
SetCpuAndRevisionPointer (
  IN VOID *CpuAndRevision
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the SysInfoVar pointer.

  This function abstracts the retrieval of the SysInfoVar pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to SysInfoVar

**/
VOID *
EFIAPI
GetSysInfoVarPointer (
  VOID
  )
{
  return NULL;
}

/**
  Sets the SysInfoVar pointer.

  This function abstracts the setting of the SysInfoVar pointer.
  We may have implementation specific way to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  SysInfoVar              Pointer to be stored

**/
VOID
EFIAPI
SetSysInfoVarPointer (
  IN VOID *SysInfoVar
  )
{
  return;
}

/**
  Gets the SysSetup pointer.

  This function abstracts the retrieval of the SysSetup pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to SysSetup

**/
VOID *
EFIAPI
GetSysSetupPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptSysSetup];
}

/**
  Sets the SysSetup pointer.

  This function abstracts the setting of the SysSetup pointer.
  We may have implementation specific way to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  SysSetup                Pointer to be stored

**/
VOID
EFIAPI
SetSysSetupPointer (
  IN VOID *SysSetup
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the SemaphoreData pointer.

  This function abstracts the retrieval of the SemaphoreData pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to SemaphoreData

**/
VOID *
EFIAPI
GetSemaphoreDataPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptSemaphoreData];
}

/**
  Sets the SemaphoreData pointer.

  This function abstracts the setting of the SemaphoreData pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  SemaphoreData           Pointer to be stored

**/
VOID
EFIAPI
SetSemaphoreDataPointer (
  IN VOID *SemaphoreData
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the SpdData pointer.

  This function abstracts the retrieval of the SpdData pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to SpdData

**/
VOID *
EFIAPI
GetSpdDataPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptSpdData];
}

/**
  Sets the SpdData pointer.

  This function abstracts the setting of the SpdData pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  SpdData                 Pointer to be stored

**/
VOID
EFIAPI
SetSpdDataPointer (
  IN VOID *SpdData
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the DebugData pointer.

  This function abstracts the retrieval of the DebugData pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to DebugData

**/
VOID *
EFIAPI
GetDebugDataPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptDebugData];
}

/**
  Sets the DebugData pointer.

  This function abstracts the setting of the DebugData pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  DebugData               Pointer to be stored

**/
VOID
EFIAPI
SetDebugDataPointer (
  IN VOID *DebugData
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the MemMapHost pointer.

  This function abstracts the retrieval of the MemMapHost pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to MemMapHost

**/
VOID *
EFIAPI
GetMemMapHostPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptMemMapHost];
}

/**
  Sets the MemMapHost pointer.

  This function abstracts the setting of the MemMapHost pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  MemMapHost              Pointer to be stored

**/
VOID
EFIAPI
SetMemMapHostPointer (
  IN VOID *MemMapHost
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the EmulationSetting pointer.

  This function abstracts the retrieval of the EmulationSetting pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to EmulationSetting

**/
VOID *
EFIAPI
GetEmulationSettingPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptEmulationSetting];
}

/**
  Sets the EmulationSetting pointer.

  This function abstracts the setting of the EmulationSetting pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  EmulationSetting        Pointer to be stored

**/
VOID
EFIAPI
SetEmulationSettingPointer (
  IN VOID *EmulationSetting
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the MrcPerformanceData pointer.

  This function abstracts the retrieval of the MrcPerformanceData pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to MrcPerformanceData

**/
VOID *
EFIAPI
GetMrcPerformanceDataPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptMrcPerformanceData];
}

/**
  Sets the MrcPerformanceData pointer.

  This function abstracts the setting of the MrcPerformanceData pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  MrcPerformanceData      Pointer to be stored

**/
VOID
EFIAPI
SetMrcPerformanceDataPointer (
  IN VOID *MrcPerformanceData
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the DdrioData pointer.

  This function abstracts the retrieval of the DdrioData pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to DdrioData

**/
VOID *
EFIAPI
GetDdrioDataPointer (
  VOID
  )
{
  //
  // This code must be performance optimized, hence the difficulty in readability
  // and lack of error checking
  //
  return (VOID *) (UINTN) mStaticPointerTable->SptEntries[SptDdrioData];
}

/**
  Sets the DdrioData pointer.

  This function abstracts the setting of the DdrioData pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  DdrioData               Pointer to be stored

**/
VOID
EFIAPI
SetDdrioDataPointer (
  IN VOID *DdrioData
  )
{
  ASSERT (FALSE);
  return;
}

/**
  Gets the KtiHost pointer.

  This function abstracts the retrieval of the KtiHost pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to KtiHost

**/
VOID *
EFIAPI
GetKtiHostPointer (
  VOID
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Sets the KtiHost pointer.

  This function abstracts the setting of the KtiHost pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  KtiHost                 Pointer to be stored

**/
VOID
EFIAPI
SetKtiHostPointer (
  IN VOID *KtiHost
  )
{
  ASSERT (FALSE);
}

/**
  Gets the IpMteInstData pointer.

  This function abstracts the retrieval of the IpMteInstData pointer.
  There are implementation specific ways to get this pointer.
  This is designed to be very high performance for very high
  frequency operations.

  @return   Pointer to IpMteInstData

**/
VOID *
EFIAPI
GetIpMteInstDataPointer (
  VOID
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Sets the SptIpMteInstData pointer.

  This function abstracts the setting of the SptIpMteInstData pointer.
  We may have implementation specific ways to get this pointer.
  This is not expected to be a high performance function, so
  should not be called with very high frequency.

  @param[in]  IpMteInstData           Pointer to be stored

**/
VOID
EFIAPI
SetIpMteInstDataPointer (
  IN VOID *IpMteInstData
  )
{
  ASSERT (FALSE);
}

UINT8
EFIAPI
GetCurrentSocketId (
  VOID
  )
{
  PSYSHOST  Host;
  Host = GetSysHostPointer ();

  return Host->var.mem.currentSocket;
}


extern EFI_GUID  gDxeSystemInfoVarProtocol;
SYSTEM_INFO_VAR  *mSystemInfoVar;

/**
  This function returns the pointer to SYSTEM_INFO_VAR.

  @param VOID

  @retval SYSTEM_INFO_VAR*   The pointer to SYSTEM_INFO_VAR.
**/
SYSTEM_INFO_VAR *
GetSystemInfoVar (
  VOID
  )
{
  return mSystemInfoVar;
}

/**
  Enable the posted CSR access method
  Subsequent CSR accesses will be sent using Posted semantics.

  A call to this function must always be paired with a call
  to PostedMethodDisable to avoid unintentionally sending all
  future CSR accesses using posted semantics.

  @retval EFI_UNSUPPORTED if posted accesses are not supported
  @retval EFI_SUCCESS otherwise
**/
EFI_STATUS
EFIAPI
PostedMethodEnable (
  VOID
  )
{
  //
  // Posted accesses currently not enabled in DXE
  //
  return EFI_UNSUPPORTED;
}

/**
  Disable the posted CSR access method
  Subsequent CSR writes will be sent using non-posted semantics

  @retval EFI_UNSUPPORTED if posted accesses are not supported
  @retval EFI_SUCCESS otherwise
**/
EFI_STATUS
EFIAPI
PostedMethodDisable (
  VOID
  )
{
  //
  // Posted accesses currently not enabled in DXE
  //
  return EFI_UNSUPPORTED;
}

/**
  The constructor function initialize system info var.

  Standard EFI driver point.

  @param ImageHandle         -  A handle for the image that is initializing this driver.
  @param SystemTable         -  A pointer to the EFI system table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
DxeSystemInfoLibConstructor (
  VOID
  )
{
  EFI_STATUS                 Status;
  SYSTEM_INFO_VAR_PROTOCOL   *DxeSystemInfoVarProtocol;

  Status = gBS->LocateProtocol (
                    &gDxeSystemInfoVarProtocol,
                    NULL,
                    (VOID **) &DxeSystemInfoVarProtocol
                    );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  mSystemInfoVar =  (SYSTEM_INFO_VAR *) DxeSystemInfoVarProtocol;

  return EFI_SUCCESS;
}


/**
  This API gets Cpu Csr Access var info.

  @param  None

  @retval CPU_CSR_ACCESS_VAR*  The Pointer for system info returned.
**/
CPU_CSR_ACCESS_VAR *
GetSysCpuCsrAccessVar (
  VOID
  )
{
  return &(GetSystemInfoVar()->CpuCsrAccessVarHost);
}

/**
  This API gets reset required info.

  @param  None

  @retval UINT8  The data for reset required returned.
**/
UINT8
EFIAPI
GetSysResetRequired (
  VOID
  )
{
  return GetSystemInfoVar()->ResetRequired;
}

/**
  This API sets the value for reset required.

  @param[in] UINT8  Value to set for reset required.

  @retval None
**/
VOID
EFIAPI
SetSysResetRequired (
  IN UINT8  ResetRequired
  )
{
  GetSystemInfoVar()->ResetRequired = ResetRequired;
  return;
}

/**
  This API gets the value for emulation type.

  @param None

  @retval UINT8  The data for emulation type returned.
**/
UINT8
EFIAPI
GetEmulation (
  VOID
  )
{
  return GetSystemInfoVar()->Emulation;
}

/**
  This API sets the value for emulation type.

  @param[in] UINT8  Value to set for emulation type.

  @retval None
**/
VOID
EFIAPI
SetEmulation (
  IN UINT8  Emulation
  )
{
  GetSystemInfoVar()->Emulation = Emulation;
  return;
}

/**
  This API gets the value for Me requested size.

  @param None

  @retval UINT32  The data for Me requested size returned.
**/
UINT32
EFIAPI
GetMeRequestedSize (
  VOID
  )
{
  return GetSystemInfoVar()->MeRequestedSize;
}

/**
  This API sets the value for Me requested size.

  @param[in] UINT32  Value to set for Me requested size.

  @retval None
**/
VOID
EFIAPI
SetMeRequestedSize (
  IN UINT32  MeReqeustedSize
  )
{
  GetSystemInfoVar()->MeRequestedSize = MeReqeustedSize;
  return;
}

/**
  This API gets the value for Me requested alignment.

  @param None

  @retval UINT32  The data for Me requested alignment returned.
**/
UINT32
EFIAPI
GetMeRequestedAlignment (
  VOID
  )
{
  return GetSystemInfoVar()->MeRequestedAlignment;
}

/**
  This API sets the value for Me requested alignment.

  @param[in] UINT32  Value to set for Me requested alignment.

  @retval None
**/
VOID
EFIAPI
SetMeRequestedAlignment (
  IN UINT32  MeRequestedAlignment
  )
{
  GetSystemInfoVar()->MeRequestedAlignment = MeRequestedAlignment;
  return;
}

/**
  This API gets the value for check point.

  @param None

  @retval UINT32  The data for check point returned.
**/
UINT32
EFIAPI
GetCheckPoint (
  VOID
  )
{
  return GetSystemInfoVar()->CheckPoint;
}

/**
  This API sets the value for check point.

  @param[in] UINT32  Value to set for check point.

  @retval None
**/
VOID
EFIAPI
SetCheckPoint (
  IN UINT32  CheckPoint
  )
{
  GetSystemInfoVar()->CheckPoint = CheckPoint;
  return;
}

/**
  This API gets the value for boot mode.

  @param None

  @retval BootMode  The data for boot mode returned.
**/
BootMode
EFIAPI
GetSysBootMode (
  VOID
  )
{
  return GetSystemInfoVar()->SysBootMode;
}

/**
  This API sets the value for boot mode.

  @param[in] BootMode  Value to set for boot mode.

  @retval None
**/
VOID
EFIAPI
SetSysBootMode (
  IN BootMode  SysBootMode
  )
{
  GetSystemInfoVar()->SysBootMode = SysBootMode;
  return;
}

/**
  This API gets the value for cpu frequency.

  @param None

  @retval UINT64  The data for cpu frequency returned.
**/
UINT64
EFIAPI
GetCpuFreq (
  VOID
  )
{
  return GetSystemInfoVar()->CpuFreq;
}

/**
  This API sets the value for cpu frequency.

  @param[in] UINT64  Value to set for cpu frequency.

  @retval None
**/
VOID
EFIAPI
SetCpuFreq (
  IN UINT64  CpuFreq
  )
{
  GetSystemInfoVar()->CpuFreq = CpuFreq;
  return;
}

/**
  This API gets the value for cpu Socket Id.

  @param None

  @retval UINT8  The data for cpu Socket Id.
**/
UINT8
EFIAPI
GetSysSocketId (
  VOID
  )
{
  return GetSystemInfoVar()->SocketId;
}

/**
  This API sets the value for SocketId.

  @param[in] UINT8  Value to set for SocketId.

  @retval None
**/
VOID
EFIAPI
SetSysSocketId (
  IN UINT8  SocketId
  )
{
  GetSystemInfoVar()->SocketId = SocketId;
  return;
}

/**
  This API gets the pointer to SYS_INFO_VAR_NVRAM.

  @param None

  @retval SYS_INFO_VAR_NVRAM  The pointer to SYS_INFO_VAR_NVRAM.
**/
SYS_INFO_VAR_NVRAM *
EFIAPI
GetSysInfoVarNvramPtr (
  VOID
  )
{
  return &(GetSystemInfoVar ()->SysInfoVarNvram);
}

/**
  This API gets the value for MeRequestedSizeNv in SysInfoVarNvram.

  @param None

  @retval UINT32  The data for MeRequestedSizeNv in SysInfoVarNvram.
**/
UINT32
EFIAPI
GetSysMeRequestedSizeNv (
  VOID
  )
{
  return GetSystemInfoVar ()->SysInfoVarNvram.MeRequestedSizeNv;
}

/**
  This API sets the value for MeRequestedSizeNv in SysInfoVarNvram.

  @param[in] SYS_INFO_VAR_NVRAM  Value to set for MeRequestedSizeNv in SysInfoVarNvram.

  @retval None
**/
VOID
EFIAPI
SetSysMeRequestedSizeNv (
  IN UINT32  MeRequestedSizeNv
  )
{
  GetSystemInfoVar ()->SysInfoVarNvram.MeRequestedSizeNv = MeRequestedSizeNv;
  return;
}

/**
  This API gets the value for MeRequestedAlignmentNv in SysInfoVarNvram.

  @param None

  @retval UINT32  The data for MeRequestedAlignmentNv in SysInfoVarNvram.**/
UINT32
EFIAPI
GetSysMeRequestedAlignmentNv (
  VOID
  )
{
  return GetSystemInfoVar ()->SysInfoVarNvram.MeRequestedAlignmentNv;
}

/**
  This API sets the value for MeRequestedAlignmentNv in SysInfoVarNvram.

  @param[in] SYS_INFO_VAR_NVRAM  Value to set for MeRequestedAlignmentNv in SysInfoVarNvram.

  @retval None
**/
VOID
EFIAPI
SetSysMeRequestedAlignmentNv (
  IN UINT32  MeSysRequestedAlignmentNv
  )
{
  GetSystemInfoVar ()->SysInfoVarNvram.MeRequestedAlignmentNv = MeSysRequestedAlignmentNv;
  return;
}

/**
  This API gets the value for SbspSocketIdNv in SysInfoVarNvram.

  @param None

  @retval UINT8  The data for SbspSocketIdNv in SysInfoVarNvram.
**/
UINT8
EFIAPI
GetSysSbspSocketIdNv (
  VOID
  )
{
  return GetSystemInfoVar ()->SysInfoVarNvram.SbspSocketIdNv;
}

/**
  This API sets the value for SbspSocketIdNv in SysInfoVarNvram.

  @param[in] UINT8  Value to set for SbspSocketIdNv in SysInfoVarNvram.

  @retval None
**/
VOID
EFIAPI
SetSysSbspSocketIdNv (
  IN UINT8  SbspSocketIdNv
  )
{
  GetSystemInfoVar ()->SysInfoVarNvram.SbspSocketIdNv = SbspSocketIdNv;
  return;
}

/**

  Check emulation type
  @param[in] EmuType  - Emulation Type to be checked

  @retval    TRUE       Emulation Type is same as input
  @retval    FALSE      Emulation Type is not same as input

**/
BOOLEAN
EFIAPI
CheckEmulationType (
  IN UINT8     EmuType
  )
{
  return ((GetEmulation () & EmuType) != 0) ? TRUE : FALSE;
}

/**

  Set emulation type
  @param[in] EmuType  - Emulation Type to be set

**/
VOID
EFIAPI
SetEmulationType (
  IN UINT8     EmuType
  )
{
  SetEmulation (EmuType);
}

/**

  This API Starts USRA trace.

  @param None

  @retval None

**/
VOID
EFIAPI
UsraTraceStart (
  VOID
  )
{
  GetSystemInfoVar ()->UsraTraceControl = TRUE;
}

/**

  This API ends USRA trace.

  @param None

  @retval None

**/
VOID
EFIAPI
UsraTraceEnd (
  VOID
  )
{
  GetSystemInfoVar ()->UsraTraceControl = FALSE;
}

/**

  This API gets the configuration of USRA trace.

  @retval  - USRA trace configuration.

**/
UINT16
EFIAPI
GetUsraTraceConfiguration (
  VOID
  )
{
  return GetSystemInfoVar ()->UsraTraceConfiguration;
}

/**

  This API sets the configuration of USRA trace.

  @param[in] UsraTraceConfiguration  - USRA configuration to be set.

  @retval None

**/
VOID
EFIAPI
SetUsraTraceConfiguration (
  IN  UINT16     UsraTraceConfiguration
  )
{
  GetSystemInfoVar ()->UsraTraceConfiguration = UsraTraceConfiguration;
}

/**

  This API checks the status of USRA trace.

  @param None

  @retval True                    USRA Trace is working.
  @retval False                   USRA Trace is not working.

**/
BOOLEAN
EFIAPI
CheckUsraTraceContolFlag (
  VOID
  )
{
  return GetSystemInfoVar ()->UsraTraceControl;
}

/**

  This API will return whether the CpuCsrAccessVar info has been dumped.

  @param None

  @retval True                    CpuCsrAccessVar info has been dumped.
  @retval False                   CpuCsrAccessVar info has not been dumped.

**/
BOOLEAN
EFIAPI
CheckIfCpuCsrAccessVarInfoDumped (
  VOID
  )
{
  return GetSystemInfoVar ()->CpuCsrAccessVarInfoDumped;
}

/**

  This API disables the CpuCsrAccessVar dump info.

  @param  None

  @retval None

**/
VOID
EFIAPI
DisableCpuCsrAccessVarInfoDump (
  VOID
  )
{
  GetSystemInfoVar ()->CpuCsrAccessVarInfoDumped = TRUE;
}

/**
  This API gets the UPI connection node type for NodeId.

  @param[in] NodeId - Node ID

  @retval UINT8  The UPI connection node type for NodeId.
**/
UPI_CONNECTION_TYPE
EFIAPI
GetUpiConnectionType (
  IN UINT8  NodeId
  )
{
  CPU_CSR_ACCESS_VAR    *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  return CpuCsrAccessVar->UpiConnectionType[NodeId];
}

/**
  This API sets the UPI connection node type for NodeId.

  @param[in] NodeId            - Node ID
  @param[in] UpiConnectionType - UPI connection node type

  @retval None
**/
VOID
EFIAPI
SetUpiConnectionType (
  IN UINT8                NodeId,
  IN UPI_CONNECTION_TYPE  UpiConnectionType
  )
{
  CPU_CSR_ACCESS_VAR    *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  CpuCsrAccessVar->UpiConnectionType[NodeId] = UpiConnectionType;
}

/**
  Return the current state of the posted write CSR access method

  @param[out] CpuCsrAccessVarPtr - Pointer to CSR access variable
                                   If NULL, this function will locate the variable

  @retval POSTED_METHOD_ENABLED if posted writes are enabled
  @retval POSTED_METHOD_DISABLED otheriwse
**/
UINT8
EFIAPI
GetPostedMethodState (
  IN  CPU_CSR_ACCESS_VAR          *CpuCsrAccessVarPtr
  )
{
  if (CpuCsrAccessVarPtr == NULL) {
    CpuCsrAccessVarPtr = GetSysCpuCsrAccessVar ();
  }

  if (CpuCsrAccessVarPtr->PostedCsrAccessAllowed && CpuCsrAccessVarPtr->PostedWritesEnabled) {
    return POSTED_METHOD_ENABLED;
  } else {
    return POSTED_METHOD_DISABLED;
  }
}

/**
  This API gets PCIE segment number based on the socket number.

  @param  [in]  Socket             - Socket Id

  @retval UINT8  The PCIE Segment number returned.
**/
UINT8
EFIAPI
GetPcieSegment (
  UINT8 Socket
  )
{
  return GetSysCpuCsrAccessVar()->segmentSocket[Socket];
}

/**
  This API gets PCIE Socket number based on the segment and bus number.

  @param[in] Seg                       Device's Segment number.
  @param[in] Bus                       PCIe device's bus number.

  @param[out]  *Socket             Pointer Socket.

  @retval  EFI_SUCCESS              Socket number calculated successful.
  @retval  EFI_NOT_FOUND        Input parameter is invalid.
**/
EFI_STATUS
EFIAPI
GetPcieSocket (
  IN UINT8           Seg,
  IN UINT8           Bus,
  OUT UINT8        *Socket
  )
{
  UINT8                                           Soc;
  CPU_CSR_ACCESS_VAR                *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar();

  for (Soc = 0 ; Soc < MAX_SOCKET ; Soc++) {
    if ((Bus >= CpuCsrAccessVar->SocketFirstBus[Soc]) && (Bus <= CpuCsrAccessVar->SocketLastBus[Soc])) {
      if (CpuCsrAccessVar->segmentSocket[Soc] == Seg) {
        *Socket = Soc;
        return EFI_SUCCESS;
      }
    }
  }
  return EFI_NOT_FOUND;
}

/**
  Get number of channel per memory controller

  @return the number of channel per memory controller, but not higher than MAX_MC_CH

**/
UINT8
EFIAPI
GetNumChannelPerMc (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  ASSERT (CpuCsrAccessVar->DdrNumChPerMc > 0);
  if (CpuCsrAccessVar->DdrNumChPerMc > MAX_MC_CH) {
    ASSERT (FALSE);
    return MAX_MC_CH;
  }

  return CpuCsrAccessVar->DdrNumChPerMc;
}

/**
  Get maximum number of IMC

  @return the maximum number of iMC, but not higher than MAX_IMC

**/
UINT8
EFIAPI
GetMaxImc (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  ASSERT (CpuCsrAccessVar->DdrMaxImc > 0);
  if (CpuCsrAccessVar->DdrMaxImc > MAX_IMC) {
    ASSERT (FALSE);
    return MAX_IMC;
  }

  return CpuCsrAccessVar->DdrMaxImc;
}

/**
  Get maximum DDR channel number of socket

  @return the maximum DDR number of socket, but not higher than MAX_CH

**/
UINT8
EFIAPI
GetMaxChDdr (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  ASSERT (CpuCsrAccessVar->DdrMaxCh > 0);
  if (CpuCsrAccessVar->DdrMaxCh > MAX_CH) {
    ASSERT (FALSE);
    return MAX_CH;
  }

  return CpuCsrAccessVar->DdrMaxCh;
}

/**
  Get number of channel per memory controller

  @return the number of channel per memory controller, but not higher than MAX_HBM_MC_CH

**/
UINT8
EFIAPI
GetNumChannelPerMcHbm (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  ASSERT (CpuCsrAccessVar->HbmNumChPerMc > 0);
  if (CpuCsrAccessVar->HbmNumChPerMc > MAX_HBM_MC_CH) {
    ASSERT (FALSE);
    return MAX_HBM_MC_CH;
  }

  return CpuCsrAccessVar->HbmNumChPerMc;
}

/**
  Get maximum number of HBM channel per HBM IO

  @retval maximum number of HBM channel per HBM IO, but not higher than MAX_HBM_IO_CH

**/
UINT8
EFIAPI
GetNumChannelPerIoHbm (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  ASSERT (CpuCsrAccessVar->HbmNumChPerIo > 0);
  if (CpuCsrAccessVar->HbmNumChPerIo > MAX_HBM_IO_CH) {
    ASSERT (FALSE);
    return MAX_HBM_IO_CH;
  }

  return CpuCsrAccessVar->HbmNumChPerIo;
}

/**
  Get maximum number of IMC per HBM IO

  @return the maximum number of iMC

**/
UINT8
EFIAPI
GetMaxImcPerIoHbm (
  VOID
  )
{
  return (UINT8) (GetNumChannelPerIoHbm () / GetNumChannelPerMcHbm ());
}

/**
  Get maximum channel number of socket

  @return the maximum number of socket, but not higher than MAX_HBM_CH

**/
UINT8
EFIAPI
GetMaxChHbm (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  ASSERT (CpuCsrAccessVar->HbmMaxCh > 0);
  if (CpuCsrAccessVar->HbmMaxCh > MAX_HBM_CH) {
    ASSERT (FALSE);
    return MAX_HBM_CH;
  }

  return CpuCsrAccessVar->HbmMaxCh;
}

/**
  Get maximum hbm io instance number of socket

  @return the maximum hbm io instance number of socket, but not higher than MAX_HBM_IO

**/
UINT8
EFIAPI
GetMaxIoHbm (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  if (CpuCsrAccessVar->HbmSku) {
    ASSERT (CpuCsrAccessVar->HbmMaxIoInst > 0);
    if (CpuCsrAccessVar->HbmMaxIoInst > MAX_HBM_IO) {
      ASSERT (FALSE);
      return MAX_HBM_IO;
    } else {
      return CpuCsrAccessVar->HbmMaxIoInst;
    }
  } else {
    return 0;
  }
}

/**

  Indicate if HVM Mode is enabled


  @retval   TRUE      HVM Mode is enabled
  @retval   FALSE     HVM Mode is disabled

**/
BOOLEAN
EFIAPI
IsHvmModeEnable (
  VOID
  )
{
  return PcdGetBool (PcdHvmModeEnabled);
} // IsHvmModeEnable

/**
  Get the processor common information

  @param[in] Socket - Socket of the processor to get info for

  @retval - Ptr to the info

**/

PROCESSOR_COMMON_INFO *
EFIAPI
GetProcessorCommonInfo (
  IN UINT8 Socket
  )
{
  return &(GetSystemInfoVar ()->ProcessorCommonInfo[Socket]);
}

/**
  Get maximum number of IMC

  @return the maximum number of iMC, but not higher than MAX_IMC

**/
UINT8
EFIAPI
GetDdrPhysicalMaxImc (
  VOID
  )
{
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  ASSERT (CpuCsrAccessVar->DdrPhysicalMaxImc > 0);
  if (CpuCsrAccessVar->DdrPhysicalMaxImc > MAX_IMC) {
    ASSERT (FALSE);
    return MAX_IMC;
  }

  return CpuCsrAccessVar->DdrPhysicalMaxImc;
}


extern EFI_GUID gDxePseudoOffestInfoProtocol;
PSEUDO_OFFSET_TABLE_HEADER          *mDxeTableHeader;
/**
  Identifies the real hardware register for given BoxType, functional Block and pseudo table offset.

  @param[in] RegOffset: Bits <31:24>  Bits <23:20>  Bit 19  Bits <18:16>  Bits <15:0>
                         Box Number   Func Number   Pseudo  Size          pseudo table Offset

  @retval Bits <31:24>  Bits <23:20>  Bit 19  Bits <18:16>  Bits <15:0>
  @retval Box Number    Func Number   0       Size          PCICfg Offset

**/

UINT32
EFIAPI
GetPseudoRegisterOffset (
  IN  CSR_OFFSET  RegOffset
  )
{
#if (TOTAL_PSEUDO_OFFSET_TABLES == 0)
  ASSERT (FALSE);
  return RegOffset.Data;
#else
  UINT32      PseudoOffset;
  UINT8       BoxType;
  UINT8       FuncBlk;
  UINT32      Reg;

  PseudoOffset = RegOffset.Bits.offset;
  BoxType      = (UINT8)RegOffset.Bits.boxtype;
  FuncBlk      = (UINT8)RegOffset.Bits.funcblk;
  Reg          = *((UINT32*)((UINTN) mDxeTableHeader + (UINTN) mDxeTableHeader->SubTableLocation[BoxType][FuncBlk] + (UINTN) PseudoOffset * sizeof(UINT32)));

  return Reg;
#endif // #if (TOTAL_PSEUDO_OFFSET_TABLES == 0)
}

/**
  The constructor function initialize Pseudo offset table.

  Standard EFI driver point.

  @param ImageHandle         -  A handle for the image that is initializing this driver.
  @param SystemTable         -  A pointer to the EFI system table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
DxeCsrPseudoOffsetConvertLibConstructor (
  VOID
  )
{
#if (TOTAL_PSEUDO_OFFSET_TABLES == 0)
  return EFI_SUCCESS;
#else
  EFI_STATUS                       Status;
  CSR_PSEUDO_OFFEST_INFO_PROTOCOL  *DxePseudoOffestInfo;

  Status = gBS->LocateProtocol (
                    &gDxePseudoOffestInfoProtocol,
                    NULL,
                    (VOID **) &DxePseudoOffestInfo
                    );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  mDxeTableHeader  =  (PSEUDO_OFFSET_TABLE_HEADER*)DxePseudoOffestInfo;

  return EFI_SUCCESS;
#endif // #if (TOTAL_PSEUDO_OFFSET_TABLES == 0)
}

static
UINT64
InternalGetTscFrequency (
  VOID
  )
{
  MSR_PLATFORM_INFO_REGISTER     Reg;

  //
  // Read PLATFORM_INFO MSR[15:8] to get core:bus ratio (max non-turbo ratio)
  //
  Reg.Uint64 = AsmReadMsr64 (MSR_PLATFORM_INFO);

  return MultU64x32 (Reg.Bits.MaxNonTurboLimRatio, 100000000);
}

static
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }
  if (EndValue != NULL) {
    *EndValue = 0xFFFFFFFFFFFFFFFFull;
  }

  return InternalGetTscFrequency ();
}

static
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN) Shift);
  Frequency = RShiftU64 (Frequency, (UINTN) Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}

USRA_PROTOCOL     *mUsra = NULL;

/**
  Retrieves the USRA protocol from the handle database.

  @param - None

  @retval - Pointer to the USRA_PROTOCOL

**/

USRA_PROTOCOL *
EFIAPI
GetUsraProtocol (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // USRA protocol need to be installed before the module access USRA.
  //
  Status = gBS->LocateProtocol (&gDxeUsraProtocolGuid, NULL, (VOID **)&mUsra);
  ASSERT_EFI_ERROR (Status);
  if (Status != EFI_SUCCESS) {
    return NULL;
  }
  ASSERT (mUsra != NULL);
  if (mUsra == NULL) {
    return NULL;
  }

  return mUsra;
}


RETURN_STATUS
EFIAPI
RegisterRead (
  IN USRA_ADDRESS             *Address,
  OUT VOID                     *Buffer
  )
{
  if (mUsra == NULL) {
    mUsra = GetUsraProtocol ();
    if (mUsra == NULL) {
      ASSERT (FALSE);
      return RETURN_DEVICE_ERROR;
    }
  }

  return mUsra->RegRead (Address, Buffer);
}


/**
  This API Perform 8-bit, 16-bit, 32-bit or 64-bit silicon register write operations.
  It transfers data from a naturally aligned data buffer into a silicon register.

  @param[in] Address              A pointer of the address of the USRA Address Structure to be written
  @param[in] Buffer               A pointer of buffer for the value write to the register

  @retval RETURN_SUCCESS          The function completed successfully.
  @retval Others                  Return Error
**/
RETURN_STATUS
EFIAPI
RegisterWrite (
  IN USRA_ADDRESS             *Address,
  IN VOID                     *Buffer
  )
{
  if (mUsra == NULL) {
    mUsra = GetUsraProtocol ();
    if (mUsra == NULL) {
      ASSERT (FALSE);
      return RETURN_DEVICE_ERROR;
    }
  }

  return mUsra->RegWrite (Address, Buffer);
}

UINT32
EFIAPI
ReadCpuCsr (
  IN UINT8    SocId,
  IN UINT8    BoxInst,
  IN UINT32   CsrOffset
  )
{
  USRA_ADDRESS           RegisterId;
  UINT32                 Data32;

  USRA_CSR_OFFSET_ADDRESS (RegisterId, SocId, BoxInst, CsrOffset);

  RegisterRead (&RegisterId, &Data32);
  return Data32;
}

VOID
EFIAPI
WriteCpuCsr (
  IN UINT8    SocId,
  IN UINT8    BoxInst,
  IN UINT32   CsrOffset,
  IN UINT32   Data32
  )
{
  USRA_ADDRESS          RegisterId;

  USRA_CSR_OFFSET_ADDRESS (RegisterId, SocId, BoxInst, CsrOffset);

  RegisterWrite (&RegisterId, &Data32);
}

/**
  This API Perform 8-bit, 16-bit, 32-bit or 64-bit silicon register AND then OR operations. It read data from a
  register, And it with the AndBuffer, then Or it with the OrBuffer, and write the result back to the register

  @param[in] Address              A pointer of the address of the silicon register to be written
  @param[in] AndBuffer            A pointer of buffer for the value used for AND operation
                                  A NULL pointer means no AND operation. RegisterModify() equivalents to RegisterOr()
  @param[in] OrBuffer             A pointer of buffer for the value used for OR operation
                                  A NULL pointer means no OR operation. RegisterModify() equivalents to RegisterAnd()

  @retval RETURN_SUCCESS          The function completed successfully.
  @retval Others                  Return Error
**/
RETURN_STATUS
EFIAPI
RegisterModify (
  IN USRA_ADDRESS             *Address,
  IN VOID                     *AndBuffer,
  IN VOID                     *OrBuffer
  )
{
  if (mUsra == NULL) {
    mUsra = GetUsraProtocol ();
    if (mUsra == NULL) {
      ASSERT (FALSE);
      return RETURN_DEVICE_ERROR;
    }
  }
  return mUsra->RegModify (Address, AndBuffer, OrBuffer);
}

/**
  This API get the flat address from the given USRA Address.

  @param[in] Address              A pointer of the address of the USRA Address Structure to be read out

  @retval                         The flat address
**/
UINTN
EFIAPI
GetRegisterAddress (
  IN USRA_ADDRESS           *Address
  )
{
  if (mUsra == NULL) {
    mUsra = GetUsraProtocol ();
    if (mUsra == NULL) {
      ASSERT (FALSE);
      return RETURN_DEVICE_ERROR;
    }
  }

  return mUsra->GetRegAddr (Address);
}

UINT32
EFIAPI
UsraCsrRead (
  IN  UINT8              SocketId,
  IN  UINT8              BoxInst,
  IN  UINT32             CsrOffset
  )
{
  USRA_ADDRESS           RegisterId;
  UINT32                 Data32;

  USRA_CSR_OFFSET_ADDRESS (RegisterId, SocketId, BoxInst, CsrOffset);

  RegisterRead (&RegisterId, &Data32);
  return Data32;
}

/**
  This API performs silicon CSR register write operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] BoxInst            Box Instance, 0 based
  @param[in] CsrOffset          Register offset
  @param[in] Data32             The data needs to be written to CSR.

**/
VOID
EFIAPI
UsraCsrWrite (
  IN  UINT8              SocketId,
  IN  UINT8              BoxInst,
  IN  UINT32             CsrOffset,
  IN  OUT UINT32         Data32
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_OFFSET_ADDRESS (RegisterId, SocketId, BoxInst, CsrOffset);

  RegisterWrite (&RegisterId, &Data32);
}

/**
  This API performs silicon CSR register write operations and also write to
  script to support S3.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] BoxInst            Box Instance, 0 based
  @param[in] CsrOffset          Register offset
  @param[in] Data32             The data needs to be written to CSR.

**/
VOID
EFIAPI
S3UsraCsrWrite (
  IN  UINT8              SocketId,
  IN  UINT8              BoxInst,
  IN  UINT32             CsrOffset,
  IN  UINT32             Data32
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_OFFSET_ADDRESS (RegisterId, SocketId, BoxInst, CsrOffset);

  RegisterId.Attribute.S3Enable = USRA_ENABLE;
  RegisterWrite (&RegisterId, &Data32);
}

/**
  This API performs silicon CSR register modify operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] BoxInst            Box Instance, 0 based
  @param[in] CsrOffset          Register offset
  @param[in] AndBuffer          A pointer of buffer for the value used for AND operation
                                A NULL pointer means no AND operation. RegisterModify() equivalents to RegisterOr()
  @param[in] OrBuffer           A pointer of buffer for the value used for OR operation
                                A NULL pointer means no OR operation. RegisterModify() equivalents to RegisterAnd()

  @retval RETURN_SUCCESS        The function completed successfully.
  @retval Others                Return Error
**/
RETURN_STATUS
EFIAPI
UsraCsrModify (
  IN  UINT8              SocketId,
  IN  UINT8              BoxInst,
  IN  UINT32             CsrOffset,
  IN  VOID               *AndBuffer,
  IN  VOID               *OrBuffer
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_OFFSET_ADDRESS (RegisterId, SocketId, BoxInst, CsrOffset);

  return RegisterModify(&RegisterId, AndBuffer, OrBuffer);
}

/**
  This API gets the flat address .

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] BoxInst            Box Instance, 0 based
  @param[in] CsrOffset          Register offset

  @retval                       The flat address
**/
UINTN
EFIAPI
UsraGetCsrRegisterAddress (
  IN  UINT8    SocketId,
  IN  UINT8    BoxInst,
  IN  UINT32   CsrOffset
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_OFFSET_ADDRESS (RegisterId, SocketId, BoxInst, CsrOffset);

  return GetRegisterAddress (&RegisterId);
}

/**
  This API performs silicon CSR register read operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] MemBarId           BAR ID
  @param[in] Offset             Register offset
  @param[in] AccessWidth        The Access width for 8, 16, 32, 64 -bit access. See USRA_ACCESS_WIDTH enum.

  @retval UINT64                The data from CSR register returned.
**/
UINT64
EFIAPI
UsraCsrMemRead (
  IN  UINT8              SocketId,
  IN  UINT8              MemBarId,
  IN  UINT32             Offset,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;
  UINT64                 Data64;
  UINT32                 DataUpper = 0;
  UINT32                 DataLower = 0;
  UINT8                  AdjustedWidth = AccessWidth;

  if (AccessWidth == UsraWidth64) {
    USRA_CSR_MEM_ADDRESS (RegisterId, SocketId, MemBarId, Offset, UsraWidth32);
    ((USRA_ADDRESS *)(&RegisterId))->CsrMem.High64Split = 1;
    RegisterRead (&RegisterId, &DataUpper);
    AdjustedWidth = UsraWidth32;
  }


  USRA_CSR_MEM_ADDRESS (RegisterId, SocketId, MemBarId, Offset, AdjustedWidth);
  RegisterRead (&RegisterId, &DataLower);

  Data64 = LShiftU64 ((UINT64) DataUpper, 32) | DataLower;

  return Data64;
}

/**
  This API performs silicon CSR register write operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] MemBarId           BAR ID
  @param[in] Offset             Register offset
  @param[in] Buffer             The data needs to be written to CSR.
  @param[in] AccessWidth        The Access width for 8, 16, 32, 64 -bit access. See USRA_ACCESS_WIDTH enum.

**/
VOID
EFIAPI
UsraCsrMemWrite (
  IN  UINT8              SocketId,
  IN  UINT8              MemBarId,
  IN  UINT32             Offset,
  IN  VOID               *Buffer,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;
  UINT32                 DataUpper = 0;
  UINT32                 DataLower = 0;
  VOID                   *SwapBuffer = Buffer;
  UINT8                  AdjustedWidth = AccessWidth;

  if (AccessWidth == UsraWidth64) {
    USRA_CSR_MEM_ADDRESS (RegisterId, SocketId, MemBarId, Offset, UsraWidth32);
    ((USRA_ADDRESS *)(&RegisterId))->CsrMem.High64Split = 1;
    DataUpper = RShiftU64 (*(UINT64 *) Buffer, 32) & 0xFFFFFFFF;
    DataLower = (*(UINT64 *) Buffer) & 0xFFFFFFFF;
    RegisterWrite (&RegisterId, &DataUpper);
    SwapBuffer = &DataLower;
    AdjustedWidth = UsraWidth32;
  }

  USRA_CSR_MEM_ADDRESS (RegisterId, SocketId, MemBarId, Offset, AdjustedWidth);
  RegisterWrite (&RegisterId, SwapBuffer);
}

/**
  This API performs silicon CSR register write operations and also write to
  script to support S3.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] MemBarId           BAR ID
  @param[in] Offset             Register offset
  @param[in] Buffer             The data needs to be written to CSR.
  @param[in] AccessWidth        The Access width for 8, 16, 32, 64 -bit access. See USRA_ACCESS_WIDTH enum.

**/
VOID
EFIAPI
S3UsraCsrMemWrite (
  IN  UINT8              SocketId,
  IN  UINT8              MemBarId,
  IN  UINT32             Offset,
  IN  VOID               *Buffer,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_MEM_ADDRESS (RegisterId, SocketId, MemBarId, Offset, AccessWidth);

  RegisterId.Attribute.S3Enable = USRA_ENABLE;
  RegisterWrite (&RegisterId, Buffer);
}

/**
  This API performs silicon CSR register modify operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] MemBarId           BAR ID
  @param[in] Offset             Register offset
  @param[in] AndBuffer          A pointer of buffer for the value used for AND operation
                                A NULL pointer means no AND operation. RegisterModify() equivalents to RegisterOr()
  @param[in] OrBuffer           A pointer of buffer for the value used for OR operation
                                A NULL pointer means no OR operation. RegisterModify() equivalents to RegisterAnd()
  @param[in] AccessWidth        The Access width for 8, 16, 32, 64 -bit access. See USRA_ACCESS_WIDTH enum.

  @retval RETURN_SUCCESS        The function completed successfully.
  @retval Others                Return Error
**/
RETURN_STATUS
EFIAPI
UsraCsrMemModify (
  IN  UINT8              SocketId,
  IN  UINT8              MemBarId,
  IN  UINT32             Offset,
  IN  VOID               *AndBuffer,
  IN  VOID               *OrBuffer,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_MEM_ADDRESS (RegisterId, SocketId, MemBarId, Offset, AccessWidth);

  return RegisterModify (&RegisterId, AndBuffer, OrBuffer);
}

/**
  This API gets the flat address .

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] MemBarId           BAR ID
  @param[in] Offset             Register offset

  @retval                       The flat address
**/
UINTN
EFIAPI
UsraGetCsrMemRegisterAddress (
  IN  UINT8               SocketId,
  IN  UINT8               MemBarId,
  IN  UINT32              Offset
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_MEM_ADDRESS (RegisterId, SocketId, MemBarId, Offset, UsraWidthMaximum);

  return GetRegisterAddress (&RegisterId);
}

/**
  This API performs silicon CSR register read operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] Bus                Bus
  @param[in] Device             Device
  @param[in] Function           Function
  @param[in] Offset             Register offset
  @param[in] AccessWidth        The Access width for 8, 16, 32 -bit access. See USRA_ACCESS_WIDTH enum.

  @retval UINT64                The data from CSR register returned.
**/
UINT64
EFIAPI
UsraCsrCfgRead (
  IN  UINT8              SocketId,
  IN  UINT8              Bus,
  IN  UINT8              Device,
  IN  UINT8              Function,
  IN  UINT32             Offset,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;
  UINT64                 Data64;
  UINT32                 DataUpper = 0;
  UINT32                 DataLower = 0;
  UINT8                  AdjustedWidth = AccessWidth;

  if (AccessWidth == UsraWidth64) {
    USRA_CSR_CFG_ADDRESS (RegisterId, SocketId, Bus, Device, Function, Offset, UsraWidth32);
    ((USRA_ADDRESS *)(&RegisterId))->CsrCfg.High64Split = 1;
    RegisterRead (&RegisterId, &DataUpper);
    AdjustedWidth = UsraWidth32;
  }

  USRA_CSR_CFG_ADDRESS (RegisterId, SocketId, Bus, Device, Function, Offset, AdjustedWidth);
  RegisterRead (&RegisterId, &DataLower);

  Data64 = LShiftU64 ((UINT64) DataUpper, 32) | DataLower;

  return Data64;
}

/**
  This API performs silicon CSR register write operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] Bus                Bus
  @param[in] Device             Device
  @param[in] Function           Function
  @param[in] Offset             Register offset
  @param[in] Buffer             The data needs to be written to CSR.
  @param[in] AccessWidth        The Access width for 8, 16, 32 -bit access. See USRA_ACCESS_WIDTH enum.

**/
VOID
EFIAPI
UsraCsrCfgWrite (
  IN  UINT8              SocketId,
  IN  UINT8              Bus,
  IN  UINT8              Device,
  IN  UINT8              Function,
  IN  UINT32             Offset,
  IN  VOID               *Buffer,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;
  UINT32                 DataUpper = 0;
  UINT32                 DataLower = 0;
  VOID                   *SwapBuffer = Buffer;
  UINT8                  AdjustedWidth = AccessWidth;

  if (AccessWidth == UsraWidth64) {
    USRA_CSR_CFG_ADDRESS (RegisterId, SocketId, Bus, Device, Function, Offset, UsraWidth32);
    ((USRA_ADDRESS *)(&RegisterId))->CsrCfg.High64Split = 1;
    DataUpper = RShiftU64 (*(UINT64 *) Buffer, 32) & 0xFFFFFFFF;
    DataLower = (*(UINT64 *) Buffer) & 0xFFFFFFFF;
    RegisterWrite (&RegisterId, &DataUpper);
    SwapBuffer = &DataLower;
    AdjustedWidth = UsraWidth32;
  }

  USRA_CSR_CFG_ADDRESS (RegisterId, SocketId, Bus, Device, Function, Offset, AdjustedWidth);
  RegisterWrite (&RegisterId, SwapBuffer);
}

/**
  This API performs silicon CSR register write operations and also write to
  script to support S3.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] Bus                Bus
  @param[in] Device             Device
  @param[in] Function           Function
  @param[in] Offset             Register offset
  @param[in] Buffer             The data needs to be written to CSR.
  @param[in] AccessWidth        The Access width for 8, 16, 32 -bit access. See USRA_ACCESS_WIDTH enum.

**/
VOID
EFIAPI
S3UsraCsrCfgWrite (
  IN  UINT8              SocketId,
  IN  UINT8              Bus,
  IN  UINT8              Device,
  IN  UINT8              Function,
  IN  UINT32             Offset,
  IN  VOID               *Buffer,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_CFG_ADDRESS (RegisterId, SocketId, Bus, Device, Function, Offset, AccessWidth);

  RegisterId.Attribute.S3Enable = USRA_ENABLE;
  RegisterWrite (&RegisterId, Buffer);
}

/**
  This API performs silicon CSR register modify operations.

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] Bus                Bus
  @param[in] Device             Device
  @param[in] Function           Function
  @param[in] Offset             Register offset
  @param[in] AndBuffer          A pointer of buffer for the value used for AND operation
                                A NULL pointer means no AND operation. RegisterModify() equivalents to RegisterOr()
  @param[in] OrBuffer           A pointer of buffer for the value used for OR operation
                                A NULL pointer means no OR operation. RegisterModify() equivalents to RegisterAnd()
  @param[in] AccessWidth        The Access width for 8, 16, 32, 64 -bit access. See USRA_ACCESS_WIDTH enum.

  @retval RETURN_SUCCESS        The function completed successfully.
  @retval Others                Return Error
**/
RETURN_STATUS
EFIAPI
UsraCsrCfgModify (
  IN  UINT8              SocketId,
  IN  UINT8              Bus,
  IN  UINT8              Device,
  IN  UINT8              Function,
  IN  UINT32             Offset,
  IN  VOID               *AndBuffer,
  IN  VOID               *OrBuffer,
  IN  UINT8              AccessWidth
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_CFG_ADDRESS (RegisterId, SocketId, Bus, Device, Function, Offset, AccessWidth);

  return RegisterModify (&RegisterId, AndBuffer, OrBuffer);
}

/**
  This API gets the flat address .

  @param[in] SocketId           CPU Socket Node number (Socket ID)
  @param[in] Bus                Bus
  @param[in] Device             Device
  @param[in] Function           Function
  @param[in] Offset             Register offset

  @retval                       The flat address
**/
UINTN
EFIAPI
UsraGetCsrCfgRegisterAddress (
  IN  UINT8              SocketId,
  IN  UINT8              Bus,
  IN  UINT8              Device,
  IN  UINT8              Function,
  IN  UINT32             Offset
  )
{
  USRA_ADDRESS           RegisterId;

  USRA_CSR_CFG_ADDRESS (RegisterId, SocketId, Bus, Device, Function, Offset, UsraWidthMaximum);

  return GetRegisterAddress (&RegisterId);
}


/**

  Converts NodeId, ChannelId to instance of type BoxType/FuncBlk based on
  Cpu type and cpu sub type

  @param NodeId    - Memory controller index
  @param BoxType   - Box Type; values come from CpuPciAccess.h
  @param ChannelId - DDR channel Id within a Memory controller, 0 based, 0xFF if not used
  @param FuncBlk   - Functional Block; values come from CpuPciAccess.h

  @retval Box Instance

**/
STATIC
UINT8
MemGetBoxInst (
  UINT8    McId,
  UINT8    BoxType,
  UINT8    ChannelId,
  UINT8    FuncBlk,
  UINT8    MaxCh,
  UINT8    NumChPerMC
  )
{
  UINT8 BoxInst = 0xFF;

  switch (BoxType) {
  case BOX_MC:
    if ((FuncBlk > 1) && (FuncBlk != BOX_FUNC_MC_UNSPTD)) {
      BoxInst = McId * MAX_MC_CH;
    } else {
      BoxInst = ChannelId;
    }
    break;
  case BOX_MCCPGC:
  case BOX_MCCADB:
  case BOX_HBM:
  case BOX_HBM2E_MC:
  case BOX_MCDDC:
  case BOX_MCCMI:
    if (ChannelId < MaxCh) {
      BoxInst = ChannelId;             // One instance per DDR Channel
    } else {
      CpuCsrAccessError ("Wrong Channel ID parameter: 0x%08x passed with BOX_MCDDC\n", (UINT32) ChannelId);
    }
    break;

  case BOX_MCIO:
    switch (FuncBlk) {
    case BOX_FUNC_MCIO_DDRIO:
    case BOX_FUNC_MCIO_DDRIOEXT:
    case BOX_FUNC_MCIO_DDRIOMC:
    case BOX_FUNC_MCIO_DDRIOBC:
    case BOX_FUNC_MCIO_DDRIO_UNSPTD:
      BoxInst = ChannelId;                                        // one instance per DDR Channel
      break;
    default:
      CpuCsrAccessError ("Invalid FuncBlk: 0x%08x passed with BOX_MCIO\n", (UINT32) FuncBlk);
    } // funcblk
    break;

  default:
    CpuCsrAccessError ("Invalid BoxType: 0x%08x passed\n", (UINT32) BoxType);
  }
  return BoxInst;
}

/**

  Returns regBase with true offset (after processing pseudo offset, if needed)

  @param[in] NodeId        - Memory controller index
  @param[in] ChIdOrBoxInst - DDR channel Id within a memory controller
                             or Box Instance (in case of non-MC boxes), 0 based, 0xFF if not used
  @param[in] regBase       - Register offset; values come from the auto generated header file
                       - may be a pseudo offset

  @retval Updated Register OFfset

**/
UINT32
MemPciCfgOffset (
  IN  UINT8    NodeId,
  IN  UINT8    ChIdOrBoxInst,
  IN  UINT32   regBase
  )
{
  CSR_OFFSET RegOffset;
  CSR_OFFSET TempReg;

  RegOffset.Data = regBase;

  //
  // Adjust offset if pseudo
  //
  if (RegOffset.Bits.pseudo) {
    TempReg.Data = GetPseudoRegisterOffset (RegOffset);
    RegOffset.Bits.offset = TempReg.Bits.offset;
    RegOffset.Bits.size = TempReg.Bits.size;
    //
    // Clear offset bit
    //
    RegOffset.Bits.pseudo = 0;
  }

  //
  // Return updated reg offset
  //
  return RegOffset.Data;
}

/**

  Reads CPU Memory Controller configuration space using the MMIO mechanism

  @param[in] socket        - Socket number
  @param[in] ChIdOrBoxInst - DDR channel Id within a memory controller
                             or Box Instance (in case of non-MC boxes), 0 based, 0xFF if not used
  @param[in] Offset        - Register offset; values come from the auto generated header file

  @retval Register value

**/
UINT32
MemReadPciCfgEp (
  IN  UINT8       socket,
  IN  UINT8       ChIdOrBoxInst,
  IN  UINT32      Offset
  )
{
  UINT8 BoxInst;
  UINT8 InstancesPerCh;
  UINT8 SocId;
  UINT8 McId;
  UINT8 BoxType;
  UINT8 FuncBlk;
  UINT8 MemSsType;
  CSR_OFFSET RegOffset;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  McId = 0;
  SocId = socket;
  MemSsType = CpuCsrAccessVar->MemSsType[SocId];

  RegOffset.Data = Offset;
  BoxType = (UINT8)RegOffset.Bits.boxtype;
  FuncBlk = (UINT8)RegOffset.Bits.funcblk;
  if ( ((BoxType == BOX_MC) && (MemSsType != Hbm2MemSs))  ||
       ((BoxType == BOX_MCDDC) && (MemSsType != Hbm2MemSs)) ||
       (BoxType == BOX_MCIO) ||
       (BoxType == BOX_MCCPGC) || (BoxType == BOX_MCCMI))
  {

    if ((FuncBlk != BOX_FUNC_MCIO_DDRIO_UNSPTD) && (FuncBlk != BOX_FUNC_MC_UNSPTD)) {
      if ((ChIdOrBoxInst >= CpuCsrAccessVar->DdrMaxCh) || (ChIdOrBoxInst >= MAX_CH)) {
        CpuCsrAccessError ("Assert on access to ch >= DdrMaxCh\n");
        return MRC_STATUS_FAILURE;
      }

      McId = CpuCsrAccessVar->mcId[socket][ChIdOrBoxInst];
    } else {
      //
      // McId is calclated later during physical address translation for the MCIO_DDRIO_UNSPTD
      // and MC_UNSPTD FuncBlk types, and is unused during box instance resolution
      //
      McId = 0;
    }

    //
    // Need to convert the NodeId/DDR channel ID to box Instance for MC boxes
    //
    BoxInst = MemGetBoxInst (McId, BoxType, ChIdOrBoxInst, FuncBlk, CpuCsrAccessVar->DdrMaxCh, CpuCsrAccessVar->DdrNumChPerMc);
  }  else if ((BOX_HBM == BoxType) || (BOX_HBM2E_MC == BoxType) || (Hbm2MemSs == MemSsType)) {

    CpuCsrAccessError ("HBM or Hbm2MemSs are not supported in this function\n");
    //
    // Need to convert the NodeId/DDR channel ID to box Instance for MC boxes
    //
    BoxInst = (ALL_CH == ChIdOrBoxInst) ? 0:ChIdOrBoxInst;
  } else if (BoxType == BOX_MCCADB) {
    //
    // Before Gen3 platforms, the CADB instance within the channel is encoded in the offset
    //
    InstancesPerCh = 1;

    if (FuncBlk == BOX_FUNC_CADB_GEN3) {
      //
      // CADB addressing at the SOC level in Gen3 platforms requires the instance of the CADB in the SOC
      // The CADB instance within a channel cannot be encoded in the offset
      //
      InstancesPerCh = CpuCsrAccessVar->DdrNumPseudoChPerCh;

    }

    if ((ChIdOrBoxInst >= (CpuCsrAccessVar->DdrMaxCh * InstancesPerCh)) ||
        (InstancesPerCh == 0))
    {
      CpuCsrAccessError ("MemReadPciCfgEp: Assert on access to CADB instance %d >= %d\n",
        ChIdOrBoxInst,
        CpuCsrAccessVar->DdrMaxCh * InstancesPerCh);
    }

    McId = CpuCsrAccessVar->mcId[socket][ChIdOrBoxInst / InstancesPerCh];
    BoxInst = ChIdOrBoxInst;

  } else {
    //
    // For non-MC boxes pass the Box Instance directly
    //
    BoxInst = ChIdOrBoxInst;
  }

  return UsraCsrRead (SocId, BoxInst, Offset);
}

/**

  Reads CPU Memory Controller configuration space using the MMIO mechanism

  @param[in] socket        - Socket number
  @param[in] mcId          - Memory controller ID
  @param[in] Offset        - Register offset; values come from the auto generated header file

  @retval Register value

**/
UINT32
MemReadPciCfgMC (
  IN  UINT8       socket,
  IN  UINT8       mcId,
  IN  UINT32      Offset
  )
{
  UINT8 BoxInst;
  UINT8 SocId;
  UINT8 BoxType;
  UINT8              MemSsType;
  CSR_OFFSET RegOffset;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  SocId = socket;
  RegOffset.Data = Offset;
  BoxType = (UINT8)RegOffset.Bits.boxtype;
  MemSsType      = CpuCsrAccessVar->MemSsType[SocId];

  if (( BoxType == BOX_MC || BoxType == BOX_MCDDC || BoxType == BOX_MCIO || BoxType == BOX_MCCPGC || BoxType == BOX_MCCADB || BoxType == BOX_MCCMI ) &&
      (MemSsType != Hbm2MemSs)){

    // SKX hack for now...
    if (mcId >= CpuCsrAccessVar->DdrMaxImc) {
      CpuCsrAccessError ("Assert on access to mc >= MAX_IMC\n");
    }

    //
    // Need to convert the NodeId/DDR channel ID to box Instance for
    // MC boxes
    //
    BoxInst = mcId * CpuCsrAccessVar->DdrNumChPerMc;
  } else if ((BOX_HBM == BoxType) || (BOX_HBM2E_MC == BoxType) || (MemSsType == Hbm2MemSs)){
    CpuCsrAccessError ("HBM or Hbm2MemSs are not supported in this function\n");
    //
    // Need to convert the NodeId/DDR channel ID to box Instance for MC boxes
    //
    BoxInst = mcId;
  } else {

    //
    // For non-MC boxes pass the Box Instance directly
    //
    BoxInst = mcId;
  }

  return UsraCsrRead (SocId, BoxInst, Offset);
} // MemReadPciCfgMC

/**

  Writes CPU Memory Controller configuration space using the MMIO mechanism

  @param[in] socket        - Socket number
  @param[in] ChIdOrBoxInst - DDR channel Id within a memory controller
                             or Box Instance (in case of non-MC boxes), 0 based, 0xFF if not used
  @param[in] Offset        - Register offset; values come from the auto generated header file
  @param[in] Data          - Register data to be written

**/
VOID
MemWritePciCfgEp (
  IN  UINT8       socket,
  IN  UINT8       ChIdOrBoxInst,
  IN  UINT32      Offset,
  IN  UINT32      Data
  )
{
  UINT8 BoxInst;
  UINT8 SocId;
  UINT8 BoxType;
  UINT8 McId;
  UINT8 FuncBlk;
  UINT8 MemSsType;
  CSR_OFFSET RegOffset;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  McId = 0;
  SocId = socket;
  MemSsType = CpuCsrAccessVar->MemSsType[SocId];

  RegOffset.Data = Offset;
  BoxType = (UINT8)RegOffset.Bits.boxtype;
  FuncBlk = (UINT8)RegOffset.Bits.funcblk;
  if (((BoxType == BOX_MC) && (MemSsType != Hbm2MemSs)) || ((BoxType == BOX_MCDDC) && (MemSsType != Hbm2MemSs)) || BoxType == BOX_MCIO) {

    if ((FuncBlk != BOX_FUNC_MCIO_DDRIO_UNSPTD) && (FuncBlk != BOX_FUNC_MC_UNSPTD)) {
      if ((ChIdOrBoxInst >= CpuCsrAccessVar->DdrMaxCh) || (ChIdOrBoxInst >= MAX_CH)) {
        CpuCsrAccessError ("Assert on access to ch >= DdrMaxCh\n");
        return;
      }

      McId = CpuCsrAccessVar->mcId[socket][ChIdOrBoxInst];
    } else {

      //
      // McId is calclated later during physical address translation for the MCIO_DDRIO_UNSPTD
      // and MC_UNSPTD FuncBlk types, and is unused during box instance resolution
      //
      McId = 0;
    }

    //
    // Need to convert the NodeId/DDR channel ID to box Instance for MC boxes
    //
    BoxInst = MemGetBoxInst (McId, BoxType, ChIdOrBoxInst, FuncBlk, CpuCsrAccessVar->DdrMaxCh, CpuCsrAccessVar->DdrNumChPerMc);
  } else {
    //
    // For non-MC boxes pass the Box Instance directly
    //
    if ((BOX_HBM == BoxType) || (BOX_HBM2E_MC == BoxType)) {
      CpuCsrAccessError ("HBM or Hbm2MemSs are not supported in this function\n");
      BoxInst = (ALL_CH == ChIdOrBoxInst) ? 0 : ChIdOrBoxInst;
    } else {
      BoxInst = ChIdOrBoxInst;
    }
  }
  UsraCsrWrite (SocId, BoxInst, Offset, Data);
}

/**

  Writes CPU Memory Controller configuration space using the MMIO mechanism

  @param[in] socket        - Socket number
  @param[in] mcId          - Memory controller ID
  @param[in] Offset        - Register offset; values come from the auto generated header file
  @param[in] Data          - Register data to be written

**/
VOID
MemWritePciCfgMC (
  IN  UINT8       socket,
  IN  UINT8       mcId,
  IN  UINT32      Offset,
  IN  UINT32      Data
  )
{
  UINT8 BoxInst;
  UINT8 SocId;
  UINT8 BoxType;
  CSR_OFFSET RegOffset;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  SocId = socket;

  RegOffset.Data = Offset;
  BoxType = (UINT8)RegOffset.Bits.boxtype;

  if ( BoxType == BOX_MC || BoxType == BOX_MCDDC || BoxType == BOX_MCIO ) {

    // SKX hack for now...
    if (mcId >= CpuCsrAccessVar->DdrMaxImc) {
      CpuCsrAccessError ("Assert on access to mc %d >= MAX_IMC\n", mcId);
    }

    //
    // Need to convert the NodeId/DDR channel ID to box Instance for
    // MC boxes
    //
    BoxInst = mcId * CpuCsrAccessVar->DdrNumChPerMc;

  } else {

    //
    // For non-MC boxes pass the Box Instance directly
    //
    BoxInst = mcId;
  }
  UsraCsrWrite (SocId, BoxInst, Offset, Data);
} // MemWritePciCfgMC

/**

  Writes CPU Memory Controller configuration space using the MMIO mechanism

  @param[in] socket        - Socket number
  @param[in] mcId          - Memory controller ID
  @param[in] Offset        - Register offset; values come from the auto generated header file
  @param[in] Data          - Register data to be written

**/
VOID
MemWritePciCfgMC_AllCh (
  IN  UINT8    socket,
  IN  UINT8    mcId,
  IN  UINT32   Offset,
  IN  UINT32   Data
  )
{
  UINT8 SocId;
  UINT8              MemSsType;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;
  UINT8 i;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  SocId = socket;

  MemSsType      = CpuCsrAccessVar->MemSsType[SocId];

  //
  // Write all channels in MC
  //
  if (MemSsType != Hbm2MemSs) {
    // For DDR
    if (mcId >= CpuCsrAccessVar->DdrMaxImc){
      CpuCsrAccessError ("Assert on access to ddr mc %d >= MAX_IMC\n", mcId);
    }
    for (i = 0; i < CpuCsrAccessVar->DdrNumChPerMc; i++) {
      MemWritePciCfgEp (SocId, i + (mcId * CpuCsrAccessVar->DdrNumChPerMc), Offset, Data);
    }
  } else {
    CpuCsrAccessError ("HBM or Hbm2MemSs are not supported in this function\n");
  }
} // MemWritePciCfgMC_AllCh

/**

  Get the Memory Controller PCI config Address

  @param[in] socket   - CPU Socket Node number (Socket ID)
  @param[in] mcId     - Memory controller ID
  @param[in] Offset   - Register offset; values come from the auto generated header file

  @retval Returns the return value from UsraGetCsrRegisterAddress

**/
UINTN
MemGetPciCfgMCAddr (
  IN  UINT8    socket,
  IN  UINT8    mcId,
  IN  UINT32   Offset
  )
{
  UINT8 BoxInst;
  UINT8 SocId;
  UINT8 BoxType;
  CSR_OFFSET RegOffset;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  SocId = socket;

  RegOffset.Data = Offset;
  BoxType = (UINT8)RegOffset.Bits.boxtype;

  if ( BoxType == BOX_MC || BoxType == BOX_MCDDC || BoxType == BOX_MCIO ) {

    // SKX hack for now...
    if (mcId >= CpuCsrAccessVar->DdrMaxImc) {
      CpuCsrAccessError ("Assert on access to mc %d >= MAX_IMC\n", mcId);
    }

    //
    // Need to convert the NodeId/DDR channel ID to box Instance for
    // MC boxes
    //
    BoxInst = mcId * CpuCsrAccessVar->DdrNumChPerMc;

  } else {

    //
    // For non-MC boxes pass the Box Instance directly
    //
    BoxInst = mcId;
  }
  return UsraGetCsrRegisterAddress (SocId, BoxInst, Offset);
} // MemGetPciCfgMCAddr

/**

  Get a Memory channel's EPMC Main value

  @param[in] socket    - CPU Socket Node number (Socket ID)
  @param[in] regBase   - MMIO Reg address of first base device

  @retval EPMC main value

**/
UINT32
MemReadPciCfgMain (
  IN  UINT8       socket,
  IN  UINT32      regBase
  )
{
  UINT32  data = 0;
  UINT8 i;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  for (i = 0; i < CpuCsrAccessVar->DdrMaxImc; i++) {
    if (CpuCsrAccessVar->imcEnabled[socket][i]) {
      data = MemReadPciCfgMC (socket, i, regBase);
    }
  }
  return data;

} // MemReadPciCfgMain

/**

  Write a Memory channel's EPMC Main value

  @param[in] socket    - CPU Socket Node number (Socket ID)
  @param[in] regBase   - MMIO Reg address of first base device
  @param[in] data      - Data to write

**/
VOID
MemWritePciCfgMain (
  IN  UINT8       socket,
  IN  UINT32      regBase,
  IN  UINT32      data
  )
{
  UINT8 i;
  CPU_CSR_ACCESS_VAR *CpuCsrAccessVar;

  CpuCsrAccessVar = GetSysCpuCsrAccessVar ();

  for (i = 0; i < CpuCsrAccessVar->DdrMaxImc; i++) {
    if (CpuCsrAccessVar->imcEnabled[socket][i]) {
      MemWritePciCfgMC_AllCh (socket, i, regBase, data);
    }
  }
} // MemWritePciCfgMain

SOCKET_NVRAM *
EFIAPI
GetSocketNvramData (
  IN        UINT8           Socket
)
{
  SYSHOST *Host;

  Host = GetSysHostPointer ();

  return &Host->nvram.mem.socket[Socket];
}

struct channelNvram
(*GetChannelNvList (
  UINT8     socket
  ))[MAX_CH]
{
  SOCKET_NVRAM *SocketNvramPtr;
  SocketNvramPtr = GetSocketNvramData (socket);

  return (&SocketNvramPtr->channelList);
}

struct dimmNvram
(*GetDimmNvList (
  UINT8     socket,
  UINT8     ch
  ))[MAX_DIMM]
{

  SOCKET_NVRAM *SocketNvramPtr;
  SocketNvramPtr = GetSocketNvramData (socket);

  return (&SocketNvramPtr->channelList[ch].dimmList);
}

struct ddrRank
(*GetRankNvList (
  UINT8     socket,
  UINT8     ch,
  UINT8     dimm
  ))[MAX_RANK_DIMM]
{
  SOCKET_NVRAM *SocketNvramPtr;
  SocketNvramPtr = GetSocketNvramData (socket);

  return (&SocketNvramPtr->channelList[ch].dimmList[dimm].rankList);
}

BOOLEAN
EFIAPI
IsChannelEnabled (
  IN UINT8 Socket,
  IN UINT8 Channel
  )
{
  CHANNEL_NVRAM_STRUCT (*ChannelNvList)[MAX_CH];

  ChannelNvList = GetChannelNvList (Socket);

  if ((*ChannelNvList)[Channel].enabled == 0) {
    return FALSE;
  }

  return TRUE;

} // IsChannelEnabled

BOOLEAN
EFIAPI
Is3dsDimmPresentInChannel (
  IN UINT8 Socket,
  IN UINT8 Channel
  )
{
  CHANNEL_NVRAM_STRUCT (*ChannelNvList)[MAX_CH];

  ChannelNvList = GetChannelNvList (Socket);

  if (((*ChannelNvList)[Channel].encodedCSMode == 2) && IsChannelEnabled (Socket, Channel)) {
    return TRUE;
  }
  return FALSE;
}

BOOLEAN
EFIAPI
IsX4Dimm (
  IN UINT8     Socket,
  IN UINT8     Channel,
  IN UINT8     Dimm
  )
{
  struct dimmNvram  (*DimmNvList)[MAX_DIMM];

  DimmNvList = GetDimmNvList (Socket, Channel);

  if ((*DimmNvList)[Dimm].x4Present != 0) {
    return TRUE;
  }

  return FALSE;

} // IsX4Dimm

BOOLEAN
EFIAPI
IsX8Dimm (
  IN UINT8     Socket,
  IN UINT8     Channel,
  IN UINT8     Dimm
  )
{
  struct dimmNvram  (*DimmNvList)[MAX_DIMM];

  DimmNvList = GetDimmNvList (Socket, Channel);

  if ((*DimmNvList)[Dimm].dimmPresent == 0) {
    return FALSE;
  }

  if (((*DimmNvList)[Dimm].dramIOWidth) == 1) {
    return TRUE;
  }

  return FALSE;

} // IsX8Dimm

#define FLAG_UNSUP 0
#define FLAG_MINUS 1
#define FLAG_ADD   2
#define BLOCK_ID_OFFSET 8

typedef struct {
  UINT8     BlockId;
  UINT8     SignFlag;
  UINT16    Offset;
} DDR_TO_HBM_MAPPING;

//
// -------------------------------------------------------------
// |  BlockId  - DDR Register offset left sheft 8 bits         |
// |  SignFlag - DDR offset to HBM offset add or minus flag    |
// |  Offset   - Offset used to convert DDR registers to HBM   |
// -------------------------------------------------------------
//ww09b
//                        DDR MC  HBM MC  SignFlag Offset
//  mc_2lmcntl            0x0008  0x0008  0
//  mcdecs                0x080c  0x080c  0
//  mc_dec                0x0c00  0x0c00  0
//  mc_2lmddrt            0x1400                   // Unmapped
//  mcscheds_bs           0x1800  0x1800  0
//  mcmisc                0x2000  0x8000  +0x6000
//  mcscheds_pq           0x2800  0x1c00  -0x0c00
//  mc_rrd                0x2c08  0x2808  -0x0400  // 0x2c08 for mc_rrd_crnode1
//  mc_wdb                0x3000  0x3000  0        // 0x3400 for mc_wdb_crnode1
//  mc_wp                 0x3400  0x1000  -0x2400  // 0x1400 for mc_wp_crnode1
//  cpgc_shared_cpgc_p    0x9000  0x3800  -0x5800
//  cpgc_shared_cpgc_v    0x9018  0x3818  -0x5800
//  cpgc_reqgen0_cpgc_t   0x9030  0x3830  -0x5800
//  cpgc_channel0_cpgc_d  0x9120  0x3920  -0x5800
//  cpgc_buffer0_cpgc_b   0x92d0  0x3ad0  -0x5800
//  cpgc_reqgen0_cpgc_a   0x9320  0x3b20  -0x5800
//  cpgc_reqgen1_cpgc_t   0x9430  0x3c30  -0x5800
//  cpgc_channel1_cpgc_d  0x9520  0x3d20  -0x5800
//  cpgc_buffer1_cpgc_b   0x96d0  0x3ed0  -0x5800
//  cpgc_reqgen1_cpgc_a   0x9720  0x3f20  -0x5800

//
DDR_TO_HBM_MAPPING DdrToHbm[] = {
  {0x00, FLAG_MINUS, 0     }, // mc_2lmcntl, mcdecs, mc_dec
  {0x14, FLAG_UNSUP, 0     }, // mc_2lmddrt
  {0x18, FLAG_MINUS, 0     }, // mcscheds_bs
  {0x20, FLAG_ADD,   0x6000}, // mcmisc
  {0x28, FLAG_MINUS, 0x0c00}, // mcscheds_pq
  {0x2c, FLAG_MINUS, 0x0400}, // mc_rrd
  {0x30, FLAG_MINUS, 0     }, // mc_wdb
  {0x34, FLAG_MINUS, 0x2400}, // mc_wp
  {0x90, FLAG_MINUS, 0x5800}, // cpgc_shared_cpgc_p, cpgc_shared_cpgc_v, cpgc_reqgen0_cpgc_t, cpgc_channel0_cpgc_d, cpgc_buffer0_cpgc_b, cpgc_reqgen0_cpgc_a
                              // cpgc_reqgen1_cpgc_t, cpgc_channel1_cpgc_d, cpgc_buffer1_cpgc_b, cpgc_reqgen1_cpgc_a
};

VOID
DdrToHbmOffset (
  IN  OUT  CSR_OFFSET  *CsrOffset
  )
{
  UINT8           Index;
  UINT8           BlockId;
  UINT8           Limit;

  //
  // Get DdrToHbm Offset Table
  //
  BlockId = (UINT8) (CsrOffset->Bits.offset >> BLOCK_ID_OFFSET);
  Limit = sizeof (DdrToHbm) / sizeof (DdrToHbm[0]);
  for (Index = 1; Index < Limit; Index++) {
    if (DdrToHbm[Index].BlockId > BlockId) {
        break;
    }
  }
  Index = Index - 1;

  //
  // Convert Offset from DDR to HBM
  //
  if (DdrToHbm[Index].SignFlag == FLAG_MINUS) {
    CsrOffset->Bits.offset -= DdrToHbm[Index].Offset;
  } else if (DdrToHbm[Index].SignFlag == FLAG_ADD) {
    CsrOffset->Bits.offset += DdrToHbm[Index].Offset;
  } else {
    ASSERT(FALSE);
  }
}

UINT32
DdrToHbmCompatibility (
  IN  MEM_TECH_TYPE  MemTechType,
  IN  UINT32         RegOffset
  )
{
  CSR_OFFSET      CsrOffset;
  UINT8           BoxType;
  UINT8           FuncBlk;
  CSR_OFFSET      TempReg;

  CsrOffset.Data = RegOffset;
  BoxType = (UINT8) CsrOffset.Bits.boxtype;
  FuncBlk = (UINT8) CsrOffset.Bits.funcblk;

  //
  // Adjust offset if pseudo
  //
  if (CsrOffset.Bits.pseudo) {
    TempReg.Data = GetPseudoRegisterOffset (CsrOffset);
    CsrOffset.Bits.offset = TempReg.Bits.offset;
    CsrOffset.Bits.size = TempReg.Bits.size;
    //
    // Clear offset bit
    //
    CsrOffset.Bits.pseudo = 0;
  }

  //
  // Do convert only if the Mem Technology Type is HBM and the Rc Header is Combine MC Header(DDR)
  //
  if ((MemTechType == MemTechHbm) && (BoxType != BOX_HBM && BoxType != BOX_HBM2E_MC)) {

    //
    // Not support pseudo to access HBM MC registers
    //
    if (CsrOffset.Bits.pseudo) {
      ASSERT(FALSE);
    }

    //
    // Convert Function Block from DDR to HBM
    //
    if (BoxType == BOX_MC) {
      if (FuncBlk == BOX_FUNC_MC_MAIN) {
        CsrOffset.Bits.funcblk = SPR_HBM_FUNC_MAIN;
      } else if (FuncBlk == BOX_FUNC_MC_2LM) {
        CsrOffset.Bits.funcblk = SPR_HBM_FUNC_2LM;
      } else if (FuncBlk == BOX_FUNC_MC_GLOBAL) {
        CsrOffset.Bits.funcblk = SPR_HBM_FUNC_MC_GLOBAL;
      } else {
        ASSERT(FALSE);
      }
    } else if (BoxType == BOX_MCDDC) {
      if (FuncBlk == BOX_FUNC_MCDDC_CTL) {
        CsrOffset.Bits.funcblk = SPR_HBM_FUNC_DDC_CTL;
      } else if (FuncBlk == BOX_FUNC_MCDDC_DP) {
        CsrOffset.Bits.funcblk = SPR_HBM_FUNC_DDC_DP;
      } else {
        ASSERT(FALSE);
      }
    } else if (BoxType == BOX_MCCPGC) {
      CsrOffset.Bits.funcblk = SPR_HBM_FUNC_CPGC;
    } else {
      ASSERT(FALSE);
    }

    //
    // Convert Box Type from DDR to HBM
    //
    CsrOffset.Bits.boxtype = BOX_HBM2E_MC;

    DdrToHbmOffset(&CsrOffset);

  }

  return CsrOffset.Data;
}

UINT32
EFIAPI
ChRegisterRead (
  IN  MEM_TECH_TYPE      MemTechType,
  IN  UINT8              SocketId,
  IN  UINT8              ChId,
  IN  UINT32             CsrOffset
  )
{
  USRA_ADDRESS           RegisterId;
  UINT32                 Data32;

  CsrOffset = DdrToHbmCompatibility (MemTechType, CsrOffset);

  USRA_CSR_CHID_OFFSET_ADDRESS (RegisterId, SocketId, ChId, CsrOffset);

  RegisterRead (&RegisterId, &Data32);

  return Data32;
}

// UINT8 mSpdTableDDR[MAX_CH][MAX_DIMM][MAX_SPD_BYTE_DDR];
UINT8 mSpdTableDDR[MAX_CH][MAX_DIMM][2][MAX_SPD_BYTE_DDR];
UINT8 mSpdModuleType;

EFI_STATUS
EFIAPI
SpdAccessLibOnBoardConstructor (
  VOID
  )
{
  VOID              *SpdTable;
  EFI_HOB_GUID_TYPE *GuidHob = NULL;

  //
  // Get SPD table HOB
  //
  GuidHob = GetFirstGuidHob (&gSpdTableHobGuid);

  if (GuidHob != NULL) {
    SpdTable = GET_GUID_HOB_DATA (GuidHob);
    // Read the SPD_KEY_BYTE in Channel 0, Dimm 0, do not support DIMM mixed plugged.
    mSpdModuleType = *((UINT8 *) SpdTable + SPD_KEY_BYTE);
    // Do SpdTable copy work by different DDR type.
    // switch (mSpdModuleType) {
    //   case SPD_TYPE_DDR4:
    //   case SPD_TYPE_AEP:
    //   case SPD_TYPE_DDR5:
        CopyMem ((VOID *) mSpdTableDDR, SpdTable, sizeof (mSpdTableDDR));
    //     break;

    //   default:
    //     DEBUG ((EFI_D_ERROR, "SpdAccessLibOnBoardConstructor: Unsupported mSpdModuleType = 0x%x\n", mSpdModuleType));
    //     return EFI_UNSUPPORTED;
    // }
  } else {
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SpdReadByte (
  IN     UINT8  Socket,
  IN     UINT8  Channel,
  IN     UINT8  Dimm,
  IN     UINT16 ByteOffset,
     OUT UINT8  *Data
  )
{
  //
  // Sanity check of incoming data
  //
  if ((Socket >= MAX_SOCKET) || (Channel >= MAX_CH) || (Dimm >= MAX_DIMM) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Do parameter check and Spd data read work by different DDR type.
  switch (mSpdModuleType) {
    case SPD_TYPE_DDR4:
    case SPD_TYPE_AEP:
      if (ByteOffset >= MAX_SPD_BYTE_DDR4) {
        return EFI_INVALID_PARAMETER;
      }
      break;

    case SPD_TYPE_DDR5:
      if (ByteOffset >= MAX_SPD_BYTE_DDR5) {
        return EFI_INVALID_PARAMETER;
      }
      break;

    default:
      DEBUG ((EFI_D_ERROR, "SpdReadByte: Unsupported mSpdModuleType = 0x%x\n", mSpdModuleType));
      return EFI_UNSUPPORTED;
  }

  if (mSpdTableDDR[Channel][Dimm][0] == 0x00) {
    return EFI_NOT_FOUND;
  } else {
    *Data = mSpdTableDDR[Channel][Dimm][ByteOffset];
  }

  return EFI_SUCCESS;
}

SYSTEM_MEMORY_MAP_HOB *
EFIAPI
GetSystemMemoryMapData (
  VOID
  )
{
  EFI_HOB_GUID_TYPE             *GuidHob;
  EFI_PHYSICAL_ADDRESS          *MemMapDataAddr;

  GuidHob = GetFirstGuidHob (&gEfiMemoryMapGuid);
  if (GuidHob == NULL) {
    return NULL;
  }

  MemMapDataAddr = GET_GUID_HOB_DATA (GuidHob);
  return (SYSTEM_MEMORY_MAP_HOB *) (UINTN) (*MemMapDataAddr);
}

UINT8
EFIAPI
GetNumberOfRanksOnDimm (
  IN UINT8     Socket,
  IN UINT8     Channel,
  IN UINT8     Dimm
  )
{
  struct dimmNvram  (*DimmNvList)[MAX_DIMM];

  DimmNvList = GetDimmNvList (Socket, Channel);

  return MIN ((*DimmNvList)[Dimm].numRanks, MAX_RANK_DIMM);

} // GetNumberOfRanksOnDimm

//OKN_20240822_yjb_EgsAddrTrans >>
MEMORY_ADDRESS_TRANSLATION_PROTOCOL *gAddrTrans = NULL;

EFI_STATUS MemoryAddressTranslationInit(VOID)
{
  EFI_STATUS        Status = 0;

  Status = gBS->LocateProtocol(&gMemoryAddressTranslationProtocolGuid, NULL, (void **)&gAddrTrans);

  return Status;
}
//OKN_20240822_yjb_EgsAddrTrans <<

UINT8
EFIAPI
Reverse (
  IN  UINT8  Data
)
{

#ifdef IA32
  //
  // reverse this (8-bit) byte
  //
  Data = (UINT8)(((Data * 0x0802LU & 0x22110LU) | (Data * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16);
#else
  //
  // reverse this (8-bit) byte
  //
  Data = (UINT8) ((Data * 0x0202020202ULL & 0x010884422010ULL) % 1023);
#endif
  return Data;
}

UINT32
EFIAPI
ReverseReadResult (
  IN  UINT32    Data
)
{
  MR_READ_RESULT_STRUCT   *MrReadDqData;

  MrReadDqData = (MR_READ_RESULT_STRUCT *) &Data;
  MrReadDqData->Bits.Device0 = Reverse(MrReadDqData->Bits.Device0);
  MrReadDqData->Bits.Device1 = Reverse(MrReadDqData->Bits.Device1);
  MrReadDqData->Bits.Device2 = Reverse(MrReadDqData->Bits.Device2);
  MrReadDqData->Bits.Device3 = Reverse(MrReadDqData->Bits.Device3);

  return MrReadDqData->Data;
}

EFI_STATUS
EFIAPI
MrReadRas (
  IN  UINT8                         Socket,
  IN  UINT8                         Chonskt,
  IN  UINT8                         Subch,
  IN  UINT8                         Rank,
  IN  UINT8                         Addr,
  OUT RAS_ECS_READ_RESULT_STRUCT    *Data,
  IN  BOOLEAN                       Unlocked
)
{
  EFI_STATUS                        Status = EFI_SUCCESS;
  MR_CONFIG_MCDDC_CTL_STRUCT        CrMrConfig;
  CRDEFEATURE2_BS_MCDDC_CTL_STRUCT  CrDefeature2Bs;
  UINT64                            StartCount;

  CrMrConfig.Data = ReadCpuCsr (Socket, Chonskt, MR_CONFIG_MCDDC_CTL_REG);
  if (CrMrConfig.Bits.start_inprogress == 1) {
    return EFI_UNSUPPORTED;
  }

  if (Unlocked == TRUE) {
    CrDefeature2Bs.Data = (UINT16) ReadCpuCsr (Socket, Chonskt, CRDEFEATURE2_BS_MCDDC_CTL_REG);
    Print(L"CrDefeature2Bs.Data:0x%x\n", CrDefeature2Bs.Data);
    if ((CrDefeature2Bs.Bits.spare & BIT0) == 0) {

    }
    // WriteCpuCsr (Socket, Chonskt, CRDEFEATURE2_BS_MCDDC_CTL_REG, CrDefeature2Bs.Data);
  }

  CrMrConfig.Bits.start_inprogress = 1;
  CrMrConfig.Bits.mrw = 0;
  CrMrConfig.Bits.subchannel = Subch;
  CrMrConfig.Bits.rank = Rank;
  CrMrConfig.Bits.mrr_addr = Addr;
  WriteCpuCsr (Socket, Chonskt, MR_CONFIG_MCDDC_CTL_REG, CrMrConfig.Data);

  StartCount = AsmReadTsc();
  do {
    CrMrConfig.Data = ReadCpuCsr (Socket, Chonskt, MR_CONFIG_MCDDC_CTL_REG);
    if (GetTimeInNanoSecond (AsmReadTsc() - StartCount) > (CATCHALL_TIMEOUT * 1000)) {
      Status = EFI_TIMEOUT;
      break;
    }
    
  } while ((CrMrConfig.Bits.start_inprogress == 1));

  if ((CrMrConfig.Bits.failed == 1)) {
    Status = EFI_UNSUPPORTED;
  }

  if (Status == EFI_SUCCESS) {
    Data->Data[0] = ReverseReadResult (ReadCpuCsr (Socket, Chonskt, MR_READ_RESULT_DQ12TO0_MCDDC_DP_REG));
    Data->Data[1] = ReverseReadResult (ReadCpuCsr (Socket, Chonskt, MR_READ_RESULT_DQ28TO16_MCDDC_DP_REG));
    Data->Data[2] = ReverseReadResult (ReadCpuCsr (Socket, Chonskt, MR_READ_RESULT_DQ44TO32_MCDDC_DP_REG));
    Data->Data[3] = ReverseReadResult (ReadCpuCsr (Socket, Chonskt, MR_READ_RESULT_DQ60TO48_MCDDC_DP_REG));
    Data->Data[4] = ReverseReadResult (ReadCpuCsr (Socket, Chonskt, MR_READ_RESULT_DQ68TO64_MCDDC_DP_REG));
  }

  // if ((Unlocked == FALSE) && IsSiliconWorkaroundEnabled ("S14014005573")) {
  //   CrDefeature2Bs.Data = (UINT16) ReadCpuCsr (Socket, Chonskt, CRDEFEATURE2_BS_MCDDC_CTL_REG);
  //   CrDefeature2Bs.Bits.spare &= (~BIT0);
  //   WriteCpuCsr (Socket, Chonskt, CRDEFEATURE2_BS_MCDDC_CTL_REG, CrDefeature2Bs.Data);
  // }

  return Status;
}

EFI_STATUS
EFIAPI
IntelPortingLibConstructor (
  VOID
  )
{
  EFI_STATUS Status = 0;
  Status |= StaticPointerTableLibConstructor();
  Status |= DxeSystemInfoLibConstructor();
  Status |= DxeCsrPseudoOffsetConvertLibConstructor();
  Status |= SpdAccessLibOnBoardConstructor();
  Status |= MemoryAddressTranslationInit();               //OKN_20240822_yjb_EgsAddrTrans
  return Status;
}
