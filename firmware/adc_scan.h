#ifndef ADC_SCAN_H
#define ADC_SCAN_H

#include <stdint.h>
#include "config.h"

// Free-running ADC scan, driven by the ADC interrupt (~9.6 kHz).
//
// The scan has 14 steps. Even step 2*n reads channel n (Temp, Ih, Uh, Ua,
// Ia, Ug2, Ig2); every odd step reads Ug1 and runs the bang-bang regulator
// of the Ug1 charge pump. Because the ADC is free running, a new channel
// takes effect one conversion after it is selected.
//
// Per sample it also handles the over-current protections, ramps the Ua and
// Ug2 PWM toward their set points and switches the Ia range. Every 64 scans
// it latches the sums as the new averages, regulates the heater and checks
// the heatsink temperature.

void adc_scan_init(void);
void adc_scan_sample(uint16_t adc);

// Snapshot of the latest sums of 64 samples, indexed by enum adc_channel
void adc_averages(uint16_t avg[ADC_CH_COUNT]);

uint8_t adc_ia_range(void);     // 1 = 200 mA range
uint8_t adc_heater_pwm(void);

#endif
