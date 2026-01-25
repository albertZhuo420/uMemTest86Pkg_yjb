//PassMark MemTest86
//
//Copyright (c) 2022
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
//	DMATest.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	[DMA test, disk transfer] - This test writes and reads a series of 
//  random numbers into disk via DMA.
//
//History
//	19 Jan 2022: Initial version


#ifndef _DMA_TEST_H_
#define _DMA_TEST_H_

EFI_STATUS
EFIAPI
RunDMATest (
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

