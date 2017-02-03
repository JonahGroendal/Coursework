#include <msp430g2253.h>
#include "a6.h"

void setPWMFrequency(unsigned int frequency) {
	unsigned int cycleDelay = ((1000000/8)/frequency)/2 -1;

	P1DIR |= BIT6;
	P1SEL |= BIT6;
	
	// Set delay
	TA0CCR0  = cycleDelay;
}
void disablePWM(void) {
	P1DIR &= ~BIT6;
}

