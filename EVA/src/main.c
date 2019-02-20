#include "smart210.h"


void delay(unsigned long count)
{
	volatile unsigned long i = count;
	while (i--)
		;
}

int main(void)
{
	GPJ2CON = 0x00001111;		// 配置引脚
	while(1)					// 闪烁
	{
		GPJ2DAT = 0;			// LED on
		delay(0x100000);
		GPJ2DAT = 0xa;			// LED off
		delay(0x100000);
	}
}

