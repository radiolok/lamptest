#include "adc_scan.h"
#include "app.h"
#include "check.h"
#include "config.h"
#include "convert.h"
#include "fake_hal.h"
#include "lampdb.h"
#include "sequencer.h"
#include "test_env.h"

#define ECC82_SECTION1  4   // ECC82_G11: Uh 12.6 V, Ug -8.5 V, Ua 250 V, Ia 10.5 mA
#define ECC82_SECTION2  5

// Linear triode around its operating point:
// Ia [0.01 mA] = 1050 + 22 per 0.1 V less bias + 13 per volt of anode
static void tube_model(void)
{
    int ug = conv_ug1(64 * sp.ug1);
    int ia = 1050 + 22 * (lamp.ug1def - ug) + 13 * ((int)sp.ua - (int)lamp.uadef);
    live[PARAM(F_UG1)] = ug;
    live[PARAM(F_UA)]  = sp.ua;
    live[PARAM(F_IA)]  = sp.ua ? ia : 0;
    live[PARAM(F_UH)]  = sp.uh;
    live[PARAM(F_IH)]  = sp.uh ? 30 : 0;
}

static void tick(void)
{
    tube_model();
    seq_tick();
}

static void run_until(uint16_t point)
{
    for (unsigned n = 0; n < 3000 && seq_now() != point; n++)
        tick();
}

static void select(uint8_t num)
{
    lamp_num = num;
    field = F_LAMP;
    lampdb_load(num, &lamp);
    seq_set_warmup(lamp_warmup_steps(num, &lamp));
    ug1_ref = conv_ug1_to_adc(lamp.ug1def);
}

static void timeline(void)
{
    select(ECC82_SECTION1);
    seq_click();
    CHECK_EQ(seq_now(), WARMUP_TICKS_PER_MIN + SEQ_UG_R);  // 1 min warm-up

    tick();
    CHECK_EQ(sp.uh, 126);
    CHECK_EQ(sp.ih, 0);
    CHECK_EQ(sp.ua, 0);

    run_until(SEQ_UG_R);
    CHECK_EQ(sp.ug1, ug1_off);
    tick();
    CHECK_EQ(sp.ug1, ug1_ref - UG1_DELTA_ADC);
    CHECK_EQ(fake.anode2, 0);

    run_until(SEQ_UA_ON - 1);
    CHECK_EQ(sp.ua, 250);

    run_until(SEQ_UG_L - 1);
    CHECK_EQ(sp.ug1, ug1_ref + UG1_DELTA_ADC);
    run_until(SEQ_UG_NOMINAL - 1);
    CHECK_EQ(sp.ug1, ug1_ref);
    run_until(SEQ_UA_L - 1);
    CHECK_EQ(sp.ua, 250 - UA_DELTA);
    run_until(SEQ_UA_R - 1);
    CHECK_EQ(sp.ua, 250 + UA_DELTA);
    run_until(SEQ_UA_NOMINAL - 1);
    CHECK_EQ(sp.ua, 250);

    CHECK_EQ(report_request, 0);
    run_until(SEQ_REPORT - 1);
    CHECK_EQ(report_request, 1);

    run_until(SEQ_UG2_OFF - 1);
    run_until(SEQ_UA_OFF - 1);
    CHECK_EQ(sp.ua, 0);
    run_until(SEQ_UG1_OFF - 1);
    CHECK_EQ(sp.ug1, ug1_off);

    // One long beep, then hold with the heater on
    run_until(SEQ_BEEP - 1);
    CHECK_EQ(fake.beeper, 1);
    tick();
    CHECK_EQ(fake.beeper, 1);
    tick();
    CHECK_EQ(fake.beeper, 1);
    tick();
    CHECK_EQ(fake.beeper, 0);
    CHECK_EQ(seq_now(), SEQ_HOLD);
    for (int i = 0; i < 100; i++)
        tick();
    CHECK_EQ(seq_now(), SEQ_HOLD);
    CHECK(seq_holding());
    CHECK_EQ(sp.uh, 126);
}

static void results(void)
{
    select(ECC82_SECTION1);
    seq_click();
    run_until(SEQ_HOLD);

    // S = 2.2 mA/V, R = 20 V / 2.60 mA
    CHECK_EQ(seq_results()->s, 22);
    CHECK_EQ(seq_results()->r, 76);
    CHECK_EQ(seq_results()->k, 167);

    const uint16_t *l = seq_latched();
    CHECK_EQ(l[PARAM(F_S)], 22);
    CHECK_EQ(l[PARAM(F_R)], 76);
    CHECK_EQ(l[PARAM(F_K)], 167);
    CHECK_EQ(l[PARAM(F_UA)], 250);
    CHECK_EQ(l[PARAM(F_IA)], 1050);
    CHECK_EQ(l[PARAM(F_UH)], 126);
    CHECK_EQ(l[PARAM(F_UG1)], 85);
}

static void second_section_uses_anode_two(void)
{
    select(ECC82_SECTION2);
    seq_click();
    run_until(SEQ_UG_R - 1);
    CHECK_EQ(fake.anode2, 1);
    run_until(SEQ_UG1_OFF - 1);
    CHECK_EQ(fake.anode2, 0);
}

static void remeasure_skips_warmup(void)
{
    select(ECC82_SECTION1);
    seq_click();
    run_until(SEQ_HOLD);
    seq_click();
    CHECK_EQ(seq_now(), SEQ_UG_R);
    CHECK_EQ(seq_results()->s, 0);
    CHECK_EQ(seq_latched()[PARAM(F_S)], 0);
    CHECK_EQ(seq_latched()[PARAM(F_UH)], 126);     // only anode side is cleared
    run_until(SEQ_HOLD);
    CHECK_EQ(seq_results()->k, 167);
}

static void release_switches_heater_off(void)
{
    select(ECC82_SECTION1);
    seq_click();
    run_until(SEQ_HOLD);
    seq_release();
    tick();
    CHECK_EQ(sp.uh, 0);
    run_until(SEQ_IDLE);
    CHECK_EQ(seq_results()->s, 0);
    for (int i = 0; i < 10; i++)
        tick();
    CHECK_EQ(seq_now(), SEQ_IDLE);
}

static void abort_without_hold(void)
{
    select(ECC82_SECTION1);
    seq_click();
    run_until(SEQ_UG2_ON);
    seq_abort(0);
    CHECK_EQ(seq_now(), SEQ_REPORT);
    unsigned beeps = 0;
    for (int i = 0; i < 30; i++) {
        tick();
        beeps += fake.beeper;
    }
    CHECK_EQ(beeps, 0);
    CHECK_EQ(seq_now(), SEQ_IDLE);
    CHECK_EQ(sp.uh, 0);
    CHECK_EQ(sp.ua, 0);
    CHECK_EQ(report_request, 1);
}

static void error_beeps_twice_and_holds_heater_off(void)
{
    select(ECC82_SECTION1);
    seq_click();
    run_until(SEQ_UA_ON);
    err = ERR_IA;
    seq_abort(1);

    char beeps[8];
    run_until(SEQ_BEEP);
    for (int i = 0; i < 4; i++) {
        tick();
        beeps[i] = fake.beeper ? '#' : '.';
    }
    CHECK_MEM(beeps, "#.#.");
    CHECK_EQ(sp.uh, 0);
    CHECK_EQ(seq_now(), SEQ_HOLD);

    // Error is shown until released, then cleared
    seq_release();
    run_until(SEQ_IDLE);
    CHECK_EQ(err, 0);
}

static void click_conditions(void)
{
    select(LAMP_SUPPLY);
    seq_click();
    CHECK_EQ(seq_now(), SEQ_IDLE);
    select(1);
    seq_click();
    CHECK_EQ(seq_now(), SEQ_IDLE);

    select(ECC82_SECTION1);
    err = ERR_TEMP;
    seq_click();
    CHECK_EQ(seq_now(), SEQ_IDLE);
    err = 0;

    field = F_UA;
    seq_click();
    CHECK_EQ(seq_now(), SEQ_IDLE);
    field = F_LAMP;

    seq_click();
    uint16_t t = seq_now();
    tick();
    seq_click();    // ignored while running
    CHECK_EQ(seq_now(), t - 1);
}

static void current_heated_tube(void)
{
    select(38);     // PCL86T: series heater, Ih 300 mA
    CHECK_EQ(lamp.uhdef, 0);
    CHECK_EQ(lamp.ihdef, 30);
    seq_click();
    tick();
    CHECK_EQ(sp.ih, 30);
    CHECK_EQ(sp.uh, 0);
}

static void supply_outputs_cleared_on_switch_off(void)
{
    select(LAMP_SUPPLY);
    lamp.uadef = 100;
    lamp.ug2def = 50;
    lamp.uhdef = 63;
    sp.ua = 100;
    sp.ug2 = 50;
    sp.uh = 63;
    seq_abort(0);
    run_until(SEQ_IDLE);
    CHECK_EQ(sp.ua, 0);
    CHECK_EQ(sp.ug2, 0);
    CHECK_EQ(sp.uh, 0);
    CHECK_EQ(lamp.uadef, 0);
    CHECK_EQ(lamp.ug2def, 0);
    CHECK_EQ(lamp.uhdef, 0);
}

int main(void)
{
    RUN_TEST(timeline);
    RUN_TEST(results);
    RUN_TEST(second_section_uses_anode_two);
    RUN_TEST(remeasure_skips_warmup);
    RUN_TEST(release_switches_heater_off);
    RUN_TEST(abort_without_hold);
    RUN_TEST(error_beeps_twice_and_holds_heater_off);
    RUN_TEST(click_conditions);
    RUN_TEST(current_heated_tube);
    RUN_TEST(supply_outputs_cleared_on_switch_off);
    return check_summary("sequencer");
}
