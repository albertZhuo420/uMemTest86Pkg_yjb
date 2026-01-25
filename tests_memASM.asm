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
;	SupportLib.h
;
;Author(s):
;	Keith Mah
;
;Description:
;	Memory latency test functions taken from PerformanceTest
;
.CODE
       ALIGN     8


; void tests_memASM_read_cached (void* memory, UINTN loops)
; loops should not be more than the size of the buffer divide the size of a QWORD
tests_memASM_read_block PROC FRAME
    ;Save the registers we are going to play with  
    push          rbx
    .endprolog

		; Function Call Value Locations 
		; (found by looking at compiled assembler, if debugging check these are correct first)
		; rdx = loops
		; rcx = pdwMemBlock

		; do{
		do_loop_start:

			; QwordRegister = *pdwMemBlock; //Read a QWORD
			mov rbx, QWORD PTR [rcx]

			; pdwMemBlock+=8; // Move a QWORD forward
			add rcx, 8

			; Counter--;
			dec rdx

		; } while (loops > 0);
		jnz do_loop_start
	
	;Restore the registers we played with
    pop                 rbx
    ret
tests_memASM_read_block ENDP

; void tests_memASM_read_cached (void* memory, UINTN loops)
; loops should not be more than the size of the buffer divide the size of a QWORD
tests_memASM_write_block PROC FRAME
    ;Save the registers we are going to play with  
    push          rbx
    .endprolog

		; Function Call Value Locations 
		; (found by looking at compiled assembler, if debugging check these are correct first)
		; rdx = loops
		; rcx = pdwMemBlock

		; do{
		do_loop_start:

			; QwordRegister = *pdwMemBlock; //Read a QWORD
			mov QWORD PTR [rcx], rbx

			; pdwMemBlock+=8; // Move a QWORD forward
			add rcx, 8

			; Counter--;
			dec rdx

		; } while (loops > 0);
		jnz do_loop_start
	
	;Restore the registers we played with
    pop                 rbx
    ret
tests_memASM_write_block ENDP

;void tests_memASM_latency (void* memory, UINTN loops)
tests_memASM_latency PROC FRAME
    .endprolog

		; Function Call Value Locations 
		; (found by looking at compiled assembler, if debugging check these are correct first)
		; rdx = Loops
		; rcx = Buffer

		;do {
		latency_loop:
			
			;Buffer = *Buffer; //Read a DWORD		
			mov rcx, QWORD PTR [rcx]
		
			;ReadsPerLoop--;	
			dec rdx
		
		;} while (Loops != 0); 	
		jnz latency_loop

    ret
tests_memASM_latency ENDP

_TEXT ENDS
END