#include "lcd.h"

//*************************************************************************
//                 O B S L U G A   W Y S W I E T L A C Z A
//*************************************************************************

unsigned char _blink = 0;

void lcdBlink(unsigned char flag){
    _blink++;
    _blink &= 0x03;
    if ((_blink == 2) && (flag))
        _blink = 0;
}

void cmd2lcd(char rs, char bajt)
{
	delay(1);
	if (rs)
	{
		RSSET;
	}
	else
	{
		RSRST;
	}
	PORTC &= 0x0f;
	PORTC |= (bajt & 0xf0);
	ENSET;
	NOP2;
	NOP2;
	NOP2;
	NOP2;
	ENRST;
	PORTC &= 0x0f;
	PORTC |= ((bajt << 4) & 0xf0);
	ENSET;
	NOP2;
	NOP2;
	NOP2;
	NOP2;
	ENRST;
}

void gotoxy(char x, char y)
{
	cmd2lcd(0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x)); // 4x20
}

void char2lcd(char f, char c)
{
	cmd2lcd(1, ((f == 1) && (_blink == 0)) ? ' ' : c);
}

void cstr2lcd(char f, const unsigned char *c)
{
	while (*c)
	{
		char2lcd(f, *c);
		c++;
	}
}

void str2lcd(char f, unsigned char *c)
{
	while (*c)
	{
		char2lcd(f, *c);
		c++;
	}
}