#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

//*************************************************************
//                           TESTER
//*************************************************************
// OCD  JTAG SPI CK  EE   BOOT BOOT BOOT BOD   BOD SU SU CK   CK   CK   CK
// EN    EN  EN  OPT SAVE SZ1  SZ0  RST  LEVEL EN  T1 T0 SEL3 SEL2 SEL1 SEL0
//  1     0   0   1   1    0    0    1    1     1   1  0  0    0    0    1
//  1     1   0   0   1    0    0    1    1     1   1  0  1    1    1    1
//    ICCAVR = 0xc9ef
//*************************************************************

#define BIT(x)  (1<<(x))
#define LOW(x)  ((char)((int)(x)))
#define HIGH(x) ((char)(((int)(x))>>8))
#define ESC     '\033'
#define WDR     __asm("WDR")
#define SEI     __asm("SEI")
#define CLI     __asm("CLI")
#define NOP     __asm("NOP")
#define NOP2    __asm("RJMP .+2")

#define SET200  PORTB |= BIT(0)          // ustaw zakres 200mA
#define RST200  PORTB &= ~BIT(0)          // ustaw zakres 20mA

#define SETSEL  PORTB |= BIT(1)                   // ustaw UA1
#define RSTSEL  PORTB &= ~BIT(1)                  // ustaw UA2

#define ENSET   PORTC |= BIT(3)                // ustaw ENABLE
#define ENRST   PORTC &= ~BIT(3)               // kasuj ENABLE
#define RSSET   PORTC |= BIT(2)                   // ustaw R\S
#define RSRST   PORTC &= ~BIT(2)                  // kasuj R\S

#define DUSK0   ( PIND & BIT(2) ) == 0        // K0 nacisniety

#define RIGHT   ( PIND & BIT(6) ) == 0        // obrot w prawo

#define TOPPWM  ICR1                      // gorna granica PWM
#define PWMUG2  OCR1A                               // PWM Ug2
#define PWMUA   OCR1B                                // PWM Ua
#define PWMUH   OCR0                                // 8bit Uh

#define SPKON   TCCR2 |= BIT(COM20)               // wlacz ton
#define SPKOFF  TCCR2 &= ~BIT(COM20)             // wylacz ton

#define CLKUG1SET PORTB |= BIT(2)              // ustaw CLKUG1
#define CLKUG1RST PORTB &= ~BIT(2)             // kasuj CLKUG1

#define KWARC   16000000               // czestotliwosc kwarcu
#define RATE    103                              // 9600,N,8,1
#define MS1     250                      // podstawa czasu 1ms
#define MRUG    250                      // taktowanie migania
#define DMIN    20                          // czas drgan 20ms
#define DMAX    250                        // czas drgan 250ms
#define LPROB   63            // liczba usrednianych probek 64

#define HITEMP  10240         // 80"C = 0.8V/5.12V * 1024 * 64
#define LOTEMP  8960          // 70"C = 0.7V/5.12V * 1024 * 64

#define OVERSAMP 2              // liczba probek do alarmu - 1
#define OVERIH  0b00000001                // znacznik bledu IH
#define OVERIA  0b00000010                // znacznik bledu IA
#define OVERIG  0b00000100               // znacznik bledu IG2
#define OVERTE  0b00001000     // znacznik wzrostu temperatury

#define ADRREZ  0b00000000                // adres REZ w ADMUX
#define ADRIH   0b00000001                 // adres IH w ADMUX
#define ADRUH   0b00000010                 // adres UH w ADMUX
#define ADRUA   0b00000011                 // adres UA w ADMUX
#define ADRIA   0b00000100                 // adres IA w ADMUX
#define ADRUG2  0b00000101                // adres UG2 w ADMUX
#define ADRIG2  0b00000110                // adres IG2 w ADMUX
#define ADRUG1  0b00000111                // adres UG1 w ADMUX

#define TMAR    2
//      tuh     *240
#define TUA     8
#define TUG2    8
#define TUG     16
#define TUH     8

#define FUA     2
#define FUG2    2
#define FUG     2
#define FUH     2
#define BIP     4

#endif