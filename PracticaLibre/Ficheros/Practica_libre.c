#include "lpc17xx.h"

uint32_t SystemFrequency=100000000;

uint32_t cuenta_1ms,visualizar;  // Contador de ms 

int NUMEROS[10] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90}; //0....9
int HOLA[4] = {0x89,0xC0,0xC7,0x88};	//H O L A
int tiempo[8] = {0,0,0,0,0,0,0,0}; //, centesimas de s,decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int alarma1[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int alarma2[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int temp1[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int temp2[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int m;	//cambia cada 5ms para visualizar
int interrupciones;
int entradas;	//para gestionar el switch
int switch_horas_seg;	//elegir horas+mins o segs+decs
int switch_alarma_1;
int switch_alarma_2;
int switch_temp_1;
int switch_temp_2;
int x;	//para mostrar horas o segundos

void config_GPIO (void)
{
	//Configuración de los pines P2.10 a P2.12, como entradas de interrupción
	LPC_PINCON->PINSEL4 |= 1 << (10*2);	//EINT0
	LPC_PINCON->PINSEL4 |= 1 << (11*2); //EINT1 
	LPC_PINCON->PINSEL4 |= 1 << (12*2);	//EINT2
	
  LPC_GPIO1->FIODIR |= (127<<20);				// P1.20 hasta P1.26 definido como salida
	LPC_GPIO2->FIODIR |= (15<<0);					// P2.0 hasta P2.3 como salida
  LPC_GPIO2->FIODIR &= ~(1<<13);				// P2.13 definido como entrada - PULSADOR Reseteo timer
	LPC_GPIO0->FIODIR &= ~(1<<3);					// P0.3 definido como entrada - SELECTOR
	LPC_PINCON->PINMODE2 &= ~(3<<2*13);		//P2.13 a pull-up
	LPC_PINCON->PINMODE0 &= ~(3<<2*3);		//P0.3 a pull-up
} 

void config_Systick(void)
{
	SysTick->LOAD = 9999999;		//interrumpe cada 0.1s
	SysTick->VAL = 0;
	SysTick->CTRL = 7;
}

void config_Timer0(void)	//Interrumpe cada 5ms
 {
	LPC_SC ->PCLKSEL0 |= 1<<3;
	LPC_TIM0 ->MCR =0x3;
	LPC_TIM0 -> MR0 = 250000;
	LPC_TIM0 ->TC =0;
	LPC_TIM0 ->TCR |=(1<<0);
 }
 
void configura_Interrupciones()
{
	//EINT0 Y EINT1 activas por flanco
	//LPC_SC -> EXTMODE |= (1<<0);
	//LPC_SC -> EXTMODE |= (1<<1);
	//EINT0 ACTIVA POR SUBIDA Y EINT1 POR BAJADA
	//LPC_SC -> EXTPOLAR |= (1<<0);		//EINT0 activa por flanco de subida
	//LPC_SC -> EXTPOLAR &= ~(1<<1);	//EINT1 activa por flanco de bajada
	LPC_SC -> EXTINT |= (0x07);			//flags
	
	//Bajar el flag
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_ClearPendingIRQ(EINT1_IRQn);
	NVIC_ClearPendingIRQ(EINT2_IRQn);
	
	//Asignación de prioridades
  NVIC_SetPriorityGrouping(4);
	NVIC_SetPriority(SysTick_IRQn, 0x2);
	NVIC_SetPriority (TIMER0_IRQn, 0x2);	//000.10XXX
  NVIC_SetPriority(EINT0_IRQn, 0x6);		//001.10XXX
  NVIC_SetPriority(EINT1_IRQn, 0x6);		//001.10XXX
  NVIC_SetPriority(EINT2_IRQn, 0x6);		//001.10XXX
	

	//Habilitación de las interrupciones
  NVIC_EnableIRQ(EINT0_IRQn);
  NVIC_EnableIRQ(EINT1_IRQn);
  NVIC_EnableIRQ(EINT2_IRQn);
	NVIC_EnableIRQ (TIMER0_IRQn);
	NVIC_EnableIRQ (TIMER0_IRQn);
	NVIC_EnableIRQ (SysTick_IRQn);
}    
 
  void TIMER0_IRQHandler (void)	//Hace que i varíe de 0 a 4
 {
		LPC_TIM0 -> IR |= (1<<0);	//Borra el flag 
		m++;
		if(m>=4)
			m=0;
 }
 
void SysTick_Handler(void)		//Gestiona los valores del reloj
{
	//tiempo[0] son las centesimas de s, que siempre valdrán 0
	tiempo[1]++;	//decima de segundo
	if(tiempo[1]>=10)
	{
		tiempo[2]++;	//unidad de segundo
		tiempo[1]=0;
	}
	if(tiempo[2]>=10)
	{
		tiempo[3]++;	//decena de segundo
		tiempo[2]=0;
	}
	if(tiempo[3]>=6)
	{
		tiempo[4]++;	//unidad de minuto
		tiempo[3]=0;
	}
	if(tiempo[4]>=10)
	{
		tiempo[5]++;	//decena de minuto
		tiempo[4]=0;
	}
	if(tiempo[5]>=6)
	{
		tiempo[6]++;	//unidad de hora
		tiempo[5]=0;
	}
	if(tiempo[6]>=10)
	{
		tiempo[7]++;	//decima de hora
		tiempo[6]=0;
	}
	if(tiempo[7]>=2 && tiempo[6]==4)	//si llega a 24h reinicia
	{
		int i;
		for (i=0;i<7;i++)
		{
			tiempo[i]=0;
		}
	}
}

void EINT0_IRQHandler()
{
	interrupciones++;
}

void EINT1_IRQHandler()
{
	interrupciones++;
}

void EINT2_IRQHandler()
{
	interrupciones++;
}

int main (void)
{ 
	cuenta_1ms=0;
  configura_Interrupciones();
	config_GPIO();
	config_Timer0();
	config_Systick();
	
	while(1) 
	{
		if ((LPC_GPIO0->FIOPIN & (1<<3))==0)	//P0.3 == 0
		{
			if(switch_horas_seg == 0)
						x=0;	//muestra horas y minutos
					else
						x=0;	//muestrasegundos y decimas
					
			entradas = LPC_GPIO0->FIOPIN & 4;
			switch(entradas)
			{
				case 0:				//visualizar Timer
					LPC_GPIO1->FIOPIN = (NUMEROS[tiempo[m+x]])<<20;
					LPC_GPIO2->FIOPIN = (1 << (m)) & 0xF;
					continue;
				
				case 1:				//visualizar Alarma_1
					LPC_GPIO1->FIOPIN = (NUMEROS[alarma1[m+1]])<<20;
					LPC_GPIO2->FIOPIN = (1 << (m)) & 0xF;
					continue;
				
				case 2:				//visualizar Alarma_2
					LPC_GPIO1->FIOPIN = (NUMEROS[alarma2[m+1]])<<20;
					LPC_GPIO2->FIOPIN = (1 << (m)) & 0xF;
					continue;
				
				case 3:				//visualizar Temporizador_1
					LPC_GPIO1->FIOPIN = (NUMEROS[temp1[m+1]])<<20;
					LPC_GPIO2->FIOPIN = (1 << (m)) & 0xF;
					continue;
				
				case 4:				//visualizar Temporizador_2
					LPC_GPIO1->FIOPIN = (NUMEROS[temp2[m+1]])<<20;
					LPC_GPIO2->FIOPIN = (1 << (m)) & 0xF;
					continue;
			 }
		}
		else			//P0.3 == 1 Visualiza HOLA
		{	
				LPC_GPIO1->FIOPIN = (HOLA[m])<<20;
				LPC_GPIO2->FIOPIN = (1 << m) & 0xF;
				continue;
		}
	}
}
