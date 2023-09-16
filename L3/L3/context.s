	AREA	handle_pend,CODE,READONLY
	EXTERN _global_resume_stack_pointer
	EXTERN _global_save_stack_pointer
	GLOBAL PendSV_Handler
	PRESERVE8
		
PendSV_Handler
	
	; The exception entry hardware pushes PSR,PC,LR,r12,r3-r0
	; that just leaves r4-r11
	PUSH {r4-r11}
	
	; save the old stack pointer (indirect, so 2 loads)
	LDR r0,=_global_save_stack_pointer
	LDR r0,[r0]
	STR SP,[r0]

	; load the requested new stack pointer (indirect, so 3 loads)
	LDR r0,=_global_resume_stack_pointer
	LDR r0,[r0]
	LDR SP,[r0]
	
	; magic return value to get us back to Thread mode with MSP stack
	MOV LR,#0xFFFFFFF9 

	; The exception exit hardware pops PSR,PC,LR,r12,r3-r0
	; that just leaves r4-r11
	POP {r4-r11}
	
	;return
	BX LR
	
	NOP

	END