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
//	Hammer.h
//
//Author(s):
//	Keith Mah
//
//Description:
//  Test 13 [Hammer test] - The hammer test initializes all of memory with a pattern, 
// hammers (ie. repeatedly read) from 2 memory addresses and then checks for flipped bits in memory
//
//
//History
//	13 Aug 2014: Initial version


#ifndef _HAMMER_H_
#define _HAMMER_H_


#define ROW_INC			(1 << 24)

#define NUM_ROW_TOGGLES			1000000			// Number of times to hammer address pairs
#define HAMMER_RATE_REFI		200000			// Number of times to hammer per refresh interval for slow hammering
#define REFI_MS					64				// Refresh interval time

EFI_STATUS
EFIAPI
RunHammerTest_write (
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
RunHammerTest_hammer_fast (
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
RunHammerTest_hammer_slow (
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
RunHammerTest_double_sided_hammer_fast (
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
RunHammerTest_double_sided_hammer_slow (
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
RunHammerTest_verify (
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

