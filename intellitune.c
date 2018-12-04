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
SearchParams search_parameters(uint8_t settings);
void clock_configure(void);
void init_gpio(void);

// Globals
char cap_val[8] = {'\0'};
char ind_val[6] = {'\0'};
char swr_val[5] = {'\0'};
char load_imp[8] = {'\0'};
uint8_t tune_task = 0;
uint8_t relay_setting = 0;
uint8_t cap_relay_1, cap_relay_2;
uint16_t ind_position_1, ind_position_2, cap_position_1, cap_position_2;


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
    static _iq16 temp, angular_frequency, gamma_1, gamma_2, vswr, numerator, denominator, div_res, vswr_high_load, vswr_low_load;
    static _iq16 Q_factor_1, Z_load_1, ind_react_1, cap_react_1, estimated_inductance_1, estimated_capacitance_1;
    static _iq16 Q_factor_2, Z_load_2, ind_react_2, cap_react_2, estimated_inductance_2, estimated_capacitance_2;
    static uint16_t ind_step;
    static _iq16 minimum_vswr, vswr_search;
    static uint16_t optimal_inductance, optimal_capacitance;
    static uint8_t fine_tune, optimal_relay, optimal_cap_position, ind_update;
    static SearchParams fine_tune_limits;
    static const _iq16 Z_source = _IQ16(50.0);
    static const _iq16 iq_one = _IQ16(1.0);
    static uint8_t task_status = 0;
    static char swrh[7] = {'\0'};
    static char swrl[7] = {'\0'};
    static char min_swr[7] = {'\0'};

    if(median_fwd_sample < 100) { return; }
    task_flag |= TUNE_BUSY;
    switch(tune_task)
    {
        case INITIALIZE_TUNE_COMPONENTS:
        {
            if(task_status == 0) {
                memset(&cap_val[0], 0, sizeof(cap_val));
                memset(&ind_val[0], 0, sizeof(ind_val));
                memset(&swr_val[0], 0, sizeof(swr_val));
                memset(&load_imp[0], 0, sizeof(load_imp));
                while(TB1CTL != MC_0) { asm("   NOP"); }
                measure_freq();
                while(TB1CTL != MC_0) { asm("   NOP"); }
                relay_setting = 0;
                switch_cap_relay(relay_setting);
                step_cap_motor(RETURN_START_MODE);
                step_ind_motor(RETURN_START_MODE);
                task_status++;
                break;
            } else if(task_status == 1) {
                if(!(task_flag & MOTOR_ACTIVE)) {
                    tune_task++;
                    task_status = 0;
                }
            }
            break;
        }

        case CALCULATE_SWR:
        {
            gamma_1 = calculate_ref_coeff(KNOWN_SWITCHED_OUT);
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
                TB0CCR1  = TB0R + 400;
                task_status++;
            }
            gamma_2 = calculate_ref_coeff(KNOWN_SWITCHED_IN);
            if(gamma_2 != 0) { tune_task++; task_status = 0; }
            break;
        }

        case ESTIMATE_TUNE_VALUES:
        {
            char cap_val_1[7] = {'\0'};
            char ind_val_1[7] = {'\0'};
            char q_val_1[6] = {'\0'};
            char str_cap_react_1[6] = {'\0'};
            char str_ind_react_1[6] = {'\0'};
            char load_imp_1[8] = {'\0'};

            char cap_val_2[7] = {'\0'};
            char ind_val_2[7] = {'\0'};
            char q_val_2[6] = {'\0'};
            char str_cap_react_2[6] = {'\0'};
            char str_ind_react_2[6] = {'\0'};
            char load_imp_2[8] = {'\0'};

            char ref_coeff[7] = {'\0'};
            char ref_w_25[7] = {'\0'};
            uint8_t error = 0;

            _iq16 iq_freq = _IQ16div(_IQ16(frequency), _IQ16(1000));
            angular_frequency = _IQ16mpy(_IQ16(2*PI), iq_freq); // in Mega rad/s
            numerator = iq_one + gamma_1;
            denominator = iq_one - gamma_1;
            vswr = _IQ16div(numerator, denominator);
            if(!(vswr < _IQ16(0.0))) { error += _IQ16toa(swr_val,"%2.1f", vswr); }
            div_res = vswr;

            Z_load_1 = _IQ16mpy(Z_source, div_res);
            temp = _IQ16div(Z_load_1, Z_source);
            temp = temp - iq_one;
            Q_factor_1 = _IQ16sqrt(temp);
            ind_react_1 = _IQ16mpy(Q_factor_1, Z_source);
            cap_react_1 = _IQ16div(Z_load_1, Q_factor_1);
            estimated_capacitance_1 = _IQ16div(iq_one, cap_react_1);
            estimated_capacitance_1 = _IQ16mpy(estimated_capacitance_1, _IQ16(10000));
            estimated_capacitance_1 = _IQ16div(estimated_capacitance_1, angular_frequency);
            estimated_capacitance_1 = _IQ16mpy(estimated_capacitance_1, _IQ16(100)); // in pF
            estimated_inductance_1 = _IQ16div(ind_react_1, angular_frequency); // in uH

            if((estimated_capacitance_1 < _IQ16(0.0)) || (estimated_inductance_1 < _IQ16(0.0))) {
                tune_task = CALCULATE_SWR;
            } else if((estimated_capacitance_1 > _IQ16(CAP_MAX + ADDITIONAL_CAP)) || (estimated_inductance_1 > _IQ16(IND_MAX))) {
                tune_task = CALCULATE_SWR;
            } else if(Z_load_1 < _IQ16(0.0)) {
                tune_task = CALCULATE_SWR;
            } else {
                error += _IQ16toa(q_val_1, "%2.2f", Q_factor_1);
                error += _IQ16toa(load_imp_1, "%4.2f", Z_load_1);
                error += _IQ16toa(str_cap_react_1, "%4.2f", cap_react_1);
                error += _IQ16toa(str_ind_react_1, "%4.2f", ind_react_1);
                error += _IQ16toa(cap_val_1, "%4.2f", estimated_capacitance_1);
                error += _IQ16toa(ind_val_1, "%2.2f", estimated_inductance_1);
            }

            div_res = _IQ16div(iq_one, div_res);
            Z_load_2 = _IQ16mpy(Z_source, div_res);
            temp = _IQ16div(Z_source, Z_load_2);
            temp = temp - iq_one;
            Q_factor_2 = _IQ16sqrt(temp);
            ind_react_2 = _IQ16mpy(Q_factor_2, Z_load_2);
            cap_react_2 = _IQ16div(Z_source, Q_factor_2);
            estimated_capacitance_2 = _IQ16div(iq_one, cap_react_2);
            estimated_capacitance_2 = _IQ16mpy(estimated_capacitance_2, _IQ16(10000));
            estimated_capacitance_2 = _IQ16div(estimated_capacitance_2, angular_frequency);
            estimated_capacitance_2 = _IQ16mpy(estimated_capacitance_2, _IQ16(100)); // in pF
            estimated_inductance_2 = _IQ16div(ind_react_2, angular_frequency); // in uH

            if((estimated_capacitance_2 < _IQ16(0.0)) || (estimated_inductance_2 < _IQ16(0.0))) {
                tune_task = CALCULATE_SWR;
            } else if((estimated_capacitance_2 > _IQ16(CAP_MAX + ADDITIONAL_CAP)) || (estimated_inductance_2 > _IQ16(IND_MAX))) {
                tune_task = CALCULATE_SWR;
            } else if(Z_load_2 < _IQ16(0.0)) {
                tune_task = CALCULATE_SWR;
            } else {
                error += _IQ16toa(q_val_2, "%2.2f", Q_factor_2);
                error += _IQ16toa(load_imp_2, "%4.2f", Z_load_2);
                error += _IQ16toa(str_cap_react_2, "%4.2f", cap_react_2);
                error += _IQ16toa(str_ind_react_2, "%4.2f", ind_react_2);
                error += _IQ16toa(cap_val_2, "%4.2f", estimated_capacitance_2);
                error += _IQ16toa(ind_val_2, "%2.2f", estimated_inductance_2);
            }

            error += _IQ16toa(ref_coeff,"%1.4f", gamma_1);
            error += _IQ16toa(ref_w_25,"%1.4f", gamma_2);

            if(error == 0) {
                tune_task++;
                task_status = 0;
            }
            break;
        }

        case ADJUST_TO_ESTIMATE_1:
        {
            if(task_status == 0) {
                _iq16 varicap_value, temp, iq_position;

                P3OUT &= ~BIT4; // Capacitors switched to output side.
                temp = estimated_capacitance_1 - _IQ16(30.0);
                temp = _IQ16div(temp, _IQ16(470));
                relay_setting = (uint8_t)_IQ16int(temp);
                cap_relay_1 = relay_setting;
                varicap_value = _IQ16frac(temp);
                varicap_value = _IQ16mpy(varicap_value, _IQ16(470));
                varicap_value = varicap_value + _IQ16(30.0);
                iq_position = _IQ16div((_IQ16(C_UPPER_LIMIT) - _IQ16(C_LOWER_LIMIT)), _IQ16(CAP_MAX));
                iq_position = _IQ16rmpy(iq_position, varicap_value);
                cap_position_1 = (uint16_t)_IQ16int(iq_position);

                iq_position = _IQ16div((_IQ16(L_UPPER_LIMIT) - _IQ16(L_LOWER_LIMIT)), _IQ16(IND_MAX));
                iq_position = _IQ16rmpy(iq_position, estimated_inductance_1);
                ind_position_1 = (uint16_t)_IQ16int(iq_position);

                step_cap_motor((uint32_t)((cap_position_1 << 3) | CMD_POS_MODE));
                step_ind_motor((uint32_t)((ind_position_1 << 3) | CMD_POS_MODE));

                switch_cap_relay(relay_setting);
                task_status++;
                break;

            } else if(task_status == 1) {
                if(!(task_flag & MOTOR_ACTIVE)) {
                    task_status++;
                    adc_flg &= ~SWR_SENSE;
                    break;
                }
            } else if(task_status == 2) {
                _iq16 ref_coeff = calculate_ref_coeff(KNOWN_SWITCHED_OUT);
                if(ref_coeff != 0) {
                    numerator = iq_one + ref_coeff;
                    denominator = iq_one - ref_coeff;
                    vswr_high_load = _IQ16div(numerator, denominator);
                    _IQ16toa(swrh, "%2.3f", vswr_high_load);
                    tune_task++;
                    task_status = 0;
                }
            }
            break;
        }

        case ADJUST_TO_ESTIMATE_2:
        {
            if(task_status == 0) {
                _iq16 varicap_value, temp, iq_position;

                P3OUT |= BIT4; // Capacitors switched to input side.
                temp = estimated_capacitance_2 - _IQ16(30.0);
                temp = _IQ16div(temp, _IQ16(470));
                relay_setting = (uint8_t)_IQ16int(temp);
                cap_relay_2 = relay_setting;
                varicap_value = _IQ16frac(temp);
                varicap_value = _IQ16mpy(varicap_value, _IQ16(470));
                varicap_value = varicap_value + _IQ16(30.0);
                iq_position = _IQ16div((_IQ16(C_UPPER_LIMIT) - _IQ16(C_LOWER_LIMIT)), _IQ16(CAP_MAX));
                iq_position = _IQ16rmpy(iq_position, varicap_value);
                cap_position_2 = (uint16_t)_IQ16int(iq_position);

                iq_position = _IQ16div((_IQ16(L_UPPER_LIMIT) - _IQ16(L_LOWER_LIMIT)), _IQ16(IND_MAX));
                iq_position = _IQ16rmpy(iq_position, estimated_inductance_2);
                ind_position_2 = (uint16_t)_IQ16int(iq_position);

                step_cap_motor((uint32_t)((cap_position_2 << 3) | CMD_POS_MODE));
                step_ind_motor((uint32_t)((ind_position_2 << 3) | CMD_POS_MODE));

                switch_cap_relay(relay_setting);
                task_status++;
                break;

            } else if(task_status == 1) {
                if(!(task_flag & MOTOR_ACTIVE)) {
                    task_status++;
                    adc_flg &= ~SWR_SENSE;
                    break;
                }
            } else if(task_status == 2) {
                _iq16 ref_coeff = calculate_ref_coeff(KNOWN_SWITCHED_OUT);
                if(ref_coeff != 0) {
                    numerator = iq_one + ref_coeff;
                    denominator = iq_one - ref_coeff;
                    vswr_low_load = _IQ16div(numerator, denominator);
                    _IQ16toa(swrl, "%2.3f", vswr_low_load);
                    tune_task++;
                    task_status = 0;
                }
            }
            break;
        }

        case SELECT_NETWORK_CONFIG:
        {
            uint8_t error = 0;
            if(task_status == 0) {
                if((vswr_low_load > _IQ16(32.0)) && (vswr_high_load > _IQ16(32.0))) {
                    // Neither estimate was close to network value. Perform search over all values
                    P3OUT &= ~BIT4; // Capacitors switched to output side.
                    step_cap_motor(RETURN_START_MODE);
                    step_ind_motor(RETURN_START_MODE);
                    task_status++;
                    // Neither estimate good, search all applicable values
                    fine_tune_limits = search_parameters(BIT0);
                    if(fine_tune_limits.lower_inductance > ind_position_1) { fine_tune_limits.lower_inductance = ind_position_1; }
                    if(fine_tune_limits.upper_inductance < ind_position_1) { fine_tune_limits.upper_inductance = ind_position_1; }
                    fine_tune = 1;
                } else if(vswr_high_load < vswr_low_load) {
                    // perform search algorithm around estimate 1.
                    P3OUT &= ~BIT4; // Capacitors switched to output side.
                    error += _IQ16toa(load_imp, "%4.2f", Z_load_1);
                    error += _IQ16toa(cap_val, "%4.2f", estimated_capacitance_1);
                    error += _IQ16toa(ind_val, "%2.2f", estimated_inductance_1);
                    task_status++;
                    // Estimate 1 produced the lowest SWR, optimize around those component values
                    fine_tune_limits = search_parameters(BIT0 | BIT1);
                    if(fine_tune_limits.lower_inductance > ind_position_1) { fine_tune_limits.lower_inductance = ind_position_1; }
                    if(fine_tune_limits.upper_inductance < ind_position_1) { fine_tune_limits.upper_inductance = ind_position_1; }
                    step_ind_motor((uint32_t)((fine_tune_limits.lower_inductance << 3) | CMD_POS_MODE));
                    fine_tune = 2;
                } else {
                    // perform search algorithm around estimate 2.
                    P3OUT |= BIT4; // Capacitors switched to input side.
                    error += _IQ16toa(load_imp, "%4.2f", Z_load_2);
                    error += _IQ16toa(cap_val, "%4.2f", estimated_capacitance_2);
                    error += _IQ16toa(ind_val, "%2.2f", estimated_inductance_2);
                    task_status++;
                    // Estimate 2 produced the lowest SWR, optimize around those component values
                    fine_tune_limits = search_parameters(BIT1);
                    if(fine_tune_limits.lower_inductance > ind_position_2) { fine_tune_limits.lower_inductance = ind_position_2; }
                    if(fine_tune_limits.upper_inductance < ind_position_2) { fine_tune_limits.upper_inductance = ind_position_2; }
                    step_ind_motor((uint32_t)((fine_tune_limits.lower_inductance << 3) | CMD_POS_MODE));
                    fine_tune = 3;
                }
            } else if(task_status == 1) {
                if(!(task_flag & MOTOR_ACTIVE)) {
                    tune_task++;
                    task_status = 0;
                    minimum_vswr = _IQ16(99.0);
                    ind_step = (fine_tune_limits.upper_inductance - fine_tune_limits.lower_inductance) / 8;
                    if(ind_step == 0) { ind_step = 1; }
                    ind_update = 0;
                }
            }
            break;
        }

        case FINE_TUNE:
        {
            uint8_t error = 0;
            _iq16 ref_coeff = calculate_ref_coeff(KNOWN_SWITCHED_OUT);
            if(ref_coeff != 0) {
                numerator = iq_one + ref_coeff;
                denominator = iq_one - ref_coeff;
                vswr_search = _IQ16div(numerator, denominator);
                if(vswr_search < minimum_vswr) {
                    optimal_inductance = ind_sample;
                    optimal_capacitance = cap_sample;
                    optimal_relay = relay_setting;
                    optimal_cap_position = fine_tune_limits.capacitor_location;
                    minimum_vswr = vswr_search;
                    _IQ16toa(min_swr, "%2.3f", minimum_vswr);
                }
            }
            if(task_status == 0) {
                relay_setting = fine_tune_limits.lower_relay;
                switch_cap_relay(relay_setting);
                step_cap_motor((uint32_t)((fine_tune_limits.lower_capacitance << 3) | CMD_POS_MODE));
                task_status++;
                break;
            } else if(task_status == 1) {
                if(!(task_flag & MOTOR_ACTIVE)) {
                    if(relay_setting == fine_tune_limits.upper_relay) {
                        if(cap_sample <= fine_tune_limits.lower_capacitance)
                        { step_cap_motor((uint32_t)((fine_tune_limits.upper_capacitance << 3) | CMD_POS_MODE)); break; }
                        if(cap_sample >= fine_tune_limits.upper_capacitance) {
                            relay_setting = fine_tune_limits.lower_relay;
                            switch_cap_relay(relay_setting);
                            step_cap_motor(RETURN_START_MODE);
                            task_status++;
                            break;
                        }
                    } else if(relay_setting < fine_tune_limits.upper_relay) {
                        if(cap_sample >= C_UPPER_LIMIT) {
                            switch_cap_relay(relay_setting++);
                            step_cap_motor(RETURN_START_MODE);
                        } else {
                            step_cap_motor((uint32_t)((C_UPPER_LIMIT << 3) | CMD_POS_MODE));
                        }
                    }
                    break;
                }
            } else if(task_status == 2) {
                if(ind_sample >= fine_tune_limits.upper_inductance) {
                    task_status++;
                } else if(!(task_flag & IND_ACTIVE) && !(task_flag & CAP_ACTIVE ) && (ind_update != 1)) {
                    uint16_t ind_pos = ind_sample + ind_step;
                    if(ind_pos > L_UPPER_LIMIT) { ind_pos = L_UPPER_LIMIT; }
                    step_ind_motor((uint32_t)((ind_pos << 3) | CMD_POS_MODE));
                    ind_update = 1;
                    break;
                } else if(!(task_flag & MOTOR_ACTIVE )) {
                    task_status = 0;
                    ind_update = 0;
                }
                break;
            } else if(task_status == 3) {
                // Either iterate process again or change values to the minimums found
                if(task_flag & MOTOR_ACTIVE) { break; }
                if(fine_tune == 1) {
                    // Try optimization for capacitor on input side
                    fine_tune_limits = search_parameters(0);
                    P3OUT |= BIT4; // Capacitors switched to input side.
                    step_ind_motor((uint32_t)((fine_tune_limits.lower_inductance << 3) | CMD_POS_MODE));
                    fine_tune = 0;
                    task_status = 0;
                } else if((fine_tune == 2) && (minimum_vswr > _IQ16(2.5))) {
                    fine_tune_limits = search_parameters(BIT1);
                    if(fine_tune_limits.lower_inductance > ind_position_2) { fine_tune_limits.lower_inductance = ind_position_2; }
                    if(fine_tune_limits.upper_inductance < ind_position_2) { fine_tune_limits.upper_inductance = ind_position_2; }
                    P3OUT |= BIT4; // Capacitors switched to input side.
                    error += _IQ16toa(load_imp, "%4.2f", Z_load_2);
                    error += _IQ16toa(cap_val, "%4.2f", estimated_capacitance_2);
                    error += _IQ16toa(ind_val, "%2.2f", estimated_inductance_2);
                    step_ind_motor((uint32_t)((fine_tune_limits.lower_inductance << 3) | CMD_POS_MODE));
                    fine_tune = 0;
                    task_status = 0;
                } else if((fine_tune == 3) && (minimum_vswr > _IQ(2.5))) {
                    fine_tune_limits = search_parameters(BIT0 | BIT1);
                    if(fine_tune_limits.lower_inductance > ind_position_1) { fine_tune_limits.lower_inductance = ind_position_1; }
                    if(fine_tune_limits.upper_inductance < ind_position_1) { fine_tune_limits.upper_inductance = ind_position_1; }
                    P3OUT &= ~BIT4; // Capacitors switched to input side.
                    error += _IQ16toa(load_imp, "%4.2f", Z_load_1);
                    error += _IQ16toa(cap_val, "%4.2f", estimated_capacitance_1);
                    error += _IQ16toa(ind_val, "%2.2f", estimated_inductance_1);
                    step_ind_motor((uint32_t)((fine_tune_limits.lower_inductance << 3) | CMD_POS_MODE));
                    fine_tune = 0;
                    task_status = 0;
                } else {
                    step_cap_motor((uint32_t)((optimal_capacitance << 3) | CMD_POS_MODE));
                    step_ind_motor((uint32_t)((optimal_inductance << 3) | CMD_POS_MODE));
                    relay_setting = optimal_relay;
                    switch_cap_relay(relay_setting);
                    if(optimal_cap_position == 1) { P3OUT &= ~BIT4; } // Capacitors switched to output side.
                    else if(optimal_cap_position == 2) { P3OUT |= BIT4; } // Capacitors switched to input side.
                    task_status++;
                }
            } else if(task_status == 4) {
                if(!(task_flag & MOTOR_ACTIVE)) {
                    button_press &= ~TUNE & ~MODE_LOCK;
                    task_status = 0;
                    tune_task = INITIALIZE_TUNE_COMPONENTS;
                }
            }
            break;
        }
    }
    task_flag &= ~TUNE_BUSY;
}


// TODO: Search algorithm for load greater than 50 ohms
SearchParams search_parameters(uint8_t settings) {
    uint16_t max_ind_pos, max_cap_pos, lower_ind_search, upper_ind_search,
             lower_cap_search, upper_cap_search, ind_pos, cap_pos;
    uint8_t max_relay_setting, lower_relay_setting, upper_relay_setting, cap_location;
    _iq16 varicap_value, temp, iq_position;
    _iq16 iq_freq = _IQ16(frequency);
    uint8_t iterations, cap_relay;

    if(settings & BIT0) {   // Estimates for load > characteristic impedance
        cap_location = 1; // Cap bank on the output side of the inductor
        ind_pos = ind_position_1;
        cap_pos = cap_position_1;
        cap_relay = cap_relay_1;
        _iq16 max_ind = _IQ16div(_IQ16(4430.68263), iq_freq);
        max_ind = _IQ16mpy(max_ind, _IQ16(10.0));
        if(_IQ16(IND_MAX) < max_ind) { max_ind = _IQ16(IND_MAX); }
        iq_position = _IQ16div((_IQ16(L_UPPER_LIMIT) - _IQ16(L_LOWER_LIMIT)), _IQ16(IND_MAX));
        iq_position = _IQ16rmpy(iq_position, max_ind);
        max_ind_pos = (uint16_t)_IQ16int(iq_position);

        _iq16 max_cap = _IQ16div(_IQ16(17722.7305), iq_freq);
        max_cap = _IQ16mpy(max_cap, _IQ16(1000.0));
        if(_IQ16(CAP_MAX + ADDITIONAL_CAP) < max_cap) { max_cap = _IQ16(CAP_MAX + ADDITIONAL_CAP); }
        temp = max_cap - _IQ16(30.0);
        temp = _IQ16div(temp, _IQ16(470));
        max_relay_setting = (uint8_t)_IQ16int(temp);
        varicap_value = _IQ16frac(temp);
        varicap_value = _IQ16mpy(varicap_value, _IQ16(470));
        varicap_value = varicap_value + _IQ16(30.0);
        iq_position = _IQ16div((_IQ16(C_UPPER_LIMIT) - _IQ16(C_LOWER_LIMIT)), _IQ16(CAP_MAX));
        iq_position = _IQ16rmpy(iq_position, varicap_value);
        max_cap_pos = (uint16_t)_IQ16int(iq_position);
    } else {                // Estimates for load < characteristic impedance
        cap_location = 2; // Cap bank on the input side of the inductor
        ind_pos = ind_position_2;
        cap_pos = cap_position_2;
        cap_relay = cap_relay_2;
        _iq16 max_ind = _IQ16div(_IQ16(3398.594655), iq_freq);
        if(_IQ16(IND_MAX) < max_ind) { max_ind = _IQ16(IND_MAX); }
        iq_position = _IQ16div((_IQ16(L_UPPER_LIMIT) - _IQ16(L_LOWER_LIMIT)), _IQ16(IND_MAX));
        iq_position = _IQ16rmpy(iq_position, max_ind);
        max_ind_pos = (uint16_t)_IQ16int(iq_position);

        _iq16 max_cap = _IQ16div(_IQ16(5664.3244), iq_freq);
        max_cap = _IQ16mpy(max_cap, _IQ16(1000.0));
        if(_IQ16(CAP_MAX + ADDITIONAL_CAP) < max_cap) { max_cap = _IQ16(CAP_MAX + ADDITIONAL_CAP); }
        temp = max_cap - _IQ16(30.0);
        temp = _IQ16div(temp, _IQ16(470));
        max_relay_setting = (uint8_t)_IQ16int(temp);
        varicap_value = _IQ16frac(temp);
        varicap_value = _IQ16mpy(varicap_value, _IQ16(470));
        varicap_value = varicap_value + _IQ16(30.0);
        iq_position = _IQ16div((_IQ16(C_UPPER_LIMIT) - _IQ16(C_LOWER_LIMIT)), _IQ16(CAP_MAX));
        iq_position = _IQ16rmpy(iq_position, varicap_value);
        max_cap_pos = (uint16_t)_IQ16int(iq_position);
    }

    if(settings & BIT1) {
        iterations = 1;
        lower_ind_search = ind_pos - 512;
        if((lower_ind_search < L_LOWER_LIMIT) || (lower_ind_search > ind_pos)) { lower_ind_search = L_LOWER_LIMIT; }
        upper_ind_search = ind_pos + 1024;
        if(upper_ind_search > max_ind_pos) { upper_ind_search = max_ind_pos; }
        lower_cap_search = cap_pos - 3072;
        if((lower_cap_search < C_LOWER_LIMIT) || (lower_cap_search > cap_pos)) {
            if(cap_relay > 0) {
                lower_cap_search = C_UPPER_LIMIT - 3072;
                lower_relay_setting = cap_relay - 1;
            } else {
                lower_cap_search = C_LOWER_LIMIT;
            }
        }
        upper_cap_search = cap_pos + 3072;
        if(upper_cap_search > C_UPPER_LIMIT) {
            if(cap_relay < max_relay_setting) {
                upper_cap_search = C_LOWER_LIMIT + (upper_cap_search - C_UPPER_LIMIT);
                upper_relay_setting = cap_relay + 1;
            } else {
                upper_cap_search = C_UPPER_LIMIT;
            }
        }
    } else {
        iterations = 3;
        lower_ind_search = L_LOWER_LIMIT;
        upper_ind_search = max_ind_pos;
        lower_cap_search = C_LOWER_LIMIT;
        lower_relay_setting = 0;
        upper_cap_search = max_cap_pos;
        upper_relay_setting = max_relay_setting;
    }
    // Now, all upper and lower values for the search have been established. Proceed to run
    // optimization function to find optimal configuration in range.
    if(lower_ind_search < L_LOWER_LIMIT) { lower_ind_search = L_LOWER_LIMIT; }
    if(upper_ind_search > L_UPPER_LIMIT) { upper_ind_search = L_UPPER_LIMIT; }
    if(lower_cap_search < C_LOWER_LIMIT) { lower_cap_search = C_LOWER_LIMIT; }
    if(upper_cap_search > C_UPPER_LIMIT) { upper_cap_search = C_UPPER_LIMIT; }

    SearchParams search_settings;
    search_settings.lower_inductance = lower_ind_search;
    search_settings.upper_inductance = upper_ind_search;
    search_settings.lower_capacitance = lower_cap_search;
    search_settings.upper_capacitance = upper_cap_search;
    search_settings.lower_relay = lower_relay_setting;
    search_settings.upper_relay = upper_relay_setting;
    search_settings.capacitor_location = cap_location;
    search_settings.num_itr = iterations;
    return search_settings;
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


// TODO: Initialize GPIO outputs to reduce power consumption
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
