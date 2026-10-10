#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <stdint.h>
#include "lamp.h"

// Measurement sequence, one step every 250 ms.
//
// `start` counts the remaining steps down to 0 (idle). Each point below is
// the value of `start` at which its action runs:
//
//   heat (warm-up) UgR  Ua  Ug2  IagR  UgL  IagL  Ug  UaL  IaaL  UaR  IaaR  Ua
//   REPORT  Ug2off  Uaoff  Ug1off  beep...  HOLD  heater off  ...  idle
//
// IagR/IagL give S, IaaL/IaaR give R, and K = S * R. At REPORT the results
// are latched for the display and sent over the serial port. With `hold`
// set, the sequence then stops at HOLD with the heater still on.

// Step durations
#define SEQ_MARGIN  2   // settling before a reading
#define SEQ_T_UA    8
#define SEQ_T_UG2   8
#define SEQ_T_UG    16
#define SEQ_T_OFF   2   // between switch-off steps
#define SEQ_T_BEEP  4

enum seq_point {
    SEQ_IDLE        = 0,
    SEQ_CLEAR       = 1,                            // clear error and results
    SEQ_HEATER_OFF  = 3,
    SEQ_HOLD        = SEQ_HEATER_OFF + 1,           // 4
    SEQ_BEEP        = SEQ_HOLD + SEQ_T_BEEP,        // 8
    SEQ_UG1_OFF     = SEQ_BEEP + SEQ_T_OFF,         // 10
    SEQ_UA_OFF      = SEQ_UG1_OFF + SEQ_T_OFF,      // 12
    SEQ_UG2_OFF     = SEQ_UA_OFF + SEQ_T_OFF,       // 14
    SEQ_REPORT      = SEQ_UG2_OFF + SEQ_MARGIN,     // 16, also where an abort jumps
    SEQ_UA_NOMINAL  = SEQ_REPORT + SEQ_T_UA,        // 24, K
    SEQ_READ_IAA_R  = SEQ_UA_NOMINAL + SEQ_MARGIN,  // 26, R
    SEQ_UA_R        = SEQ_READ_IAA_R + SEQ_T_UA,    // 34
    SEQ_READ_IAA_L  = SEQ_UA_R + SEQ_MARGIN,        // 36
    SEQ_UA_L        = SEQ_READ_IAA_L + SEQ_T_UA,    // 44
    SEQ_UG_NOMINAL  = SEQ_UA_L + SEQ_T_UG,          // 60
    SEQ_READ_IAG_L  = SEQ_UG_NOMINAL + SEQ_MARGIN,  // 62, S
    SEQ_UG_L        = SEQ_READ_IAG_L + SEQ_T_UG,    // 78
    SEQ_READ_IAG_R  = SEQ_UG_L + SEQ_MARGIN,        // 80
    SEQ_UG2_ON      = SEQ_READ_IAG_R + SEQ_T_UG2,   // 88
    SEQ_UA_ON       = SEQ_UG2_ON + SEQ_T_UA,        // 96
    SEQ_UG_R        = SEQ_UA_ON + SEQ_T_UG,         // 112, re-measure starts here
    // Heater on: SEQ_UG_R + warm-up
};

typedef struct {
    uint16_t s, r, k;
} results_t;

void seq_init(void);

// Called every 250 ms: runs the action due now and advances
void seq_tick(void);

// Click: start a measurement (when idle) or re-measure (when holding)
void seq_click(void);

// Jump to the switch-off part. `hold`: stop at HOLD afterwards.
void seq_abort(uint8_t hold);

// Encoder turned while holding: switch the heater off and go idle
void seq_release(void);

uint16_t seq_now(void);
uint8_t  seq_holding(void);
void     seq_set_warmup(uint16_t steps);

// Clear S, R, K and the latched anode and screen readings
void seq_clear_results(void);

const results_t *seq_results(void);

// Readings latched at SEQ_REPORT, indexed by PARAM(F_xxx)
const uint16_t *seq_latched(void);
uint8_t seq_latched_range(void);

#endif
