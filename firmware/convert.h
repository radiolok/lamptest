#ifndef CONVERT_H
#define CONVERT_H

#include <stdint.h>

// Conversion of the ADC averages (sums of 64 samples) to physical units.
// All results use the display units: Uh 0.1 V, Ih 10 mA, Ug1 0.1 V
// (magnitude of the negative bias), Ua and Ug2 1 V, Ia and Ig2 0.01 mA.

uint16_t conv_uh(uint16_t sum_uh, uint16_t sum_ih);    // minus shunt drop
uint16_t conv_ih(uint16_t sum_ih);
uint16_t conv_ug1(uint16_t sum_ug1);
uint16_t conv_ua(uint16_t sum_ua);                      // also Ug2
uint16_t conv_ia(uint16_t sum_ia, uint16_t sum_ua, uint8_t range_high);
uint16_t conv_ig2(uint16_t sum_ig2, uint16_t sum_ug2);

// Single-sample ADC code of the Ug1 divider for a bias of ug1 (0.1 V)
uint16_t conv_ug1_to_adc(uint16_t ug1);

// Transconductance S = dIa / dUg1 in 0.1 mA/V.
// ia_* are in 0.01 mA, ug_* in 0.1 V; *_r is the more negative grid.
uint16_t calc_s(uint16_t ia_l, uint16_t ia_r, uint16_t ug_l, uint16_t ug_r);

// Internal resistance R = dUa / dIa in 0.1 kOhm (ua in V, ia in 0.01 mA)
uint16_t calc_r(uint16_t ua_l, uint16_t ua_r, uint16_t ia_l, uint16_t ia_r);

// Amplification factor K = S * R in 0.1
uint16_t calc_k(uint16_t s, uint16_t r);

#endif
