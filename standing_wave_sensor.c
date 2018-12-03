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
 *              Important side note: digital potentiometer should be connected as follows:
 *              A terminal to ground
 *              B terminal to 1.5V
 *              Wiper terminal to ADC pin
 *
 ******************************************************************************/

#include "intellitune.h"


// Function Prototypes
void initialize_spi(void);
void update_digipot(void);
_iq16 calculate_ref_coeff(uint8_t reflection_to_calc);
void update_swr(void);

// Globals
uint8_t DATA_BYTE = 0x80;
const uint8_t CMD_BYTE = 0x13;


// TODO: Implement SWR measurement function
_iq16 calculate_ref_coeff(uint8_t reflection_to_calc)
{
    _iq19 numerator, denominator, reflection_coefficient;
    switch(reflection_to_calc)
    {
        case KNOWN_SWITCHED_IN:
        {
            if(adc_flg & SWR_KNOWN_SENSE)
            {
                adc_flg &= ~SWR_KNOWN_SENSE;
                numerator = _IQ19(median_ref_sample_25);
                denominator = _IQ19(median_fwd_sample_25);
                reflection_coefficient = _IQ19div(numerator, denominator);
                return _IQ19toIQ(reflection_coefficient);
            } else {
                return 0;
            }
        }

        case KNOWN_SWITCHED_OUT:
        {
            if(adc_flg & SWR_SENSE)
            {
                adc_flg &= ~SWR_SENSE;
                numerator = _IQ19(median_ref_sample);
                denominator = _IQ19(median_fwd_sample);
                if(numerator > denominator) { return 0; }
                if(median_fwd_sample < 100) { return 0; }
                reflection_coefficient = _IQ19div(numerator, denominator);
                return _IQ19toIQ(reflection_coefficient);
            } else {
                return 0;
            }
        }
        default:
            return 0;
    }
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
    uint8_t update_needed = 0;
    if((DATA_BYTE != 255) && (median_fwd_sample > 4050))
    {
        DATA_BYTE++; // Increase resistance to lower voltage
        update_needed = 1;
    } else if((DATA_BYTE != 0) && (median_fwd_sample < 1024))
    {
        DATA_BYTE--; // Decrease resistance to increase voltage
        update_needed = 1;
    }
    if(update_needed)
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
    }
}


// TODO: Update the internal reference if adc input threshold exceeded
void update_internal_reference(void)
{
    asm("   NOP");
}


// TODO: Update SWR reading from most recent ADC values
void update_swr(void)
{
    static const _iq16 iq_one = _IQ16(1.0);
    char buf1[6] = {'\0'};
    char buf2[6] = {'\0'};
    uint8_t error = 0;
    _iq16 numerator, denominator, vswr;
    _iq16 reflection_coefficient = calculate_ref_coeff(KNOWN_SWITCHED_OUT);
    if(reflection_coefficient == 0) { return; }
    numerator = iq_one + reflection_coefficient;
    denominator = iq_one - reflection_coefficient;
    error += _IQ16toa(buf1,"%1.3f", numerator);
    error += _IQ16toa(buf2,"%1.3f", denominator);
    vswr = _IQ16div(numerator, denominator);
    //error += _IQ16toa(swr_val,"%2.1f", vswr);
    if(vswr > _IQ16(0.0)) { error += _IQ16toa(swr_val,"%2.1f", vswr); }
    else if( vswr < _IQ16(1.0))
        { asm("   NOP"); }
}
