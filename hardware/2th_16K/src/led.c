
#include "led.h"

static void delay(int r0)					// ÑÓÊ±
{
    volatile int count = r0;
    while (count--)
        ;
}

/*
* index : 1~4
* state : 1->on, 0->off
*/
void led_switch(unsigned char index, unsigned char state)
{
	switch(index)
	{
		case 1:
			if(state)
			{
					GPJ2DAT &= ~(1 << 0);	
			}
			else
			{
					GPJ2DAT |= (1 << 0);	
			}
			break;
			
		case 2:
			if(state)
			{
					GPJ2DAT &= ~(1 << 1);	
			}
			else
			{
					GPJ2DAT |= (1 << 1);	
			}
			break;
			
		case 3:
			if(state)
			{
					GPJ2DAT &= ~(1 << 2);	
			}
			else
			{
					GPJ2DAT |= (1 << 2);	
			}
			break;
			
		case 4:
			if(state)
			{
					GPJ2DAT &= ~(1 << 3);	
			}
			else
			{
					GPJ2DAT |= (1 << 3);	
			}
			break;	
			
		default:
			break;
	}
}

void led_blink()					
{
	GPJ2CON = 0x00001111;		
	while(1)							
	{
		GPJ2DAT = 0;				// LED on
		delay(0x100000);
		GPJ2DAT = 0xf;				// LED off
		delay(0x100000);
	}
}


void led_init(void)
{
	GPJ2CON |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);	
}
