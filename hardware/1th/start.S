.globl _start

_start:
	// 设置GPJ2CON的bit[0:15]，配置GPJ2_0/1/2/3引脚为输出功能
	ldr r1, =0xE0200280 					
	ldr r0, =0x00001111
	str r0, [r1]

	mov r2, #0x1000
led_blink:
	// 设置GPJ2DAT的bit[0:3]，使GPJ2_0/1/2/3引脚输出低电平，LED亮
	ldr r1, =0xE0200284 					
	mov r0, #0
	str r0, [r1]

	// 延时
	bl delay							

	// 设置GPJ2DAT的bit[0:3]，使GPJ2_0/1/2/3引脚输出高电平，LED灭
	ldr r1, =0xE0200284 					
	mov r0, #0xf
	str r0, [r1]

	// 延时
	bl delay	

	sub r2, r2, #1
	cmp r2,#0
	bne led_blink


halt:
	b halt


delay:
	mov r0, #0x10000
delay_loop:
	cmp r0, #0
	sub r0, r0, #1
	bne delay_loop
	mov pc, lr


