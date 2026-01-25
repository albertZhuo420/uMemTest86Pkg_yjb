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
//	MovingInv32bitPattern.h
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 6 [Moving inversions, 32 bit pat] - This is a variation of the 
//  moving inversions algorithm that shifts the data pattern left 
//  one bit for each successive address.
//
//  Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
//
//History
//	27 Feb 2013: Initial version

/* test.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */

#ifndef _MOVING_INV_32BIT_PATTERN_H_
#define _MOVING_INV_32BIT_PATTERN_H_

EFI_STATUS
EFIAPI
RunMovingInv32bitPatternTest (
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

