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
uint8_t adc_ready = 1;


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
    if(task_flag & BIT0)
    {
        task_flag &= ~BIT0; // Clear flag

        //-----------------------------------------------------------
        (*A_Task_Ptr)();        // jump to an A Task (A1,A2,A3,...)
        //-----------------------------------------------------------
    }
    Alpha_State_Ptr = &B0;      // Comment out to allow only A tasks
}

void B0(void)
{
    // loop rate synchronizer for B-tasks
    if(task_flag & BIT1)
    {
        task_flag &= ~BIT1; // Clear flag

        //-----------------------------------------------------------
        (*B_Task_Ptr)();        // jump to a B Task (B1,B2,B3,...)
        //-----------------------------------------------------------
    }

    Alpha_State_Ptr = &C0;      // Allow C state tasks
}

void C0(void)
{
    // loop rate synchronizer for C-tasks
    if(task_flag & BIT2)
    {
        task_flag &= ~BIT2; // Clear flag

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
    if(!(ADCCTL1 & ADCBUSY)) {adc_ready = 1;} // Wait if ADC core is active
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
    updateBoardStatus();
#if INCR_BUILD==1
    #if DC_CHECK==1
        BuildInfo=BuildLevel1_OpenLoop_DC;
    #else
        BuildInfo=BuildLevel1_OpenLoop_AC;
    #endif
#elif INCR_BUILD==2
    #if DC_CHECK==1
        BuildInfo=BuildLevel2_CurrentLoop_DC;
    #else
        BuildInfo=BuildLevel2_CurrentLoop_AC;
    #endif
#elif INCR_BUILD==3
    #if DC_CHECK==1
        BuildInfo=BuildLevel3_VoltageLoop_DC;
    #else
        BuildInfo=BuildLevel3_VoltageLoop_AC;
    #endif
#endif
    //-----------------
    //the next time Timer3 counter 2 reaches period value go to B2
    B_Task_Ptr = &B2;
    //-----------------
}

//----------------------------------------
void B2(void) //  SPARE
//----------------------------------------
{
    switch(boardState)
    {
        case boardState_InverterOFF:
            if (guiInvStart==1)
                {
                    guiInvStart=0;
                    boardState=boardState_CheckDCBus;
                }
            break;
        case boardState_CheckDCBus:
            // assuming a ~10% margin on the DC Bus vs Typical AC Output Max
            // Only start the inverter if the DC Bus is higher than that
            if(guiVbus>(float)1.55*VAC_TYPICAL && guiVbus<VAC_MAX_SENSE)
            {
                boardState=boardState_RlyConnect;
                rlyConnect=1;
                slewState=20;
            }
            break;
        case boardState_RlyConnect:
            slewState--;
            if(slewState==0)
            {
                boardState=boardState_InverterON;
                invVoRef=0;
                clearInvTrip=1;
            }
            break;
        case boardState_InverterON:
            if(invVoRef<((float)VAC_TYPICAL*(float)1.414/(float)VAC_MAX_SENSE))
            {
                invVoRef=invVoRef+0.005;
            }
            if(guiInvStop==1)
                {
                    guiInvStop=0;
                    boardState=boardState_InverterOFF;
                    invVoRef=0;
                    rlyConnect=0;
                    closeILoopInv=0;
                    DINT;
                    //CNTL3P3Z Inverter Current Loop
                    CNTL_3P3Z_F_VARS_init(&cntl3p3z_InvI_vars);
                    //CNTL2P2Z PR 1H Controller
                    CNTL_2P2Z_F_VARS_init(&cntl2p2z_PRV_vars);
                    //CNTL2P2Z R 3HController
                    CNTL_2P2Z_F_VARS_init(&cntl2p2z_RV3_vars);
                    //CNTL2P2Z R 5H Controller
                    CNTL_2P2Z_F_VARS_init(&cntl2p2z_RV5_vars);
                    //CNTL2P2Z R 7H Controller
                    CNTL_2P2Z_F_VARS_init(&cntl2p2z_RV7_vars);
                    //CNTL2P2Z R 9H Controller
                    CNTL_2P2Z_F_VARS_init(&cntl2p2z_RV9_vars);
                    //CNTL2P2Z R 11H Controller
                    CNTL_2P2Z_F_VARS_init(&cntl2p2z_RV11_vars);
                    //CNTL2P2Z Lead Lag Controller
                    CNTL_2P2Z_F_VARS_init(&cntl2p2z_LeadLag_vars);
                    EINT;
                }
            if(boardStatus==boardStatus_OverCurrentTrip ||boardStatus== boardStatus_EmulatorStopTrip)
            {
                boardState= boardState_TripCondition;
            }
            break;
        case boardState_TripCondition: break;
    }
    //-----------------
    //the next time Timer3 counter 2 reaches period value go to B3
    B_Task_Ptr = &B3;
    //-----------------
}

//----------------------------------------
void B3(void) //  SPARE
//----------------------------------------
{

    //-----------------
    //the next time Timer3 counter 2 reaches period value go to B1
    //tune();
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

    //-----------------
    //the next time Timer3 counter 3 reaches period value go to C3
    C_Task_Ptr = &C3;
    //-----------------
}


//-----------------------------------------
void C3(void) //  SPARE
//-----------------------------------------
{

    //-----------------
    //the next time Timer3 counter 3 reaches period value go to C1
    C_Task_Ptr = &C1;
    //-----------------
}
