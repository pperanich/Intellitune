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


// Macro definitions to improve readability of code
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
// Macros for Stepper Motors
#define CAPACITOR_MOTOR 1
#define INDUCTOR_MOTOR  0
#define STEP_DUTY_CYCLE 12000


// Globals
extern uint16_t frequency;
extern const uint8_t CMD_BYTE;
extern const uint8_t DATA_BYTE;
extern _iq16 cap_sample, ind_sample;
extern uint16_t adc_result;
extern uint8_t display_menu;
extern char cap2_val[8];
extern char ind2_val[6];
extern char swr_val[5];
extern char load_imp[7];
extern uint8_t task_flag;
extern uint8_t adc_ready;
extern uint16_t overflowCount;
extern uint32_t  total_pulses;


// Subsystem function declarations

// Stepper motor subsystem
extern void initialize_stepper_control(void);
void step_motor(uint16_t command);
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

// Relay subsystem
extern void initialize_relay(void);
extern void switch_relay(int relay_num, int position);
extern void switch_net_config(void);
extern void switch_known_impedance(void);

// State Machine function prototypes
//------------------------------------
extern void initialize_task_manager(void);

// Variable declarations for state machine
extern void (*Alpha_State_Ptr)(void);  // Base States pointer
extern void (*A_Task_Ptr)(void);       // State pointer A branch
extern void (*B_Task_Ptr)(void);       // State pointer B branch
extern void (*C_Task_Ptr)(void);       // State pointer C branch

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
