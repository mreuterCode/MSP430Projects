//
// MSP430G2152 LCD (HD44780) Library 4Bit
//
//
//                               5V
//                              + |  -
//                        o-------||----o--o
//                        |       |     |  |  .--------------------.
//                        |             |  |  |                    |
//                        |             |  o--|1: GND              |
//                        o-------------+-----|2: VCC              |
//         .-------------.              o-----|3: VEE (Contrast)   |
//         |         P2.0|--------------------|4: RS               |
//         |         P2.1|--------------------|5: RW               |
//         |         P2.2|--------------------|6: Enable           |
//         |             |                    |                    |
//         |         P1.0|(-----for 8Bit-----)|7: D0               |
//         |         P1.1|(-----for 8Bit-----)|8: D1               |
//         |         P1.2|(-----for 8Bit-----)|9: D2               |
//         |         P1.3|(-----for 8Bit-----)|10: D3              |
//         |         P1.4|--------------------|11: D4              |
//         |         P1.5|--------------------|12: D5              |
//         |         P1.6|--------------------|13: D6              |
//         |         P1.7|--------------------|14: D7              |
//         |             |                    |                    |
//         |          VCC|--------------o-----|15: Backlight +     |
//         |          GND|--o-----------+-----|16: Backlight -     |
//         |             |  |           |     |                    |
//         '-------------'  |      |    |     '--------------------'
//                          o-----||----o
//                              -  | +
//                                3.6V
//(c) 2013 Maximilian Reuter
//

#include <msp430g2152.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define     LCM_DIR               P1DIR
#define     LCM_OUT               P1OUT

#define RS BIT0
#define RW BIT1
#define E BIT2

#define D0 BIT0
#define D1 BIT1
#define D2 BIT2
#define D3 BIT3
#define D4 BIT4
#define D5 BIT5
#define D6 BIT6
#define D7 BIT7
int i;
int j;
int a;
int c;
int d;
int busy;
int readBus;
int checkBusy;
int pos;
int b;

int bitMode;
int useRW;
char str[];

void initialize();
void wait(int time);

void writeLCD(int bus, int rsbit, int checkBusy);
int readLCD(int rsbit2); //experimental

void writeString(char str[]);
void writeStringToPos(char str[], int strPosition);

void writeChar(char c);
void writeCharToPos(char c, int position);


void moveCursToPos(int cursPos);
void moveDisplayToPos(int dispPos);


void main(void){
    WDTCTL = WDTPW | WDTHOLD;
    bitMode = 4;	//select Bitmode 4/8

	P2OUT &= ~RW;
	P2DIR |= RW;		//if you dont want to read, pull this to gnd

	initialize();

	writeStringToPos("Hallo Welt", 0);
	writeStringToPos("hallo", 40);

	for (;;){}
}

void writeString(char str[]){
	for (b = 0; b< strlen(str); b++){
		writeChar(str[b]);
	}
}

void writeStringToPos(char str[], int strPosition){
	//set cursor to 0
	writeLCD(D1, 0, 0);

	moveCursToPos(strPosition);
	writeString(str);
}

void moveDisplayToPos(int dispPos){
	writeLCD(D1, 0, 0);

	for(d = 0; d< dispPos; d++){
		writeLCD(D4 + D3, 0, 0);
	}
}

void moveCursToPos(int cursPos){
	//set cursor to 0
	writeLCD(D1, 0, 0);
	for (c = 0; c < cursPos; c++){
		writeLCD(D4 + D2, 0, 0);
	}
}

void writeCharToPos(char c, int position){
	moveCursToPos(position);
	writeChar(c);
}

void writeChar(char c){
	writeLCD(c, 1, 0);
}

void writeLCD(int bus, int rsbit, int checkBusy){
	wait(300);
	if(rsbit){
		P2OUT |= rsbit;
	} else {
		P2OUT &= ~rsbit;
	}

	P2OUT &= ~E;
	if(bitMode == 4){
		wait(300);

		P1OUT &= ~(D4 + D5 + D6 + D7);

		P2DIR |= RS + E;
		P1DIR |= (D4 + D5 + D6 + D7);

		//first raising edge
		P2OUT |= E;
		wait(100);
		P1OUT |= (bus & (D4 + D5 + D6 + D7));
		wait(100);
		//first falling edge
		P2OUT &= ~E;
		wait(100);
		P1OUT &= ~ (D4 + D5 + D6 + D7);
		//second raising edge
		P2OUT |= E;
		wait(100);
		P1OUT |= (bus&(D0 + D1 + D2 + D3))*16;
		wait(100);
		//second falling edge
		P2OUT &= ~E;

		wait(200);
		P2OUT &= ~RS;
		P1OUT &= ~(D4 + D5 + D6 + D7);

	} else {

		P1OUT = 0;

		P2DIR |= RS + E;
		P1DIR = ~0;
		//raising edge
		P2OUT |= E;
		P1OUT = bus;
		wait(100);
		//falling edge
		P2OUT &= ~E;
		wait(200);
		P2OUT &= ~RS;
		P1OUT = 0;
	}
}


void initialize(){
	if(bitMode == 4){
		wait(15000);
		bitMode = 8;
		//set interface on 8bit
		for (j = 0; j<3; j++){
			writeLCD(D5 + D4 , 0 , 0);
			wait(30000);
			wait(30000);

		}

		//4bit
		writeLCD(D5 , 0 , 0);

		bitMode = 4;

		//set 4bit, 2 lines
		writeLCD(D3 + D5, 0, 0);

		//display off, cursor on
		writeLCD(D1 + D3, 0, 0);

		//cursor going right, no display shift
		writeLCD(D1 + D2, 0, 0);

		//display on, cursor blinking
		writeLCD(D0, 0, 0);

		wait(1000);

		//display on, cursor to beginning
		writeLCD( D2 + D3, 0, 0);

	}else{
		//set interface on 8bit
		for (j = 0; j<3; j++){
			writeLCD(D5 + D4 + D3 + D2, 0 , 0);
			wait(30000);
			wait(30000);
		}

		//turn display off
		writeLCD(D3, 0, 0);

		//clear display
		writeLCD(D0, 0, 0);

		//cursor going right, no display shift
		writeLCD(D2 + D1 + D0, 0, 0);

		//display on, cursor blinking
		writeLCD(D3 + D2 + D1 + D0 , 0, 0);

		//display to beginning
		writeLCD(D2 + D1, 0, 0);
	}



}

void wait(int time){
	for(a = 0; a<time; a++){}
}
