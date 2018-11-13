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

    switch(tune_task)
    {
        case INITIALIZE_TUNE_COMPONENTS:
        {
            uint16_t cap_motor_command = 1441;
            uint16_t ind_motor_command = 44642;
            step_cap_motor(cap_motor_command);
            step_ind_motor(ind_motor_command);
            break;
        }

        case CALCULATE_SWR:
        {
            gamma_1 = calculate_ref_coeff(KNOWN_SWITCHED_OUT);//_IQ16(0.818182);
            if(gamma_1 == 0) { return; }
            else { tune_task++; }
            break;
        }

        case CALCULATE_SWR_W_KNOWN_IMP:
        {
            P3OUT |= BIT6;
            adc_flg |= IMP_SWITCH;
            gamma_2 = calculate_ref_coeff(KNOWN_SWITCHED_IN);//_IQ16(0.826);
            if(gamma_2 == 0) { return; }
            else { tune_task++; }
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
            error += _IQ16toa(swr_val,"%2.1f", vswr);
            div_res = vswr;
            if(gamma_2 > gamma_1)
            {
                Z_load = _IQ16mpy(Z_source, div_res);
                error += _IQ16toa(load_imp, "%4.4f", Z_load);
                temp = _IQ16div(Z_load, Z_source);
                temp = temp - iq_one;
                Q_factor = _IQ16sqrt(temp);
                error += _IQ16toa(q_val, "%2.2f", Q_factor);
                ind_react = _IQ16mpy(Q_factor, Z_source);
                error += _IQ16toa(ind_val, "%4.2f", ind_react);
                cap_react = _IQ16div(Z_load, Q_factor);
                error += _IQ16toa(cap_val, "%4.2f", cap_react);
                P3OUT &= ~BIT4; // Capacitors switched to output side.
            } else if(gamma_2 < gamma_1)
            {
                div_res = _IQ16div(iq_one, div_res);
                Z_load = _IQ16mpy(Z_source, div_res);
                error += _IQ16toa(load_imp, "%3.2f", Z_load);
                temp = _IQ16div(Z_source, Z_load);
                temp = temp - iq_one;
                Q_factor = _IQ16sqrt(temp);
                error += _IQ16toa(q_val, "%2.2f", Q_factor);
                ind_react = _IQ16mpy(Q_factor, Z_load);
                error += _IQ16toa(ind_val, "%4.2f", ind_react);
                cap_react = _IQ16div(Z_source, Q_factor);
                error += _IQ16toa(cap_val, "%4.2f", cap_react);
                P3OUT |= BIT4; // Capacitors switched to input side.
            } else { return; }


            estimated_capacitance = _IQ16div(iq_one, cap_react);
            estimated_capacitance = _IQ16mpy(estimated_capacitance, _IQ16(10000));
            estimated_capacitance = _IQ16div(estimated_capacitance, angular_frequency);
            estimated_capacitance = _IQ16mpy(estimated_capacitance, _IQ16(100)); // in pF
            error += _IQ16toa(cap2_val, "%4.2f", estimated_capacitance);

            estimated_inductance = _IQ16div(ind_react, angular_frequency); // in uH
            error += _IQ16toa(ind2_val, "%2.2f", estimated_inductance);

            if(error == 0) { tune_task++; }
            break;
        }

        case ADJUST_TO_ESTIMATES:
        {
            tune_task++;
            break;
        }

        case FINE_TUNE:
        {
            tune_task = 0;
            button_press &= ~TUNE;
            break;
        }
    }
}


// TODO: Change clock speed to 24MHz.
void clock_configure(void)
{
    // Configure MCLK for 16MHz. FLL reference clock is XT1. At this
    // speed, the FRAM requires wait states. ACLK = XT1 ~32768Hz, SMCLK = MCLK = 16MHz.

    // Configure one FRAM waitstate as required by the device datasheet for MCLK
    // operation beyond 8MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_1;

    P2SEL1 |= BIT6 | BIT7;                       // set XT1 pin as second function
    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);           // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);                   // Test oscillator fault flag

    __bis_SR_register(SCG0);                     // disable FLL
    CSCTL3 |= SELREF__XT1CLK;                    // Set XT1 as FLL reference source
    CSCTL0 = 0;                                  // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                      // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_5;                         // Set DCO = 16MHz
    CSCTL2 = FLLD_0 + 487;                       // DCOCLKDIV = 16MHz
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
