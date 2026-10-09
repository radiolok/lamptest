#include "editor.h"
#include "app.h"
#include "button.h"
#include "sequencer.h"

static void edit_value(uint8_t right)
{
    if (field == F_LAMP) {
        // The slot number wraps around
        if (right)
            lamp_num = (lamp_num < LAMP_COUNT - 1) ? lamp_num + 1 : 0;
        else
            lamp_num = (lamp_num > 0) ? lamp_num - 1 : LAMP_COUNT - 1;
        return;
    }

    uint16_t min, max;
    lamp_field_range(lamp_num, field, &min, &max);
    uint16_t value = lamp_field_get(&lamp, field);
    if (right && value < max)
        value++;
    if (!right && value > min)
        value--;
    lamp_field_set(&lamp, field, value);
}

static void move_cursor(uint8_t right)
{
    if (right) {
        if (lamp_num == LAMP_SUPPLY) {
            // Power supply: slot, Ug1, Uh, Ih, Ua, Ug2
            if (field == F_LAMP)
                field = F_UG1 - 1;
            if (field == F_UA)
                field++;
            if (field < F_UG2)
                field++;
        }
        if (lamp_is_user(lamp_num) && field < F_K)
            field++;
    } else if (field > F_LAMP) {
        if (lamp_num == LAMP_SUPPLY) {
            if (field == F_UG1) {
                // Back to the slot number: switch all outputs off
                field = F_LAMP + 1;
                seq_abort(0);
            }
            if (field == F_UG2)
                field--;
        }
        if (lamp_num == LAMP_SUPPLY || lamp_is_user(lamp_num))
            field--;
    }
}

static void switch_section(uint8_t right)
{
    if (!lamp_fresh)
        return;     // wait until the main loop has loaded the new record
    uint8_t section = lamp_section(&lamp);
    if (right && section == 1) {
        lamp_num++;
        lamp_fresh = 0;
    }
    if (!right && section == 2) {
        lamp_num--;
        lamp_fresh = 0;
    }
}

void editor_on_encoder(uint8_t right)
{
    uint16_t t = seq_now();

    if (t > SEQ_REPORT) {
        if (button_released())
            seq_abort(0);
    } else if (t == SEQ_HOLD) {
        if (button_held()) {
            switch_section(right);
            seq_clear_results();
        }
        if (button_released())
            seq_release();
    } else if (t == SEQ_IDLE) {
        if (button_held())
            edit_value(right);
        if (button_released())
            move_cursor(right);
    }
}
