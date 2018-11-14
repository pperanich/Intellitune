/*
 * File: freq_counter.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will contain the functions necessary to determine
 *              the frequency of the transmitted RF wave.
 *
 ******************************************************************************/

#include "intellitune.h"

// Globals
uint16_t overflowCount;
uint16_t frequency;
uint32_t total_pulses;


// Function Prototypes
void measure_freq(void);
void initialize_freq_counter(void);

// TODO: Implement frequency measuring
void measure_freq(void)
{
    static const uint8_t timing_correction = 2;

    if(TB1CTL != MC_0) { return; } // Frequency is currently being measured
    // Timer1_B3 and Timer0_B3 setup

    overflowCount = 0;

    //Reset the timer1 count register to 0.
    TB1R = 0;

    // TBCCR0 interrupt enabled and set interval to 1 sec
    TB3CCR5 = TB3R + 32768 + timing_correction;
    TB3CCTL5 = CCIE;
    // 16-bit, TBxCLK, halt mode, divide by 4, enable Interrupt
    TB1CTL = (CNTL_0 | TBSSEL_2 | MC__CONTINUOUS | ID_2 | TBIE);
}


// TODO: Function to initialize and configure timers for frequency counter
void initialize_freq_counter(void)
{
    // Pin P2.2 will be used as the external source. This corresponds with TB1CLK.
    // Configure P2.2 as GPIO input
    P2DIR &= ~BIT2;
    P2SEL0 |= BIT2;
    P2SEL1 &= ~BIT2;

    // IDEX divide by 4
    TB1EX0 = TBIDEX_3;
}


#pragma vector=TIMER1_B1_VECTOR
__interrupt void Timer1_B1(void)
{
    overflowCount++;
    TB1IV = 0;
}
