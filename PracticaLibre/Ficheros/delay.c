/* delay.c */
#include "lpc17xx.h"

void delay_1ms(uint32_t ms)
{
   	uint32_t i;
   	for(i=0;i<ms*14283;i++);
}
