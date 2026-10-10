#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>

// Pure control laws used by the ADC scan

// One step of `value` toward `target`
uint16_t ramp_step(uint16_t value, uint16_t target);

// Over-current filter. Call once per sample; returns 1 once `over` has been
// seen on TRIP_SAMPLES + 1 consecutive samples.
uint8_t trip_check(uint8_t *counter, uint8_t over);

// Ia shunt auto-range with hysteresis. Never switches down while the
// anode over-current error is set.
uint8_t ia_range_next(uint8_t high, uint16_t adc_ia, uint8_t ia_error);

// One regulation step of the heater PWM, once per averaged block.
// Regulates Uh when uh_set != 0 and/or Ih when ih_set != 0.
uint8_t heater_regulate(uint8_t pwm, uint16_t uh_set, uint16_t ih_set,
                        uint16_t sum_uh, uint16_t sum_ih);

#endif
