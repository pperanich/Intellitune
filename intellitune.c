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


// Main Program func
int main(void) {

    volatile uint32_t i;
    uint16_t curr_freq=0;

    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    // Initialize the frequency counter
    initialize_freq_counter();

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();

    __bis_SR_register(GIE);       // Enable interrupts

    while(1)
    {
        curr_freq = measure_freq();
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
