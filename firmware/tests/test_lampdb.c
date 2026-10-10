#include <ctype.h>
#include "check.h"
#include "config.h"
#include "lamp.h"
#include "lampdb.h"

static void check_record(uint8_t num, const lamp_t *l)
{
    char name[10] = { 0 };
    for (int i = 0; i < 9; i++)
        name[i] = lamp_name_char(num, l, i);

    int ok = 1;
    for (int i = 0; i < 9; i++)
        ok &= isprint((unsigned char)name[i]) && name[i] != '?';
    if (num >= LAMP_FIRST_TUBE) {
        ok &= name[NAME_SOCKET] >= 'A' && name[NAME_SOCKET] <= 'J';
        ok &= name[NAME_SECTION] >= '0' && name[NAME_SECTION] <= '2';
        ok &= name[NAME_WARMUP] >= '1' && name[NAME_WARMUP] <= '9';
        ok &= (l->uhdef != 0) != (l->ihdef != 0);   // Uh or Ih, not both
        ok &= l->ug1def <= UG1_MAX && l->uhdef <= 150 && l->ihdef <= 250;
        ok &= l->uadef <= 300 && l->ug2def <= 300;
        ok &= lamp_warmup_steps(num, l) == (name[NAME_WARMUP] - '0') * WARMUP_TICKS_PER_MIN;
        ok &= lamp_section(l) == name[NAME_SECTION] - '0';
    }
    if (!ok)
        printf("slot %u: bad record \"%s\"\n", num, name);
    CHECK(ok);
}

static void all_records_valid(void)
{
    for (uint8_t n = 0; n < LAMP_COUNT; n++) {
        lamp_t l;
        lampdb_load(n, &l);
        check_record(n, &l);
    }
}

// Switching sections while holding steps to the neighbouring slot
static void twin_sections_are_adjacent(void)
{
    for (uint8_t n = LAMP_FIRST_TUBE; n < LAMP_COUNT; n++) {
        lamp_t a, b;
        lampdb_load(n, &a);
        if (lamp_section(&a) != 1)
            continue;
        int ok = n + 1 < LAMP_COUNT;
        if (ok) {
            lampdb_load(n + 1, &b);
            ok = lamp_section(&b) == 2 && lamp_is_user(n) == lamp_is_user(n + 1);
        }
        if (!ok)
            printf("slot %u: section 1 without section 2 after it\n", n);
        CHECK(ok);
    }
}

static void known_records(void)
{
    lamp_t l;
    lampdb_load(0, &l);
    CHECK_MEM(l.name, "PwrSupply");
    lampdb_load(4, &l);
    CHECK_MEM(l.name, "ECC82_G11");
    CHECK_EQ(l.uhdef, 126);
    CHECK_EQ(l.ug1def, 85);
    CHECK_EQ(l.uadef, 250);
    CHECK_EQ(l.iadef, 105);
    CHECK_EQ(l.kdef, 170);
    lampdb_load(LAMP_FLASH_COUNT, &l);
    CHECK_EQ(lamp_name_char(LAMP_FLASH_COUNT, &l, 0), '6');    // 6N16S_J11
    CHECK_EQ(lamp_name_char(LAMP_FLASH_COUNT, &l, NAME_SOCKET), 'J');
    CHECK_EQ(lamp_section(&l), 1);
    CHECK_EQ(lamp_warmup_steps(LAMP_FLASH_COUNT, &l), WARMUP_TICKS_PER_MIN);
}

static void record_layout(void)
{
    CHECK_EQ(sizeof(lamp_t), 26);   // EEPROM format
}

static void last_slot(void)
{
    CHECK_EQ(lampdb_last(), LAMP_SUPPLY);
    lampdb_set_last(42);
    CHECK_EQ(lampdb_last(), 42);
    lampdb_set_last(0xFF);          // erased EEPROM
    CHECK_EQ(lampdb_last(), LAMP_SUPPLY);
}

int main(void)
{
    RUN_TEST(all_records_valid);
    RUN_TEST(twin_sections_are_adjacent);
    RUN_TEST(known_records);
    RUN_TEST(record_layout);
    RUN_TEST(last_slot);
    return check_summary("lampdb");
}
