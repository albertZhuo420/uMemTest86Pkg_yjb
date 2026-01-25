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
//	libeg.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	libeg graphics library declaration
//
//  Source code based on refit/libeg code base

/*
 * libeg/libeg.h
 * EFI graphics library header for users
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

#ifndef __LIBEG_LIBEG_H__
#define __LIBEG_LIBEG_H__

#include <Protocol/SimpleFileSystem.h>
#include <Library/MemTestSupportLib.h>
#if 0
#ifndef __MAKEWITH_GNUEFI
#include "../include/tiano_includes.h"
#endif
#endif

/* types */

typedef enum ColorTypes {
   white,
   black
} Colors;

/* This should be compatible with EFI_UGA_PIXEL */
typedef struct {
    UINT8 b, g, r, a;
} EG_PIXEL;

typedef struct {
    UINTN       Width;
    UINTN       Height;
    BOOLEAN     HasAlpha;
    EG_PIXEL    *PixelData;
} EG_IMAGE;

typedef enum {
    ROTATE_0,
    ROTATE_90,
    ROTATE_180,
    ROTATE_270
} EG_ROTATION;

typedef enum {
    FLIP_HORIZ,
    FLIP_VERT
} EG_FLIP;

#define EG_EIPIXELMODE_GRAY         (0)
#define EG_EIPIXELMODE_GRAY_ALPHA   (1)
#define EG_EIPIXELMODE_COLOR        (2)
#define EG_EIPIXELMODE_COLOR_ALPHA  (3)
#define EG_EIPIXELMODE_ALPHA        (4)
#define EG_MAX_EIPIXELMODE          EG_EIPIXELMODE_ALPHA

#define EG_EICOMPMODE_NONE          (0)
#define EG_EICOMPMODE_RLE           (1)
#define EG_EICOMPMODE_EFICOMPRESS   (2)

#define ICON_EXTENSIONS L"icns,png"

typedef struct {
    UINTN       Width;
    UINTN       Height;
    UINTN       PixelMode;
    UINTN       CompressMode;
    const UINT8 *Data;
    UINTN       DataLength;
} EG_EMBEDDED_IMAGE;

/* functions */

VOID egInitScreen(VOID);
UINT32 egGetNumModes(VOID);
BOOLEAN egGetResFromMode(UINTN *ModeWidth, UINTN *Height);
VOID egGetScreenSize(OUT UINTN *ScreenWidth, OUT UINTN *ScreenHeight);
BOOLEAN egSetScreenSize(IN OUT UINTN *ScreenWidth, IN OUT UINTN *ScreenHeight);
CHAR16 * egScreenDescription(VOID);
BOOLEAN egHasGraphicsMode(VOID);
BOOLEAN egIsGraphicsModeEnabled(VOID);
VOID egSetGraphicsModeEnabled(IN BOOLEAN Enable);
// NOTE: Even when egHasGraphicsMode() returns FALSE, you should
//  call egSetGraphicsModeEnabled(FALSE) to ensure the system
//  is running in text mode. egHasGraphicsMode() only determines
//  if libeg can draw to the screen in graphics mode.

EG_IMAGE * egCreateImage(IN UINTN Width, IN UINTN Height, IN BOOLEAN HasAlpha);
EG_IMAGE * egCreateFilledImage(IN UINTN Width, IN UINTN Height, IN BOOLEAN HasAlpha, IN EG_PIXEL *Color);
EG_IMAGE * egCopyImage(IN EG_IMAGE *Image);
EG_IMAGE * egCropImage(IN EG_IMAGE *Image, IN UINTN StartX, IN UINTN StartY, IN UINTN Width, IN UINTN Height);
VOID egFreeImage(IN EG_IMAGE *Image);

EG_IMAGE * egLoadImage(IN EFI_FILE* BaseDir, IN CHAR16 *FileName, IN BOOLEAN WantAlpha);
EG_IMAGE * egLoadIcon(IN EFI_FILE* BaseDir, IN CHAR16 *FileName, IN UINTN IconSize);
EG_IMAGE * egLoadIconAnyType(IN EFI_FILE *BaseDir, IN CHAR16 *SubdirName, IN CHAR16 *BaseName, IN UINTN IconSize);
EG_IMAGE * egFindIcon(IN CHAR16 *BaseName, IN UINTN IconSize);
EG_IMAGE * egPrepareEmbeddedImage(IN EG_EMBEDDED_IMAGE *EmbeddedImage, IN BOOLEAN WantAlpha);

EG_IMAGE * egEnsureImageSize(IN EG_IMAGE *Image, IN UINTN Width, IN UINTN Height, IN EG_PIXEL *Color);

EFI_STATUS egLoadFile(IN EFI_FILE* BaseDir, IN CHAR16 *FileName,
                      OUT UINT8 **FileData, OUT UINTN *FileDataLength);
EFI_STATUS egSaveFile(IN EFI_FILE* BaseDir OPTIONAL, IN CHAR16 *FileName,
                      IN UINT8 *FileData, IN UINTN FileDataLength);

VOID egFillImage(IN OUT EG_IMAGE *CompImage, IN EG_PIXEL *Color);
VOID egFillImageArea(IN OUT EG_IMAGE *CompImage,
                     IN UINTN AreaPosX, IN UINTN AreaPosY,
                     IN UINTN AreaWidth, IN UINTN AreaHeight,
                     IN EG_PIXEL *Color);
VOID egComposeImage(IN OUT EG_IMAGE *CompImage, IN EG_IMAGE *TopImage, IN UINTN PosX, IN UINTN PosY);
EG_IMAGE* egRotateImage(IN OUT EG_IMAGE* Image, EG_ROTATION Rotation );
EG_IMAGE* egFlipImage(IN OUT EG_IMAGE* Image, EG_FLIP Flip);

UINTN egGetFontHeight(VOID);
UINTN egGetFontCellWidth(VOID);
UINTN egComputeTextWidth(IN CHAR16 *Text);
VOID egMeasureText(IN CHAR16 *Text, OUT UINTN *Width, OUT UINTN *Height);
VOID egRenderText(IN CHAR16 *Text, IN OUT EG_IMAGE *CompImage, IN UINTN PosX, IN UINTN PosY, IN UINT8 BGBrightness);
VOID egLoadFont(IN CHAR16 *Filename);

VOID egClearScreen(IN EG_PIXEL *Color);
VOID egClearScreenArea(IN EG_PIXEL *Color, UINTN XPos, UINTN YPos, UINTN Width, UINTN Height);
VOID egDrawImage(IN EG_IMAGE *Image, IN EG_PIXEL* BgColor, IN UINTN ScreenPosX, IN UINTN ScreenPosY);
VOID egDrawImageWithTransparency(EG_IMAGE *Image, EG_IMAGE *BadgeImage, UINTN XPos, UINTN YPos, UINTN Width, UINTN Height);
VOID egDrawImageArea(IN EG_IMAGE *Image,
                     IN UINTN AreaPosX, IN UINTN AreaPosY,
                     IN UINTN AreaWidth, IN UINTN AreaHeight,
                     IN UINTN ScreenPosX, IN UINTN ScreenPosY);
VOID egTakeImage(IN EG_IMAGE *Image, UINTN ScreenPosX, UINTN ScreenPosY,
                 IN UINTN AreaWidth, IN UINTN AreaHeight);
VOID egDisplayMessage(IN CHAR16 *Text, EG_PIXEL *BGColor);
EG_IMAGE * egCopyScreen(VOID);
VOID egScreenShot(VOID);
BOOLEAN egSetTextMode(UINT32 RequestedMode);

VOID egFillArea(EG_IMAGE *Image, IN EG_PIXEL *Color, UINTN XPos, UINTN YPos, UINTN Width, UINTN Height);

VOID
egDrawLine(
  EG_IMAGE *Image,
  int x1,
  int y1,
  int x2,
  int y2,
  int width,
  IN EG_PIXEL *color
);

VOID
egDrawRectangle(
  EG_IMAGE *Image, 
  int x,
  int y,
  int width,
  int height,
  EG_PIXEL *color
);

VOID
egDrawEllipse(
  EG_IMAGE *Image, 
  int x,
  int y,
  int width,
  int height,
  EG_PIXEL *color
);

BOOLEAN egSaveScreenImage(EFI_FILE_HANDLE FileHandle, UINTN ScreenPosX, UINTN ScreenPosY,
                 IN UINTN AreaWidth, IN UINTN AreaHeight);

BOOLEAN egSaveImage(MT_HANDLE FileHandle, EG_IMAGE *Image);

#endif /* __LIBEG_LIBEG_H__ */

/* EOF */
