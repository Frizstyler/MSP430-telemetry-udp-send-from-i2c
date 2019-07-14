//UNDER CONSTRUCTION

//Copyright (c) 2018 Alex Kallergis

//based on examples from:
// Copyright (C) Texas Instruments Incorporated - http://www.ti.com/


/*This project incorporates an msp430g2553 MCU which calculates the input
 * signal for a servo,and relays the 2-byte result to a CC3200 Wireless MCU via i2c after request.
 * The cc3200 has an AP open where one can connect and receive the UDP packets with the telemetry.
 *This is the msp430 code.

//connections:
*P1.1<--->servo signal

*P1.6<---->UCB0SCL
*P1.7<---->UCB0SDA
*/

#include <msp430.h>

volatile unsigned int temp,stpcnt;
unsigned char of,TXData,LSBs,again;		// txdata Used to hold TX data,again used to notify ready to calculate tdif
unsigned short int diffus;		//16-bit
int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  P1DIR |= BIT0;                            // P1.0 output
  P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
  P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
  P1DIR &= ~BIT1;							//SERVO IN
  P1SEL |= BIT1;

  
  TA0CCTL0 = CAP + CM_3 + CCIE + SCS + CCIS_0;
  TA0CTL |= TASSEL_2 + MC_2 + TACLR+ TAIE;        // SMCLK, Cont Mode; start timer

  UCB0CTL1 |= UCSWRST;                      // Enable SW reset
  UCB0CTL0 = UCMODE_3 + UCSYNC;             // I2C Slave, synchronous mode
  UCB0I2COA = 0x48;                         // Own Address is 048h
  UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operation
  UCB0I2CIE |= UCSTPIE;                     // Enable STT interrupt
  IE2 |= UCB0TXIE;                          // Enable TX interrupt
  __bis_SR_register(GIE);
  while (1)
  { __no_operation();                     // For debugger
    __bis_SR_register(CPUOFF);        // Enter LPM0

  }
}

// USCI_B0 Data ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCIAB0TX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0TX_VECTOR))) USCIAB0TX_ISR (void)
#else
#error Compiler not supported!
#endif
{

  if (LSBs){
	  TA0CCTL0 |=CCIE;		//whole register has been transmitted,can update it now
	  UCB0TXBUF = diffus&0xff;                       // TX data
	  LSBs=0;		//next time is MSBs
  }
  else{
	  TA0CCTL0 &=~CCIE;		//wait for the LSBs before updating the difference register
	  UCB0TXBUF = ((diffus&0xff00)>>8);                       // TX data
	  LSBs=1;
  }
}

// USCI_B0 State ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCIAB0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{stpcnt+=1;
	LSBs=0;									//transmit starts with msbs
  UCB0STAT &= ~UCSTPIFG;                    // Clear stp condition int flag
}

// Timer_A3 Interrupt Vector (TA0IV) handler
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A(void)
{
	if (TA0IV==TA0IV_TAIFG){


		//TACTL &=~(TAIFG);	//not needed,we have accessed taiv
	}
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0(void)
{	//CCTL0&=~(CCIFG);	//not needed
	if (again>0)
	{
		again=0;
		diffus=TA0CCR0-temp;
		__bic_SR_register_on_exit(LPM0_bits);  // Exit LPM0 on return to main
	}
	else if (TA0CCTL0 & CCI){		//MAKE SURE CCI IS HIGH
		temp=TA0CCR0;
		again=1;
	}
}
