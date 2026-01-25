//PassMark MemTest86
//
//Copyright (c) 2013-2016
//  This software is the confidential and proprietary information of PassMark
//  Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//  such Confidential Information and shall use it only in accordance with
//  the terms of the license agreement you entered into with PassMark
//  Software.
//
//Program:
//  MemTest86
//
//Module:
//  libeg.h
//
//Author(s):
//  Keith Mah
//
//Description:
//  libeg screen handling functinos
//
//  Source code based on refit/libeg code base

/*
 * libeg/screen.c
 * Screen handling functions
 *
 * Copyright (c) 2006 Christoph Pfisterer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Modifications copyright (c) 2012 Roderick W. Smith
 *
 * Modifications distributed under the terms of the GNU General Public
 * License (GPL) version 3 (GPLv3), a copy of which must be distributed
 * with this source code or binaries made from it.
 *
 */

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include "libeg.h"
#include "libegint.h"
#if 0
#include "../refind/screen.h"
#include "../refind/lib.h"
#include "../include/refit_call_wrapper.h"
#endif

#include<Protocol/UgaDraw.h>
#include<Protocol/GraphicsOutput.h>
#include "efiConsoleControl.h"

// Console defines and variables

static EFI_GUID ConsoleControlProtocolGuid = EFI_CONSOLE_CONTROL_PROTOCOL_GUID;
static EFI_CONSOLE_CONTROL_PROTOCOL *ConsoleControl = NULL;

static EFI_GUID UgaDrawProtocolGuid = EFI_UGA_DRAW_PROTOCOL_GUID;
static EFI_UGA_DRAW_PROTOCOL *UgaDraw = NULL;

static EFI_GUID GraphicsOutputProtocolGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
static EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput = NULL;

static BOOLEAN egHasGraphics = FALSE;
static UINTN egScreenWidth  = 800;
static UINTN egScreenHeight = 600;

//
// Screen handling
//

// Make the necessary system calls to identify the current graphics mode.
// Stores the results in the file-global variables egScreenWidth,
// egScreenHeight, and egHasGraphics. The first two of these will be
// unchanged if neither GraphicsOutput nor UgaDraw is a valid pointer.
static VOID egDetermineScreenSize(VOID) {
    EFI_STATUS Status = EFI_SUCCESS;
    UINT32 UGAWidth, UGAHeight, UGADepth, UGARefreshRate;

    // get screen size
    egHasGraphics = FALSE;
    if (GraphicsOutput != NULL) {
        egScreenWidth = GraphicsOutput->Mode->Info->HorizontalResolution;
        egScreenHeight = GraphicsOutput->Mode->Info->VerticalResolution;
        egHasGraphics = TRUE;
    } else if (UgaDraw != NULL) {
        Status = UgaDraw->GetMode(UgaDraw, &UGAWidth, &UGAHeight, &UGADepth, &UGARefreshRate);
        if (EFI_ERROR(Status)) {
            UgaDraw = NULL;   // graphics not available
        } else {
            egScreenWidth  = UGAWidth;
            egScreenHeight = UGAHeight;
            egHasGraphics = TRUE;
        }
    }
} // static VOID egDetermineScreenSize()

VOID egGetScreenSize(OUT UINTN *ScreenWidth, OUT UINTN *ScreenHeight)
{
    egDetermineScreenSize();

    if (ScreenWidth != NULL)
        *ScreenWidth = egScreenWidth;
    if (ScreenHeight != NULL)
        *ScreenHeight = egScreenHeight;
}

VOID egInitScreen(VOID)
{
    EFI_STATUS Status = EFI_SUCCESS;
#if 0
    // get protocols
    Status = gBS->LocateProtocol(&ConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
    if (EFI_ERROR(Status))
        ConsoleControl = NULL;
#endif

    Status = gBS->LocateProtocol(&UgaDrawProtocolGuid, NULL, (VOID **) &UgaDraw);
    if (EFI_ERROR(Status))
        UgaDraw = NULL;

    Status = gBS->LocateProtocol(&GraphicsOutputProtocolGuid, NULL, (VOID **) &GraphicsOutput);
    if (EFI_ERROR(Status))
        GraphicsOutput = NULL;

    egDetermineScreenSize();
}

UINT32 egGetNumModes() {
    if (GraphicsOutput != NULL)
        return GraphicsOutput->Mode->MaxMode;
    else if (UgaDraw != NULL) // UGA mode (EFI 1.x)
        return 0;
    return 0;
} 

// Convert a graphics mode (in *ModeWidth) to a width and height (returned in
// *ModeWidth and *Height, respectively).
// Returns TRUE if successful, FALSE if not (invalid mode, typically)
BOOLEAN egGetResFromMode(UINTN *ModeWidth, UINTN *Height) {
   UINTN                                 Size;
   EFI_STATUS                            Status;
   EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info = NULL;

   if ((ModeWidth != NULL) && (Height != NULL)) {
       Status = GraphicsOutput->QueryMode(GraphicsOutput, (UINT32)*ModeWidth, &Size, &Info);
      if ((Status == EFI_SUCCESS) && (Info != NULL)) {
         *ModeWidth = Info->HorizontalResolution;
         *Height = Info->VerticalResolution;
         return TRUE;
      }
   }
   return FALSE;
} // BOOLEAN egGetResFromMode()

static UINTN ConWidth;
static UINTN ConHeight;

VOID SwitchToText(IN BOOLEAN CursorEnabled)
{
    egSetGraphicsModeEnabled(FALSE);
    gST->ConOut->EnableCursor( gST->ConOut, CursorEnabled);
    // get size of text console
    if (gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &ConWidth, &ConHeight) != EFI_SUCCESS) {
        // use default values on error
        ConWidth = 80;
        ConHeight = 25;
    }
    // PrepareBlankLine();
}

BOOLEAN AllowGraphicsMode;
BOOLEAN GraphicsScreenDirty;

VOID SwitchToGraphics(VOID)
{
    if (AllowGraphicsMode && !egIsGraphicsModeEnabled()) {
        egSetGraphicsModeEnabled(TRUE);
        GraphicsScreenDirty = TRUE;
    }
}

static BOOLEAN ReadAllKeyStrokes(VOID)
{
    BOOLEAN       GotKeyStrokes;
    EFI_STATUS    Status;
    EFI_INPUT_KEY key;

    GotKeyStrokes = FALSE;
    for (;;) {
        Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        if (Status == EFI_SUCCESS) {
            GotKeyStrokes = TRUE;
            continue;
        }
        break;
    }
    return GotKeyStrokes;
}

VOID PauseForKey(VOID)
{
    UINTN index;

    Print(L"\n* Hit any key to continue *");

    if (ReadAllKeyStrokes()) {  // remove buffered key strokes
        gBS->Stall(5000000);     // 5 seconds delay
        ReadAllKeyStrokes();    // empty the buffer again
    }

    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &index);
    ReadAllKeyStrokes();        // empty the buffer to protect the menu

    Print(L"\n");
}

// Sets the screen resolution to the specified value, if possible. If *ScreenHeight
// is 0 and GOP mode is detected, assume that *ScreenWidth contains a GOP mode
// number rather than a horizontal resolution. If the specified resolution is not
// valid, displays a warning with the valid modes on GOP (UEFI) systems, or silently
// fails on UGA (EFI 1.x) systems. Note that this function attempts to set ANY screen
// resolution, even 0x0 or ridiculously large values.
// Upon success, returns actual screen resolution in *ScreenWidth and *ScreenHeight.
// These values are unchanged upon failure.
// Returns TRUE if successful, FALSE if not.
BOOLEAN egSetScreenSize(IN OUT UINTN *ScreenWidth, IN OUT UINTN *ScreenHeight) {
   EFI_STATUS                            Status = EFI_SUCCESS;
   EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
   UINTN                                 Size;
   UINT32                                ModeNum = 0;
   UINT32                                UGAWidth, UGAHeight, UGADepth, UGARefreshRate;
   BOOLEAN                               ModeSet = FALSE;

   if ((ScreenWidth == NULL) || (ScreenHeight == NULL))
      return FALSE;

   if (GraphicsOutput != NULL) { // GOP mode (UEFI)
      if (*ScreenHeight == 0) { // User specified a mode number (stored in *ScreenWidth); use it directly
         ModeNum = (UINT32) *ScreenWidth;
         if (egGetResFromMode(ScreenWidth, ScreenHeight) &&
             (GraphicsOutput->SetMode(GraphicsOutput, ModeNum) == EFI_SUCCESS)) {
            ModeSet = TRUE;
         }

      // User specified width & height; must find mode...
      } else {
         // Do a loop through the modes to see if the specified one is available;
         // and if so, switch to it....
         do {
            Status = GraphicsOutput->QueryMode(GraphicsOutput, ModeNum, &Size, &Info);
            if ((Status == EFI_SUCCESS) && (Size >= sizeof(*Info) && (Info != NULL)) &&
                (Info->HorizontalResolution == *ScreenWidth) && (Info->VerticalResolution == *ScreenHeight)) {
               Status = GraphicsOutput->SetMode(GraphicsOutput, ModeNum);
               ModeSet = (Status == EFI_SUCCESS);
            } // if
         } while ((++ModeNum < GraphicsOutput->Mode->MaxMode) && !ModeSet);
      } // if/else

      if (ModeSet) {
         egScreenWidth = *ScreenWidth;
         egScreenHeight = *ScreenHeight;
      } else {// If unsuccessful, display an error message for the user....
#if 0
         SwitchToText(FALSE);
         Print(L"Error setting graphics mode %d x %d; using default mode!\nAvailable modes are:\n", *ScreenWidth, *ScreenHeight);
         ModeNum = 0;
         do {
            Status = GraphicsOutput->QueryMode(GraphicsOutput, ModeNum, &Size, &Info);
            if ((Status == EFI_SUCCESS) && (Info != NULL)) {
               Print(L"Mode %d: %d x %d\n", ModeNum, Info->HorizontalResolution, Info->VerticalResolution);
            } // else
         } while (++ModeNum < GraphicsOutput->Mode->MaxMode);
         PauseForKey();
         SwitchToGraphics();
#endif
      } // if GOP mode (UEFI)

   } else if (UgaDraw != NULL) { // UGA mode (EFI 1.x)
      // Try to use current color depth & refresh rate for new mode. Maybe not the best choice
      // in all cases, but I don't know how to probe for alternatives....
      Status = UgaDraw->GetMode(UgaDraw, &UGAWidth, &UGAHeight, &UGADepth, &UGARefreshRate);
      Status = UgaDraw->SetMode(UgaDraw, (UINT32)*ScreenWidth, (UINT32)*ScreenHeight, UGADepth, UGARefreshRate);
      if (Status == EFI_SUCCESS) {
         egScreenWidth = *ScreenWidth;
         egScreenHeight = *ScreenHeight;
         ModeSet = TRUE;
      } else {
#if 0
         // TODO: Find a list of supported modes and display it.
         // NOTE: Below doesn't actually appear unless we explicitly switch to text mode.
         // This is just a placeholder until something better can be done....
         Print(L"Error setting graphics mode %d x %d; unsupported mode!\n");
#endif
      } // if/else
   } // if/else if UGA mode (EFI 1.x)
   return (ModeSet);
} // BOOLEAN egSetScreenSize()

#define DONT_CHANGE_TEXT_MODE 1024 /* textmode # that's a code to not change the text mode */

// Set a text mode.
// Returns TRUE if the mode actually changed, FALSE otherwise.
// Note that a FALSE return value can mean either an error or no change
// necessary.
BOOLEAN egSetTextMode(UINT32 RequestedMode) {
   UINTN         i = 0, Width, Height;
   EFI_STATUS    Status;
   BOOLEAN       ChangedIt = FALSE;

   if ((RequestedMode != DONT_CHANGE_TEXT_MODE) && ((INT32)RequestedMode != gST->ConOut->Mode->Mode)) {
      Status = gST->ConOut->SetMode(gST->ConOut, RequestedMode);
      if (Status == EFI_SUCCESS) {
         ChangedIt = TRUE;
      } else {
         SwitchToText(FALSE);
         Print(L"\nError setting text mode %d; available modes are:\n", RequestedMode);
         do {
            Status = gST->ConOut->QueryMode(gST->ConOut, i, &Width, &Height);
            if (Status == EFI_SUCCESS)
               Print(L"Mode %d: %d x %d\n", i, Width, Height);
         } while (++i < (UINTN)gST->ConOut->Mode->MaxMode);
         Print(L"Mode %d: Use default mode\n", DONT_CHANGE_TEXT_MODE);

         PauseForKey();
         SwitchToGraphics();
      } // if/else successful change
   } // if need to change mode
   return ChangedIt;
} // BOOLEAN egSetTextMode()

// Merges two strings, creating a new one and returning a pointer to it.
// If AddChar != 0, the specified character is placed between the two original
// strings (unless the first string is NULL or empty). The original input
// string *First is de-allocated and replaced by the new merged string.
// This is similar to StrCat, but safer and more flexible because
// MergeStrings allocates memory that's the correct size for the
// new merged string, so it can take a NULL *First and it cleans
// up the old memory. It should *NOT* be used with a constant
// *First, though....
VOID MergeStrings(IN OUT CHAR16 **First, IN CHAR16 *Second, CHAR16 AddChar) {
   UINTN Length1 = 0, Length2 = 0;
   CHAR16* NewString;

   if (*First != NULL)
      Length1 = StrLen(*First);
   if (Second != NULL)
      Length2 = StrLen(Second);
   UINTN BufSize = Length1 + Length2 + 2;
   NewString = AllocatePool(sizeof(CHAR16) * BufSize);
   if (NewString != NULL) {
      if ((*First != NULL) && (StrLen(*First) == 0)) {
         FreePool(*First);
         *First = NULL;
      }
      NewString[0] = L'\0';
      if (*First != NULL) {
         StrCatS(NewString, BufSize, *First);
         if (AddChar) {
            NewString[Length1] = AddChar;
            NewString[Length1 + 1] = '\0';
         } // if (AddChar)
      } // if (*First != NULL)
      if (Second != NULL)
         StrCatS(NewString, BufSize, Second);
      FreePool(*First);
      *First = NewString;
   } else {
      Print(L"Error! Unable to allocate memory in MergeStrings()!\n");
   } // if/else
} // static CHAR16* MergeStrings()

CHAR16 * egScreenDescription(VOID)
{
    CHAR16 *GraphicsInfo, *TextInfo = NULL;

    GraphicsInfo = AllocateZeroPool(256 * sizeof(CHAR16));
    if (GraphicsInfo == NULL)
       return L"memory allocation error";

    if (egHasGraphics) {
        if (GraphicsOutput != NULL) {
            UnicodeSPrint(GraphicsInfo, 255, L"Graphics Output (UEFI), %dx%d", egScreenWidth, egScreenHeight);
        } else if (UgaDraw != NULL) {
            GraphicsInfo = AllocateZeroPool(256 * sizeof(CHAR16));
            UnicodeSPrint(GraphicsInfo, 255, L"UGA Draw (EFI 1.10), %dx%d", egScreenWidth, egScreenHeight);
        } else {
            FreePool(GraphicsInfo);
            FreePool(TextInfo);
            return L"Internal Error";
        }
        if (!AllowGraphicsMode) { // graphics-capable HW, but in text mode
           TextInfo = AllocateZeroPool(256 * sizeof(CHAR16));
           UnicodeSPrint(TextInfo, 255, L"(in %dx%d text mode)", ConWidth, ConHeight);
           MergeStrings(&GraphicsInfo, TextInfo, L' ');
        }
    } else {
        UnicodeSPrint(GraphicsInfo, 255, L"Text-foo console, %dx%d", ConWidth, ConHeight);
    }
    FreePool(TextInfo);
    return GraphicsInfo;
}

BOOLEAN egHasGraphicsMode(VOID)
{
    return egHasGraphics;
}

BOOLEAN egIsGraphicsModeEnabled(VOID)
{
    EFI_CONSOLE_CONTROL_SCREEN_MODE CurrentMode;

    // get protocols
    if (ConsoleControl == NULL) {
        EFI_STATUS Status = EFI_SUCCESS;
        Status = gBS->LocateProtocol(&ConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
        if (EFI_ERROR(Status))
            ConsoleControl = NULL;
    }

    if (ConsoleControl != NULL) {
        ConsoleControl->GetMode(ConsoleControl, &CurrentMode, NULL, NULL);
        return (CurrentMode == EfiConsoleControlScreenGraphics) ? TRUE : FALSE;
    }

    return FALSE;
}

VOID egSetGraphicsModeEnabled(IN BOOLEAN Enable)
{

    EFI_CONSOLE_CONTROL_SCREEN_MODE CurrentMode;
    EFI_CONSOLE_CONTROL_SCREEN_MODE NewMode;

    // get protocols
    if (ConsoleControl == NULL) {
        EFI_STATUS Status = EFI_SUCCESS;
        Status = gBS->LocateProtocol(&ConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
        if (EFI_ERROR(Status))
            ConsoleControl = NULL;
    }

    if (ConsoleControl != NULL) {
        ConsoleControl->GetMode(ConsoleControl, &CurrentMode, NULL, NULL);

        NewMode = Enable ? EfiConsoleControlScreenGraphics
                         : EfiConsoleControlScreenText;
        if (CurrentMode != NewMode)
           ConsoleControl->SetMode(ConsoleControl, NewMode);
    }
}

//
// Drawing to the screen
//

VOID egClearScreen(IN EG_PIXEL *Color)
{
    EFI_UGA_PIXEL FillColor;

    if (!egHasGraphics)
        return;

    if (Color != NULL) {
       FillColor.Red   = Color->r;
       FillColor.Green = Color->g;
       FillColor.Blue  = Color->b;
    } else {
       FillColor.Red   = 0x0;
       FillColor.Green = 0x0;
       FillColor.Blue  = 0x0;
    }
    FillColor.Reserved = 0;

    if (GraphicsOutput != NULL) {
        // EFI_GRAPHICS_OUTPUT_BLT_PIXEL and EFI_UGA_PIXEL have the same
        // layout, and the header from TianoCore actually defines them
        // to be the same type.
        GraphicsOutput->Blt(GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&FillColor, EfiBltVideoFill,
                             0, 0, 0, 0, egScreenWidth, egScreenHeight, 0);
    } else if (UgaDraw != NULL) {
        UgaDraw->Blt(UgaDraw, &FillColor, EfiUgaVideoFill, 0, 0, 0, 0, egScreenWidth, egScreenHeight, 0);
    }
}

VOID egClearScreenArea(IN EG_PIXEL *Color, UINTN XPos, UINTN YPos, UINTN Width, UINTN Height)
{
    EFI_UGA_PIXEL FillColor;

    if (!egHasGraphics)
        return;

    if (Width == 0 || Height == 0)
        return;

    if (Color != NULL) {
       FillColor.Red   = Color->r;
       FillColor.Green = Color->g;
       FillColor.Blue  = Color->b;
    } else {
       FillColor.Red   = 0x0;
       FillColor.Green = 0x0;
       FillColor.Blue  = 0x0;
    }
    FillColor.Reserved = 0;

    if (GraphicsOutput != NULL) {
        // EFI_GRAPHICS_OUTPUT_BLT_PIXEL and EFI_UGA_PIXEL have the same
        // layout, and the header from TianoCore actually defines them
        // to be the same type.
        GraphicsOutput->Blt(GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&FillColor, EfiBltVideoFill,
                             0, 0, XPos, YPos, Width, Height, 0);
    } else if (UgaDraw != NULL) {
        UgaDraw->Blt(UgaDraw, &FillColor, EfiUgaVideoFill, 0, 0, XPos, YPos, Width, Height, 0);
    }
}

VOID egDrawImage(IN EG_IMAGE *Image, IN EG_PIXEL *BgColor, IN UINTN ScreenPosX, IN UINTN ScreenPosY)
{
    EG_IMAGE *CompImage = NULL;

    if (Image == NULL)
        return;

    // NOTE: Weird seemingly redundant tests because some placement code can "wrap around" and
    // send "negative" values, which of course become very large unsigned ints that can then
    // wrap around AGAIN if values are added to them.....
    if (!egHasGraphics || ((ScreenPosX + Image->Width) > egScreenWidth) || ((ScreenPosY + Image->Height) > egScreenHeight) ||
        (ScreenPosX > egScreenWidth) || (ScreenPosY > egScreenHeight))
        return;

    if (BgColor) {
        CompImage = egCreateFilledImage(Image->Width, Image->Height, TRUE, BgColor);
        egComposeImage(CompImage, Image, 0, 0);
    } else {
        CompImage = Image;
    } /* else {
       CompImage = egCropImage(GlobalConfig.ScreenBackground, ScreenPosX, ScreenPosY, Image->Width, Image->Height);
       if (CompImage == NULL) {
          Print(L"Error! Can't crop image in egDrawImage()!\n");
          return;
       }
       egComposeImage(CompImage, Image, 0, 0);
    } */

    if (GraphicsOutput != NULL) {
       GraphicsOutput->Blt(GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)CompImage->PixelData,
                            EfiBltBufferToVideo, 0, 0, ScreenPosX, ScreenPosY, CompImage->Width, CompImage->Height, 0);
    } else if (UgaDraw != NULL) {
       UgaDraw->Blt(UgaDraw, (EFI_UGA_PIXEL *)CompImage->PixelData, EfiUgaBltBufferToVideo,
                            0, 0, ScreenPosX, ScreenPosY, CompImage->Width, CompImage->Height, 0);
    }
    if ((CompImage != NULL /*GlobalConfig.ScreenBackground */) && (CompImage != Image))
       egFreeImage(CompImage);
} /* VOID egDrawImage() */

#if 0
// Display an unselected icon on the screen, so that the background image shows
// through the transparency areas. The BadgeImage may be NULL, in which case
// it's not composited in.
VOID egDrawImageWithTransparency(EG_IMAGE *Image, EG_IMAGE *BadgeImage, UINTN XPos, UINTN YPos, UINTN Width, UINTN Height) {
   EG_IMAGE *Background;

   Background = egCropImage(GlobalConfig.ScreenBackground, XPos, YPos, Width, Height);
   if (Background != NULL) {
      BltImageCompositeBadge(Background, Image, BadgeImage, XPos, YPos);
      egFreeImage(Background);
   }
} // VOID DrawImageWithTransparency()
#endif

VOID egDrawImageArea(IN EG_IMAGE *Image,
                     IN UINTN AreaPosX, IN UINTN AreaPosY,
                     IN UINTN AreaWidth, IN UINTN AreaHeight,
                     IN UINTN ScreenPosX, IN UINTN ScreenPosY)
{
    if (!egHasGraphics)
        return;

    egRestrictImageArea(Image, AreaPosX, AreaPosY, &AreaWidth, &AreaHeight);
    if (AreaWidth == 0)
        return;

    if (GraphicsOutput != NULL) {
        GraphicsOutput->Blt(GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Image->PixelData,
                             EfiBltBufferToVideo, AreaPosX, AreaPosY, ScreenPosX, ScreenPosY, AreaWidth, AreaHeight,
                             Image->Width * 4);
    } else if (UgaDraw != NULL) {
        UgaDraw->Blt(UgaDraw, (EFI_UGA_PIXEL *)Image->PixelData, EfiUgaBltBufferToVideo,
                             AreaPosX, AreaPosY, ScreenPosX, ScreenPosY, AreaWidth, AreaHeight, Image->Width * 4);
    }
}

VOID egTakeImage(IN EG_IMAGE *Image, UINTN ScreenPosX, UINTN ScreenPosY,
                 IN UINTN AreaWidth, IN UINTN AreaHeight)
{
//  if (GraphicsOutput != NULL) {
    if (ScreenPosX + AreaWidth > egScreenWidth)
    {
      AreaWidth = egScreenWidth - ScreenPosX;
    }
    if (ScreenPosY + AreaHeight > egScreenHeight)
    {
      AreaHeight = egScreenHeight - ScreenPosY;
    }
    
    if (GraphicsOutput != NULL) {
      GraphicsOutput->Blt(GraphicsOutput,
                          (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Image->PixelData,
                          EfiBltVideoToBltBuffer,
                          ScreenPosX,
                          ScreenPosY,
                          0, 0, AreaWidth, AreaHeight, (UINTN)Image->Width * 4);
    } else if (UgaDraw != NULL) {
      UgaDraw->Blt(UgaDraw,
                   (EFI_UGA_PIXEL *)Image->PixelData,
                   EfiUgaVideoToBltBuffer,
                   ScreenPosX,
                   ScreenPosY,
                   0, 0, AreaWidth, AreaHeight, (UINTN)Image->Width * 4);
    }

//  }
}

// Display a message in the center of the screen, surrounded by a box of the
// specified color. For the moment, uses graphics calls only. (It still works
// in text mode on GOP/UEFI systems, but not on UGA/EFI 1.x systems.)
VOID egDisplayMessage(IN CHAR16 *Text, EG_PIXEL *BGColor) {
   UINTN BoxWidth, BoxHeight;
   EG_IMAGE *Box;

   if ((Text != NULL) && (BGColor != NULL)) {
      egMeasureText(Text, &BoxWidth, &BoxHeight);
      BoxWidth += 14;
      BoxHeight *= 2;
      if (BoxWidth > egScreenWidth)
         BoxWidth = egScreenWidth;
      Box = egCreateFilledImage(BoxWidth, BoxHeight, FALSE, BGColor);
      egRenderText(Text, Box, 7, BoxHeight / 4, (BGColor->r + BGColor->g + BGColor->b) / 3);
      egDrawImage(Box, NULL, (egScreenWidth - BoxWidth) / 2, (egScreenHeight - BoxHeight) / 2);
   } // if non-NULL inputs
} // VOID egDisplayMessage()

// Copy the current contents of the display into an EG_IMAGE....
// Returns pointer if successful, NULL if not.
EG_IMAGE * egCopyScreen(VOID) {
   EG_IMAGE *Image = NULL;

   if (!egHasGraphics)
      return NULL;

   // allocate a buffer for the whole screen
   Image = egCreateImage(egScreenWidth, egScreenHeight, FALSE);
   if (Image == NULL) {
      return NULL;
   }

   // get full screen image
   if (GraphicsOutput != NULL) {
      GraphicsOutput->Blt(GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Image->PixelData,
                           EfiBltVideoToBltBuffer, 0, 0, 0, 0, Image->Width, Image->Height, 0);
   } else if (UgaDraw != NULL) {
      UgaDraw->Blt(UgaDraw, (EFI_UGA_PIXEL *)Image->PixelData, EfiUgaVideoToBltBuffer,
                           0, 0, 0, 0, Image->Width, Image->Height, 0);
   }
   return Image;
} // EG_IMAGE * egCopyScreen()

//
// Make a screenshot
//

VOID egScreenShot(VOID)
{
    EFI_STATUS      Status;
    EG_IMAGE        *Image;
    UINT8           *FileData;
    UINTN           FileDataLength;
    UINTN           Index;

    Image = egCopyScreen();
    if (Image == NULL) {
       Print(L"Error: Unable to take screen shot\n");
       goto bailout_wait;
    }

    // encode as BMP
    egEncodeBMP(Image, &FileData, &FileDataLength);
    egFreeImage(Image);
    if (FileData == NULL) {
        Print(L"Error egEncodeBMP returned NULL\n");
        goto bailout_wait;
    }

    // save to file on the ESP
    Status = egSaveFile(NULL, L"screenshot.bmp", FileData, FileDataLength);
    FreePool(FileData);
    if (EFI_ERROR(Status)) {
        Print(L"Error egSaveFile: %x\n", Status);
        goto bailout_wait;
    }

    return;

    // DEBUG: switch to text mode
bailout_wait:
    egSetGraphicsModeEnabled(FALSE);
    gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &Index);
}

EFI_STATUS
egWritePixel(EG_IMAGE *Image, UINTN x, UINTN y, IN EG_PIXEL *color)
{
  EFI_STATUS  Status = EFI_UNSUPPORTED;
  EFI_UGA_PIXEL PixelColor;
  
  if (color != NULL) {
      PixelColor.Red   = color->r;
      PixelColor.Green = color->g;
      PixelColor.Blue  = color->b;
  } else {
      PixelColor.Red   = 0x0;
      PixelColor.Green = 0x0;
      PixelColor.Blue  = 0x0;
  }
  PixelColor.Reserved = 0;
  
  if (Image == NULL)
  {
      if (!egHasGraphics)
          return Status;

      if (GraphicsOutput != NULL) {
          // EFI_GRAPHICS_OUTPUT_BLT_PIXEL and EFI_UGA_PIXEL have the same
          // layout, and the header from TianoCore actually defines them
          // to be the same type.
          Status = GraphicsOutput->Blt(GraphicsOutput, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&PixelColor, EfiBltBufferToVideo,
              0, 0, x, y, 1, 1, 0);
      } else if (UgaDraw != NULL) {
          Status = UgaDraw->Blt(UgaDraw, &PixelColor, EfiUgaBltBufferToVideo, 0, 0, x, y, 1, 1, 0);
      }
  }
  else
  {
      if (x >= Image->Width || y >= Image->Height)
          return EFI_INVALID_PARAMETER;

      Image->PixelData[y * Image->Width + x].r = PixelColor.Red;
      Image->PixelData[y * Image->Width + x].g = PixelColor.Green;
      Image->PixelData[y * Image->Width + x].b = PixelColor.Blue;

      Status = EFI_SUCCESS;
  }
  return Status;
}

VOID egFillArea(EG_IMAGE *Image, IN EG_PIXEL *Color, UINTN XPos, UINTN YPos, UINTN Width, UINTN Height)
{
    if (Image == NULL)
        egClearScreenArea(Color, XPos, YPos, Width, Height);
    else
    {
        UINTN x,y;
        if (XPos + Width > Image->Width || 
            YPos + Height > Image->Height)
            return;

        for (y = YPos; y < YPos + Height; y++)
        {
            for (x = XPos; x < XPos + Width; x++)
            {
                egWritePixel(Image, x, y, Color);
            }
        }
    }
}

VOID
egDrawLine(
  EG_IMAGE *Image, 
  int x1,
  int y1,
  int x2,
  int y2,
  int width,
  EG_PIXEL *color
)
{
    if (width <= 1)
        egBresenhamLine(Image, x1, y1, x2, y2, color);
    else
        egAADrawLine(Image, x1,y1,x2,y2, width, color);
}


// Source: http://kurtqiao.blogspot.ca/2013/07/how-to-draw-line-in-uefi.html
VOID
egBresenhamLine(
  EG_IMAGE *Image, 
  int x1,
  int y1,
  int x2,
  int y2,
  EG_PIXEL *color
)
{
  int  x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;

  dx = x2 - x1;
  dy = y2 - y1;
  dx1 = (x2 > x1)?(x2 - x1):(x1 - x2);
  dy1 = (y2 > y1)?(y2 - y1):(y1 - y2);

  px = 2*dy1 - dx1;
  py = 2*dx1 - dy1;

  if(dy1 <= dx1)
  {
    if(dx>= 0){
  x=x1;
  y=y1;
  xe=x2;
  }else{
  x=x2;
  y=y2;
  xe=x1;
  }
 egWritePixel(Image, x, y, color);
 for(i=0; x<xe;i++){
  x=x+1;
  if(px<0){
    px = px+2*dy1;
    }else{
    if (((x2>x1)&&(y2>y1))||((x1>x2)&&(y1>y2)))
     {
     y=y+1;
   }else{
   y=y-1;
   }
    px=px+2*(dy1- dx1);
    }
  egWritePixel(Image, x, y, color);
 }
  }
  else{
   if(dy>= 0){
  x=x1;
  y=y1;
  ye=y2;
  }else{
  x=x2;
  y=y2;
  ye=y1;
  }
 egWritePixel(Image,x, y, color);
 for(i=0;y<ye;i++){
  y=y+1;
  if(py<= 0){
   py=py+2*dx1;
   }else{
   if(((x2>x1)&&(y2>y1))||((x1>x2)&&(y1>y2))){
    x=x+1;
    }else{
    x=x-1;
    }
   py = py+2*(dx1-dy1);
   }
  egWritePixel(Image, x, y, color);
  }
 }

}

// source: http://stackoverflow.com/questions/10322341/simple-algorithm-for-drawing-filled-ellipse-in-c-c
VOID
egDrawEllipse(
  EG_IMAGE *Image,
  int x,
  int y,
  int width,
  int height,
  EG_PIXEL *color
)
{
    int hh = height * height;
    int ww = width * width;
    int hhww = hh * ww;
    int x0 = width;
    int dx = 0;
    int i,j;

    // do the horizontal diameter
    for (i = -width; i <= width; i++)
        egWritePixel(Image, x + i, y, color);

    // now do both halves at the same time, away from the diameter
    for (i = 1; i <= height; i++)
    {
        int x1 = x0 - (dx - 1);  // try slopes of dx - 1 or more
        for ( ; x1 > 0; x1--)
            if (x1*x1*hh + y*y*ww <= hhww)
                break;
        dx = x0 - x1;  // current approximation of the slope
        x0 = x1;

        for (j = -x0; j <= x0; j++)
        {
            egWritePixel(Image, x + j, y - i, color);
            egWritePixel(Image, x + j, y + i, color);
        }
    }
}

// source: http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
UINT32 squareroot(UINT32 a_nInput)
{
    UINT32 op  = a_nInput;
    UINT32 res = 0;
    UINT32 one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for uint32_t type


    // "one" starts at the highest power of four <= than the argument.
    while (one > op)
    {
        one >>= 2;
    }

    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }
    return res;
}

// source: http://members.chello.at/easyfilter/bresenham.html
void egAADrawLine(EG_IMAGE *Image, int x0, int y0, int x1, int y1, int wd, EG_PIXEL *color)
{ 
   int dx = ABS(x1-x0), sx = x0 < x1 ? 1 : -1; 
   int dy = ABS(y1-y0), sy = y0 < y1 ? 1 : -1; 
   int err = dx-dy, e2, x2, y2;                          /* error value e_xy */
   int ed = dx+dy == 0 ? 1 : squareroot(dx*dx+dy*dy);
   EG_PIXEL LineColour;

   LineColour.r = color->r;
   LineColour.g = color->g;
   LineColour.b = color->b;
   
   for (wd = (wd+1)/2; ; ) {                                   /* pixel loop */
       LineColour.r = (UINT8)MAX(color->r,color->r*(ABS(err-dx+dy)/ed-wd+1));
       LineColour.g = (UINT8)MAX(color->g,color->g*(ABS(err-dx+dy)/ed-wd+1));
       LineColour.b = (UINT8)MAX(color->b,color->b*(ABS(err-dx+dy)/ed-wd+1));
      egWritePixel(Image, x0,y0,&LineColour);
      e2 = err; x2 = x0;
      if (2*e2 >= -dx) {                                           /* x step */
         for (e2 += dy, y2 = y0; e2 < ed*wd && (y1 != y2 || dx > dy); e2 += dx)
         {
             LineColour.r = (UINT8)MAX(color->r,color->r*(ABS(e2)/ed-wd+1));
             LineColour.g = (UINT8)MAX(color->g,color->g*(ABS(e2)/ed-wd+1));
             LineColour.b = (UINT8)MAX(color->b,color->b*(ABS(e2)/ed-wd+1));
            egWritePixel(Image, x0, y2 += sy, &LineColour);
         }
         if (x0 == x1) break;
         e2 = err; err -= dy; x0 += sx; 
      } 
      if (2*e2 <= dy) {                                            /* y step */
         for (e2 = dx-e2; e2 < ed*wd && (x1 != x2 || dx < dy); e2 += dy)
         {
             LineColour.r = (UINT8)MAX(color->r,color->r*(ABS(e2)/ed-wd+1));
             LineColour.g = (UINT8)MAX(color->g,color->g*(ABS(e2)/ed-wd+1));
             LineColour.b = (UINT8)MAX(color->b,color->b*(ABS(e2)/ed-wd+1));
            egWritePixel(Image, x2 += sx, y0, &LineColour);
         }
         if (y0 == y1) break;
         err += dx; y0 += sy; 
      }
   }
}

VOID
egDrawRectangle(
  EG_IMAGE *Image, 
  int x,
  int y,
  int width,
  int height,
  EG_PIXEL *color
)
{
    egDrawLine(Image, x,y, x + width, y, 1, color);
    egDrawLine(Image, x,y, x, y + height, 1, color);
    egDrawLine(Image, x,y + height, x + width, y + height, 1, color);
    egDrawLine(Image, x + width,y, x + width, y + height, 1, color);
}

BOOLEAN egSaveScreenImage(EFI_FILE_HANDLE FileHandle, UINTN ScreenPosX, UINTN ScreenPosY,
                 IN UINTN AreaWidth, IN UINTN AreaHeight)
{
    EFI_STATUS      Status = EFI_SUCCESS;
    EG_IMAGE        *Image;
    UINT8           *FileData;
    UINTN           FileDataLength;
    UINTN           BufferSize;

    if (FileHandle == NULL)
        return FALSE;

   // allocate a buffer for the whole screen
   Image = egCreateImage(AreaWidth, AreaHeight, FALSE);
   if (Image == NULL) {
      return FALSE;
   }

   if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt(GraphicsOutput,
                          (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)Image->PixelData,
                          EfiBltVideoToBltBuffer,
                          ScreenPosX,
                          ScreenPosY,
                          0, 0, AreaWidth, AreaHeight, 0);
    } else if (UgaDraw != NULL) {
      Status = UgaDraw->Blt(UgaDraw,
                   (EFI_UGA_PIXEL *)Image->PixelData,
                   EfiUgaVideoToBltBuffer,
                   ScreenPosX,
                   ScreenPosY,
                   0, 0, AreaWidth, AreaHeight, 0);
    }

   if (EFI_ERROR(Status)) {
       egFreeImage(Image);
       return FALSE;
    }
    // encode as BMP
    egEncodeBMP(Image, &FileData, &FileDataLength);
    egFreeImage(Image);
    if (FileData == NULL) {
        return FALSE;
    }

    // save to file on the ESP
    BufferSize = FileDataLength;
    Status = FileHandle->Write(FileHandle, &BufferSize, FileData);

    FreePool(FileData);
    if (EFI_ERROR(Status)) {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN egSaveImage(MT_HANDLE FileHandle, EG_IMAGE *Image)
{
    EFI_STATUS      Status = EFI_SUCCESS;
    UINT8           *FileData;
    UINTN           FileDataLength;
    UINTN           BufferSize;

    if (FileHandle == NULL)
        return FALSE;

    // encode as BMP
    egEncodeBMP(Image, &FileData, &FileDataLength);

    if (FileData == NULL) {
        return FALSE;
    }

    // save to file on the ESP
    BufferSize = FileDataLength;
    Status = MtSupportWriteFile(FileHandle, &BufferSize, FileData);

    FreePool(FileData);
    if (EFI_ERROR(Status)) {
        return FALSE;
    }

    return TRUE;
}
/* EOF */
