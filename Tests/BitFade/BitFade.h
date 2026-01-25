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
//	BitFade.h
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 9 [Bit fade test, 2 patterns] - The bit fade test initializes 
//  all of memory with a pattern and then sleeps for a few minutes. 
//
//  Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
//
//History
//	27 Feb 2013: Initial version

/* test.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */

#ifndef _BIT_FADE_H_
#define _BIT_FADE_H_

#define DEF_SLEEP_SECS			(5 * 60)		// Default Bit fade test sleep time in seconds
#define MIN_SLEEP_SECS			(3 * 60)		// Minimum Bit fade test sleep time in seconds
#define MAX_SLEEP_SECS			(10000 * 60)	// MAximum Bit fade test sleep time in seconds

EFI_STATUS
EFIAPI
RunBitFadeTest_write (
    IN UINT32                   TestNum,
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
	IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context
    );

EFI_STATUS
EFIAPI
RunBitFadeTest_verify (
    IN UINT32                   TestNum,
    IN UINTN					ProcNum,
    IN EFI_PHYSICAL_ADDRESS     Start,
    IN UINT64                   Length,
	IN UINT64                   IterCount,
    IN TESTPAT                  Pattern,
    IN UINT64                   PatternUser,
    IN VOID                     *Context
    );

#endif

