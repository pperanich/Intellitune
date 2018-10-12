/*
 * File: standing_wave_sensor.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will define the functions necessary to sample the FWD
 *              and REF voltages derived from the SWR sensor board. With these measurements,
 *              the SWR will be calculated and returned to the main loop. In addition,
 *              another function will regulate the digipot on the SWR board to protect
 *              the launchpad from voltage inputs above its rating.
 *
 ******************************************************************************/

#include "intellitune.h"


// Function Prototypes
SWR measure_swr(void);
void update_digipot(void);


// TODO: Implement SWR measurement function
SWR measure_swr(void)
{
    // Not Implemented
    asm("    NOP");
    SWR temp;
    temp.fwd = 0;
    temp.ref = 0;
    return temp;
}


// TODO: Change digipot for safe ADC voltages
void update_digipot(void)
{
    // Not Implemented
    asm("    NOP");
}
