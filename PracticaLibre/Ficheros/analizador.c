#include <LPC17xx.h>     /* Definiciones para el LPC17xx */

int32_t variable_i;

//Función que de retardo
void delay_1ms(uint32_t ms)
{
   	uint32_t n;
   	for(n=0;n<ms*14283;n++);
}

int main(void)
{
  while(1){
		for (variable_i=0;variable_i<=8;variable_i++){
			delay_1ms(10);
		}
  /*  if(variable_i==8) {
      variable_i=0;
      delay_1ms(25);     
    }
    else{
      variable_i=0;
      delay_1ms(25);     
    }*/
   }
}
