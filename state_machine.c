/*
 * File: state_machine.c
 *
 * Author(s): Preston Peranich
 *
 * Description: This file will contain the state machine tasks
 *              and establish the different timer intervals for each subroutine.
 *              Timer 3 Counter/Compare 1 is used for A tasks.
 *              Timer 3 Counter/Compare 2 is used for B tasks.
 *              Timer 3 Counter/Compare 3 is used for C tasks.
 *
 ******************************************************************************/
#include "intellitune.h"


// Function Prototypes
void initialize_task_manager(void);
// Alpha states
void A0(void);  //state A0
void B0(void);  //state B0
void C0(void);  //state C0

// A branch states
void A1(void);  //state A1
void A2(void);  //state A2
void A3(void);  //state A3

// B branch states
void B1(void);  //state B1
void B2(void);  //state B2
void B3(void);  //state B3

// C branch states
void C1(void);  //state C1
void C2(void);  //state C2
void C3(void);  //state C3


// Variable declarations for state machine
void (*Alpha_State_Ptr)(void);  // Base States pointer
void (*A_Task_Ptr)(void);       // State pointer A branch
void (*B_Task_Ptr)(void);       // State pointer B branch
void (*C_Task_Ptr)(void);       // State pointer C branch

//Globals
uint8_t task_flag = 0;


// TODO: Create timer interrupt that will execute next task once previous task is completed.
void initialize_task_manager(void)
{
    TB3CCR1  = TB3R + 64; // Set CCR1 value for 1 ms interrupt
    TB3CCTL1 = CCIE; // Compare interrupt enable

    TB3CCR2  = TB3R + 328; // Set CCR2 value for 5 ms interrupt
    TB3CCTL2 = CCIE; // Compare interrupt enable

    TB3CCR3  = TB3R + 3277; // Set CCR3 value for 50 ms interrupt
    TB3CCTL3 = CCIE; // Compare interrupt enable

    // Tasks State-machine init
    Alpha_State_Ptr = &A0;
    A_Task_Ptr = &A1;
    B_Task_Ptr = &B1;
    C_Task_Ptr = &C1;
}


// TODO: Implement all state machine routines
//=================================================================================
//  STATE-MACHINE SEQUENCING AND SYNCRONIZATION FOR SLOW BACKGROUND TASKS
//=================================================================================

void A0(void)
{
    // loop rate synchronizer for A-tasks
    if(task_flag & A_TASK)
    {
        task_flag &= ~A_TASK; // Clear flag

        //-----------------------------------------------------------
        (*A_Task_Ptr)();        // jump to an A Task (A1,A2,A3,...)
        //-----------------------------------------------------------
    }
    Alpha_State_Ptr = &B0;      // Comment out to allow only A tasks
}

void B0(void)
{
    // loop rate synchronizer for B-tasks
    if(task_flag & B_TASK)
    {
        task_flag &= ~B_TASK; // Clear flag

        //-----------------------------------------------------------
        (*B_Task_Ptr)();        // jump to a B Task (B1,B2,B3,...)
        //-----------------------------------------------------------
    }

    Alpha_State_Ptr = &C0;      // Allow C state tasks
}

void C0(void)
{
    // loop rate synchronizer for C-tasks
    if(task_flag & C_TASK)
    {
        task_flag &= ~C_TASK; // Clear flag

        //-----------------------------------------------------------
        (*C_Task_Ptr)();        // jump to a C Task (C1,C2,C3,...)
        //-----------------------------------------------------------
    }

    Alpha_State_Ptr = &A0;  // Back to State A0
}


//=================================================================================
//  A - TASKS (executed in every 1 msec)
//=================================================================================
//--------------------------------------------------------
void A1(void)
//--------------------------------------------------------
{
    hd44780_timer_isr(); // Call HD44780 state machine
    //-------------------
    //the next time Timer3 counter 1 reaches Period value go to A2
    A_Task_Ptr = &A2;
    //-------------------
}

//--------------------------------------------------------
void A2(void)
//--------------------------------------------------------
{
    if(button_press & TUNE) { tune(); }
    //-------------------
    //the next time Timer3 counter 1 reaches Period value go to A1
    A_Task_Ptr = &A1;
    //-------------------
}

//=================================================================================
//  B - TASKS (executed in every 5 msec)
//=================================================================================

//----------------------------------- USER ----------------------------------------

//----------------------------------------
void B1(void)
//----------------------------------------
{
    update_digipot();
    //-----------------
    //the next time Timer3 counter 2 reaches period value go to B2
    B_Task_Ptr = &B2;
    //-----------------
}

//----------------------------------------
void B2(void) //  SPARE
//----------------------------------------
{

    //-----------------
    //the next time Timer3 counter 2 reaches period value go to B3
    B_Task_Ptr = &B1;
    //-----------------
}


//=================================================================================
//  C - TASKS (executed in every 50 msec)
//=================================================================================

//--------------------------------- USER ------------------------------------------

//----------------------------------------
void C1(void)
//----------------------------------------
{


    lcd_update();

    //-----------------
    //the next time Timer3 counter 3 reaches period value go to C2
    C_Task_Ptr = &C2;
    //-----------------

}

//----------------------------------------
void C2(void) //  SPARE
//----------------------------------------
{
    measure_freq();
    //-----------------
    //the next time Timer3 counter 3 reaches period value go to C3
    C_Task_Ptr = &C1;
    //-----------------
}
