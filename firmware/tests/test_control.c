#include "check.h"
#include "config.h"
#include "control.h"
#include "legacy.h"

static void ramp(void)
{
    CHECK_EQ(ramp_step(10, 20), 11);
    CHECK_EQ(ramp_step(20, 10), 19);
    CHECK_EQ(ramp_step(15, 15), 15);
    CHECK_EQ(ramp_step(0, 0), 0);
}

static void trip_after_consecutive_samples(void)
{
    uint8_t counter = 0;
    CHECK_EQ(trip_check(&counter, 0), 0);
    CHECK_EQ(counter, TRIP_SAMPLES);
    for (int i = 0; i < TRIP_SAMPLES; i++)
        CHECK_EQ(trip_check(&counter, 1), 0);
    CHECK_EQ(trip_check(&counter, 1), 1);
    CHECK_EQ(trip_check(&counter, 1), 1);       // stays tripped while over
}

static void trip_filter_resets_on_a_good_sample(void)
{
    uint8_t counter = TRIP_SAMPLES;
    for (int n = 0; n < 10; n++) {
        CHECK_EQ(trip_check(&counter, 1), 0);
        CHECK_EQ(trip_check(&counter, 0), 0);
    }
}

static void trip_is_immediate_from_power_up(void)
{
    // Counters start at 0, so a fault present at power-up trips at once
    uint8_t counter = 0;
    CHECK_EQ(trip_check(&counter, 1), 1);
}

static void ia_range_hysteresis(void)
{
    CHECK_EQ(ia_range_next(0, IA_RANGE_UP_ADC, 0), 0);
    CHECK_EQ(ia_range_next(0, IA_RANGE_UP_ADC + 1, 0), 1);
    CHECK_EQ(ia_range_next(1, 500, 0), 1);
    CHECK_EQ(ia_range_next(1, IA_RANGE_DOWN_ADC, 0), 1);
    CHECK_EQ(ia_range_next(1, IA_RANGE_DOWN_ADC - 1, 0), 0);
    CHECK_EQ(ia_range_next(1, 0, 1), 1);        // keep the 200 mA shunt after a trip
    CHECK_EQ(ia_range_next(0, 0, 0), 0);
}

static void heater_matches_legacy(void)
{
    int bad = 0;
    for (int i = 0; i < 2000000; i++) {
        uint8_t pwm = xorshift();
        uint16_t uh = (i & 3) == 0 ? 0 : xorshift() % 160;
        uint16_t ih = (i & 3) == 1 ? 0 : xorshift() % 260;
        uint16_t muh = xorshift() % 65473, mih = xorshift() % (i & 4 ? 65473 : 64);
        bad += heater_regulate(pwm, uh, ih, muh, mih) != legacy_heater(pwm, uh, ih, muh, mih);
    }
    CHECK_EQ(bad, 0);
}

static void heater_voltage_mode(void)
{
    CHECK_EQ(heater_regulate(100, 0, 0, 0, 0), 0);      // off
    CHECK_EQ(heater_regulate(100, 63, 0, 0, 0), 101);   // too low
    CHECK_EQ(heater_regulate(255, 63, 0, 0, 0), 255);   // saturates
    CHECK_EQ(heater_regulate(100, 63, 0, 65000, 0), 99);
    CHECK_EQ(heater_regulate(0, 63, 0, 65000, 0), 0);
    CHECK_EQ(heater_regulate(100, 63, 0, 20279, 0), 100); // on target
}

static void heater_current_mode_needs_current_flow(void)
{
    // Up to pwm 8 it may rise blindly, beyond that only with Ih > 5 mA
    CHECK_EQ(heater_regulate(7, 0, 30, 0, 0), 8);
    CHECK_EQ(heater_regulate(8, 0, 30, 0, 0), 8);
    CHECK_EQ(heater_regulate(8, 0, 30, 0, 33), 9);
    CHECK_EQ(heater_regulate(100, 0, 30, 0, 64 * 400), 99);
}

int main(void)
{
    RUN_TEST(ramp);
    RUN_TEST(trip_after_consecutive_samples);
    RUN_TEST(trip_filter_resets_on_a_good_sample);
    RUN_TEST(trip_is_immediate_from_power_up);
    RUN_TEST(ia_range_hysteresis);
    RUN_TEST(heater_matches_legacy);
    RUN_TEST(heater_voltage_mode);
    RUN_TEST(heater_current_mode_needs_current_flow);
    return check_summary("control");
}
