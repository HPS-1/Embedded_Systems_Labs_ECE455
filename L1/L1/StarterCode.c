// Starter code for ECE 455

// Written by Bernie Roehl, December 2021

#include <lpc17xx.h>
#include <stdio.h>

const unsigned char led_pos[8] = { 28, 29, 31, 2, 3, 4, 5, 6 };

void SetLED(int n) {
	int mask = 1 << led_pos[n];
	if (n < 3) LPC_GPIO1->FIOSET = mask;
	else LPC_GPIO2->FIOSET = mask;
}

void ClearLED(int n) {
	int mask = 1 << led_pos[n];
	if (n < 3) LPC_GPIO1->FIOCLR = mask;
	else LPC_GPIO2->FIOCLR = mask;
}

 unsigned int ReadJoystick() {
		return 0x79 & ~(LPC_GPIO1->FIOPIN >> 20);
 }
 
 unsigned int ReadPotentiometer() {
 		LPC_ADC->ADCR |= 1 << 24;  // start ADC
 		while (!(LPC_ADC->ADGDR & 0x80000000));
 		return (LPC_ADC->ADGDR >> 4) & 0x0FFF;
}

int ReadPushbutton() {
		return !(LPC_GPIO2->FIOPIN & (1 << 10));  // true if button is down
}

void Initialize() {
	// initialize the LEDs
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;

	// initialize the ADC
	LPC_PINCON->PINSEL1 &= ~(3<<18);
	LPC_PINCON->PINSEL1 |=  (1<<18);      // P0.25 is AD0.2
	LPC_SC->PCONP       |=  (1<<12);      // enable power to ADC block
	LPC_ADC->ADCR = (1<< 2) |   // select AD0.2 pin
	          (4<< 8) |         // ADC clock is 25MHz/5
	          (1<<21);          // enable ADC 
}


