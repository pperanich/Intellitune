/*
 * File: user_iface.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will include user interface functions.
 *
 ******************************************************************************/

#include "intellitune.h"


// Function Prototypes
void lcd_init(void);
void lcd_update(void);


// TODO: LCD initialize function
void lcd_init(void)
{
    P6DIR |= 0x1F;
    P3DIR |= 0x80;

    TB3CCR1  = 48; // Set CCR1 value for 1 ms interrupt (1000 / 1 MHz) = 0.001
    TB3CCTL1 = CCIE; // Compare interrupt enable
    TB3CTL   = (TBSSEL_1 | MC_2 | TBCLR); // ACLK as clock source, continuous mode, timer clear

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
__interrupt void timer_3_b1_isr( void )
{
  switch( TB3IV ) // Determine interrupt source
  {
    case TBIV_2: // CCR1 caused the interrupt
    {
      TB3CCR1 += 48; // Add CCR1 value for next interrupt in 1 ms

      hd44780_timer_isr(); // Call HD44780 state machine

      break; // CCR1 interrupt handling done
    }
  }
}
