#include "board.h"

static volatile uint8_t delay_ticks;

void board_init(void)
{
    //***** I/O ports ****************************************************
    //   7   6   5   4   3   2   1   0
    // UG1 IG2 UG2  IA  UA  UH  IH TMP     analog inputs
    DDRA = 0x00;
    PORTA = 0x00;
    // SCK MISO MOSI K1 UH  CKG SEL RNG
    DDRB = 0x0F;
    PORTB = 0xF0;
    //  D7  D6  D5  D4  E   RS  SDA SCL    LCD
    DDRC = 0xFC;
    PORTC = 0x03;
    // SPK DIR UG2  UA CLK  K0 TXD RXD
    DDRD = 0xB2;
    PORTD = 0x4F;

    ACSR = BIT(ACD);    // analog comparator off

    //***** Timer2: 1 ms tick, OC2 drives the beeper *********************
    TCCR2 = BIT(FOC2) | BIT(WGM21) | BIT(CS22);     // CTC, XTAL/64
    OCR2 = TIMER2_1MS;

    //***** PWM ************************************************************
    TCCR0 = BIT(WGM01) | BIT(WGM00) | BIT(CS00);    // Uh: fast PWM, XTAL, OC0 off
    TCCR1A = BIT(COM1A1) | BIT(COM1B1);             // Ug2, Ua
    TCCR1B = BIT(WGM13) | BIT(CS10);                // phase+freq correct, TOP = ICR1, XTAL
    PWM_TOP = 61 * VREF / 100;                      // PWM period of Ua and Ug2

    //***** ADC: free running, interrupt per conversion, XTAL/128 ********
    ADMUX = ADC_CH_UG1;
    ADCSRA = BIT(ADEN) | BIT(ADSC) | BIT(ADATE) | BIT(ADIF) | BIT(ADIE)
           | BIT(ADPS2) | BIT(ADPS1) | BIT(ADPS0);

    //***** UART: 9600 8N1 ***********************************************
    UBRRL = UART_UBRR;
    UCSRB = BIT(RXCIE) | BIT(RXEN) | BIT(TXCIE) | BIT(TXEN);
    UCSRC = BIT(URSEL) | BIT(UCSZ1) | BIT(UCSZ0);

    //***** Watchdog: ~1 s *************************************************
    WDTCR = BIT(WDE);
    WDTCR = BIT(WDE) | BIT(WDP2) | BIT(WDP1);
    wdt_kick();

    //***** Interrupts ****************************************************
    MCUCR = BIT(ISC11);     // INT1 (encoder) on the falling edge
    TIMSK = BIT(OCIE2);     // Timer2 compare
    GIFR = BIT(INTF1);      // clear a pending INT1
}

void delay_ms(uint8_t ms)
{
    delay_ticks = ms + 1;
    while (delay_ticks != 0)
        ;
}

void board_tick_1ms(void)
{
    if (delay_ticks != 0)
        delay_ticks--;
}
