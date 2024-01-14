#include "util.h"

unsigned char uart_busy;

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

void fp2ascii(unsigned int num,
                unsigned char integral,
                unsigned char frac,
                unsigned char *to)
{
    int2asc(num, temp_str);
    unsigned char *from = &temp_str[integral + frac - 1];
    for (; integral > 0; --integral)
    {
        if (integral){
            *to = (*from != '0')? *from : ' ';
            }
        else{
            *to = *from;
        }
        to++;
        from--;
    }
    if (frac != 0){
        *to = '.';
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

unsigned char txen;

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