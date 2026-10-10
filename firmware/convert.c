#include "config.h"
#include "convert.h"

// Anode and screen voltage dividers
#define HV_DIV          107436UL
// Current drawn by the high-side voltage divider, subtracted from Ia / Ig2
#define HV_LEAK_DIV     4369064UL

uint16_t conv_uh(uint16_t sum_uh, uint16_t sum_ih)
{
    uint32_t uh = ((uint32_t)sum_uh * VREF) >> 14;      // 0..200
    uint32_t drop = ((uint32_t)sum_ih * VREF) >> 16;    // shunt voltage drop
    uh = (uh > drop) ? uh - drop : 0;
    return (uint16_t)(uh / 10);
}

uint16_t conv_ih(uint16_t sum_ih)
{
    return (uint16_t)(((uint32_t)sum_ih * VREF) >> 15); // 0..250
}

uint16_t conv_ug1(uint16_t sum_ug1)
{
    // The divider sees -Ug1 offset against +Vref: 0 V reads 640000/725
    uint32_t t = (uint32_t)sum_ug1 * 725;
    uint32_t ug = (40960000UL > t) ? 40960000UL - t : 0;
    ug >>= 16;
    ug *= VREF;
    return (uint16_t)(ug / 1000);
}

uint16_t conv_ua(uint16_t sum_ua)
{
    return (uint16_t)((uint32_t)sum_ua * VREF / HV_DIV);
}

uint16_t conv_ia(uint16_t sum_ia, uint16_t sum_ua, uint8_t range_high)
{
    uint32_t ia = ((uint32_t)sum_ia * VREF) >> 14;
    uint32_t leak = (uint32_t)sum_ua * VREF;
    if (!range_high)
        leak *= 10;
    leak /= HV_LEAK_DIV;
    return (uint16_t)((ia > leak) ? ia - leak : 0);
}

uint16_t conv_ig2(uint16_t sum_ig2, uint16_t sum_ug2)
{
    uint32_t ig = ((uint32_t)sum_ig2 * VREF) >> 13;     // 40 mA range
    uint32_t leak = (uint32_t)sum_ug2 * VREF * 10 / HV_LEAK_DIV;
    return (uint16_t)((ig > leak) ? ig - leak : 0);
}

uint16_t conv_ug1_to_adc(uint16_t ug1)
{
    uint32_t code = 640000UL * VREF - 1024000UL * ug1;
    return (uint16_t)(code / 725 / VREF);               // 882 (0 V) .. 216 (-24 V)
}

uint16_t calc_s(uint16_t ia_l, uint16_t ia_r, uint16_t ug_l, uint16_t ug_r)
{
    if (ia_l == ia_r || ug_l == ug_r)
        return RESULT_MAX;
    uint16_t dia = ia_l - ia_r;
    uint16_t dug = ug_r - ug_l;
    return dia / dug;
}

uint16_t calc_r(uint16_t ua_l, uint16_t ua_r, uint16_t ia_l, uint16_t ia_r)
{
    if (ia_l == ia_r)
        return RESULT_MAX;
    uint16_t dua = (uint16_t)((uint16_t)(ua_r - ua_l) * 1000u);
    uint16_t dia = ia_r - ia_l;
    return dua / dia;
}

uint16_t calc_k(uint16_t s, uint16_t r)
{
    uint32_t k = ((uint32_t)s * r + 5) / 10;
    return (k < RESULT_MAX) ? (uint16_t)k : RESULT_MAX;
}
