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
//	Ui.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	UI functions for MemTest86
//
//  Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
//
//References:
//  https://github.com/jljusten/MemTestPkg
//
//History
//	27 Feb 2013: Initial version

/** @file
  User interface functions for Mem Test application

  Copyright (c) 2006 - 2009, Intel Corporation
  Copyright (c) 2009, Jordan Justen
  All rights reserved.

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.  The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS"
  BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER
  EXPRESS OR IMPLIED.

**/

/* init.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemTestUiLib.h>
#include <Library/MemTestSupportLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/SynchronizationLib.h>
#include <Library/HiiLib.h>
#include <Library/IoLib.h>
#include <Protocol/HiiFont.h>
#include<Protocol/UgaDraw.h>
#include<Protocol/GraphicsOutput.h>

#include <Library/Cpuid.h>

#include <Library/libeg/libeg.h>

#include <SysInfoLib/pci.h>
#include <SysInfoLib/smbus.h>
#include <SysInfoLib/cpuinfo.h>
#include <SysInfoLib/cputemp.h>
#include <SysInfoLib/SMBIOS.h>
#include <Library/MPSupportLib.h>

#include <uMemTest86.h>

#define LINE_INFO	11							// the y-coordinates of the elapsed time/pass #/error count
#define COL_MID		30							// the x-coordinates of the divider line
#define BAR_SIZE	(MaxUIWidth-2-COL_MID-15)	// the width of the progress bar

#define MAX_DISPLAYABLE_CPUS	31				// maximum number of CPU states to display due to screen space

// Piezo Speaker Related
#define EFI_SPEAKER_CONTROL_PORT       0x61
#define EFI_SPEAKER_OFF_MASK           0xFC
#define EFI_BEEP_ON_TIME_INTERVAL      0x50000
#define EFI_BEEP_OFF_TIME_INTERVAL     0x50000

//
// Global variables
//
UINTN				gLastPercent = 0;
STATIC UINT64		mProgressTotal;				// maximum progress value
STATIC UINTN		mLastPercent;				// last progress %
STATIC CHAR16		mTestName[64];				// current test name
STATIC UINT64		mStartAddr;					// current memory range start
STATIC UINT64		mEndAddr;					// current memory range end
struct PAT_UI {
	UINT32 CurPattern[4];
	UINT32 NewPattern[4];
	UINT8 CurSize;
	UINT8 NewSize;
} mPatternUI;                                  // current pattern

#define NUM_RUN_STATES			4
CONST STATIC CHAR16 CPU_RUN_STATE[NUM_RUN_STATES] = { L'-', L'\\', L'|', L'/' };

STATIC UINT8		mCPURunState[MAX_CPUS];		// the current cursor character for the CPU

#define MAX_LINES				11
#define LINE_BUF_SIZE			128
#define MAX_LINE_LEN			(LINE_BUF_SIZE - 1)
#define START_LINE_NUM			13
#define END_LINE_NUM			(START_LINE_NUM + MAX_LINES - 1)

static int			mCurLineNo = -1;			// next line number to print the text to display under the test status
STATIC CHAR16		mLineBuf[MAX_LINES][LINE_BUF_SIZE];			// line buffer for storing the text to display under the test status
STATIC UINT8		mLineColour[MAX_LINES];		// store the colour of each line

// console fg/bg colours
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL mEfiColors[16] = {
  { 0x00, 0x00, 0x00, 0x00 },
  { 0x98, 0x00, 0x00, 0x00 },
  { 0x00, 0x98, 0x00, 0x00 },
  { 0x98, 0x98, 0x00, 0x00 },
  { 0x00, 0x00, 0x98, 0x00 },
  { 0x98, 0x00, 0x98, 0x00 },
  { 0x00, 0x98, 0x98, 0x00 },
  { 0x98, 0x98, 0x98, 0x00 },
  { 0x10, 0x10, 0x10, 0x00 },
  { 0xff, 0x10, 0x10, 0x00 },
  { 0x10, 0xff, 0x10, 0x00 },
  { 0xff, 0xff, 0x10, 0x00 },
  { 0x10, 0x10, 0xff, 0x00 },
  { 0xf0, 0x10, 0xff, 0x00 },
  { 0x10, 0xff, 0xff, 0x00 },
  { 0xff, 0xff, 0xff, 0x00 }
};

/* Hardware details extern */
extern struct cpu_ident cpu_id;
extern CPUINFO gCPUInfo;
//extern int l1_cache, l2_cache, l3_cache;
extern UINT64 memsize;

extern UINTN cpu_speed;
extern UINTN mem_speed;
extern UINTN l1_speed;
extern UINTN l2_speed;
extern UINTN l3_speed;

extern CPUMSRINFO cpu_msr;

extern SPDINFO				*g_MemoryInfo;		//Var to hold info from smbus on memory
extern int					g_numMemModules;
extern int					g_numSMBIOSMem;
extern int					g_numSMBIOSModules; // Num of SMBIOS memory devices occupied

extern SMBIOS_MEMORY_DEVICE *g_SMBIOSMemDevices;

extern UINTN				gNumPasses; // number of passes of the test sequence to perform

extern EG_IMAGE         *Banner;
extern UINTN            ScreenHeight;
extern UINTN            ScreenWidth;
extern UINTN            ConHeight;
extern UINTN            ConWidth;
extern UINTN			MaxUIWidth;
extern UINTN            gCharWidth;
extern UINTN            gTextHeight;
extern UINTN			gBgColour;					// The background colour to use for the screen

extern EFI_GRAPHICS_OUTPUT_BLT_PIXEL DEFAULT_BG_PIXEL;			// default bg colour
extern EFI_GRAPHICS_OUTPUT_BLT_PIXEL DEFAULT_SEL_PIXEL;			// default bg colour for selected items
extern EFI_GRAPHICS_OUTPUT_BLT_PIXEL DEFAULT_TEXT_PIXEL;		// default text colour

extern BOOL				gTextDirectToScreen;

extern UINTN
UefiLibGetStringWidth(
    IN  CHAR16* String,
    IN  BOOLEAN              LimitLen,
    IN  UINTN                MaxWidth,
    OUT UINTN* Offset
);

// cpu_type
//
// Get the CPU type string
void cpu_type(CHAR16 *szwCPUType, UINTN bufSize);

// MtUiDrawAddressRange
//
// Print the current address range
VOID
MtUiDrawAddressRange (  );

// MtUiDrawProgress
//
// Draw the progress bra
VOID
EFIAPI
MtUiDrawProgress (
  );

// ImageDrawTextXY
//
// (Graphics mode) Render the specified text to the image using libeg using the specified formatting parameters
INTN 
EFIAPI
ImageDrawTextXY(IN EG_IMAGE *Image, IN CHAR16 *Text, IN EG_PIXEL *BgColour, IN INTN XPos, IN INTN YPos, IN UINT8 XAlign, UINT8 Brightness );

// DrawTextXY
//
// (Graphics mode) Render the specified text to screen using libeg using the specified formatting parameters
INTN 
EFIAPI
DrawTextXY(IN CHAR16 *Text, IN EG_PIXEL *BgColour, IN INTN XPos, IN INTN YPos, IN UINT8 XAlign);

//
// Define the maximum message length that this library supports
//
#define MAX_MESSAGE_LENGTH  0x100


VOID
MtUiSetTestName (
  IN CHAR16        *TestName
  )
{
	UINTN TestNameLen = 0;
	UINTN OriginalAttribute;
	CHAR16 TextBuf[128];

	OriginalAttribute = gST->ConOut->Mode->Attribute;

	if (TestName)
		StrCpyS(mTestName, sizeof(mTestName) / sizeof(mTestName[0]), TestName);

	gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID, 3);
	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));

	StrCpyS(TextBuf, sizeof(TextBuf) / sizeof(TextBuf[0]), mTestName);

	// Truncate with elipsis '...' if test name too long
	while (UnicodeStringDisplayLength(TextBuf) > (MaxUIWidth - 2 - COL_MID))
	{
		int LastIdx = (int)StrLen(TextBuf) - 1; 
		TextBuf[LastIdx] = L'\0';
		TextBuf[LastIdx - 1] = L'.';
		TextBuf[LastIdx - 2] = L'.';
		TextBuf[LastIdx - 3] = L'.';
	}
	gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID, 3);
	TestNameLen = Print(L"%s", TextBuf);
	gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID + UnicodeStringDisplayLength(TextBuf), 3);
	
	// Clear any existing text
	int NumSpaces = (int)MaxUIWidth - 2 - gST->ConOut->Mode->CursorColumn;
	if (NumSpaces > 0)
	{
		SetMem16(TextBuf, NumSpaces * sizeof(TextBuf[0]), L' ');
		TextBuf[NumSpaces] = L'\0';
		Print(TextBuf);
	}	

	gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);
}

VOID
MtUiSetPattern (
  IN UINTN         Pattern
  )
{
  CopyMem(mPatternUI.NewPattern, &Pattern, sizeof(Pattern));
  if (sizeof(void*) > 4 && Pattern & 0xFFFFFFFF00000000ULL)
	  mPatternUI.NewSize = 8;
  else
	  mPatternUI.NewSize = 4;
}

VOID
MtUiSetPattern64 (
  IN UINT64         Pattern
  )
{
  CopyMem(mPatternUI.NewPattern, &Pattern, sizeof(Pattern));
  mPatternUI.NewSize = sizeof(Pattern);
}

VOID
MtUiSetPattern128 (
  IN UINT32         Pattern[4]
  )
{
  CopyMem(mPatternUI.NewPattern, Pattern, 16);
  mPatternUI.NewSize = 16;
}

VOID
MtUiUpdatePattern(BOOLEAN Redraw)
{
	CHAR16 TempBuf[128];

	// Force redraw even if the pattern is the same
	if (Redraw)
	{
		mPatternUI.CurSize = 0;
		SetMem(mPatternUI.CurPattern, sizeof(mPatternUI.CurPattern), 0);
	}

	// Don't redraw if the pattern hasn't changed
	if (mPatternUI.CurSize == mPatternUI.NewSize && CompareMem(mPatternUI.CurPattern, mPatternUI.NewPattern, mPatternUI.CurSize) == 0)
		return;

	gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID, 5);
	Print(L"%s", GetStringById(STRING_TOKEN(STR_PATTERN), TempBuf, sizeof(TempBuf)));
	gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID + 10, 5);

	switch (mPatternUI.NewSize)
	{
	case 4:
		Print(L": 0x%08X        ", mPatternUI.NewPattern[0]);
		break;
	case 8:
		Print(L": 0x%016lX", *((UINT64 *)mPatternUI.NewPattern));
		break;
	case 16:
		Print(L": 0x%08X%04X....", mPatternUI.NewPattern[0], mPatternUI.NewPattern[1] & 0xFFFF);
		break;
	}

	mPatternUI.CurSize = mPatternUI.NewSize;
	CopyMem(mPatternUI.CurPattern, mPatternUI.NewPattern, sizeof(mPatternUI.CurPattern));
}

VOID
MtUiSetAddressRange (
  IN UINT64	       Start,
  IN UINT64	       End
  )
{
  mStartAddr = Start;
  mEndAddr = End;
  MtUiDrawAddressRange();
}

VOID
MtUiDrawAddressRange (  )
{
  CHAR16 TempBuf[128];
  UINTN i;

  gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID, 4);
  Print (L"%s", GetStringById (STRING_TOKEN(STR_ADDRESS), TempBuf, sizeof(TempBuf)));

  gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID + 10, 4);
  if (mStartAddr != 0 || mEndAddr != 0)
	  Print (L": 0x%lx - 0x%lx", mStartAddr, mEndAddr);
  else
	  Print (L":");

  TempBuf[0] = 0;
  for (i = gST->ConOut->Mode->CursorColumn; i < MaxUIWidth-2; i++)
	  StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]),L" ");

  Print (TempBuf);

}

VOID
EFIAPI
MtUiSetTestsCompleted (
  IN UINTN        NumCompleted,
  IN UINTN        NumTest
  )
{
  UINTN Percent;

  UINTN i;
  UINTN numticks;
  
  UINTN OriginalAttribute;  
  CHAR16 TempBuf[128];

  OriginalAttribute = gST->ConOut->Mode->Attribute;

  if (NumCompleted > NumTest)
  {
	  AsciiSPrint(gBuffer, BUF_SIZE, "Warning - attempting to set tests completed (%d) higher than the number of tests (%d)", NumCompleted, NumTest);
	  MtSupportDebugWriteLine(gBuffer);
	  NumCompleted = NumTest;
  }

  if (NumTest == 0)
  {
	  Percent = 0;
	  numticks = 0;
  }
  else
  {
	  Percent = 
		(UINTN) DivU64x64Remainder (
				  MultU64x32 (NumCompleted, 100),
				  NumTest,
				  NULL
				  );
	  numticks = BAR_SIZE * NumCompleted/NumTest;
  }

  
  gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID, 1);
  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_GREEN, ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)));
  Print(GetStringById(STRING_TOKEN(STR_PASSES), TempBuf, sizeof(TempBuf)));
  gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID + 7, 1);
  Print(L"%3d%% ", Percent);

  for (i = 0; i < BAR_SIZE; i++)
  {
	if ( i < numticks )
	Print(L"#");
	else
	Print(L" ");
  }
  gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);
}

VOID
EFIAPI
MtUiSetIterationCount (
  IN UINTN        IterCount
  )
{
  gST->ConOut->SetCursorPosition(gST->ConOut, 29, LINE_INFO);
  Print (L"%4d", IterCount);
}

VOID
EFIAPI
MtUiSetPassNo (
  IN UINTN        Pass
  )
{
	CHAR16 TempBuf[16];
	ConsolePrintXY(41, LINE_INFO, L"%s: %d / %d", GetStringById(STRING_TOKEN(STR_PASSES), TempBuf, sizeof(TempBuf)), Pass, gNumPasses);
}

VOID
EFIAPI
MtUiSetErrorCount (
  IN UINTN        NumError
  )
{
  UINTN OriginalAttribute;  
  CHAR16 TempBuf[128];

  OriginalAttribute = gST->ConOut->Mode->Attribute;

  if (NumError > 0)
	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTRED, ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)));

  gST->ConOut->SetCursorPosition(gST->ConOut, 63, LINE_INFO);
  Print (L"%s:", GetStringById(STRING_TOKEN(STR_ERRORS), TempBuf, sizeof(TempBuf)));
  gST->ConOut->SetCursorPosition(gST->ConOut, 70, LINE_INFO);
  Print(L"%5d", NumError);
  gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);
}

VOID
EFIAPI
MtUiPrintReset()
{
	mCurLineNo = -1;
	SetMem(mLineBuf,sizeof(mLineBuf),0);
	SetMem(mLineColour, sizeof(mLineColour), gST->ConOut->Mode->Attribute & 0x0F);
}

VOID
EFIAPI
MtUiRePrint()
{
	int i;
	UINTN OriginalAttribute;

	OriginalAttribute = gST->ConOut->Mode->Attribute;

	for (i = 0; i < MAX_LINES; i++)
	{
	  // Clear line first
	  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(mLineColour[i], ((OriginalAttribute&(BIT4 | BIT5 | BIT6)) >> 4)));

	  gST->ConOut->SetCursorPosition(gST->ConOut, 0, START_LINE_NUM+i);
	  if (mCurLineNo == START_LINE_NUM+i)
		  gST->ConOut->OutputString(gST->ConOut, L">");
	  else
		  gST->ConOut->OutputString(gST->ConOut, L" ");
      gST->ConOut->OutputString (gST->ConOut, mLineBuf[i]);
	}

	gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);
}

UINTN
EFIAPI
MtUiPrint (
  IN UINT8 Colour,
  IN CONST CHAR16  *Format,
  ...
  )
{
	CHAR16 Buffer[MAX_MESSAGE_LENGTH];
	VA_LIST Marker;
	UINTN Return;
	UINTN OriginalAttribute;

	OriginalAttribute = gST->ConOut->Mode->Attribute;

	//
	// If Format is NULL, then ASSERT().
	//
	ASSERT(Format != NULL);

	//
	// Convert the DEBUG() message to a Unicode String
	//
	VA_START(Marker, Format);
	Return = UnicodeVSPrint(Buffer, sizeof(Buffer), Format, Marker);
	VA_END(Marker);

	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(Colour, ((OriginalAttribute & (BIT4 | BIT5 | BIT6)) >> 4)));

	//
	// Send the print string to the Console Output device
	//
	if ((gST != NULL) && (gST->ConOut != NULL))
	{
		if (mCurLineNo >= START_LINE_NUM && mCurLineNo <= END_LINE_NUM)
		{
			gST->ConOut->SetCursorPosition(gST->ConOut, 0, mCurLineNo);
			gST->ConOut->OutputString(gST->ConOut, L" ");
		}

		if (mCurLineNo >= END_LINE_NUM || mCurLineNo < START_LINE_NUM)
			mCurLineNo = START_LINE_NUM;
		else
			mCurLineNo++;

		StrnCpyS(mLineBuf[mCurLineNo - START_LINE_NUM], ARRAY_SIZE(mLineBuf[mCurLineNo - START_LINE_NUM]), Buffer, MAX_LINE_LEN);
		mLineBuf[mCurLineNo - START_LINE_NUM][MAX_LINE_LEN] = L'\0';
		mLineColour[mCurLineNo - START_LINE_NUM] = Colour;

		gST->ConOut->SetCursorPosition(gST->ConOut, 0, mCurLineNo);
		gST->ConOut->OutputString(gST->ConOut, L">");
		gST->ConOut->OutputString(gST->ConOut, Buffer);

		gST->ConOut->SetCursorPosition(gST->ConOut, UnicodeStringDisplayLength(Buffer) + 1, mCurLineNo);

		// Pad the rest of the line wth spaces
		int NumSpaces = (int)MaxUIWidth - 2 - gST->ConOut->Mode->CursorColumn;
		if (NumSpaces > 0)
		{
			SetMem16(Buffer, NumSpaces * sizeof(Buffer[0]), L' ');
			Buffer[NumSpaces] = L'\0';
			Print(Buffer);
		}
	}

	gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);

	return Return;
}

#ifdef APDEBUG
CHAR16			mAPBuf[100][256];
UINT32			mAPBufHead;		// head of buffer
UINT32			mAPBufTail;		// tail of buffer
SPIN_LOCK		mAPBufLock;
INT32			mCheckPoint = 0;

UINTN
EFIAPI
MtUiAPPrint (
  IN CONST CHAR16  *Format,
  ...
  )
{
	CHAR16   Buffer[MAX_MESSAGE_LENGTH];
	VA_LIST  Marker;
	UINTN    Return;

	if (MPSupportWhoAmI() == MPSupportGetBspId())
		return 0;

	//
	// If Format is NULL, then ASSERT().
	//
	ASSERT (Format != NULL);

	//
	// Convert the DEBUG() message to a Unicode String
	//
	VA_START (Marker, Format);
	Return = UnicodeVSPrint (Buffer, sizeof(Buffer),  Format, Marker);
	VA_END (Marker);

	AcquireSpinLock (&mAPBufLock);
	UnicodeSPrint(mAPBuf[mAPBufTail], 256 * sizeof(CHAR16), Buffer);
	mAPBufTail = (mAPBufTail + 1) % 100;
	// StrCatS(mAPBuf, sizeof(mAPBuf) / sizeof(mAPBuf[0]), Buffer);
	ReleaseSpinLock(&mAPBufLock);

	return Return;
}


VOID
EFIAPI
MtUiAPInitBuf ( )
{
	InitializeSpinLock (&mAPBufLock);
	SetMem(mAPBuf, sizeof(mAPBuf), 0);
	mAPBufHead = mAPBufTail = 0;
}

VOID
EFIAPI
MtUiAPFlushBuf ( )
{
	if (mAPBufHead == mAPBufTail)
		return;

	// Go through the error circular buffer and display any new errors
	AcquireSpinLock (&mAPBufLock);
	for (; mAPBufHead != mAPBufTail; mAPBufHead = (mAPBufHead + 1) % 100)
	{
		MtSupportUCDebugWriteLine(mAPBuf[mAPBufHead]);
	}
	ReleaseSpinLock(&mAPBufLock);

#if 0
	AcquireSpinLock (&mAPBufLock);
	if (StrLen(mAPBuf) > 0)
	{
		// MtUiPrint(L"%s", mAPBuf);
		MtSupportUCDebugWriteLine(mAPBuf);
		SetMem(mAPBuf, sizeof(mAPBuf), 0);
	}
	ReleaseSpinLock(&mAPBufLock);
#endif
}
#endif

/**
  Indicates the total size of a pass through the memory test.
  
**/
VOID
EFIAPI
MtUiSetProgressTotal (
  IN UINT64   Total
  )
{
  if (Total == 0)
	  MtSupportDebugWriteLine("Warning - Setting total progress to 0");

  mLastPercent = (UINTN)-1;
  mProgressTotal = Total;
}


/**
  Allows the memory test to indicate progress to the UI library.

**/
VOID
EFIAPI
MtUiUpdateProgress (
  IN UINT64   Progress
  )
{
  UINTN Percent;

  if (Progress > mProgressTotal)
  {
	  AsciiSPrint(gBuffer, BUF_SIZE, "Warning - attempting to set progress (%ld) higher than total progress (%ld)", Progress, mProgressTotal);
	  MtSupportDebugWriteLine(gBuffer);
	  Progress = mProgressTotal;
  }

  if ( mProgressTotal == 0)
	  Percent = 0;
  else
  {
	  Percent = 
		(UINTN) DivU64x64Remainder (
				  MultU64x32 (Progress, 100),
				  mProgressTotal,
				  NULL
				  );
  }
  if ((Percent / 2) != (mLastPercent / 2)) {
	  mLastPercent = Percent;
	  MtUiDrawProgress(); 
  }
  mLastPercent = Percent;
  gLastPercent = Percent;
}

VOID
EFIAPI
MtUiDrawProgress (
  )
{
	UINTN i;
	UINTN OriginalAttribute = gST->ConOut->Mode->Attribute;
	UINTN numticks = BAR_SIZE * mLastPercent / 100;
	CHAR16 TempBuf[128];

	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BROWN, ((OriginalAttribute&(BIT4|BIT5|BIT6))>>4)));
	gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID, 2);
	Print(GetStringById(STRING_TOKEN(STR_TEST), TempBuf, sizeof(TempBuf)));
	gST->ConOut->SetCursorPosition(gST->ConOut, COL_MID + 7, 2);
	Print(L"%3d%% ", mLastPercent);
	 
	for (i = 0; i < BAR_SIZE; i++)
	{
		if ( i < numticks )
			Print(L"#");
		else
			Print(L" ");
	}
	gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);
}

VOID
EFIAPI
MtUiUpdateTime ( )
{
	UINT32 hours,mins,secs;
	UINT32 t;
	EFI_TIME start;
	EFI_TIME cur;

	MtSupportGetTestStartTime(&start);
	gRT->GetTime(&cur, NULL);

	t = efi_time(&cur) - efi_time(&start);

	if (t == 0) // something is wrong with the GetTime function
	{
		t = (UINT32)MtSupportGetTimeInMilliseconds() / 1000;
	}

	secs = t % 60;
	t /= 60;
	mins = t % 60;
	t /= 60;
	hours = t;

	gST->ConOut->SetCursorPosition(gST->ConOut, 7, LINE_INFO);
	Print(L"%4d:%02d:%02d", hours,mins,secs);	
}

VOID
EFIAPI
MtUiUpdateCPUState ( )
{
	UINTN i;
	for (i = 0; i < MPSupportGetNumProcessors() && i < MAX_DISPLAYABLE_CPUS; i++)
	{
		if (MtSupportCPURunning(i))
		{
			gST->ConOut->SetCursorPosition(gST->ConOut, 7+i, 9);
			Print(L"%c", CPU_RUN_STATE[mCPURunState[i]]);
			mCPURunState[i] = (mCPURunState[i] + 1) % NUM_RUN_STATES;			
		}
		else if (MPSupportIsProcEnabled(i))
		{
			ConsolePrintXY(7+i, 9, L"W");
		}
		else
		{
			ConsolePrintXY(7+i, 9, L"D");
		}
	}
	
}

VOID
EFIAPI
MtUiUpdateCPUTemp ( int iTemp )
{
	if (ABS(iTemp) < 200)
	{
		gST->ConOut->SetCursorPosition(gST->ConOut, 21, 1);
		Print(L" / %3dC", iTemp);
	}
	else
	{
		ConsolePrintXY(21, 1, L"       ");
	}
}

VOID
EFIAPI
MtUiUpdateRAMTemp( int iTemp )
{
	CHAR16 TextBuf[128];
	if (ABS(iTemp) < MAX_TSOD_TEMP)
	{
		ConsolePrintXY(72, 5, L"%dC", iTemp / 1000);
	}
	else
	{
		CHAR16 TempBuf[16];
		gST->ConOut->SetCursorPosition(gST->ConOut, 72, 5);
		Print(L"%s", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
		UINTN Col = 72 + UnicodeStringDisplayLength(TempBuf);
		if (Col >= MaxUIWidth)
			Col = MaxUIWidth - 1;
		gST->ConOut->SetCursorPosition(gST->ConOut, Col, 5);
	}

	int NumSpaces = (int)MaxUIWidth - 2 - gST->ConOut->Mode->CursorColumn;
	if (NumSpaces > 0)
	{
		SetMem16(TextBuf, NumSpaces * sizeof(TextBuf[0]), L' ');
		TextBuf[NumSpaces] = L'\0';
		Print(TextBuf);
	}	
}

#if 0 // not used
#define NARROW_CHAR         0xFFF0
#define WIDE_CHAR           0xFFF1

VOID
EFIAPI
AddCtrlChar(CHAR16 *Str, CHAR16 *NewStr)
{
	UINTN i, j;
	UINT16 State = NARROW_CHAR;

	for (i = 0, j = 0; i < StrLen(Str); i++)
	{
		UINTN Width = GetGlyphWidth(Str[i]);
		if (Width == 2 && State == NARROW_CHAR)
		{
			NewStr[j++] = WIDE_CHAR;
			State = WIDE_CHAR;
		}
		else if (Width == 1 && State == WIDE_CHAR)
		{
			NewStr[j++] = NARROW_CHAR;
			State = NARROW_CHAR;
		}
		NewStr[j++] = Str[i];
	}
	NewStr[j] = L'\0';
}

VOID
EFIAPI
RemoveCtrlChar(CHAR16 *Str, CHAR16 *NewStr)
{
	UINTN i, j;

	for (i = 0, j = 0; i < StrLen(Str); i++)
	{
		if (Str[i] == NARROW_CHAR || Str[i] == WIDE_CHAR)
			continue;

		NewStr[j++] = Str[i];
	}
	NewStr[j] = L'\0';
}
#endif

VOID
EFIAPI
MtUiShowMenu ( )
{
	EFI_INPUT_KEY key;
	
	key.UnicodeChar = 0;

  while (key.UnicodeChar != L'0')
  {
	  CHAR16 TempStr[64];
	  CHAR16 HeaderStr[64];
	  CHAR16 MenuSkipTest[64];
	  CHAR16 MenuEndTest[64];
	  CHAR16 MenuContinue[64];

	  GetStringById (STRING_TOKEN(STR_SETTINGS), TempStr, sizeof(TempStr));
	  StrCpyS(HeaderStr, sizeof(HeaderStr) / sizeof(HeaderStr[0]),TempStr);
	  GetStringById (STRING_TOKEN(STR_MENU_SKIP_TEST), TempStr,sizeof(TempStr));
	  StrCpyS(MenuSkipTest, sizeof(MenuSkipTest) / sizeof(MenuSkipTest[0]),TempStr);
	  GetStringById (STRING_TOKEN(STR_MENU_END_TEST), TempStr,sizeof(TempStr));
	  StrCpyS(MenuEndTest, sizeof(MenuEndTest) / sizeof(MenuEndTest[0]),TempStr);
	  GetStringById (STRING_TOKEN(STR_MENU_CONTINUE), TempStr,sizeof(TempStr));
	  StrCpyS(MenuContinue, sizeof(MenuContinue) / sizeof(MenuContinue[0]),TempStr);
	  
	  
	  MtCreatePopUp(gST->ConOut->Mode->Attribute, &key, 
		  HeaderStr,
		  MenuSkipTest,
		  MenuEndTest,
		  MenuContinue,
		  NULL);

	  switch (key.UnicodeChar)
	  {
	  case L'1': // Skip current test
		  {
			  MtSupportSkipCurrentTest();
			  key.UnicodeChar = L'0';
		  }
		  break;	  
	  case L'2': // End test
		  {
			  MtSupportAbortTesting ();
			  key.UnicodeChar = L'0';
		  }
		  break;
	  case L'0': // Continue
		  {
			  key.UnicodeChar = L'0';
		  }
		  break;

	  default:
		  break;
	  }
  }

  MtUiDisplayTestStatus();
}

CHAR16 MtUiWaitForKey(UINT16 skey, EFI_INPUT_KEY *outkey)
{
	UINTN			EventIndex;
	EFI_INPUT_KEY	key;
	while (1)
	{
		SetMem(&key, sizeof(key), 0);

		gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
		if (gST->ConIn->ReadKeyStroke(gST->ConIn, &key) == EFI_SUCCESS)
		{
			if (outkey != NULL)
			{
				CopyMem(outkey, &key, sizeof(*outkey));
			}

			if (skey == 0)
				return key.UnicodeChar;

			if (key.ScanCode == skey)
				return 0;
		}
		gBS->Stall(100);
	}
}

// Remove random junk in CPU type strings
char* makeCleanCpuName(char* typeString)
{
	// Remove these keywords
	{
		const char* removeStrings[] = {"(R)", "(TM)","(tm)", "CPU", "Processor", "Technology", "Genuine", "processor", " with Radeon HD Graphics"};
		int i;
		for (i = 0; i < sizeof(removeStrings)/sizeof(removeStrings[0]); ++i)
		{
			char* found;
			while ((found = AsciiStrStr(typeString, removeStrings[i])) != NULL)
			{
				char* afterFound = found + AsciiStrLen(removeStrings[i]);
				while (*afterFound != 0)
				{
					*found = *afterFound;
					++found; ++afterFound;
				}
				*found = 0;
			}
		}
	}

	// Turn any double created by previous exchanges into single spaces
	{
		char *i, *j;
		for (i=typeString, j=typeString; *i!=0; ++j, ++i)
		{
			if (i!=j)
				*i = *j;

			// If this character is a space skip any subsequent spaces
			if (*i == ' ')
			{
				while (*(j+1) == ' ')
					++j;
			}
		}
	}

	return typeString;
}

/*
 * Find CPU type
 */
void cpu_type(CHAR16 *szwCPUType, UINTN bufSize)
{
#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
	/* If we can get a brand string use it, and we are done */
	if (cpu_id.max_xcpuid >= 0x80000004) {
		CHAR8 Temp[64];
		CHAR16 Buf[64];
		AsciiStrCpyS(Temp, sizeof(Temp) / sizeof(Temp[0]), cpu_id.brand_id.char_array);
		makeCleanCpuName(Temp);
		UnicodeSPrint(Buf, sizeof(Buf), L"%a", Temp);
		StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), Buf);
		return;
	}

	/* The brand string is not available so we need to figure out 
	 * CPU what we have */
	switch(cpu_id.vend_id.char_array[0]) {
	/* AMD Processors */
	case 'A':
		switch(cpu_id.vers.bits.family) {
		case 4:
			switch(cpu_id.vers.bits.model) {
			case 3:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD 486DX2");
				break;
			case 7:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD 486DX2-WB");
				break;
			case 8:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD 486DX4");
				break;
			case 9:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD 486DX4-WB");
				break;
			case 14:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD 5x86-WT");
				break;
			case 15:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD 5x86-WB");
				break;
			}
			/* Since we can't get CPU speed or cache info return */
			return;
		case 5:
			switch(cpu_id.vers.bits.model) {
			case 0:
			case 1:
			case 2:
			case 3:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD K5");
				// l1_cache = 8;
				break;
			case 6:
			case 7:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD K6");
				break;
			case 8:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD K6-2");
				break;
			case 9:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD K6-III");
				break;
			case 13: 
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD K6-III+"); 
				break;
			}
			break;
		case 6:

			switch(cpu_id.vers.bits.model) {
			case 1:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD Athlon (0.25)");
				break;
			case 2:
			case 4:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD Athlon (0.18)");
				break;
			case 6:
				if (gCPUInfo.L2_cache_size == 64) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD Duron (0.18)");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Athlon XP (0.18)");
				}
				break;
			case 8:
			case 10:
				if (gCPUInfo.L2_cache_size == 64) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD Duron (0.13)");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Athlon XP (0.13)");
				}
				break;
			case 3:
			case 7:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"AMD Duron");
#if 0
				/* Duron stepping 0 CPUID for L2 is broken */
				/* (AMD errata T13)*/
				if (cpu_id.vers.bits.stepping == 0) { /* stepping 0 */
					/* Hard code the right L2 size */
					l2_cache = 64;
				} else {
				}
#endif
				break;
			}
			break;

			/* All AMD family values >= 10 have the Brand ID
			 * feature so we don't need to find the CPU type */
		}
		break;

	/* Intel or Transmeta Processors */
	case 'G':
		if ( cpu_id.vend_id.char_array[7] == 'T' ) { /* GenuineTMx86 */
			if (cpu_id.vers.bits.family == 5) {
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"TM 5x00");
			} else if (cpu_id.vers.bits.family == 15) {
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"TM 8x00");
			}
#if 0
			l1_cache = cpu_id.cache_info.ch[3] + cpu_id.cache_info.ch[7];
			l2_cache = (cpu_id.cache_info.ch[11]*256) + cpu_id.cache_info.ch[10];
#endif
		} else {				/* GenuineIntel */
			if (cpu_id.vers.bits.family == 4) {
			switch(cpu_id.vers.bits.model) {
			case 0:
			case 1:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486DX");
				break;
			case 2:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486SX");
				break;
			case 3:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486DX2");
				break;
			case 4:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486SL");
				break;
			case 5:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486SX2");
				break;
			case 7:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486DX2-WB");
				break;
			case 8:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486DX4");
				break;
			case 9:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel 486DX4-WB");
				break;
			}
			/* Since we can't get CPU speed or cache info return */
			return;
		}


		switch(cpu_id.vers.bits.family) {
		case 5:
			switch(cpu_id.vers.bits.model) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 7:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium");
#if 0
				if (l1_cache == 0) {
					l1_cache = 8;
				}
#endif
				break;
			case 4:
			case 8:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium-MMX");
#if 0
				if (l1_cache == 0) {
					l1_cache = 16;
				}
#endif
				break;
			}
			break;
		case 6:
			switch(cpu_id.vers.bits.model) {
			case 0:
			case 1:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium Pro");
				break;
			case 3:
			case 4:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium II");
				break;
			case 5:
				if (gCPUInfo.L2_cache_size == 0) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Celeron");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium II");
				}
				break;
			case 6:
				  if (gCPUInfo.L2_cache_size == 128) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Celeron");
				  } else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium II");
				  }
				break;
			case 7:
			case 8:
			case 11:
				if (gCPUInfo.L2_cache_size == 128) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Celeron");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium III");
				}
				break;
			case 9:
				if (gCPUInfo.L2_cache_size == 512) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Celeron M (0.13)");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium M (0.13)");
				}
				break;
     			case 10:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium III Xeon");
				break;
			case 12:
				// l1_cache = 24;
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Atom (0.045)");
				break;					
			case 13:
				if (gCPUInfo.L2_cache_size == 1024) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Celeron M (0.09)");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium M (0.09)");
				}
				break;
			case 14:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel Core");
				break;				
			case 15:
				if (gCPUInfo.L2_cache_size == 1024) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium E");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Intel Core 2");
				}
				break;
			}
			break;
		case 15:
			switch(cpu_id.vers.bits.model) {
			case 0:
			case 1:			
			case 2:
				if (gCPUInfo.L2_cache_size == 128) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Celeron");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium 4");
				}
				break;
			case 3:
			case 4:
				if (gCPUInfo.L2_cache_size == 256) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Celeron (0.09)");
				} else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium 4 (0.09)");
				}
				break;
			case 6:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Pentium D (65nm)");
				break;
			default:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Unknown Intel");
 				break;
			}
			break;
		}

	}
	break;

	/* VIA/Cyrix/Centaur Processors with CPUID */
	case 'C':
		if ( cpu_id.vend_id.char_array[1] == 'e' ) { /* CentaurHauls */
#if 0
			l1_cache = cpu_id.cache_info.ch[3] + cpu_id.cache_info.ch[7];
			l2_cache = cpu_id.cache_info.ch[11];
#endif
			switch(cpu_id.vers.bits.family){
			case 5:
				StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Centaur 5x86");
				break;
			case 6: // VIA C3
				switch(cpu_id.vers.bits.model){
				default:
				    if (cpu_id.vers.bits.stepping < 8) {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C3 Samuel2");
				    } else {
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C3 Eden");
				    }
				break;
				case 10:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C7 (C5J)");
#if 0
					l1_cache = 64;
					l2_cache = 128;
#endif
					break;
				case 13:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C7 (C5R)");
#if 0
					l1_cache = 64;
					l2_cache = 128;
#endif
					break;
				case 15:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA Isaiah (CN)");
#if 0
					l1_cache = 64;
					l2_cache = 128;
#endif
					break;
				}
			}
		} else {				/* CyrixInstead */
			switch(cpu_id.vers.bits.family) {
			case 5:
				switch(cpu_id.vers.bits.model) {
				case 0:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Cyrix 6x86MX/MII");
					break;
				case 4:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Cyrix GXm");
					break;
				}
				return;

			case 6: // VIA C3
				switch(cpu_id.vers.bits.model) {
				case 6:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Cyrix III");
					break;
				case 7:
					if (cpu_id.vers.bits.stepping < 8) {
						StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C3 Samuel2");
					} else {
						StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C3 Ezra-T");
					}
					break;
				case 8:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C3 Ezra-T");
					break;
				case 9:
					StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"VIA C3 Nehemiah");
					break;
				}
#if 0
				// L1 = L2 = 64 KB from Cyrix III to Nehemiah
				l1_cache = 64;
				l2_cache = 64;
#endif
				break;
			}
		}
		break;
	/* Unknown processor */
	default:
		/* Make a guess at the family */
		switch(cpu_id.vers.bits.family) {
		case 5:
			StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"586");
			break;
		case 6:
			StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"686");
			break;
		default:
			StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), L"Unidentified Processor");
			break;
		}
	}
#elif defined(MDE_CPU_AARCH64)
	CHAR16 Buf[64];
	UnicodeSPrint(Buf, sizeof(Buf), L"%a %a", cpu_id.vend_id.char_array, cpu_id.brand_id.char_array);
	StrCpyS(szwCPUType, bufSize / sizeof(CHAR16), Buf);
#endif
}

// display_init
//
// Print the top and bottom of the console screen
static void display_init(void)
{
	UINTN OriginalAttribute;
	CHAR16 TempBuf[128];

	OriginalAttribute = gST->ConOut->Mode->Attribute;

	/* Clear top of screen */
	gST->ConOut->ClearScreen(gST->ConOut);

	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_GREEN));
	Print(L"OKN %s %s", PROGRAM_NAME, PROGRAM_VERSION);

	//
	// Retrieve the number of columns and rows in the current console mode
	//0
	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
	ConsolePrintXY(0, ConHeight-1, GetStringById(STRING_TOKEN(STR_CONFIG_SHORTCUT), TempBuf, sizeof(TempBuf))); 
	gST->ConOut->SetCursorPosition(gST->ConOut, UnicodeStringDisplayLength(TempBuf), ConHeight-1);
	
	int NumSpaces = (int)MaxUIWidth - 2 - gST->ConOut->Mode->CursorColumn;
	if (NumSpaces > 0)
	{
		SetMem16(TempBuf, NumSpaces * sizeof(TempBuf[0]), L' ');
		TempBuf[NumSpaces] = L'\0';
		Print(TempBuf);
	}	
	
	gST->ConOut->SetAttribute(gST->ConOut, OriginalAttribute);
}

VOID
EFIAPI
MtUiDisplayTestStatus ( )
{
	UINTN i;
	UINTN OriginalAttribute;
	CHAR16 CPUType[64];
	CHAR16 TempBuf[128];
	CHAR16 TempBuf2[32];

	OriginalAttribute = gST->ConOut->Mode->Attribute;

	/* Setup the display */
	display_init();

	TempBuf[0] = 0;
	for (i = 0; i < MaxUIWidth - 2; i++)
		StrCatS(TempBuf, sizeof(TempBuf) / sizeof(TempBuf[0]), L"-");

	// Print the first horizontal line
	ConsolePrintXY(0, 7, TempBuf);

	// Print the second horizontal line
	ConsolePrintXY(0, 10, TempBuf);

	// Print the vertical divider
	for(i=1; i < 6; i++) {
		ConsolePrintXY(COL_MID-2, i, L"| ");
	}
	ConsolePrintXY(0, LINE_INFO, L"%s:", GetStringById( STRING_TOKEN(STR_TIME), TempBuf, sizeof(TempBuf)));
#ifdef MDE_CPU_X64
	ConsolePrintXY(20, LINE_INFO, L"%s: 64-bit", GetStringById( STRING_TOKEN(STR_ADDRESS_MODE), TempBuf, sizeof(TempBuf)));
#elif defined(MDE_CPU_IA32)
	ConsolePrintXY(20, LINE_INFO, L"%s: 32-bit", GetStringById( STRING_TOKEN(STR_ADDRESS_MODE), TempBuf, sizeof(TempBuf)));
#endif
	ConsolePrintXY(41, LINE_INFO, L"%s:", GetStringById( STRING_TOKEN(STR_PASSES), TempBuf, sizeof(TempBuf)));
	ConsolePrintXY(63, LINE_INFO, L"%s:", GetStringById (STRING_TOKEN(STR_ERRORS), TempBuf, sizeof(TempBuf)));


	/* Print the CPU/cache information */
	cpu_type(CPUType, sizeof(CPUType));
	ConsolePrintXY(COL_MID, 0, CPUType);

	if (cpu_msr.raw_freq_cpu /* cpu_speed */ < 999499) {
		UINT32 speed = cpu_msr.raw_freq_cpu + 50; /* for rounding */
		ConsolePrintXY(0, 1, L"%s:%4d.%1d MHz", GetStringById( STRING_TOKEN(STR_CLK_TEMP), TempBuf, sizeof(TempBuf)), speed/1000, (speed/100)%10);
	} else {
		UINT32 speed = cpu_msr.raw_freq_cpu + 500; /* for rounding */
		ConsolePrintXY(0, 1, L"%s:%6d MHz", GetStringById( STRING_TOKEN(STR_CLK_TEMP), TempBuf, sizeof(TempBuf)), speed/1000);
	}

	MtUiUpdateCPUTemp( MtSupportGetCPUTemp() );

	ConsolePrintXY(0, 2, L"%s: %s", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L1_CACHE), TempBuf, sizeof(TempBuf)), MtSupportHumanReadableSize((gCPUInfo.L1_data_cache_size + gCPUInfo.L1_instruction_cache_size) * SIZE_1KB, TempBuf2, sizeof(TempBuf2)));
	if (l1_speed == 0)
	{
		ConsolePrintXY(17, 2, L"%s", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
	}
	else if (l1_speed > 10000)
	{
		ConsolePrintXY(17, 2, L"%3d.%01d GB/s", l1_speed / 1024, ModU64x32(DivU64x32(MultU64x32(l1_speed, 10), 1024), 10));
	}
	else
	{
		ConsolePrintXY(17, 2, L"%6d MB/s", l1_speed);
	}

	ConsolePrintXY(0, 3, L"%s: %s", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L2_CACHE), TempBuf, sizeof(TempBuf)), MtSupportHumanReadableSize(gCPUInfo.L2_cache_size * SIZE_1KB, TempBuf2, sizeof(TempBuf2)));	
	if (l2_speed == 0)
	{
		ConsolePrintXY(17, 3, L"%s", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
	}
	else if (l2_speed > 10000)
	{
		ConsolePrintXY(17, 3, L"%3d.%01d GB/s", l2_speed / 1024, ModU64x32(DivU64x32(MultU64x32(l2_speed, 10), 1024), 10));
	}
	else
	{
		ConsolePrintXY(17, 3, L"%6d MB/s", l2_speed);
	}

	if (gCPUInfo.L3_cache_size)
	{
		ConsolePrintXY(0, 4, L"%s: %s", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)), MtSupportHumanReadableSize(gCPUInfo.L3_cache_size * SIZE_1KB, TempBuf2, sizeof(TempBuf2)));
		if (l3_speed == 0)
		{
			ConsolePrintXY(17, 4, L"%s", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
		}
		else if (l3_speed > 10000)
		{
			ConsolePrintXY(17, 4, L"%3d.%01d GB/s", l3_speed / 1024, ModU64x32(DivU64x32(MultU64x32(l3_speed, 10), 1024), 10));
		}
		else
		{
			ConsolePrintXY(17, 4, L"%6d MB/s", l3_speed);
		}
	}
	else
	{
		ConsolePrintXY(0, 4, L"%s: %s", GetStringById(STRING_TOKEN(STR_MENU_SYSINFO_L3_CACHE), TempBuf, sizeof(TempBuf)), GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf2, sizeof(TempBuf2)));
	}

	ConsolePrintXY(0, 5, L"%s: %s", GetStringById(STRING_TOKEN(STR_MEMORY), TempBuf, sizeof(TempBuf)), MtSupportHumanReadableSize(memsize, TempBuf2, sizeof(TempBuf2)));

	if (mem_speed == 0)
	{
		ConsolePrintXY(17, 5, L"%s", GetStringById(STRING_TOKEN(STR_MENU_NA), TempBuf, sizeof(TempBuf)));
	}
	else if (mem_speed > 10000)
	{
		ConsolePrintXY(17, 5, L"%3d.%01d GB/s", mem_speed / 1024, ModU64x32(DivU64x32(MultU64x32(mem_speed, 10), 1024), 10));
	}
	else
	{
		ConsolePrintXY(17, 5, L"%6d MB/s", mem_speed);
	}

	// Print the RAM details
	CHAR16 szwRAMSummary[66];
	getMemConfigSummaryLine(szwRAMSummary, sizeof(szwRAMSummary));
	ConsolePrintXY(0, 6, L"%s: %s", GetStringById(STRING_TOKEN(STR_RAM_CONFIG), TempBuf, sizeof(TempBuf)), szwRAMSummary);
	
	ConsolePrintXY(61, 5, L"%s :", GetStringById(STRING_TOKEN(STR_RAM_TEMP), TempBuf, sizeof(TempBuf)));
	MtUiUpdateRAMTemp(MtSupportGetRAMTemp(0));

	// Print the CPU#s
	ConsolePrintXY(0, 8, L"%s:", GetStringById(STRING_TOKEN(STR_CPU), TempBuf, sizeof(TempBuf)));

	for (i = 0; i < MPSupportGetNumProcessors() && i < MAX_DISPLAYABLE_CPUS; i++)
	{
		if (i <= 9)
		{
			ConsolePrintXY(7+i,8,L"%d", i);
		}
		else
		{
			ConsolePrintXY(7+i,8,L"%c", 0x41+i-10);
		}
	}

	ConsolePrintXY(0, 9, L"%s:", GetStringById(STRING_TOKEN(STR_STATE), TempBuf, sizeof(TempBuf)));

	ConsolePrintXY(38, 8, L"| %s:", GetStringById( STRING_TOKEN(STR_CPUS_FOUND), TempBuf, sizeof(TempBuf)));

	ConsolePrintXY(38, 9, L"| %s:", GetStringById( STRING_TOKEN(STR_CPUS_STARTED), TempBuf, sizeof(TempBuf)));
	ConsolePrintXY(60, 9, L"%s:", GetStringById( STRING_TOKEN(STR_CPUS_ACTIVE), TempBuf, sizeof(TempBuf)));

	ConsolePrintXY(56, 8, L"%3d", MPSupportGetNumProcessors());
	ConsolePrintXY(56, 9, L"%3d", MPSupportGetNumEnabledProcessors());

	ConsolePrintXY(72, 9, L"%3d", MtSupportCPUSelMode() == CPU_PARALLEL ? MPSupportGetNumEnabledProcessors() : 1);

	MtUiSetTestName(NULL);
	MtUiSetTestsCompleted(MtSupportCurTest(),MtSupportNumTests());
	MtUiDrawProgress();
	MtUiDrawAddressRange();
	// MtUiSetIterationCount(MtSupportCurIterCount());
	MtUiSetPassNo(MtSupportCurPass());
	MtUiRePrint();
	MtSupportUpdateErrorDisp();
	MtUiUpdateCPUState();
	MtUiUpdatePattern(TRUE);
}

INTN 
EFIAPI
DrawTextXY(IN CHAR16 *Text, IN EG_PIXEL *BgColour, IN INTN XPos, IN INTN YPos, IN UINT8 XAlign)
{
  UINTN TextWidth, TextHeight;
  EG_IMAGE *TextBufferXY = NULL;

  if (!Text) return 0;
  
  egMeasureText(Text, &TextWidth, &TextHeight);
  TextBufferXY = egCreateImage(TextWidth, TextHeight, TRUE);
  
  if (BgColour != NULL) {
	  egFillImage(TextBufferXY, BgColour);
  } else {
	  egFillImage(TextBufferXY, (EG_PIXEL *)&mEfiColors[gST->ConOut->Mode->Attribute >> 4]);
  }
  
  
  // render the text
  egRenderText(Text, TextBufferXY, 0, 0, 0); //input only
  // BltImageAlpha(TextBufferXY, (XPos - (TextWidth >> XAlign)), YPos,  &MenuBackgroundPixel, 16);
  	if (XAlign == ALIGN_CENTRE)
		egDrawImage(TextBufferXY, NULL, XPos - (TextWidth >> 1), YPos);
	else if (XAlign == ALIGN_RIGHT)
		egDrawImage(TextBufferXY, NULL, XPos - TextWidth, YPos);
	else
		egDrawImage(TextBufferXY, NULL, XPos, YPos);
  // egDrawImage(TextBufferXY, XPos - (TextWidth >> XAlign), YPos);
  egFreeImage(TextBufferXY);
  return TextWidth;
}

BOOLEAN GetTextInfo(IN CHAR16 *Text, UINTN *Width, UINTN *Height)
{
	EFI_STATUS                          Status;
	EFI_HII_FONT_PROTOCOL               *HiiFont = NULL;
	EFI_IMAGE_OUTPUT                    *Blt;
	EFI_HII_ROW_INFO                    *RowInfoArray;
	UINTN                               RowInfoArraySize = 0;
	const UINTN UNIFONT_CHAR_WIDTH = 8;
	const UINTN UNIFONT_CHAR_HEIGHT = 19;

	Blt                   = NULL;
	RowInfoArray          = NULL;

	if (Text == NULL)
		return FALSE;

	if (Text[0] == L'\0')
	{
		if (Width != NULL)
			*Width = 0;
		if (Height != NULL)
			*Height = 0;
		return TRUE;
	}

	Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **) &HiiFont);
	
	if (HiiFont)
	{

		Status = HiiFont->StringToImage (
								HiiFont,
								0,
								Text,
								NULL,
								&Blt,
								0,
								0,
								&RowInfoArray,
								&RowInfoArraySize,
								NULL
								);


		if (EFI_ERROR (Status))
		{
			AsciiFPrint(DEBUG_FILE_HANDLE, "GetTextInfo - StringToImage failed: %r", Status);
			return FALSE;
		}

		if (RowInfoArraySize != 0) {
			if (RowInfoArray[0].LineWidth > ScreenWidth || RowInfoArray[0].LineHeight > ScreenHeight)
			{
				AsciiFPrint(DEBUG_FILE_HANDLE, "GetTextInfo - **Warning** \"%s\" Text Width: %ld, Height: %ld", Text, (UINT64)RowInfoArray[0].LineWidth, (UINT64)RowInfoArray[0].LineHeight);

				if (Width != NULL)
					*Width = StrLen(Text) * UNIFONT_CHAR_WIDTH;
				if (Height != NULL)
					*Height = UNIFONT_CHAR_HEIGHT;
			}
			else
			{
				if (Width)
					*Width  = RowInfoArray[0].LineWidth;

				if (Height)
					*Height = RowInfoArray[0].LineHeight;
			}

		} else {
			AsciiFPrint(DEBUG_FILE_HANDLE, "GetTextInfo - **Warning** \"%s\" RowInfoArraySize: %d", Text, RowInfoArraySize);

			if (Width != NULL)
				*Width = StrLen(Text) * UNIFONT_CHAR_WIDTH;
			if (Height != NULL)
				*Height = UNIFONT_CHAR_HEIGHT;
		}

		if (RowInfoArray)
			FreePool (RowInfoArray);
		if (Blt->Image.Bitmap)
			FreePool (Blt->Image.Bitmap);
		if (Blt)
			FreePool (Blt);
	}
	else
	{
		egMeasureText(Text, Width, Height);
	}

	return TRUE;
}

// <km> The PrintXY UDK function does not work properly for some UEFI implementations
UINTN
EFIAPI
PrintXYAlign (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ForeGround,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGround,
  IN UINT8                            XAlign,
  IN CONST CHAR16                     *Format,
  ...
  )
{
	EFI_STATUS                          Status;
	UINT32                              HorizontalResolution;
	UINT32                              VerticalResolution;
	UINT32                              ColorDepth;
	UINT32                              RefreshRate;
	EFI_HII_OUT_FLAGS                   HiiFlags = 0;
	EFI_HII_FONT_PROTOCOL               *HiiFont = NULL;
	EFI_IMAGE_OUTPUT                    Blt;
	EFI_IMAGE_OUTPUT                    *pBlt = &Blt;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL       *Bitmap = NULL;
	UINTN                               BltX, BltY;
	EFI_FONT_DISPLAY_INFO               FontInfo;
	EFI_HII_ROW_INFO                    *RowInfoArray = NULL;
	UINTN                               RowInfoArraySize = 0;
	UINTN                               ScreenPointX = PointX;
	UINTN                               Width;
	UINTN                               Height;
	UINTN                               Delta;
	EFI_GRAPHICS_OUTPUT_PROTOCOL        *GraphicsOutput = NULL;

	EFI_UGA_DRAW_PROTOCOL *UgaDraw = NULL;
	CHAR16   Buffer[MAX_MESSAGE_LENGTH];
	VA_LIST  Marker;
	UINTN    Return;

	//
	// If Format is NULL, then ASSERT().
	//
	ASSERT (Format != NULL);
	  
	//
	// Convert the DEBUG() message to a Unicode String
	//
	VA_START (Marker, Format);
	Return = UnicodeVSPrint (Buffer, sizeof(Buffer),  Format, Marker);
	VA_END (Marker);

	if (StrLen(Buffer) == 0)
	return 0;

	// Get the Hii Font protocol
	Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **) &HiiFont);
	if (HiiFont != NULL)
	{
		// Get UgaDraw (deprecated)/GOP (current) protocol to see which one is supported
		Status = gBS->LocateProtocol(&gEfiUgaDrawProtocolGuid, NULL, (VOID **) &UgaDraw);
		if (EFI_ERROR(Status))
			UgaDraw = NULL;

		Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
		if (EFI_ERROR(Status))
			GraphicsOutput = NULL;
		
		// Get resolution
		if (GraphicsOutput != NULL)
		{
			HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
			VerticalResolution = GraphicsOutput->Mode->Info->VerticalResolution;
		} 
		else if (UgaDraw != NULL) 
		{
			Status = UgaDraw->GetMode (UgaDraw, &HorizontalResolution, &VerticalResolution, &ColorDepth, &RefreshRate);
			if (Status != EFI_SUCCESS)
			{
				AsciiFPrint(DEBUG_FILE_HANDLE, "GetMode failed: %r", Status);
				return 0;
			}
		} 
		else
		{
			AsciiFPrint(DEBUG_FILE_HANDLE, "No graphics protocol found");
			return 0;
		}

		GetTextInfo(Buffer, &Width, &Height);

		SetMem(&Blt, sizeof(Blt), 0);
	 
		ZeroMem (&FontInfo, sizeof (EFI_FONT_DISPLAY_INFO));

		// Set fg/bg colour
		if (ForeGround != NULL) {
			CopyMem (&FontInfo.ForegroundColor, ForeGround, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
		} else {
			CopyMem (
				&FontInfo.ForegroundColor,
				&mEfiColors[gST->ConOut->Mode->Attribute & 0x0f],
				sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
				);
		}
		if (BackGround != NULL) {
			CopyMem (&FontInfo.BackgroundColor, BackGround, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
		} else {
			CopyMem (
				&FontInfo.BackgroundColor,
				&mEfiColors[gST->ConOut->Mode->Attribute >> 4],
				sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
				);
		}

		// Calculate the coordinates based on alignment
		if (XAlign == ALIGN_CENTRE)
		{
			ScreenPointX = PointX - Width / 2;
		}
		else if (XAlign == ALIGN_RIGHT)
		{
			ScreenPointX  = PointX - Width;
		}
		
		// If using GOP, try drawing directly to screen
		if (gTextDirectToScreen)
		{
			if (GraphicsOutput)
			{
				HiiFlags = EFI_HII_DIRECT_TO_SCREEN;
				Blt.Image.Screen = GraphicsOutput;
				Blt.Width = (UINT16)HorizontalResolution;
				Blt.Height = (UINT16)VerticalResolution;

				BltX = ScreenPointX;
				BltY = PointY;

				Status = HiiFont->StringToImage(
					HiiFont,
					HiiFlags,
					Buffer,
					&FontInfo,
					&pBlt,
					BltX,
					BltY,
					NULL,
					NULL,
					NULL
				);

				if (Status == EFI_SUCCESS)
					return Status;
			}
		}

		// Create a bitmap, then Blt to screen
		HiiFlags = EFI_HII_OUT_FLAG_CLIP;

		Bitmap = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)AllocateZeroPool (Width * Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
		Blt.Image.Bitmap = Bitmap;
		Blt.Width = (UINT16)Width;
		Blt.Height = (UINT16)Height;

		BltX = 0;
		BltY = 0;

		//
		//  We ask StringToImage to print the string to blt buffer, then blt to device using UgaDraw.
		//
		Status = HiiFont->StringToImage (
								HiiFont,
								HiiFlags,
								Buffer,
								&FontInfo,
								&pBlt,
								BltX,
								BltY,
								&RowInfoArray,
								&RowInfoArraySize,
								NULL
								);

		if (EFI_ERROR (Status))
			AsciiFPrint(DEBUG_FILE_HANDLE, "StringToImage returned: %r", Status);
		else
		{
			// Get the text width/height info
			if (RowInfoArraySize != 0) {
				Width  = RowInfoArray[0].LineWidth;
				Height = RowInfoArray[0].LineHeight;
				Delta  = Blt.Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
			} else {
				AsciiFPrint(DEBUG_FILE_HANDLE, "Warning - RowInfoArraySize=%d (Width=%d, Height=%d)", RowInfoArraySize, Width, Height);
				GetTextInfo(Buffer, &Width, &Height);
				Delta  = Blt.Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
			}

			// Check whether the text will be off screen. If so, possible bug in firmware so use libeg to draw the text
			if (ScreenPointX + Width > (HorizontalResolution + 64) ||
				PointY + Height > (VerticalResolution + 64))
			{
				AsciiFPrint(DEBUG_FILE_HANDLE, "Warning - Text outside screen (XPos=%d, YPos=%d, Width=%d, Height=%d)", ScreenPointX, PointY, Width, Height);

				DrawTextXY(Buffer, (EG_PIXEL*)BackGround, PointX, PointY, XAlign);
			}
			else
			{
				// Blt the buffer to the screen
				if (GraphicsOutput != NULL) {
					Status = GraphicsOutput->Blt(GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)Blt.Image.Bitmap,
						EfiBltBufferToVideo, 0, 0, ScreenPointX, PointY, Width, Height, Delta);
				}
				else if (UgaDraw != NULL) {
					Status = UgaDraw->Blt(UgaDraw, (EFI_UGA_PIXEL*)Blt.Image.Bitmap, EfiUgaBltBufferToVideo,
						0, 0, ScreenPointX, PointY, Width, Height, Delta);
				}

				if (EFI_ERROR(Status))
					AsciiFPrint(DEBUG_FILE_HANDLE, "%s Blt returned: %r (Text=\"%s\" XPos=%d, YPos=%d, Width=%d, Height=%d, Delta=%d)", GraphicsOutput != NULL ? L"GraphicsOutput" : L"UgaDraw", Status, Buffer, ScreenPointX, PointY, Width, Height, Delta);

			}

			if (RowInfoArray)
				FreePool(RowInfoArray);
		}

		if (Bitmap)
			FreePool (Bitmap);
	}
	else
		return DrawTextXY(Buffer, (EG_PIXEL *)BackGround, PointX, PointY, XAlign);

	return Return;
}

#if 0
UINTN
EFIAPI
PrintXYAlign (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN UINT8                            XAlign,
  IN CONST CHAR16                     *Format,
  ...
  )
{
  CHAR16   Buffer[MAX_MESSAGE_LENGTH];
  VA_LIST  Marker;
  UINTN    Return;

  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  //
  // Convert the DEBUG() message to a Unicode String
  //
  VA_START (Marker, Format);
  Return = UnicodeVSPrint (Buffer, sizeof(Buffer),  Format, Marker);
  VA_END (Marker);


	if (XAlign == ALIGN_LEFT)
	{
		return PrintXY2(PointX, PointY, Foreground, Background, L"%s", Buffer);
	}
	else if (XAlign == ALIGN_CENTRE || XAlign == ALIGN_RIGHT)
	{
		UINTN Width;

		GetTextInfo(Buffer, &Width, NULL);

		if (XAlign == ALIGN_CENTRE)
			PrintXY2(PointX - Width / 2, PointY, Foreground, Background, Buffer);
		else if (XAlign == ALIGN_RIGHT)
			PrintXY2(PointX - Width, PointY, Foreground, Background, Buffer);
	}

  return 0;
}
#endif

INTN 
EFIAPI
ImagePrintXYAlign(
  IN EG_IMAGE                         *Image, 
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN UINT8 XAlign,
  IN CONST CHAR16  *Format,
  ...
  )
{
	CHAR16   Buffer[MAX_MESSAGE_LENGTH];
	VA_LIST  Marker;
	UINTN    Return;


	//
	// If Format is NULL, then ASSERT().
	//
	ASSERT (Format != NULL);

	//
	// Convert the DEBUG() message to a Unicode String
	//
	VA_START (Marker, Format);
	Return = UnicodeVSPrint (Buffer, sizeof(Buffer),  Format, Marker);
	VA_END (Marker);

	if (StrLen(Buffer) == 0)
		return 0;

	if (Image)
	{
		EFI_STATUS                          Status;
		EFI_HII_FONT_PROTOCOL               *HiiFont = NULL;
		EFI_IMAGE_OUTPUT                    Blt;
		EFI_IMAGE_OUTPUT                    *pBlt = &Blt;
		EFI_FONT_DISPLAY_INFO               FontInfo;
		UINTN                               Width;
		UINTN                               Height;

		SetMem(&Blt, sizeof(Blt), 0);

		Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **) &HiiFont);
		if (HiiFont != NULL)
		{
			GetTextInfo(Buffer, &Width, &Height);


			Blt.Width        = (UINT16) (Image->Width);
			Blt.Height       = (UINT16) (Image->Height);

			// We write directly to the image buffer
			Blt.Image.Bitmap = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Image->PixelData;
	  
			ZeroMem (&FontInfo, sizeof (EFI_FONT_DISPLAY_INFO));

			if (Foreground != NULL) {
				CopyMem (&FontInfo.ForegroundColor, Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
			} else {
				CopyMem (
					&FontInfo.ForegroundColor,
					&mEfiColors[gST->ConOut->Mode->Attribute & 0x0f],
					sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
					);
			}
			if (Background != NULL) {
				CopyMem (&FontInfo.BackgroundColor, Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
			} else {
				CopyMem (
					&FontInfo.BackgroundColor,
					&mEfiColors[gST->ConOut->Mode->Attribute >> 4],
					sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
					);
			}

			Status = HiiFont->StringToImage (
									HiiFont,
									Image->HasAlpha ? EFI_HII_OUT_FLAG_TRANSPARENT : 0,
									Buffer,
									&FontInfo,
									&pBlt,
									XAlign == ALIGN_LEFT ? PointX : XAlign == ALIGN_CENTRE ? PointX - Width / 2 : PointX - Width,
									PointY,
									NULL,
									NULL,
									NULL
									);

			if (EFI_ERROR (Status))
				AsciiFPrint(DEBUG_FILE_HANDLE, "StringToImage returned: %r", Status);

		}
		else
			return ImageDrawTextXY(Image, Buffer, (EG_PIXEL *)Background, PointX, PointY, XAlign, Background && Background->Red > 128 ? 255 : 0);

	}
	else
	{
		return PrintXYAlign(PointX, PointY, Foreground, Background, XAlign, L"%s", Buffer);
	}
	return Return;
}

UINTN
EFIAPI
ClearTextXYAlign (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN UINT8 XAlign,
  IN CONST CHAR16  *Format,
  ...
  )
{
  CHAR16   Buffer[MAX_MESSAGE_LENGTH];
  VA_LIST  Marker;
  UINTN    Return;
  UINTN TextWidth, TextHeight;
  UINTN ScreenPointX;


  //
  // If Format is NULL, then ASSERT().
  //
  ASSERT (Format != NULL);

  //
  // Convert the DEBUG() message to a Unicode String
  //
  VA_START (Marker, Format);
  Return = UnicodeVSPrint (Buffer, sizeof(Buffer),  Format, Marker);
  VA_END (Marker);

  if (StrLen(Buffer) == 0)
	  return 0;

  // Erase the text by finding the width/height of the previous text and fill with background colour
  GetTextInfo(Buffer, &TextWidth, &TextHeight);

  if (TextWidth == 0 || TextHeight == 0)
	  return 0;

  if (XAlign == ALIGN_CENTRE)
	  ScreenPointX = PointX - (TextWidth >> 1);
  else if (XAlign == ALIGN_RIGHT)
	  ScreenPointX  = PointX - TextWidth;
  else
	  ScreenPointX  = PointX;

  if (Background != NULL)
	  egFillArea(NULL, (EG_PIXEL *)Background, ScreenPointX, PointY, TextWidth, TextHeight);
  else
	  egFillArea(NULL, (EG_PIXEL *)&mEfiColors[gST->ConOut->Mode->Attribute >> 4], ScreenPointX, PointY, TextWidth, TextHeight);
#if 0
  if (Background != NULL)
	  return PrintXYAlign(PointX, PointY, Background, Background, XAlign, L"%s", Buffer);
  
  return PrintXYAlign(PointX, PointY, &mEfiColors[gST->ConOut->Mode->Attribute >> 4], &mEfiColors[gST->ConOut->Mode->Attribute >> 4], XAlign, L"%s", Buffer);
#endif
  return 0;
}

INTN 
EFIAPI
ImageDrawTextXY(IN EG_IMAGE *Image, IN CHAR16 *Text, IN EG_PIXEL *BgColour, IN INTN XPos, IN INTN YPos, IN UINT8 XAlign, UINT8 Brightness )
{
  UINTN TextWidth, TextHeight;
  UINTN LeftXPos = XPos;
  EG_IMAGE *TextBufferXY = NULL;

  if (!Text) return 0;
  
  // Get the text width/height
  egMeasureText(Text, &TextWidth, &TextHeight);
  TextBufferXY = egCreateImage(TextWidth, TextHeight, TRUE);
  
  // Fill the bg colour
  if (BgColour != NULL) {
	  egFillImage(TextBufferXY, BgColour);
  } else {
	  egFillImage(TextBufferXY, (EG_PIXEL *)&mEfiColors[gST->ConOut->Mode->Attribute >> 4]);
  }
  
  // render the text
  egRenderText(Text, TextBufferXY, 0, 0, Brightness); //input only
  // BltImageAlpha(TextBufferXY, (XPos - (TextWidth >> XAlign)), YPos,  &MenuBackgroundPixel, 16);

  if (XAlign == ALIGN_CENTRE)
	  LeftXPos = XPos - (TextWidth >> 1);
  else if (XAlign == ALIGN_RIGHT)
	  LeftXPos = XPos - TextWidth;
  else
	  LeftXPos = XPos;

  
  if (Image == NULL) // Draw the text to screen
	egDrawImage(TextBufferXY, NULL, LeftXPos, YPos);
  else // Draw the text to image
	  egComposeImage( Image, TextBufferXY, LeftXPos, YPos);
  egFreeImage(TextBufferXY);
  return TextWidth;
}

#if 0
INTN 
EFIAPI
DrawTextXYDark(IN CHAR16 *Text, IN EG_PIXEL *BgColour, IN INTN XPos, IN INTN YPos, IN UINT8 XAlign)
{
  UINTN TextWidth, TextHeight;
  EG_IMAGE *TextBufferXY = NULL;

  if (!Text) return 0;
  
  egMeasureText(Text, &TextWidth, &TextHeight);
  TextBufferXY = egCreateImage(TextWidth, TextHeight, TRUE);
  
  egFillImage(TextBufferXY, BgColour);
  
  // render the text
  egRenderText(Text, TextBufferXY, 0, 0, 255); //input only
  // BltImageAlpha(TextBufferXY, (XPos - (TextWidth >> XAlign)), YPos,  &MenuBackgroundPixel, 16);
  egDrawImage(TextBufferXY, XPos - (TextWidth >> XAlign), YPos);
  egFreeImage(TextBufferXY);
  return TextWidth;
}
#endif

BOOLEAN
EFIAPI
DrawProgressBar(IN EG_PIXEL *BarColour, IN EG_PIXEL *BorderColour, IN INTN XPos, IN INTN YPos, IN INTN Width, IN INTN Height, UINTN Percent)
{
	CHAR16 TextBuf[16];
	UINTN TextWidth,TextHeight;

  if (Percent < 0 || Percent > 100)
	  return FALSE;

  if (Width == 0 || Height == 0)
	  return FALSE;

  egDrawRectangle(NULL, (int)XPos, (int)YPos, (int)Width, (int)Height, BorderColour);
  egClearScreenArea((EG_PIXEL *)(&mEfiColors[gST->ConOut->Mode->Attribute >> 4]), XPos+1, YPos+1, Width-2, Height-2);
  egClearScreenArea(BarColour, XPos+1, YPos+1, (Width-2) * Percent / 100, Height-2);

  UnicodeSPrint(TextBuf, sizeof(TextBuf), L"%d%%", Percent);
  GetTextInfo(TextBuf, &TextWidth, &TextHeight);
  PrintXYAlign(XPos + (Width - TextWidth)/2, YPos + (Height - TextHeight)/2, &DEFAULT_TEXT_PIXEL, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)(&mEfiColors[gST->ConOut->Mode->Attribute >> 4]), ALIGN_LEFT, L"%d%%", Percent);
  return TRUE;
}

UINTN
DisplayMenuOption(
	IN UINTN                            PointX,
	IN UINTN                            PointY,
	CHAR16*                             Text,
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*		TextColour,
	BOOLEAN                             Selected,
	UINTN                               OptWidth
	)
{
	if ( Selected )
	{
		// If selected, make the bg colour the 'selected' bg colour
		egFillArea(NULL, (EG_PIXEL *)&DEFAULT_SEL_PIXEL, (int)PointX, (int)PointY, (int)OptWidth, (int)gTextHeight);
		return PrintXYAlign(PointX, PointY, TextColour, &DEFAULT_SEL_PIXEL, ALIGN_LEFT, Text);
	}
	else
	{
		// make the bg colour the default bg colour
		egFillArea(NULL, (EG_PIXEL *)&mEfiColors[gST->ConOut->Mode->Attribute >> 4], (int)PointX, (int)PointY, (int)OptWidth, (int)gTextHeight);
		return PrintXYAlign(PointX, PointY, TextColour, NULL, ALIGN_LEFT, Text);
	}
}

EFI_STATUS
MtUiTurnOnSpeaker(BOOLEAN On)
{
	UINT8 Data;

	Data = IoRead8(EFI_SPEAKER_CONTROL_PORT);
	if (On)
		Data |= 0x03;
	else
		Data &= EFI_SPEAKER_OFF_MASK;

	IoWrite8(EFI_SPEAKER_CONTROL_PORT, Data);

	return EFI_SUCCESS;
}

EFI_STATUS
MtUiGenerateBeep(UINTN NumberBeeps)
{
	for (UINTN Num = 0; Num < NumberBeeps; Num++) {
		MtUiTurnOnSpeaker(TRUE);
		gBS->Stall(EFI_BEEP_ON_TIME_INTERVAL);
		MtUiTurnOnSpeaker(FALSE);
		gBS->Stall(EFI_BEEP_OFF_TIME_INTERVAL);
	}

	return EFI_SUCCESS;
}

VOID
EFIAPI
MtCreatePopUpAtPos(
    IN  UINTN          Attribute,
    IN  INTN           Row,
    IN  INTN           Column,
    OUT EFI_INPUT_KEY* Key, OPTIONAL
    ...
)
{
    EFI_STATUS                       Status;
    VA_LIST                          Args;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* ConOut;
    EFI_SIMPLE_TEXT_OUTPUT_MODE      SavedConsoleMode;
    UINTN                            Columns;
    UINTN                            Rows;
    UINTN                            NumberOfLines;
    UINTN                            MaxLength;
    CHAR16* String;
    UINTN                            Length;
    CHAR16* Line;
    UINTN                            EventIndex;
    CHAR16* TmpString;

    //
    // Determine the length of the longest line in the popup and the the total
    // number of lines in the popup
    //
    VA_START(Args, Key);
    MaxLength = 0;
    NumberOfLines = 0;
    while ((String = VA_ARG(Args, CHAR16*)) != NULL) {
        MaxLength = MAX(MaxLength, UnicodeStringDisplayLength(String));
        NumberOfLines++;
    }
    VA_END(Args);

    //
    // If the total number of lines in the popup is zero, then ASSERT()
    //
    ASSERT(NumberOfLines != 0);

    //
    // If the maximum length of all the strings is zero, then ASSERT()
    //
    ASSERT(MaxLength != 0);

    //
    // Cache a pointer to the Simple Text Output Protocol in the EFI System Table
    //
    ConOut = gST->ConOut;

    //
    // Save the current console cursor position and attributes
    //
    CopyMem(&SavedConsoleMode, ConOut->Mode, sizeof(SavedConsoleMode));

    //
    // Retrieve the number of columns and rows in the current console mode
    //
    ConOut->QueryMode(ConOut, SavedConsoleMode.Mode, &Columns, &Rows);

    //
    // Disable cursor and set the foreground and background colors specified by Attribute
    //
    ConOut->EnableCursor(ConOut, FALSE);
    ConOut->SetAttribute(ConOut, Attribute);

    //
    // Limit NumberOfLines to height of the screen minus 3 rows for the box itself
    //
    NumberOfLines = MIN(NumberOfLines, Rows - 3);

    //
    // Limit MaxLength to width of the screen minus 2 columns for the box itself
    //
    MaxLength = MIN(MaxLength, Columns - 2);

    //
    // Compute the starting row and starting column for the popup
    //
    if (Row < 0)
        Row = (Rows - (NumberOfLines + 3)) / 2;
    if (Column < 0)
        Column = (Columns - (MaxLength + 2)) / 2;

    //
    // Allocate a buffer for a single line of the popup with borders and a Null-terminator
    //
    Line = AllocateZeroPool((MaxLength + 3) * sizeof(CHAR16));
    ASSERT(Line != NULL);

    //
    // Draw top of popup box
    //
    SetMem16(Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
    Line[0] = BOXDRAW_DOWN_RIGHT;
    Line[MaxLength + 1] = BOXDRAW_DOWN_LEFT;
    Line[MaxLength + 2] = L'\0';
    ConOut->SetCursorPosition(ConOut, Column, Row++);
    ConOut->OutputString(ConOut, Line);

    //
    // Draw middle of the popup with strings
    //
    VA_START(Args, Key);
    while ((String = VA_ARG(Args, CHAR16*)) != NULL && NumberOfLines > 0) {
        SetMem16(Line, (MaxLength + 2) * 2, L' ');
        Line[0] = BOXDRAW_VERTICAL;
        Line[MaxLength + 1] = BOXDRAW_VERTICAL;
        Line[MaxLength + 2] = L'\0';
        ConOut->SetCursorPosition(ConOut, Column, Row);
        ConOut->OutputString(ConOut, Line);
        Length = UnicodeStringDisplayLength (String);
        if (Length <= MaxLength) {
            //
            // Length <= MaxLength
            //
            ConOut->SetCursorPosition(ConOut, Column + 1 + (MaxLength - Length) / 2, Row++);
            ConOut->OutputString(ConOut, String);
        }
        else {
            //
            // Length > MaxLength
            //
            UefiLibGetStringWidth(String, TRUE, MaxLength, &Length);
            TmpString = AllocateZeroPool((Length + 1) * sizeof(CHAR16));
            ASSERT(TmpString != NULL);
            StrnCpyS(TmpString, Length + 1, String, Length - 3);
            StrCatS(TmpString, Length + 1, L"...");

            ConOut->SetCursorPosition(ConOut, Column + 1, Row++);
            ConOut->OutputString(ConOut, TmpString);
            FreePool(TmpString);
        }
        NumberOfLines--;
    }
    VA_END(Args);

    //
    // Draw bottom of popup box
    //
    SetMem16(Line, (MaxLength + 2) * 2, BOXDRAW_HORIZONTAL);
    Line[0] = BOXDRAW_UP_RIGHT;
    Line[MaxLength + 1] = BOXDRAW_UP_LEFT;
    Line[MaxLength + 2] = L'\0';
    ConOut->SetCursorPosition(ConOut, Column, Row++);
    ConOut->OutputString(ConOut, Line);

    //
    // Free the allocated line buffer
    //
    FreePool(Line);

    //
    // Restore the cursor visibility, position, and attributes
    //
    ConOut->EnableCursor(ConOut, SavedConsoleMode.CursorVisible);
    ConOut->SetCursorPosition(ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
    ConOut->SetAttribute(ConOut, SavedConsoleMode.Attribute);

    //
    // Wait for a keystroke
    //
    if (Key != NULL) {
        while (TRUE) {
            Status = gST->ConIn->ReadKeyStroke(gST->ConIn, Key);
            if (!EFI_ERROR(Status)) {
                break;
            }

            //
            // If we encounter error, continue to read another key in.
            //
            if (Status != EFI_NOT_READY) {
                continue;
            }
            gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &EventIndex);
        }
    }
}