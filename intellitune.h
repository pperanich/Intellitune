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

#include <string.h>
#include <IQmathLib.h>
#include "hd44780.h"



#define PI      3.1415926536
// Macros for ADC flags
#define FWD_SENSE   BIT0
#define REF_SENSE   BIT1
#define FWD_WITH_25 BIT2
#define REF_WITH_25 BIT3
#define CAP_POT     BIT4
#define IND_POT     BIT5
#define ADC_STATUS  BIT6
// Macros for task flags
#define A_TASK      BIT0
#define B_TASK      BIT1
#define C_TASK      BIT2


// Globals
extern uint16_t frequency;
extern uint8_t CMD_BYTE;
extern uint8_t DATA_BYTE;
extern const uint32_t DELAY_CYC_NUM;
extern _iq16 cap_sample, ind_sample;
extern unsigned int adc_result;
extern uint8_t display_menu;
extern char cap2_val[8];
extern char ind2_val[6];
extern char swr_val[5];
extern char load_imp[7];
extern uint8_t task_flag;

// Subsystem function declarations

// Stepper motor subsystem
extern void initialize_stepper_control(void);
extern void step_motor(uint8_t motor, uint8_t dir, uint16_t degrees);
extern void current_setting(void);

// Frequency Counter subsystem
extern void measure_freq(void);
extern void initialize_freq_counter(void);

// Standing Wave Ratio subsystem
extern _iq16 measure_ref_coeff(void);
extern void initialize_spi(void);
extern void initialize_adc(void);
extern void update_digipot(void);

// User Interface subsystem
extern void ui_init(void);
extern void lcd_update(void);
extern void utoa(unsigned int n, char s[]);
extern void reverse(char s[]);
extern void mode_0(void);
extern void mode_1(void);
extern void mode_2(void);
extern void mode_3(void);
extern void mode_4(void);

// Relay subsystem
extern void initialize_relay(void);
extern void switch_relay(int relay_num, int position);
extern void switch_net_config(void);
extern void switch_known_impedance(void);

// State Machine function prototypes
//------------------------------------
extern void initialize_task_manager(void);
// Alpha states
extern void A0(void);  //state A0
extern void B0(void);  //state B0
extern void C0(void);  //state C0

// A branch states
extern void A1(void);  //state A1
extern void A2(void);  //state A2
extern void A3(void);  //state A3

// B branch states
extern void B1(void);  //state B1
extern void B2(void);  //state B2
extern void B3(void);  //state B3

// C branch states
extern void C1(void);  //state C1
extern void C2(void);  //state C2
extern void C3(void);  //state C3
