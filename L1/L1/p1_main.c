//This file contains relevant pin and other settings 
#include <LPC17xx.h>

//This file is for printf and other IO functions
#include "stdio.h"

//this file sets up the UART
#include "uart.h"

//Starter code for lab 1
#include "StarterCode.h"

//Macros
//Maximum-on/Minimum-off duration in second
#define DEF_ON 4
#define DEF_OFF 1
//Hysteresis gap
#define GAP 4


static volatile uint32_t PrintTimer = 0;
static volatile uint32_t FurnaceTimer = 0;
//Code below is partly borrowed from NXP community to provide timer functionality based on SysTick
//Source: https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/tech-days/282/1/AMF-DES-T2632_Lab%20Guide.pdf

static void InitSysTick(void)
{
 SysTick_Config(SystemCoreClock/1000); /* Configure the SysTick timer */
}


void SysTick_Handler(void)
{
if (PrintTimer<0xFFFFFFFF) { PrintTimer++; } /* increment timer */
if (FurnaceTimer<0xFFFFFFFF) { FurnaceTimer++; } /* increment timer */
}

//void Delay_SysTick(uint32_t SysTicks)
//{
//DelayTimerTick = 0; /* reset timer value */
//while(DelayTimerTick < SysTicks); /* wait for timer */
//}
//Code borrowed ends here

float current_temp = 0.0;
unsigned int joy_flag = 0;
unsigned int joy = 0;
float setpoint = 20.0;

void FSMFramework (unsigned int numofstates, unsigned int (*states[numofstates])(void), unsigned int init){
	unsigned int next = init;
	while (1){
		next = (*states[next])();
	}
}

unsigned int state0(void) //State for Furnace On
{
	//Enter Code
	SetLED(4);
	FurnaceTimer = 0;
	//Execute Code
	while(1){
		current_temp = (-102.4)+(0.05*ReadPotentiometer());
		joy = ReadJoystick();
		if (joy != 0 && joy_flag == 0){
			joy_flag = 1;
			if (joy == 16) { //NORTH
				setpoint += 0.5;
			} else if (joy == 64) { //SOUTH
				setpoint -= 0.5;
			}
		} else if (joy == 0 && joy_flag == 1){
			joy_flag = 0;
		}
		if (PrintTimer >= 1000){
			printf ("Set Point is %.4f Celsius\r\n", setpoint);
			printf ("Current Temp is %.4f Celsius\r\n", current_temp);
			PrintTimer = 0;
		}
		//Exit Code
		if(FurnaceTimer >= DEF_ON*1000){
			ClearLED(4);
			return 1;
	  } else if (current_temp > (setpoint + (GAP/2))){
		  ClearLED(4);
			return 2;
		}
	}
}

unsigned int state1(void) //State for Furnace Off (On Furnace Cool-down)
{
	//Enter Code
	FurnaceTimer = 0;
	//Execute Code
	while(1){
		current_temp = (-102.4)+(0.05*ReadPotentiometer());
		joy = ReadJoystick();
		if (joy != 0 && joy_flag == 0){
			joy_flag = 1;
			if (joy == 16) { //NORTH
				setpoint += 0.5;
			} else if (joy == 64) { //SOUTH
				setpoint -= 0.5;
			}
		} else if (joy == 0 && joy_flag == 1){
			joy_flag = 0;
		}
		if (PrintTimer >= 1000){
			printf ("Set Point is %.4f Celsius\r\n", setpoint);
			printf ("Current Temp is %.4f Celsius\r\n", current_temp);
			PrintTimer = 0;
		}
		//Exit Code
		if(FurnaceTimer >= DEF_OFF*1000){
			return 2;
	  }
	}
}

unsigned int state2(void) //State for Furnace Off (Temperature Successfully Heated to)
{
	//Enter Code(None here)
	//Execute Code
	while(1){
		current_temp = (-102.4)+(0.05*ReadPotentiometer());
		joy = ReadJoystick();
		if (joy != 0 && joy_flag == 0){
			joy_flag = 1;
			if (joy == 16) { //NORTH
				setpoint += 0.5;
			} else if (joy == 64) { //SOUTH
				setpoint -= 0.5;
			}
		} else if (joy == 0 && joy_flag == 1){
			joy_flag = 0;
		}
		if (PrintTimer >= 1000){
			printf ("Set Point is %.4f Celsius\r\n", setpoint);
			printf ("Current Temp is %.4f Celsius\r\n", current_temp);
			PrintTimer = 0;
		}
		//Exit Code
		if (current_temp < (setpoint - (GAP/2))){
			return 0;
	  }
	}
}




//This is C. The expected function heading is int main(void)
int main( void ) 
{
	//Always call this function at the start. It sets up various peripherals, the clock etc. If you don't call this
	//you may see some weird behaviour
	SystemInit();
	Initialize();
	InitSysTick();
	unsigned int (*lab1states[3]) (void);
  	lab1states[0] = state0;
  	lab1states[1] = state1;
  	lab1states[2] = state2;

	//Printf now goes to the UART, so be sure to have PuTTY open and connected
	printf("SYSTEM STARTED\r\n");
	PrintTimer = 0;
	while(1){
		FSMFramework (3, lab1states, 0);
	}
	
	//Your code should always terminate in an endless loop if it is done. If you don't
	//the processor will enter a hardfault and will be weird
	while(1);
}




	
