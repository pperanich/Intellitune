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
void switch_cap_relay(uint8_t setting);
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
void switch_cap_relay(uint8_t setting)
{
    switch(setting)
    {
    case 0:
        P1OUT &= ~BIT5 & ~BIT6 & ~BIT7;
        break;
    case 1:
        P1OUT |= BIT5; P1OUT &= ~BIT6 & ~BIT7;
        break;
    case 2:
        P1OUT |= BIT6; P1OUT &= ~BIT5 & ~BIT7;
        break;
    case 3:
        P1OUT |= BIT5 | BIT6; P1OUT &= ~BIT7;
        break;
    case 4:
        P1OUT |= BIT7; P1OUT &= ~BIT5 & ~BIT6;
        break;
    case 5:
        P1OUT |= BIT7 | BIT5; P1OUT &= ~BIT6;
        break;
    case 6:
        P1OUT |= BIT7 | BIT6; P1OUT &= ~BIT5;
        break;
    case 7:
        P1OUT |= BIT5 | BIT6 | BIT7;
        break;
    }
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
