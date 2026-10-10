#ifndef APP_H
#define APP_H

// State shared between the main loop and the interrupt handlers

#include <stdint.h>
#include "lamp.h"

// Output set points, read by the ADC scan
typedef struct {
    uint16_t ug1;   // ADC code of the Ug1 divider (higher = less negative)
    uint16_t uh;    // 0.1 V, 0 = not regulated
    uint16_t ih;    // 10 mA, 0 = not regulated
    uint16_t ua;    // PWM compare value ~ volts
    uint16_t ug2;   // PWM compare value ~ volts
} setpoints_t;

extern volatile setpoints_t sp;

// ADC codes of the selected lamp's Ug1 and of the -24 V "grid off" bias
extern uint16_t ug1_ref;
extern uint16_t ug1_off;

// ERR_* flags
extern volatile uint8_t err;

// Send a report line over the serial port (end of a measurement, or ESC)
extern volatile uint8_t report_request;

// Selected tube and editor cursor
extern uint8_t lamp_num;
extern lamp_t  lamp;
extern uint8_t field;       // enum field under the cursor
extern uint8_t lamp_fresh;  // record reloaded since the last section switch

// Live measurements in display units, indexed by PARAM(F_xxx).
// S, R and K are not measured continuously and stay 0 here.
extern uint16_t live[PARAM_COUNT];

void app_init(void);

#endif
