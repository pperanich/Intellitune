/*
 * File: user_iface.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will include user interface functions.
 *
 ******************************************************************************/

#include "intellitune.h"

#define NUM_QUICK_MENUS 1
#define NUM_SETUP_MENUS 4

// Globals
uint8_t MODE_SWITCH = 0;
uint8_t PREV_MODE = 0;
uint8_t button_press = 0;
// Broken down as follows:    BIT0  |  BIT1  |  BIT2  |  BIT3  |  BIT4  |  BIT5  |    BIT6  |  BIT7
//                            TUNE     MODE     ANT     L-UP      C-UP     L-DN       C-DN    unused
//                         (w/o 25ohm res)   (with 25ohm res)
uint8_t display_menu= 0;

// Function Prototypes
void ui_button_init(void);
void lcd_update(void);
void utoa(unsigned int n, char s[]);
void reverse(char s[]);
void mode_0(void);
void mode_1(void);
void mode_2(void);
void mode_3(void);
void mode_4(void);


// Display variables and titles
static const char period[2] = {'.', '\0'};
static const char f_unit[4] = {'M', 'H', 'z', '\0'};
static const char name[13] = "Intellitune\0";
static const char cap_disp[3] = {'C', ':', '\0'};
static const char ind_disp[3] = {'L', ':', '\0'};
static const char cap_unit[4] = {'p', 'F', ' ', '\0'};
static const char ind_unit[4] = {'u', 'H', ' ', '\0'};
static const char target_mode_name[11] = "Target SWR\0";
static const char threshold_mode_name[14] = "SWR Threshold\0";
static const char lclimit_mode_name[9] = "LC Limit\0";




// TODO: User interface button configuration
void ui_init(void)
{
    // Configure selected button pins as inputs
    P2DIR &= ~BIT0 & ~BIT1 & ~BIT5;
    P3DIR &= ~BIT0 & ~BIT1 & ~BIT5;
    P4DIR &= ~BIT0;

    // Enable pullup or pulldown resistor on configured pins
    P2REN |= BIT0 | BIT1 | BIT5;
    P3REN |= BIT0 | BIT1 | BIT5;
    P4REN |= BIT0;
    // Setting PxOUT bit high configures pullup
    P2OUT |= BIT0 | BIT1 | BIT5;
    P3OUT |= BIT0 | BIT1 | BIT5;
    P4OUT |= BIT0;

    // Configure interrupts for button pins
    P2IES |= BIT0 | BIT1 | BIT5;
    P3IES |= BIT0 | BIT1 | BIT5;
    P4IES |= BIT0;

    P2IE |= BIT0 | BIT1 | BIT5;
    P3IE |= BIT0 | BIT1 | BIT5;
    P4IE |= BIT0;

    // Initialize pins for the LCD module
    P6DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4;
    P3DIR |= BIT7;

    TB3R = 0;
    TB3CCR1  = 64; // Set CCR1 value for 1.46 ms interrupt (48 / 32768 Hz) = 0.00146
    TB3CCTL1 = CCIE; // Compare interrupt enable
    TB3CTL   = (CNTL_0 | TBSSEL_1 | MC__CONTINUOUS | TBIE); // ACLK as clock source, continuous mode, timer clear

    hd44780_clear_screen(); // Clear display content
}


// TODO: LCD update function
void lcd_update(void)
{

    switch(display_mode)
    {
    case 0:
        mode_0();
        break;
    case 1:
        mode_1();
        break;
    case 2:
        mode_2();
        break;
    case 3:
        mode_3();
        break;
    case 4:
        mode_4();
        break;
    }

}

void mode_0(void)  // Default display w/ frequ.
{

    char buf2[4] = {'\0'};
    char buf3[3] = {'\0'};
    char row1[17] = {'\0'};
    char row2[17] = {'\0'};

    uint8_t str_length = 0;
    uint8_t freq_whole = frequency / 1000;
    uint16_t freq_decimal = frequency % 1000;
    utoa(freq_whole, row1);
    utoa(freq_decimal, buf2);

    char temp1, temp2;
    if(buf2[1] == '\0'){
        temp1 = buf2[0];
        buf2[0] = '0';
        buf2[1] = '0';
        buf2[2] = temp1;
    } else if(buf2[2] == '\0'){
        temp1 = buf2[0]; temp2 = buf2[1];
        buf2[0] = '0';
        buf2[1] = temp1;
        buf2[2] = temp2;
    }
    strcat(row1, period);
    strcat(row1, buf2);
    strcat(row1, f_unit);
    str_length = strlen(row1);
    // Write text string to first row and first column
    hd44780_write_string(row1, 1, 1, CR_LF );
    hd44780_blank_out_remaining_row(1,str_length+1);
    utoa(str_length, buf3);
    if(buf3[0]!='1') buf3[1] = ' ';
    // Write text string to second row and first column
    hd44780_write_string(buf3, 2, 14, CR_LF );
    strcat(row2, name);
    hd44780_write_string(row2, 2, 1, NO_CR_LF );
}

void mode_1(void)  // Tuning display with impedance network
{

    char row1[17] = {'\0'};
    char row2[17] = {'\0'};

    strcat(row1, cap_disp);
    strcat(row2, ind_disp);

    strcat(row1, cap2_val);
    strcat(row2, ind2_val);

    strcat(row1, cap_unit);
    strcat(row2, ind_unit);

    strcat(row1, swr_val);
    strcat(row2, load_imp);

    hd44780_write_string(row1, 1, 1, NO_CR_LF );
    hd44780_write_string(row2, 2, 1, NO_CR_LF );
}

void mode_2(void) // Target SWR mode
{

    char row1[17] = {'\0'};
    // char row2[17] = {'\0'};

    strcat(row1, target_mode_name);

    hd44780_write_string(row1, 1, 1, CR_LF );
    hd44780_blank_out_remaining_row(1,11);

    // Adjust target SWR from 1.5 to 2.0
    asm("   NOP");
}

void mode_3(void)  // AutoTune Threshold SWR mode
{
    char row1[17] = {'\0'};
    // char row2[17] = {'\0'};

    strcat(row1, threshold_mode_name);

    hd44780_write_string(row1, 1, 1, CR_LF );
    hd44780_blank_out_remaining_row(1, 14);
    hd44780_blank_out_remaining_row(2, 1);

    // Threshold of SWR and tuning will begin when it is surpassed
    asm("   NOP");
}

void mode_4(void) // LC Limit
{
    char row1[17] = {'\0'};
    // char row2[17] = {'\0'};

    strcat(row1, lclimit_mode_name);

    hd44780_write_string(row1, 1, 1, CR_LF );
    hd44780_blank_out_remaining_row(1, 9);
    hd44780_blank_out_remaining_row(2, 1);


    // Turns off limits for L and C -OR- Display max values instead
    asm("   NOP");
}

// utoa:  convert n to characters in s
void utoa(unsigned int n, char s[])
{
    uint8_t i = 0;
    do {  // generate digits in reverse order
        s[i++] = n % 10 + '0';  // get next digit
    } while ((n /= 10) > 0);  // delete it
    s[i] = '\0';
    reverse(s);
}


// reverse string s in place
void reverse(char s[])
{
    uint8_t i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}


// Directive for timer interrupt
#pragma vector = TIMER3_B1_VECTOR
__interrupt void Timer3_B1( void )
{
  switch( TB3IV ) // Determine interrupt source
  {
    case TBIV_2: // CCR1 caused the interrupt
      TB3CCR1 += 64; // Add CCR1 value for next interrupt in 1 ms
      task_flag |= A_TASK;
      break; // CCR1 interrupt handling done

    case TBIV_4: // CCR2 caused the interrupt
      TB3CCR2 += 328; // Add CCR2 value for next interrupt in 5 ms
      task_flag |= B_TASK;
      break; // CCR2 interrupt handling done

    case TBIV_6: // CCR3 caused the interrupt
      TB3CCR3 += 3277; // Add CCR3 value for next interrupt in 50 ms
      task_flag |= C_TASK;
      break; // CCR3 interrupt handling done
      
    case TBIV_8: // CCR4 caused the interrupt
      break;

    case TBIV_10: // CCR5 caused the interrupt
      TB1CTL = MC_0;
      total_pulses = ((uint32_t)overflowCount << 16) | TB1R;
      frequency = ((total_pulses << 4)) / 1000;
      TB3CCTL5 = CCIE_0; // Compare interrupt disable
      break;

    case TBIV_12: // CCR6 caused the interrupt
        MODE_SWITCH += 1;
        TB3CCTL6 = CCIE_0; // Compare interrupt disable
        break;

    case TBIV_14: // timer overflow caused the interrupt
        if(P1OUT & BIT0) P1OUT &= ~BIT0;
        else P1OUT |= BIT0;
        break;
  }
}


// Directive for port 2 interrupt
#pragma vector = PORT2_VECTOR
__interrupt void Port_2( void )
{
  switch( P2IV )
  {
    case P2IV_2: // Pin 2.0: C-Up btn
        if(P2IN & BIT0)
        {
            P2IES |= BIT0;
            button_press &= ~Cup;
        }
        else {
            P2IES &= ~BIT0;
            button_press |= Cup;
        }
        break;

    case P2IV_4: // Pin 2.1: Antenna btn
        button_press |= ANT;
        break;
      
    case P2IV_12: // Pin 2.5: Tune btn
        button_press |= TUNE;
        display_mode = 1;
        break;
  }
}


// Directive for port 3 interrupt
#pragma vector = PORT3_VECTOR
__interrupt void Port_3( void )
{
  switch( P3IV )
  {
    case P3IV_2: // Pin 3.0: Mode btn
        if(P3IN & BIT0)
        {
            P3IES |= BIT0;
            button_press &= ~MODE;
            TB3CCTL6 = CCIE_0; // Compare interrupt disable
            if (PREV_MODE == MODE_SWITCH){
                if(display_mode == NUM_QUICK_MENUS && !(MODE_SWITCH % 2)) display_mode = 0;
                else if(display_mode == NUM_SETUP_MENUS && (MODE_SWITCH % 2)) display_mode = 2;
                else {display_mode++;}
            }
            else{
                display_mode = 20;
            }
        }
        else {
            P3IES &= ~BIT0;
            button_press |= MODE;
            TB3CCR6  = TB3R + 65530; // Set CCR2 value for 2 s interrupt
            TB3CCTL6 = CCIE; // Compare interrupt enable
            PREV_MODE = MODE_SWITCH;
            if(display_mode == NUM_QUICK_MENUS) display_mode = 0;
            else display_mode++;
        }
        break;

    case P3IV_4: // Pin 3.1: L-Dn btn
        if(P3IN & BIT1)
        {
            P3IES |= BIT1;
            button_press &= ~Ldn;
        }
        else {
            P3IES &= ~BIT1;
            button_press |= Ldn;
        }
        break;

    case P3IV_12: // Pin 3.5: L-Up btn
        if(P3IN & BIT5)
        {
            P3IES |= BIT5;
            button_press &= ~Lup;
        }
        else {
            P3IES &= ~BIT5;
            button_press |= Lup;
        }
        break;
  }
}


// Directive for port 4 interrupt
#pragma vector = PORT4_VECTOR
__interrupt void Port_4( void )
{
  switch( P4IV )
  {
    case P4IV_2: // Pin 4.0: C-Dn btn
        if(P4IN & BIT0)
        {
            P4IES |= BIT0;
            button_press &= ~Cdn;
        }
        else {
            P4IES &= ~BIT0;
            button_press |= Cdn;
        }
        break;
  }
}
