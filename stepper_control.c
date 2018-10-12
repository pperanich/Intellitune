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
void step_motor(uint8_t motor, uint8_t dir, uint8_t amount);
void current_setting(void);


// TODO: Implement current settings function.
void current_setting(void)
{
    // Not Implemented
    asm("    NOP");
}


// TODO: Implment stepper motor control function.
void step_motor(uint8_t motor, uint8_t dir, uint8_t amount)
{
    // Not Implemented
    asm("    NOP");
}
