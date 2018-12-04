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
inline void sample_adc_channel(uint8_t adc_channel);
inline void update_adc_value(uint16_t adc_reading);
uint16_t median(uint16_t samples[]);




// Globals
uint8_t adc_channel_select = FWD_PIN;
uint16_t fwd_sample[24] = {0};
uint16_t ref_sample[24] = {0};
uint16_t fwd_25_sample[24] = {0};
uint16_t ref_25_sample[24] = {0};
uint16_t cap_sample = 3;
uint16_t ind_sample = 3;
uint16_t median_fwd_sample = 0;
uint16_t median_ref_sample = 0;
uint16_t median_ref_sample_25 = 0;
uint16_t median_fwd_sample_25 = 0;

// TODO: Initialize ADC module
void initialize_adc(void)
{
    // Configure pins 5.0->5.3 ADC inputs
    P5DIR &= ~0x0C;
    P5SEL0 |= 0x0C;
    P5SEL1 |= 0x0C;

    // Configure ADC
    ADCCTL0 &= ~ADCENC; // Disable ADC
    ADCCTL0 |= ADCSHT_4 | ADCON; // 16ADCclks, MSC, ADC ON
    ADCCTL1 |= ADCSHP; // s/w trig, single ch/conv, MODOSC
    ADCCTL2 &= ~ADCRES; // clear ADCRES in ADCCTL
    ADCCTL2 |= ADCRES_2; // 12-bit conversion results
    ADCMCTL0 |= ADCSREF_1; // Vref=1.5V
    ADCIE |= ADCIE0; // Enable ADC conv complete interrupt

    // Configure reference
    PMMCTL0_H = PMMPW_H;                                        // Unlock the PMM registers
    PMMCTL2 |= INTREFEN | REFVSEL_0;                            // Enable internal 1.5V reference
    while(!(PMMCTL2 & REFGENRDY));                            // Poll till internal reference settles

    adc_flg |= ADC_STATUS;
    TB0R = 0;
    TB0CCR1  = 64; // First interrupt will start the first sample and conversion
    TB0CCTL1 = CCIE; // Compare interrupt enable
    TB0CTL   = (CNTL_0 | TBSSEL_1 | MC__CONTINUOUS); // ACLK as clock source, continuous mode
}


//TODO: Sample ADC function
inline void sample_adc_channel(uint8_t adc_channel)
{
    ADCCTL0 &= ~ADCENC;
    ADCMCTL0 &= ~ADCINCH;
    ADCMCTL0 |= adc_channel;
    adc_flg &= ~ADC_STATUS;
    TB0CCR1  = TB0R + 40; // Time delay to let adc channel RC circuit charge
}


// TODO: Function to update adc sample to most recent value
inline void update_adc_value(uint16_t adc_reading)
{
    static uint8_t i = 0;
    static uint8_t j = 0;
    if(i == 24) {
        i = 0;
        median_fwd_sample = median(fwd_sample);
        median_ref_sample = median(ref_sample);
        adc_flg |= SWR_SENSE;
    }
    if(j == 24) {
        j = 0;
        median_fwd_sample_25 = median(fwd_25_sample);
        median_ref_sample_25 = median(ref_25_sample);
        adc_flg |= SWR_KNOWN_SENSE;
        P3OUT &= ~BIT6; // Switch out impedance after reading FWD and REF
        adc_flg &= ~IMP_SWITCH;
    }
    switch(adc_channel_select)
    {
    case FWD_PIN:
        adc_channel_select = REF_PIN;
        if(adc_flg & IMP_SWITCH) // The known impedance is switched in
        {
            fwd_25_sample[j] = adc_reading;
        } else {
            fwd_sample[i] = adc_reading;
        }
        break;

    case REF_PIN:
        adc_channel_select = FWD_PIN;
        if(adc_flg & IMP_SWITCH) // The known impedance is switched in
        {
            ref_25_sample[j++] = adc_reading;
        } else {
            ref_sample[i++] = adc_reading;
        }
        break;
    }
    adc_flg |= ADC_STATUS;
    TB0CCR1  = TB0R + 40; // Time delay to next adc sample interval
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
    update_adc_value(ADCMEM0);
    break;
  default:
    break;
  }
}


#pragma vector=TIMER0_B1_VECTOR
__interrupt void Timer0_B1(void)
{
    switch( TB0IV ) // Determine interrupt source
    {
        case TBIV_2: // CCR1 caused the interrupt - used for adc_sampling
        {
          if(adc_flg & ADC_STATUS) // ADC is ready to sample next channel
          {
              sample_adc_channel(adc_channel_select);
          } else if(!(ADCCTL0 & ADCENC))
          {
              ADCCTL0 |= ADCENC | ADCSC; // Sampling start
          }
          break;
        }

        case TBIV_4:
        {
          asm("   NOP");
          break;
        }
    }
}


uint16_t median(uint16_t samples[]) {
    uint16_t temp;
    uint8_t i, j;
    uint16_t n = sizeof(samples)/sizeof(uint16_t);
    // the following two loops sort the array of samples in ascending order
    for(i=0; i<n-1; i++) {
        for(j=i+1; j<n; j++) {
            if(samples[j] < samples[i]) {
                // swap elements
                temp = samples[i];
                samples[i] = samples[j];
                samples[j] = temp;
            }
        }
    }

    if(n%2==0) {
        // if there is an even number of elements, return mean of the two elements in the middle
        return((samples[n/2] + samples[n/2 - 1]) / 2);
    } else {
        // else return the element in the middle
        return samples[n/2];
    }
}
