#include "ui.h"
#include "app.h"
#include "button.h"
#include "config.h"
#include "format.h"
#include "lcd.h"
#include "panel.h"
#include "sequencer.h"

void ui_splash(void)
{
    lcd_goto(0, 0); lcd_puts(0, "VTTester 1.16");
    lcd_goto(0, 1); lcd_puts(0, "AVT 5229");
    lcd_goto(0, 2); lcd_puts(0, "Adam Tomasz");
    lcd_goto(0, 3); lcd_puts(0, "Tatus Gumny");
}

void ui_draw_labels(void)
{
    lcd_goto(0, 0); lcd_puts(0, "             G     V");
    lcd_goto(0, 1); lcd_puts(0, "H=    V A=    G2=   ");
    lcd_goto(0, 2); lcd_puts(0, "      mA            ");
}

static char error_code(void)
{
    // The most severe error wins
    uint8_t e = err;
    if (e & ERR_TEMP)
        return 'T';
    if (e & ERR_IG2)
        return 'G';
    if (e & ERR_IA)
        return 'A';
    if (e & ERR_IH)
        return 'H';
    return ' ';
}

static void draw_param(uint8_t x, uint8_t y, uint8_t f, uint8_t pos)
{
    lcd_goto(x, y);
    lcd_puts(field == f, panel_report() + pos);
}

void ui_draw(void)
{
    const char *rpt = panel_report();
    uint16_t t = seq_now();
    uint8_t holding = (t == SEQ_HOLD);

    lcd_goto(0, 0);
    lcd_puts(field == F_LAMP && !holding, rpt + RPT_NUM);

    // While holding on a twin tube, the section digit blinks: it can be
    // switched with the knob.
    char section = rpt[RPT_NAME + NAME_SECTION];
    uint8_t twin = holding && (section == '1' || section == '2');
    lcd_goto(3, 0);
    for (uint8_t i = 0; i < sizeof(lamp.name); i++)
        lcd_putc(field == F_NAME + i || (i == NAME_SECTION && twin), rpt[RPT_NAME + i]);

    lcd_goto(14, 0);
    lcd_putc(field == F_UG1, '-');
    lcd_puts(field == F_UG1, rpt + RPT_UG1);

    draw_param(2, 1, F_UH, RPT_UH);

    lcd_goto(0, 2);
    lcd_putc(0, error_code());

    draw_param(2, 2, F_IH, RPT_IH);
    draw_param(10, 1, F_UA, RPT_UA);
    draw_param(9, 2, F_IA, RPT_IA);
    draw_param(17, 1, F_UG2, RPT_UG2);
    draw_param(15, 2, F_IG2, RPT_IG2);

    lcd_goto(0, 3);
    if (lamp_num < LAMP_FIRST_TUBE) {
        lcd_puts(0, "* OVERHEAT Warning *");
        return;
    }
    lcd_puts(0, "S=");
    lcd_puts(field == F_S, rpt + RPT_S);
    lcd_goto(6, 3);
    lcd_puts(0, " R=");
    lcd_puts(field == F_R, rpt + RPT_R);

    lcd_goto(13, 3);
    if (t <= SEQ_HOLD || button_held()) {
        lcd_puts(0, " K=");
        lcd_puts(field == F_K, rpt + RPT_K);
    } else {
        // Remaining time of the measurement in seconds
        char secs[3];
        fmt_fixed(t >> 2, 3, 0, secs);
        lcd_puts(0, " T=");
        for (uint8_t i = 0; i < 3; i++)
            lcd_putc(0, secs[i]);
        lcd_putc(0, 's');
    }
}
