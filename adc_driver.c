/*
 * File: adc_driver.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will contain the functions for driving ADC sampling
 *              and configure a timer to continually check ADC and sample the
 *              channels sequentially.
 *
 ******************************************************************************/
#include "intellitune.h"


// Function Prototypes
void initialize_adc(void);
void current_setting(void);


// TODO: Initialize ADC module
void initialize_adc(void)
{
    // Configure pins 5.0->5.3 ADC inputs
    P5SEL0 |= 0x0f;
    P5SEL1 |= 0x0f;

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
void sample_adc_channel(uint8_t channel)
{
    ADCCTL0 &= ~ADCENC;
    ADCMCTL0 &= ~ADCINCH;
    ADCMCTL0 |= channel; // A10
    ADCCTL0 |= ADCENC | ADCSC; // Sampling and conversion start
    adc_ready = 0;
}


// TODO: Implement current settings function.
void current_setting(void)
{
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
    adc_ready = 1;
    break;
  default:
    break;
  }
}
