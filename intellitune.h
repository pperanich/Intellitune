/*
 * File: intellitune.h
 *
 * Author(s): Preston Peranich, Noah Ihediwa
 *
 * Description: This header file will serve to identify all functions that are defined
 *              across the project and declare external globals as well. The different functions
 *              are separated according to their respective subsystem.
 *
 ******************************************************************************/

//#include "driverlib.h"
#include <stdio.h>
#include "hd44780.h"

// All custom typedefs used for this project
typedef struct
{
    uint32_t fwd;
    uint32_t ref;
} SWR;


// Globals
extern uint16_t frequency;
extern uint8_t CMD_BYTE;
extern uint8_t DATA_BYTE;
extern const uint32_t DELAY_CYC_NUM;

// Subsystem function declarations

// Stepper motor subsystem
extern void initialize_stepper_control(void);
extern void step_motor(uint8_t motor, uint8_t dir, uint16_t degrees);
extern void current_setting(void);

// Frequency Counter subsystem
extern void measure_freq(void);
extern void initialize_freq_counter(void);

// Standing Wave Ratio subsystem
extern SWR measure_swr(void);
extern void initialize_spi(void);
extern void update_digipot(void);

// User Interface subsystem
extern void ui_init(void);
extern void lcd_update(void);

// Relay subsystem
extern void initialize_relay(void);
extern void switch_relay(int relay_num, int position);
extern void switch_net_config(void);
extern void switch_known_impedance(void);
