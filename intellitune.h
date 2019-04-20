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
#include "IQmathLib.h"
#include "hd44780.h"


// Macro definitions to improve readability of code
#define PI                          3.1415926536
// Macros for ADC flags
#define IMP_SWITCH                  BIT0
#define SWR_SENSE                   BIT1
#define SWR_KNOWN_SENSE             BIT2
#define CAP_POT                     BIT4
#define IND_POT                     BIT5
#define ADC_STATUS                  BIT6
#define FWD_PIN                     ADCINCH_10
#define REF_PIN                     ADCINCH_11
#define IND_PIN                     ADCINCH_9
#define CAP_PIN                     ADCINCH_8
// Macros for task flags
#define A_TASK                      BIT0
#define B_TASK                      BIT1
#define C_TASK                      BIT2
#define MOTOR_ACTIVE                BIT3
#define REVERT_TO_BTN_MODE          BIT4
#define CAP_ACTIVE                  BIT5
#define IND_ACTIVE                  BIT6
#define TUNE_BUSY                   BIT7
// Macros for Stepper Motors
#define CAPACITOR_MOTOR             1
#define INDUCTOR_MOTOR              0
#define INCREASE_IND_DIR            1
#define DECREASE_IND_DIR            0
#define INCREASE_CAP_DIR            1
#define DECREASE_CAP_DIR            0
#define STEP_DUTY_CYCLE             12000
#define SET_ENABLE_AND_DIRECTION    0
#define STEP_HIGH                   1
#define STEP_LOW                    2
#define DISABLE_DRIVER              3
#define L_LOWER_LIMIT               0003
#define L_UPPER_LIMIT               6180
#define C_LOWER_LIMIT               0001
#define C_UPPER_LIMIT               5374
#define BTN_CONTROL_MODE            2
#define RETURN_START_MODE           4
#define CMD_POS_MODE                6
#define FINE_TUNE_MODE              10
#define Lup_CMD                     (BIT1 | INCREASE_IND_DIR)
#define Ldn_CMD                     (BIT1 | DECREASE_IND_DIR)
#define Cup_CMD                     (BIT1 | INCREASE_CAP_DIR)
#define Cdn_CMD                     (BIT1 | DECREASE_CAP_DIR)
// Macros for SWR sense
#define KNOWN_SWITCHED_OUT          0
#define KNOWN_SWITCHED_IN           1
// Macros for tune task algorithm
#define INITIALIZE_TUNE_COMPONENTS  0
#define CALCULATE_SWR               1
#define CALCULATE_SWR_W_KNOWN_IMP   2
#define ESTIMATE_TUNE_VALUES        3
#define ADJUST_TO_ESTIMATE_1        4
#define ADJUST_TO_ESTIMATE_2        5
#define SELECT_NETWORK_CONFIG       6
#define FINE_TUNE                   7
// Macros for the push button flag
#define TUNE                        BIT0
#define MODE                        BIT1
#define ANT                         BIT2
#define Lup                         BIT3
#define Cup                         BIT4
#define Ldn                         BIT5
#define Cdn                         BIT6
#define MODE_LOCK                   BIT7
// Macros for UI menus
#define SETTING_MENU_OFFSET         20
#define NUM_QUICK_MENUS             3
#define NUM_SETUP_MENUS             3
#define DEFAULT_DISPLAY             1
#define TUNING_DISPLAY              2
#define COMPONENT_VALUES            3
#define TARGET_SWR                  21
#define AUTOTUNE_THRESH             22
#define LC_DISPLAY                  23
#define DEFAULT_QUICK_MENU          DEFAULT_DISPLAY
#define DEFAULT_SETTING_MENU        TARGET_SWR
// Macros for other
#define CAP_MAX                     500.00 // in pF
#define CAP_MIN                     30.0   // in pF
#define ADDITIONAL_CAP              3290.0 // in pF
#define IND_MAX                     24.6    // in uH


typedef struct {
    uint16_t lower_inductance;
    uint16_t upper_inductance;
    uint16_t lower_capacitance;
    uint16_t upper_capacitance;
    uint8_t lower_relay;
    uint8_t upper_relay;
    uint8_t capacitor_location;
    uint8_t num_itr;
} SearchParams;


// Globals
extern uint32_t  total_pulses;
extern uint16_t frequency, overflowCount, inductor_position, capacitor_position;
extern uint16_t cap_sample, ind_sample;
extern uint8_t adc_channel_select, adc_flg, task_flag,
               display_menu, cap_motor_task, ind_motor_task,
               tune_task, button_press, relay_setting, target_btn;
extern uint16_t fwd_25_sample[16];
extern uint16_t ref_25_sample[16];
extern uint16_t fwd_sample[16];
extern uint16_t ref_sample[16];
extern uint16_t median_fwd_sample;
extern uint16_t median_ref_sample;
extern uint16_t median_ref_sample_25;
extern uint16_t median_fwd_sample_25;
extern char cap_val[8];
extern char ind_val[6];
extern char swr_val[5];
extern char load_imp[8];
extern uint8_t adc_index_25;
extern uint8_t adc_index;
extern uint16_t latest_fwd;
extern uint16_t latest_ref;


// Subsystem function declarations
extern void tune(void);

// Stepper motor subsystem
extern void initialize_stepper_control(void);
extern void step_cap_motor(uint32_t command);
extern void step_ind_motor(uint32_t command);
extern void current_setting(void);

// Frequency Counter subsystem
extern void measure_freq(void);
extern void initialize_freq_counter(void);

// Standing Wave Ratio subsystem
extern _iq16 calculate_ref_coeff(uint8_t reflection_to_calc);
extern void initialize_spi(void);
extern void initialize_adc(void);
extern void update_digipot(void);
extern void update_swr(void);

// User Interface subsystem
extern void ui_init(void);
extern void lcd_update(void);
extern void utoa(unsigned int n, char s[]);
extern void reverse(char s[]);

// Relay subsystem
extern void initialize_relay(void);
extern void switch_cap_relay(uint8_t setting);

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
