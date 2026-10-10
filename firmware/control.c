#include "config.h"
#include "control.h"
#include "convert.h"

uint16_t ramp_step(uint16_t value, uint16_t target)
{
    if (value < target)
        return value + 1;
    if (value > target)
        return value - 1;
    return value;
}

uint8_t trip_check(uint8_t *counter, uint8_t over)
{
    if (!over) {
        *counter = TRIP_SAMPLES;
        return 0;
    }
    if (*counter > 0) {
        (*counter)--;
        return 0;
    }
    return 1;
}

uint8_t ia_range_next(uint8_t high, uint16_t adc_ia, uint8_t ia_error)
{
    if (!high && adc_ia > IA_RANGE_UP_ADC)
        return 1;
    if (high && !ia_error && adc_ia < IA_RANGE_DOWN_ADC)
        return 0;
    return high;
}

uint8_t heater_regulate(uint8_t pwm, uint16_t uh_set, uint16_t ih_set,
                        uint16_t sum_uh, uint16_t sum_ih)
{
    if (uh_set == 0 && ih_set == 0)
        return 0;   // fast switch-off

    if (uh_set > 0) {
        uint16_t uh = conv_uh(sum_uh, sum_ih);
        if (uh_set > uh && pwm < 255)
            pwm++;
        if (uh_set < uh && pwm > 0)
            pwm--;
    }
    if (ih_set > 0) {
        uint16_t ih = conv_ih(sum_ih);
        // Rise freely below Uh = 0.5 V; above it only while current flows
        // (Ih > 5 mA), so an open heater does not get full voltage.
        if (ih_set > ih && (pwm < 8 || (sum_ih > 32 && pwm < 255)))
            pwm++;
        if (ih_set < ih && pwm > 0)
            pwm--;
    }
    return pwm;
}
