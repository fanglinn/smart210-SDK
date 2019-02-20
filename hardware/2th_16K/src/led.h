
#ifndef LED_H
#define LED_H


#define 	GPJ2CON 	(*(volatile unsigned long *) 0xE0200280)
#define 	GPJ2DAT		(*(volatile unsigned long *) 0xE0200284)





void led_blink()	;

/*
* index : 1~4
* state : 1->on, 0->off
*/
void led_switch(unsigned char index, unsigned char state);


void led_init(void);

#endif /* LED_H */