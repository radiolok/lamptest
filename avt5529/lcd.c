#include "board.h"
#include "lcd.h"

static uint8_t blink_phase;     // characters are blanked in phase 0

#define NOP2() __asm__ __volatile__("rjmp .+0")

static void pulse_enable(void)
{
    PORTC |= BIT(PC_LCD_E);
    NOP2();
    NOP2();
    NOP2();
    NOP2();
    PORTC &= ~BIT(PC_LCD_E);
}

static void lcd_write(uint8_t rs, uint8_t byte)
{
    delay_ms(1);
    if (rs)
        PORTC |= BIT(PC_LCD_RS);
    else
        PORTC &= ~BIT(PC_LCD_RS);

    PORTC = (PORTC & ~PC_LCD_DATA) | (byte & 0xF0);
    pulse_enable();
    PORTC = (PORTC & ~PC_LCD_DATA) | (byte << 4);
    pulse_enable();
}

void lcd_init(void)
{
    delay_ms(30);
    lcd_write(0, 0x28);     // 4-bit bus, 2 lines, 5x8 font
    lcd_write(0, 0x06);     // increment, no shift
    lcd_write(0, 0x0C);     // display on, cursor off
    lcd_write(0, 0x01);     // clear
    lcd_write(0, 0x40);     // CGRAM address 0
}

void lcd_goto(uint8_t x, uint8_t y)
{
    // 4x20: lines 2 and 3 continue lines 0 and 1
    lcd_write(0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x));
}

void lcd_putc(uint8_t blink, char c)
{
    lcd_write(1, (blink && blink_phase == 0) ? ' ' : c);
}

void lcd_puts(uint8_t blink, const char *s)
{
    while (*s)
        lcd_putc(blink, *s++);
}

void lcd_blink_tick(uint8_t fast)
{
    blink_phase = (blink_phase + 1) & 0x03;
    if (blink_phase == 2 && fast)
        blink_phase = 0;
}
