#include "button.h"
#include "config.h"

static uint8_t pressed_ms;      // saturates at BTN_LONG
static uint8_t released_ms;     // saturates at BTN_DEBOUNCE

void button_init(void)
{
    pressed_ms = 0;
    released_ms = 0;
}

uint8_t button_poll(uint8_t pressed)
{
    uint8_t click = 0;

    if (pressed) {
        if (pressed_ms < BTN_LONG)
            pressed_ms++;
        else
            released_ms = 0;
    } else if (released_ms < BTN_DEBOUNCE) {
        released_ms++;
    } else {
        click = (pressed_ms > BTN_DEBOUNCE && pressed_ms < BTN_LONG);
        pressed_ms = 0;
    }
    return click;
}

uint8_t button_held(void)
{
    return pressed_ms == BTN_LONG;
}

uint8_t button_released(void)
{
    return released_ms == BTN_DEBOUNCE;
}
