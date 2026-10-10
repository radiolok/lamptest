#ifndef LEGACY_H
#define LEGACY_H

// Formulas copied verbatim from the original avt5229.c (main loop, ADC
// interrupt and sequencer), with `unsigned int` spelled uint16_t because
// int is 16 bits on the AVR. The refactored code must match them exactly.

#include <stdint.h>

static const uint16_t vref = 509;

static inline uint16_t legacy_liczug1(uint16_t pug1)
{
    uint32_t licz, temp;
    licz = 640000;
    licz *= vref;
    temp = 1024000;
    temp *= pug1;
    licz -= temp;
    licz /= 725;
    licz /= vref;
    return (uint16_t)licz;
}

static inline uint16_t legacy_ug1(uint16_t mug1adc)
{
    uint32_t licz, temp;
    temp = mug1adc;
    temp *= 725;
    licz = 40960000;
    if (licz > temp)
        licz -= temp;
    else
        licz = 0;
    licz >>= 16;
    licz *= vref;
    licz /= 1000;
    return (uint16_t)licz;
}

static inline uint16_t legacy_uh(uint16_t muhadc, uint16_t mihadc)
{
    uint32_t licz, temp;
    licz = muhadc;
    licz *= vref;
    licz >>= 14;
    temp = mihadc;
    temp *= vref;
    temp >>= 16;
    if (licz > temp)
        licz -= temp;
    else
        licz = 0;
    licz /= 10;
    return (uint16_t)licz;
}

static inline uint16_t legacy_ih(uint16_t mihadc)
{
    uint32_t licz = mihadc;
    licz *= vref;
    licz >>= 15;
    return (uint16_t)licz;
}

static inline uint16_t legacy_ua(uint16_t muaadc)
{
    uint32_t licz = muaadc;
    licz *= vref;
    licz /= 107436;
    return (uint16_t)licz;
}

static inline uint16_t legacy_ia(uint16_t miaadc, uint16_t muaadc, uint8_t range)
{
    uint32_t licz, temp;
    licz = miaadc;
    licz *= vref;
    licz >>= 14;
    temp = muaadc;
    temp *= vref;
    if (range == 0)
        temp *= 10;
    temp /= 4369064;
    if (licz > temp)
        licz -= temp;
    else
        licz = 0;
    return (uint16_t)licz;
}

static inline uint16_t legacy_ig2(uint16_t mig2adc, uint16_t mug2adc)
{
    uint32_t licz, temp;
    licz = mig2adc;
    licz *= vref;
    licz >>= 13;
    temp = mug2adc;
    temp *= vref;
    temp *= 10;
    temp /= 4369064;
    if (licz > temp)
        licz -= temp;
    else
        licz = 0;
    return (uint16_t)licz;
}

// The original divides by zero when ugr == ugl; callers avoid that case
static inline uint16_t legacy_s(uint16_t ial, uint16_t iar, uint16_t ugl, uint16_t ugr)
{
    uint16_t s;
    if (iar != ial) {
        ial -= iar;
        ugr -= ugl;
        s = ial;
        s /= ugr;
    } else
        s = 999;
    return s;
}

static inline uint16_t legacy_r(uint16_t ual, uint16_t uar, uint16_t ial, uint16_t iar)
{
    uint16_t r;
    if (iar != ial) {
        uar -= ual;
        uar *= 1000;
        iar -= ial;
        r = uar;
        r /= iar;
    } else
        r = 999;
    return r;
}

static inline uint16_t legacy_k(uint16_t s, uint16_t r)
{
    uint32_t lint = s;
    lint *= r;
    lint += 5;
    lint /= 10;
    return (lint < 999) ? (uint16_t)lint : 999;
}

static inline uint8_t legacy_heater(uint8_t pwm, uint16_t uhset, uint16_t ihset,
                                    uint16_t muhadc, uint16_t mihadc)
{
    uint32_t lint, tint;
    if ((uhset == 0) && (ihset == 0))
        pwm = 0;
    else {
        if (uhset > 0) {
            lint = muhadc;
            lint *= vref;
            lint >>= 14;
            tint = mihadc;
            tint *= vref;
            tint >>= 16;
            if (lint > tint)
                lint -= tint;
            else
                lint = 0;
            lint /= 10;
            if ((uhset > (uint16_t)lint) && (pwm < 255))
                pwm++;
            if ((uhset < (uint16_t)lint) && (pwm > 0))
                pwm--;
        }
        if (ihset > 0) {
            lint = mihadc;
            lint *= vref;
            lint >>= 15;
            if ((ihset > (uint16_t)lint)) {
                if ((pwm < 8) || ((mihadc > 32) && (pwm < 255)))
                    pwm++;
            }
            if ((ihset < (uint16_t)lint) && (pwm > 0))
                pwm--;
        }
    }
    return pwm;
}

// Deterministic pseudo-random numbers for the comparisons
static inline uint32_t xorshift(void)
{
    static uint32_t x = 2463534242u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

#endif
