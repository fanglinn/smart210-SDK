/*
* File      : led.S
* Create on : Jan 6 , 2019
*/


.global _start

/*
* LED1   GPJ2-0
* LED2   GPJ2-1
* LED3   GPJ2-2
* LED4   GPJ2-3
*
*
* GPJ2CON  0xE0200280
* GPJ2DAT  0xE0200284
* GPJ2PUD  0xE0200288
*
*/

_start:
	ldr r0, =0xE0200280		
	ldr r1, =0x00001111       /* SET LED 1~4  AS OUTPUT */		
	str r1, [r0]			

	ldr r0, =0xE0200284		
	ldr r1, =0x00000000		    /* SET LOW, THE LIGHT THE LED */
	str r1, [r0]				

halt:
	b halt