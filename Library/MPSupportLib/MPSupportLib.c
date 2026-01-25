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
//	MPSupportLib.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Multiprocessor support functions
//
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MtrrLib.h>
#include <Library/MemTestSupportLib.h>
#include <Library/MPSupportLib.h>
#include <SysInfoLib/cpuinfoARMv8.h>
#include <Library/Rand.h>
#include <uMemTest86.h>

#define MAX_PKGS 16

STATIC UINTN							gNumProc = 1;			// Total number of processors
STATIC UINTN							gNumProcEn = 1;			// Total number of enabled processors
STATIC BOOLEAN							gProcEn[MAX_CPUS];  // TRUE if processor is enabled, FALSE otherwise
STATIC BOOLEAN							gProcHT[MAX_CPUS];  // TRUE if processor is hyperthread, FALSE otherwise
STATIC UINTN							gPkgProcMap[MAX_PKGS];  // Map CPU package number to CPU number
STATIC UINTN							gNumPkg = 1;
STATIC UINTN                            gBspId;				// Proc# of BSP


STATIC EFI_MP_SERVICES_PROTOCOL         *PiMpService = NULL;			// MPServices Protocol

// Parameters to pass to each processor when running memory tests
typedef struct {
  UINT32                   TestNum;
  UINTN                    ProcNum;
  TEST_MEM_RANGE           RangeTest;
  EFI_PHYSICAL_ADDRESS     Start;
  UINT64                   Length;
  UINT64                   NumIters;
  TESTPAT                  Pattern;
  UINT64                   PatternUser;
  VOID                     *Context;
  EFI_STATUS			   Return;
  BOOLEAN                  Finished;
  UINT64                   TestTime;
} AP_MEMTEST_ARG;

STATIC AP_MEMTEST_ARG                   MemTestArg[MAX_CPUS];

typedef struct {
  BOOLEAN                  Enable;
  struct barrier_s         *barr;
} AP_ENCACHE_ARG;

STATIC AP_ENCACHE_ARG                   gEnCacheArg;

typedef struct {
  MTRR_SETTINGS            *AllMtrrs;
  struct barrier_s         *barr;
} AP_ALLMTRRS_ARG;

STATIC AP_ALLMTRRS_ARG                   	gAllMtrrsArg;

static __inline UINT64
EFIAPI
MPSupportGetTimeInMilliseconds ( UINT64 StartTime );

extern UINTN				clks_msec;

extern BOOLEAN				gEnableHT;

extern BOOLEAN EnableSSE ();

// APMemTestProc
//
// Dispatch function for AP
VOID
EFIAPI
APMemTestProc( IN VOID* parameter)
{
	AP_MEMTEST_ARG	*pMemTestArg = (AP_MEMTEST_ARG	*)parameter;

	UINT64 StartTime = 0;

	MtSupportSetCPURunning(pMemTestArg->ProcNum, TRUE);

	StartTime = MtSupportReadTSC();
	// MtUiAPPrint(L"[AP#%d] APMemTestProc 0x%lX - 0x%lX\r\n", MPSupportWhoAmI(), MemTestArg.Start, MemTestArg.Start + MemTestArg.Length);
	pMemTestArg->Return = pMemTestArg->RangeTest(pMemTestArg->TestNum, pMemTestArg->ProcNum, pMemTestArg->Start, pMemTestArg->Length, pMemTestArg->NumIters, pMemTestArg->Pattern, pMemTestArg->PatternUser, pMemTestArg->Context);
	
	pMemTestArg->TestTime = MPSupportGetTimeInMilliseconds(StartTime);
	pMemTestArg->Finished = TRUE;

	MtSupportSetCPURunning(pMemTestArg->ProcNum, FALSE);
}

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
  IN VOID                     *Context)
{
	EFI_STATUS Status;

	if (PiMpService == NULL)
		return EFI_NOT_FOUND;

	SetMem(&MemTestArg[ProcNum], sizeof(MemTestArg[ProcNum]), 0);
	MemTestArg[ProcNum].TestNum = TestNum;
	MemTestArg[ProcNum].ProcNum = ProcNum;
	MemTestArg[ProcNum].RangeTest = RangeTest;
	MemTestArg[ProcNum].Start = Start;
	MemTestArg[ProcNum].Length = Length;
	MemTestArg[ProcNum].NumIters = NumIters;
	MemTestArg[ProcNum].Pattern = Pattern;
	MemTestArg[ProcNum].PatternUser = PatternUser;

	MemTestArg[ProcNum].Context = Context;

	Status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, WaitEvent);
	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportStartMemTestAP - CreateEvent failed");
		return Status;
	}

	if (PiMpService)
		Status = PiMpService->StartupThisAP(  PiMpService, APMemTestProc, ProcNum, *WaitEvent, 0, &MemTestArg[ProcNum], NULL);
	else
		Status = EFI_NOT_FOUND;

	if (Status != EFI_SUCCESS)
	{
		gBS->CloseEvent(*WaitEvent);
		*WaitEvent = NULL;
	}

	return Status;
}

// MPSupportGetMemTestAPResult
//
// Retrieves the return value for given AP
EFI_STATUS
EFIAPI
MPSupportGetMemTestAPResult(
IN UINTN                    ProcNum
)
{
	return MemTestArg[ProcNum].Return;
}

// MPSupportGetMemTestAPFinished
//
// Returns whether the given AP has finished running the memory test dispatch function
BOOLEAN
EFIAPI
MPSupportGetMemTestAPFinished(
IN UINTN                    ProcNum
)
{
	return MemTestArg[ProcNum].Finished;
}

// MPSupportGetMemTestAPTestTime
//
// Returns the time taken for the given AP to finish running the memory test dispatch function
UINT64
EFIAPI
MPSupportGetMemTestAPTestTime(
IN UINTN                    ProcNum
)
{
	return MemTestArg[ProcNum].TestTime;
}

// APEnableCacheProc
//
// Dispatch function for enabling/disabling cache on each AP
VOID
EFIAPI
APEnableCacheProc( IN VOID* parameter)
{

	AP_ENCACHE_ARG	EnCacheArg;
	CopyMem(&EnCacheArg, parameter, sizeof(EnCacheArg));

#define EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE       0x000002FF
	
	if (EnCacheArg.Enable)
	{
#if 0
		UINT64 TempQword = 0;
		TempQword = AsmReadMsr64(EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE);
		// TempQword = (OldMtrr) & ~B_EFI_MSR_GLOBAL_MTRR_ENABLE & ~B_EFI_MSR_FIXED_MTRR_ENABLE;
		TempQword &= ~0xFFULL;
		TempQword |= 0x06;
		AsmWriteMsr64(EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE, TempQword);
#endif
		// barrier(EnCacheArg.barr);
		MtSupportEnableCache(TRUE);
		// barrier(EnCacheArg.barr);
	}
	else
	{
#if 0
		UINT64 TempQword = 0;
		TempQword = AsmReadMsr64(EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE);
		// TempQword = (OldMtrr) & ~B_EFI_MSR_GLOBAL_MTRR_ENABLE & ~B_EFI_MSR_FIXED_MTRR_ENABLE;
		TempQword &= ~0xFFULL;
		AsmWriteMsr64(EFI_MSR_CACHE_IA32_MTRR_DEF_TYPE, TempQword);
#endif
		// barrier(EnCacheArg.barr);
		MtSupportEnableCache(FALSE);
		// barrier(EnCacheArg.barr);
	}
}

// MPSupportEnableCacheAP
//
// Prepares the enable/disable cache parameters, then dispatch to each AP
EFI_STATUS
EFIAPI
MPSupportEnableCacheAP(
  IN UINTN                    ProcNum,
  IN EFI_EVENT                *WaitEvent,
  IN BOOLEAN                  Enable,
  IN struct barrier_s         *barr)
{
	EFI_STATUS Status;

	if (PiMpService == NULL)
		return EFI_NOT_FOUND;

	if (Enable != gEnCacheArg.Enable)
		gEnCacheArg.Enable = Enable;
	if (barr != gAllMtrrsArg.barr)
		gAllMtrrsArg.barr = barr;

	Status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, WaitEvent);
	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportEnableCacheAP - CreateEvent failed");
		return Status;
	}

	if (PiMpService)
		Status = PiMpService->StartupThisAP(  PiMpService, APEnableCacheProc, ProcNum, *WaitEvent, 0,&gEnCacheArg, NULL);
	else
		Status = EFI_NOT_FOUND;
#if 0
	if (Status == EFI_SUCCESS)
	{
		EFI_STATUS APStatus = EFI_NOT_READY;
		int LoopCount = 0;
		while (APStatus == EFI_NOT_READY && LoopCount < 5)
		{
			APStatus = gBS->CheckEvent(WaitEvent);
			gBS->Stall(100000);
		}
		if (APStatus != EFI_SUCCESS)
		{
			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportEnableCacheAP - Proc %d did not complete within %d ms (%r)", ProcNum, 500, APStatus);
			MtSupportDebugWriteLine(gBuffer);
		}
	}
#endif
	if (Status != EFI_SUCCESS)
	{
		gBS->CloseEvent(*WaitEvent);
		*WaitEvent = NULL;
	}

	return Status;
}

// APSetAllMtrrsProc
//
// Dispatch function for setting MTRR on each AP
VOID
EFIAPI
APSetAllMtrrsProc( IN VOID* parameter)
{
	AP_ALLMTRRS_ARG	*MtrrArg = (AP_ALLMTRRS_ARG *)parameter;

	MTRR_SETTINGS CurMtrrs;
    SetMem(&CurMtrrs, sizeof(CurMtrrs), 0);

    MtrrGetAllMtrrs(&CurMtrrs);
    if (CompareMem(&CurMtrrs, MtrrArg->AllMtrrs, sizeof(CurMtrrs)) != 0)
    {
		MtrrSetAllMtrrs(MtrrArg->AllMtrrs);
    }
}

EFI_STATUS
EFIAPI
MPSupportSetAllMtrrsAP(
  IN UINTN                    ProcNum,
  IN EFI_EVENT                *WaitEvent,
  IN MTRR_SETTINGS            *AllMtrrs,
  IN struct barrier_s         *barr)
{
	EFI_STATUS Status;

	if (PiMpService == NULL)
		return EFI_NOT_FOUND;

	if (AllMtrrs != gAllMtrrsArg.AllMtrrs)
		gAllMtrrsArg.AllMtrrs = AllMtrrs;
	if (barr != gAllMtrrsArg.barr)
		gAllMtrrsArg.barr = barr;

	Status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, WaitEvent);
	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportSetAllMtrrsAP - CreateEvent failed");
		return Status;
	}

	if (PiMpService)
		Status = PiMpService->StartupThisAP(  PiMpService, APSetAllMtrrsProc, ProcNum, *WaitEvent, 0,&gAllMtrrsArg, NULL);
	else
		Status = EFI_NOT_FOUND;

	if (Status != EFI_SUCCESS)
	{
		gBS->CloseEvent(*WaitEvent);
		*WaitEvent = NULL;
	}

	return Status;
}

#if 0
EFI_STATUS
EFIAPI
MPSupportEnableCacheAllAPs(IN BOOLEAN Enable)
{
	EFI_STATUS Status;
	EFI_EVENT  WaitEvent = NULL;

	if (PiMpService == NULL)
		return EFI_NOT_FOUND;

	Status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, &WaitEvent);
	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportEnableCacheAllAPs - CreateEvent failed");
		return Status;
	}

	if (PiMpService)
		Status = PiMpService->StartupAllAPs(  PiMpService, APEnableCacheProc, FALSE, WaitEvent, 0, (void *)(UINTN)Enable, NULL);
	else
		Status = EFI_NOT_FOUND;

	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportEnableCacheAllAPs - StartupAllAPs failed");
	}

	gBS->CloseEvent(WaitEvent);

	return Status;
}
#endif

// APTestProc
//
// Test dispatch function used during startup to test MP capabilities
VOID
EFIAPI
APTestProc( IN VOID* parameter)
{
	BOOLEAN *pFinished = (BOOLEAN *)parameter;

#if defined(MDE_CPU_AARCH64)
	EnableARMv8Cntr();
#endif

	UINT64 StartTime = MtSupportReadTSC();
	int ProcNum = (int)MPSupportWhoAmI();
	UINT64 sum = 0;

	rand_seed(0x12345678, ProcNum);
	while(1)
	{
		if (MPSupportGetTimeInMilliseconds(StartTime) >= 1000)
			break;

		sum += rand((int)ProcNum);
	}

	*pFinished = TRUE;
}

// MPSupportTestAP
//
// Dispatch test function  used during startup to test MP capabilities
EFI_STATUS
EFIAPI
MPSupportTestAP(
  IN UINTN                    ProcNum,
  IN BOOLEAN                  *Finished,
  IN EFI_EVENT                *WaitEvent)
{
	EFI_STATUS Status;

	if (PiMpService == NULL)
		return EFI_NOT_FOUND;

	Status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, WaitEvent);
	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportTestAP - CreateEvent failed");
		return Status;
	}

	if (PiMpService)
		Status = PiMpService->StartupThisAP(  PiMpService, APTestProc, ProcNum, *WaitEvent, 0, Finished, NULL);
	else
		Status = EFI_NOT_FOUND;

	if (Status != EFI_SUCCESS)
	{
		gBS->CloseEvent(*WaitEvent);
		*WaitEvent = NULL;
	}

	return Status;
}


// APEnableSSEProc
//
// Dispatch function for enabling SSE instructions
VOID
EFIAPI
APEnableSSEProc( IN VOID* parameter)
{
	EnableSSE();
}

// MPSupportEnableSSEAP
//
// Prepares the enable/disable SSE parameters, then dispatch to each AP
EFI_STATUS
EFIAPI
MPSupportEnableSSEAP(
  IN UINTN                    ProcNum,
  IN EFI_EVENT                *WaitEvent)
{
	EFI_STATUS Status;

	if (PiMpService == NULL)
		return EFI_NOT_FOUND;

	Status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, WaitEvent);
	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportEnableSSEAP - CreateEvent failed");
		return Status;
	}

	if (PiMpService)
		Status = PiMpService->StartupThisAP(  PiMpService, APEnableSSEProc, ProcNum, *WaitEvent, 0, NULL, NULL);
	else
		Status = EFI_NOT_FOUND;

	if (Status != EFI_SUCCESS)
	{
		gBS->CloseEvent(*WaitEvent);
		*WaitEvent = NULL;
	}

	return Status;
}

// MPSupportEnableSSEAllAPs
//
// Enable SSE instructions on all APs
EFI_STATUS
EFIAPI
MPSupportEnableSSEAllAPs()
{
	EFI_STATUS Status;
	EFI_EVENT  WaitEvent = NULL;

	if (PiMpService == NULL)
		return EFI_NOT_FOUND;

	Status = gBS->CreateEvent( 0, TPL_NOTIFY, NULL, NULL, &WaitEvent);
	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportEnableSSEAllAPs - CreateEvent failed");
		return Status;
	}


	if (PiMpService)
		return PiMpService->StartupAllAPs(  PiMpService, APEnableSSEProc, FALSE, WaitEvent, 0, NULL, NULL);
	else
		Status = EFI_NOT_FOUND;

	if (Status != EFI_SUCCESS)
	{
		MtSupportDebugWriteLine("MPSupportEnableSSEAllAPs - StartupAllAPs failed");
	}

	gBS->CloseEvent(WaitEvent);

	return Status;
}

// MPSupportWhoAmI
//
// Return the proc number of calling processor
UINTN
EFIAPI
MPSupportWhoAmI()
{
	EFI_STATUS Status;
	UINTN CpuNo = 0;

	if (PiMpService)
		Status = PiMpService->WhoAmI(PiMpService, &CpuNo);

	return CpuNo;
}

// MPSupportGetNumProcessors
//
// Return the number of processors in the system
UINTN
EFIAPI
MPSupportGetNumProcessors()
{
	if (PiMpService)
		return gNumProc;

	return 1;
}

// MPSupportGetMaxProcessors
//
// Return the maximum number of processors available for testing
UINTN
EFIAPI
MPSupportGetMaxProcessors()
{
	return MPSupportGetNumProcessors() < MAX_CPUS ? MPSupportGetNumProcessors() : MAX_CPUS;
}

// MPSupportGetNumEnabledProcessors
//
// Return the number of enabled processors in the system
UINTN
EFIAPI
MPSupportGetNumEnabledProcessors()
{
	if (PiMpService)
		return gNumProcEn;

	return 1;
}

UINTN
EFIAPI
MPSupportGetNumPackages()
{
	if (PiMpService)
		return gNumPkg;

	return 1;
}

// MPSupportIsProcEnabled
//
// Return TRUE if specified processor is enabled, FALSE otherwise
BOOLEAN
EFIAPI
MPSupportIsProcEnabled(UINTN ProcNum)
{
	// EFI_STATUS Status;
	// EFI_PROCESSOR_INFORMATION ProcessorInfoBuffer;

	if (ProcNum == gBspId)
		return TRUE;

	if (ProcNum >= gNumProc)
		return FALSE;

	if (ProcNum >= MAX_CPUS)
		return FALSE;

	if (PiMpService)
	{
		return gProcEn[ProcNum];
		//Status = PiMpService->GetProcessorInfo( PiMpService, ProcNum, &ProcessorInfoBuffer);
	}
	return FALSE;
}

// MPSupportEnableProc
//
// Enables/Disables the specified processor
BOOLEAN
EFIAPI
MPSupportEnableProc(UINTN ProcNum, BOOLEAN Enable)
{
	// EFI_STATUS Status;
	// EFI_PROCESSOR_INFORMATION ProcessorInfoBuffer;

	if (ProcNum == gBspId && Enable == FALSE)
		return FALSE;

	if (ProcNum >= gNumProc)
		return FALSE;

	if (ProcNum >= MAX_CPUS)
		return FALSE;

	if (PiMpService)
	{
		if (Enable != gProcEn[ProcNum])
		{
			if (gProcEn[ProcNum] == FALSE)
				gNumProcEn++;
			else
				gNumProcEn--;
		}
		gProcEn[ProcNum] = Enable != FALSE;
		return TRUE;
	}
	else
		return FALSE;
}

// MPSupportResetProc
//
// Reset the specified processor
EFI_STATUS
EFIAPI
MPSupportResetProc(UINTN ProcNum)
{
	EFI_STATUS Status;
	UINT32 HealthFlag = PROCESSOR_HEALTH_STATUS_BIT;

	if (PiMpService)
	{
		Status = PiMpService->EnableDisableAP(PiMpService, ProcNum, FALSE, &HealthFlag);

		if (EFI_ERROR(Status)) {
			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportResetProc - Unable to disable processor %d: %r", (UINT32)ProcNum, Status);
			MtSupportDebugWriteLine(gBuffer);
		}

		Status = PiMpService->EnableDisableAP(PiMpService, ProcNum, TRUE, &HealthFlag);

		if (EFI_ERROR(Status)) {
			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportResetProc - Unable to enable processor %d: %r", (UINT32)ProcNum, Status);
			MtSupportDebugWriteLine(gBuffer);
		}

	}
	else
		return EFI_NOT_FOUND;

	return Status;
}

// MPSupportGetBspId
//
// Return the processor number of the BSP
UINTN
EFIAPI
MPSupportGetBspId()
{
	if (PiMpService)
		return gBspId;

	return 0;
}

// MPSupportSwitchBSP
//
// Switch the specified processor to be the new BSP
EFI_STATUS
EFIAPI
MPSupportSwitchBSP(UINTN ProcNum)
{
	EFI_STATUS Status;
	if (PiMpService)
		Status = PiMpService->SwitchBSP( PiMpService, ProcNum, TRUE);
	else
		return EFI_NOT_FOUND;

	if (Status == EFI_SUCCESS)
	{
		gBspId = ProcNum;
	}
	else
	{
		AsciiSPrint(gBuffer, BUF_SIZE, "Unable to switch BSP to Proc #%d: %r", ProcNum, Status);
		MtSupportDebugWriteLine(gBuffer);
	}
	return Status;
}

EFI_STATUS
EFIAPI
MPSupportSwitchPkg(IN UINTN Pkg)
{
	EFI_STATUS Status;

	if (Pkg >= gNumPkg)
		return EFI_NOT_FOUND;

	UINTN ProcNum = gPkgProcMap[Pkg];

	if (MPSupportGetBspId() == ProcNum)
		return EFI_SUCCESS;

	if (PiMpService)
		Status = PiMpService->SwitchBSP(PiMpService, ProcNum, TRUE);
	else
		return EFI_NOT_FOUND;

	if (Status == EFI_SUCCESS)
	{
		gBspId = ProcNum;
	}
	else
	{
		AsciiSPrint(gBuffer, BUF_SIZE, "Unable to switch BSP to Proc #%d: %r", ProcNum, Status);
		MtSupportDebugWriteLine(gBuffer);
	}
	return Status;
}

// MPSupportGetProcessorInfo
//
// Get the processor info of the specified processor
EFI_STATUS
EFIAPI
MPSupportGetProcessorInfo(
	IN UINTN ProcNum, 
	OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer)
{
	EFI_STATUS Status;
	// Retrieve detailed health, status, and location info about the target processor
	if (PiMpService)
		Status = PiMpService->GetProcessorInfo(PiMpService, ProcNum, ProcessorInfoBuffer);
	else
		return EFI_NOT_FOUND;

	if (EFI_ERROR(Status)) {
		AsciiSPrint(gBuffer, BUF_SIZE, "Unable to get information for processor %d: %r", (UINT32)ProcNum, Status);
		MtSupportDebugWriteLine(gBuffer);
	}

	return Status;
}

// MPSupportInit
//
// Initialize the MPSupport library. Enumerate all processors and determine their status
EFI_STATUS
EFIAPI
MPSupportInit(UINTN MaxCPUs)
{
  EFI_STATUS                Status;
  UINTN						NumEnabledProcs = 0;
  UINTN						NumHT = 0;
  UINTN						i;

  gNumProc = 1;
  gNumProcEn = 1;
  SetMem(gProcEn, sizeof(gProcEn), 0);
  SetMem(gProcHT, sizeof(gProcHT), 0);
  SetMem(gPkgProcMap, sizeof(gPkgProcMap), 0);
  gNumPkg = 1;
  gBspId = 0;

  if (MaxCPUs > MAX_CPUS)
	  MaxCPUs = MAX_CPUS;

  do
  {
    // Find the PI MpService protocol
    Status = gBS->LocateProtocol(&gEfiMpServiceProtocolGuid, NULL, (VOID **)&PiMpService);
    if (EFI_ERROR(Status)) {
	  AsciiSPrint(gBuffer, BUF_SIZE, "Unable to locate the PI MpServices procotol: %r", Status);
	  MtSupportDebugWriteLine(gBuffer);
	  break;
    }
	else {
		MtSupportDebugWriteLine("Successfully located the PI MpService protocol.");
	}
	
	// PI: I am running on the BSP.  Find out my logical processor number.
	if (PiMpService)
		Status = PiMpService->WhoAmI( PiMpService, &gBspId);

    if (EFI_ERROR(Status)) {
	  AsciiSPrint(gBuffer, BUF_SIZE, "WhoAmI failed: %r. Setting BSP to 0.", Status);
	  MtSupportDebugWriteLine(gBuffer);
	  gBspId = 0;
    }

	AsciiSPrint(gBuffer, BUF_SIZE, "BSP is Proc %d", gBspId);
	MtSupportDebugWriteLine(gBuffer);

    // Get the total number and number of enabled processors
	if (PiMpService)
		Status = PiMpService->GetNumberOfProcessors( PiMpService, &gNumProc, &NumEnabledProcs);

    if (EFI_ERROR(Status)) {
      AsciiSPrint(gBuffer, BUF_SIZE, "Unable to get the number of processors: %r", Status);
	  MtSupportDebugWriteLine(gBuffer);
    }

    AsciiSPrint(gBuffer, BUF_SIZE, "This platform has %d logical processors of which %d are enabled.",
          (UINT32)gNumProc, (UINT32)NumEnabledProcs);
	MtSupportDebugWriteLine(gBuffer);
    
	if (gBspId >= gNumProc)
	{
		AsciiSPrint(gBuffer, BUF_SIZE, "Warning: BSP ID (%d) is greater than the number of processors (%d). Setting BSP ID to 0.", gBspId, gNumProc);
		gBspId = 0;
	}

	MtSupportDebugWriteLine(" ProcID   Enabled   Type   Status   Pkg   Core  Thread  Hyperthread?");
	MtSupportDebugWriteLine("--------  --------  ----  -------- -----  ----  ------  ------------");

	gNumProcEn = 0;
	for (i = 0; i < gNumProc; i++)
	{
		EFI_PROCESSOR_INFORMATION				ProcessorInfoBuffer;		

		SetMem(&ProcessorInfoBuffer, sizeof(ProcessorInfoBuffer), 0);

		// Retrieve detailed health, status, and location info about the target processor
		AsciiFPrint(DEBUG_FILE_HANDLE, "Get Proc Info Proc #%d", i);
		if (PiMpService)
			Status = PiMpService->GetProcessorInfo( PiMpService, i, &ProcessorInfoBuffer);			

		if (EFI_ERROR(Status)) {
			AsciiSPrint(gBuffer, BUF_SIZE, "Unable to get information for processor %d: %r", (UINT32)i, Status);
			MtSupportDebugWriteLine(gBuffer);
			continue;
		}
		
		// Check if processor is hyperthread
		BOOLEAN IsHT = i != gBspId &&  ProcessorInfoBuffer.Location.Thread == 1;

		if (IsHT)
			NumHT++;

		if (i < MaxCPUs)
		{
			// Enable the target processor
			if (i != gBspId && (ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT) == 0)
			{
				AsciiFPrint(DEBUG_FILE_HANDLE, "Proc %d is disabled. Enabling...", i);
			
				if (PiMpService)
					Status = PiMpService->EnableDisableAP(PiMpService, i, TRUE, NULL);

				if (EFI_ERROR(Status)) {
					AsciiSPrint(gBuffer, BUF_SIZE, "Unable to enable processor %d: %r", (UINT32)i, Status);
					MtSupportDebugWriteLine(gBuffer);
				}

				AsciiFPrint(DEBUG_FILE_HANDLE, "Get Proc Info Proc #%d", i);
				if (PiMpService)
					Status = PiMpService->GetProcessorInfo( PiMpService, i, &ProcessorInfoBuffer);			

				if (EFI_ERROR(Status)) {
					AsciiSPrint(gBuffer, BUF_SIZE, "Unable to get information for processor %d: %r", (UINT32)i, Status);
					MtSupportDebugWriteLine(gBuffer);
					continue;
				}
			}


			gProcEn[i] = (i == gBspId) || (ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT);

			if (IsHT)
				gProcHT[i] = TRUE;				

			if (gProcEn[i])
				gNumProcEn++;
		}		

		if (ProcessorInfoBuffer.Location.Package > 0 && ProcessorInfoBuffer.Location.Package < MAX_PKGS)
		{
			if (gPkgProcMap[ProcessorInfoBuffer.Location.Package] == 0)
			{
				gPkgProcMap[ProcessorInfoBuffer.Location.Package] = i;
				gNumPkg = ProcessorInfoBuffer.Location.Package + 1;
			}
		}
		// Display Processor Info Buffer
		AsciiSPrint(gBuffer, BUF_SIZE, "%08Lx      %s      %3s  %08x  %4d    %2d      %d      %s",
		  ProcessorInfoBuffer.ProcessorId,
		  (ProcessorInfoBuffer.StatusFlag & PROCESSOR_ENABLED_BIT) ? L"Y"   : L"N",
		  (ProcessorInfoBuffer.StatusFlag & PROCESSOR_AS_BSP_BIT)  ? L"BSP" : L" AP",
		  ProcessorInfoBuffer.StatusFlag,
		  ProcessorInfoBuffer.Location.Package,
		  ProcessorInfoBuffer.Location.Core,
		  ProcessorInfoBuffer.Location.Thread,
		  (ProcessorInfoBuffer.Location.Thread == 1) ? L"Y" : L"N" );
		MtSupportDebugWriteLine(gBuffer);
	}

	AsciiFPrint(DEBUG_FILE_HANDLE, "Package    ProcNum");
	for (i = 0; i < gNumPkg; i++)
	{
		AsciiFPrint(DEBUG_FILE_HANDLE, "%7d    %7d", i, gPkgProcMap[i]);
	}

	// Refresh the total number and number of enabled processors
	if (PiMpService)
		Status = PiMpService->GetNumberOfProcessors( PiMpService, &gNumProc, &NumEnabledProcs);

    if (EFI_ERROR(Status)) {
      AsciiSPrint(gBuffer, BUF_SIZE, "Unable to get the number of processors: %r", Status);
	  MtSupportDebugWriteLine(gBuffer);
      break;
    }

	AsciiSPrint(gBuffer, BUF_SIZE, "This platform has %d logical processors of which %d are enabled.",
		(UINT32)gNumProc, (UINT32)NumEnabledProcs);
	MtSupportDebugWriteLine(gBuffer);

	if (gNumProc > MaxCPUs)
	{
		AsciiSPrint(gBuffer, BUF_SIZE, "The number of cores (%d) is greater than the maximum supported (%d). Limiting the number of cores to %d.",
			(UINT32)gNumProc, (UINT32)MaxCPUs, MaxCPUs);
		MtSupportDebugWriteLine(gBuffer);
	}

	UINTN MaxAvailCPU = gNumProc < MaxCPUs ? gNumProc : MaxCPUs;

	AsciiSPrint(gBuffer, BUF_SIZE, "Number of hyperthreads detected: %d (Total threads: %d)",
		NumHT, MaxAvailCPU);
	MtSupportDebugWriteLine(gBuffer);


	if (NumHT <= (MaxAvailCPU >> 1)) // the number of hyperthreads should be less or equal to half the number of processors
	{
		if (gEnableHT == FALSE)
		{
			AsciiSPrint(gBuffer, BUF_SIZE, "Disabling all hyperthreads");
			MtSupportDebugWriteLine(gBuffer);

			for (i = 0; i < MaxAvailCPU; i++)
			{
				if (gProcEn[i] && gProcHT[i])
				{
					AsciiSPrint(gBuffer, BUF_SIZE, "Disabling hyperthread processor %d", i);
					MtSupportDebugWriteLine(gBuffer);

					gProcEn[i] = FALSE;
					gNumProcEn--;
				}
			}
		}		
	}

	if (gNumProcEn == 0)
	{
		gNumProcEn = 1;
		gProcEn[gBspId] = TRUE;
	}
  } while( FALSE );

  if (PiMpService)
	return Status;

  return EFI_UNSUPPORTED;
}

// MPSupportGetTimeInMilliseconds
//
// Return the elapsed time in milliseconds
__inline UINT64
EFIAPI
MPSupportGetTimeInMilliseconds ( UINT64 StartTime )
{
	UINT64 t;

	/* We can't do the elapsed time unless the rdtsc instruction
	 * is supported
	 */ 

	if (MtSupportReadTSC() >= StartTime)
		t= MtSupportReadTSC() - StartTime;
	else // overflow?
		t= ((UINT64)-1) - (StartTime - MtSupportReadTSC() + 1);

	t = DivU64x64Remainder(t, clks_msec, NULL);

	return t;
}

// MPSupportTestMPServices
//
// For testing EFI firmware multiprocessor services (eg. for debugging). Not used in Memtest86 directly
BOOLEAN
EFIAPI
MPSupportTestMPServices()
{
	EFI_STATUS Status;
	UINTN ProcNum;

	EFI_EVENT		APEvent[MAX_CPUS];
	BOOLEAN			APFinished[MAX_CPUS];
	UINT64			APWaitTime[MAX_CPUS];

	UINT64          StartTime;
	UINT64			CheckPointTime = 0;

	BOOLEAN			bAPFinished = TRUE; 
	BOOLEAN			bMPTestPassed = TRUE;

#if 0
	UINTN OldBSP = MPSupportGetBspId();
	AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - BSP switching test");
	MtSupportDebugWriteLine(gBuffer);

	for (ProcNum = 0; ProcNum < gNumProc; ProcNum++)
	{
		if (ProcNum != MPSupportGetBspId() &&
			MPSupportIsProcEnabled(ProcNum))
		{
			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Switch BSP to Proc#%d", ProcNum);
			MtSupportDebugWriteLine(gBuffer);

			Status = MPSupportSwitchBSP(ProcNum);
			if (EFI_ERROR(Status))
			{
				AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Could not switch BSP to Proc#%d (%r)", ProcNum, Status);
				MtSupportDebugWriteLine(gBuffer);
			}

			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Enabling memory cache");
			MtSupportDebugWriteLine(gBuffer);

			AsmEnableCache();

			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Intializing random library");
			MtSupportDebugWriteLine(gBuffer);
			// Set the random seed
			rand_init((int)gNumProc);
			
			{
				int i;
				UINT64 sum = 0;
				unsigned int seed = (unsigned int)(MtSupportReadTSC() & 0xffffffffull);

				AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Setting random seed to 0x%08X", seed);
				MtSupportDebugWriteLine(gBuffer);

				for (i = 0; i < (int)gNumProc; i++)
					rand_seed(seed,i);

				for (i = 0; i < 1000000000; i++)
					sum += rand((int)ProcNum);
			}

			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Cleaning up random library");
			MtSupportDebugWriteLine(gBuffer);

			// Cleanup rand lib
			rand_cleanup();
		}
	}

	AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Restoring BSP to Proc#%d", OldBSP);
	MtSupportDebugWriteLine(gBuffer);

	Status = MPSupportSwitchBSP(OldBSP);
	if (EFI_ERROR(Status))
	{
		AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Could not switch BSP to Proc#%d (%r)", OldBSP, Status);
		MtSupportDebugWriteLine(gBuffer);
	}

	AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - BSP switching test complete");
	MtSupportDebugWriteLine(gBuffer);
	
#endif

	SetMem(APEvent, sizeof(APEvent), 0);
	SetMem(APFinished, sizeof(APFinished), 0);
	SetMem(APWaitTime, sizeof(APWaitTime), 0);

	AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - AP dispatch test");
	MtSupportDebugWriteLine(gBuffer);
	
	// Set the random seed
	rand_init((int)MPSupportGetMaxProcessors());

	for (ProcNum = 0; ProcNum < MPSupportGetMaxProcessors(); ProcNum++)
	{
		if (ProcNum != MPSupportGetBspId() &&
			MPSupportIsProcEnabled(ProcNum))
		{
			AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Starting AP#%d", ProcNum);
			MtSupportDebugWriteLine(gBuffer);

			Status = MPSupportTestAP( ProcNum, &APFinished[ProcNum], &APEvent[ProcNum] );
			if (Status != EFI_SUCCESS)
			{
				bMPTestPassed = FALSE;

				AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Could not start AP#%d (%r).", ProcNum, Status);
				MtSupportDebugWriteLine(gBuffer);

				break;
			}
		}
	}

	StartTime = MtSupportReadTSC();

	AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - Waiting for AP's to complete execution");
	MtSupportDebugWriteLine(gBuffer);

	// Now check the result
	CheckPointTime = 1000;
	do {
		bAPFinished = TRUE;

		for (ProcNum = 0; ProcNum < MPSupportGetMaxProcessors(); ProcNum++)
		{
			if (APEvent[ProcNum])
			{
				Status = gBS->CheckEvent(APEvent[ProcNum]);
							  
				if ( Status == EFI_NOT_READY) // CPU finished event has not been signalled
				{
					if (APFinished[ProcNum] != FALSE) // CPU has finished the memory test, event should be signalled so check again next iteration
					{
						if (APWaitTime[ProcNum] == 0)
							APWaitTime[ProcNum] = MPSupportGetTimeInMilliseconds(StartTime);

						if ((MPSupportGetTimeInMilliseconds(StartTime) - APWaitTime[ProcNum]) > 1000) // Event still hasn't been signalled after 1 second
						{
							bMPTestPassed = FALSE;

							AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - CPU #%d completed but did not signal (event wait time = %ldms) ", ProcNum, MPSupportGetTimeInMilliseconds(StartTime) - APWaitTime[ProcNum]);
							MtSupportDebugWriteLine(gBuffer);

							gBS->CloseEvent(APEvent[ProcNum]);
							APEvent[ProcNum] = NULL;
						}
						else // Finished, but event hasn't been triggered. Wait at most 1 second to be triggered
						{
							bAPFinished = FALSE;
						}
					}
					else // CPU has not finished the memory test
					{
						if (MPSupportGetTimeInMilliseconds(StartTime) > CheckPointTime)
						{
							AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - waited for %lds for CPU #%d to finish", DivU64x32(MPSupportGetTimeInMilliseconds(StartTime), 1000), ProcNum);
							MtSupportDebugWriteLine(gBuffer);
						}

						bAPFinished = FALSE;
					}
				}		
				else if ( Status == EFI_SUCCESS) // CPU finished event has been signalled
				{
					if (APFinished[ProcNum] == FALSE) // CPU has not finished the memory test, maybe timed out or serious error?
					{
						bMPTestPassed = FALSE;

						AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - CPU #%d timed out", ProcNum);
						MtSupportDebugWriteLine(gBuffer);
					}
					AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - CPU #%d finished execution", ProcNum);
					MtSupportDebugWriteLine(gBuffer);

					gBS->CloseEvent(APEvent[ProcNum]);
					APEvent[ProcNum] = NULL;
				}
			}
		}

		if (MPSupportGetTimeInMilliseconds(StartTime) > CheckPointTime)
			CheckPointTime += 1000;

		// If wait time is more than 30 seconds and still not finished, abort
		if (bAPFinished == FALSE && MPSupportGetTimeInMilliseconds(StartTime) >= 3000)
		{
			bMPTestPassed = FALSE;
			bAPFinished = TRUE;
			// Clean up event handle
			for (ProcNum = 0; ProcNum < MPSupportGetMaxProcessors(); ProcNum++)
			{
				if (APEvent[ProcNum])
				{
					gBS->CloseEvent(APEvent[ProcNum]);
					APEvent[ProcNum] = NULL;

					AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - CPU #%d did not finish", ProcNum);
					MtSupportDebugWriteLine(gBuffer);
				}
			}
		}
	} while (bAPFinished == FALSE);

	// Cleanup rand lib
	rand_cleanup();

	AsciiSPrint(gBuffer, BUF_SIZE, "MPSupportTestMPServices - AP dispatch test complete");
	MtSupportDebugWriteLine(gBuffer);

	return bMPTestPassed;
}


// barrier_init
//
// Initialize the barrier structure for synchronization
void barrier_init(struct barrier_s *barr, int max)
{
	InitializeSpinLock(&barr->lck);
	InitializeSpinLock(&barr->mutex);
    barr->maxproc = max;
    barr->count = max;
	InitializeSpinLock(&barr->st1);
	InitializeSpinLock(&barr->st2);
	AcquireSpinLock(&barr->st2);
        // barr->st1.slock = 1;
        // barr->st2.slock = 0;
}

// barrier
//
// Synchronize all processors to the same point in code
void barrier(struct barrier_s *barr)
{
	if (barr->maxproc == 1) {
		return;
	}
	AcquireSpinLock(&barr->st1);
	ReleaseSpinLock(&barr->st1);
	// spin_wait(&barr->st1);     /* Wait if the barrier is active */
        AcquireSpinLock(&barr->lck);	   /* Get lock for barr struct */
        if (--barr->count == 0) {  /* Last process? */
				AcquireSpinLock(&barr->st1);
				ReleaseSpinLock(&barr->st2);
				// barr->st1.slock = 0;   /* Hold up any processes re-entering */
				// barr->st2.slock = 1;   /* Release the other processes */
                barr->count++;
                ReleaseSpinLock(&barr->lck); 
        } else {
				ReleaseSpinLock(&barr->lck);
				AcquireSpinLock(&barr->st2);
				ReleaseSpinLock(&barr->st2);
				// spin_wait(&barr->st2);	/* wait for peers to arrive */
                AcquireSpinLock(&barr->lck);   
                if (++barr->count == barr->maxproc) { 
					ReleaseSpinLock(&barr->st1);
					AcquireSpinLock(&barr->st2);
#if 0
                        barr->st1.slock = 1; 
                        barr->st2.slock = 0; 
#endif
                }
                ReleaseSpinLock(&barr->lck); 
        }
}
