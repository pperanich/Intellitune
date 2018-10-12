// Header file for Intellitune project.
// Includes global variable declarations and typedefs


// All custom typedefs used for this project
typedef struct
{
    uint32_t fwd = 0;
    uint32_t ref = 0;
} SWR;


// Globals


// Subsystem function declarations

// Stepper motor subsystem
extern void step_motor(uint8_t motor, uint8_t dir, uint8_t );
extern void current_setting(void);

// Frequency Counter subsystem
extern float measure_freq(void);

// Standing Wave Ratio subsystem
extern SWR measure_swr(void);
extern void update_digipot(void);

// LCD subsystem
extern void lcd_init(void);
extern void lcd_update(void);

// Relay subsystem
extern void switch_relay(int relay_num, int position);
extern void switch_net_config(void);
