#include <string.h>
#include "sequencer.h"
#include "adc_scan.h"
#include "app.h"
#include "config.h"
#include "convert.h"
#include "hal.h"

static uint16_t start;          // remaining steps, see enum seq_point
static uint16_t warmup;         // heater warm-up of the selected lamp
static uint8_t  hold;           // stop at SEQ_HOLD after the measurement

// Readings at the two grid and the two anode voltages
static uint16_t ug_r, ug_l, ia_r, ia_l, ua_l, ua_r;

static results_t res;
static uint16_t latched[PARAM_COUNT];
static uint8_t  latched_range;

void seq_init(void)
{
    start = SEQ_IDLE;
    warmup = 0;
    hold = 0;
    memset(&res, 0, sizeof(res));
    memset(latched, 0, sizeof(latched));
    latched_range = 0;
}

// Ia in 0.01 mA regardless of the range
static uint16_t ia_now(void)
{
    uint16_t ia = live[PARAM(F_IA)];
    return adc_ia_range() ? ia * 10 : ia;
}

static void heater_off(void)
{
    sp.uh = sp.ih = 0;
    if (lamp_num == LAMP_SUPPLY)
        lamp.uhdef = lamp.ihdef = 0;
}

static void latch_results(void)
{
    latched[PARAM(F_UH)]  = live[PARAM(F_UH)];
    latched[PARAM(F_IH)]  = live[PARAM(F_IH)];
    latched[PARAM(F_UG1)] = live[PARAM(F_UG1)];
    latched[PARAM(F_UA)]  = live[PARAM(F_UA)];
    latched[PARAM(F_IA)]  = live[PARAM(F_IA)];
    latched[PARAM(F_UG2)] = live[PARAM(F_UG2)];
    latched[PARAM(F_IG2)] = live[PARAM(F_IG2)];
    latched[PARAM(F_S)]   = res.s;
    latched[PARAM(F_R)]   = res.r;
    latched[PARAM(F_K)]   = res.k;
    latched_range = adc_ia_range();
    report_request = 1;
}

static void run_step(uint16_t t)
{
    // Heater on. With no warm-up this coincides with SEQ_UG_R.
    if (t == warmup + SEQ_UG_R) {
        // A record sets either Uh or Ih (series heaters)
        if (lamp.uhdef != 0) {
            sp.uh = lamp.uhdef;
            sp.ih = lamp.ihdef = 0;
        }
        if (lamp.ihdef != 0) {
            sp.ih = lamp.ihdef;
            sp.uh = lamp.uhdef = 0;
        }
    }

    switch (t) {
    case SEQ_UG_R:
        if (lamp_section(&lamp) == 2)
            hal_anode_select(1);
        sp.ug1 = ug1_ref - UG1_DELTA_ADC;   // slightly more negative
        break;
    case SEQ_UA_ON:
        sp.ua = lamp.uadef;
        break;
    case SEQ_UG2_ON:
        sp.ug2 = lamp.ug2def;
        break;
    case SEQ_READ_IAG_R:
        ug_r = live[PARAM(F_UG1)];
        ia_r = ia_now();
        break;
    case SEQ_UG_L:
        sp.ug1 = ug1_ref + UG1_DELTA_ADC;   // slightly less negative
        break;
    case SEQ_READ_IAG_L:
        ug_l = live[PARAM(F_UG1)];
        ia_l = ia_now();
        res.s = calc_s(ia_l, ia_r, ug_l, ug_r);
        break;
    case SEQ_UG_NOMINAL:
        sp.ug1 = ug1_ref;
        break;
    case SEQ_UA_L:
        sp.ua = lamp.uadef - UA_DELTA;
        break;
    case SEQ_READ_IAA_L:
        ua_l = live[PARAM(F_UA)];
        ia_l = ia_now();
        break;
    case SEQ_UA_R:
        sp.ua = lamp.uadef + UA_DELTA;
        break;
    case SEQ_READ_IAA_R:
        ua_r = live[PARAM(F_UA)];
        ia_r = ia_now();
        res.r = calc_r(ua_l, ua_r, ia_l, ia_r);
        break;
    case SEQ_UA_NOMINAL:
        sp.ua = lamp.uadef;
        res.k = calc_k(res.s, res.r);
        break;
    case SEQ_REPORT:
        latch_results();
        break;
    case SEQ_UG2_OFF:
        sp.ug2 = 0;
        if (lamp_num == LAMP_SUPPLY)
            lamp.ug2def = 0;
        break;
    case SEQ_UA_OFF:
        sp.ua = 0;
        if (lamp_num == LAMP_SUPPLY)
            lamp.uadef = 0;
        break;
    case SEQ_UG1_OFF:
        sp.ug1 = ug1_off;
        hal_anode_select(0);
        break;
    // A normal end gives one long beep, an error two short ones
    case SEQ_BEEP:
        if (hold)
            hal_beeper(1);
        break;
    case SEQ_BEEP - 1:
        if (err) {
            hal_beeper(0);
            heater_off();       // on an error, switch the heater off too
        }
        break;
    case SEQ_BEEP - 2:
        if (hold)
            hal_beeper(1);
        break;
    case SEQ_BEEP - 3:
        hal_beeper(0);
        break;
    case SEQ_HEATER_OFF:
        heater_off();
        break;
    case SEQ_CLEAR:
        err = 0;
        seq_clear_results();
        break;
    }
}

void seq_tick(void)
{
    uint16_t t = start;

    run_step(t);

    if (t == SEQ_HOLD) {
        if (!hold)
            start = SEQ_HEATER_OFF;
    } else if (t > SEQ_IDLE) {
        start--;
    }
}

void seq_click(void)
{
    if (lamp_num < LAMP_FIRST_TUBE || err != 0)
        return;
    if (start == SEQ_IDLE && field == F_LAMP)
        start = warmup + SEQ_UG_R;
    else if (start == SEQ_HOLD)
        start = SEQ_UG_R;       // the heater is still warm
    else
        return;
    hold = 1;
    seq_clear_results();
}

void seq_abort(uint8_t hold_after)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        hold = hold_after;
        start = SEQ_REPORT;
    }
}

void seq_release(void)
{
    start = SEQ_HEATER_OFF;
}

uint16_t seq_now(void)
{
    uint16_t t;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        t = start;
    }
    return t;
}

uint8_t seq_holding(void)
{
    return seq_now() == SEQ_HOLD;
}

void seq_set_warmup(uint16_t steps)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        warmup = steps;
    }
}

void seq_clear_results(void)
{
    memset(&res, 0, sizeof(res));
    for (uint8_t p = PARAM(F_UA); p < PARAM_COUNT; p++)
        latched[p] = 0;
}

const results_t *seq_results(void)
{
    return &res;
}

const uint16_t *seq_latched(void)
{
    return latched;
}

uint8_t seq_latched_range(void)
{
    return latched_range;
}
