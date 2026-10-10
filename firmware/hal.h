#ifndef HAL_H
#define HAL_H

// Hardware access used by the logic modules. On the AVR these are inline
// register accesses from board.h; the host tests link fakes instead.

#include <stdint.h>

#if defined(__AVR__)

#include "board.h"

#else

void     hal_adc_select(uint8_t channel);
void     hal_ug1_clock(uint8_t on);        // Ug1 charge pump clock line
uint16_t hal_pwm_ua(void);
void     hal_set_pwm_ua(uint16_t value);
uint16_t hal_pwm_ug2(void);
void     hal_set_pwm_ug2(uint16_t value);
void     hal_heater_pwm(uint8_t value);    // 0 also disconnects OC0
void     hal_ia_range(uint8_t high);       // 0: 20 mA, 1: 200 mA shunt
void     hal_anode_select(uint8_t second); // 0: anode 1, 1: anode 2
void     hal_beeper(uint8_t on);

// Host builds are single threaded
#define ATOMIC_BLOCK(type)  for (uint8_t atomic_once_ = 1; atomic_once_; atomic_once_ = 0)
#define ATOMIC_RESTORESTATE 0

#endif

#endif
