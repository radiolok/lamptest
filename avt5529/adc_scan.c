#include <string.h>
#include "adc_scan.h"
#include "app.h"
#include "control.h"
#include "hal.h"

#define SCAN_STEPS  (2 * ADC_CH_COUNT - 2)  // 14

static struct {
    uint8_t  step;
    uint8_t  scans;
    uint16_t sum[ADC_CH_COUNT];
    uint16_t avg[ADC_CH_COUNT];
    uint8_t  trip_ih, trip_ia, trip_ig2;
    uint8_t  range_high;
    uint8_t  heater;
} scan;

void adc_scan_init(void)
{
    memset(&scan, 0, sizeof(scan));
}

static void check_ih(uint16_t adc)
{
    if (trip_check(&scan.trip_ih, adc > IH_TRIP_ADC)) {
        sp.uh = sp.ih = 0;
        err |= ERR_IH;
    }
}

static void check_ia(uint16_t adc)
{
    if (trip_check(&scan.trip_ia, scan.range_high && adc >= IA_TRIP_ADC)) {
        sp.ua = sp.ug2 = 0;
        hal_set_pwm_ua(0);
        hal_set_pwm_ug2(0);
        err |= ERR_IA;
    }
    hal_set_pwm_ua(ramp_step(hal_pwm_ua(), sp.ua));

    uint8_t high = ia_range_next(scan.range_high, adc, err & ERR_IA);
    if (high != scan.range_high) {
        scan.range_high = high;
        hal_ia_range(high);
    }
}

static void check_ig2(uint16_t adc)
{
    if (trip_check(&scan.trip_ig2, adc >= IG2_TRIP_ADC)) {
        sp.ug2 = 0;
        hal_set_pwm_ug2(0);
        err |= ERR_IG2;
    }
    hal_set_pwm_ug2(ramp_step(hal_pwm_ug2(), sp.ug2));
}

static void end_of_block(void)
{
    memcpy(scan.avg, scan.sum, sizeof(scan.avg));
    memset(scan.sum, 0, sizeof(scan.sum));

    scan.heater = heater_regulate(scan.heater, sp.uh, sp.ih,
                                  scan.avg[ADC_CH_UH], scan.avg[ADC_CH_IH]);
    hal_heater_pwm(scan.heater);

    if (scan.avg[ADC_CH_TEMP] > TEMP_TRIP_SUM)
        err |= ERR_TEMP;
    if (scan.avg[ADC_CH_TEMP] < TEMP_CLEAR_SUM)
        err &= ~ERR_TEMP;
}

void adc_scan_sample(uint16_t adc)
{
    uint8_t step = scan.step;

    if (step & 1) {
        // Ug1 sample: stop pumping once the bias is reached
        if (adc >= sp.ug1)
            hal_ug1_clock(0);
        hal_adc_select(ADC_CH_UG1);
        if (step == SCAN_STEPS - 1) {
            scan.sum[ADC_CH_UG1] += adc;
            if (++scan.scans == ADC_BLOCK_SCANS) {
                scan.scans = 0;
                end_of_block();
            }
        }
    } else {
        uint8_t ch = step >> 1;
        scan.sum[ch] += adc;
        hal_ug1_clock(1);
        hal_adc_select(ch == ADC_CH_IG2 ? ADC_CH_TEMP : ch + 1);
        if (ch == ADC_CH_IH)
            check_ih(adc);
        else if (ch == ADC_CH_IA)
            check_ia(adc);
        else if (ch == ADC_CH_IG2)
            check_ig2(adc);
    }

    scan.step = (step == SCAN_STEPS - 1) ? 0 : step + 1;
}

void adc_averages(uint16_t avg[ADC_CH_COUNT])
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        memcpy(avg, scan.avg, sizeof(scan.avg));
    }
}

uint8_t adc_ia_range(void)
{
    return scan.range_high;
}

uint8_t adc_heater_pwm(void)
{
    return scan.heater;
}
