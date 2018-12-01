/*
 * Intellitune project
 *
 * File: intellitune.c
 *
 * Date: 10/10/2018
 *
 * Author(s): Preston Peranich
 *
 * Description: This project will be utilized to control an automatic antenna
 *              tuning unit for use in amateur radio applications.
 *
 * For more information on this project, visit the team's project page at:
 *          https://design.ece.msstate.edu/2018/team_peranich/index.html
 *
 * Several subsystems must be implemented for the product to operate as intended.
 *   The subsystems are as follows:
 *     * Frequency counter to track transmitted signal frequency
 *     * VSWR measurement from FWD and REF voltage input. In addition, control digiPot
 *         to ensure safe voltages for ADC
 *     * Stepper motor control via driver interfacing
 *     * Stepper motor location tracking. current plan is voltage divider network with
 *         potentiometer and gear connected to shaft of tuning components
 *     * Relay control to switch capacitor on either side of inductor
 *     * Add or remove additional capacitance to match impedance
 *     * Switch known impedance in / out, taking VSWR measurements for each
 *     * Tuning algorithm to meet required SWR
 *     * Drive 16x2 LED and update continuously
 *
 * When implementing, it would be best to create separate files for each subsystem
 * and implement all functions for a single subsystem into a single routine. For example,
 * when frequency counter called, also update digipots to keep voltages at safe levels for ADC.
 *
 ******************************************************************************/

#include "intellitune.h"

// Function Prototypes
void tune(void);
void clock_configure(void);
void init_gpio(void);

char cap2_val[8] = {'\0'};
char ind2_val[6] = {'\0'};
char swr_val[5] = {'\0'};
char load_imp[7] = {'\0'};
uint8_t tune_task = 0;
uint8_t relay_setting = 0;


// This global will be used to notify user that a new adc value has been sampled
uint8_t adc_flg = 0;
// Broken down as follows:    BIT0    |    BIT1   |      BIT2       |  BIT3  |  BIT4  |  BIT5  |    BIT6    |  BIT7
//                         IMP_SWITCH   SWR_SENSE   SWR_KNOWN_SENSE   unused   CAP POT  IND POT  ADC Status   unused
//                         (w/o 25ohm res)   (with 25ohm res)

// Main Program function
int main(void) {
    volatile uint32_t i;

    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    clock_configure();

    // Initialize task manager
    initialize_task_manager();

    init_gpio();

    // Initialize relay pins and output states
    initialize_relay();

    // Initialize the frequency counter
    initialize_freq_counter();

    // Initialize SPI communication to digital potentiometer
    initialize_spi();

    // Initialize pins for stepper motor drivers
    initialize_stepper_control();

    // Initialize the user interface buttons and lcd
    ui_init();

    //Initialize ADC for position tracking and SWR sensing.
    initialize_adc();

    // Set output LED
    P1DIR |= BIT0;
    P1OUT |= BIT0;

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    __bis_SR_register(GIE);       // Enable interrupts

    while(1)
    {
        // State machine entry & exit point
        //===========================================================
        (*Alpha_State_Ptr)();   // jump to an Alpha state (A0,B0,...)
        //===========================================================
    }
}


// TODO: Implement tuning algorithm
void tune(void)
{
    static _iq16 Q_factor, temp, Z_load, ind_react, cap_react, estimated_inductance, estimated_capacitance,
                    angular_frequency, gamma_1, gamma_2, vswr, numerator, denominator, div_res;
    static const _iq16 Z_source = _IQ16(50.0);
    static const _iq16 iq_one = _IQ16(1.0);
    static uint8_t task_status = 0;
    task_flag |= TUNE_BUSY;
    switch(tune_task)
    {
        case INITIALIZE_TUNE_COMPONENTS:
        {
            if(task_status == 0) {
                while(TB1CTL != MC_0) { asm("   NOP"); }
                measure_freq();
                while(TB1CTL != MC_0) { asm("   NOP"); }
                relay_setting = 0;
                switch_cap_relay(relay_setting);
                step_cap_motor(RETURN_START_MODE);
                step_ind_motor(RETURN_START_MODE);
                task_status++;
            } else if(task_status == 1) {
                if(!(task_flag & IND_ACTIVE) && !(task_flag & CAP_ACTIVE)) {
                    tune_task++;
                    task_status = 0;
                }
            }
            break;
        }

        case CALCULATE_SWR:
        {
            gamma_1 = calculate_ref_coeff(KNOWN_SWITCHED_OUT);//_IQ16(0.818182);
            if(gamma_1 != 0) { tune_task++; }
            break;
        }

        case CALCULATE_SWR_W_KNOWN_IMP:
        {
            if(task_status == 0){
                adc_flg &= ~SWR_KNOWN_SENSE;
                P3OUT |= BIT6;
                adc_flg |= IMP_SWITCH;
                adc_channel_select = FWD_PIN;
                task_status++;
            }
            gamma_2 = calculate_ref_coeff(KNOWN_SWITCHED_IN);//_IQ16(0.826);
            if(gamma_2 != 0) { tune_task++; task_status = 0; }
            break;
        }

        case ESTIMATE_TUNE_VALUES:
        {

            char cap_val[7] = {'\0'};
            char ind_val[7] = {'\0'};
            char q_val[6] = {'\0'};
            memset(&cap2_val[0], 0, sizeof(cap2_val));
            memset(&ind2_val[0], 0, sizeof(ind2_val));
            memset(&swr_val[0], 0, sizeof(swr_val));
            memset(&load_imp[0], 0, sizeof(load_imp));
            uint8_t error = 0;

            _iq16 iq_freq = _IQ16div(_IQ16(frequency), _IQ16(1000));
            angular_frequency = _IQ16mpy(_IQ16(2*PI), iq_freq); // in Mega rad/s
            numerator = iq_one + gamma_1;
            denominator = iq_one - gamma_1;
            vswr = _IQ16div(numerator, denominator);
            div_res = vswr;
            if(gamma_2 > gamma_1)
            {
                Z_load = _IQ16mpy(Z_source, div_res);
                temp = _IQ16div(Z_load, Z_source);
                temp = temp - iq_one;
                Q_factor = _IQ16sqrt(temp);
                ind_react = _IQ16mpy(Q_factor, Z_source);
                cap_react = _IQ16div(Z_load, Q_factor);
                P3OUT &= ~BIT4; // Capacitors switched to output side.
            } else if(gamma_2 < gamma_1)
            {
                div_res = _IQ16div(iq_one, div_res);
                Z_load = _IQ16mpy(Z_source, div_res);
                temp = _IQ16div(Z_source, Z_load);
                temp = temp - iq_one;
                Q_factor = _IQ16sqrt(temp);
                ind_react = _IQ16mpy(Q_factor, Z_load);
                cap_react = _IQ16div(Z_source, Q_factor);
                P3OUT |= BIT4; // Capacitors switched to input side.
            } else { asm("   NOP"); }


            estimated_capacitance = _IQ16div(iq_one, cap_react);
            estimated_capacitance = _IQ16mpy(estimated_capacitance, _IQ16(10000));
            estimated_capacitance = _IQ16div(estimated_capacitance, angular_frequency);
            estimated_capacitance = _IQ16mpy(estimated_capacitance, _IQ16(100)); // in pF

            estimated_inductance = _IQ16div(ind_react, angular_frequency); // in uH

            if((estimated_capacitance < _IQ16(0.0)) || (estimated_inductance < _IQ16(0.0))) {
                tune_task = CALCULATE_SWR;
            } else if((estimated_capacitance > _IQ16(CAP_MAX)) || (estimated_inductance > _IQ16(IND_MAX))) {
                            tune_task = CALCULATE_SWR;
            } else if((vswr < _IQ16(0.0)) || (Z_load < _IQ16(0.0))) {
                tune_task = CALCULATE_SWR;
            } else {
                error += _IQ16toa(swr_val,"%2.1f", vswr);
                error += _IQ16toa(q_val, "%2.2f", Q_factor);
                error += _IQ16toa(load_imp, "%3.2f", Z_load);
                error += _IQ16toa(cap_val, "%4.2f", cap_react);
                error += _IQ16toa(ind_val, "%4.2f", ind_react);
                error += _IQ16toa(cap2_val, "%4.2f", estimated_capacitance);
                error += _IQ16toa(ind2_val, "%2.2f", estimated_inductance);
            }

            if(error == 0) {
                //tune_task++;
                task_status = 0;
                tune_task = INITIALIZE_TUNE_COMPONENTS;
                button_press &= ~TUNE & ~MODE_LOCK;
            }
            break;
        }

        case ADJUST_TO_ESTIMATES:
        {
            if(task_status == 0) {
                _iq16 varicap_value, temp, iq_position;
                uint16_t cap_position, ind_position;
                temp = estimated_capacitance - _IQ16(30.0);
                temp = _IQ16div(temp, _IQ16(470));
                relay_setting = (uint8_t)_IQ16int(temp);
                varicap_value = _IQ16frac(temp);
                varicap_value = _IQ16mpy(varicap_value, _IQ16(470));
                varicap_value = varicap_value + _IQ16(30.0);
                iq_position = _IQ16div((_IQ16(C_UPPER_LIMIT) - _IQ16(C_LOWER_LIMIT)), _IQ16(500));
                iq_position = _IQ16rmpy(iq_position, varicap_value);
                cap_position = (uint16_t)_IQ16int(iq_position);

                iq_position = _IQ16div((_IQ16(L_UPPER_LIMIT) - _IQ16(L_LOWER_LIMIT)), _IQ16(24));
                iq_position = _IQ16rmpy(iq_position, estimated_inductance);
                ind_position = (uint16_t)_IQ16int(iq_position);

                step_cap_motor((uint32_t)((cap_position << 4) | CMD_POS_MODE));
                step_ind_motor((uint32_t)((ind_position << 4) | CMD_POS_MODE));

                switch_cap_relay(relay_setting);

                task_status++;
            } else if(task_status == 1) {
                if(!(task_flag & MOTOR_ACTIVE)) {
                    tune_task++;
                    task_status = 0;
                }
            }
            break;
        }

        case FINE_TUNE:
        {
            if(task_status == 0) {
                step_cap_motor(FINE_TUNE_MODE);
                task_status = 3;
            } else if(task_status == 1) {
                step_ind_motor(FINE_TUNE_MODE);
                task_status = 4;
            } else if(task_status == 3) {
                if(!(task_flag & MOTOR_ACTIVE)) { task_status = 1; }
                return;
            } else if(task_status == 4) {
                if(!(task_flag & MOTOR_ACTIVE)) { task_status = 2; }
                return;
            } else if(task_status == 2) {
                task_status = 0;
                tune_task = INITIALIZE_TUNE_COMPONENTS;
                button_press &= ~TUNE & ~MODE_LOCK;
            }
            break;
        }
    }
    task_flag &= ~TUNE_BUSY;
}


// TODO: Change clock speed to 24MHz.
void clock_configure(void)
{
    // Configure two FRAM waitstate as required by the device datasheet for MCLK
        // operation at 24MHz(beyond 8MHz) _before_ configuring the clock system.
        FRCTL0 = FRCTLPW | NWAITS_2 ;

        P2SEL1 |= BIT6 | BIT7;                       // P2.6~P2.7: crystal pins
        do
        {
            CSCTL7 &= ~(XT1OFFG | DCOFFG);           // Clear XT1 and DCO fault flag
            SFRIFG1 &= ~OFIFG;
        } while (SFRIFG1 & OFIFG);                   // Test oscillator fault flag

        __bis_SR_register(SCG0);                     // disable FLL
        CSCTL3 |= SELREF__XT1CLK;                    // Set XT1 as FLL reference source
        CSCTL0 = 0;                                  // clear DCO and MOD registers
        CSCTL1 |= DCORSEL_7;                         // Set DCO = 24MHz
        CSCTL2 = FLLD_0 + 731;                       // DCOCLKDIV = 24MHz
        __delay_cycles(3);
        __bic_SR_register(SCG0);                     // enable FLL
        while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));   // FLL locked

        CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;   // set XT1 (~32768Hz) as ACLK source, ACLK = 32768Hz
                                                     // default DCOCLKDIV as MCLK and SMCLK source
        }

void init_gpio(void)
{
    // Initialize all pins to digital outputs to improve power consumption
    PADIR |= 0xFFFF;
    PBDIR |= 0xFFFF;
    PCDIR |= 0xFFFF;
    PDDIR |= 0xFFFF;
    PEDIR |= 0xFFFF;
    P6DIR |= 0xFF;
    P7DIR |= 0xFF;
    P8DIR |= 0xFF;
    P9DIR |= 0xFF;
    P10DIR |= 0xFF;

    PAOUT |= 0x00;
    PBOUT |= 0x00;
    PCOUT |= 0x00;
    PDOUT |= 0x00;
    PEOUT |= 0x00;
    P6OUT |= 0x00;
    P7OUT |= 0x00;
    P8OUT |= 0x00;
    P9OUT |= 0x00;
    P10OUT |= 0x00;
}
