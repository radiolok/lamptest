#include "util.h"

volatile unsigned char uart_busy;

void int2asc(unsigned int liczba, unsigned char* ascii)
{
    unsigned char temp;

    for (uint8_t i = 0; i < 4; i++)
    {
        temp = liczba % 10;
        liczba /= 10;
        ascii[i] = '0' + temp;
    }
}

// Writes num as fixed point: <integral> digits, '.', <frac> digits.
// Leading zeros of the integral part are blanked, the last one is kept.
// Output is not null-terminated.
void fp2ascii(unsigned int num,
                unsigned char integral,
                unsigned char frac,
                unsigned char *to)
{
    unsigned char digits[4];
    int2asc(num, digits);
    unsigned char *from = &digits[integral + frac - 1];
    unsigned char leading = 1;
    for (; integral > 0; --integral)
    {
        if (leading && (integral > 1) && (*from == '0')){
            *to = ' ';
        }
        else{
            *to = *from;
            leading = 0;
        }
        to++;
        from--;
    }
    if (frac != 0){
        *to++ = '.';
        for (;frac > 0; --frac){
            *to = *from;
            to++;
            from--;
        }
    }
}

void char2rs(unsigned char data)
{
	UDR = data;
	uart_busy = 1;
	while (uart_busy)
		; // czekaj na koniec wysylania bajtu
}

void cstr2rs(const char *q)
{
	while (*q) // do konca stringu
	{
		UDR = *q;
		q++;
		uart_busy = 1;
		while (uart_busy)
			; // czekaj na koniec wysylania bajtu
	}
}

volatile unsigned char txen;

void setTxen(const unsigned char state)
{
	txen = state;
}

unsigned char getTxen(void)
{
	return txen;
}

ISR(USART_TXC_vect)
{
	uart_busy = 0;
}

ISR(USART_RXC_vect)
{
	if (UDR == ESC)
		txen = 1;
}