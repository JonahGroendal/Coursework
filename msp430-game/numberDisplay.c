#include <msp430.h>
#include "a6.h"

/* Displays a single digit on a display (with range [0,3]) */
void displayDigit(int digit, int display) {
	// Allow manipulation of display bits
	P1DIR  |= 0b10110001;
	P2DIR  |= 0b00111111;
	
	// Set digit pinouts low, dispaly pinouts high
	P1OUT  &= ~0b10110001;
	P2OUT  &= ~0b00000111;
	P2OUT  |=  0b00111000;
	
	// Set active display
	switch (display) {
		case 0:
			P2OUT &= ~BIT5;
			break;
		case 1:
			P2OUT &= ~BIT4;
			break;
		case 2:
			P2OUT &= ~BIT3;
			break;
	}

	// Display digit
	switch (digit) {
		case 0:
			P1OUT |= 0b10110001;
			P2OUT |= 0b00000101;
			break;
		case 1:
			P1OUT |= 0b00110000;
			P2OUT |= 0b00000000;
			break;
		case 2:
			P1OUT |= 0b10010001;
			P2OUT |= 0b00000110;
			break;
		case 3:
			P1OUT |= 0b00110001;
			P2OUT |= 0b00000110;
			break;
		case 4:
			P1OUT |= 0b00110000;
			P2OUT |= 0b00000011;
			break;
		case 5:
			P1OUT |= 0b00100001;
			P2OUT |= 0b00000111;
			break;
		case 6:
			P1OUT |= 0b10100001;
			P2OUT |= 0b00000111;
			break;
		case 7:
			P1OUT |= 0b00110000;
			P2OUT |= 0b00000100;
			break;
		case 8:
			P1OUT |= 0b10110001;
			P2OUT |= 0b00000111;
			break;
		case 9:
			P1OUT |= 0b00110000;
			P2OUT |= 0b00000111;
			break;
	}
}

