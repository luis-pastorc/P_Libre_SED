#include "lpc17xx.h"

uint32_t SystemFrequency=100000000;

uint32_t cuenta_1ms,visualizar;  // Contador de ms 

int NUMEROS[10] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90}; //0....9
int HOLA[4] = {0x89,0xC0,0xC7,0x88};	//H O L A
int tiempo[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int alarma1[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int alarma2[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int temp1[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int temp2[7] = {0,0,0,0,0,0,0}; //decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de hora, decenas de horas
int i;
int interrupciones;
int entradas;	//para gestionar el switch
int switch_horas_seg;	//elegir horas+mins o segs+decs
int switch_alarma_1;
int switch_alarma_2;
int switch_temp_1;
int switch_temp_2;

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

void configura_Interrupciones()
{
	NVIC_SetPriority(SysTick_IRQn, 0x1);
	
	SysTick->LOAD = 999999;		//interrumpe cada ms 0.001
	SysTick->VAL = 0;
	SysTick->CTRL = 7;
	
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
  NVIC_SetPriority(EINT0_IRQn, 0x2);	//001.00XXX
  NVIC_SetPriority(EINT1_IRQn, 0x2);	//001.10XXX
  NVIC_SetPriority(EINT2_IRQn, 0x2);	//000.10XXX

	//Habilitación de las interrupciones
  NVIC_EnableIRQ(EINT0_IRQn);
  NVIC_EnableIRQ(EINT1_IRQn);
  NVIC_EnableIRQ(EINT2_IRQn);
}    

void timer_init(void)
{
    LPC_SC->PCONP |= (1 << 1);	//habilitar el clock del timer
    LPC_TIM1->TCR = (1 << 1); //resetear el contador del timer
    LPC_TIM1->TCR = (1 << 0); //habilitar el contador del timer
    NVIC_EnableIRQ(TIMER1_IRQn);	//habilitar la interrupción del timer
    LPC_TIM1->MCR = (1 << 0); //generar interrupción al llegar a MR0
    LPC_TIM1->MR0 = 5000000;	//establecer el valor de comparación para generar la interrupción cada 5 milisegundos
}

void SysTick_Handler(void)		// ISRdel SYSTICK Interrumpe cada 100ms
{
	cuenta_1ms++;
	visualizar++;
	if(cuenta_1ms>=10)	//100ms = 0,1s
	{
		tiempo[0]++;	//decima de segundo
		cuenta_1ms=0;
	}
	if(tiempo[0]>=10)
	{
		tiempo[1]++;	//unidad de segundo
		tiempo[0]=0;
	}
	if(tiempo[1]>=10)
	{
		tiempo[2]++;	//decima de segundo
		tiempo[1]=0;
	}
	if(tiempo[2]>=6)
	{
		tiempo[3]++;	//unidad de minuto
		tiempo[2]=0;
	}
	if(tiempo[3]>=10)
	{
		tiempo[4]++;	//decima de minuto
		tiempo[3]=0;
	}
	if(tiempo[4]>=6)
	{
		tiempo[5]++;	//unidad de hora
		tiempo[4]=0;
	}
	if(tiempo[5]>=10)
	{
		tiempo[6]++;	//decima de hora
		tiempo[5]=0;
	}
	if(tiempo[6]>=2 && tiempo[5]==4)	//si llega a 24h reinicia
	{
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
	visualizar=0;
  configura_Interrupciones();
	config_GPIO();
	config_GPIO();
	int x;
	
	while(1) 
	{
		if ((LPC_GPIO0->FIOPIN & (1<<3))==0)	//P0.3 == 0
		{
			if(switch_horas_seg == 0)
						x=5;	//muestra horas y minutos
					else
						x=3;	//muestrasegundos y decimas
					
			entradas = LPC_GPIO0->FIOPIN & 4;
			switch(entradas)
			{
				case 0:				//visualizar Timer
					for(i=0;i<4;i++)
					{	
						visualizar=0;
						LPC_GPIO1->FIOPIN = (NUMEROS[tiempo[i+1]])<<20;
						LPC_GPIO2->FIOPIN = (1 << (x-i)) & 0xF;
						if(visualizar>=5)
							continue;
					} 
				
				case 1:				//visualizar Alarma_1
					for(i=0;i<4;i++)
					{	
						visualizar=0;
						LPC_GPIO1->FIOPIN = (NUMEROS[alarma1[i+1]])<<20;
						LPC_GPIO2->FIOPIN = (1 << (x-i)) & 0xF;
						if(visualizar>=5)
							continue;
					} 
				
				case 2:				//visualizar Alarma_2
					for(i=0;i<4;i++)
					{	
						visualizar=0;
						LPC_GPIO1->FIOPIN = (NUMEROS[alarma2[i+1]])<<20;
						LPC_GPIO2->FIOPIN = (1 << (x-i)) & 0xF;
						if(visualizar>=5)
							continue;
					}
				
				case 3:				//visualizar Temporizador_1
					for(i=0;i<4;i++)
					{	
						visualizar=0;
						LPC_GPIO1->FIOPIN = (NUMEROS[temp1[i+1]])<<20;
						LPC_GPIO2->FIOPIN = (1 << (x-i)) & 0xF;
						if(visualizar>=5)
							continue;
					}
				
				case 4:				//visualizar Temporizador_2
					for(i=0;i<4;i++)
					{	
						visualizar=0;
						LPC_GPIO1->FIOPIN = (NUMEROS[temp2[i+1]])<<20;
						LPC_GPIO2->FIOPIN = (1 << (x-i)) & 0xF;
						if(visualizar>=5)
							continue;
					}
			 }
		}
		else			//P0.3 == 1 Visualiza HOLA
		{	
			for(i=0;i<4;i++)
			{	
				visualizar=0;
				LPC_GPIO1->FIOPIN = (HOLA[i])<<20;
				LPC_GPIO2->FIOPIN = (1 << i) & 0xF;
				if(visualizar>=5)
					continue;
			}
		}
	}
}