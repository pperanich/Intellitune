/*
 * File: standing_wave_sensor.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will define the functions necessary to sample the FWD
 *              and REF voltages derived from the SWR sensor board. With these measurements,
 *              the SWR will be calculated and returned to the main loop. In addition,
 *              another function will regulate the digipot on the SWR board to protect
 *              the launchpad from voltage inputs above its rating.
 *
 ******************************************************************************/

#include "intellitune.h"


// Function Prototypes
SWR measure_swr(void);
void initialize_spi(void);
void update_digipot(void);

// Globals
uint8_t DATA_BYTE = 0x80;
uint8_t CMD_BYTE = 0x13;

// TODO: Implement SWR measurement function
SWR measure_swr(void)
{
    // Not Implemented
    asm("    NOP");
    SWR temp;
    temp.fwd = 0;
    temp.ref = 0;
    return temp;
}


// TODO: Init SPI function for digital potentiometer
void initialize_spi(void)
{
    // Configure P4.4 as output for Chip Select Line,
    // P4.5 as UCB1CLK and P4.6 as UCB1SIMO
    P4DIR |= BIT4 | BIT5 | BIT6;
    P4SEL0 |= BIT5 | BIT6;

    // Stop USCI
    UCB1CTLW0 |= UCSWRST;

    // Configure the eUSCI_B1 module for 3-pin SPI at 1 MHz
    // Use SMCLK at 16 MHz
    UCB1CTLW0 |= UCSSEL__SMCLK + UCMODE_0 + UCMST + UCSYNC + UCMSB + UCCKPH;
    UCB1BRW |= 0x10; //1 MHz SPI clock = SMCLK / 16
}


// TODO: Change digipot for safe ADC voltages
void update_digipot(void)
{
    volatile unsigned char LoopBack_cmd;
    volatile unsigned char LoopBack_data;

    P4OUT &= ~BIT4; //Pull select line low on P4.4
    UCB1STATW |= UCLISTEN;  //Enable loopback mode since no MISO line
    UCB1CTLW0 &= ~UCSWRST; //Start USCI


    // First write the Command Byte
    while (!(UCB1IFG & UCTXIFG)); //Check if it is OK to write
    UCB1TXBUF = CMD_BYTE; //Load data into transmit buffer

    while (!(UCB1IFG & UCRXIFG)); //Wait until complete RX byte is received
    LoopBack_cmd = UCB1RXBUF; //Read buffer to clear RX flag

    // Then write the Data Byte
    while (!(UCB1IFG & UCTXIFG)); //Check if it is OK to write
    UCB1TXBUF = DATA_BYTE; //Load data into transmit buffer

    while (!(UCB1IFG & UCRXIFG)); //Wait until complete RX byte is received
    LoopBack_data = UCB1RXBUF; //Read buffer to clear RX flag


    UCB1CTLW0 |= UCSWRST; //Stop USCI
    UCB1STATW &= ~UCLISTEN;  //Disable loopback mode
    P4OUT |= BIT4; //De-select digi pot on SPI

    asm("    NOP");
}

/*
unsigned char ADC_Result[8]={0};
void init_ADC_using_TA1_trigger()
{
  // Configure ADC A0~7 pins
  SYSCFG2  = ADCPCTL0 | ADCPCTL1 | ADCPCTL2 | ADCPCTL3 | ADCPCTL4 | ADCPCTL5 | ADCPCTL6 | ADCPCTL7;


  // Configure ADC

  //@\!   change ADCSHTx bits to change the sample time to get  presicion ADC result

  ADCCTL0 |= ADCSHT_2 |  ADCON;                          // 16ADCclks, , ADC ON
  ADCCTL1 |= ADCSHS_2 |  ADCSHP | ADCCONSEQ_3 | ADCSSEL_0;     // ADC clock MODCLK, sampling timer, TA1 trig.,repeat  sequence channel
  ADCCTL2 &= ~ADCRES;                                         // 8-bit conversion results
  ADCMCTL0 |= ADCINCH_7 | ADCSREF_0;        // A7~0(EoS); Vref=Vcc
  ADCIE |= ADCIE0;                                                 // Enable ADC conv complete interrupt
  ADCCTL0 |= ADCENC;                                           // ADC Enable
}

// ADC interrupt service routine

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
  static char i = 7;
  switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
  {
  case ADCIV_NONE:
    break;
  case ADCIV_ADCOVIFG:
    break;
  case ADCIV_ADCTOVIFG:
    break;
  case ADCIV_ADCHIIFG:
    break;
  case ADCIV_ADCLOIFG:
    break;
  case ADCIV_ADCINIFG:
    break;
  case ADCIV_ADCIFG:
    ADC_Result[i] = ADCMEM0;

     if i = 0,means:
     (1)sequence ADC channels sample has been completed,
     (2)you can stop ADC conversion here to ignore data overwrite to the ADC result array

    if(i == 0)
    {
        i = 7;

        //P2OUT ^= BIT0;//toggle P2.0 to see whether the period of the ADC sample is correct
         __no_operation();
    }
    else
    {
      i--;
    }
    break;
  default:
    break;
  }
}*/
