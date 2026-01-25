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
;	AdvancedMemoryTestASM.asm
;
;Author(s):
;	Keith Mah
;
;Description:
;	Assembly functions for RAM benchmarking taken from PerformanceTest
;

; call cpuid with args in eax, ecx
; store eax, ebx, ecx, edx to p
.CODE
       ALIGN     8

;void AdvMemTestStepReadAsm (ADV_MEM_STEP_TEST_ARGS *mta)
AdvMemTestStepReadAsm PROC FRAME
    ;Save the registers we are going to play with  
    push          rax
    push          rbx
    push          rsi
    push          rdi
    .endprolog
		;Initialise values
		mov r8, rcx					; Pointer address
		mov rax, QWORD PTR [r8+0]   ; reference Pointer address (offset 0 bytes) - memoryPtr
		mov rbx, rax				; reference Pointer address (offset 0 bytes) - memoryPtr
		mov rcx, QWORD PTR [r8+8]   ; numSteps
		mov rsi, QWORD PTR [r8+16]  ; stepSize
		mov rdi, QWORD PTR [r8+24]  ; maxMemoryPtr
      
		;Do work
		start_outer_read_loop:
		start_inner_read_loop:
		mov rdx, QWORD PTR [rbx]
		add rbx, rsi
		cmp rbx, rdi
		jl start_inner_read_loop
		mov rbx, rax
		dec rcx
		jnz start_outer_read_loop
	
	;Restore the registers we played with
    pop                 rdi
    pop                 rsi        
    pop                 rbx
    pop                 rax
    ret
AdvMemTestStepReadAsm ENDP

;void AdvMemTestStepWriteAsm (ADV_MEM_STEP_TEST_ARGS *mta)
AdvMemTestStepWriteAsm PROC FRAME
    ;Save the registers we are going to play with  
    push          rax
    push          rbx
    push          rsi
    push          rdi
    .endprolog
		;Initialise values
		mov r8, rcx						;Pointer address
		mov rax, QWORD PTR [r8+0]		; reference Pointer address (offset 0 bytes) - memoryPtr
		mov rbx, rax					; reference Pointer address (offset 0 bytes) - memoryPtr
		mov rcx, QWORD PTR [r8+8]		; numSteps
		mov edx, 18446744073709551615	; Max QWORD
		mov rsi, QWORD PTR [r8+16]		; stepSize
		mov rdi, QWORD PTR [r8+24]		; maxMemoryPtr
      
		;Do work
		start_outer_read_loop:
		start_inner_read_loop:
		mov QWORD PTR [rbx], rdx
		add rbx, rsi
		cmp rbx, rdi
		jl start_inner_read_loop
		mov rbx, rax
		dec rcx
		jnz start_outer_read_loop
	
	;Restore the registers we played with
    pop                 rdi
    pop                 rsi        
    pop                 rbx
    pop                 rax
    ret
AdvMemTestStepWriteAsm ENDP

;void AdvMemTestBlockStepReadBytesAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepReadBytesAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov dl, BYTE PTR [rbx]		;		*((BYTE*)currentPointer) = ByteRegister; //Read a BYTE
		inc rbx						;		currentPointer++; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepReadBytesAsm ENDP

;void AdvMemTestBlockStepReadWordsAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepReadWordsAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov dx, WORD PTR [rbx]		;		*((WORD*)currentPointer) = WordRegister; //Read a WORD
		add rbx, 2					;		currentPointer+=2; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepReadWordsAsm ENDP

;void AdvMemTestBlockStepReadDWordsAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepReadDWordsAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov edx, DWORD PTR [rbx]	;		*((DWORD*)currentPointer) = DWordRegister; //Read a DWORD
		add rbx, 4					;		currentPointer+=4; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepReadDWordsAsm ENDP

;void AdvMemTestBlockStepReadQWordsAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepReadQWordsAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov rdx, QWORD PTR [rbx]	;		*((QWORD*)currentPointer) = QWordRegister; //Read a QWORD
		add rbx, 8					;		currentPointer+=4; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepReadQWordsAsm ENDP

;void AdvMemTestBlockStepWriteBytesAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepWriteBytesAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;
		mov dl,  255				;Max Byte

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov BYTE PTR [rbx], dl 		;		ByteRegister = *((BYTE*)currentPointer); //Write a BYTE
		inc rbx						;		currentPointer++; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepWriteBytesAsm ENDP

;void AdvMemTestBlockStepWriteWordsAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepWriteWordsAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;
		mov dx,  65535				;Max Word

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov WORD PTR [rbx], dx 		;		WordRegister = *((WORD*)currentPointer); //Write a WORD
		add rbx, 2					;		currentPointer+=2; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepWriteWordsAsm ENDP

;void AdvMemTestBlockStepWriteDWordsAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepWriteDWordsAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;
		mov edx, 4294967295			;Max DWord

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov DWORD PTR [rbx], edx 	;		DWordRegister = *((DWORD*)currentPointer); //Write a DWORD
		add rbx, 4					;		currentPointer+=4; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepWriteDWordsAsm ENDP

;void AdvMemTestBlockStepWriteQWordsAsm (ADV_MEM_BLOCK_STEP_TEST_ARGS *mta)
AdvMemTestBlockStepWriteQWordsAsm PROC FRAME
	;Save the CPU registers we are going to play with
	push rax	;Byte Pointer to start of array
	push rbx	;Byte pointer to current array location
	push rcx	;inner loop counter
	push rdx	;write the data to, or other temporary stuff
	push rsi	;outer loops (counter)
	push rdi	;inner loops
	
	.endprolog
		;Initialise values
		mov r8, rcx					;Pointer address
		mov rax, QWORD PTR [r8+0]   ;startPointer = pBlock;
		mov rsi, QWORD PTR [r8+8]   ;outerCounter = dwLoops;
		mov rdi, QWORD PTR [r8+16]	;numInnerLoops = DataItemsPerBlock;
		mov rdx, 18446744073709551615	;Max QWord

		;Do work
		outer_loop:					;do {
		mov rbx, rax				;	currentPointer = startPointer;
		mov rcx, rdi				;	innerCounter = numInnerLoops;
		inner_loop:					;	do {
		mov QWORD PTR [rbx], rdx 	;		QWordRegister = *((QWORD*)currentPointer); //Write a QWORD
		add rbx, 8					;		currentPointer+=8; //move by number of bytes written
		dec rcx						;		innerCounter--;
		jnz inner_loop				;	} while (innerCounter != 0);
		dec rsi						;	outerCounter--;
		jnz outer_loop				;} while (outerCounter != 0);

	;pop back off
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	ret
AdvMemTestBlockStepWriteQWordsAsm ENDP

_TEXT ENDS
END