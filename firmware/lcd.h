#ifndef LCD_H
#define LCD_H

#include <stdint.h>

// HD44780 4x20 character LCD, 4-bit bus on PORTC

void lcd_init(void);
void lcd_goto(uint8_t x, uint8_t y);

// With `blink` set the character is replaced by a space during the off
// phase, which marks the field being edited.
void lcd_putc(uint8_t blink, char c);
void lcd_puts(uint8_t blink, const char *s);

// Advance the blink phase, every 250 ms. `fast` blinks at 2 Hz instead of 1 Hz.
void lcd_blink_tick(uint8_t fast);

#endif
