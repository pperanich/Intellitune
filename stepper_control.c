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
void step_cap_motor(uint32_t command);
void step_ind_motor(uint32_t command);


// Globals
uint8_t cap_motor_task = 0;
uint8_t ind_motor_task = 0;


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

    // Pull MS1 pin for capacitor microstep high
    P4OUT |= BIT7;

    // Initialize step pins low
    P1OUT &= ~BIT1;
    P1OUT &= ~BIT3;

    // Enable pin logic low enables FETs on driver.
    P3OUT &= ~BIT2;
    P5OUT &= ~BIT4;

    // Pulse DIR pin to initialize direction
    P1OUT |= BIT4;
    P2OUT |= BIT4;

    __delay_cycles(12000);

    P1OUT &= ~BIT4;
    P2OUT &= ~BIT4;

    // Enable pin logic high disables FETs on driver.
    P3OUT |= BIT2;
    P5OUT |= BIT4;

    TB2R = 0;
    TB2CTL   = (CNTL_0 | TBSSEL_1 | MC__CONTINUOUS); // ACLK as clock source, continuous mode
}


// TODO: Implement capacitor stepper motor control function.
inline void step_cap_motor(uint32_t command)
{
    /*
     * Vari-Capacitor has 5373 unique values, calculated by
     *      180(deg) / 0.067(deg) * 2(MS1 halves step angle)
     *
     * Input: command
     *      This can be used to select different modes of stepper operation.
     *      By this, I mean that one command may tell the component to go to its
     *      minimum setting while another command may specify a certain position.
     *      Further, a command should be implemented that simply increments component
     *      until flag is erased.
     *
     */

    static uint16_t position;
    static uint8_t direction, mode;
    uint8_t step_status;
    if(command != 0)
    {
        mode = command & 0x000E;
        switch(mode)
        {
        case BTN_CONTROL_MODE:
            direction = command & BIT0;
            break;
        case RETURN_START_MODE:
            direction = DECREASE_CAP_DIR;
            break;
        case CMD_POS_MODE:
            position = (command >> 4);
            if(position < cap_sample) { direction = DECREASE_CAP_DIR; }
            else { direction = INCREASE_CAP_DIR; }
            break;
        default:
            return;
        }
    }

    if(((cap_sample < C_LOWER_LIMIT) && (direction == DECREASE_CAP_DIR)) ||
       ((cap_sample > C_UPPER_LIMIT) && (direction == INCREASE_CAP_DIR)))
    {
        if((mode == BTN_CONTROL_MODE) && (button_press & Cup)){
            if(relay_setting < 7) {
                switch_cap_relay(++relay_setting);
                direction = DECREASE_CAP_DIR;
                mode = CMD_POS_MODE;
                position = C_LOWER_LIMIT;
                cap_motor_task = SET_ENABLE_AND_DIRECTION;
                task_flag |= REVERT_TO_BTN_MODE;
            } else { cap_motor_task = DISABLE_DRIVER; }
        } else if((mode == BTN_CONTROL_MODE) && (button_press & Cdn)) {
            if(relay_setting > 0) {
                switch_cap_relay(--relay_setting);
                direction = INCREASE_CAP_DIR;
                mode = CMD_POS_MODE;
                position = C_UPPER_LIMIT;
                cap_motor_task = SET_ENABLE_AND_DIRECTION;
                task_flag |= REVERT_TO_BTN_MODE;
            } else { cap_motor_task = DISABLE_DRIVER; }
        } else {
            cap_motor_task = DISABLE_DRIVER;
        }
    }

    switch(cap_motor_task)
    {
        case SET_ENABLE_AND_DIRECTION:
        {
            task_flag |= MOTOR_ACTIVE | CAP_ACTIVE;
            P5OUT &= ~BIT4; // Enable FETs on driver
            if(direction == INCREASE_CAP_DIR) { P1OUT &= ~BIT4; }
            else if(direction == DECREASE_CAP_DIR) { P1OUT |= BIT4; }
            TB2CCR1  = TB2R + 16;
            cap_motor_task = STEP_HIGH;
            TB2CCTL1 = CCIE;
            break;
        }
        case STEP_HIGH:
        {
            switch(mode)
            {
                case BTN_CONTROL_MODE:
                {
                    if((direction == INCREASE_CAP_DIR) && (button_press & Cup)) { step_status = 1; }
                    else if((direction == DECREASE_CAP_DIR) && (button_press & Cdn)) { step_status = 1; }
                    else { step_status = 0; }
                    break;
                }
                case RETURN_START_MODE:
                {
                    if(cap_sample > C_LOWER_LIMIT) { step_status = 1; }
                    else { step_status = 0; }
                    break;
                }
                case CMD_POS_MODE:
                {
                    if((position > cap_sample) && (direction == INCREASE_CAP_DIR)) { step_status = 1; }
                    else if((position < cap_sample) && (direction == DECREASE_CAP_DIR)) { step_status = 1; }
                    else {
                        if(task_flag & REVERT_TO_BTN_MODE) {
                            task_flag &= ~ REVERT_TO_BTN_MODE;
                            mode = BTN_CONTROL_MODE;
                            if(button_press & Cup) { direction = INCREASE_CAP_DIR; }
                            else if(button_press & Cdn) { direction = DECREASE_CAP_DIR; }
                            cap_motor_task = SET_ENABLE_AND_DIRECTION;
                            TB2CCR2  = TB2R + 2;
                            return;
                        }
                        step_status = 0;
                    }
                    break;
                }
                default:
                    return;
            }
            if(step_status == 0) { cap_motor_task = DISABLE_DRIVER; TB2CCR1  = TB2R + 18; }
            else if(step_status == 1)
            {
                if(direction == INCREASE_CAP_DIR) { cap_sample++; }
                else if(direction == DECREASE_CAP_DIR) { cap_sample--; }
                P1OUT |= BIT1;
                TB2CCR1  = TB2R + 18;
                cap_motor_task = STEP_LOW;
            }
            break;
        }
        case STEP_LOW:
        {
            P1OUT &= ~BIT1;
            TB2CCR1  = TB2R + 18;
            cap_motor_task = STEP_HIGH;
            break;
        }
        case DISABLE_DRIVER:
        {
            P1OUT &= ~BIT1; // Make sure step pin is low
            P5OUT |= BIT4; // Disable FETs on driver
            cap_motor_task = SET_ENABLE_AND_DIRECTION;
            TB2CCTL1 = CCIE_0;
            if(task_flag & IND_ACTIVE) { task_flag &= ~CAP_ACTIVE; }
            else { task_flag &= ~MOTOR_ACTIVE & ~CAP_ACTIVE; }
            break;
        }
    }
}


// TODO: Implement inductor stepper motor control function.
inline void step_ind_motor(uint32_t command)
{
    /*
     * Roller Inductor has 6200 unique values, calculated by
     *      31(rotations) * 360(deg) / 1.8(deg step angle)
     *
     * Input: command
     *      This can be used to select different modes of stepper operation.
     *      By this, I mean that one command may tell the component to go to its
     *      minimum setting while another command may specify a certain position.
     *      Further, a command should be implemented that simply increments component
     *      until flag is erased.
     *
     */

    static uint16_t position;
    static uint8_t direction, mode;
    uint8_t step_status;
    if(command != 0)
    {
        mode = command & 0x0E;
        switch(mode)
        {
        case BTN_CONTROL_MODE:
            direction = command & BIT0;
            break;
        case RETURN_START_MODE:
            direction = DECREASE_IND_DIR;
            break;
        case CMD_POS_MODE:
            position = (command >> 4);
            if(position < ind_sample) { direction = DECREASE_IND_DIR; }
            else { direction = INCREASE_IND_DIR; }
            break;
        default:
            return;
        }
    }

    if(((ind_sample < L_LOWER_LIMIT) && (direction == DECREASE_IND_DIR)) ||
       ((ind_sample > L_UPPER_LIMIT) && (direction == INCREASE_IND_DIR)))
    {
        ind_motor_task = DISABLE_DRIVER;
    }

    switch(ind_motor_task)
    {
        case SET_ENABLE_AND_DIRECTION:
        {
            task_flag |= MOTOR_ACTIVE | IND_ACTIVE;
            P3OUT &= ~BIT2; // Enable FETs on driver
            if(direction) { P2OUT |= BIT4; }
            else { P2OUT &= ~BIT4; }
            TB2CCR2  = TB2R + 16;
            ind_motor_task = STEP_HIGH;
            TB2CCTL2 = CCIE;
            break;
        }
        case STEP_HIGH:
        {
            switch(mode)
            {
                case BTN_CONTROL_MODE:
                {
                    if((direction == INCREASE_IND_DIR) && (button_press & Lup)) { step_status = 1; }
                    else if((direction == DECREASE_IND_DIR) && (button_press & Ldn)) { step_status = 1; }
                    else { step_status = 0; }
                    break;
                }
                case RETURN_START_MODE:
                {
                    if(ind_sample > L_LOWER_LIMIT) { step_status = 1; }
                    else { step_status = 0; }
                    break;
                }
                case CMD_POS_MODE:
                {
                    if((position < ind_sample) && (direction == DECREASE_IND_DIR)) { step_status = 1; }
                    else if((position > ind_sample) && (direction == INCREASE_IND_DIR)) { step_status = 1; }
                    else { step_status = 0; }
                    break;
                }
                default:
                    return;
            }
            if(step_status == 0) { ind_motor_task = DISABLE_DRIVER; TB2CCR2  = TB2R + 24; }
            else if(step_status == 1)
            {
                if(direction == INCREASE_IND_DIR) { ind_sample++; }
                else if(direction == DECREASE_IND_DIR) { ind_sample--; }
                P1OUT |= BIT3;
                TB2CCR2  = TB2R + 24;
                ind_motor_task = STEP_LOW;
            }
            break;
        }
        case STEP_LOW:
        {
            P1OUT &= ~BIT3;
            TB2CCR2  = TB2R + 24;
            ind_motor_task = STEP_HIGH;
            update_swr();
            break;
        }
        case DISABLE_DRIVER:
        {
            P1OUT &= ~BIT3; // Make sure step pin is low
            P3OUT |= BIT2; // Disable FETs on driver
            ind_motor_task = SET_ENABLE_AND_DIRECTION;
            TB2CCTL2 = CCIE_0;
            if(task_flag & CAP_ACTIVE) { task_flag &= ~IND_ACTIVE; }
            else { task_flag &= ~MOTOR_ACTIVE & ~IND_ACTIVE; }
            break;
        }
    }
}


#pragma vector=TIMER2_B1_VECTOR
__interrupt void Timer2_B1(void)
{
    switch( TB2IV ) // Determine interrupt source
    {
        case TBIV_2: // CCR1 caused the interrupt - used for capacitor stepper motor delay
        {
          step_cap_motor(0);
          break;
        }

        case TBIV_4: // CCR2 caused the interrupt - used for inductor stepper motor delay
        {
          step_ind_motor(0);
          break;
        }
    }
}
