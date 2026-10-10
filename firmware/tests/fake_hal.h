#ifndef FAKE_HAL_H
#define FAKE_HAL_H

#include <stdint.h>
#include "hal.h"

// State of the simulated outputs
typedef struct {
    uint8_t  adc_mux;
    uint8_t  ug1_clock;
    uint16_t pwm_ua;
    uint16_t pwm_ug2;
    uint8_t  heater_pwm;
    uint8_t  ia_range;
    uint8_t  anode2;
    uint8_t  beeper;
} fake_hal_t;

extern fake_hal_t fake;

// Number of EEPROM bytes or words changed (see fakes/avr/eeprom.h)
extern unsigned fake_eeprom_writes;

void fake_hal_reset(void);

#endif
