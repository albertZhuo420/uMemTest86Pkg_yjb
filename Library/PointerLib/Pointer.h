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
//	Pointer.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Include files for Pointer.c
//
//  Source code based on refit code base

#ifndef _POINTER_H_INCLUDED_
#define _POINTER_H_INCLUDED_

EFI_STATUS  MouseBirth();

VOID KillMouse();

typedef enum {
  NoEvents,
  Move,
  LeftClick,
  RightClick,
  DoubleClick,
  ScrollClick,
  ScrollDown,
  ScrollUp,
  LeftMouseDown,
  RightMouseDown,
  MouseMove
} MOUSE_EVENT;

MOUSE_EVENT WaitForMouseEvent(UINTN *pXPos, UINTN *pYPos, UINTN TimeoutMs);

EFI_EVENT GetMouseEvent();

typedef struct {
  UINTN     XPos;
  UINTN     YPos;
  UINTN     Width;
  UINTN     Height;
} EG_RECT;

BOOLEAN MouseInRect(EG_RECT *Place);

VOID HidePointer();
VOID DrawPointer();

#endif

