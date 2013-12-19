#include <msp430.h>

//i2c SECTION<--
unsigned char TransmittedData;
int ReceivedData;
int WriteMode = 0;
unsigned char WriteRegister;
int result;
int phase = 0;
int result2;
unsigned char result3;
int justRead = 0;
void wait(int time);
void i2cSetup(int Adress);
void i2cWrite(int Adress, int Value);
int i2cRequest(unsigned char Register);
unsigned char i2cJustRead();
//-->i2c SECTION

//LCD SECTION<--
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define     LCM_DIR               P2DIR
#define     LCM_OUT               P2OUT

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

void writeSensorData(int TemperatureInside, int TemperatureOutside);
void writeString(char str[]);
void writeStringToPos(char str[], int strPosition);

void writeChar(char c);
void writeCharToPos(char c, int position);


void moveCursToPos(int cursPos);
void moveDisplayToPos(int dispPos);

//-->LCD SECTION

//MAIN
#include <stdbool.h>

void setupSampleTimer();
void setupADC();
int getTempOutside(int adcVal);
int getTempInside(int adcVal);

int currentTemperatureOutside;
int currentTemperatureInside;
int currentIlluminationOutside;
int currentIlluminationInside;

unsigned int sample[11];

char buf[12];
char buf2[12];
int count = 0;
int ab = 0;
int LightCounter = 30;
int DisplayOn = 1;
int DoorOpen = 0;
int DoorOpenAlert = 0;
int modulationCounter = 0;
unsigned int DoorShiftRegister = 0;


int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

  setupADC();
  P2OUT |= BIT0;	//display light
  P2DIR |= BIT0;


  P2REN |= BIT2;	//button input
  P2OUT &= ~BIT2;
  P2DIR &= ~BIT2;
  P2IFG = 0;
  P2IE |= BIT2;

  P1OUT &= ~BIT4;	//beeper
  P1DIR |= BIT4;

  P2SEL &= ~(BIT6 + BIT7);  //P2.6 and P2.7 have set a P2SEL bit initially for clock purpose. Clear before use!

  bitMode = 4;	//select Bitmode 4/8

  initialize();
  DisplayOn = 1;

  /*int aj = 0;
  int ai = 0;
  for(;;){
	  wait(100);
	  writeSensorData(aj,ai);
	  aj++;
	  ai++;
  }*/

  //setupSampleTimer();

  i2cSetup(0x49);

  //0x01: temperature
  //0x02: ambient illumination
  //i2cRequest(0x01);
  //__bis_SR_register(LPM0_bits + GIE);        // Enter LPM0 w/ interrupts

	for(;;){

		currentTemperatureOutside = getTempOutside(sample[0]);
		currentIlluminationOutside = sample[5];

		currentIlluminationInside = i2cRequest(0x02);
		currentTemperatureInside = getTempInside(i2cRequest(0x01));

		//modulate alert-signal
		if(modulationCounter){
			P1OUT ^= BIT4;
			LightCounter = 20;
		}

		if((currentTemperatureInside >= 25)||(currentIlluminationInside <= 200)){
			DoorOpen = 1;
			LightCounter = 20;
			//P1OUT |= BIT4;
		} else{
			if(DoorOpen == 1){
				writeLCD(BIT0, 0, 0);
			}
			DoorOpen = 0;
			//P1OUT &= ~BIT4;

		}

		if(!DoorOpen){
			DoorOpenAlert = 0;
			modulationCounter = 0;
			P1OUT&= ~BIT4;
		} else{
			LightCounter = 20;

		}

		DoorShiftRegister = DoorShiftRegister << 1;
		DoorShiftRegister += DoorOpen;

		if(DoorShiftRegister == 65535){
			DoorShiftRegister = 0;
			DoorOpenAlert++;

			if(DoorOpenAlert>5){
				P1OUT |= BIT4;
				modulationCounter = 1;
				LightCounter = 20;
			}
		}

		wait(100);
		if(LightCounter&&DisplayOn){
			LightCounter--;
			if(DoorOpen){
				writeStringToPos("Close Door!",0);
			} else{
				writeSensorData(currentTemperatureInside,currentTemperatureOutside);
			}
		} else if((LightCounter&&!DisplayOn)){
			initialize();
			P2OUT &= ~BIT0;
			DisplayOn = 1;
			if(DoorOpen){
				writeStringToPos("Close Door!",0);
			} else{
				writeSensorData(currentTemperatureInside,currentTemperatureOutside);
			}

		} else if(!LightCounter&&DisplayOn){
			writeLCD(BIT3, 0, 0);
			P2OUT |= BIT0;
			DisplayOn = 0;
		}

  };
}

#pragma vector = PORT2_VECTOR
__interrupt void PORT2_ISR(void){
	P2IE &= ~BIT2;
	LightCounter = 20;

	while(P2IFG != 0){
		P2IFG = 0;
	}
	P2IE |= BIT2;

}

int getTempOutside(int adcVal){
	double dbTemp = adcVal;
	dbTemp *= 0.611;
	dbTemp -= 246;
	int temp = (int) (dbTemp + 0.5);
	return temp;
}

int getTempInside(int adcVal){
	double dbTemp = adcVal;
	dbTemp *= 0.526;
	dbTemp -= 74.79;
	int temp = (int) (dbTemp + 0.5);
	return temp;
}

void setupADC(){
	  int c = 0;
	  ADC10CTL1 = ADC10DIV_7 + INCH_10 + SHS_1 + CONSEQ_3;
	  ADC10CTL0 = SREF_1 + ADC10SHT_3 + REF2_5V  + REFON + ADC10ON;

	  //setup DTC
	  ADC10DTC0 = ADC10CT;
	  ADC10DTC1 = 11;
	  ADC10SA = (unsigned short) sample;

	  ADC10AE0 = BIT5;

	  __enable_interrupt();
	  for(c = 0; c<100; c++){}

	  ADC10CTL0 |= ENC;
	  TACCTL1 = OUTMOD_4;
	  TACTL = TASSEL_2 + MC_2;
}

void setupSampleTimer(){
	  //setup sample timer
	  TA1CCTL0 |= CCIE;
	  TA1CTL = TASSEL_2 | MC_2 | ID_3| TACLR;
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void ta1_isr(void)
{
	TA1CCTL0 &= ~CCIE;
	//__disable_interrupt();

	currentTemperatureInside = i2cRequest(0x01);
	currentTemperatureOutside = sample[0];
	//currentTemperatureOutside = 3;
	//currentTemperatureInside = 0;
	//currenIlluminationInside = i2cRequest(0x02);
	writeSensorData(currentTemperatureInside, currentTemperatureOutside);
	TA1CTL = TASSEL_2 | MC_2 | ID_3| TACLR;

	TA1CCTL0 |= CCIE;
	//__enable_interrupt();
}

/*unsigned char i2cJustRead(){
	justRead = 1;
    while (UCB0CTL1 & UCTXSTP);			//check if STOP is sent
    IE2 &= ~UCB0TXIE;
    IE2 |= UCB0RXIE;
    UCB0CTL1 |= UCTXSTT;
    __bis_SR_register(CPUOFF + GIE);	//go into a LPM and wait for ISR

    return result3;
}*/

void i2cWrite(int Adress, int Value){
    while (UCB0CTL1 & UCTXSTP);			//check if STOP is sent
	WriteMode = 1;						//important for the ISR procedure
	WriteRegister = Adress;
	TransmittedData = Value;
  	phase = 0;							//important for ISR procedure

	IE2 &= ~UCB0RXIE;					//clear RX interrupt-enable...
	IE2 |= UCB0TXIE;					//... and set TX interrupt-enable

    UCB0CTL1 |= UCTR + UCTXSTT;	  		//send START condition + address + Write
    __bis_SR_register(CPUOFF + GIE);	//go into a LPM and wait for ISR

	WriteMode = 0;
	return;
}

int i2cRequest(unsigned char Register){

		TransmittedData = Register;
		WriteMode = 0;						//important for ISR procedure
	  	phase = 0;							//important for ISR procedure
		IE2 &= ~UCB0RXIE;					//clear RX interrupt-enable
		IE2 |= UCB0TXIE;					//set TX interrupt-enable

	    while (UCB0CTL1 & UCTXSTP);			//ensure STOP condition was sent
	    UCB0CTL1 |= UCTR + UCTXSTT;			//send START condition + address + Write
	    __bis_SR_register(CPUOFF + GIE);	//go into a LPM and wait for ISR

	    while (UCB0CTL1 & UCTXSTP);			//ensure STOP was sent

	   	return ReceivedData;
}

void i2cSetup(int Adress){
	  P1SEL |= BIT6 + BIT7;						//declare P1.6 and P1.7 as I2c pins
	  P1SEL2|= BIT6 + BIT7;
	  UCB0CTL1 |= UCSWRST;						//stop UCB0
	  UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;		//configure UCB0
	  UCB0CTL1 = UCSSEL_2 + UCSWRST;
	  UCB0BR0 = 12;
	  UCB0BR1 = 0;
	  UCB0I2CSA = Adress;						//set sensor's I2C address
	  UCB0CTL1 &= ~UCSWRST;						//start UCB0
	  IE2 |= UCB0TXIE + UCB0RXIE;				//enable TX and RX interrupts
	  return;
}

#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
	/*if(justRead){
		result3 = UCB0RXBUF;
		justRead = 0;
		IFG2 &= ~(UCB0TXIFG + UCB0RXIFG);		//disable interrupts
	    UCB0CTL1 |= UCTXSTP;					//send STOP condition and wait for the sensor-data
		__bic_SR_register_on_exit(CPUOFF);		//exit LPM
		return;
	}*/
	if(WriteMode){
		if(phase == 0){
			UCB0TXBUF = WriteRegister;				//put the register address on the bus
			phase = 1;

		} else if (phase == 1){
			phase = 2;
			UCB0TXBUF = TransmittedData;			//put data on the bus

		}else if(phase == 2){
			IFG2 &= ~(UCB0TXIFG + UCB0RXIFG);		//disable RX and TX interrupts
			UCB0CTL1 |= UCTXSTP;					//send STOP condition
			__bic_SR_register_on_exit(CPUOFF);		//exit LPM
		    WriteMode = 0;
		    phase = 0;
		}
	}else{
		//Read-mode
		if(phase == 0){
			UCB0TXBUF = TransmittedData;			//put register address on the bus
			phase = 1;

		}else if (phase == 1)
		{
			IE2 &= ~UCB0TXIE;						//disable TX interrupts
			IE2 |= UCB0RXIE;						//enable RX interrupts

			UCB0CTL1 &= ~UCTR;						//clear UCTR bit (means READ mode on next START condition)
		    UCB0CTL1 |= UCTXSTT;					//send START condition

		    while (UCB0CTL1 & UCTXSTT);				//wait for START to be sent
		    UCB0CTL1 |= UCTXSTP;					//send STOP condition and wait for the sensor-data
			phase = 2;

		}
		else if (phase == 2)
		{
			ReceivedData = UCB0RXBUF;				//read sensor-data from the bus
			IFG2 &= ~(UCB0TXIFG + UCB0RXIFG);		//disable interrupts
			__bic_SR_register_on_exit(CPUOFF);		//exit LPM

		}

	}
}



// LCD SECTION

void writeSensorData(int temperatureInside, int temperatureOutside){
	//writeLCD(BIT0, 0, 0);

  	sprintf(buf,"T_in: %dC ", temperatureInside);
	sprintf(buf2,"T_out: %dC ", temperatureOutside);

	ab++;
	if (ab == 24){
		__no_operation();
	}

	writeStringToPos(buf, 0);
  	writeStringToPos(buf2, 40);


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
		P1OUT |= rsbit;
	} else {
		P1OUT &= ~rsbit;
	}

	P1OUT &= ~E;
	if(bitMode == 4){
		wait(300);

		P2OUT &= ~(D4 + D5 + D6 + D7);

		P1DIR |= RS + E;
		P2DIR |= (D4 + D5 + D6 + D7);

		//first raising edge
		P1OUT |= E;
		wait(100);
		P2OUT |= (bus & (D4 + D5 + D6 + D7));
		wait(100);
		//first falling edge
		P1OUT &= ~E;
		wait(100);
		P2OUT &= ~ (D4 + D5 + D6 + D7);
		//second raising edge
		P1OUT |= E;
		wait(100);
		P2OUT |= (bus&(D0 + D1 + D2 + D3))*16;
		wait(100);
		//second falling edge
		P1OUT &= ~E;

		wait(200);
		P1OUT &= ~RS;
		P2OUT &= ~(D4 + D5 + D6 + D7);

	} else {

		P2OUT = 0;

		P1DIR |= RS + E;
		P2DIR |= (D4 + D5 + D6 + D7);
		//raising edge
		P1OUT |= E;
		P2OUT = bus;
		wait(100);
		//falling edge
		P1OUT &= ~E;
		wait(200);
		P1OUT &= ~RS;
		P2OUT = 0;
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
	for(a = 0; a<100; a++){}
}
