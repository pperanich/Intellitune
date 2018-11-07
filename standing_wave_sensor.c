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
void initialize_adc(void);
void update_digipot(void);

// Globals
uint8_t DATA_BYTE = 0x80;
uint8_t CMD_BYTE = 0x13;
unsigned int adc_result;


// TODO: Initialize ADC module
void initialize_adc(void)
{
    // Configure ADC
    ADCCTL0 &= ~ADCENC; // Disable ADC
    ADCCTL0 |= ADCSHT_14 | ADCON; // 16ADCclks, MSC, ADC ON
    ADCCTL1 |= ADCSHP; // s/w trig, single ch/conv, MODOSC
    ADCCTL2 &= ~ADCRES; // clear ADCRES in ADCCTL
    ADCCTL2 |= ADCRES_2; // 12-bit conversion results
    ADCMCTL0 |= ADCSREF_1; // Vref=1.5V
    ADCIE |= ADCIE0; // Enable ADC conv complete interrupt

    // Configure reference
    PMMCTL0_H = PMMPW_H;                                        // Unlock the PMM registers
    PMMCTL2 |= INTREFEN | REFVSEL_0;                            // Enable internal 1.5V reference
    while(!(PMMCTL2 & REFGENRDY));                            // Poll till internal reference settles
}


//TODO: Sample ADC function
void sample_adc_channel(uint16_t channel)
{
    ADCCTL0 &= ~ADCENC;
    ADCMCTL0 &= ~ADCINCH;
    ADCMCTL0 |= channel; // A10
    ADCCTL0 |= ADCENC | ADCSC; // Sampling and conversion start
    adc_ready = 0;
}


// TODO: Implement SWR measurement function
_iq16 measure_ref_coeff(void)
{
    unsigned int fwd_sample, ref_sample;
    _iq19 numerator, denominator, reflection_coefficient;
    ADCCTL0 &= ~ADCENC;
    ADCMCTL0 &= ~ADCINCH;
    ADCMCTL0 |= ADCINCH_10; // A10
    ADCCTL0 |= ADCENC | ADCSC; // Sampling and conversion start
    while(ADCCTL1 & ADCBUSY); // Wait if ADC core is active
    fwd_sample = adc_result;

    ADCCTL0 &= ~ADCENC;
    ADCMCTL0 &= ~ADCINCH;
    ADCMCTL0 |= ADCINCH_11; // A11
    ADCCTL0 |= ADCENC | ADCSC; // Sampling and conversion start
    while(ADCCTL1 & ADCBUSY); // Wait if ADC core is active
    ref_sample = adc_result;

    numerator = _IQ19(ref_sample);
    denominator = _IQ19(fwd_sample);
    reflection_coefficient = _IQ19div(numerator, denominator);
    return _IQ19toIQ(reflection_coefficient);
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


// ADC interrupt service routine
#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void)
{
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
    adc_result = ADCMEM0;
    break;
  default:
    break;
  }
}




ADCCTL0 &= ~ADCENC;
ADCMCTL0 &= ~ADCINCH_9 & ~ADCINCH_10 & ~ADCINCH_11;
ADCMCTL0 |= ADCINCH_8; // A8
ADCCTL0 |= ADCENC | ADCSC;         // Sampling and conversion start
while(ADCCTL1 & ADCBUSY);                                // Wait if ADC core is active
cap_sample = _IQ16(adc_result);

ADCCTL0 &= ~ADCENC;
ADCMCTL0 &= ~ADCINCH_8 & ~ADCINCH_10 & ~ADCINCH_11;
ADCMCTL0 |= ADCINCH_9; // A9
ADCCTL0 |= ADCENC | ADCSC;         // Sampling and conversion start
while(ADCCTL1 & ADCBUSY);                                // Wait if ADC core is active
ind_sample = _IQ16(adc_result);
