#include <avr/interrupt.h>
#include "app.h"
#include "board.h"
#include "uart.h"

#define ESC '\033'

static volatile uint8_t tx_busy;

void uart_putc(char c)
{
    tx_busy = 1;
    UDR = c;
    while (tx_busy)
        ;   // wait for the transmit-complete interrupt
}

void uart_puts(const char *s)
{
    while (*s)
        uart_putc(*s++);
}

ISR(USART_TXC_vect)
{
    tx_busy = 0;
}

ISR(USART_RXC_vect)
{
    if (UDR == ESC)
        report_request = 1;
}
