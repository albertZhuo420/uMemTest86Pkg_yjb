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
//	BlockMove.h
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 5 [Block move, 64 moves] - This test stresses memory by using 
//  block move (movsl) instructions and is based on Robert Redelmeier's 
//  burnBX test.
//
//  Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
//
//History
//	27 Feb 2013: Initial version

/* test.c - MemTest-86  Version 4.1
 *
 * By Chris Brady
 */


#ifndef _BLOCK_MOVE_H_
#define _BLOCK_MOVE_H_

EFI_STATUS
EFIAPI
RunBlockMoveTest (
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

