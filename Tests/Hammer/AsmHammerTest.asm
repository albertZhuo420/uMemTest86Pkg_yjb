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
;	AsmHammerTest.asm
;
;Author(s):
;	Keith Mah
;
;Description:
;  Test 13 [Hammer test] - The hammer test initializes all of memory with a pattern, 
;  hammers (ie. repeatedly read) from 2 memory addresses and then checks for flipped bits in memory
;

    .code

;------------------------------------------------------------------------------
;  VOID
;  EFIAPI
;  AsmHammerTest (
;    IN VOID   *Addr1,				// rcx, 1st address of the row pair to hammer
;    IN VOID   *Addr2,				// rdx, 2nd address of the row pair to hammer
;    IN UINT64    toggle_count,		// r8, total number of times to hammer the row pair
;    IN UINT64    delay_ticks,		// r9, number of rdtsc cycles to delay after each row pair hammer
;    );
;------------------------------------------------------------------------------
AsmHammerTest  PROC FRAME
	;Save the registers we are going to play with  
    push          rsi
    push          rdi
	push          rax
	push          rbx
	push          r12
    .endprolog

		mov rdi,r8			; rdi holds the toggle_count
		mov rsi,0			; rsi holds the counter
		mov	r12,rdx			; r12 holds Addr2 because rdx gets overwritten by rdtsc
		jmp L60
		;.p2align 4,,7
		ALIGN 16

		L60:
		cmp		r9,0		; If slow hammering (ie. delay_ticks != 0), use rdtsc
		jnz L62

		L61: ; fast hammering
		mov rax,[rcx]
		mov rbx,[rdx]
		clflush [rcx]
		clflush [rdx]

		inc rsi
		cmp rsi,rdi
		jne L61
		jmp L64

		L62: ; slow hammering
		mov rax,[rcx]
		mov rbx,[rdx]
		clflush [rcx]
		clflush [rdx]
		;mfence

		rdtsc				; get the end timestamp
		shl     rdx, 20h
        or      rax, rdx
		mov		rbx, rax
		add		rbx, r9		; rbx holds the end timestamp (current timestamp + delay_cycles)

		L63: ; loop until we reach the end timestamp
		rdtsc
		shl     rdx, 20h
        or      rax, rdx
		cmp		rax, rbx	; check whether we have reached the end timestamp
		jb L63

		; we have reached the end timestamp
		mov		rdx, r12	; restore rdx because it was overwritten by rdtsc
		inc rsi
		cmp rsi,rdi
		jne L62				; go back to hammering

	L64:
	;Restore the registers we played with
	pop					r12
	pop                 rbx
	pop                 rax
    pop                 rdi
    pop                 rsi        
	ret
AsmHammerTest  ENDP

;------------------------------------------------------------------------------
;  UINT64
;  EFIAPI
;  AsmAddrLatencyTest (
;    IN VOID   *Addr1,
;    IN VOID   *Addr2,
;    );
;------------------------------------------------------------------------------
; Not used. For measuring the latency of reading address pairs
AsmAddrLatencyTest  PROC FRAME
	;Save the registers we are going to play with  
    push          rsi
    push          rdi
	push          rbx
    .endprolog
	
		mov rsi, rcx
		mov rdi, rdx
		clflush [rsi]
		clflush [rdi]
		jmp L60
		;.p2align 4,,7
		ALIGN 16

		L60:
		rdtsc
		shl rdx, 20h
		or rax, rdx
		mov	rbx,rax

		mov rax,[rsi]
		mov rdx,[rdi]

		rdtsc
		shl rdx, 20h
		or rax, rdx
		sub rax,rbx

	;Restore the registers we played with
	pop                 rbx
    pop                 rdi
    pop                 rsi        
	ret
AsmAddrLatencyTest  ENDP

;------------------------------------------------------------------------------
;  UINT64
;  EFIAPI
;  AsmNOPLatencyTest (
;    IN UINT64    TickCount,
;    );
;------------------------------------------------------------------------------
; Not used. For measuring the latency of NOP instruction
AsmNOPLatencyTest  PROC FRAME
	;Save the registers we are going to play with  
	push          rbx
    .endprolog

		rdtsc
		shl     rdx, 20h
        or      rax, rdx
		mov		rbx, rax
		
		;mov rdi,rcx
		;mov rsi,0
		jmp L60
		;.p2align 4,,7
		ALIGN 16

		L60:
		; clear rax
		xor		rax, rax

		L61:
		inc rax
		cmp rax,rcx
		jb L61
		
		rdtsc
		shl     rdx, 20h
        or      rax, rdx
		sub		rax, rbx

	;Restore the registers we played with
	pop					rbx
	ret
AsmNOPLatencyTest  ENDP

;------------------------------------------------------------------------------
;  UINT64
;  EFIAPI
;  AsmRdtscLatencyTest (
;    );
;------------------------------------------------------------------------------
; Not used. For measuring the latency of rdtsc instruction
AsmRdtscLatencyTest  PROC FRAME
	;Save the registers we are going to play with  
	push          rbx
    .endprolog

		rdtsc
		shl     rdx, 20h
        or      rax, rdx
		mov		rbx, rax
		
		jmp L60
		;.p2align 4,,7
		ALIGN 16

		L60:
		rdtsc
		shl     rdx, 20h
        or      rax, rdx
		sub		rax, rbx

	;Restore the registers we played with
	pop					rbx
	ret
AsmRdtscLatencyTest  ENDP

    END
