#ifndef BOARD_H
#define BOARD_H

// AVT5229 board: ATmega32 pin assignment and inline hardware access.
//
// Fuses (ICCAVR notation 0xC9EF):
// OCD  JTAG SPI CK  EE   BOOT BOOT BOOT BOD   BOD SU SU CK   CK   CK   CK
// EN    EN  EN  OPT SAVE SZ1  SZ0  RST  LEVEL EN  T1 T0 SEL3 SEL2 SEL1 SEL0
//  1     1   0   0   1    0    0    1    1     1   1  0  1    1    1    1

#include <avr/io.h>
#include <util/atomic.h>
#include "config.h"

#define BIT(x)  (1 << (x))

//***** Port B ************************************************************
#define PB_IA_RANGE     0   // relay: 1 = 200 mA shunt
#define PB_ANODE_SEL    1   // relay: 1 = anode 2
#define PB_UG1_CLK      2   // Ug1 charge pump clock
//      PB3                 // OC0: heater PWM

//***** Port C ************************************************************
#define PC_LCD_RS       2
#define PC_LCD_E        3
#define PC_LCD_DATA     0xF0    // D4..D7

//***** Port D ************************************************************
#define PD_BUTTON       2   // encoder push button, active low
#define PD_ENC_DIR      6   // encoder direction, low = clockwise

//***** PWM outputs *******************************************************
#define PWM_TOP         ICR1    // Ua / Ug2 PWM period
#define PWM_UG2         OCR1A
#define PWM_UA          OCR1B
#define PWM_UH          OCR0

static inline void hal_adc_select(uint8_t channel)
{
    ADMUX = channel;
}

static inline void hal_ug1_clock(uint8_t on)
{
    if (on)
        PORTB |= BIT(PB_UG1_CLK);
    else
        PORTB &= ~BIT(PB_UG1_CLK);
}

static inline uint16_t hal_pwm_ua(void)            { return PWM_UA; }
static inline void hal_set_pwm_ua(uint16_t value)  { PWM_UA = value; }
static inline uint16_t hal_pwm_ug2(void)           { return PWM_UG2; }
static inline void hal_set_pwm_ug2(uint16_t value) { PWM_UG2 = value; }

static inline void hal_heater_pwm(uint8_t value)
{
    if (value == 0)
        TCCR0 &= ~BIT(COM01);   // disconnect OC0
    else
        TCCR0 |= BIT(COM01);    // connect OC0
    PWM_UH = value;
}

static inline void hal_ia_range(uint8_t high)
{
    if (high)
        PORTB |= BIT(PB_IA_RANGE);
    else
        PORTB &= ~BIT(PB_IA_RANGE);
}

static inline void hal_anode_select(uint8_t second)
{
    if (second)
        PORTB |= BIT(PB_ANODE_SEL);
    else
        PORTB &= ~BIT(PB_ANODE_SEL);
}

static inline void hal_beeper(uint8_t on)
{
    // The tone comes from OC2 toggling on every Timer2 compare
    if (on)
        TCCR2 |= BIT(COM20);
    else
        TCCR2 &= ~BIT(COM20);
}

static inline uint8_t hal_button_pressed(void)
{
    return (PIND & BIT(PD_BUTTON)) == 0;
}

static inline uint8_t hal_encoder_right(void)
{
    return (PIND & BIT(PD_ENC_DIR)) == 0;
}

static inline void wdt_kick(void)
{
    __asm__ __volatile__("wdr");
}

void board_init(void);

// Blocking delay, needs interrupts enabled
void delay_ms(uint8_t ms);
void board_tick_1ms(void);

#endif
