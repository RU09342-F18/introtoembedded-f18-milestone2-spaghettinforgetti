#include <msp430.h> 
#include <math.h>

/*
 * Milestone2
 */

double thermistor;
double tVoltage;
double temp;
double ln;
double tempk;
double tempf;
int setTemp = 75;

int main(void)
{

    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT

    //Timer --------------------------------------------------------------------------------------------------------------------
    TA0CTL = TASSEL_2 + MC_1 + OUTMOD_7;      // Configures the timer for SMClk, Timer in UP mode , and outmode of Reset/set
    TA0CCTL1 = OUTMOD_2;                      // Sets TACCR1 to toggle
    TA0CCR0 = 255;                            // Sets TA0CCR0
    TA0CCR1 = 0;                              // Sets TA0CCR1

    TA1CTL = TASSEL_1 + MC_1 + TAIE;
    TA1CCTL1 |= CCIE;
    TA1CCR0 = 50000;

    //ADC ---------------------------------------------------------------------------------------------------------------------
    ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on
    ADC12CTL1 = ADC12SHP;                     // Use sampling timer
    ADC12IE = 0x01;                           // Enable interrupt
    ADC12CTL0 |= ADC12ENC;
    P6SEL |= 0x01;                            // P6.0 ADC option select

    //PWM -----------------------------------------------------------------------------------------------------------------------
    P1SEL |= BIT2;                            // Sets port 1.2 to TimerA CCR1
    P1DIR |= BIT2;                            // Sets port 1.2 to output
    P1SEL |= BIT0;                            // Sets port 1.0 to GPIO
    P1DIR |= BIT0;                            // Sets port 1.0 to output

    //UART ------------------------------------------------------------------------------------------------------------------------
    P4SEL |= (BIT4+BIT5);                     // P4.4 & 4.5 = USCI_A1 TXD/RXD
    UCA1CTL1 |= UCSWRST;                      //State machine reset + small clock initialization
    UCA1CTL1 |= UCSSEL_2;                     //SMCLK
    UCA1BR0 = 6;                              //9600 baud
    UCA1BR1 = 0;                              //9600 baud
    UCA1MCTL |= UCBRS_0 + UCBRF_13 + UCOS16;
    UCA1CTL1 &= ~UCSWRST;                     // Initialize USCI state machine
    UCA1IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt

    __bis_SR_register(GIE);                   // Enter LPM0, interrupts enabled
   while (1)
   {
       __delay_cycles(10000);
       ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion
   }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A1_VECTOR))) USCI_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{

  switch(__even_in_range(UCA1IV,4))
  {
  case 0:break;                             // Vector 0 - no interrupt
  case 2:                                   // Vector 2 - RXIFG
        TA0CCR1 = UCA1RXBUF;                //sets PWM
  break;
  case 4:break;                             // Vector 4 - TXIFG
  default: break;
  }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
      while (!(UCA1IFG&UCTXIFG));
      tVoltage = 0.000806 * ADC12MEM0;
      thermistor = 10000*((3.3/tVoltage)-1);
      ln = log(thermistor/10000);
      tempk = 1/(.003354 + .000256985*ln);      //temperature in kelvin
      tempf = 1.8*(tempk - 273) + 32;           //temperature in fahrenheit
      UCA1TXBUF =  (unsigned short int) tempf; // transmit value in ADC

    __bic_SR_register_on_exit(LPM0_bits);   // Exit active CPU
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}

#pragma vector = TIMER1_A1_VECTOR
__interrupt void TimerA (void)
{
    switch(TA1IV)
    {
        if(tempf > setTemp)
        {
            P1OUT ^= BIT0;
            TA0CCR1 += 45;
        }
        else if(tempf < setTemp)
        {
            TA0CCR1 -= 45;
        }
    }
}

