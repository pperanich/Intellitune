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
uint16_t overflowCount = 0;


// Function Prototypes
uint16_t measure_freq(void);
void initialize_freq_counter(void);

// TODO: Implement frequency measuring
uint16_t measure_freq(void)
{
    uint16_t timer0_count = 0;
    overflowCount = 0;
    uint32_t  total_pulses = 0;
    uint16_t frequency = 0;

    // Timer1_B3 and Timer0_B3 setup

    // IDEX divide by 8
    TB1EX0 = TBIDEX_7;

    // TBCCR0 interrupt enabled and set interval to 1 sec
    TB0CCTL0 = CCIE;
    TB0CCR0 = 32768;

    //Reset the timer1 count register to 0.
    TB1R = 0;

    //Reset the timer0 count register to 0.
    TB0R = 0;

    // 16-bit, TBxCLK, halt mode, divide by 8, enable Interrupt
    TB1CTL = (CNTL_0 | TBSSEL_0 | MC__CONTINUOUS | ID_3 | TBIE);

    // 16-bit, halt mode
    TB0CTL = (CNTL_0 | TBSSEL_1 | MC__UP | ID_0);

    while(TB0CTL != MC_0);
    timer0_count = TB0R;
    total_pulses = ((uint32_t)overflowCount << 16) | TB1R;
    frequency = (total_pulses << 4) / 1000 ;
    return frequency;
}


// TODO: Function to initialize and configure timers for frequency counter
void initialize_freq_counter(void)
{
    // Pin P2.2 will be used as the external source. This corresponds with TB1CLK.
    // Configure P2.2 as GPIO input
    P2DIR &= ~BIT2;
    P2SEL0 = 1;
    P2SEL1 = 0;
}

/*
void Timer1_B3_Init(void)
{
   // Timer1_B3 setup

   // IDEX divide by 8
   TB1EX0 = TBIDEX_7;

   // 16-bit, TBxCLK, halt mode, divide by 8, enable Interrupt
   TB1CTL = (CNTL_0 | TBSSEL_0 | MC_0 | ID_3 | TBIE);

   //Reset the timer1 count register to 0.
   TB1R = 0;
}


void Timer0_B3_Init(void)
{
   // Timer0_B3 setup

   // TBCCR0 interrupt enabled and set interval to 1 sec
   TB0CCTL0 = CCIE;
   TB0CCR0 = 32768;

   // 16-bit, halt mode
   TB0CTL = (CNTL_0 | TBSSEL_1 | MC_0 | ID_0);

   //Reset the timer0 count register to 0.
   TB0R = 0;
}*/


#pragma vector=TIMER1_B1_VECTOR
__interrupt void Timer1_B3 (void)
{
    overflowCount++;
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void Timer0_B3 (void)
{
    TB0CTL = MC_0;
    TB1CTL = MC_0;
}
