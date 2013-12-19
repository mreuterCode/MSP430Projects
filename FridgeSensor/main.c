#include <msp430.h>


void i2cSend(unsigned char Data);

int val = 0;
int a = 0;
unsigned int sample[11];
int temperature;
int illumination;
int getTemperatureFromSample(int spl);

void i2cSetup(int Adress);
void setupADC();

volatile unsigned char RXData;
int phase = 1;

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;
  setupADC();
  i2cSetup(0x49);
  //setup sample timer
  TA1CCTL0 |= CCIE;
  TA1CTL = TASSEL_2 | MC_2 | ID_3| TACLR;

 __bis_SR_register(LPM0_bits + GIE);        // Enter LPM0 w/ interrupts
  for(;;);

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

	  ADC10AE0 = BIT2;

	  __enable_interrupt();
	  for(c = 0; c<100; c++){}

	  ADC10CTL0 |= ENC;
	  TACCTL1 = OUTMOD_4;
	  TACTL = TASSEL_2 + MC_2;
}

// USCI_B0 Data ISR
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
{
	TA1CCTL0 &= ~CCIE;
	__disable_interrupt();
	if(phase == 1){
		RXData = UCB0RXBUF;
		phase = 2;
		IE2 &= ~UCB0RXIE;
		IE2 |= UCB0TXIE;
	}else if (phase == 2){
		if(RXData == 0x01){
			UCB0TXBUF = temperature;
		} else if(RXData == 0x02){
			UCB0TXBUF = illumination;
		}
		IE2 &= ~UCB0TXIE;
		phase = 1;
		IE2 |= UCB0RXIE;

	}
	TA1CCTL0 |= CCIE;
	__enable_interrupt();
  //RXData = UCB0RXBUF;                       // Get RX data
  //__bic_SR_register_on_exit(CPUOFF);        // Exit LPM0

}
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
	 ADC10CTL0 &= ~ADC10ON;

	val = ADC10MEM - 418;
	if(val >= 25){
		P1OUT |= BIT0;
	} else{
		P1OUT &= ~BIT0;
	}

	ADC10CTL0 |= ADC10ON;

}
int getTemperatureFromSample(int spl){
	int a = 986;
	double b = 3.55;
	return (spl-a)/b;
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void ta1_isr(void)
{
	temperature = sample[0];
	if(sample[8]<=255){
		illumination = sample[8];
	} else {
		illumination = 255;
	}
}

void i2cSetup(int Adress){
	P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
	UCB0CTL1 |= UCSWRST;                      // Enable SW reset
	UCB0CTL0 = UCMODE_3 + UCSYNC;             // I2C Slave, synchronous mode
	UCB0I2COA = Adress;                        // Own Address
	UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
	IE2 |= UCB0RXIE;                          // Enable RX interrupt

}
#pragma vector=TIMER0_A0_VECTOR
__interrupt void ta0_isr(void)
{

}
