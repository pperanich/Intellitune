/*
 * File: user_iface.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will include user interface functions.
 *
 ******************************************************************************/

#include "intellitune.h"


// Globals
uint8_t MODE_SWITCH = 0;
uint16_t BUTTON_PRESS = 0;

// Function Prototypes
void ui_button_init(void);
void lcd_update(void);


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
    TB3CCR1  = 48; // Set CCR1 value for 1.46 ms interrupt (48 / 32768 Hz) = 0.00146
    TB3CCTL1 = CCIE; // Compare interrupt enable
    TB3CTL   = (CNTL_0 | TBSSEL_1 | MC__CONTINUOUS | TBIE); // ACLK as clock source, continuous mode, timer clear

    hd44780_clear_screen(); // Clear display content
}


// TODO: LCD update function
void lcd_update(void)
{
    // Not Implemented
    asm("    NOP");
}


// Directive for timer interrupt
#pragma vector = TIMER3_B1_VECTOR
__interrupt void Timer3_B1( void )
{
  switch( TB3IV ) // Determine interrupt source
  {
    case TBIV_2: // CCR1 caused the interrupt
      TB3CCR1 += 48; // Add CCR1 value for next interrupt in 1 ms
      hd44780_timer_isr(); // Call HD44780 state machine
      break; // CCR1 interrupt handling done

    case TBIV_4:
        MODE_SWITCH += 1;
        TB3CCTL2 = CCIE_0; // Compare interrupt disable
        break;

    case TBIV_14:
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
    case P2IV_2:
        BUTTON_PRESS |= BIT0;
        break;

    case P2IV_4:
        BUTTON_PRESS |= BIT1;
        break;

    case P2IV_12:
        BUTTON_PRESS |= BIT5;
        break;
  }
}


// Directive for port 3 interrupt
#pragma vector = PORT3_VECTOR
__interrupt void Port_3( void )
{
  switch( P3IV )
  {
    case P3IV_2:
        if(P2IN & BIT0)
        {
            P2IES |= BIT0;
            BUTTON_PRESS &= ~(BIT0 << 8);
            TB3CCTL2 = CCIE_0; // Compare interrupt enable
        }
        else {
            P2IES &= ~BIT0;
            BUTTON_PRESS |= (BIT0 << 8);
            TB3CCR2  = TB3R + 65530; // Set CCR2 value for 2 s interrupt
            TB3CCTL2 = CCIE; // Compare interrupt enable
        }
        break;

    case P3IV_4:
        BUTTON_PRESS |= (BIT1 << 8);
        break;

    case P3IV_12:
        BUTTON_PRESS |= (BIT5 << 8);
        break;
  }
}


// Directive for port 4 interrupt
#pragma vector = PORT4_VECTOR
__interrupt void Port_4( void )
{
  switch( P4IV )
  {
    case P3IV_2:
        BUTTON_PRESS |= (BIT0 << 12);
        break;
  }
}
