#include "adc_scan.h"
#include "app.h"
#include "button.h"
#include "check.h"
#include "convert.h"
#include "fake_hal.h"
#include "lampdb.h"
#include "panel.h"
#include "sequencer.h"
#include "test_env.h"

#define FLASH_SLOT  4       // ECC82_G11
#define USER_SLOT   83      // EF86__J02 (user table)

// The serial report: separators become two spaces
static const char *report_line(void)
{
    static char line[2 * RPT_LEN + 1];
    const char *rpt = panel_report();
    char *p = line;
    for (int i = 0; i < RPT_LEN; i++) {
        if (rpt[i] != '\0') {
            *p++ = rpt[i];
        } else {
            *p++ = ' ';
            *p++ = ' ';
        }
    }
    *p = '\0';
    return line;
}

static void preview_record_while_held(void)
{
    select_lamp(FLASH_SLOT);
    button_hold();
    panel_update();
    CHECK_MEM(report_line(),
        "04  ECC82_G11  12.6     0   8.5  250   10.5    0   0.00   2.2   7.7  17.0");
}

static void live_readings(void)
{
    static const uint16_t v[ADC_CH_COUNT] = {
        [ADC_CH_UH] = 317, [ADC_CH_IH] = 9, [ADC_CH_UG1] = 700,
        [ADC_CH_UA] = 825, [ADC_CH_IA] = 500, [ADC_CH_UG2] = 412, [ADC_CH_IG2] = 300,
    };
    select_lamp(FLASH_SLOT);
    sim_adc_blocks(v, 2);
    panel_update();

    uint16_t uh = conv_uh(64 * 317, 64 * 9), ih = conv_ih(64 * 9);
    uint16_t ia = conv_ia(64 * 500, 64 * 825, 0);
    CHECK_EQ(uh, 62);      // 630 minus 4 for the shunt drop
    CHECK_EQ(ih, 8);
    CHECK_EQ(ia, 933);      // 994 minus 61 for the divider current
    CHECK_EQ(live[PARAM(F_UA)], 250);
    CHECK_EQ(live[PARAM(F_UG2)], 124);
    CHECK_MEM(report_line(),
        "04  ECC82_G11   6.2    80   6.5  250   9.33  124  11.62   0.0   0.0   0.0");
}

static void hold_shows_latched_results(void)
{
    select_lamp(FLASH_SLOT);
    seq_click();
    seq_run_until(SEQ_HOLD, 3000);
    panel_update();
    // Nothing connected in the test: everything reads 0, S and R saturate
    CHECK_MEM(panel_report() + RPT_S, "99.9");
    CHECK_MEM(panel_report() + RPT_R, "99.9");
    CHECK_MEM(panel_report() + RPT_K, "99.9");
}

static void error_aborts(void)
{
    select_lamp(FLASH_SLOT);
    err = ERR_TEMP;
    panel_update();
    CHECK_EQ(seq_now(), SEQ_REPORT);
}

static void user_value_saved_on_release(void)
{
    select_lamp(USER_SLOT);
    uint16_t old = lamp.uadef;
    field = F_UA;
    button_hold();
    panel_update();
    lamp.uadef = old + 7;       // as the encoder would
    panel_update();
    CHECK_EQ(fake_eeprom_writes, 0);

    button_release();
    panel_update();
    CHECK_EQ(fake_eeprom_writes, 1);
    lamp_t stored;
    lampdb_load(USER_SLOT, &stored);
    CHECK_EQ(stored.uadef, old + 7);

    panel_update();             // saved once
    CHECK_EQ(fake_eeprom_writes, 1);

    lamp.uadef = old;
    lampdb_save_field(USER_SLOT, &lamp, F_UA);
}

static void user_name_saved_on_release(void)
{
    select_lamp(USER_SLOT);
    uint8_t old = lamp.name[2];
    field = F_NAME + 2;
    button_hold();
    panel_update();
    lamp.name[2] = 0;           // 'A'
    panel_update();
    CHECK_EQ(panel_report()[RPT_NAME + 2], 'A');
    button_release();
    panel_update();

    lamp_t stored;
    lampdb_load(USER_SLOT, &stored);
    CHECK_EQ(stored.name[2], 0);

    lamp.name[2] = old;
    lampdb_save_field(USER_SLOT, &lamp, F_NAME + 2);
}

static void flash_slot_is_never_written(void)
{
    select_lamp(FLASH_SLOT);
    button_hold();
    panel_update();
    button_release();
    panel_update();
    CHECK_EQ(fake_eeprom_writes, 0);
}

static void supply_applies_values(void)
{
    select_lamp(LAMP_SUPPLY);
    field = F_UH;
    button_hold();
    panel_update();
    lamp.uhdef = 63;
    button_release();
    panel_update();
    CHECK_EQ(sp.uh, 63);
    CHECK_EQ(sp.ih, 0);

    field = F_IH;
    button_hold();
    panel_update();
    lamp.ihdef = 30;
    button_release();
    panel_update();
    CHECK_EQ(sp.ih, 30);
    CHECK_EQ(sp.uh, 0);
    CHECK_EQ(lamp.uhdef, 0);

    field = F_UG1;
    button_hold();
    panel_update();
    lamp.ug1def = 50;
    button_release();
    panel_update();
    CHECK_EQ(sp.ug1, conv_ug1_to_adc(50));
    CHECK_EQ(fake_eeprom_writes, 0);
}

static void ia_format_follows_range(void)
{
    static const uint16_t v[ADC_CH_COUNT] = { [ADC_CH_IA] = 1000, [ADC_CH_UG1] = 700 };
    select_lamp(FLASH_SLOT);
    sim_adc_blocks(v, 2);       // switches to the 200 mA range
    CHECK_EQ(adc_ia_range(), 1);
    panel_update();
    CHECK_MEM(panel_report() + RPT_IA, "198.8");    // mA, 0.1 mA resolution
}

int main(void)
{
    RUN_TEST(preview_record_while_held);
    RUN_TEST(live_readings);
    RUN_TEST(hold_shows_latched_results);
    RUN_TEST(error_aborts);
    RUN_TEST(user_value_saved_on_release);
    RUN_TEST(user_name_saved_on_release);
    RUN_TEST(flash_slot_is_never_written);
    RUN_TEST(supply_applies_values);
    RUN_TEST(ia_format_follows_range);
    return check_summary("panel");
}
