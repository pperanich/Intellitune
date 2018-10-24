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
void step_motor(uint8_t motor, uint8_t dir, uint16_t degrees);
void current_setting(void);


// Globals
const uint32_t DELAY_CYC_NUM = 12000;

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
void step_motor(uint8_t motor, uint8_t dir, uint16_t degrees)
{
    /*
     * Inputs:
     *      motor:
     *          0 - inductor motor
     *          1 - capacitor motor
     *      dir:
     *          0 - Forward
     *          1 - reverse
     *      degrees:
     *          amount of degrees to rotate, shifted left 2 for more precision.
     */

    uint32_t step_cycles;

    switch(motor)
    {
    case 0: // Inductor motor driver
        P3OUT &= ~BIT2; // Enable FETs on driver
        if(dir==0) { P2OUT &= ~BIT4; }
        else if(dir==1) { P2OUT |= BIT4; }
        step_cycles = (uint32_t)degrees * 10 / 18;
        __delay_cycles(40);
        while(step_cycles){
            P3OUT |= BIT3;
            __delay_cycles(200);
            P3OUT &= ~BIT3;
            __delay_cycles(200);
            step_cycles--;
        }
        P3OUT |= BIT2; // Disable FETs on driver
        break;

    case 1: // Capacitor motor driver
        P5OUT &= ~BIT4; // Enable FETs on driver
        if(dir==0) { P1OUT &= ~BIT4; }
        else if(dir==1) { P1OUT |= BIT4; }
        step_cycles = (uint32_t)degrees * 1000 / 67;
        __delay_cycles(4000);
        while(step_cycles){
            P1OUT |= BIT1;
            __delay_cycles(DELAY_CYC_NUM);
            P1OUT &= ~BIT1;
            __delay_cycles(DELAY_CYC_NUM);
            step_cycles--;
        }
        P5OUT |= BIT4; // Disable FETs on driver
        break;

    default:
        asm("    NOP");
        break;
    }
}
