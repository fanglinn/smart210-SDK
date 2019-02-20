/*
* File      : start.S
* Create on : Jan 6 , 2019
*/


.global _start
_start:
	ldr	r0, =0xE2700000			 // close watchdog
	mov	r1, #0
	str	r1, [r0]		
	
	ldr	sp, =0xD0037D80		   // defined by samsung
	
	adr r0, _start           // get current address , 0xd0020000
	ldr r1, =_start          // get ld address , 0xd0024000 ~ 0xd0035000 (84K)
	
	ldr r2, =bss_start
	
	cmp r0, r1
	beq clean_bss            // clear bss
	
copy_loop:
	ldr r3, [r0], #4         // src
	str r3, [r1], #4       	 // dest
	cmp r1, r2
	bne copy_loop	

run_on_dram:			 
	ldr pc, =main	           // run on the new address	
	
	
clean_bss:
	ldr r0, =bss_start					
	ldr r1, =bss_end
	cmp r0, r1
	beq run_on_dram
	mov r2, #0
clear_loop:
	str r2, [r0], #4
	cmp r0, r1
	bne clear_loop
	

halt:
	b halt