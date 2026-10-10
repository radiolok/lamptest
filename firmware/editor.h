#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>

// Encoder handling, one call per detent.
//
// Idle:    with the button held, change the value under the cursor;
//          otherwise move the cursor. Only the power supply (slot 0) and
//          user slots have fields beyond the slot number.
// Holding: with the button held, switch to the other section of a twin
//          tube; otherwise switch the heater off.
// Running: turning the knob aborts the measurement.
void editor_on_encoder(uint8_t right);

#endif
