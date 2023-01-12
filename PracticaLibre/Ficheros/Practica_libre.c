#include "lpc17xx.h"
#include "delay.h"
#include "math.h"

uint32_t SystemFrequency=100000000;
#define DAC_BIAS	0x00010000  // Settling time a valor 2,5us

int NUMEROS[10]={0x81,0xF3,0x49,0x61,0x33,0x25,0x05,0xF1,0x01,0x31}; //0....9
int HOLA[4]= {0x13,0x81,0x8F,0x11};	//H O L A gfedcbaP
int tiempo[8]={0,0,0,0,0,0,0,0}; 	//centesimas de s, decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de h, decenas de h
int alarma1[4]={0,0,0,0}; 				//unidades de m, decenas de m, unidades de h, decenas de h
int alarma2[4]={0,0,0,0}; 				//unidades de m, decenas de m, unidades de h, decenas de h
int temp1[8]={0,0,0,0,0,0,0,0}; 	//centesimas de s, decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de h, decenas de h
int temp2[8]={0,0,0,0,0,0,0,0}; 	//centesimas de s, decimas de s, unidades de s, decenas de s, unidades de m, decenas de m, unidades de h, decenas de h
uint16_t Onda_Alarma1[100];
uint16_t Onda_Alarma2[100];
uint16_t Onda_Temporizadores[100];
int sel_onda;											//Selecciona la onda de salida del DAC
int m;														//Cambia cada 5ms para visualizar
int entradas;											//Gestión del switch-case Reloj-Alarmas-Temporizadores
int x;														//Selector de modos de visualización
int prog; 												//Control del modo programación
int incr;
int f; 														//Frecuencia de Timer1 que gestiona el DAC
int k=0;													//Muestra del DAC
int contador10s;									//Gestiona el tiempo de activación del DAC

void config_GPIO (void)
{
	//Configuración de los pines P2.10 a P2.13, como entradas de interrupción
	LPC_PINCON->PINSEL4 |= 1 << (10*2);		//EINT0
	LPC_PINCON->PINSEL4 |= 1 << (11*2);	 	//EINT1 
	LPC_PINCON->PINSEL4 |= 1 << (12*2);		//EINT2
	LPC_PINCON->PINSEL4 |= 1 << (13*2);		//EINT3

	
  LPC_GPIO1->FIODIR |= (255<<19);				// P1.19 hasta P1.26 definidos como salida - Segmentos display
	LPC_GPIO0->FIODIR |= (1<<26);					// P0.26 definido como salida - DAC
	LPC_GPIO2->FIODIR |= (15<<0);					// P2.0 hasta P2.3 como salida - On/Off displays
	LPC_GPIO0->FIODIR &= ~(1<<2);					// P0.2 definido como entrada - Switch horas/segundos
	LPC_GPIO0->FIODIR &= ~(1<<3);					// P0.3 definido como entrada - Switch texto/timer
	LPC_GPIO1->FIODIR &= ~(1<<0);					// P1.0 definido como entrada - Switch Temp1 
	LPC_GPIO1->FIODIR &= ~(1<<4);					// P1.4 definido como entrada - Switch Alarma1
	LPC_GPIO1->FIODIR &= ~(1<<1);					// P1.1 definido como entrada - Switch Temp2
	LPC_GPIO1->FIODIR &= ~(1<<9);					// P1.9 definido como entrada - Switch Alarma2
	
	LPC_PINCON->PINMODE0 &= ~(3<<2*3);	//P0.3 a pull-down
	LPC_PINCON->PINMODE0 |= (3<<2*2);		//P0.2 a pull-down
	LPC_PINCON->PINMODE2 |= (3<<0*2);		//P1.0 a pull-down
	LPC_PINCON->PINMODE2 |= (3<<4*2);		//P1.4 a pull-down
	LPC_PINCON->PINMODE2 |= (3<<1*2);		//P1.1 a pull-down
	LPC_PINCON->PINMODE2 |= (3<<9*2);		//P1.9 a pull-down
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
 
 void config_Timer1(void)	//Interrumpe en función del DAC
 {
	LPC_SC ->PCLKSEL0 |= 1<<3;
	LPC_TIM1 ->MCR =0x3;
	LPC_TIM1 ->PR =0;
	LPC_TIM1 -> MR0 = (25000000/(f*100))-1;	
	LPC_TIM1 ->TC =0;
	LPC_TIM1 ->TCR |=(1<<0);
 }
 
void config_Interrupciones()
{
	LPC_SC -> EXTMODE |= (1<<0);		//EINT0 activa por flanco
	LPC_SC -> EXTMODE |= (1<<1);		//EINT1 activa por flanco
	LPC_SC -> EXTMODE |= (1<<2);		//EINT2 activa por flanco
	LPC_SC -> EXTMODE |= (1<<3);		//EINT3 activa por flanco
	LPC_SC -> EXTINT |= (0x15);			//flags
	
	//Bajar el flag
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_ClearPendingIRQ(EINT1_IRQn);
	NVIC_ClearPendingIRQ(EINT2_IRQn);
	NVIC_ClearPendingIRQ(EINT3_IRQn);
	
	//Asignación de prioridades
  NVIC_SetPriorityGrouping(4);
	NVIC_SetPriority(SysTick_IRQn, 0x2);
	NVIC_SetPriority (TIMER0_IRQn, 0x2);	//000.10XXX
	NVIC_SetPriority (TIMER1_IRQn, 0x6);	//001.10XXX
  NVIC_SetPriority(EINT0_IRQn, 0x6);		//001.10XXX
  NVIC_SetPriority(EINT1_IRQn, 0x6);		//001.10XXX
  NVIC_SetPriority(EINT2_IRQn, 0x6);		//001.10XXX
	NVIC_SetPriority(EINT3_IRQn, 0x6);		//001.10XXX

	//Habilitación de las interrupciones
  NVIC_EnableIRQ(EINT0_IRQn);
  NVIC_EnableIRQ(EINT1_IRQn);
  NVIC_EnableIRQ(EINT2_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);
	NVIC_EnableIRQ (TIMER0_IRQn);
	NVIC_EnableIRQ (TIMER1_IRQn);
	NVIC_EnableIRQ (SysTick_IRQn);
}    
 
void config_DAC( void )	// ConfiguraP0.26 como salida DAC 
{
  LPC_PINCON->PINSEL1 &= ~(0x1<<20);	//Bit 20 a 0
	LPC_PINCON->PINSEL1 |= (0x1<<21);		//Bit 21 a 1
  return;
}

void generar_Ondas (void)	//Genera las Ondas para el DAC.
{
	int i;
	for (i=0;i<100;i++)
	{
		if(i<=50)
			Onda_Alarma1[i]=(1024/50)*i;
		else
			Onda_Alarma1[i]=(1024/50)*(100-i);
	}
	
	for (i=0;i<100;i++)
	{
		if(i<=50)
			Onda_Alarma2[i]=0;
		else
			Onda_Alarma2[i]=1023;
	}
	
	for (i=0;i<100;i++)
	{
		Onda_Temporizadores[i]=511+(511*sin(2*3.14159*i/100));
	}
}

void TIMER0_IRQHandler (void)	//Gestiona la visualización durante 5ms
{
	LPC_TIM0 -> IR |= (1<<0);	//Borra el flag 
	m++;
	if(m>=4)
		m=0;
}

void TIMER1_IRQHandler (void)	//Timer para el DAC
{
	LPC_TIM1 -> IR |= (1<<0);	//Borra el flag
	
		if(sel_onda==0)	//Alarma1
		{
			LPC_DAC -> DACR = (Onda_Alarma1[k]<<6) | DAC_BIAS;		 
		}
		else if (sel_onda==1)	//Alarma2
		{
			LPC_DAC -> DACR = (Onda_Alarma2[k]<<6) | DAC_BIAS;
		}
		else if (sel_onda==2)//Temporizadores (solo cambia f)
		{
			LPC_DAC -> DACR = (Onda_Temporizadores[k]<<6) | DAC_BIAS;
		}

		k++;
		if(k==100)
			k=0;
}
 
void Sys_Reloj()	//Gestiona los valores del Reloj
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
		tiempo[7]++;	//decena de hora
		tiempo[6]=0;
	}
	if(tiempo[7]>=2 && tiempo[6]>=4)	//si llega a 24h reinicia
	{
		int i;
		for (i=0;i<8;i++)
		{
			tiempo[i]=0;
		}
	}
}

void Sys_Timer1()	//Gestiona los valores del Temporizador1
{
	if(temp1[1]!=0)
	{
		temp1[1]--;
	}
	else if(temp1[2]!=0)
	{
		temp1[2]--;
		temp1[1]=9;
	}
	else if(temp1[3]!=0)
	{
		temp1[3]--;
		temp1[2]=9;
		temp1[1]=9;
	}
	else if(temp1[4]!=0)
	{
		temp1[4]--;
		temp1[3]=5;
		temp1[2]=9;
		temp1[1]=9;
	}
	else if(temp1[5]!=0)
	{
		temp1[5]--;
		temp1[4]=9;
		temp1[3]=5;
		temp1[2]=9;
		temp1[1]=9;
	}
	else if(temp1[6]!=0)
	{
		temp1[6]--;
		temp1[5]=5;
		temp1[4]=9;
		temp1[3]=5;
		temp1[2]=9;
		temp1[1]=9;	
	}
	else if(temp1[7]!=0)
	{
		temp1[7]--;
		temp1[6]=9;
		temp1[5]=5;
		temp1[4]=9;
		temp1[3]=5;
		temp1[2]=9;
		temp1[1]=9;
	}
	else
	{
		//FIN DEL TEMP1
		contador10s++;
		if(contador10s>=200)
		{
			LPC_TIM1->MCR &= 0xFE;	//Desactiva Timer1
			contador10s=0;
		}
	}
}

void Sys_Timer2()	//Gestiona los valores del Temporizador2
{
	if(temp2[1]!=0)
	{
		temp2[1]--;
	}
	else if(temp2[2]!=0)
	{
		temp2[2]--;
		temp2[1]=9;
	}
	else if(temp2[3]!=0)
	{
		temp2[3]--;
		temp2[2]=9;
		temp2[1]=9;
	}
	else if(temp2[4]!=0)
	{
		temp2[4]--;
		temp2[3]=5;
		temp2[2]=9;
		temp2[1]=9;
	}
	else if(temp2[5]!=0)
	{
		temp2[5]--;
		temp2[4]=9;
		temp2[3]=6;
		temp2[2]=9;
		temp2[1]=9;
	}
	else if(temp2[6]!=0)
	{
		temp2[6]--;
		temp2[5]=5;
		temp2[4]=9;
		temp2[3]=5;
		temp2[2]=9;
		temp2[1]=9;	
	}
	else if(temp2[7]!=0)
	{
		temp2[7]--;
		temp2[6]=9;
		temp2[5]=5;
		temp2[4]=9;
		temp2[3]=5;
		temp2[2]=9;
		temp2[1]=9;
	}
	else
	{
		//FIN DEL TEMP2
		contador10s++;
		if(contador10s>=200)	
		{
			LPC_TIM1->MCR &= 0xFE;	//Desactiva Timer1
			contador10s=0;
		}
	}
}

void SysTick_Handler(void)		//Gestiona el SysTick
{
	Sys_Reloj();
	
	if((LPC_GPIO1->FIOPIN & (1<<4))==16)	//Alarma 1
	{
		if(alarma1[0]==tiempo[4] && alarma1[1]==tiempo[5] && alarma1[2]==tiempo[6] && alarma1[3]==tiempo[7] && tiempo[3]==0 && tiempo[2]== 0 && tiempo[1]==0)
		{	
			sel_onda=0;
			f=300;
			config_Timer1();
		}
		if(alarma1[0]==tiempo[4] && alarma1[1]==tiempo[5] && alarma1[2]==tiempo[6] && alarma1[3]==tiempo[7] && tiempo[3]==0 && tiempo[2]==9 && tiempo[1]==9)
			LPC_TIM1->MCR &= 0xFE;//Desactiva Timer1
	}
	
	if((LPC_GPIO1->FIOPIN & (1<<9))==512)	//Alarma 2
	{
		if(alarma2[0]==tiempo[4] && alarma2[1]==tiempo[5] && alarma2[2]==tiempo[6] && alarma2[3]==tiempo[7] && tiempo[3]==0 && tiempo[2]== 0 && tiempo[1]==0)
		{
			sel_onda=1;
			f=5000;
			config_Timer1();
		}
		if(alarma2[0]==tiempo[4] && alarma2[1]==tiempo[5] && alarma2[2]==tiempo[6] && alarma2[3]==tiempo[7] && tiempo[3]==0 && tiempo[2]==9 && tiempo[1]==9)
			LPC_TIM1->MCR &= 0xFE;//Desactiva Timer1
	}
	
	if((LPC_GPIO1->FIOPIN & (1<<0))==1)	//Temporizador 1
	{
		if(temp1[7]==0 && temp1[6]==0 && temp1[5]==0 && temp1[4]==0 && temp1[3]==0 && temp1[2]== 0 && temp1[1]==1)
		{	
			sel_onda=2;
			f=1000;
			config_Timer1();
		}
		Sys_Timer1();
	}
	
	if((LPC_GPIO1->FIOPIN & (1<<1))==2)	//Temporizador 2
	{
		if(temp2[7]==0 && temp2[6]==0 && temp2[5]==0 && temp2[4]==0 && temp2[3]==0 && temp2[2]== 0 && temp2[1]==1)
		{	
			sel_onda=2;
			f=2000;
			config_Timer1();
		}
		Sys_Timer2();
	}
}

void EINT0_IRQHandler()	//Controla la visualización - Pulsador ISP
{
	delay_1ms(150);
	LPC_SC->EXTINT |= (1);   // Borrar el flag de la EINT0 --> EXTINT.0
	entradas++;
	if (entradas>4)
		entradas=0;
}

void EINT1_IRQHandler()	//Accede al modo programación con KEY1: 0 No progamacion... 1-4 cada digito
{
	delay_1ms(150);
	LPC_SC->EXTINT |= (1<<1);   // Borrar el flag de la EINT1
	incr=0;
	prog++;
	if (prog>4)
		prog = 0;
}

void EINT2_IRQHandler()	//Boton incr
{
	delay_1ms(150);
	LPC_SC->EXTINT |= (1<<2);   //Borrar el flag de la EINT2
	incr++;
	if (incr>9)
		incr = 0;
}

void EINT3_IRQHandler()	//Boton decr
{
	delay_1ms(150);
	LPC_SC->EXTINT |= (1<<3);   //Borrar el flag de la EINT3
	incr--;
	if (incr<0)
		incr = 9;
}

int main (void)
{ 
	config_GPIO();
  config_Interrupciones();
	config_Timer0();
	config_Systick();
	config_DAC();
	generar_Ondas();

	while(1) 
	{
		
		
		if((LPC_GPIO0->FIOPIN & (1<<3))==0)	//P0.3==0 switch_texto_aviso
		{
			
			if((LPC_GPIO0->FIOPIN & (1<<2))==0)	//P0.2 switch_horas_seg
				x=0;	//muestra segundos
			else
				x=4;	//muestra horas:mins
					
			
			switch(entradas)
			{
				case 0:				//visualizar Timer
					LPC_GPIO1->FIOPIN = (NUMEROS[tiempo[m+x]])<<19;
					LPC_GPIO2->FIOPIN = (1 << (3-m)) & 0xF;
					if(m==2)
						LPC_GPIO1->FIOPIN &= ~(1<<19);
	
				
					while (prog!=0)
					{
						LPC_GPIO1->FIOPIN = (NUMEROS[tiempo[3+prog]])<<19;
						LPC_GPIO2->FIOPIN = (1 << (4-prog)) & 0xF;
						tiempo[3+prog]=incr;
					}	
					continue;
				
				case 1:				//visualizar Alarma_1
					LPC_GPIO1->FIOPIN = (NUMEROS[alarma1[m]])<<19;
					LPC_GPIO2->FIOPIN = (1 << (3-m)) & 0xF;
					if(m==2)
						LPC_GPIO1->FIOPIN &= ~(1<<19);
					
					while (prog!=0)
					{
						LPC_GPIO1->FIOPIN = (NUMEROS[alarma1[prog-1]])<<19;
						LPC_GPIO2->FIOPIN = (1 << (4-prog)) & 0xF;
						alarma1[prog-1]=incr;
					}	
					continue;
				
				case 2:				//visualizar Alarma_2
					LPC_GPIO1->FIOPIN = (NUMEROS[alarma2[m]])<<19;
					LPC_GPIO2->FIOPIN = (1 << (3-m)) & 0xF;
					if(m==2)
						LPC_GPIO1->FIOPIN &= ~(1<<19);
					
					while (prog!=0)
					{
						LPC_GPIO1->FIOPIN = (NUMEROS[alarma2[prog-1]])<<19;
						LPC_GPIO2->FIOPIN = (1 << (4-prog)) & 0xF;
						alarma2[prog-1]=incr;
					}	
					continue;
				
				case 3:				//visualizar Temporizador_1
					LPC_GPIO1->FIOPIN = (NUMEROS[temp1[m+x]])<<19;	//Si hago m+x+2 este modo mostrará mins:segs
					LPC_GPIO2->FIOPIN = (1 << (3-m)) & 0xF;
					if(m==2)
						LPC_GPIO1->FIOPIN &= ~(1<<19);
					
					while (prog!=0)
					{
						LPC_GPIO1->FIOPIN = (NUMEROS[temp1[1+prog]])<<19;
						LPC_GPIO2->FIOPIN = (1 << (4-prog)) & 0xF;
						temp1[1+prog]=incr;
					}	
					continue;
				
				case 4:				//visualizar Temporizador_2
					LPC_GPIO1->FIOPIN = (NUMEROS[temp2[m+x]])<<19;
					LPC_GPIO2->FIOPIN = (1 << (3-m)) & 0xF;
					if(m==2)
						LPC_GPIO1->FIOPIN &= ~(1<<19);
					
					while (prog!=0)
					{
						LPC_GPIO1->FIOPIN = (NUMEROS[temp2[1+prog]])<<19;
						LPC_GPIO2->FIOPIN = (1 << (4-prog)) & 0xF;
						temp2[1+prog]=incr;
					}	
					continue;
			 }
		}
		else			//P0.3 == 1 Visualiza HOLA
		{	
				LPC_GPIO1->FIOPIN = (HOLA[m])<<19;
				LPC_GPIO2->FIOPIN = (1<<m) & 0xF;
				continue;
		}
	}
}
