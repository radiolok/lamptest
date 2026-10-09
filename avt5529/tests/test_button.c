#include "button.h"
#include "check.h"
#include "config.h"
#include "test_env.h"

static unsigned clicks_for(uint8_t pressed, unsigned ms)
{
    unsigned n = 0;
    while (ms--)
        n += button_poll(pressed);
    return n;
}

static void short_press_clicks_once(void)
{
    CHECK_EQ(clicks_for(0, 50), 0);
    CHECK_EQ(clicks_for(1, 100), 0);
    CHECK_EQ(button_held(), 0);
    CHECK_EQ(clicks_for(0, 50), 1);
}

static void bounce_is_ignored(void)
{
    CHECK_EQ(clicks_for(0, 50), 0);
    CHECK_EQ(clicks_for(1, BTN_DEBOUNCE), 0);
    CHECK_EQ(clicks_for(0, 50), 0);
}

static void long_press_holds_without_click(void)
{
    CHECK_EQ(clicks_for(0, 50), 0);
    CHECK_EQ(clicks_for(1, BTN_LONG - 1), 0);
    CHECK_EQ(button_held(), 0);
    CHECK_EQ(clicks_for(1, 1), 0);
    CHECK_EQ(button_held(), 1);
    CHECK_EQ(clicks_for(1, 1000), 0);
    CHECK_EQ(button_released(), 0);

    // Released: debounced, then no longer held
    CHECK_EQ(clicks_for(0, BTN_DEBOUNCE), 0);
    CHECK_EQ(button_released(), 1);
    CHECK_EQ(button_held(), 1);     // for one more tick
    CHECK_EQ(clicks_for(0, 1), 0);
    CHECK_EQ(button_held(), 0);
}

static void released_at_power_up_after_debounce(void)
{
    CHECK_EQ(button_released(), 0);
    button_for(0, BTN_DEBOUNCE);
    CHECK_EQ(button_released(), 1);
}

int main(void)
{
    RUN_TEST(short_press_clicks_once);
    RUN_TEST(bounce_is_ignored);
    RUN_TEST(long_press_holds_without_click);
    RUN_TEST(released_at_power_up_after_debounce);
    return check_summary("button");
}
