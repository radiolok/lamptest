// AVT5229 vacuum tube tester firmware

#include <avr/interrupt.h>
#include "adc_scan.h"
#include "app.h"
#include "board.h"
#include "button.h"
#include "editor.h"
#include "lampdb.h"
#include "lcd.h"
#include "panel.h"
#include "sequencer.h"
#include "uart.h"
#include "ui.h"

static volatile uint8_t redraw;    // set every 250 ms

//***** Interrupts *********************************************************

ISR(ADC_vect)
{
    adc_scan_sample(ADC);
}

ISR(TIMER2_COMP_vect)
{
    static uint8_t div;

    board_tick_1ms();
    if (button_poll(hal_button_pressed()))
        seq_click();

    if (++div >= TICKS_PER_STEP) {
        div = 0;
        redraw = 1;
        seq_tick();
        lcd_blink_tick(button_held());
    }
}

ISR(INT1_vect)
{
    editor_on_encoder(hal_encoder_right());
}

//***** Main loop **********************************************************

static void send_report(void)
{
    const char *rpt = panel_report();

    lampdb_set_last(lamp_num);
    uart_puts("\r\n");
    for (uint8_t i = 0; i < RPT_LEN; i++) {
        if (rpt[i] != '\0')
            uart_putc(rpt[i]);
        else
            uart_puts("  ");
    }
}

int main(void)
{
    board_init();
    app_init();
    adc_scan_init();
    button_init();
    seq_init();
    panel_init();

    lamp_num = lampdb_last();
    hal_adc_select(ADC_CH_IH);

    sei();

    lcd_init();
    hal_beeper(1);
    ui_splash();
    for (uint8_t i = 0; i < 2; i++) {
        wdt_kick();
        delay_ms(100);
    }
    hal_beeper(0);
    ui_draw_labels();

    uart_puts("\r\nPress <ESC> to get LCD copy\r\n"
              "Nr Type Uh[V] Ih[mA] -Ug[V] Ua[V] Ia[mA] Ug2[V] Ig2[mA] S[mA/V] R[k] K[V/V]");

    GICR = BIT(INT1);   // encoder on

    for (;;) {
        wdt_kick();
        if (redraw) {
            redraw = 0;
            ui_draw();
        }
        panel_update();
        if (report_request) {
            send_report();
            report_request = 0;
        }
    }
}
