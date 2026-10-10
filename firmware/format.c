#include "format.h"

void fmt_digits(uint16_t num, char digits[4])
{
    for (uint8_t i = 0; i < 4; i++) {
        digits[i] = (char)('0' + num % 10);
        num /= 10;
    }
}

void fmt_fixed(uint16_t num, uint8_t integral, uint8_t frac, char *to)
{
    char digits[4];
    fmt_digits(num, digits);

    const char *from = &digits[integral + frac - 1];
    uint8_t leading = 1;
    for (; integral > 0; integral--) {
        if (leading && integral > 1 && *from == '0') {
            *to++ = ' ';
        } else {
            *to++ = *from;
            leading = 0;
        }
        from--;
    }
    if (frac != 0) {
        *to++ = '.';
        for (; frac > 0; frac--)
            *to++ = *from--;
    }
}
