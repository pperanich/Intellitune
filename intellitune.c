/*
 * Intellitune_msp project
 *
 * File: intellitune.c
 *
 * Date: 10/10/2018
 *
 * Author(s): Preston Peranich
 *
 * Description: This project will be utilized to control an automatic antenna
 *              tuning unit for use in amateur radio applications.
 *
 * For more information on this project, visit the team's project page at:
 *          https://design.ece.msstate.edu/2018/team_peranich/index.html
 *
 * Several subsystems must be implemented for the product to operate as intended.
 *   The subsystems are as follows:
 *     * Frequency counter to track transmitted signal frequency
 *     * VSWR measurement from FWD and REF voltage input. In addition, control digiPot
 *         to ensure safe voltages for ADC
 *     * Stepper motor control via driver interfacing
 *     * Stepper motor location tracking. current plan is voltage divider network with
 *         potentiometer and gear connected to shaft of tuning components
 *     * Relay control to switch capacitor on either side of inductor
 *     * Add or remove additional capacitance to match impedance
 *     * Switch known impedance in / out, taking VSWR measurements for each
 *     * Tuning algorithm to meet required SWR
 *     * Drive 16x2 LED and update continuously
 *
 * When implementing, it would be best to create separate files for each subsystem
 * and implement all functions for a single subsystem into a single routine. For example,
 * when frequency counter called, also update digipots to keep voltages at safe levels for ADC.
 *
 ******************************************************************************/

#include "intellitune.h"

// Function Prototypes
void tune(void);
void clock_configure(void);


// Main Program function
int main(void) {

    volatile uint32_t i;

    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    clock_configure();

    // Initialize the frequency counter
    initialize_freq_counter();

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

    __bis_SR_register(GIE);       // Enable interrupts

    while(1)
    {
        measure_freq();
        while(TB0CTL != MC_0);
        // Delay
        for(i=50000; i>0; i--);
    }

}


// TODO: Implement tuning algorithm
void tune(void)
{
    // Not Implemented
    asm("    NOP");
}



void clock_configure(void)
{
    // Configure MCLK for 16MHz. FLL reference clock is XT1. At this
    // speed, the FRAM requires wait states. ACLK = XT1 ~32768Hz, SMCLK = MCLK = 16MHz.

    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    P2SEL1 |= BIT6 | BIT7;                       // set XT1 pin as second function
    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);           // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);                   // Test oscillator fault flag

    __bis_SR_register(SCG0);                     // disable FLL
    CSCTL3 |= SELREF__XT1CLK;                    // Set XT1 as FLL reference source
    CSCTL0 = 0;                                  // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                      // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_5;                         // Set DCO = 16MHz
    CSCTL2 = FLLD_0 + 487;                       // DCOCLKDIV = 16MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                     // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));   // FLL locked

    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;   // set XT1 (~32768Hz) as ACLK source, ACLK = 32768Hz
                                                 // default DCOCLKDIV as MCLK and SMCLK source
}
