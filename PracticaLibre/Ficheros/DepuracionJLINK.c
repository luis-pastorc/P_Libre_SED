/* main.c */
#include "delay.h"
#include "lpc17xx.h"
 
void config_GPIO (void)
{
  LPC_GPIO3->FIODIR |= (1<<25);		/* P3.25 definido como salida */ 
	LPC_GPIO3->FIODIR |= (1<<26);		/* P3.26 definido como salida */ 
  LPC_GPIO2->FIODIR &= ~(1<<12);	/* P2.12 definido como entrada */ 
  LPC_GPIO3->FIOSET |= (1<<25);		/* apago el LED conectado a P3.25 */
}
     
  int main (void)
{ 
  
	config_GPIO();
	while(1) {

  if ((LPC_GPIO2->FIOPIN & (1<<12))==0){  /* pulsador pulsado? */
		LPC_GPIO3->FIOPIN |= (1<<26);					/* apago LED2 */
    LPC_GPIO3->FIOPIN |= (1<<25);      	 	/* apago LED1 */
    delay_1ms (750);			        				/* retardo de 750ms */
    LPC_GPIO3->FIOPIN &= ~(1<<25);      	/* enciendo LED1 */
    delay_1ms (750);											/* retardo de 750ms */
    }
  else {
		LPC_GPIO3->FIOPIN |= (1<<25);					/* apago LED1 */
		LPC_GPIO3->FIOPIN |= (1<<26);					/* apago el LED2 */
    delay_1ms (250);											/* retardo de 250ms */
    LPC_GPIO3->FIOPIN &= ~(1<<26);				/* enciendo el LED2 */
    delay_1ms (250);											/* retardo de 250ms */
    } 
  }
}



