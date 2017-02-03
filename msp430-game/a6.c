#include <msp430g2253.h>
#include <libemb/serial/serial.h>
#include <libemb/shell/shell.h>
#include <libemb/conio/conio.h>
#include <stdlib.h>
#include <stdio.h>


unsigned int display_number = 0;
unsigned int display_counter = 0;
unsigned int frequency = 440;
unsigned int game_mode = 0;
unsigned int game_lives = 3;
int game_difficulty = 1;
unsigned int game_level = 1;
const int primes[168] = {2,3,5,7,11,13,17,19,23,29,31,37,41,
43,47,53,59,61,67,71,73,79,83,89,97,101,103,107,109,113,
127,131,137,139,149,151,157,163,167,173,179,181,191,193,
197,199,211,223,227,229,233,239,241,251,257,263,269,271,
277,281,283,293,307,311,313,317,331,337,347,349,353,359,
367,373,379,383,389,397,401,409,419,421,431,433,439,443,
449,457,461,463,467,479,487,491,499,503,509,521,523,541,
547,557,563,569,571,577,587,593,599,601,607,613,617,619,
631,641,643,647,653,659,661,673,677,683,691,701,709,719,
727,733,739,743,751,757,761,769,773,787,797,809,811,821,
823,827,829,839,853,857,859,863,877,881,883,887,907,911,
919,929,937,941,947,953,967,971,977,983,991,997};
const int powersOfTwo[9] = {2,4,8,16,32,64,128,256,512};

int main(void) {
	WDTCTL  = WDTPW + WDTHOLD;
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL  = CALDCO_1MHZ;

	P2SEL &= ~(BIT6|BIT7);
	P2DIR = BIT7 | BIT6;
	P2OUT &= ~BIT7;
	P2OUT &= ~BIT6;
	
	serial_init(9600);

	//Configure button
	P1REN  =  BIT3;
	P1OUT |=  BIT3;
	P1IE  |=  BIT3;
	P1IES |=  BIT3;
	P1IFG &= ~BIT3;
	
	// Configure timer 0
	TA0CCTL1 |= OUTMOD_4;
	TA0CTL   |= TASSEL_2 + MC_1 + ID_3;
	
	// Configure timer 1
	TA1CCR0  |= 256;
	TA1CCTL0 |= CCIE;
	TA1CTL   |= TASSEL_2 + MC_1 + ID_3;	
	
	
	cio_printf("\n\r\n\r-----------------------------------");
	cio_printf("\n\rWelcome to the game!\n\r");
	cio_printf("\n\rYour objective is to stop the counter on 3 types of numbers:\n\r");
	cio_printf("1st - stop the counter on an even number\n\r");
	cio_printf("2nd - stop the counter on a power of 2\n\r");
	cio_printf("3rd - stop the counter on a prime number\n\r");
	cio_printf("\n\rThe difficulty will increase after each round, and you have ");
	cio_printf("three lives (failures) before the game resets.\n\r");
	cio_printf("\n\rGame Options:\n\r");
	cio_printf("Increase difficulty - type \"diffUp\"\n\r");
	cio_printf("Increase # of lives - type \"lives\"\n\r");
	cio_printf("\n\rType \"help\" for a list of commands.\n\r\n\r");
	
	__enable_interrupt();

	for (;;) {
	    int j = 0;                              // Char array counter
	    char cmd_line[90] = {0};		    // Init empty array

	    cio_print((char *) "$ ");               // Display prompt
	    char c = cio_getc();                    // Wait for a character
	    while(c != '\r') {                      // until return sent then ...
	      if(c == 0x08) {                       //  was it the delete key?
		if(j != 0) {                        //  cursor NOT at start?
		  cmd_line[--j] = 0;                //  delete key logic
		  cio_printc(0x08); cio_printc(' '); cio_printc(0x08);
		}
	      } else {                              // otherwise ...
		cmd_line[j++] = c; cio_printc(c);   //  echo received char
	      }
	      c = cio_getc();                       // Wait for another
	    }

	    cio_print((char *) "\n\n\r");           // Delimit command result

	    switch(shell_process(cmd_line))         // Execute specified shell command
	    {                                       // and handle any errors
	      case SHELL_PROCESS_ERR_CMD_UNKN:
		cio_print((char *) "ERROR, unknown command given\n\r");
		break;
	      case SHELL_PROCESS_ERR_ARGS_LEN:
		cio_print((char *) "ERROR, an arguement is too lengthy\n\r");
		break;
	      case SHELL_PROCESS_ERR_ARGS_MAX:
		cio_print((char *) "ERROR, too many arguements given\n\r");
		break;
	      default:
		break;
	    }

	    cio_print((char *) "\n");               // Delimit Result
	  }


}

/******
 *
 *    PROTOTYPES
 *
 ******/
int shell_cmd_help(shell_cmd_args *args);
int shell_cmd_argt(shell_cmd_args *args);
int shell_cmd_diffUp(shell_cmd_args *args);
int shell_cmd_moarLives(shell_cmd_args *args);
/******
 *
 *    SHELL COMMANDS STRUCT
 *
 ******/
shell_cmds my_shell_cmds = {
  .count = 4,
  .cmds  = {
    {
      .cmd  = "help",
      .desc = "list available commands",
      .func = shell_cmd_help
    },
    {
      .cmd  = "args",
      .desc = "print back given arguments",
      .func = shell_cmd_argt
    },
    {
      .cmd  = "diffUp",
      .desc = "increases difficulty",
      .func = shell_cmd_diffUp
    },
    {
      .cmd  = "moarLives",
      .desc = "adds a life",
      .func = shell_cmd_moarLives
    }
  }
};

/******
 *
 *    CALLBACK HANDLERS
 *
 ******/
int shell_cmd_help(shell_cmd_args *args)
{
  int k;

  for(k = 0; k < my_shell_cmds.count; k++) {
    cio_printf("%s: %s\n\r", my_shell_cmds.cmds[k].cmd, my_shell_cmds.cmds[k].desc);
  }

  return 0;
}

int shell_cmd_argt(shell_cmd_args *args)
{
  int k;

  cio_print((char *)"args given:\n\r");

  for(k = 0; k < args->count; k++) {
    cio_printf(" - %s\n\r", args->args[k].val);
  }

  return 0;
}

int shell_cmd_diffUp(shell_cmd_args *args)
{
	game_difficulty++;
	cio_printf("difficulty increased\n\r");
}
int shell_cmd_moarLives(shell_cmd_args *args)
{
	game_lives++;
	cio_printf("added a life\n\r");
}

int shell_process(char *cmd_line)
{
  return shell_process_cmds(&my_shell_cmds, cmd_line);
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0 (void) {

	// Calculate indevidual digits
	int d0 = display_number % 10;
	int d1 = (display_number / 10)  % 10;
	int d2 = (display_number / 100)  % 10;
	
	//Display digits
	switch(display_counter % 3) {
		case 0:
			displayDigit(d0, 0);
			break;
		case 1:
			displayDigit(d1, 1);
			break;
		case 2:
			displayDigit(d2, 2);
			break;
	}
	if (display_counter == 300/game_difficulty) {
		display_number++;
		display_counter = 0;
	}
	if (display_number == 999) {
		display_number = 0;
	}
	display_counter++;
	
	//Toggle RGB LED
	if (game_lives > 2) {
		P2OUT &= ~BIT7;
		P2OUT |= BIT6;
	}
	else if (game_lives == 2) {
		P2OUT |= BIT6;
		P2OUT |= BIT7;
	}
	else if (game_lives == 1) {
		P2OUT |= BIT7;
		P2OUT &= ~BIT6;
	}
}
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void) {
	//Turn off display
	P1OUT  &= ~0b10110001;
	P2OUT  &= ~0b00000111;
	
	//Output number
	char *outNum;
	outNum = (char *) malloc(4);
	sprintf(outNum, "%d", display_number);
	cio_printf("\n\r\n\rStopped on: %s\n\r", outNum);
	
	//Initialize test cases (makeshift booleans)
	int c0 = 0;
	int c1 = 0;
	int c2 = 0;

	// Test for prime
	int i = 0;
	while (i<168 && c2==0) {
		if (display_number == primes[i]) {
			c2=1;
		}
	i++;
	}
	// Test for power of 2
	i=0;
	while (i<9 && c1==0) {
		if (display_number == powersOfTwo[i]) {
			c1=1;
		}
		i++;
	}
	
	char *lvlNum;
	lvlNum = (char *) malloc(5);
	int livesStart = game_lives;

	// Test for even number
	if (display_number%2 == 0) { c0 = 1; }
	
	// Check player input
	if (game_mode == 0 && c0 == 1) {
		cio_printf("%s is even!\n\rNow stop on a power of 2.\n\r\n\r", outNum);
	}
	else if (game_mode == 1 && c1 == 1) {
		cio_printf("%s is a power of 2!\n\rNow stop on a prime.\n\r\n\r", outNum);
	}
	else if (game_mode == 2 && c2 == 1) {
		sprintf(lvlNum, "%d", (game_level+1));
		cio_printf("%s is prime!\n\rProceeding to level %s\n\r\n\r", outNum, lvlNum);
		game_level++;
		game_difficulty += 1;
	}
	else {
		game_lives--;
	}
	if (game_mode == 0 && c0 != 1) {
		cio_printf("%s is not even!\n\r-1 life", outNum);
		if (game_lives != 0) {
			cio_printf(", try again.");
		}
		cio_printf("\n\r\n\r");
	}
	else if (game_mode == 1 && c1 != 1) {
		cio_printf("%s is not a power of 2!\n\r-1 life", outNum);
		if (game_lives != 0) {
			cio_printf(", try again.");
		}
		cio_printf("\n\r\n\r");
	}
	else if (game_mode == 2 && c2 != 1) {
		cio_printf("%s is not prime!\n\r-1 life", outNum);
		if (game_lives != 0) {
			cio_printf(", try again.");
		}
		cio_printf("\n\r\n\r");
	}
	if (game_lives != livesStart && game_lives != 0) {
		char *livesNum;
		livesNum = (char *) malloc(5);
		sprintf(livesNum, "%d", game_lives);
		cio_printf("( %s live", livesNum);
		if (game_lives > 1) {
			cio_print("s left)\n\r\n\r");
		}
		else {
			cio_print("s left)\n\r\n\r");
		}
	}
	else if (game_lives == 0) {
		if (game_level > 5) {
			cio_print("Nooooo, you were almost there!");
		}
		display_number = 0;
		display_counter = 0;
		game_mode = 0;
		game_lives = 3;
		game_difficulty = 1;
		game_level = 1;
	
		sprintf(lvlNum, "%d", game_level-1);
		cio_printf("you survived %s rounds.\n\rRestarting!\n\r", lvlNum);
	}
	
	cio_print("$ ");
	
	// Play sound
	if ((game_mode == 0 && c0 == 1) ||
	    (game_mode == 1 && c1 == 1) ||
	    (game_mode == 2 && c2 == 1))
	{
		while (frequency < 1761) {	
			setPWMFrequency(frequency);
			__delay_cycles(100000);
			frequency *= 2;
		}
		disablePWM();
		frequency = 440;
		
		//Set next game mode
		game_mode++;
		game_mode %= 3;
	}
	else
	{
		while (frequency > 219) {	
			setPWMFrequency(frequency);
			__delay_cycles(100000);
			frequency /= 2;
		}
		disablePWM();
		frequency = 440;

	}
	display_number = 0;
	
	while (!(BIT3 & P1IN)) {}
	__delay_cycles(32000);
	P1IFG &= ~BIT3;
}


