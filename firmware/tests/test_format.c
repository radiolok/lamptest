#include "check.h"
#include "format.h"

static void digits(void)
{
    char d[4];
    fmt_digits(1234, d);
    CHECK_MEM(d, "4321");
    fmt_digits(7, d);
    CHECK_MEM(d, "7000");
    fmt_digits(65535, d);   // only the 4 lowest digits
    CHECK_MEM(d, "5355");
}

static void fixed_examples(void)
{
    char b[8];
    fmt_fixed(126, 2, 1, b);  CHECK_MEM(b, "12.6");
    fmt_fixed(5, 2, 1, b);    CHECK_MEM(b, " 0.5");
    fmt_fixed(0, 2, 1, b);    CHECK_MEM(b, " 0.0");
    fmt_fixed(100, 3, 0, b);  CHECK_MEM(b, "100");
    fmt_fixed(5, 3, 0, b);    CHECK_MEM(b, "  5");
    fmt_fixed(0, 3, 0, b);    CHECK_MEM(b, "  0");
    fmt_fixed(1999, 2, 2, b); CHECK_MEM(b, "19.99");
    fmt_fixed(1999, 3, 1, b); CHECK_MEM(b, "199.9");
    fmt_fixed(7, 2, 2, b);    CHECK_MEM(b, " 0.07");
    fmt_fixed(2500, 4, 0, b); CHECK_MEM(b, "2500");
    fmt_fixed(50, 4, 0, b);   CHECK_MEM(b, "  50");
    fmt_fixed(1005, 3, 0, b); CHECK_MEM(b, "  5");   // overflow keeps the low digits
}

static void fixed_writes_exact_width(void)
{
    static const struct { uint8_t integral, frac, width; } f[] = {
        { 2, 1, 4 }, { 3, 0, 3 }, { 4, 0, 4 }, { 2, 2, 5 }, { 3, 1, 5 },
    };
    for (unsigned i = 0; i < sizeof(f) / sizeof(f[0]); i++) {
        char b[8];
        memset(b, '#', sizeof(b));
        fmt_fixed(0, f[i].integral, f[i].frac, b);
        CHECK(b[f[i].width - 1] != '#');
        CHECK_EQ(b[f[i].width], '#');
    }
}

// Every value of every format against printf
static void fixed_matches_printf(void)
{
    static const struct { uint8_t integral, frac; } f[] = {
        { 2, 1 }, { 3, 0 }, { 4, 0 }, { 2, 2 }, { 3, 1 },
    };
    for (unsigned i = 0; i < sizeof(f) / sizeof(f[0]); i++) {
        unsigned scale = f[i].frac == 2 ? 100 : f[i].frac == 1 ? 10 : 1;
        unsigned limit = 1;
        for (int d = 0; d < f[i].integral + f[i].frac; d++)
            limit *= 10;
        int bad = 0;
        for (unsigned v = 0; v < limit; v++) {
            char got[8] = { 0 }, want[2 * 256 + 8];     // room for any width
            fmt_fixed((uint16_t)v, f[i].integral, f[i].frac, got);
            if (f[i].frac)
                snprintf(want, sizeof(want), "%*u.%0*u", f[i].integral, v / scale,
                         f[i].frac, v % scale);
            else
                snprintf(want, sizeof(want), "%*u", f[i].integral, v);
            if (strcmp(got, want) != 0 && bad++ < 3)
                printf("fmt_fixed(%u, %u, %u) = \"%s\", expected \"%s\"\n",
                       v, f[i].integral, f[i].frac, got, want);
        }
        CHECK_EQ(bad, 0);
    }
}

int main(void)
{
    RUN_TEST(digits);
    RUN_TEST(fixed_examples);
    RUN_TEST(fixed_writes_exact_width);
    RUN_TEST(fixed_matches_printf);
    return check_summary("format");
}
