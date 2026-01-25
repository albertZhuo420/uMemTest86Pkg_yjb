//PassMark MemTest86
//
//Copyright (c) 2013-2016
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
//Program:
//	MemTest86
//
//Module:
//	MPSupportLib.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Multiprocessor support function declarations

#ifndef _MPSUPPORT_H_
#define _MPSUPPORT_H_

#include <Pi/PiDxeCis.h>      // Needed for the definition of EFI_AP_PROCEDURE
#include <Protocol/MpService.h>
#include <Library/SynchronizationLib.h>
#include <Library/MemTestSupportLib.h>

struct barrier_s
{
    SPIN_LOCK mutex;
    SPIN_LOCK lck;
    int maxproc;
    volatile int count;
    SPIN_LOCK st1;
    SPIN_LOCK st2;
};

// barrier_init
//
// Initialize the barrier structure for synchronization
void barrier_init(struct barrier_s *barr, int max);

// barrier
//
// Synchronize all processors to the same point in code
void barrier(struct barrier_s *barr);

// MPSupportStartMemTestAP
//
// Prepares the memory test parameters, then dispatch to each AP
EFI_STATUS
EFIAPI
MPSupportStartMemTestAP(
  IN UINT32                   TestNum,
  IN UINTN                    ProcNum,
  IN EFI_EVENT                *WaitEvent,
  IN TEST_MEM_RANGE			  RangeTest,
  IN EFI_PHYSICAL_ADDRESS     Start,
  IN UINT64                   Length,
  IN UINT64                   NumIters,
  IN TESTPAT                  Pattern,
  IN UINT64                   PatternUser,
  IN VOID                     *Context);

// MPSupportGetMemTestAPResult
//
// Retrieves the return value for given AP
EFI_STATUS
EFIAPI
MPSupportGetMemTestAPResult(
IN UINTN                    ProcNum
);

// MPSupportGetMemTestAPFinished
//
// Returns whether the given AP has finished running the memory test dispatch function
BOOLEAN
EFIAPI
MPSupportGetMemTestAPFinished(
IN UINTN                    ProcNum
);

// MPSupportGetMemTestAPTestTime
//
// Returns the time taken for the given AP to finish running the memory test dispatch function
UINT64
EFIAPI
MPSupportGetMemTestAPTestTime(
IN UINTN                    ProcNum
);

// MPSupportEnableCacheAP
//
// Prepares the enable/disable cache parameters, then dispatch to each AP
EFI_STATUS
EFIAPI
MPSupportEnableCacheAP(
  IN UINTN                    ProcNum,
  IN EFI_EVENT                *WaitEvent,
  IN BOOLEAN                  Enable,
  IN struct barrier_s         *barr);

typedef struct _MTRR_SETTINGS_ MTRR_SETTINGS;

// MPSupportSetAllMtrrsAP
//
// Set the MTRR registers for each AP
EFI_STATUS
EFIAPI
MPSupportSetAllMtrrsAP(
  IN UINTN                    ProcNum,
  IN EFI_EVENT                *WaitEvent,
  IN MTRR_SETTINGS            *AllMtrrs,
  IN struct barrier_s         *barr);

// MPSupportTestAP
//
// Dispatch test function  used during startup to test MP capabilities
EFI_STATUS
EFIAPI
MPSupportTestAP(
  IN UINTN                    ProcNum,
  IN BOOLEAN                  *Finished,
  IN EFI_EVENT                *WaitEvent);

// MPSupportTestMPServices
//
// For testing EFI firmware multiprocessor services (eg. for debugging). Not used in Memtest86 directly
BOOLEAN
EFIAPI
MPSupportTestMPServices();

// MPSupportEnableSSEAP
//
// Prepares the enable SSE parameters, then dispatch to each AP
EFI_STATUS
EFIAPI
MPSupportEnableSSEAP(
  IN UINTN                    ProcNum,
  IN EFI_EVENT                *WaitEvent);

// MPSupportEnableSSEAllAPs
//
// Enable SSE instructions on all APs
EFI_STATUS
EFIAPI
MPSupportEnableSSEAllAPs();

// MPSupportWhoAmI
//
// Return the proc number of calling processor
UINTN
EFIAPI
MPSupportWhoAmI();

// MPSupportGetNumProcessors
//
// Return the number of processors in the system
UINTN
EFIAPI
MPSupportGetNumProcessors();

// MPSupportGetMaxProcessors
//
// Return the maximum number of processors available for testing
UINTN
EFIAPI
MPSupportGetMaxProcessors();

// MPSupportGetNumEnabledProcessors
//
// Return the number of enabled processors in the system
UINTN
EFIAPI
MPSupportGetNumEnabledProcessors();

// MPSupportGetNumPackages
//
// Return the number of CPU packages in the system
UINTN
EFIAPI
MPSupportGetNumPackages();


// MPSupportIsProcEnabled
//
// Return TRUE if specified processor is enabled, FALSE otherwise
BOOLEAN
EFIAPI
MPSupportIsProcEnabled(UINTN ProcNum);

// MPSupportEnableProc
//
// Enables/Disables the specified processor
BOOLEAN
EFIAPI
MPSupportEnableProc(UINTN ProcNum, BOOLEAN Enable);

// MPSupportResetProc
//
// Reset the specified processor
EFI_STATUS
EFIAPI
MPSupportResetProc(UINTN ProcNum);

// MPSupportGetBspId
//
// Return the processor number of the BSP
UINTN
EFIAPI
MPSupportGetBspId();

// MPSupportSwitchBSP
//
// Switch the specified processor to be the new BSP
EFI_STATUS
EFIAPI
MPSupportSwitchBSP(UINTN ProcNum);

// MPSupportSwitchBSP
//
// Switch BSP to the specified CPU Package
EFI_STATUS
EFIAPI
MPSupportSwitchPkg(IN UINTN Pkg);

// MPSupportGetProcessorInfo
//
// Get the processor info of the specified processor
EFI_STATUS
EFIAPI
MPSupportGetProcessorInfo(
  IN UINTN ProcNum,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer);

// MPSupportInit
//
// Initialize the MPSupport library. Enumerate all processors and determine their status
EFI_STATUS
EFIAPI
MPSupportInit(IN UINTN MaxCPUs);

#endif