#ifndef LCD_H
#define LCD_H

    #include "definitions.h"

    void lcdBlink(unsigned char flag);

    void cmd2lcd(char rs, char bajt);
    void gotoxy(char x, char y);
    void char2lcd(char f, char c);
    void cstr2lcd(char f, const unsigned char *c);
    void str2lcd(char f, unsigned char *c);

#endif