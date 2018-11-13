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
void step_motor(uint16_t command);


// Globals
uint8_t motor_task = 0;


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
     *  P1.3 - Step
     *  P2.4 - Direction
     */

    // Configure pins connected to stepper driver as output
    P1DIR |= BIT1 | BIT3 | BIT4;
    P2DIR |= BIT4;
    P3DIR |= BIT2;
    P4DIR |= BIT7;
    P5DIR |= BIT4;

    // Enable pin logic high disables FETs on driver.
    P3OUT |= BIT2;
    P5OUT |= BIT4;

    // Pull MS1 pin for capacitor microstep low
    P4OUT &= ~BIT7;

    // Initialize step pins low
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT3;

    // Initialize direction pins low (Forward)
    P1OUT &= ~BIT4;
    P2OUT &= ~BIT4;
}


// TODO: Implement stepper motor control function.
void step_motor(uint16_t command)
{
    /*
     * Inputs:
     *      command: composed of the following parameters
     *
     *      motor: <- Bit 0
     *          0 - inductor motor
     *          1 - capacitor motor
     *      direction: <- Bit 1
     *          1 - Forward
     *          0 - reverse
     *      degrees: <- Bits 2-16
     *          amount of degrees to rotate, shifted left 2 for more precision.
     */

    static uint32_t step_cycles;
    static uint16_t current_command;
    if(command != 0) { current_command = command; }

    switch(current_command & BIT0)
    {
        case INDUCTOR_MOTOR: // Inductor motor driver
        {
            switch(motor_task)
            {
            case SET_ENABLE_AND_DIRECTION:
                P3OUT &= ~BIT2; // Enable FETs on driver
                if(current_command & BIT1) { P2OUT &= ~BIT4; }
                else { P2OUT |= BIT4; }
                step_cycles = (uint32_t)(current_command >> 2) * 10 / 18;
                TB0CCR2  = TB0R + 8;
                motor_task = STEP_HIGH;
                TB0CCTL2 = CCIE;
                break;

            case STEP_HIGH:
                if(step_cycles)
                {
                    P1OUT |= BIT3;
                    TB0CCR2  = TB0R + 24;
                    motor_task = STEP_LOW;
                }
                else {
                    motor_task = DISABLE_DRIVER;
                }
                break;

            case STEP_LOW:
                P1OUT &= ~BIT3;
                TB0CCR2  = TB0R + 24;
                step_cycles--;
                motor_task = STEP_HIGH;
                break;

            case DISABLE_DRIVER:
                P3OUT |= BIT2; // Disable FETs on driver
                motor_task = 0;
                TB0CCTL2 = CCIE_0;
                break;
            }
            break;
        }

        case CAPACITOR_MOTOR: // Capacitor motor driver
        {
            switch(motor_task)
            {
            case SET_ENABLE_AND_DIRECTION:
                P5OUT &= ~BIT4; // Enable FETs on driver
                if(current_command & BIT1) { P1OUT &= ~BIT4; }
                else { P1OUT |= BIT4; }
                step_cycles = (uint32_t)(current_command >> 2) * 1000 / 67;
                TB0CCR2  = TB0R + 8;
                motor_task = STEP_HIGH;
                TB0CCTL2 = CCIE;
                break;

            case STEP_HIGH:
                if(step_cycles)
                {
                    P1OUT |= BIT1;
                    TB0CCR2  = TB0R + 24;
                    motor_task = STEP_LOW;
                }
                else {
                    motor_task = DISABLE_DRIVER;
                }
                break;

            case STEP_LOW:
                P1OUT &= ~BIT1;
                TB0CCR2  = TB0R + 24;
                step_cycles--;
                motor_task = STEP_HIGH;
                break;

            case DISABLE_DRIVER:
                P5OUT |= BIT4; // Disable FETs on driver
                motor_task = 0;
                TB0CCTL2 = CCIE_0;
                break;
            }
            break;
        }
    default:
        asm("    NOP");
        break;
    }
}
