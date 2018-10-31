/*
 * File: relay.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will contain the functions to control the relays in the Intellitune.
 *
 ******************************************************************************/

#include "intellitune.h"


// Function Prototypes
void initialize_relay(void);
void switch_cap_relay(int relay_num, int position);
void switch_net_config(void);
void switch_kwown_impedance(void);


// TODO: Initialize relay outputs
void initialize_relay(void)
{
    // Configure relay pins as outputs
    P1DIR |= BIT5 | BIT6 | BIT7;
    P3DIR |= BIT4 | BIT6;
    // Set pins low
    P1OUT &= ~BIT5 & ~BIT6 & ~BIT7;
    P3OUT &= ~BIT4 & ~BIT7;

    // Not Implemented
    asm("    NOP");
}


// TODO: Binary sequenced capacitor relay function.
void switch_cap_relay(int relay_num, int position)
{
    // Not Implemented
    asm("    NOP");
}


// TODO: Function to switch capacitor to either side of inductor.
void switch_net_config(void)
{
    // Not Implemented
    asm("    NOP");
}


// TODO Implement function to switch known impedance in and out.
void switch_known_impedance(void)
{
    // Not Implemented
    asm("    NOP");
}
