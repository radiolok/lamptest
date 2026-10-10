#include "adc_scan.h"
#include "app.h"
#include "check.h"
#include "config.h"
#include "fake_hal.h"
#include "test_env.h"

// Distinct, harmless value on every channel
static const uint16_t quiet[ADC_CH_COUNT] = {
    [ADC_CH_TEMP] = 100, [ADC_CH_IH] = 11, [ADC_CH_UH] = 22, [ADC_CH_UA] = 33,
    [ADC_CH_IA] = 44, [ADC_CH_UG2] = 55, [ADC_CH_IG2] = 66, [ADC_CH_UG1] = 777,
};

static void averages_are_sums_of_64_samples(void)
{
    uint16_t avg[ADC_CH_COUNT];
    sim_adc_blocks(quiet, 2);   // the first block holds power-up garbage
    adc_averages(avg);
    for (int ch = 0; ch < ADC_CH_COUNT; ch++)
        CHECK_EQ(avg[ch], 64 * quiet[ch]);
}

static void averages_update_once_per_block(void)
{
    uint16_t avg[ADC_CH_COUNT], v[ADC_CH_COUNT];
    sim_adc_blocks(quiet, 2);
    memcpy(v, quiet, sizeof(v));
    v[ADC_CH_UA] = 500;
    sim_adc_samples(v, SAMPLES_PER_BLOCK - 1);
    adc_averages(avg);
    CHECK_EQ(avg[ADC_CH_UA], 64 * 33);
    sim_adc_samples(v, 1);
    adc_averages(avg);
    CHECK_EQ(avg[ADC_CH_UA], 64 * 500);
}

static void ug1_is_sampled_every_other_conversion(void)
{
    sim_adc_blocks(quiet, 1);
    for (int i = 0; i < SAMPLES_PER_SCAN; i++) {
        uint8_t was = fake.adc_mux;
        sim_adc_samples(quiet, 1);
        if (was == ADC_CH_UG1)
            CHECK(fake.adc_mux != ADC_CH_UG1);
        else
            CHECK_EQ(fake.adc_mux, ADC_CH_UG1);
    }
}

static void ug1_pump_runs_until_bias_reached(void)
{
    uint16_t v[ADC_CH_COUNT];
    memcpy(v, quiet, sizeof(v));
    sim_adc_blocks(v, 1);

    // Reading below the set point: keep pumping
    sp.ug1 = v[ADC_CH_UG1] + 1;
    for (int i = 0; i < SAMPLES_PER_SCAN; i++) {
        sim_adc_samples(v, 1);
        CHECK_EQ(fake.ug1_clock, 1);
    }
    // Reached: the clock stops after each Ug1 sample
    sp.ug1 = v[ADC_CH_UG1];
    int stops = 0;
    for (int i = 0; i < SAMPLES_PER_SCAN; i++) {
        sim_adc_samples(v, 1);
        stops += fake.ug1_clock == 0;
    }
    CHECK_EQ(stops, SAMPLES_PER_SCAN / 2);
}

static void heater_overcurrent_trips_after_three_samples(void)
{
    uint16_t v[ADC_CH_COUNT];
    memcpy(v, quiet, sizeof(v));
    sim_adc_blocks(v, 1);
    sp.uh = 63;
    v[ADC_CH_IH] = IH_TRIP_ADC + 1;
    sim_adc_samples(v, 2 * SAMPLES_PER_SCAN);
    CHECK_EQ(err, 0);
    CHECK_EQ(sp.uh, 63);
    sim_adc_samples(v, SAMPLES_PER_SCAN);
    CHECK_EQ(err, ERR_IH);
    CHECK_EQ(sp.uh, 0);
}

static void anode_pwm_ramps_to_set_point(void)
{
    sim_adc_blocks(quiet, 1);
    sp.ua = 5;
    sp.ug2 = 3;
    sim_adc_samples(quiet, 3 * SAMPLES_PER_SCAN);
    CHECK_EQ(fake.pwm_ua, 3);
    CHECK_EQ(fake.pwm_ug2, 3);
    sim_adc_samples(quiet, 10 * SAMPLES_PER_SCAN);
    CHECK_EQ(fake.pwm_ua, 5);
    sp.ua = 0;
    sim_adc_samples(quiet, 10 * SAMPLES_PER_SCAN);
    CHECK_EQ(fake.pwm_ua, 0);
}

static void ia_auto_range_and_overcurrent(void)
{
    uint16_t v[ADC_CH_COUNT];
    memcpy(v, quiet, sizeof(v));
    sim_adc_blocks(v, 1);
    sp.ua = 250;
    sp.ug2 = 250;
    fake.pwm_ua = fake.pwm_ug2 = 200;

    // Full scale on the 20 mA range only switches to 200 mA
    v[ADC_CH_IA] = 1023;
    sim_adc_samples(v, SAMPLES_PER_SCAN);
    CHECK_EQ(adc_ia_range(), 1);
    CHECK_EQ(fake.ia_range, 1);
    CHECK_EQ(err, 0);

    // Full scale on the 200 mA range trips
    sim_adc_samples(v, 3 * SAMPLES_PER_SCAN);
    CHECK_EQ(err, ERR_IA);
    CHECK_EQ(sp.ua, 0);
    CHECK_EQ(sp.ug2, 0);
    CHECK_EQ(fake.pwm_ua, 0);
    CHECK_EQ(fake.pwm_ug2, 0);

    // The shunt stays on 200 mA while the error is set
    v[ADC_CH_IA] = 0;
    sim_adc_samples(v, SAMPLES_PER_SCAN);
    CHECK_EQ(adc_ia_range(), 1);
    err = 0;
    sim_adc_samples(v, SAMPLES_PER_SCAN);
    CHECK_EQ(adc_ia_range(), 0);
    CHECK_EQ(fake.ia_range, 0);
}

static void screen_overcurrent(void)
{
    uint16_t v[ADC_CH_COUNT];
    memcpy(v, quiet, sizeof(v));
    sim_adc_blocks(v, 1);
    sp.ua = sp.ug2 = 100;
    v[ADC_CH_IG2] = IG2_TRIP_ADC;
    sim_adc_samples(v, 3 * SAMPLES_PER_SCAN);
    CHECK_EQ(err, ERR_IG2);
    CHECK_EQ(sp.ug2, 0);
    CHECK_EQ(sp.ua, 100);
}

static void heater_regulated_once_per_block(void)
{
    sim_adc_blocks(quiet, 1);
    CHECK_EQ(adc_heater_pwm(), 0);
    sp.uh = 63;
    sim_adc_blocks(quiet, 5);
    CHECK_EQ(adc_heater_pwm(), 5);
    CHECK_EQ(fake.heater_pwm, 5);
    sp.uh = 0;
    sim_adc_blocks(quiet, 1);
    CHECK_EQ(fake.heater_pwm, 0);
}

static void overtemperature_hysteresis(void)
{
    uint16_t v[ADC_CH_COUNT];
    memcpy(v, quiet, sizeof(v));
    v[ADC_CH_TEMP] = TEMP_TRIP_SUM / 64 + 1;    // > 80 C
    sim_adc_blocks(v, 2);
    CHECK_EQ(err, ERR_TEMP);
    v[ADC_CH_TEMP] = 145;                       // 75 C
    sim_adc_blocks(v, 1);
    CHECK_EQ(err, ERR_TEMP);
    v[ADC_CH_TEMP] = TEMP_CLEAR_SUM / 64 - 1;   // < 70 C
    sim_adc_blocks(v, 1);
    CHECK_EQ(err, 0);
}

int main(void)
{
    RUN_TEST(averages_are_sums_of_64_samples);
    RUN_TEST(averages_update_once_per_block);
    RUN_TEST(ug1_is_sampled_every_other_conversion);
    RUN_TEST(ug1_pump_runs_until_bias_reached);
    RUN_TEST(heater_overcurrent_trips_after_three_samples);
    RUN_TEST(anode_pwm_ramps_to_set_point);
    RUN_TEST(ia_auto_range_and_overcurrent);
    RUN_TEST(screen_overcurrent);
    RUN_TEST(heater_regulated_once_per_block);
    RUN_TEST(overtemperature_hysteresis);
    return check_summary("adc_scan");
}
