//PassMark MemTest86
//
//Copyright (c) 2016
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
//	win32_defines.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	Glue code for modules using win32 types

#ifndef _WIN32_DEFINES_H_
#define _WIN32_DEFINES_H_

#define WORD UINT16
#define DWORD UINT32
typedef DWORD *PDWORD;
#define BYTE UINT8
typedef BYTE *PBYTE;
#define UINT_PTR UINTN
#define UINT UINT32
#define BOOL BOOLEAN
#define bool BOOLEAN
#define true TRUE
#define false FALSE
#define wchar_t CHAR16

//---------------------------------------------------------------------
//	Generic Defines
//---------------------------------------------------------------------
#define	VSHORT_STRING_LEN		16		//Size of a generic very short string
#define	SHORT_STRING_LEN		64		//Size of a generic short string
#define	LONG_STRING_LEN			256		//Size of a generic long string
#define	VLONG_STRING_LEN		1024	//Size of a generic very long string

#endif