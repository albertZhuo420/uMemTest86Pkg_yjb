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
;	BlockMove.asm
;
;Author(s):
;	Keith Mah
;
;Description:
;  Test 5 [Block move, 64 moves] - This test stresses memory by using 
;  block move (movsl) instructions and is based on Robert Redelmeier's 
;  burnBX test.
;
;  Some source code taken from the original MemTest86 source code by Chris Brady, cbrady@sgi.com
;
;History
;	27 Feb 2013: Initial version

; test.c - MemTest-86  Version 4.1
;
; By Chris Brady
;

    .code

;------------------------------------------------------------------------------
; VOID*
; EFIAPI
; AsmBlockMove (
;    IN VOID   *p,
;    IN UINTN  Count
;    )
;------------------------------------------------------------------------------
AsmBlockMove  PROC    USES    rsi rax
	mov rsi, rcx                    ; rsi <- p
	mov rcx, rdx					; rcx <- Count
	mov eax,1
	jmp L100

	;.p2align 4,,7
	ALIGN 16
	L100:
	mov edx,eax
	not edx
	mov [rsi+0],eax
	mov [rsi+4],eax
	mov [rsi+8],eax
	mov [rsi+12],eax
	mov [rsi+16],edx
	mov [rsi+20],edx
	mov [rsi+24],eax
	mov [rsi+28],eax
	mov [rsi+32],eax
	mov [rsi+36],eax
	mov [rsi+40],edx
	mov [rsi+44],edx
	mov [rsi+48],eax
	mov [rsi+52],eax
	mov [rsi+56],edx
	mov [rsi+60],edx
	rcl eax,1
	lea rsi,[rsi+64]
	dec rcx
	jnz  L100

	mov rax,rsi
	ret
AsmBlockMove  ENDP

;------------------------------------------------------------------------------
; VOID
; EFIAPI
; AsmBlockMove2 (
;    IN VOID   *Source,
;    IN VOID   *Destination,
;    IN UINTN  Count
;    )
;------------------------------------------------------------------------------
AsmBlockMove2  PROC    USES    rsi rax
	mov rax, rcx
	mov rsi, rax                    ; rsi <- Source
	mov rdi, rdx					; rdi <- Destination
	mov rcx, r8						; rcx <- Count
	
	cld
	jmp L110

	;.p2align 4,,7
	ALIGN 16
	L110:
	rep	movsd

	mov rdi,rax
	add rdi,32
	mov rsi,rdx
	mov rcx,r8
	sub rcx,8
	rep movsd
	mov rdi,rax
	mov rcx,8
	rep	movsd
	ret
AsmBlockMove2  ENDP

    END
