#include "app.h"
#include "button.h"
#include "check.h"
#include "editor.h"
#include "lampdb.h"
#include "panel.h"
#include "sequencer.h"
#include "test_env.h"

#define USER_SLOT   85
#define FLASH_SLOT  4       // ECC82_G11
#define RIGHT       1
#define LEFT        0

static void turn(uint8_t right, int n)
{
    while (n--)
        editor_on_encoder(right);
}

static void supply_cursor_visits_output_fields(void)
{
    select_lamp(LAMP_SUPPLY);
    button_release();
    static const uint8_t path[] = { F_UG1, F_UH, F_IH, F_UA, F_UG2, F_UG2 };
    for (unsigned i = 0; i < sizeof(path); i++) {
        turn(RIGHT, 1);
        CHECK_EQ(field, path[i]);
    }
    static const uint8_t back[] = { F_UA, F_IH, F_UH, F_UG1 };
    for (unsigned i = 0; i < sizeof(back); i++) {
        turn(LEFT, 1);
        CHECK_EQ(field, back[i]);
    }
    CHECK_EQ(seq_now(), SEQ_IDLE);
    turn(LEFT, 1);
    CHECK_EQ(field, F_LAMP);
    CHECK_EQ(seq_now(), SEQ_REPORT);    // outputs are switched off
}

static void user_cursor_visits_every_field(void)
{
    select_lamp(USER_SLOT);
    button_release();
    for (int f = 1; f < F_COUNT; f++) {
        turn(RIGHT, 1);
        CHECK_EQ(field, f);
    }
    turn(RIGHT, 1);
    CHECK_EQ(field, F_K);
    turn(LEFT, F_COUNT + 5);
    CHECK_EQ(field, F_LAMP);
}

static void flash_slot_has_no_fields(void)
{
    select_lamp(FLASH_SLOT);
    button_release();
    turn(RIGHT, 3);
    CHECK_EQ(field, F_LAMP);
    turn(LEFT, 3);
    CHECK_EQ(field, F_LAMP);
}

static void slot_number_wraps(void)
{
    select_lamp(LAMP_COUNT - 2);
    button_hold();
    turn(RIGHT, 1);
    CHECK_EQ(lamp_num, LAMP_COUNT - 1);
    turn(RIGHT, 1);
    CHECK_EQ(lamp_num, 0);
    turn(LEFT, 1);
    CHECK_EQ(lamp_num, LAMP_COUNT - 1);
    CHECK_EQ(field, F_LAMP);
}

static void values_stay_in_range(void)
{
    select_lamp(USER_SLOT);
    field = F_UA;
    lamp.uadef = 285;
    button_hold();
    turn(RIGHT, 20);
    CHECK_EQ(lamp.uadef, 290);
    turn(LEFT, 400);
    CHECK_EQ(lamp.uadef, 10);
    CHECK_EQ(field, F_UA);      // held: the cursor does not move

    field = F_SECTION;
    turn(RIGHT, 5);
    CHECK_EQ(lamp.name[NAME_SECTION], AZ_DIGIT0 + 2);
    turn(LEFT, 5);
    CHECK_EQ(lamp.name[NAME_SECTION], AZ_DIGIT0);

    field = F_NAME;
    lamp.name[0] = 0;
    turn(LEFT, 1);
    CHECK_EQ(lamp.name[0], 0);
    turn(RIGHT, 50);
    CHECK_EQ(lamp.name[0], AZ_COUNT - 1);
}

static void supply_allows_full_range(void)
{
    select_lamp(LAMP_SUPPLY);
    field = F_UG1;
    lamp.ug1def = 3;
    button_hold();
    turn(LEFT, 5);
    CHECK_EQ(lamp.ug1def, 0);
    turn(RIGHT, 300);
    CHECK_EQ(lamp.ug1def, UG1_MAX);
}

static void turning_aborts_a_measurement(void)
{
    select_lamp(FLASH_SLOT);
    seq_click();
    uint16_t t = seq_now();

    button_hold();
    turn(RIGHT, 1);     // held: ignored
    CHECK_EQ(seq_now(), t);

    button_release();
    turn(RIGHT, 1);
    CHECK_EQ(seq_now(), SEQ_REPORT);
    CHECK_EQ(lamp_num, FLASH_SLOT);
}

static void hold_section_switch(void)
{
    select_lamp(FLASH_SLOT);
    seq_click();
    seq_run_until(SEQ_HOLD, 3000);
    CHECK(seq_holding());

    button_hold();
    turn(LEFT, 1);      // section 1 has no previous section
    CHECK_EQ(lamp_num, FLASH_SLOT);
    turn(RIGHT, 1);
    CHECK_EQ(lamp_num, FLASH_SLOT + 1);
    turn(RIGHT, 1);     // not again until the record is reloaded
    CHECK_EQ(lamp_num, FLASH_SLOT + 1);
    CHECK(seq_holding());

    panel_update();     // main loop loads section 2
    turn(LEFT, 1);
    CHECK_EQ(lamp_num, FLASH_SLOT);

    button_release();
    turn(RIGHT, 1);
    CHECK_EQ(seq_now(), SEQ_HEATER_OFF);
}

int main(void)
{
    RUN_TEST(supply_cursor_visits_output_fields);
    RUN_TEST(user_cursor_visits_every_field);
    RUN_TEST(flash_slot_has_no_fields);
    RUN_TEST(slot_number_wraps);
    RUN_TEST(values_stay_in_range);
    RUN_TEST(supply_allows_full_range);
    RUN_TEST(turning_aborts_a_measurement);
    RUN_TEST(hold_section_switch);
    return check_summary("editor");
}
