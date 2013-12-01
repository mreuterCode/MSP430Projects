#include <msp430.h>

int val = 0;
int a = 0;
unsigned int sample[11];
int temperature;
int getTemperatureFromSample(int spl);


int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;
  P1OUT = 0;

  int c = 0;
  ADC10CTL1 = ADC10DIV_7 + INCH_10 + SHS_1 + CONSEQ_3;
  ADC10CTL0 = SREF_1 + ADC10SHT_3 + REF2_5V  + REFON + ADC10ON;

  //setup DTC
  ADC10DTC0 = ADC10CT;
  ADC10DTC1 = 11;
  ADC10SA = (unsigned short) sample;

  ADC10AE0 = BIT0;
  P1DIR |= BIT6;

  __enable_interrupt();
  for(c = 0; c<100; c++){

  }

  ADC10CTL0 |= ENC;
  TACCTL1 = OUTMOD_4;

  TACTL = TASSEL_2 + MC_2;

  //setup sample timer
  TA1CCTL0 |= CCIE;
  TA1CTL = TASSEL_2 | MC_2 | ID_3| TACLR;
  //__bis_SR_register(LPM0_bits + GIE);
  for(;;){};
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
	P1OUT = BIT6;
	temperature = getTemperatureFromSample(sample[0]);
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void ta0_isr(void)
{

}
