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

int check_failures;
int check_count;

static uint8_t converting;

void test_reset(void)
{
    fake_hal_reset();
    app_init();
    adc_scan_init();
    button_init();
    seq_init();
    panel_init();
    // Power-up: the first conversion runs on Ug1, main() then selects Ih
    converting = ADC_CH_UG1;
    fake.adc_mux = ADC_CH_IH;
}

int check_summary(const char *suite)
{
    printf("%s: %d checks, %d failed\n", suite, check_count, check_failures);
    return check_failures ? 1 : 0;
}

void sim_adc_samples(const uint16_t values[ADC_CH_COUNT], unsigned n)
{
    while (n--) {
        uint16_t result = values[converting];
        converting = fake.adc_mux;      // the next conversion starts now
        adc_scan_sample(result);
    }
}

void sim_adc_blocks(const uint16_t values[ADC_CH_COUNT], unsigned blocks)
{
    sim_adc_samples(values, blocks * SAMPLES_PER_BLOCK);
}

void button_for(uint8_t pressed, unsigned ms)
{
    while (ms--)
        button_poll(pressed);
}

void button_hold(void)
{
    button_for(1, BTN_LONG + 1);
}

void button_release(void)
{
    button_for(0, BTN_DEBOUNCE + 2);
}

void select_lamp(uint8_t num)
{
    lamp_num = num;
    field = F_LAMP;
    panel_update();
}

void seq_run_until(uint16_t point, unsigned limit)
{
    while (seq_now() != point && limit--)
        seq_tick();
}
