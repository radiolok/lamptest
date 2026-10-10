#include <string.h>
#include <avr/pgmspace.h>
#include "panel.h"
#include "adc_scan.h"
#include "app.h"
#include "button.h"
#include "config.h"
#include "convert.h"
#include "format.h"
#include "hal.h"
#include "lampdb.h"
#include "sequencer.h"

static char report[RPT_LEN + 1];
static uint8_t dirty;           // a value was changed and is not saved yet
static uint8_t name_reload;

// Position and format of each parameter, indexed by PARAM(F_xxx)
typedef struct {
    uint8_t pos, integral, frac;
} param_fmt_t;

static const param_fmt_t param_fmt[PARAM_COUNT] PROGMEM = {
    [PARAM(F_UG1)] = { RPT_UG1, 2, 1 },
    [PARAM(F_UH)]  = { RPT_UH,  2, 1 },
    [PARAM(F_IH)]  = { RPT_IH,  4, 0 },     // shown in mA, stored in 10 mA
    [PARAM(F_UA)]  = { RPT_UA,  3, 0 },
    [PARAM(F_IA)]  = { RPT_IA,  2, 2 },     // 3, 1 on the 200 mA range
    [PARAM(F_UG2)] = { RPT_UG2, 3, 0 },
    [PARAM(F_IG2)] = { RPT_IG2, 2, 2 },
    [PARAM(F_S)]   = { RPT_S,   2, 1 },
    [PARAM(F_R)]   = { RPT_R,   2, 1 },
    [PARAM(F_K)]   = { RPT_K,   2, 1 },
};

void panel_init(void)
{
    memset(report, 0, sizeof(report));
    dirty = 0;
    name_reload = 1;
}

const char *panel_report(void)
{
    return report;
}

static void render_name(void)
{
    for (uint8_t i = 0; i < sizeof(lamp.name); i++)
        report[RPT_NAME + i] = lamp_name_char(lamp_num, &lamp, i);
}

static void load_lamp(void)
{
    char digits[4];
    fmt_digits(lamp_num, digits);
    report[RPT_NUM] = digits[1];
    report[RPT_NUM + 1] = digits[0];

    lampdb_load(lamp_num, &lamp);
    seq_set_warmup(lamp_warmup_steps(lamp_num, &lamp));
    render_name();
    lamp_fresh = 1;
}

static void edit_name(void)
{
    if (name_reload)
        lampdb_reload_name(lamp_num, &lamp);
    name_reload = 0;
    render_name();

    if (button_held())
        dirty = 1;
    if (button_released() && dirty) {
        lampdb_save_field(lamp_num, &lamp, field);
        dirty = 0;
    }
}

static void measure(void)
{
    uint16_t avg[ADC_CH_COUNT];
    adc_averages(avg);

    live[PARAM(F_UG1)] = conv_ug1(avg[ADC_CH_UG1]);
    live[PARAM(F_UH)]  = conv_uh(avg[ADC_CH_UH], avg[ADC_CH_IH]);
    live[PARAM(F_IH)]  = conv_ih(avg[ADC_CH_IH]);
    live[PARAM(F_UA)]  = conv_ua(avg[ADC_CH_UA]);
    live[PARAM(F_IA)]  = conv_ia(avg[ADC_CH_IA], avg[ADC_CH_UA], adc_ia_range());
    live[PARAM(F_UG2)] = conv_ua(avg[ADC_CH_UG2]);
    live[PARAM(F_IG2)] = conv_ig2(avg[ADC_CH_IG2], avg[ADC_CH_UG2]);
}

// The power supply applies a confirmed value right away
static void apply_supply(uint8_t f)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        switch (f) {
        case F_UG1:
            sp.ug1 = ug1_ref;
            break;
        case F_UH:
            sp.uh = lamp.uhdef;
            sp.ih = lamp.ihdef = 0;
            break;
        case F_IH:
            sp.ih = lamp.ihdef;
            sp.uh = lamp.uhdef = 0;
            break;
        case F_UA:
            sp.ua = lamp.uadef;
            break;
        case F_UG2:
            sp.ug2 = lamp.ug2def;
            break;
        }
    }
}

static void commit(uint8_t f)
{
    if (lamp_num == LAMP_SUPPLY)
        apply_supply(f);
    if (lamp_is_user(lamp_num))
        lampdb_save_field(lamp_num, &lamp, f);
}

static void render_params(uint16_t t)
{
    uint8_t holding = (t == SEQ_HOLD);
    const results_t *res = seq_results();
    const uint16_t *latched = seq_latched();

    for (uint8_t p = 0; p < PARAM_COUNT; p++) {
        uint8_t f = F_UG1 + p;
        uint16_t value;
        uint8_t show_def = 0;

        if (holding)
            value = latched[p];
        else if (f == F_S)
            value = res->s;
        else if (f == F_R)
            value = res->r;
        else if (f == F_K)
            value = res->k;
        else
            value = live[p];

        // Slot selection previews the record; the field under the cursor
        // shows its set value while the button is held.
        if (field == F_LAMP || field == f) {
            if (button_held()) {
                value = lamp_field_get(&lamp, f);
                show_def = 1;
                dirty = 1;
            }
            if (button_released() && field == f && dirty) {
                dirty = 0;
                commit(f);
            }
        }

        param_fmt_t fmt;
        memcpy_P(&fmt, &param_fmt[p], sizeof(fmt));
        uint8_t integral = fmt.integral;
        uint8_t frac = fmt.frac;
        if (f == F_IH)
            value *= 10;
        if (f == F_IA) {
            uint8_t range_high = holding ? seq_latched_range() : adc_ia_range();
            if (show_def || range_high) {
                integral = 3;
                frac = 1;
            }
        }
        fmt_fixed(value, integral, frac, &report[fmt.pos]);
    }
}

void panel_update(void)
{
    uint16_t t = seq_now();

    // Any error aborts a measurement and holds to show it
    if (err != 0 && (t == SEQ_IDLE || t > SEQ_REPORT)) {
        seq_abort(1);
        t = SEQ_REPORT;
    }

    if (field == F_LAMP)
        load_lamp();

    if (field >= F_NAME && field <= F_WARMUP)
        edit_name();
    else
        name_reload = 1;

    ug1_ref = conv_ug1_to_adc(lamp.ug1def);

    measure();
    render_params(t);
}
