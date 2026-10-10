#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

// Encoder push button, sampled every 1 ms.
//
// A press longer than BTN_LONG makes the button "held": turning the knob
// then changes the value under the cursor. A shorter press is reported as a
// click once the button has been released for BTN_DEBOUNCE.

void button_init(void);

// Returns 1 on a click
uint8_t button_poll(uint8_t pressed);

uint8_t button_held(void);
uint8_t button_released(void);

#endif
