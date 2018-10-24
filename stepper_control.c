/*
 * File: stepper_control.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will implement the functions to interface with the
 *              stepper motors. This will include determining the position of the
 *              motors and controlling them through the stepper motor drivers.
 *
 ******************************************************************************/

#include "intellitune.h"


// Function Prototypes
void initialize_stepper_control(void);
void step_motor(uint8_t motor, uint8_t dir, uint8_t amount);
void current_setting(void);


// TODO: Initialize stepper control
void initialize_stepper_control(void)
{
    /*
     * Stepper Driver Connections
     *
     * Connections to cap motor driver:
     *  P5.4 - Enable
     *  P1.1 - Step
     *  P1.4 - Direction
     *  P4.7 - Micro-step 1
     *
     * Connections to ind motor driver:
     *  P3.2 - Enable
     *  P3.3 - Step
     *  P2.4 - Direction
     */

    // Configure pins connected to stepper driver as output
    P1DIR |= BIT1 | BIT4;
    P2DIR |= BIT4;
    P3DIR |= BIT2 | BIT3;
    P4DIR |= BIT7;
    P5DIR |= BIT4;

    // Enable pin logic high disables FETs on driver.
    P3OUT |= BIT2;
    P5OUT |= BIT4;

    // Pull MS1 pin for capacitor microstep low
    P4OUT &= ~BIT7;

    // Initialize step pins low
    P1OUT &= ~BIT1;
    P3OUT &= ~BIT3;

    // Initialize direction pins low (Forward)
    P1OUT &= ~BIT4;
    P2OUT &= ~BIT4;
}


// TODO: Implement current settings function.
void current_setting(void)
{
    // Not Implemented
    asm("    NOP");
}


// TODO: Implement stepper motor control function.
void step_motor(uint8_t motor, uint8_t dir, uint8_t amount)
{
    // Not Implemented
    asm("    NOP");
}
