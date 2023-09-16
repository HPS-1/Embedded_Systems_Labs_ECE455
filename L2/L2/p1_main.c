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


static volatile uint32_t JoyFreezeTimer = 110;
static volatile uint32_t BtnFreezeTimer = 110;
static volatile uint32_t OldestItemTimer = 0;
static volatile uint32_t AccTimer = 0;
//Code below is partly borrowed from NXP community to provide timer functionality based on SysTick
//Source: https://community.nxp.com/pwmxy87654/attachments/pwmxy87654/tech-days/282/1/AMF-DES-T2632_Lab%20Guide.pdf

static void InitSysTick(void)
{
 SysTick_Config(SystemCoreClock/1000); /* Configure the SysTick timer */
}


void SysTick_Handler(void)
{
  if (JoyFreezeTimer<0xFFFFFFFF) { JoyFreezeTimer++; } /* increment timer */
  if (BtnFreezeTimer<0xFFFFFFFF) { BtnFreezeTimer++; } /* increment timer */
	if (OldestItemTimer<0xFFFFFFFF) { OldestItemTimer++; } /* increment timer */
	if (AccTimer<0xFFFFFFFF) { AccTimer++; } /* increment timer */
}

//void Delay_SysTick(uint32_t SysTicks)
//{
//DelayTimerTick = 0; /* reset timer value */
//while(DelayTimerTick < SysTicks); /* wait for timer */
//}
//Code borrowed ends here

struct buffer {
  unsigned int type; //0 for Pedal, 1 for ABS, 2 for Brake
  unsigned int state; //0 for Push/Engage, 1 for Release/Disengaged
  unsigned int time_elapsed_in_ms; //time_elapsed_in_ms since last event
	struct buffer *ptr_pre;
	struct buffer *ptr_nex;
};

unsigned int joy_flag = 0;
unsigned int joy = 0;
unsigned int btn_flag = 0;
unsigned int brake_flag = 0;
int btn = 0;
float voltage = 0.0;

unsigned int item_count = 0;
unsigned int total_time_elapsed = 0;
unsigned int time_elapsed_temp = 0;
struct buffer *head = 0;
struct buffer *tail = 0;
struct buffer *temp = 0;

void LogUpdate(unsigned int type, unsigned int state) {
	if (item_count == 0) {
		temp = calloc(1, sizeof(struct buffer));
		temp->type = type;
		temp->state = state;
		temp->ptr_nex = 0;
		temp->ptr_pre = 0;
		temp->time_elapsed_in_ms = 0;
		head = temp;
		tail = temp;
		item_count++;
		total_time_elapsed = 0;
		OldestItemTimer = 0;
	} else if (OldestItemTimer <= 5000){
		temp = calloc(1, sizeof(struct buffer));
		temp->type = type;
		temp->state = state;
		temp->ptr_nex = 0;
		temp->ptr_pre = head;
		temp->time_elapsed_in_ms = OldestItemTimer - total_time_elapsed;
		total_time_elapsed += temp->time_elapsed_in_ms;
		head->ptr_nex = temp;
		head = temp;
		item_count++;
	} else {//oldest log item should be deleted
		temp = calloc(1, sizeof(struct buffer));
		temp->type = type;
		temp->state = state;
		temp->ptr_nex = 0;
		temp->ptr_pre = head;
		temp->time_elapsed_in_ms = OldestItemTimer - total_time_elapsed;
		total_time_elapsed += temp->time_elapsed_in_ms;
		head->ptr_nex = temp;
		head = temp;
		item_count++;
		//new item creation completed
		//now delete the oldest item(s)
		while (OldestItemTimer > 5000){
			time_elapsed_temp = (tail->ptr_nex)->time_elapsed_in_ms;
			OldestItemTimer -= time_elapsed_temp;
			total_time_elapsed -= time_elapsed_temp;
			(tail->ptr_nex)->ptr_pre = 0;
			temp = tail->ptr_nex;
			free(tail);
			tail = temp;
			item_count--;
		}
	}
}

void ReadAndUpdate() {
		joy = ReadJoystick();
		if (joy == 16 && joy_flag == 0 && JoyFreezeTimer > 200){
			joy_flag = 1;
			JoyFreezeTimer = 0;
			//pedal engaged, log and start to care for ABS
			//printf("pedal engaged, log and start to care for ABS\r\n");
			LogUpdate(0, 0);
			if (btn_flag == 0){
				brake_flag = 1;
				LogUpdate(2, 0);
			}
		} else if (joy == 0 && joy_flag == 1){
			joy_flag = 0;
			//pedal disengaged, log
			//printf("pedal disengaged, log\r\n");
			LogUpdate(0, 1);
			if (brake_flag == 1){
				brake_flag = 0;
				LogUpdate(2, 1);
			}
		}
		//if (joy_flag == 1){
		btn = ReadPushbutton();
		if (btn != 0 && btn_flag == 0 && BtnFreezeTimer > 200){
			btn_flag = 1;
			BtnFreezeTimer = 0;
			//ABS engaged, log
			//printf("/ABS engaged, log\r\n");
			LogUpdate(1, 0);
			if (brake_flag == 1){
				brake_flag = 0;
				LogUpdate(2, 1);
			}
		} else if (btn == 0 && btn_flag == 1){
			btn_flag = 0;
			LogUpdate(1, 1);
			if (joy_flag == 1){
				brake_flag = 1;
				LogUpdate(2, 0);
			}
		}
		//}
}

void PrintLog(){
	unsigned int time_from_crash;
	struct buffer *ppp;
	time_from_crash = OldestItemTimer - total_time_elapsed;
	ppp = head;
	while (1){
		if((ppp == 0)||(time_from_crash > 5000)){break;}
		printf("%u %u %u\r\n", ppp->type, ppp->state, time_from_crash);
		time_from_crash += ppp->time_elapsed_in_ms;
		ppp = ppp->ptr_pre;
	}
	return;
}

/*float ReadVoltage(){
	 return ReadPotentiometer()*3.3/4096;
}*/
/*int DB_ReadPushbutton() {
		btn = ReadPushbutton();
		if (btn != 0 && btn_flag == 0){
			btn_flag = 1;
			return btn;
		} else if (btn == 0 && btn_flag == 1){
			btn_flag = 0;
		}
		return 0;
}*/

void FSMFramework (unsigned int numofstates, unsigned int (*states[numofstates])(void), unsigned int init){
	unsigned int next = init;
	while (1){
		next = (*states[next])();
	}
}

unsigned int state0(void) //State for ADC=<0V(0.02V actually)
{
	//Enter Code
	//Execute Code
	while(1){
		ReadAndUpdate();
		voltage = ReadPotentiometer()*3.3/4096;
		//Exit Code
		if(voltage > 3.0){//collision
			return 3;
	  }else if (voltage > 2.0){//5ms detect
		  return 2;
		}else if (voltage > 0.02){//25s detect
			return 1;
		}
	}
}

unsigned int state1(void) //State for 0V<ADC=<2V
{
	//Enter Code
	AccTimer = 0;
	//Execute Code
	while(1){
		ReadAndUpdate();
		voltage = ReadPotentiometer()*3.3/4096;
		//Exit Code
		if(voltage > 3.0){//collision
			return 3;
	  }else if (voltage > 2.0){//5ms detect
		  return 2;
		}else if (voltage <= 0.02){//0V
			return 0;
		}else if (AccTimer > 25000){//acc failure
			SetLED(0);
			SetLED(2);
			SetLED(4);
			SetLED(6);
			return 0;
		}
	}
}

unsigned int state2(void) //State for 2V<ADC=<3V
{
	//Enter Code
	AccTimer = 0;
	//Execute Code
	while(1){
		ReadAndUpdate();
		voltage = ReadPotentiometer()*3.3/4096;
		//Exit Code
		if(voltage > 3.0){//collision
			return 3;
	  }else if (voltage <= 0.02){//0V
			return 0;
		}else if (voltage <= 2.0){//25s detect
		  return 1;
		}else if (AccTimer > 5){//5ms collision
			return 3;
		}
	}
}

unsigned int state3(void) //State for 3V<ADC Imminent collision, brace for impact!
{
	//Enter Code
	SetLED(0);
	SetLED(1);
	SetLED(2);
	SetLED(3);
	SetLED(4);
	SetLED(5);
	SetLED(6);
	SetLED(7);
	PrintLog();
	//printf("Crashed!\r\n");
	while(1){
		//use an infinite loop to represent that the car is crashed and ECG is no longer responding
	}
	//Execute Code
	//Exit Code
}




//This is C. The expected function heading is int main(void)
int main( void ) 
{
	//Always call this function at the start. It sets up various peripherals, the clock etc. If you don't call this
	//you may see some weird behaviour
	SystemInit();
	Initialize();
	InitSysTick();
	unsigned int (*lab1states[4]) (void);
  lab1states[0] = state0;
  lab1states[1] = state1;
  lab1states[2] = state2;
	lab1states[3] = state3;

	//Printf now goes to the UART, so be sure to have PuTTY open and connected
	printf("SYSTEM STARTED\r\n");
	printf("%u\r\n", SystemCoreClock);
	while(1){
		FSMFramework (4, lab1states, 0);
		/*unsigned int BBB = DB_ReadJoystick();
		int CCC = DB_ReadPushbutton();
		printf("Status: %u %d\r\n", BBB, CCC);*/
		//ReadAndUpdate();
		/*if (ReadVoltage()>0.02){
		printf("%f\r\n", ReadVoltage());
		}*/
	}
	
	//Your code should always terminate in an endless loop if it is done. If you don't
	//the processor will enter a hardfault and will be weird
	while(1);
}

/*unsigned int state0(void) //State for Furnace On
{
  //Delay_SysTick (500);
  //printf("State0\r\n");
  //return 1;
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
}*/




	
