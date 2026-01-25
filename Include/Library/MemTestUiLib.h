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
//	MemTestUiLib.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	UI functions for MemTest86
//
//References:
//  https://github.com/jljusten/MemTestPkg
//
//History
//	27 Feb 2013: Initial version

/** @file
  Memory Test UI library class interface

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

#ifndef _MT_UI_LIB_H_INCLUDED_
#define _MT_UI_LIB_H_INCLUDED_

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/libeg/libeg.h>

// MtUiSetTestName
// 
// Set the current test name in the UI
VOID
MtUiSetTestName (
  IN CHAR16        *TestName
  );

// MtUiSetPattern
// 
// Set the current test pattern in the UI
VOID
MtUiSetPattern (
  IN UINTN         Pattern
  );

// MtUiSetPattern64
// 
// Set the current 64-bit test pattern in the UI
VOID
MtUiSetPattern64 (
  IN UINT64         Pattern
  );

// MtUiSetPattern128
// 
// Set the current 128-bit test pattern in the UI
VOID
MtUiSetPattern128 (
  IN UINT32         Pattern[4]
  );

// MtUiUpdatePattern
// 
// Update the test pattern in the UI
VOID
MtUiUpdatePattern(BOOLEAN Redraw);


// MtUiSetAddressRange
// 
// Set the current address range in the UI
VOID
MtUiSetAddressRange (
  IN UINT64	       Start,
  IN UINT64	       End
  );


// MtUiSetTestsCompleted
// 
// Set the number of tests completed in the current pass (progress bar)
VOID
EFIAPI
MtUiSetTestsCompleted (
  IN UINTN        NumCompleted,
  IN UINTN        NumTest
  );

// MtUiSetPassNo
// 
// Set the current pass in the UI
VOID
EFIAPI
MtUiSetPassNo (
  IN UINTN        Pass
  );

// MtUiSetErrorCount
// 
// Update the error count in the UI
VOID
EFIAPI
MtUiSetErrorCount (
  IN UINTN        NumError
  );

// MtUiPrint
// 
// Print a line of text in the space below the test progress
UINTN
EFIAPI
MtUiPrint (
  IN UINT8 Colour,
  IN CONST CHAR16  *Format,
  ...
  );

// MtUiPrintReset
// 
// Clear all text in the space below the test progress
VOID 
EFIAPI
MtUiPrintReset();

#ifdef APDEBUG
UINTN
EFIAPI
MtUiAPPrint (
  IN CONST CHAR16  *Format,
  ...
  );

VOID
EFIAPI
MtUiAPInitBuf ( );

VOID
EFIAPI
MtUiAPFlushBuf ( );
#endif


// MtUiDebugPrint
// 
// For debug output to the log file for each individual test. Disabled by default
#ifdef APDEBUG
#define MtUiDebugPrint(format, ...) \
	if (ProcNum == MPSupportGetBspId()) \
	{ \
		UnicodeSPrint(g_wszBuffer, 1024, format, __VA_ARGS__); \
		MtSupportUCDebugWriteLine(g_wszBuffer); \
	} \
	else \
		MtUiAPPrint(format, __VA_ARGS__);
#else
#define MtUiDebugPrint(format, ...)
#endif

// ConsolePrintXY
// 
// Print on the console screen at the specified coordinates
#define ConsolePrintXY(x,y,format,...) \
	gST->ConOut->SetCursorPosition(gST->ConOut, x >= 0 ? x : gST->ConOut->Mode->CursorColumn, y >= 0 ? y : gST->ConOut->Mode->CursorRow); \
	Print(format, ##__VA_ARGS__);

// MtUiSetProgressTotal
// 
// Set the maximum value for the test progress bar
VOID
EFIAPI
MtUiSetProgressTotal (
  IN UINT64   Total
  );

// MtUiUpdateProgress
// 
// Set the current value for the test progress bar
VOID
EFIAPI
MtUiUpdateProgress (
  IN UINT64   Progress
  );

// MtUiUpdateTime
// 
// Update the elapsed time in the UI
VOID
EFIAPI
MtUiUpdateTime ( );

// MtUiUpdateCPUState
// 
// Update the CPU state cursor for all processors
VOID
EFIAPI
MtUiUpdateCPUState ( );

// MtUiUpdateCPUTemp
// 
// Update the UI with the specified CPU temperature
VOID
EFIAPI
MtUiUpdateCPUTemp ( int iTemp );

// MtUiUpdateRAMTemp
// 
// Update the UI with the specified RAM temperature
VOID
EFIAPI
MtUiUpdateRAMTemp(int iTemp);

// MtUiShowMenu
// 
// Show the menu when the user presses 'c' or 'esc'
VOID
EFIAPI
MtUiShowMenu ( );

CHAR16
MtUiWaitForKey(UINT16 skey, EFI_INPUT_KEY *outkey);

// MtUiDisplayTestStatus
// 
// Redraw the UI
VOID
EFIAPI
MtUiDisplayTestStatus ( );

enum {
	ALIGN_LEFT = 0,
	ALIGN_CENTRE,
	ALIGN_RIGHT
};

// PrintXYAlign
// 
// (Graphics mode) Display text on the screen using the specified formatting parameteres
UINTN
EFIAPI
PrintXYAlign (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN UINT8 XAlign,
  IN CONST CHAR16  *Format,
  ...
  );

// ImagePrintXYAlign
// 
// (Graphics mode) Render text on the specified image using the specified formatting parameteres
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
  );


// ClearTextXYAlign
// 
// (Graphics mode) Erase text on the screen
UINTN
EFIAPI
ClearTextXYAlign (
  IN UINTN                            PointX,
  IN UINTN                            PointY,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN UINT8 XAlign,
  IN CONST CHAR16  *Format,
  ...
  );


// GetTextInfo
// 
// (Graphics mode) Get the width/height of the specified text
BOOLEAN GetTextInfo(IN CHAR16 *Text, UINTN *Width, UINTN *Height);

// DrawProgressBar
// 
// (Graphics mode) Draw the progress bar
BOOLEAN
EFIAPI
DrawProgressBar(IN EG_PIXEL *BarColour, IN EG_PIXEL *BorderColour, IN INTN XPos, IN INTN YPos, IN INTN Width, IN INTN Height, UINTN Percent);

// DisplayMenuOption
// 
// (Graphics mode) Display a list of menu options to select
UINTN
DisplayMenuOption(
	IN UINTN                            PointX,
	IN UINTN                            PointY,
	CHAR16*                             Text,
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL*		TextColour,
	BOOLEAN                             Selected,
	UINTN                               OptWidth
	);

// MtUiTurnOnSpeaker
// 
// Turn on/off Piezo Speaker
EFI_STATUS
MtUiTurnOnSpeaker(BOOLEAN On);

// MtUiGenerateBeep
// 
// Generate beep sound for the specified number of beeps
EFI_STATUS
MtUiGenerateBeep(UINTN NumberBeeps);

VOID
EFIAPI
MtCreatePopUpAtPos(
    IN  UINTN          Attribute,
    IN  INTN           Row,
    IN  INTN           Column,
    OUT EFI_INPUT_KEY* Key, OPTIONAL
    ...
);

#define MtCreatePopUp( Attribute, Key, ...) MtCreatePopUpAtPos( Attribute, -1, -1, Key, __VA_ARGS__)

#endif

