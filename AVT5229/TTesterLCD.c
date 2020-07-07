//*************************************************************
//                           TESTER
//*************************************************************
// OCD  JTAG SPI CK  EE   BOOT BOOT BOOT BOD   BOD SU SU CK   CK   CK   CK
// EN    EN  EN  OPT SAVE SZ1  SZ0  RST  LEVEL EN  T1 T0 SEL3 SEL2 SEL1 SEL0
//  1     0   0   1   1    0    0    1    1     1   1  0  0    0    0    1
//  1     1   0   0   1    0    0    1    1     1   1  0  1    1    1    1
//    ICCAVR = 0xc9ef
//*************************************************************

#include <iom16v.h>
#include <eeprom.h>

#define BIT(x)  (1<<(x))
#define LOW(x)  ((char)((int)(x)))
#define HIGH(x) ((char)(((int)(x))>>8))
#define ESC     '\033'
#define WDR     asm("WDR")
#define SEI     asm("SEI")
#define CLI     asm("CLI")
#define NOP     asm("NOP")
#define NOP2    asm("RJMP .+2")

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

#define FLAMP   (unsigned char)(1+80)
#define ELAMP   (unsigned char)19

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

typedef struct
{
   unsigned char nazwa[9];
   unsigned char uhdef;
   unsigned char ihdef;
   unsigned char ug1def;
   unsigned int  uadef;
   unsigned int  iadef;
   unsigned int  ug2def;
   unsigned int  ig2def;
   unsigned int  sdef;
   unsigned int  rdef;
   unsigned int  kdef;
} katalog;

#pragma data:data

unsigned char
   d, i,
   busy,
	sync,
   txen,
   *cwart, cwartmin, cwartmax,
   adr, adrmin, adrmax,
   typ, nowa,
	stop,
   zwloka,
	dziel,
   nodus, dusk0,
	zapisz, czytaj,
   range, rangelcd, rangedef,
   ascii[5],
   kanal,
   takt,
	overih, overia, overig2, err, errcode,
   probki,
   pwm,
	buf[63];

unsigned int
   *wart, wartmin, wartmax,
	start, tuh,
   vref,
	adcih, adcia, adcig2,
   uhset,  ihset,  ug1set,  uaset,  iaset,  ug2set,	ig2set,
	suhadc, sihadc, sug1adc, suaadc, siaadc, sug2adc, sig2adc,
   muhadc, mihadc, mug1adc, muaadc, miaadc, mug2adc, mig2adc,
   ug1zer, ug1ref,
	uh, ih,
	ua, ual, uar,
	ia, ial, iar,
	ug2, ig2,
	ug1, ugl, ugr,
	s, r, k,
	uhlcd, ihlcd, ug1lcd, ualcd, ialcd, ug2lcd, ig2lcd, slcd, rlcd, klcd,
	srezadc, mrezadc;

unsigned long
   lint, tint, licz, temp;

katalog
   lamptem;

const unsigned char
   AZ[37] =
{ 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','_','0','1','2','3','4','5','6','7','8','9' };
//00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36
const katalog
   lamprom[FLAMP] =
{
{'P','w','r','S','u','p','p','l','y',  0,  0,240,  0,   0,  0,   0,  0,  0,  0 },
{'F','o','r','F','u','t','U','s','e',  0,  0,240,  0,   0,  0,   0,  0,  0,  0 },
{'E','C','C','8','1','_','G','1','1',126,  0, 20,250, 100,  0,   0, 55,110,600 },
{'E','C','C','8','1','_','G','2','1',126,  0, 20,250, 100,  0,   0, 55,110,600 },
{'E','C','C','8','2','_','G','1','1',126,  0, 85,250, 105,  0,   0, 22, 77,170 },
{'E','C','C','8','2','_','G','2','1',126,  0, 85,250, 105,  0,   0, 22, 77,170 },
{'E','C','C','8','3','_','G','1','1',126,  0, 20,250,  12,  0,   0, 16,625,999 },
{'E','C','C','8','3','_','G','2','1',126,  0, 20,250,  12,  0,   0, 16,625,999 },
{'E','C','C','8','8','_','G','1','1', 63,  0, 13, 90, 150,  0,   0,125, 26,330 },
{'E','C','C','8','8','_','G','2','1', 63,  0, 13, 90, 150,  0,   0,125, 26,330 },
{'6','N','1','P','_','_','G','1','1', 63,  0, 40,250,  75,  0,   0, 45,  0,350 },
{'6','N','1','P','_','_','G','2','1', 63,  0, 40,250,  75,  0,   0, 45,  0,350 },
{'6','N','2','P','_','_','G','1','1', 63,  0, 15,250,  23,  0,   0, 21,  0,999 },
{'6','N','2','P','_','_','G','2','1', 63,  0, 15,250,  23,  0,   0, 21,  0,999 },
{'6','N','6','P','_','_','G','1','1', 63,  0, 20,120, 300,  0,   0,110, 19,200 },
{'6','N','6','P','_','_','G','2','1', 63,  0, 20,120, 300,  0,   0,110, 19,200 },
{'6','N','3','0','P','_','G','1','1', 63,  0, 22, 80, 400,  0,   0,180,  0,150 },
{'6','N','3','0','P','_','G','2','1', 63,  0, 22, 80, 400,  0,   0,180,  0,150 },
{'6','S','L','7','_','_','I','1','2', 63,  0, 20,250,  23,  0,   0, 16,440,700 },
{'6','S','L','7','_','_','I','2','2', 63,  0, 20,250,  23,  0,   0, 16,440,700 },
{'6','S','N','7','_','_','I','1','2', 63,  0, 80,250,  90,  0,   0, 26, 77,200 },
{'6','S','N','7','_','_','I','2','2', 63,  0, 80,250,  90,  0,   0, 26, 77,200 },
{'6','A','S','7','_','_','I','1','5', 63,  0,200, 80,1100,  0,   0,  0,  0,  0 },
{'6','A','S','7','_','_','I','2','5', 63,  0,200, 80,1100,  0,   0,  0,  0,  0 },
{'6','S','1','9','P','_','H','0','2', 63,  0,150, 90,1000,  0,   0,  0,  0,  0 },
{'E','F','8','6','_','_','E','0','1', 63,  0, 20,250,  30,140,  60, 19,999,380 },
{'6','S','J','7','_','_','D','0','2', 63,  0, 30,250,  30,100,  80, 17,999,  0 },
{'E','L','8','4','_','_','B','0','2', 63,  0, 73,250, 480,250, 550,113,400,190 },
{'6','V','6','S','_','_','A','0','2', 63,  0,125,250, 450,250, 500, 41,500,  0 },
{'6','F','6','S','_','_','A','0','2', 63,  0,200,285, 380,285, 700, 25,500,  0 },
{'6','L','6','G','_','_','A','0','3', 63,  0,140,250, 720,250, 500,  0,225,  0 },
{'E','L','3','4','_','_','A','0','5', 63,  0,135,250,1000,265,1490,110,150,110 },
{'K','T','6','6','_','_','A','0','5', 63,  0,150,250, 850,250, 700, 60,225,  0 },
{'K','T','7','7','_','_','A','0','5', 63,  0,150,250,1000,250,1000,105,230,115 },
{'K','T','8','8','_','_','A','0','5', 63,  0,150,250,1400,250, 700,115,120, 80 },
{'6','P','1','P','_','_','C','0','2', 63,  0,125,250, 450,250, 700, 45,500,  0 },
{'E','L','9','0','_','_','F','0','1', 63,  0,125,250, 450,250, 450, 41,520,  0 },
{'E','L','9','5','_','_','F','0','1', 63,  0, 90,250, 240,250, 450, 50,800,170 },
{'P','C','L','8','6','T','J','1','2',  0, 30, 17,230,  12,  0,   0, 16,620,990 },
{'P','C','L','8','6','P','J','2','2',  0, 30, 57,230, 390,230, 650,105,450,999 },
{'E','C','L','8','6','T','J','1','2', 63,  0, 19,250,  12,  0,   0, 16,620,990 },
{'E','C','L','8','6','P','J','2','2', 63,  0, 70,250, 360,250, 600,100,480,999 },
{'E','C','L','8','2','T','J','1','2', 63,  0,  5,100,  35,  0,   0, 22,  0,700 },
{'E','C','L','8','2','P','J','2','2', 63,  0,115,170, 410,170, 650, 75,160, 95 },
{'E','C','C','3','5','_','I','1','2', 63,  0, 23,250,  23,  0,   0, 20,  0,  0 },
{'E','C','C','3','5','_','I','2','2', 63,  0, 23,250,  23,  0,   0, 20,  0,  0 },
{'E','C','C','4','0','_','J','1','2', 63,  0, 52,250,  60,  0,   0, 27,  0,  0 },
{'E','C','C','4','0','_','J','2','2', 63,  0, 52,250,  60,  0,   0, 27,  0,  0 },
{'E','C','C','8','5','_','G','1','1', 63,  0, 23,250, 100,  0,   0, 59,  0,  0 },
{'E','C','C','8','5','_','G','2','1', 63,  0, 23,250, 100,  0,   0, 59,  0,  0 },
{'E','C','C','9','1','_','J','1','2', 63,  0,  9,100,  85,  0,   0, 53,  0,  0 },
{'E','C','C','9','1','_','J','2','2', 63,  0,  9,100,  85,  0,   0, 53,  0,  0 },
{'E','C','C','9','9','_','G','1','2',126,  0, 40,150, 180,  0,   0, 95, 23,220 },
{'E','C','C','9','9','_','G','2','2',126,  0, 40,150, 180,  0,   0, 95, 23,220 },
{'E','1','8','0','C','C','G','1','2',126,  0, 19,150,  85,  0,   0, 64, 72,460 },
{'E','1','8','0','C','C','G','2','2',126,  0, 19,150,  85,  0,   0, 64, 72,460 },
{'E','1','8','2','C','C','J','1','2',126,  0, 20,100, 360,  0,   0,150,  0,240 },
{'E','1','8','2','C','C','J','2','2',126,  0, 20,100, 360,  0,   0,150,  0,240 },
{'E','C','C','8','0','1','G','1','1',126,  0, 20,250, 100,  0,   0, 55,  0,  0 },
{'E','C','C','8','0','1','G','2','1',126,  0, 20,250, 100,  0,   0, 55,  0,  0 },
{'E','C','C','8','0','2','G','1','1',126,  0, 85,250, 105,  0,   0, 22, 77,170 },
{'E','C','C','8','0','2','G','2','1',126,  0, 85,250, 105,  0,   0, 22, 77,170 },
{'E','C','C','8','0','3','G','1','1',126,  0, 20,250,  12,  0,   0, 16,625,999 },
{'E','C','C','8','0','3','G','2','1',126,  0, 20,250,  12,  0,   0, 16,625,999 },
{'E','C','C','8','3','2','G','1','1',126,  0, 85,250, 105,  0,   0, 22, 77,170 },
{'E','C','C','8','3','2','G','2','1',126,  0, 20,250,  12,  0,   0, 16,625,999 },
{'P','C','C','8','4','_','G','1','1',  0, 30, 15, 90, 120,  0,   0, 60,  0,  0 },
{'P','C','C','8','4','_','G','2','1',  0, 30, 15, 90, 120,  0,   0, 60,  0,  0 },
{'P','C','C','8','5','_','G','1','1',  0, 30, 21,200, 100,  0,   0, 58,  0,  0 },
{'P','C','C','8','5','_','G','2','1',  0, 30, 21,200, 100,  0,   0, 58,  0,  0 },
{'P','C','C','8','8','_','G','1','1',  0, 30, 12, 90, 150,  0,   0,125,  0,  0 },
{'P','C','C','8','8','_','G','2','1',  0, 30, 12, 90, 150,  0,   0,125,  0,  0 },
{'6','S','C','7','_','_','J','1','2', 63,  0, 20,250,  20,  0,   0, 13,  0,  0 },
{'6','S','C','7','_','_','J','2','2', 63,  0, 20,250,  20,  0,   0, 13,  0,  0 },
{'6','N','3','P','_','_','J','1','1', 63,  0, 20,150,  82,  0,   0, 56,  0,  0 },
{'6','N','3','P','_','_','J','2','1', 63,  0, 20,150,  82,  0,   0, 56,  0,  0 },
{'6','N','5','S','_','_','I','1','3', 63,  0,200, 90,1200,  0,   0,  0,  0,  0 },
{'6','N','5','S','_','_','I','2','3', 63,  0,200, 90,1200,  0,   0,  0,  0,  0 },
{'6','N','7','S','_','_','J','1','3', 63,  0, 60,290,  35,  0,   0, 16,  0,  0 },
{'6','N','7','S','_','_','J','2','3', 63,  0, 60,290,  35,  0,   0, 16,  0,  0 },
{'6','S','2','S','_','_','A','0','2', 63,  0, 80,250,  90,  0,   0, 25,  0,  0 }
};
//  nazwa            cokol anoda czas uh  ih ug1  ua   ia ug2  ig2   S   R   K

#pragma data: eeprom

unsigned int
   poptyp = 0;

katalog
   lampeep[ELAMP] =
{
{ 33, 13, 28, 32, 18, 26,  9, 28, 28, 63,  0, 90,100,  90,  0,   0, 56,  0,  0 },
{ 33, 13, 28, 32, 18, 26,  9, 29, 28, 63,  0, 90,100,  90,  0,   0, 56,  0,  0 },
{  4,  1, 11, 29, 28, 26,  9, 27, 29, 63,  0, 60,250, 360,250, 450, 90,500,230 },
{  4,  5, 29, 28, 26, 26,  9, 27, 29, 63,  0, 20,250,  60,250, 200, 45,999,  0 },
{  4,  5, 35, 27, 33, 26,  4, 27, 28, 63,  0, 22,250,  30,140,  60, 22,999,380 },
{  4,  5, 35, 27, 26, 26,  9, 27, 28, 63,  0, 20,175, 100,175,   0, 72,  0,  0 },
{  4,  5, 35, 32, 26, 26,  9, 27, 28, 63,  0, 20,250, 100,100, 250, 60,999,260 },
{  4,  5, 35, 36, 26, 26,  9, 27, 28, 63,  0, 20,250,  90,100, 300, 36,999,  0 },
{ 34, 27, 29, 34, 26, 26,  9, 27, 32, 63,  0,140,250, 720,250, 500,  0,225,  0 },
{ 34, 32, 35, 28, 26, 26,  0, 27, 32, 63,  0,140,250, 720,250, 500, 60,225,  0 },
{ 34, 32, 36, 28, 26, 26,  9, 27, 32, 63,  0,100,290, 600,290, 800,102,290,168 },
{  4, 11, 30, 33, 26, 26,  9, 27, 32, 63,  0, 82,100,1000,100, 700,140, 50, 56 },
{ 33,  9, 34, 26, 26, 26,  9, 27, 29, 63,  0, 30,250,  20,100,  50, 12,  0,  0 },
{  4, 11, 35, 29, 26, 26,  1, 27, 29, 63,  0,104,170, 530,170,1000, 90,200,100 },
{  4, 11, 35, 33, 26, 26,  1, 27, 29, 63,  0,125,170, 700,170, 500,100,230, 80 },
{ 33, 15, 28, 32, 15, 26,  9, 27, 29, 63,  0, 35,280, 310,170, 500,150,999, 70 },
{ 33, 25, 31, 15, 26, 26,  9, 27, 29, 63,  0, 10,250, 108,150, 430, 52,999,410 },
{ 33, 15, 34, 18, 26, 26,  9, 27, 30, 63,  0,140,250, 720,250, 800, 59,999, 85 },
{ 33, 15, 36, 26, 26, 26,  9, 27, 29, 63,  0, 60,250, 300,250, 300, 70,600,  0 }
};

//*************************************************************************
//                 F U N K C J E  P O M O C N I C Z E
//*************************************************************************

void char2rs(unsigned char bajt)
{
   UDR = bajt;
   busy = 1;
   while( busy );          // czekaj na koniec wysylania bajtu
}

void cstr2rs( const char* q )
{
   while( *q )                             // do konca stringu
   {
      UDR = *q;
      q++;
      busy = 1;
      while(busy);         // czekaj na koniec wysylania bajtu
   }
}

void delay( unsigned char opoz )            // opoznienie *1ms
{
   zwloka = opoz + 1;
   while( zwloka != 0 ) ;
}

void zersrk( void )                         // zeruj S, R, K
{
	s = r = k = ualcd = ialcd = ug2lcd = ig2lcd = slcd = rlcd = klcd = 0;
}

unsigned int liczug1( unsigned int pug1 )                // przelicz Ug1
{
   licz = 640000;
   licz *= vref;
	temp = 1024000;
	temp *= pug1;                                   // ug1
	licz -= temp;
   licz /= 725;
	licz /= vref;
   return( (unsigned int)licz );       // 882..193..55
}

//*************************************************************************
//                 O B S L U G A   P R Z E R W A N
//*************************************************************************

#pragma interrupt_handler ext_int1:3
#pragma interrupt_handler timer2_comp:4
#pragma interrupt_handler usart_rxc:12
#pragma interrupt_handler usart_txc:14
#pragma interrupt_handler adc:15

void ext_int1( void )
{
   if( start > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2) ) 
   { 
  	   if( nodus == DMIN )
      {
	      start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2);  // wylaczanie kreciolkiem
	      stop = 0;
      }
   }
   else
	{
   	if( start == (FUH+2) ) 
	   {
   	   if( dusk0 == DMAX )
         {
            if( RIGHT )
            {
               if( (nowa == 1) && ((lamptem.nazwa[7] == '1') || (lamptem.nazwa[7] == 28)) ) { typ++; nowa = 0; }
            }
		      else
            {
               if( (nowa == 1) && ((lamptem.nazwa[7] == '2') || (lamptem.nazwa[7] == 29)) ) { typ--; nowa = 0; }
            }
   			zersrk();
         }
		   if( nodus == DMIN )
   		{
	   	   start = (FUH+1);
		   }
   	}
      else
		{
         if( start == 0 )
      	{
            if( adr == 0 )                          // edycja numeru
            {
               cwartmin = 0;
               cwartmax = (FLAMP+ELAMP-1);
               cwart = &typ;		
            }
            if( (adr > 0) && (adr < 7) )                 // edycja nazwy
            {
               cwartmin = 0;
		         cwartmax = 36;
               cwart = &lamptem.nazwa[adr-1];
            }
            if( adr == 7 )              // zmiana nr podstawki zarzenia
            {
               cwartmin = 0;
	         	cwartmax = 9;
               cwart = &lamptem.nazwa[6];
            }
            if( adr == 8 )                // zmiana nr systemu elektrod
            {
               cwartmin = 27;
		         cwartmax = 29;
               cwart = &lamptem.nazwa[7];
            }
            if( adr == 9 )                 // ustawianie czasu zarzenia
            {
               cwartmin = 28;
	         	cwartmax = 36;
               cwart = &lamptem.nazwa[8];
            }
            if( adr == 10 )                         // ustawianie Ug1
            {
               cwartmin = (typ == 0)? 0: 5;          // 0.5 lub 0
		         cwartmax = (typ == 0)? 240: 235;      // -23.5 lub -24.0V
               cwart = &lamptem.ug1def;
            }
            if( adr == 11 )                         // ustawianie Uh
            {
               cwartmin = 0;
   	      	cwartmax = 150;                           // 0..15.0V
               cwart = &lamptem.uhdef;
            } 
            if( adr == 12 )                         // ustawianie Ih
            {
               cwartmin = 0;
		         cwartmax = 250;                     // 0..2.50A
               cwart = &lamptem.ihdef;
            }
            if( adr == 13 )                         // ustawianie Ua
            {
               wartmin = (typ == 0)? 0: 10;
	         	wartmax = (typ == 0)? 300: 290;     // 0..300V
               wart = &lamptem.uadef;
            }
            if( adr == 14 )                         // ustawianie Ia
            {
               wartmin = 0;
		         wartmax = 2000;                      // 0..200.0mA
               wart = &lamptem.iadef;
            }
            if( adr == 15 )                        // ustawianie Ug2
            {
               wartmin = 0;
   	      	wartmax = 300;                       // 0..300V
               wart = &lamptem.ug2def;
            }
            if( adr == 16 )                         // ustawianie Ig2
            {
               wartmin = 0;
		         wartmax = 4000;                      // 0..40.00mA
               wart = &lamptem.ig2def;
            }
            if( adr == 17 )                        // ustawianie S
            {
               wartmin = 0;
   	      	wartmax = 999;                         // 99.9
               wart = &lamptem.sdef;
            }
            if( adr == 18 )                        // ustawianie R
            {
               wartmin = 0;
		         wartmax = 999;                      // 99.9
               wart = &lamptem.rdef;
            }
            if( adr == 19 )                        // ustawianie K
            {
               wartmin = 0;
	         	wartmax = 999;                       // 99.9
               wart = &lamptem.kdef;
            }

            if( RIGHT )
            {
               if( adr < 13 )
		         {
                  if( dusk0 == DMAX )
                  {
            	      if( (*cwart) < cwartmax ) { (*cwart)++; } else { if( adr == 0 ) { (*cwart) = 0; } }
	               }
           		}
             	else
        		   { 
                  if( (dusk0 == DMAX) && ((*wart) < wartmax) ) { (*wart)++; }
               }
	            if( nodus == DMIN )
      		   {
   	      	   if( typ == 0 )
	   		      {
                     if( adr == 0 ) { adr += 9; } // przeskocz nazwe
                     if( adr == 13 ) { adr++; } // przeskocz Ia
				         if( adr < 15 ) { adr++; } // nie dojezdzaj do Ig2
         			}
	         		if( typ >= FLAMP )
		   	      {
      			      if( adr < 19 ) { adr++; }
   	      		} 
      	   	}
            }
            else
            {
      	      if( adr < 13 )
		         {
                  if( dusk0 == DMAX )
      	   		{
		               if( (*cwart) > cwartmin ) { (*cwart)--; } else { if( adr == 0 ) { (*cwart) = (FLAMP+ELAMP-1); } }
   			      }
      	   	}
		         else
         		{
                  if( (dusk0 == DMAX) && ((*wart) > wartmin) ) { (*wart)--; }
               }
               if( (nodus == DMIN) && (adr > 0) )
      	   	{
	               if( (typ == 0) && (adr == 10) ) { adr -= 9; stop = 0; start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // przeskocz nazwe / wylaczenie 
                  if( (typ == 0) && (adr == 15) ) { adr--; }
         		   if( (typ == 0) || (typ >= FLAMP) ) { adr--; }
	         	}
            }
			}
		}
	}
}

void adc(void)
{
   switch( kanal )
	{
   	case 0:
		{
         srezadc += ADC;
			kanal = 1;
			CLKUG1SET;
         ADMUX = ADRIH;
			break;
		}
	   case 1:
	   {
			kanal = 2;
	      if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
		   break;
		}
		case 2:
		{
		   adcih = ADC;
         sihadc += adcih;
			kanal = 3;
	   	CLKUG1SET;
         ADMUX = ADRUH;
//***** Zabezpieczenie nadpradowe Ih **************************
         if( adcih > 400 )
			{
			   if( overih > 0 )
				{
				   overih--;
				}
				else
				{
			      uhset = ihset = 0;
				   err |= OVERIH;
				}
			}
			else
			{
			   overih = OVERSAMP;
			}
			break;
		}
	   case 3:
	   {
			kanal = 4;
	      if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
		   break;
		}
   	case 4:
		{
         suhadc += ADC;
			kanal = 5;
		   CLKUG1SET;
         ADMUX = ADRUA;
			break;
		}
	   case 5:
	   {
			kanal = 6;
	      if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
		   break;
		}
   	case 6:
		{
         suaadc += ADC;
			kanal = 7;
		   CLKUG1SET;
         ADMUX = ADRIA;
			break;
		}
	   case 7:
	   {
			kanal = 8;
	      if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
		   break;
		}
		case 8:
		{
		   adcia = ADC;
         siaadc += adcia;
			kanal = 9;
		   CLKUG1SET;
         ADMUX = ADRUG2;
//***** Zabezpieczenie nadpradowe Ia ***************************
         if( (range != 0) && (adcia >= 1020) )
			{
			   if( overia > 0 )
				{
				   overia--;
				}
				else
				{
				
   				uaset = ug2set = 0;
               PWMUA = PWMUG2 = 0;;
				   err |= OVERIA;
				}
			}
			else
			{
			   overia = OVERSAMP;
			}
//***** Wystawienie Ua ****************************************
         if( PWMUA < uaset ) PWMUA++;
      	if( PWMUA > uaset ) PWMUA--;
//***** Ustawianie wysokiego zakresu pomiarowego Ia ***********
         if( (range == 0) && (adcia > 950) )
		   {
  		      range = 1;
  			   SET200;
		   }
//***** Ustawianie niskiego zakresu pomiarowego Ia ************
         if( ((err & OVERIA) == 0) && (range != 0) && (adcia < 85) )
     		{
	   	   range = 0;
		      RST200;
	      }
			break;
		}
	   case 9:
	   {
			kanal = 10;
	      if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
		   break;
		}
   	case 10:
		{
         sug2adc += ADC;
			kanal = 11;
		   CLKUG1SET;
         ADMUX = ADRIG2;
			break;
		}
	   case 11:
	   {
			kanal = 12;
	      if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
		   break;
		}
   	case 12:
		{
		   adcig2 = ADC;
         sig2adc += adcig2;
			kanal = 13;
		   CLKUG1SET;
         ADMUX = ADRREZ;
//***** Zabezpieczenie nadpradowe Ig2 **************************
         if( adcig2 >= 1020 )
			{
			   if( overig2 > 0 )
				{
				   overig2--;
				}
				else
				{
					ug2set = 0;
					PWMUG2 = 0;
					err |= OVERIG;
				}
			}
			else
			{
			   overig2 = OVERSAMP;
			}
//***** Wystawienie Ug2 ***************************************
         if( PWMUG2 < ug2set ) PWMUG2++;
   	   if( PWMUG2 > ug2set ) PWMUG2--;
			break;
		}
	   case 13:
	   {
         sug1adc += ADC;
			kanal = 0;
	      if( ADC >= ug1set ) { CLKUG1RST; }
         ADMUX = ADRUG1;
      	if( probki == LPROB )
	      {
	         mrezadc = srezadc;
   	      mihadc = sihadc;
   	      muhadc = suhadc;
	         muaadc = suaadc;
   	      miaadc = siaadc;
	         mug2adc = sug2adc;
   	      mig2adc = sig2adc;
      	   mug1adc = sug1adc;
				srezadc = sihadc = suhadc = suaadc = siaadc = sug2adc = sig2adc = sug1adc = 0;
	      	probki = 0;
//***** Szybkie wylaczenie Uh *********************************
            if( (uhset == 0) && (ihset == 0) )
				   pwm = 0;
				else
				{
				   TCCR0 |= BIT(COM01);               // dolacz OC0
//***** Stabilizacja Uh ***************************************
               if( uhset > 0 )
               {
                  lint = muhadc;
	            	lint *= vref;
      		      lint >>= 14;        //  /= 16384;                // 0..200
                  tint = mihadc;   // poprawka na spadek napiecia na boczniku
            		tint *= vref;
            		tint >>= 16;        //  /= 65536;
            		if( lint > tint ) { lint -= tint; } else { lint = 0; }
            		lint /= 10;
                  if( (uhset > (unsigned int)lint) && (pwm < 255) ) { pwm++; }
                  if( (uhset < (unsigned int)lint) && (pwm >   0) ) { pwm--; }
               }
//***** Stabilizacja Ih ***************************************
               if( ihset > 0 )
  	            {
                  lint = mihadc;
    		         lint *= vref;
   			      lint >>= 15;    //   /= 32768;
                  if( (ihset > (unsigned int)lint) )
						{
						   if( (pwm < 8) || ((mihadc > 32) && (pwm < 255)) ) { pwm++; } // Uh<0.5V lub Ih>5mA
						}
                  if( (ihset < (unsigned int)lint) && (pwm >   0) ) { pwm--; }
               }
            }
            if( pwm == 0 )
				{
					TCCR0 &= ~BIT(COM01);                        // odlacz OC0
				}
				else
				{
				   TCCR0 |= BIT(COM01);                          // dolacz OC0
				}
				PWMUH = pwm;
//***** Ustawianie/kasowanie znacznika przegrzania ************
            if( mrezadc > HITEMP ) err |= OVERTE;
            if( mrezadc < LOTEMP ) err &= ~OVERTE;
         }
	      else
   	   {
	         probki++;
      	}
		   break;
		}
   }
//	NOP;
}

void usart_txc(void)
{
   busy = 0;
}

void usart_rxc( void )
{
   if( UDR == ESC ) txen = 1;
}

void timer2_comp( void )
{
   if( zwloka != 0 ) { zwloka--; }

   if( DUSK0 )
   {
      if( dusk0 < DMAX ) { dusk0++; } else { nodus = 0; }
   }
   else
   {
      if( nodus < DMIN )
		{
		   nodus++;
		}
		else
		{
		   if( (dusk0 > DMIN) && (dusk0 < DMAX) )
			{
			   if( (typ > 1) && (err == 0) )
				{
   			   if( (start == 0) && (adr == 0) )   // start
	   			{
		   		   start = (tuh+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2));
			   		stop = 1;                 // stop po zakonczeniu pomiaru
						zersrk(); 
				   }
   			   if( start == (FUH+2) ) // restart 
	   			{ 
         	      start = (TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); // ponowny pomiar
       	         stop = 1;                    // stop po zakonczeniu pomiaru
						zersrk(); 
				   }
				}
			}
         dusk0 = 0;
		}
   }
   dziel++;
   if( dziel >= MRUG )
   {
	   dziel = 0;
	   sync = 1;
//***** Sekwencja zalaczanie/wylaczanie napiec ****************
      if( start == (tuh+(TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) 
      {
	      if( lamptem.uhdef != 0)
		   {
		      uhset = lamptem.uhdef;
    			ihset = lamptem.ihdef = 0;
	   	}
		   if( lamptem.ihdef != 0)
   		{
	   	   ihset = lamptem.ihdef;
  		   	uhset = lamptem.uhdef = 0;
   		}
      }
      if( start == (    (TUG+TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
		{
			if( (lamptem.nazwa[7] == '2') || (lamptem.nazwa[7] == 29) ) { SETSEL; } // anoda 2		
		   ug1set = ug1ref - 11; // UgR
		}
      if( start == (    (    TUA+TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) )
		{
		   uaset = lamptem.uadef;
		}
      if( start == (    (        TUG2+TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug2set = lamptem.ug2def; }

// wystaw Uh/Ih   UgR   Ua   Ug2            UgL          Ug   UaL           UaR          Ua              Ug2    Ua   Ug   Uh/Ih   SPKON     [SPKOFF]     SPKON      SPKOFF STOP Tx
// czekaj      tuh   TUG  TUA   TUG2    TMAR   TUG   TMAR  TUG   TUA    TMAR   TUA   TMAR  TUA       TMAR   FUG2  FUA  FUG     FUH     BEEP-0      BEEP-1     BEEP-2      2    1  0
// czytaj                           IagR          IagL              IaaL          IaaR        Ug2/Ig2
// oblicz                                          S                               R      K

      if( start == (    (             TMAR+TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ugr = ug1; iar = (range==0)?ia:ia*10; } // IagR
      if( start == (    (                  TUG+TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug1set = ug1ref + 11; } // UgL
      if( start == (    (                      TMAR+TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ugl = ug1; ial = (range==0)?ia:ia*10; if( iar != ial ) { ial -= iar; ugr -= ugl; s = ial; s /= ugr; } else s = 999; } // IagL S
      if( start == (    (                           TUG+TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug1set = ug1ref; } // Ug
      if( start == (    (                               TUA+TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = lamptem.uadef - 10; } // UaL
      if( start == (    (                                   TMAR+TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ual = ua; ial = (range==0)?ia:ia*10; } // IaaL
      if( start == (    (                                        TUA+TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = lamptem.uadef + 10; } // UaR
      if( start == (    (                                            TMAR+TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uar = ua; iar = (range==0)?ia:ia*10; if( iar != ial ) { uar -= ual; uar *= 1000; iar -= ial; r = uar; r /= iar; } else r = 999; } // IaaR R
      if( start == (    (                                                 TUA+TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = lamptem.uadef; lint = s; lint *= r; lint += 5; lint /= 10; if( lint < 999 ) { k = (unsigned int)lint; } else k = 999; } // K=R*S
      if( start == (    (                                                     TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { uhlcd = uh; ihlcd = ih; ug1lcd = ug1; ualcd = ua; ialcd = ia; rangelcd = range; ug2lcd = ug2; ig2lcd = ig2; slcd = s; rlcd = r; klcd = k; txen = 1; }
      if( start == (    (                                                          FUG2+FUA+FUG+(BIP-0)+FUH+2)) ) { ug2set = 0; if( typ == 0 ) lamptem.ug2def = 0; }
      if( start == (    (                                                               FUA+FUG+(BIP-0)+FUH+2)) ) { uaset = 0; if( typ == 0 ) lamptem.uadef = 0; }
      if( start == (    (                                                                   FUG+(BIP-0)+FUH+2)) ) { ug1set = ug1zer; RSTSEL; }
      if( start == (    (                                                                       (BIP-0)+FUH+2)) ) { if( stop != 0 ) SPKON; }
      if( start == (    (                                                                       (BIP-1)+FUH+2)) ) { if( err != 0 ) { SPKOFF; uhset = ihset = 0; if( typ == 0 ) { lamptem.uhdef = lamptem.ihdef = 0; } } } // alarm - wylacz tez zarzenie
      if( start == (    (                                                                       (BIP-2)+FUH+2)) ) { if( stop != 0 ) SPKON; }
      if( start == (    (                                                                       (BIP-3)+FUH+2)) ) { SPKOFF; }
      if( start == (    (                                                                               FUH+1)) ) { uhset = ihset = 0; if( typ == 0 ) { lamptem.uhdef = lamptem.ihdef = 0; } }
      if( start == (    (                                                                                   1)) ) { err = 0; zersrk(); }

      takt++;
      takt &= 0x03;
      if( (takt == 2) && (dusk0 == DMAX) ) takt = 0;

		if( start == (FUH+2) ) 
		{
         if( stop == 0 ) start = (FUH+1);
		}
		else
		{
		   if( start > 0 ) { start--; }
      }
   }
}

//*************************************************************************
//                 O B S L U G A   W Y S W I E T L A C Z A
//*************************************************************************

void cmd2lcd( char rs, char bajt )
{
   delay( 1 );
   if( rs ) { RSSET; } else { RSRST; }
   PORTC &= 0x0f;
   PORTC |= ( bajt & 0xf0 );
   ENSET;
   NOP2; NOP2; NOP2; NOP2;
   ENRST;
   PORTC &= 0x0f;
   PORTC |= ( (bajt << 4) & 0xf0 );
   ENSET;
   NOP2; NOP2; NOP2; NOP2;
   ENRST;
}

void gotoxy( char x, char y )
{
   cmd2lcd( 0, 0x80 | (64 * (y % 2) + 20 * (y / 2) + x) ); // 4x20
}

void char2lcd( char f, char c )
{
   cmd2lcd( 1, ( (f == 1) && (takt == 0) )? ' ': c );
}

void cstr2lcd( char f, const unsigned char* c )
{
   while( *c ) { char2lcd( f, *c ); c++; }
}

void str2lcd( char f, unsigned char* c )
{
   while( *c ) { char2lcd( f, *c ); c++; }
}

void int2asc( unsigned int liczba )
{
   unsigned char
   i, temp;

   for( i = 0; i < 4; i++ )
   {
      temp = liczba % 10;
      liczba /= 10;
      ascii[i] = '0' + temp;
   }
}

//*************************************************************
//      Program glowny
//*************************************************************

void main(void)
{
//***** Konfiguracja portow ***********************************
                            //   7   6   5   4   3   2   1   0
                            // UG1 IG2 UG2  IA  UA  UH  IH REZ
   DDRA  = 0x00;            //   0   0   0   0   0   0   0   0
   PORTA = 0x00;            //   0   0   0   0   0   0   0   0
                            //  D7  D6  D5  K1  UH CKG SEL RNG
   DDRB  = 0x0f;            //   0   0   0   0   1   1   1   1
   PORTB = 0xf0;            //   1   1   1   1   0   0   0   0
                            //  D7  D6  D5  D4  ENA RS SDA SCL
   DDRC  = 0xfc;            //   1   1   1   1   1   1   0   0
   PORTC = 0x03;            //   0   0   0   0   0   0   1   1
                            // SPK DIR UG2  UA CLK  K0 TXD RXD
   DDRD  = 0xb2;            //   1   0   1   1   0   0   1   0
   PORTD = 0x4f;            //   0   1   0   0   1   1   1   1

//***** Konfiguracja procesora ********************************

   ACSR = BIT(ACD);                       // wylacz komparator

//***** Konfiguracja timerow **********************************

   TCCR2 = BIT(FOC2)|BIT(WGM21)|BIT(CS22);  // FOC2/CTC,XTAL/64
   OCR2 = MS1;                                           // 1ms

//***** Konfiguracja wyjsc PWM ********************************

   TCCR0 = BIT(WGM01)|BIT(WGM00)|BIT(CS00); // FAST,XTAL, OC0 odlaczone

   TCCR1A = BIT(COM1A1)|BIT(COM1B1);                    // PWM
   TCCR1B = BIT(WGM13)|BIT(CS10);      // PHASE+FREQ,ICR1,XTAL

//***** Konfiguracja przetwornika ADC *************************

   ADMUX = ADRUG1;
   ADCSRA = BIT(ADEN)|BIT(ADSC)|BIT(ADATE)|BIT(ADIF)|BIT(ADIE)|BIT(ADPS2)|BIT(ADPS1)|BIT(ADPS0);

//***** Konfiguracja portu szeregowego ************************

   UBRRL = RATE;                             // ustaw szybkosc
   UCSRB = BIT(RXCIE)|BIT(RXEN)|BIT(TXCIE)|BIT(TXEN);
   UCSRC = BIT(URSEL)|BIT(UCSZ1)|BIT(UCSZ0);

//***** Inicjalizacja watchdoga *******************************

   WDTCR = BIT(WDE);
   WDTCR = BIT(WDE)|BIT(WDP2)|BIT(WDP1);
   WDR;

//***** Inicjalizacja przerwan ********************************

   MCUCR = BIT(ISC11);                       // INT1 na zboczu
   TIMSK = BIT(OCIE2);                               // T2 CTC
   GIFR = BIT(INTF1);                  // skasuj znacznik INT1

//***** Inicjalizacja zmiennych *******************************

   vref = 509;                                  // vref >= 480
   TOPPWM = 61 * vref / 100;             // okres PWM Ua i Ug2

   ug1set = ug1zer = ug1ref = liczug1( 240 );;    // -24.0V
   lamptem.ug1def = 240;

	cwartmax = (ELAMP+FLAMP-1);  // wszystkie lampy
   cwart = &typ;                               // wskaz na Typ
   EEPROM_READ((int)&poptyp, typ); // ustaw na ostatnio aktywny
	ADMUX = ADRIH;

   SEI;                                    // wlacz przerwania

//***** Inicjalizacja wyswietlacza LCD ************************

   delay( 30 );                                        // 30ms
   cmd2lcd( 0, 0x28 );
   cmd2lcd( 0, 0x06 );
   cmd2lcd( 0, 0x0c );
   cmd2lcd( 0, 0x01 );
   cmd2lcd( 0, 0x40 );

   gotoxy( 4, 0 ); cstr2lcd( 0, "VTTester 1.16" );
   gotoxy( 4, 2 ); cstr2lcd( 0, "Adam   Tomasz" );
   gotoxy( 4, 3 ); cstr2lcd( 0, "Tatus   Gumny" );
   for( i = 0; i < 20; i++ ) { WDR; if( i == 19 ) { SPKON; } delay( 100 ); SPKOFF; };

   gotoxy( 0, 0 ); cstr2lcd( 0, "             G     V" );
   gotoxy( 0, 1 ); cstr2lcd( 0, "H=    V A=    G2=   " );
   gotoxy( 0, 2 ); cstr2lcd( 0, "      mA            " );

// 01234567890123456789
// ====================
// 01 ECC83_J29 G=24.0V
// H=12.6V A=300 G2=300
// T 2550mA 199.9 19.99
// S=99.9 u=99.9 R=99.9
// ====================

   cstr2rs( "\r\nPress <ESC> to get LCD copy\r\nNr Type Uh[V] Ih[mA] -Ug[V] Ua[V] Ia[mA] Ug2[V] Ig2[mA] S[mA/V] R[k] K[V/V]" );

//***** Glowna petla programu *********************************

   while( 1 )
   {
	   WDR;
	   if ( sync == 1 )
		{
		   sync = 0;
//***** Wyswietlenie zawartosci bufora ************************

         gotoxy( 0, 0 );                           	// Numer
         str2lcd( ((adr == 0) && (start != (FUH+2))), &buf[0] );

         gotoxy( 3, 0 );	                           // Nazwa
		   for( i = 0; i < 9; i++ ) char2lcd( (((adr-1) == i) || ((i == 7) && (start == (FUH+2)) && ((buf[10] == '1') || (buf[10] == '2')) )), buf[i+3] );

         gotoxy( 14, 0 );
			char2lcd( (adr == 10), '-' );
		   str2lcd( (adr == 10), &buf[23] );                // Ug1

         gotoxy( 2, 1 );
		   str2lcd( (adr == 11), &buf[13] );                 // Uh

         gotoxy( 0, 2 );
			errcode = ' ';
			if( (err & OVERIH) == OVERIH ) errcode = 'H';
			if( (err & OVERIA) == OVERIA ) errcode = 'A';
			if( (err & OVERIG) == OVERIG ) errcode = 'G';
			if( (err & OVERTE) == OVERTE ) errcode = 'T';
		   char2lcd( 0, errcode );                           // Err
			
         gotoxy( 2, 2 );
		   str2lcd( (adr == 12), &buf[18] );                 // Ih

         gotoxy( 10, 1 );
		   str2lcd( (adr == 13), &buf[28] );                 // Ua

         gotoxy( 9, 2 );
  		   str2lcd( (adr == 14), &buf[32] );                 // Ia

         gotoxy( 17, 1 );
         str2lcd( (adr == 15), &buf[38] );                // Ug2

         gotoxy( 15, 2 );
		   str2lcd( (adr == 16), &buf[42] );                // Ig2

		   gotoxy( 0, 3 );
         if( typ > 1 )
			{
            gotoxy( 0, 3 );
		      cstr2lcd( 0, "S=" );
		      str2lcd( (adr == 17), &buf[48] );                 // S

            gotoxy( 6, 3 );
		      cstr2lcd( 0, " R=" );
		      str2lcd( (adr == 18), &buf[53] );                 // R

            gotoxy( 13, 3 );
				if( (start <= (FUH+2)) || (dusk0 == DMAX) )
				{
		         cstr2lcd( 0, " K=" );
		         str2lcd( (adr == 19), &buf[58] );                 // K
            }
				else
				{
  		         cstr2lcd( 0, " T=" );
               int2asc( start >> 2 );
					
			      if( ascii[2] != '0' )
               {
					   char2lcd( 0, ascii[2] );
						char2lcd( 0, ascii[1] );
               }
               else
               {
                  char2lcd( 0, ' ' );
                  if( ascii[1] != '0' )
                  {
                     char2lcd( 0, ascii[1] );
                  }
                  else
			         {
	                  char2lcd( 0, ' ' );
			         }
               }
               char2lcd( 0, ascii[0] );
   				char2lcd( 0, 's' );
				}
			}
			else
			{
				cstr2lcd( 0, "* OVERHEAT Warning *" );
			}
      }
      GICR = BIT(INT1);                             // wlacz INT1
  		if( (err != 0) && ((start == 0) || (start > (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2))) ) { stop = 1; start = (TMAR+FUG2+FUA+FUG+(BIP-0)+FUH+2); } // awaryjne wylaczenie

//***** Wyswietlanie Numeru ***********************************
      if( adr == 0 )                      // ustawianie numeru
      {
         int2asc( typ );
         buf[0] = ascii[1];
         buf[1] = ascii[0];
//***** Pobieranie nowej Nazwy ************************************
         if( typ < FLAMP )
     	   {
     	      lamptem = lamprom[typ];
		   	tuh = (lamptem.nazwa[8] - '0') * 240;    // 240 = 1min
     	   	for( i = 0; i < 9; i++ ) buf[i+3] = (unsigned char)lamptem.nazwa[i];
  	   	}
         else
  		   {
            EEPROM_READ((int)&lampeep[typ-FLAMP], lamptem);
	   	   tuh = (lamptem.nazwa[8] - 27) * 240;     // 240 = 1min
     		 	for( i = 0; i < 9; i++ ) buf[i+3] = AZ[(unsigned char)lamptem.nazwa[i]];
         }
			nowa = 1;                     // zaktualizowano nazwe
		}

//***** Edycja Nazwy ******************************************
      if( (adr > 0) && (adr < 10) )            // edycja nazwy
      {
//***** Wyswietlanie edytowanej Nazwy *************************
  			if( czytaj == 1 ) EEPROM_READ((int)&lampeep[typ-FLAMP].nazwa, lamptem.nazwa);
			for( i = 0; i < 9; i++ ) buf[3+i] = AZ[(unsigned char)lamptem.nazwa[i]];
			czytaj = 0;
		   if( dusk0 == DMAX ) zapisz = 1;
		   if( (nodus == DMIN) && (zapisz == 1) )
			{
			   EEPROM_WRITE((int)&lampeep[typ-FLAMP].nazwa[adr-1], lamptem.nazwa[adr-1]);
			   zapisz = 0;
			}
		}
		else
		   czytaj = 1;
//***** Ustawianie Ug1 ****************************************

      ug1ref = liczug1( lamptem.ug1def );               // przelicz Ug1
		
		temp = mug1adc;
		temp *= 725;
		licz = 40960000;
		if( licz > temp ) { licz -= temp; } else licz = 0;
      licz >>= 16;     //   /= 65536;
		licz *= vref;
		licz /= 1000;
		ug1 = (unsigned int)licz;
      if( start == (FUH+2) ) licz = ug1lcd;
      if( (adr == 0) || (adr == 10) )
		{
		   if( dusk0 == DMAX )
		   {
            licz = lamptem.ug1def;              // 0..250..300
				zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 10) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
            if( typ == 0 )                           // SUPPLY
				{
   			   ug1set = ug1ref;
				}
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].ug1def, lamptem.ug1def);
				}
			}
		}
//***** Wyswietlanie Ug1 **************************************
      int2asc( (unsigned int)licz );
      buf[23] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[24] = ascii[1];
      buf[25] = '.';
      buf[26] = ascii[0];
//***** Ustawianie Uh *****************************************
      licz = muhadc;
		licz *= vref;
		licz >>= 14;   //  /= 16384;                      // 0..200
      temp = mihadc;   // poprawka na spadek napiecia na boczniku
		temp *= vref;
		temp >>= 16;         //    /= 65536;
		if( licz > temp ) { licz -= temp; } else licz = 0;
		licz /= 10;
		uh = (unsigned int)licz;
      if( start == (FUH+2) ) licz = uhlcd;
      if( (adr == 0) || (adr == 11) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.uhdef;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 11) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
            if( typ == 0 )                           // SUPPLY
				{
				   uhset = lamptem.uhdef;
					ihset = lamptem.ihdef = 0;
				}
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].uhdef, lamptem.uhdef);
				}
			}
		}
//***** Wyswietlanie Uh ***************************************
      int2asc( (unsigned int)licz );
      buf[13] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[14] = ascii[1];
      buf[15] = '.';
      buf[16] = ascii[0];
//***** Ustawianie Ih *****************************************
      licz = mihadc;
		licz *= vref;
		licz >>= 15;    //   /= 32768;                  // 0..250
		ih = (unsigned int)licz;
      if( start == (FUH+2) ) licz = ihlcd;
      if( (adr == 0) || (adr == 12) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.ihdef;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 12) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
            if( typ == 0 )                           // SUPPLY
				{
				   ihset = lamptem.ihdef;
					uhset = lamptem.uhdef = 0;
				}
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].ihdef, lamptem.ihdef);
				}
			}
		}
//***** Wyswietlanie Ih ***************************************
      int2asc( (unsigned int)licz );
      if( ascii[2] != '0' )
      {
         buf[18] = ascii[2];
         buf[19] = ascii[1];
         buf[20] = ascii[0];
      }
      else
      {
         buf[18] = ' ';
         if( ascii[1] != '0' )
         {
            buf[19] = ascii[1];
            buf[20] = ascii[0];
         }
         else
			{
	         buf[19] = ' ';
            buf[20] = (ascii[0] != '0')? ascii[0]: ' ';
			}
      }
      buf[21] = '0';
//***** Ustawianie Ua *****************************************
      licz = muaadc;
		licz *= vref;
		licz /= 107436;
		ua = (unsigned int)licz;
      if( start == (FUH+2) ) licz = ualcd;
      if( (adr == 0) || (adr == 13) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.uadef;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 13) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
            if( typ == 0 )                           // SUPPLY
				{
				   uaset = lamptem.uadef;
				}
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].uadef, lamptem.uadef);
				}
			}
		}
//***** Wyswietlanie Ua ***************************************
      int2asc( (unsigned int)licz );
      if( ascii[2] != '0' )
      {
         buf[28] = ascii[2];
         buf[29] = ascii[1];
      }
      else
      {
         buf[28] = ' ';
         buf[29] = (ascii[1] != '0')? ascii[1]: ' ';
      }
      buf[30] = ascii[0];
//***** Ustawianie Ia ***************************************
	   licz = miaadc;
		licz *= vref;
		licz >>= 14;       //    /= 16384;
      temp = muaadc;
		temp *= vref;
		if( range == 0 ) { temp *= 10; }
		temp /= 4369064;
      if( licz > temp ) { licz -= temp; } else licz = 0;
		ia = (unsigned int)licz;
      if( start == (FUH+2) ) licz = ialcd;
		rangedef = 0;
      if( (adr == 0) || (adr == 14) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.iadef;
				rangedef = 1;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 14) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].iadef, lamptem.iadef);
				}
			}
		}
//***** Wyswietlanie Ia ***************************************
      int2asc( (unsigned int)licz );
		if( (rangedef == 0) && (((start != (FUH+2)) && (range == 0)) || ((start == (FUH+2)) && (rangelcd == 0))) )
		{
         buf[32] = (ascii[3] != '0')? ascii[3]: ' ';
			buf[33] = ascii[2];
	   	buf[34] = '.';
         buf[35] = ascii[1];
         buf[36] = ascii[0];
	   }
		else
		{
         if( ascii[3] != '0' )
         {
            buf[32] = ascii[3];
            buf[33] = ascii[2];
         }
         else
         {
            buf[32] = ' ';
            buf[33] = (ascii[2] != '0')? ascii[2]: ' ';
         }
         buf[34] = ascii[1];
		   buf[35] = '.';
         buf[36] = ascii[0];
      }
//***** Ustawianie Ug2 ****************************************
      licz = mug2adc;
		licz *= vref;
		licz /= 107436;
		ug2 = (unsigned int)licz;
      if( start == (FUH+2) ) licz = ug2lcd;
      if( (adr == 0) || (adr == 15) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.ug2def;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 15) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
            if( typ == 0 )                           // SUPPLY
				{
				   ug2set = lamptem.ug2def;
				}
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].ug2def, lamptem.ug2def);
				}
			}
		}
//***** Wyswietlanie Ug2 **************************************
      int2asc( (unsigned int)licz );
      if( ascii[2] != '0' )
      {
         buf[38] = ascii[2];
         buf[39] = ascii[1];
      }
      else
      {
         buf[38] = ' ';
         buf[39] = (ascii[1] != '0')? ascii[1]: ' ';
      }
      buf[40] = ascii[0];
//***** Ustawianie Ig2 **************************************
	   licz = mig2adc;
		licz *= vref;
		licz >>= 13;   //  /8192(40mA)  /16384(20mA);
      temp = mug2adc;
		temp *= vref;
		temp *= 10;
		temp /= 4369064;
      if( licz > temp ) { licz -= temp; } else licz = 0;
		ig2 = (unsigned int)licz;
      if( start == (FUH+2) ) licz = ig2lcd;
      if( (adr == 0) || (adr == 16) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.ig2def;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 16) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].ig2def, lamptem.ig2def);
				}
			}
		}
//***** Wyswietlanie Ig2 **************************************
      int2asc( (unsigned int)licz );
      buf[42] = (ascii[3] != '0')? ascii[3]: ' ';
      buf[43] = ascii[2];
      buf[44] = '.';
      buf[45] = ascii[1];
      buf[46] = ascii[0];
//***** Ustawianie S ****************************************
      licz = s;
      if( start == (FUH+2) ) licz = slcd;
      if( (adr == 0) || (adr == 17) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.sdef;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 17) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].sdef, lamptem.sdef);
				}
			}
		}
//***** Wyswietlanie S ****************************************
      int2asc( licz );
      buf[48] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[49] = ascii[1];
      buf[50] = '.';
      buf[51] = ascii[0];
//***** Ustawianie R ****************************************
      licz = r;
      if( start == (FUH+2) ) licz = rlcd;
      if( (adr == 0) || (adr == 18) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.rdef;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 18) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].rdef, lamptem.rdef);
				}
			}
		}
//***** Wyswietlanie R ****************************************
      int2asc( licz );
      buf[53] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[54] = ascii[1];
      buf[55] = '.';
      buf[56] = ascii[0];
//***** Ustawianie K ****************************************
      licz = k;
      if( start == (FUH+2) ) licz = klcd;
      if( (adr == 0) || (adr == 19) )
		{
		   if( dusk0 == DMAX )
		   {
   		   licz = lamptem.kdef;
   		   zapisz = 1;
		   }
         if( (nodus == DMIN) && (adr == 19) && (zapisz == 1) )
			{
  		 	   zapisz = 0;
  				if( typ >= FLAMP )                        // ELAMP
				{
   		      EEPROM_WRITE((int)&lampeep[typ-FLAMP].kdef, lamptem.kdef);
				}
			}
		}
//***** Wyswietlanie K ****************************************
      int2asc( licz );
      buf[58] = (ascii[2] != '0')? ascii[2]: ' ';
      buf[59] = ascii[1];
      buf[60] = '.';
      buf[61] = ascii[0];
//***** Wyslanie pomiarow do PC *******************************
      if( txen )
      {
		   EEPROM_WRITE((int)&poptyp, typ);
		   cstr2rs( "\r\n" );
		   for( i = 0; i < 62; i++ )
			{
            if( buf[i] != '\0' ) char2rs( buf[i] ); else cstr2rs( "  " );
			}
         txen = 0;
      }
   }
}
