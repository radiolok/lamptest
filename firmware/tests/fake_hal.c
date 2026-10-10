#include <string.h>
#include "fake_hal.h"

fake_hal_t fake;
unsigned fake_eeprom_writes;

void fake_hal_reset(void)
{
    memset(&fake, 0, sizeof(fake));
    fake_eeprom_writes = 0;
}

void hal_adc_select(uint8_t channel)        { fake.adc_mux = channel; }
void hal_ug1_clock(uint8_t on)              { fake.ug1_clock = on; }
uint16_t hal_pwm_ua(void)                   { return fake.pwm_ua; }
void hal_set_pwm_ua(uint16_t value)         { fake.pwm_ua = value; }
uint16_t hal_pwm_ug2(void)                  { return fake.pwm_ug2; }
void hal_set_pwm_ug2(uint16_t value)        { fake.pwm_ug2 = value; }
void hal_heater_pwm(uint8_t value)          { fake.heater_pwm = value; }
void hal_ia_range(uint8_t high)             { fake.ia_range = high; }
void hal_anode_select(uint8_t second)       { fake.anode2 = second; }
void hal_beeper(uint8_t on)                 { fake.beeper = on; }
