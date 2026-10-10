#include "check.h"
#include "config.h"
#include "convert.h"
#include "legacy.h"

#define SUM_MAX (64u * 1023u)   // largest sum of 64 samples

static void single_input_conversions_match_legacy(void)
{
    int bad = 0;
    for (uint32_t m = 0; m <= 0xFFFF; m++) {
        bad += conv_ug1(m) != legacy_ug1(m);
        bad += conv_ih(m) != legacy_ih(m);
        bad += conv_ua(m) != legacy_ua(m);
    }
    CHECK_EQ(bad, 0);
}

static void two_input_conversions_match_legacy(void)
{
    int bad = 0;
    for (uint32_t a = 0; a <= SUM_MAX; a += 7) {
        for (uint32_t b = 0; b <= SUM_MAX; b += 1009) {
            bad += conv_uh(a, b) != legacy_uh(a, b);
            bad += conv_ia(a, b, 0) != legacy_ia(a, b, 0);
            bad += conv_ia(a, b, 1) != legacy_ia(a, b, 1);
            bad += conv_ig2(a, b) != legacy_ig2(a, b);
        }
    }
    CHECK_EQ(bad, 0);
}

static void ug1_to_adc(void)
{
    for (uint16_t u = 0; u <= UG1_MAX; u++)
        CHECK_EQ(conv_ug1_to_adc(u), legacy_liczug1(u));
    CHECK_EQ(conv_ug1_to_adc(0), 882);
    CHECK_EQ(conv_ug1_to_adc(UG1_MAX), 216);

    // A set point read back through the measurement is within 0.1 V
    for (uint16_t u = 0; u <= UG1_MAX; u++) {
        int back = conv_ug1(64 * conv_ug1_to_adc(u));
        CHECK(back >= u - 1 && back <= u + 1);
    }
}

static void physical_examples(void)
{
    // 6.3 V heater needs 630 << 14 / 509 = 20279
    CHECK_EQ(conv_uh(20279, 0), 63);
    CHECK_EQ(conv_ih(64 * 400), 397);       // trip level: ~4 A
    CHECK_EQ(conv_ua(SUM_MAX), 310);        // full scale ~310 V
    CHECK_EQ(conv_ia(SUM_MAX, 0, 0), 2034); // 20 mA range: 20.34 mA
}

static void s_r_k_match_legacy(void)
{
    int bad = 0;
    for (int i = 0; i < 200000; i++) {
        uint16_t a = xorshift(), b = xorshift(), c = xorshift(), d = xorshift();
        if (i & 1) {    // realistic magnitudes
            a %= 20000; b %= 20000; c %= 240; d %= 240;
        }
        if (c != d)
            bad += calc_s(a, b, c, d) != legacy_s(a, b, c, d);
        bad += calc_r(c, d, a, b) != legacy_r(c, d, a, b);
        bad += calc_k(a % 1000, b % 1000) != legacy_k(a % 1000, b % 1000);
    }
    CHECK_EQ(bad, 0);
}

static void s_r_k_examples(void)
{
    // ECC83: Ia 1.20 -> 1.42 mA for Ug -2.2 -> -1.0 V (0.1 V units)
    CHECK_EQ(calc_s(142, 120, 10, 22), 1);      // 0.1 mA/V resolution
    CHECK_EQ(calc_s(1050, 980, 74, 96), 3);
    CHECK_EQ(calc_s(100, 100, 74, 96), RESULT_MAX);
    CHECK_EQ(calc_s(120, 100, 85, 85), RESULT_MAX);   // was a division by zero
    // R: 20 V for 2.60 mA -> 7.6 kOhm
    CHECK_EQ(calc_r(240, 260, 920, 1180), 76);
    CHECK_EQ(calc_r(240, 260, 920, 920), RESULT_MAX);
    CHECK_EQ(calc_k(22, 76), 167);
    CHECK_EQ(calc_k(500, 500), RESULT_MAX);
}

int main(void)
{
    RUN_TEST(single_input_conversions_match_legacy);
    RUN_TEST(two_input_conversions_match_legacy);
    RUN_TEST(ug1_to_adc);
    RUN_TEST(physical_examples);
    RUN_TEST(s_r_k_match_legacy);
    RUN_TEST(s_r_k_examples);
    return check_summary("convert");
}
