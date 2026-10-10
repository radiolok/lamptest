#ifndef UART_H
#define UART_H

// RS-232 at 9600 8N1. Receiving ESC requests a report line.

void uart_putc(char c);
void uart_puts(const char *s);

#endif
