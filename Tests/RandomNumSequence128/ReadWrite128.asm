;PassMark MemTest86
;
;Copyright (c) 2013-2016
;	This software is the confidential and proprietary information of PassMark
;	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
;	such Confidential Information and shall use it only in accordance with
;	the terms of the license agreement you entered into with PassMark
;	Software.
;
;Program:
;	MemTest86
;
;Module:
;	ReadWrite128.asm
;
;Author(s):
;	Keith Mah
;
;Description:
;  Test 12 [Random number sequence] - This test writes a series of 
;  random numbers into memory
;

    .code

;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  ReadMem128 (
;    IN VOID   *Addr,
;    OUT UINT64	Data[2]
;    );
;------------------------------------------------------------------------------
ReadMem128  PROC
	movdqa  xmm0, [rcx]
	movdqa  [rdx], xmm0
	ret
ReadMem128  ENDP

;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  WriteMem128 (
;    IN VOID   *Addr,
;    IN UINT64	Data[2]
;    );
;------------------------------------------------------------------------------
WriteMem128  PROC
	movdqa  xmm0, [rdx]
	movdqa  [rcx], xmm0
	ret
WriteMem128  ENDP

    END
