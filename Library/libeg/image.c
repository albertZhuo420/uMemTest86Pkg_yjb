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
//	image.c
//
//Author(s):
//	Keith Mah
//
//Description:
//	Image handling funtions
//
//  Source code based on refit/libeg code base

/*
 * libeg/image.c
 * Image handling functions
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

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Library/UefiLib.h>

#include "libegint.h"
#if 0
#include "../refind/global.h"
#include "../refind/lib.h"
#include "../refind/screen.h"
#include "../include/refit_call_wrapper.h"
#endif
#include "lodepng.h"
#include "BmLib.h"

#define MAX_FILE_SIZE (1024*1024*1024)

//
// Basic image handling
//

EG_IMAGE * egCreateImage(IN UINTN Width, IN UINTN Height, IN BOOLEAN HasAlpha)
{
    EG_IMAGE        *NewImage;

    NewImage = (EG_IMAGE *) AllocatePool(sizeof(EG_IMAGE));
    if (NewImage == NULL)
        return NULL;
    NewImage->PixelData = (EG_PIXEL *) AllocatePool(Width * Height * sizeof(EG_PIXEL));
    if (NewImage->PixelData == NULL) {
        FreePool(NewImage);
        return NULL;
    }

    NewImage->Width = Width;
    NewImage->Height = Height;
    NewImage->HasAlpha = HasAlpha;
    return NewImage;
}

EG_IMAGE * egCreateFilledImage(IN UINTN Width, IN UINTN Height, IN BOOLEAN HasAlpha, IN EG_PIXEL *Color)
{
    EG_IMAGE        *NewImage;

    NewImage = egCreateImage(Width, Height, HasAlpha);
    if (NewImage == NULL)
        return NULL;

    egFillImage(NewImage, Color);
    return NewImage;
}

EG_IMAGE * egCopyImage(IN EG_IMAGE *Image)
{
    EG_IMAGE        *NewImage;

    NewImage = egCreateImage(Image->Width, Image->Height, Image->HasAlpha);
    if (NewImage == NULL)
        return NULL;

    CopyMem(NewImage->PixelData, Image->PixelData, Image->Width * Image->Height * sizeof(EG_PIXEL));
    return NewImage;
}

// Returns a smaller image composed of the specified crop area from the larger area.
// If the specified area is larger than is in the original, returns NULL.
EG_IMAGE * egCropImage(IN EG_IMAGE *Image, IN UINTN StartX, IN UINTN StartY, IN UINTN Width, IN UINTN Height) {
   EG_IMAGE *NewImage = NULL;
   UINTN x, y;

   if (((StartX + Width) > Image->Width) || ((StartY + Height) > Image->Height))
      return NULL;

   NewImage = egCreateImage(Width, Height, Image->HasAlpha);
   if (NewImage == NULL)
      return NULL;

   for (y = 0; y < Height; y++) {
      for (x = 0; x < Width; x++) {
         NewImage->PixelData[y * NewImage->Width + x] = Image->PixelData[(y + StartY) * Image->Width + x + StartX];
      }
   }
   return NewImage;
} // EG_IMAGE * egCropImage()

VOID egFreeImage(IN EG_IMAGE *Image)
{
    if (Image != NULL) {
        if (Image->PixelData != NULL)
            FreePool(Image->PixelData);
        FreePool(Image);
    }
}

//
// Basic file operations
//

EFI_STATUS egLoadFile(IN EFI_FILE* BaseDir, IN CHAR16 *FileName, OUT UINT8 **FileData, OUT UINTN *FileDataLength)
{
    EFI_STATUS          Status;
    EFI_FILE_HANDLE     FileHandle;
    EFI_FILE_INFO       *FileInfo;
    UINT64              ReadSize;
    UINTN               BufferSize;
    UINT8               *Buffer;

    Status = BaseDir->Open(BaseDir, &FileHandle, FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    FileInfo = EfiLibFileInfo(FileHandle);
    if (FileInfo == NULL) {
        FileHandle->Close(FileHandle);
        return EFI_NOT_FOUND;
    }
    ReadSize = FileInfo->FileSize;
    if (ReadSize > MAX_FILE_SIZE)
        ReadSize = MAX_FILE_SIZE;
    FreePool(FileInfo);

    BufferSize = (UINTN)ReadSize;   // was limited to 1 GB above, so this is safe
    Buffer = (UINT8 *) AllocatePool(BufferSize);
    if (Buffer == NULL) {
        FileHandle->Close(FileHandle);
        return EFI_OUT_OF_RESOURCES;
    }

    Status = FileHandle->Read(FileHandle, &BufferSize, Buffer);
    FileHandle->Close(FileHandle);
    if (EFI_ERROR(Status)) {
        FreePool(Buffer);
        return Status;
    }

    *FileData = Buffer;
    *FileDataLength = BufferSize;
    return EFI_SUCCESS;
}

static EFI_GUID ESPGuid = { 0xc12a7328, 0xf81f, 0x11d2, { 0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b } };

static EFI_STATUS egFindESP(OUT EFI_FILE_HANDLE *RootDir)
{
    EFI_STATUS          Status;
    UINTN               HandleCount = 0;
    EFI_HANDLE          *Handles;

    Status = gBS->LocateHandleBuffer(ByProtocol, &ESPGuid, NULL, &HandleCount, &Handles);
    if (!EFI_ERROR(Status) && HandleCount > 0) {
        *RootDir = EfiLibOpenRoot(Handles[0]);
        if (*RootDir == NULL)
            Status = EFI_NOT_FOUND;
        FreePool(Handles);
    }
    return Status;
}

EFI_STATUS egSaveFile(IN EFI_FILE* BaseDir OPTIONAL, IN CHAR16 *FileName,
                      IN UINT8 *FileData, IN UINTN FileDataLength)
{
    EFI_STATUS          Status;
    EFI_FILE_HANDLE     FileHandle;
    UINTN               BufferSize;

    if (BaseDir == NULL) {
        Status = egFindESP(&BaseDir);
        if (EFI_ERROR(Status))
            return Status;
    }

    Status = BaseDir->Open(BaseDir, &FileHandle, FileName,
                                 EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(Status))
        return Status;

    BufferSize = FileDataLength;
    Status = FileHandle->Write(FileHandle, &BufferSize, FileData);
    FileHandle->Close(FileHandle);

    return Status;
}

//
// Loading images from files and embedded data
//

// Decode the specified image data. The IconSize parameter is relevant only
// for ICNS, for which it selects which ICNS sub-image is decoded.
// Returns a pointer to the resulting EG_IMAGE or NULL if decoding failed.
static EG_IMAGE * egDecodeAny(IN UINT8 *FileData, IN UINTN FileDataLength, IN UINTN IconSize, IN BOOLEAN WantAlpha)
{
   EG_IMAGE        *NewImage = NULL;

   NewImage = egDecodeICNS(FileData, FileDataLength, IconSize, WantAlpha);
   if (NewImage == NULL)
      NewImage = egDecodePNG(FileData, FileDataLength, IconSize, WantAlpha);
   if (NewImage == NULL)
      NewImage = egDecodeBMP(FileData, FileDataLength, IconSize, WantAlpha);

   return NewImage;
}

EG_IMAGE * egLoadImage(IN EFI_FILE* BaseDir, IN CHAR16 *FileName, IN BOOLEAN WantAlpha)
{
    EFI_STATUS      Status;
    UINT8           *FileData;
    UINTN           FileDataLength;
    EG_IMAGE        *NewImage;

    if (BaseDir == NULL || FileName == NULL)
        return NULL;

    // load file
    Status = egLoadFile(BaseDir, FileName, &FileData, &FileDataLength);
    if (EFI_ERROR(Status))
        return NULL;

    // decode it
    NewImage = egDecodeAny(FileData, FileDataLength, 128, WantAlpha);
    FreePool(FileData);

    return NewImage;
}

// Load an icon from (BaseDir)/Path, extracting the icon of size IconSize x IconSize.
// Returns a pointer to the image data, or NULL if the icon could not be loaded.
EG_IMAGE * egLoadIcon(IN EFI_FILE* BaseDir, IN CHAR16 *Path, IN UINTN IconSize)
{
    EFI_STATUS      Status;
    UINT8           *FileData;
    UINTN           FileDataLength;
    EG_IMAGE        *NewImage;

    if (BaseDir == NULL || Path == NULL)
        return NULL;

    // load file
    Status = egLoadFile(BaseDir, Path, &FileData, &FileDataLength);
    if (EFI_ERROR(Status))
       return NULL;

    // decode it
    NewImage = egDecodeAny(FileData, FileDataLength, IconSize, TRUE);
    FreePool(FileData);
    if ((NewImage->Width != IconSize) || (NewImage->Height != IconSize)) {
       Print(L"Warning: Attempt to load icon of the wrong size from '%s'\n", Path);
       FreePool(NewImage);
       NewImage = NULL;
    }

    return NewImage;
} // EG_IMAGE *egLoadIcon()

#if 0
// Returns an icon of any type from the specified subdirectory using the specified
// base name. All directory references are relative to BaseDir. For instance, if
// SubdirName is "myicons" and BaseName is "os_linux", this function will return
// an image based on "myicons/os_linux.icns" or "myicons/os_linux.png", in that
// order of preference. Returns NULL if no such file is a valid icon file.
EG_IMAGE * egLoadIconAnyType(IN EFI_FILE *BaseDir, IN CHAR16 *SubdirName, IN CHAR16 *BaseName, IN UINTN IconSize) {
   EG_IMAGE *Image = NULL;
   CHAR16 *Extension;
   CHAR16 FileName[256];
   UINTN i = 0;

   while (((Extension = FindCommaDelimited(ICON_EXTENSIONS, i++)) != NULL) && (Image == NULL)) {
      SPrint(FileName, 255, L"%s\\%s.%s", SubdirName, BaseName, Extension);
      Image = egLoadIcon(BaseDir, FileName, IconSize);
      FreePool(Extension);
   } // while()

   return Image;
} // EG_IMAGE *egLoadIconAnyType()


// Returns an icon with any extension in ICON_EXTENSIONS from either the directory
// specified by GlobalConfig.IconsDir or DEFAULT_ICONS_DIR. The input BaseName
// should be the icon name without an extension. For instance, if BaseName is
// os_linux, GlobalConfig.IconsDir is myicons, DEFAULT_ICONS_DIR is icons, and
// ICON_EXTENSIONS is "icns,png", this function will return myicons/os_linux.icns,
// myicons/os_linux.png, icons/os_linux.icns, or icons/os_linux.png, in that
// order of preference. Returns NULL if no such icon can be found. All file
// references are relative to SelfDir.
EG_IMAGE * egFindIcon(IN CHAR16 *BaseName, IN UINTN IconSize) {
   EG_IMAGE *Image = NULL;

   if (GlobalConfig.IconsDir != NULL) {
      Image = egLoadIconAnyType(SelfDir, GlobalConfig.IconsDir, BaseName, IconSize);
   }

   if (Image == NULL) {
      Image = egLoadIconAnyType(SelfDir, DEFAULT_ICONS_DIR, BaseName, IconSize);
   }

   return Image;
} // EG_IMAGE * egFindIcon()
#endif

EG_IMAGE * egPrepareEmbeddedImage(IN EG_EMBEDDED_IMAGE *EmbeddedImage, IN BOOLEAN WantAlpha)
{
    EG_IMAGE            *NewImage;
    UINT8               *CompData;
    UINTN               CompLen;
    UINTN               PixelCount;

    // sanity check
    if (EmbeddedImage->PixelMode > EG_MAX_EIPIXELMODE ||
        (EmbeddedImage->CompressMode != EG_EICOMPMODE_NONE && EmbeddedImage->CompressMode != EG_EICOMPMODE_RLE))
        return NULL;

    // allocate image structure and pixel buffer
    NewImage = egCreateImage(EmbeddedImage->Width, EmbeddedImage->Height, WantAlpha);
    if (NewImage == NULL)
        return NULL;

    CompData = (UINT8 *)EmbeddedImage->Data;   // drop const
    CompLen  = EmbeddedImage->DataLength;
    PixelCount = EmbeddedImage->Width * EmbeddedImage->Height;

    // FUTURE: for EG_EICOMPMODE_EFICOMPRESS, decompress whole data block here

    if (EmbeddedImage->PixelMode == EG_EIPIXELMODE_GRAY ||
        EmbeddedImage->PixelMode == EG_EIPIXELMODE_GRAY_ALPHA) {

        // copy grayscale plane and expand
        if (EmbeddedImage->CompressMode == EG_EICOMPMODE_RLE) {
            egDecompressIcnsRLE(&CompData, &CompLen, PLPTR(NewImage, r), PixelCount);
        } else {
            egInsertPlane(CompData, PLPTR(NewImage, r), PixelCount);
            CompData += PixelCount;
        }
        egCopyPlane(PLPTR(NewImage, r), PLPTR(NewImage, g), PixelCount);
        egCopyPlane(PLPTR(NewImage, r), PLPTR(NewImage, b), PixelCount);

    } else if (EmbeddedImage->PixelMode == EG_EIPIXELMODE_COLOR ||
               EmbeddedImage->PixelMode == EG_EIPIXELMODE_COLOR_ALPHA) {

        // copy color planes
        if (EmbeddedImage->CompressMode == EG_EICOMPMODE_RLE) {
            egDecompressIcnsRLE(&CompData, &CompLen, PLPTR(NewImage, r), PixelCount);
            egDecompressIcnsRLE(&CompData, &CompLen, PLPTR(NewImage, g), PixelCount);
            egDecompressIcnsRLE(&CompData, &CompLen, PLPTR(NewImage, b), PixelCount);
        } else {
            egInsertPlane(CompData, PLPTR(NewImage, r), PixelCount);
            CompData += PixelCount;
            egInsertPlane(CompData, PLPTR(NewImage, g), PixelCount);
            CompData += PixelCount;
            egInsertPlane(CompData, PLPTR(NewImage, b), PixelCount);
            CompData += PixelCount;
        }

    } else {

        // set color planes to black
        egSetPlane(PLPTR(NewImage, r), 0, PixelCount);
        egSetPlane(PLPTR(NewImage, g), 0, PixelCount);
        egSetPlane(PLPTR(NewImage, b), 0, PixelCount);

    }

    if (WantAlpha && (EmbeddedImage->PixelMode == EG_EIPIXELMODE_GRAY_ALPHA ||
                      EmbeddedImage->PixelMode == EG_EIPIXELMODE_COLOR_ALPHA ||
                      EmbeddedImage->PixelMode == EG_EIPIXELMODE_ALPHA)) {

        // copy alpha plane
        if (EmbeddedImage->CompressMode == EG_EICOMPMODE_RLE) {
            egDecompressIcnsRLE(&CompData, &CompLen, PLPTR(NewImage, a), PixelCount);
        } else {
            egInsertPlane(CompData, PLPTR(NewImage, a), PixelCount);
            CompData += PixelCount;
        }

    } else {
        egSetPlane(PLPTR(NewImage, a), WantAlpha ? 255 : 0, PixelCount);
    }

    return NewImage;
}

//
// Compositing
//

VOID egRestrictImageArea(IN EG_IMAGE *Image,
                         IN UINTN AreaPosX, IN UINTN AreaPosY,
                         IN OUT UINTN *AreaWidth, IN OUT UINTN *AreaHeight)
{
    if (AreaPosX >= Image->Width || AreaPosY >= Image->Height) {
        // out of bounds, operation has no effect
        *AreaWidth  = 0;
        *AreaHeight = 0;
    } else {
        // calculate affected area
        if (*AreaWidth > Image->Width - AreaPosX)
            *AreaWidth = Image->Width - AreaPosX;
        if (*AreaHeight > Image->Height - AreaPosY)
            *AreaHeight = Image->Height - AreaPosY;
    }
}

VOID egFillImage(IN OUT EG_IMAGE *CompImage, IN EG_PIXEL *Color)
{
    UINTN       i;
    EG_PIXEL    FillColor;
    EG_PIXEL    *PixelPtr;

    FillColor = *Color;
    if (!CompImage->HasAlpha)
        FillColor.a = 0;

    PixelPtr = CompImage->PixelData;
    for (i = 0; i < CompImage->Width * CompImage->Height; i++, PixelPtr++)
        *PixelPtr = FillColor;
}

VOID egFillImageArea(IN OUT EG_IMAGE *CompImage,
                     IN UINTN AreaPosX, IN UINTN AreaPosY,
                     IN UINTN AreaWidth, IN UINTN AreaHeight,
                     IN EG_PIXEL *Color)
{
    UINTN       x, y;
    EG_PIXEL    FillColor;
    EG_PIXEL    *PixelPtr;
    EG_PIXEL    *PixelBasePtr;

    egRestrictImageArea(CompImage, AreaPosX, AreaPosY, &AreaWidth, &AreaHeight);

    if (AreaWidth > 0) {
        FillColor = *Color;
        if (!CompImage->HasAlpha)
            FillColor.a = 0;

        PixelBasePtr = CompImage->PixelData + AreaPosY * CompImage->Width + AreaPosX;
        for (y = 0; y < AreaHeight; y++) {
            PixelPtr = PixelBasePtr;
            for (x = 0; x < AreaWidth; x++, PixelPtr++)
                *PixelPtr = FillColor;
            PixelBasePtr += CompImage->Width;
        }
    }
}

VOID egRawCopy(IN OUT EG_PIXEL *CompBasePtr, IN EG_PIXEL *TopBasePtr,
               IN UINTN Width, IN UINTN Height,
               IN UINTN CompLineOffset, IN UINTN TopLineOffset)
{
    UINTN       x, y;
    EG_PIXEL    *TopPtr, *CompPtr;

    for (y = 0; y < Height; y++) {
        TopPtr = TopBasePtr;
        CompPtr = CompBasePtr;
        for (x = 0; x < Width; x++) {
            *CompPtr = *TopPtr;
            TopPtr++, CompPtr++;
        }
        TopBasePtr += TopLineOffset;
        CompBasePtr += CompLineOffset;
    }
}

VOID egRawCompose(IN OUT EG_PIXEL *CompBasePtr, IN EG_PIXEL *TopBasePtr,
                  IN UINTN Width, IN UINTN Height,
                  IN UINTN CompLineOffset, IN UINTN TopLineOffset)
{
    UINTN       x, y;
    EG_PIXEL    *TopPtr, *CompPtr;
    UINTN       Alpha;
    UINTN       RevAlpha;
    UINTN       Temp;

    for (y = 0; y < Height; y++) {
        TopPtr = TopBasePtr;
        CompPtr = CompBasePtr;
        for (x = 0; x < Width; x++) {
            Alpha = TopPtr->a;
            RevAlpha = 255 - Alpha;
            Temp = (UINTN)CompPtr->b * RevAlpha + (UINTN)TopPtr->b * Alpha + 0x80;
            CompPtr->b = (UINT8)((Temp + (Temp >> 8)) >> 8);
            Temp = (UINTN)CompPtr->g * RevAlpha + (UINTN)TopPtr->g * Alpha + 0x80;
            CompPtr->g = (UINT8)((Temp + (Temp >> 8)) >> 8);
            Temp = (UINTN)CompPtr->r * RevAlpha + (UINTN)TopPtr->r * Alpha + 0x80;
            CompPtr->r = (UINT8)((Temp + (Temp >> 8)) >> 8);
            /*
            CompPtr->b = ((UINTN)CompPtr->b * RevAlpha + (UINTN)TopPtr->b * Alpha) / 255;
            CompPtr->g = ((UINTN)CompPtr->g * RevAlpha + (UINTN)TopPtr->g * Alpha) / 255;
            CompPtr->r = ((UINTN)CompPtr->r * RevAlpha + (UINTN)TopPtr->r * Alpha) / 255;
            */
            TopPtr++, CompPtr++;
        }
        TopBasePtr += TopLineOffset;
        CompBasePtr += CompLineOffset;
    }
}

VOID egComposeImage(IN OUT EG_IMAGE *CompImage, IN EG_IMAGE *TopImage, IN UINTN PosX, IN UINTN PosY)
{
    UINTN       CompWidth, CompHeight;

    CompWidth  = TopImage->Width;
    CompHeight = TopImage->Height;
    egRestrictImageArea(CompImage, PosX, PosY, &CompWidth, &CompHeight);

    // compose
    if (CompWidth > 0) {
        if (TopImage->HasAlpha) {
            egRawCompose(CompImage->PixelData + PosY * CompImage->Width + PosX, TopImage->PixelData,
                         CompWidth, CompHeight, CompImage->Width, TopImage->Width);
        } else {
            egRawCopy(CompImage->PixelData + PosY * CompImage->Width + PosX, TopImage->PixelData,
                      CompWidth, CompHeight, CompImage->Width, TopImage->Width);
        }
    }
} /* VOID egComposeImage() */

EG_IMAGE* egRotateImage(IN OUT EG_IMAGE* Image, EG_ROTATION Rotation)
{
    EG_IMAGE* NewImage = NULL;
    switch (Rotation)
    {
    case ROTATE_0:
        NewImage = egCopyImage(Image);
        break;

    case ROTATE_90:
    {
        UINTN i = 0;
        UINTN j = 0;
        UINTN DstIndex = 0;
        UINTN SrcIndex = 0;

        NewImage = egCreateImage(Image->Height, Image->Width, Image->HasAlpha);
        if (NewImage == NULL)
            return NULL;
                
        for (j = 0; j < Image->Height; j++)
        {
            for (i = 0; i < Image->Width; i++)
            {
                SrcIndex = j * Image->Width + i;
                DstIndex =  i * Image->Height + (Image->Height - j - 1);

                gBS->CopyMem(&(NewImage->PixelData[DstIndex]), &(Image->PixelData[SrcIndex]), sizeof(EG_PIXEL));
            }
        }
    }
    break;

    case ROTATE_180:
    {
        UINTN i;
        UINTN j = 0;
        UINTN TranslatedIndex = 0;
        UINTN Index = 0;

        NewImage = egCreateImage(Image->Width, Image->Height, Image->HasAlpha);
        if (NewImage == NULL)
            return NULL;

        for (j = 0; j < Image->Height; j++)
        {
            for (i = 0; i < Image->Width; i++)
            {
                Index = NewImage->Width * (NewImage->Height - j - 1) + (NewImage->Width - i - 1);
                TranslatedIndex = Image->Width * j + i;
                gBS->CopyMem(&(NewImage->PixelData[Index]), &(Image->PixelData[TranslatedIndex]), sizeof(EG_PIXEL));
            }
        }
    }
    break;

    case ROTATE_270:
    {
        UINTN i;
        UINTN j = 0;
        UINTN TranslatedIndex = 0;
        UINTN Index = 0;

        UINTN OriginalWidth = Image->Width;
        UINTN OriginalHeight = Image->Height;

        NewImage = egCreateImage(OriginalHeight, OriginalWidth, Image->HasAlpha);
        if (NewImage == NULL)
            return NULL;

        for (j = 0; j < OriginalHeight; j++)
        {
            for (i = 0; i < OriginalWidth; i++)
            {
                TranslatedIndex = OriginalWidth * (OriginalHeight - j - 1) + i;
                Index = NewImage->Height * i + j;
                gBS->CopyMem(&(NewImage->PixelData[Index]), &(Image->PixelData[TranslatedIndex]), sizeof(EG_PIXEL));
            }
        }
        break;
    }
    break;    
    }
    return NewImage;
}

EG_IMAGE* egFlipImage(IN OUT EG_IMAGE* Image, EG_FLIP Flip)
{
    EG_IMAGE* NewImage = NULL;

    if (Image == NULL)
        return NULL;

    switch (Flip)
    {
    case FLIP_HORIZ:
    {
        UINTN i;
        UINTN j = 0;
        UINTN TranslatedIndex = 0;
        UINTN Index = 0;

        NewImage = egCreateImage(Image->Width, Image->Height, Image->HasAlpha);
        if (NewImage == NULL)
            return NULL;

        for (j = 0; j < Image->Height; j++)
        {
            for (i = 0; i < Image->Width; i++)
            {
                Index = NewImage->Width * (NewImage->Height - j - 1) + i;
                TranslatedIndex = Image->Width * j + i;
                gBS->CopyMem(&(NewImage->PixelData[Index]), &(Image->PixelData[TranslatedIndex]), sizeof(EG_PIXEL));
            }
        }
    }
    break;

    case FLIP_VERT:
    {
        UINTN i;
        UINTN j = 0;
        UINTN TranslatedIndex = 0;
        UINTN Index = 0;

        NewImage = egCreateImage(Image->Width, Image->Height, Image->HasAlpha);
        if (NewImage == NULL)
            return NULL;

        for (j = 0; j < Image->Height; j++)
        {
            for (i = 0; i < Image->Width; i++)
            {
                Index = NewImage->Width * j + (NewImage->Width - i - 1);
                TranslatedIndex = Image->Width * j + i;
                gBS->CopyMem(&(NewImage->PixelData[Index]), &(Image->PixelData[TranslatedIndex]), sizeof(EG_PIXEL));
            }
        }
    }
    break;
    }
    return NewImage;
}

EG_IMAGE * egEnsureImageSize(IN EG_IMAGE *Image, IN UINTN Width, IN UINTN Height, IN EG_PIXEL *Color)
{
    EG_IMAGE *NewImage;

    if (Image == NULL)
        return NULL;
    if (Image->Width == Width && Image->Height == Height)
        return Image;

    NewImage = egCreateFilledImage(Width, Height, Image->HasAlpha, Color);
    if (NewImage == NULL) {
        egFreeImage(Image);
        return NULL;
    }
    Image->HasAlpha = FALSE;
    egComposeImage(NewImage, Image, 0, 0);
    egFreeImage(Image);

    return NewImage;
}

//
// misc internal functions
//

VOID egInsertPlane(IN UINT8 *SrcDataPtr, IN UINT8 *DestPlanePtr, IN UINTN PixelCount)
{
    UINTN i;

    for (i = 0; i < PixelCount; i++) {
        *DestPlanePtr = *SrcDataPtr++;
        DestPlanePtr += 4;
    }
}

VOID egSetPlane(IN UINT8 *DestPlanePtr, IN UINT8 Value, IN UINTN PixelCount)
{
    UINTN i;

    for (i = 0; i < PixelCount; i++) {
        *DestPlanePtr = Value;
        DestPlanePtr += 4;
    }
}

VOID egCopyPlane(IN UINT8 *SrcPlanePtr, IN UINT8 *DestPlanePtr, IN UINTN PixelCount)
{
    UINTN i;

    for (i = 0; i < PixelCount; i++) {
        *DestPlanePtr = *SrcPlanePtr;
        DestPlanePtr += 4, SrcPlanePtr += 4;
    }
}

/* EOF */
