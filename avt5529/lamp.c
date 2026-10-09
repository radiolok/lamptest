#include <stddef.h>
#include <avr/pgmspace.h>
#include "config.h"
#include "lamp.h"

const char AZ[AZ_COUNT] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

char lamp_name_char(uint8_t num, const lamp_t *lamp, uint8_t i)
{
    uint8_t c = lamp->name[i];
    if (!lamp_is_user(num))
        return (char)c;
    return (c < AZ_COUNT) ? AZ[c] : '?';
}

uint8_t lamp_section(const lamp_t *lamp)
{
    // Flash records hold ASCII, user records hold AZ indexes; the two
    // encodings never overlap.
    uint8_t c = lamp->name[NAME_SECTION];
    if (c == '1' || c == AZ_DIGIT0 + 1)
        return 1;
    if (c == '2' || c == AZ_DIGIT0 + 2)
        return 2;
    return 0;
}

uint16_t lamp_warmup_steps(uint8_t num, const lamp_t *lamp)
{
    uint8_t zero = lamp_is_user(num) ? AZ_DIGIT0 : '0';
    return (uint16_t)((uint16_t)(lamp->name[NAME_WARMUP] - zero) * WARMUP_TICKS_PER_MIN);
}

//***** Field table ********************************************************

typedef struct {
    uint8_t  offset;
    uint8_t  is_word;
    uint16_t min, max;
} field_desc_t;

#define BYTE_FIELD(m, lo, hi) { offsetof(lamp_t, m), 0, lo, hi }
#define WORD_FIELD(m, lo, hi) { offsetof(lamp_t, m), 1, lo, hi }

static const field_desc_t fields[F_COUNT] PROGMEM = {
    [F_LAMP]     = { 0, 0, 0, LAMP_COUNT - 1 },
    [F_NAME + 0] = BYTE_FIELD(name[0], 0, AZ_COUNT - 1),
    [F_NAME + 1] = BYTE_FIELD(name[1], 0, AZ_COUNT - 1),
    [F_NAME + 2] = BYTE_FIELD(name[2], 0, AZ_COUNT - 1),
    [F_NAME + 3] = BYTE_FIELD(name[3], 0, AZ_COUNT - 1),
    [F_NAME + 4] = BYTE_FIELD(name[4], 0, AZ_COUNT - 1),
    [F_NAME + 5] = BYTE_FIELD(name[5], 0, AZ_COUNT - 1),
    [F_SOCKET]   = BYTE_FIELD(name[NAME_SOCKET], 0, 9),                         // A..J
    [F_SECTION]  = BYTE_FIELD(name[NAME_SECTION], AZ_DIGIT0, AZ_DIGIT0 + 2),    // 0..2
    [F_WARMUP]   = BYTE_FIELD(name[NAME_WARMUP], AZ_DIGIT0 + 1, AZ_DIGIT0 + 9), // 1..9
    [F_UG1]      = BYTE_FIELD(ug1def, 5, UG1_MAX - 5),  // -0.5..-23.5 V
    [F_UH]       = BYTE_FIELD(uhdef, 0, 150),           // 0..15.0 V
    [F_IH]       = BYTE_FIELD(ihdef, 0, 250),           // 0..2.50 A
    [F_UA]       = WORD_FIELD(uadef, UA_DELTA, 300 - UA_DELTA),
    [F_IA]       = WORD_FIELD(iadef, 0, 2000),          // 0..200.0 mA
    [F_UG2]      = WORD_FIELD(ug2def, 0, 300),          // 0..300 V
    [F_IG2]      = WORD_FIELD(ig2def, 0, 4000),         // 0..40.00 mA
    [F_S]        = WORD_FIELD(sdef, 0, RESULT_MAX),
    [F_R]        = WORD_FIELD(rdef, 0, RESULT_MAX),
    [F_K]        = WORD_FIELD(kdef, 0, RESULT_MAX),
};

static field_desc_t desc(uint8_t field)
{
    field_desc_t d;
    memcpy_P(&d, &fields[field], sizeof(d));
    return d;
}

uint8_t lamp_field_offset(uint8_t field)
{
    return pgm_read_byte(&fields[field].offset);
}

uint8_t lamp_field_is_word(uint8_t field)
{
    return pgm_read_byte(&fields[field].is_word);
}

uint16_t lamp_field_get(const lamp_t *lamp, uint8_t field)
{
    field_desc_t d = desc(field);
    const uint8_t *p = (const uint8_t *)lamp + d.offset;
    if (d.is_word)
        return *(const uint16_t *)p;
    return *p;
}

void lamp_field_set(lamp_t *lamp, uint8_t field, uint16_t value)
{
    field_desc_t d = desc(field);
    uint8_t *p = (uint8_t *)lamp + d.offset;
    if (d.is_word)
        *(uint16_t *)p = value;
    else
        *p = (uint8_t)value;
}

void lamp_field_range(uint8_t num, uint8_t field, uint16_t *min, uint16_t *max)
{
    field_desc_t d = desc(field);
    *min = d.min;
    *max = d.max;
    // The power supply has no S/R measurement, so it needs no headroom
    // for the grid and anode steps.
    if (num == LAMP_SUPPLY) {
        if (field == F_UG1) {
            *min = 0;
            *max = UG1_MAX;
        }
        if (field == F_UA) {
            *min = 0;
            *max = 300;
        }
    }
}
