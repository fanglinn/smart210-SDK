.globl _start

_start:

	// close watch-dog
	ldr r0, =0xE2700000
	mov r1, #0
	str r1, [r0]

	//set bit 12 (I) I-cache
	orr	r0, r0, #0x00001000
	mcr	p15, 0, r0, c1, c0, 0

	// set SP
	ldr	sp, =0xD0037D80	

	// 重定位						
	// _start当前所位于的地址：0xd0022000
	adr r0, _start  		
	// _start的链接地址:0xd0024000
	ldr r1, =_start 	
	// bss段的起始地址
	ldr r2, =bss_start
	cmp r0, r1
	beq clean_bss

copy_loop:
	ldr r3, [r0], #4    // 源
	str r3, [r1], #4	// 目的
	cmp r1, r2
	bne copy_loop


	// 清bss段
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

run_on_dram:			 
	ldr pc, =main		


halt:
	b halt
