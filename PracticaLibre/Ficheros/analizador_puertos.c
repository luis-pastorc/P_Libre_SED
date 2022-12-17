#include <LPC17xx.H>     /* Definiciones para el LPC17xx */

int32_t sentido;

// Funcion de retardo
void delay_1ms(uint32_t ms)
{
  uint32_t i;
  for(i=0;i<ms*14283;i++);
}

int main(void)
{
  int32_t n;

	LPC_PINCON ->PINSEL0 &= ~0xFF; 
	LPC_PINCON ->PINMODE0 &= 0xFF; 
	LPC_PINCON ->PINMODE0 |= 0xAA;
  // Los pines P0.[0..3]  configurados como salidas,
  //           P0.[4..31] configurados como entradas.
  LPC_GPIO0->FIODIR=0x0000000F; 

  // Se ponen a cero los pines P0.[0..3].	
  LPC_GPIO0->FIOCLR=0x0000000F; 
  
	sentido=1;		
	n=1;
	
  while(1){
    if(sentido) {
      n=n<<1;
	  if(n==(1<<4)) n=1;
	  }
	else{
      n=n>>1;
	  if(n==0) n=(1<<3);
	  }
     // Activar la salida que corresponda.
    LPC_GPIO0->FIOPIN=(n<<0); 
    //Retardo de 25 ms.
    delay_1ms(25); 
      }
}