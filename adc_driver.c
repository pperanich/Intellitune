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


// Globals
uint8_t adc_channel_select = FWD_PIN;
uint16_t cap_sample = 0;
uint16_t ind_sample = 0;
uint16_t fwd_sample = 0;
uint16_t ref_sample = 0;
uint16_t fwd_25_sample = 0;
uint16_t ref_25_sample = 0;


// TODO: Initialize ADC module
void initialize_adc(void)
{
    // Configure pins 5.0->5.3 ADC inputs
    P5DIR &= ~0x0f;
    P5SEL0 |= 0x0f;
    P5SEL1 |= 0x0f;

    // Configure ADC
    ADCCTL0 &= ~ADCENC; // Disable ADC
    ADCCTL0 |= ADCSHT_8 | ADCON; // 16ADCclks, MSC, ADC ON
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
    TB0CCR1  = TB0R + 2; // Time delay to let adc channel RC circuit charge
}


// TODO: Function to update adc sample to most recent value
inline void update_adc_value(uint16_t adc_reading)
{
    switch(adc_channel_select)
    {
    case FWD_PIN:
        adc_channel_select = REF_PIN;
        if(adc_flg & IMP_SWITCH) // The known impedance is switched in
        {
            fwd_25_sample = adc_reading;
        } else {
            fwd_sample = adc_reading;
        }
        break;

    case REF_PIN:
        adc_channel_select = IND_PIN;
        if(adc_flg & IMP_SWITCH) // The known impedance is switched in
        {
            ref_25_sample = adc_reading;
            adc_flg |= SWR_KNOWN_SENSE;
            P3OUT &= ~BIT6; // Switch out impedance after reading FWD and REF
            adc_flg &= ~IMP_SWITCH;
        } else {
            ref_sample = adc_reading;
            adc_flg |= SWR_SENSE;
        }
        break;

    case IND_PIN:
        adc_channel_select = CAP_PIN;
        ind_sample = adc_reading;
        adc_flg |= IND_POT;
        break;

    case CAP_PIN:
        adc_channel_select = FWD_PIN;
        cap_sample = adc_reading;
        adc_flg |= CAP_POT;
        break;
    }
    adc_flg |= ADC_STATUS;
    TB0CCR1  = TB0R + 4; // Time delay to next adc sample interval
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
