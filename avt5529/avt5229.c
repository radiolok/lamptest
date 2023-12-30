#include "definitions.h"
#include "lcd.h"
#include "util.h"
#include "lamprom.h"

unsigned char currentLampNum;
unsigned char
	d,
	i,
	busy,
	sync,
	txen,
	*cwart, cwartmin, cwartmax,
	adr, adrmin, adrmax,
	nowa,
	stop,
	zwloka,
	dziel,
	nodus, dusk0,
	zapisz, czytaj,
	range, rangelcd, rangedef,
	ascii[5],
	kanal,
	overih, overia, overig2, err, errcode,
	probki,
	pwm,
	buf[63];

unsigned int
	*wart,
	wartmin, wartmax,
	start, tuh,
	vref,
	adcih, adcia, adcig2,
	uhset, ihset, ug1set, uaset, iaset, ug2set, ig2set,
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
	lint,tint, licz, temp;

katalog_t currentLampData;

const unsigned char
	AZ[37] =
		{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
       // 00   01  02   03   04   05   06    07   08  09   10    11  12   13   14    15  16   17    18   19  20   21    22  23    24  25    26   27  28   29    30  31    32  33   34   35   36


unsigned int poptyp = 0;

void readPgmLampData(unsigned char _currentLampNum, katalog_t *lampData)
{
	unsigned char *lampDataPtr = (unsigned char *)lampData;
	unsigned char *lampRomPtr = (unsigned char *)lamprom + _currentLampNum * sizeof(katalog_t);
	for (uint8_t i = 0; i < sizeof(katalog_t); ++i, lampDataPtr++, lampRomPtr++)
	{
		*lampDataPtr = pgm_read_byte(lampRomPtr);
	}
}

//*************************************************************************
//                 F U N K C J E  P O M O C N I C Z E
//*************************************************************************

void char2rs(unsigned char bajt)
{
	UDR = bajt;
	busy = 1;
	while (busy)
		; // czekaj na koniec wysylania bajtu
}

void cstr2rs(const char *q)
{
	while (*q) // do konca stringu
	{
		UDR = *q;
		q++;
		busy = 1;
		while (busy)
			; // czekaj na koniec wysylania bajtu
	}
}

void delay(unsigned char opoz) // opoznienie *1ms
{
	zwloka = opoz + 1;
	while (zwloka != 0)
		;
}

void zersrk(void) // zeruj S, R, K
{
	s = r = k = ualcd = ialcd = ug2lcd = ig2lcd = slcd = rlcd = klcd = 0;
}

unsigned int liczug1(unsigned int pug1) // przelicz Ug1
{
	licz = 640000;
	licz *= vref;
	temp = 1024000;
	temp *= pug1; // ug1
	licz -= temp;
	licz /= 725;
	licz /= vref;
	return ((unsigned int)licz); // 882..193..55
}

//*************************************************************************
//                 O B S L U G A   P R Z E R W A N
//*************************************************************************

ISR(INT1_vect)
{
	if (start > (TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2))
	{
		if (nodus == DMIN)
		{
			start = (TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2); // wylaczanie kreciolkiem
			stop = 0;
		}
	}
	else
	{
		if (start == (FUH + 2))
		{
			if (dusk0 == DMAX)
			{
				if (RIGHT)
				{
					if ((nowa == 1) && ((currentLampData.name[7] == '1') || (currentLampData.name[7] == 28)))
					{
						currentLampNum++;
						nowa = 0;
					}
				}
				else
				{
					if ((nowa == 1) && ((currentLampData.name[7] == '2') || (currentLampData.name[7] == 29)))
					{
						currentLampNum--;
						nowa = 0;
					}
				}
				zersrk();
			}
			if (nodus == DMIN)
			{
				start = (FUH + 1);
			}
		}
		else
		{
			if (start == 0)
			{
				if (adr == 0) // edycja numeru
				{
					cwartmin = 0;
					cwartmax = (FLAMP + ELAMP - 1);
					cwart = &currentLampNum;
				}
				if ((adr > 0) && (adr < 7)) // edycja nazwy
				{
					cwartmin = 0;
					cwartmax = 36;
					cwart = &currentLampData.name[adr - 1];
				}
				if (adr == 7) // zmiana nr podstawki zarzenia
				{
					cwartmin = 0;
					cwartmax = 9;
					cwart = &currentLampData.name[6];
				}
				if (adr == 8) // zmiana nr systemu elektrod
				{
					cwartmin = 27;
					cwartmax = 29;
					cwart = &currentLampData.name[7];
				}
				if (adr == 9) // ustawianie czasu zarzenia
				{
					cwartmin = 28;
					cwartmax = 36;
					cwart = &currentLampData.name[8];
				}
				if (adr == 10) // ustawianie Ug1
				{
					cwartmin = (currentLampNum == 0) ? 0 : 5;		// 0.5 lub 0
					cwartmax = (currentLampNum == 0) ? 240 : 235; // -23.5 lub -24.0V
					cwart = &currentLampData.ug1def;
				}
				if (adr == 11) // ustawianie Uh
				{
					cwartmin = 0;
					cwartmax = 150; // 0..15.0V
					cwart = &currentLampData.uhdef;
				}
				if (adr == 12) // ustawianie Ih
				{
					cwartmin = 0;
					cwartmax = 250; // 0..2.50A
					cwart = &currentLampData.ihdef;
				}
				if (adr == 13) // ustawianie Ua
				{
					wartmin = (currentLampNum == 0) ? 0 : 10;
					wartmax = (currentLampNum == 0) ? 300 : 290; // 0..300V
					wart = &currentLampData.uadef;
				}
				if (adr == 14) // ustawianie Ia
				{
					wartmin = 0;
					wartmax = 2000; // 0..200.0mA
					wart = &currentLampData.iadef;
				}
				if (adr == 15) // ustawianie Ug2
				{
					wartmin = 0;
					wartmax = 300; // 0..300V
					wart = &currentLampData.ug2def;
				}
				if (adr == 16) // ustawianie Ig2
				{
					wartmin = 0;
					wartmax = 4000; // 0..40.00mA
					wart = &currentLampData.ig2def;
				}
				if (adr == 17) // ustawianie S
				{
					wartmin = 0;
					wartmax = 999; // 99.9
					wart = &currentLampData.sdef;
				}
				if (adr == 18) // ustawianie R
				{
					wartmin = 0;
					wartmax = 999; // 99.9
					wart = &currentLampData.rdef;
				}
				if (adr == 19) // ustawianie K
				{
					wartmin = 0;
					wartmax = 999; // 99.9
					wart = &currentLampData.kdef;
				}

				if (RIGHT)
				{
					if (adr < 13)
					{
						if (dusk0 == DMAX)
						{
							if ((*cwart) < cwartmax)
							{
								(*cwart)++;
							}
							else
							{
								if (adr == 0)
								{
									(*cwart) = 0;
								}
							}
						}
					}
					else
					{
						if ((dusk0 == DMAX) && ((*wart) < wartmax))
						{
							(*wart)++;
						}
					}
					if (nodus == DMIN)
					{
						if (currentLampNum == 0)
						{
							if (adr == 0)
							{
								adr += 9;
							} // przeskocz nazwe
							if (adr == 13)
							{
								adr++;
							} // przeskocz Ia
							if (adr < 15)
							{
								adr++;
							} // nie dojezdzaj do Ig2
						}
						if (currentLampNum >= FLAMP)
						{
							if (adr < 19)
							{
								adr++;
							}
						}
					}
				}
				else
				{
					if (adr < 13)
					{
						if (dusk0 == DMAX)
						{
							if ((*cwart) > cwartmin)
							{
								(*cwart)--;
							}
							else
							{
								if (adr == 0)
								{
									(*cwart) = (FLAMP + ELAMP - 1);
								}
							}
						}
					}
					else
					{
						if ((dusk0 == DMAX) && ((*wart) > wartmin))
						{
							(*wart)--;
						}
					}
					if ((nodus == DMIN) && (adr > 0))
					{
						if ((currentLampNum == 0) && (adr == 10))
						{
							adr -= 9;
							stop = 0;
							start = (TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2);
						} // przeskocz nazwe / wylaczenie
						if ((currentLampNum == 0) && (adr == 15))
						{
							adr--;
						}
						if ((currentLampNum == 0) || (currentLampNum >= FLAMP))
						{
							adr--;
						}
					}
				}
			}
		}
	}
}

ISR(ADC_vect)
{
	switch (kanal)
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
		if (ADC >= ug1set)
		{
			CLKUG1RST;
		}
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
		if (adcih > 400)
		{
			if (overih > 0)
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
		if (ADC >= ug1set)
		{
			CLKUG1RST;
		}
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
		if (ADC >= ug1set)
		{
			CLKUG1RST;
		}
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
		if (ADC >= ug1set)
		{
			CLKUG1RST;
		}
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
		if ((range != 0) && (adcia >= 1020))
		{
			if (overia > 0)
			{
				overia--;
			}
			else
			{

				uaset = ug2set = 0;
				PWMUA = PWMUG2 = 0;
				;
				err |= OVERIA;
			}
		}
		else
		{
			overia = OVERSAMP;
		}
		//***** Wystawienie Ua ****************************************
		if (PWMUA < uaset)
			PWMUA++;
		if (PWMUA > uaset)
			PWMUA--;
		//***** Ustawianie wysokiego zakresu pomiarowego Ia ***********
		if ((range == 0) && (adcia > 950))
		{
			range = 1;
			SET200;
		}
		//***** Ustawianie niskiego zakresu pomiarowego Ia ************
		if (((err & OVERIA) == 0) && (range != 0) && (adcia < 85))
		{
			range = 0;
			RST200;
		}
		break;
	}
	case 9:
	{
		kanal = 10;
		if (ADC >= ug1set)
		{
			CLKUG1RST;
		}
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
		if (ADC >= ug1set)
		{
			CLKUG1RST;
		}
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
		if (adcig2 >= 1020)
		{
			if (overig2 > 0)
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
		if (PWMUG2 < ug2set)
			PWMUG2++;
		if (PWMUG2 > ug2set)
			PWMUG2--;
		break;
	}
	case 13:
	{
		sug1adc += ADC;
		kanal = 0;
		if (ADC >= ug1set)
		{
			CLKUG1RST;
		}
		ADMUX = ADRUG1;
		if (probki == LPROB)
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
			if ((uhset == 0) && (ihset == 0))
				pwm = 0;
			else
			{
				TCCR0 |= BIT(COM01); // dolacz OC0
									 //***** Stabilizacja Uh ***************************************
				if (uhset > 0)
				{
					lint = muhadc;
					lint *= vref;
					lint >>= 14;   //  /= 16384;                // 0..200
					tint = mihadc; // poprawka na spadek napiecia na boczniku
					tint *= vref;
					tint >>= 16; //  /= 65536;
					if (lint > tint)
					{
						lint -= tint;
					}
					else
					{
						lint = 0;
					}
					lint /= 10;
					if ((uhset > (unsigned int)lint) && (pwm < 255))
					{
						pwm++;
					}
					if ((uhset < (unsigned int)lint) && (pwm > 0))
					{
						pwm--;
					}
				}
				//***** Stabilizacja Ih ***************************************
				if (ihset > 0)
				{
					lint = mihadc;
					lint *= vref;
					lint >>= 15; //   /= 32768;
					if ((ihset > (unsigned int)lint))
					{
						if ((pwm < 8) || ((mihadc > 32) && (pwm < 255)))
						{
							pwm++;
						} // Uh<0.5V lub Ih>5mA
					}
					if ((ihset < (unsigned int)lint) && (pwm > 0))
					{
						pwm--;
					}
				}
			}
			if (pwm == 0)
			{
				TCCR0 &= ~BIT(COM01); // odlacz OC0
			}
			else
			{
				TCCR0 |= BIT(COM01); // dolacz OC0
			}
			PWMUH = pwm;
			//***** Ustawianie/kasowanie znacznika przegrzania ************
			if (mrezadc > HITEMP)
				err |= OVERTE;
			if (mrezadc < LOTEMP)
				err &= ~OVERTE;
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

ISR(USART_TXC_vect)
{
	busy = 0;
}

ISR(USART_RXC_vect)
{
	if (UDR == ESC)
		txen = 1;
}

ISR(TIMER2_COMP_vect)
{
	if (zwloka != 0)
	{
		zwloka--;
	}

	if (DUSK0)
	{
		if (dusk0 < DMAX)
		{
			dusk0++;
		}
		else
		{
			nodus = 0;
		}
	}
	else
	{
		if (nodus < DMIN)
		{
			nodus++;
		}
		else
		{
			if ((dusk0 > DMIN) && (dusk0 < DMAX))
			{
				if ((currentLampNum > 1) && (err == 0))
				{
					if ((start == 0) && (adr == 0)) // start
					{
						start = (tuh + (TUG + TUA + TUG2 + TMAR + TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2));
						stop = 1; // stop po zakonczeniu pomiaru
						zersrk();
					}
					if (start == (FUH + 2)) // restart
					{
						start = (TUG + TUA + TUG2 + TMAR + TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2); // ponowny pomiar
						stop = 1;																															  // stop po zakonczeniu pomiaru
						zersrk();
					}
				}
			}
			dusk0 = 0;
		}
	}
	dziel++;
	if (dziel >= MRUG)
	{
		dziel = 0;
		sync = 1;
		//***** Sekwencja zalaczanie/wylaczanie napiec ****************
		if (start == (tuh + (TUG + TUA + TUG2 + TMAR + TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			if (currentLampData.uhdef != 0)
			{
				uhset = currentLampData.uhdef;
				ihset = currentLampData.ihdef = 0;
			}
			if (currentLampData.ihdef != 0)
			{
				ihset = currentLampData.ihdef;
				uhset = currentLampData.uhdef = 0;
			}
		}
		if (start == ((TUG + TUA + TUG2 + TMAR + TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			if ((currentLampData.name[7] == '2') || (currentLampData.name[7] == 29))
			{
				SETSEL;
			}					  // anoda 2
			ug1set = ug1ref - 11; // UgR
		}
		if (start == ((TUA + TUG2 + TMAR + TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			uaset = currentLampData.uadef;
		}
		if (start == ((TUG2 + TMAR + TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			ug2set = currentLampData.ug2def;
		}

		// wystaw Uh/Ih   UgR   Ua   Ug2            UgL          Ug   UaL           UaR          Ua              Ug2    Ua   Ug   Uh/Ih   SPKON     [SPKOFF]     SPKON      SPKOFF STOP Tx
		// czekaj      tuh   TUG  TUA   TUG2    TMAR   TUG   TMAR  TUG   TUA    TMAR   TUA   TMAR  TUA       TMAR   FUG2  FUA  FUG     FUH     BEEP-0      BEEP-1     BEEP-2      2    1  0
		// czytaj                           IagR          IagL              IaaL          IaaR        Ug2/Ig2
		// oblicz                                          S                               R      K

		if (start == ((TMAR + TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			ugr = ug1;
			iar = (range == 0) ? ia : ia * 10;
		} // IagR
		if (start == ((TUG + TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			ug1set = ug1ref + 11;
		} // UgL
		if (start == ((TMAR + TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			ugl = ug1;
			ial = (range == 0) ? ia : ia * 10;
			if (iar != ial)
			{
				ial -= iar;
				ugr -= ugl;
				s = ial;
				s /= ugr;
			}
			else
				s = 999;
		} // IagL S
		if (start == ((TUG + TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			ug1set = ug1ref;
		} // Ug
		if (start == ((TUA + TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			uaset = currentLampData.uadef - 10;
		} // UaL
		if (start == ((TMAR + TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			ual = ua;
			ial = (range == 0) ? ia : ia * 10;
		} // IaaL
		if (start == ((TUA + TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			uaset = currentLampData.uadef + 10;
		} // UaR
		if (start == ((TMAR + TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			uar = ua;
			iar = (range == 0) ? ia : ia * 10;
			if (iar != ial)
			{
				uar -= ual;
				uar *= 1000;
				iar -= ial;
				r = uar;
				r /= iar;
			}
			else
				r = 999;
		} // IaaR R
		if (start == ((TUA + TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			uaset = currentLampData.uadef;
			lint = s;
			lint *= r;
			lint += 5;
			lint /= 10;
			if (lint < 999)
			{
				k = (unsigned int)lint;
			}
			else
				k = 999;
		} // K=R*S
		if (start == ((TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			uhlcd = uh;
			ihlcd = ih;
			ug1lcd = ug1;
			ualcd = ua;
			ialcd = ia;
			rangelcd = range;
			ug2lcd = ug2;
			ig2lcd = ig2;
			slcd = s;
			rlcd = r;
			klcd = k;
			txen = 1;
		}
		if (start == ((FUG2 + FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			ug2set = 0;
			if (currentLampNum == 0)
				currentLampData.ug2def = 0;
		}
		if (start == ((FUA + FUG + (BIP - 0) + FUH + 2)))
		{
			uaset = 0;
			if (currentLampNum == 0)
				currentLampData.uadef = 0;
		}
		if (start == ((FUG + (BIP - 0) + FUH + 2)))
		{
			ug1set = ug1zer;
			RSTSEL;
		}
		if (start == (((BIP - 0) + FUH + 2)))
		{
			if (stop != 0)
				SPKON;
		}
		if (start == (((BIP - 1) + FUH + 2)))
		{
			if (err != 0)
			{
				SPKOFF;
				uhset = ihset = 0;
				if (currentLampNum == 0)
				{
					currentLampData.uhdef = currentLampData.ihdef = 0;
				}
			}
		} // alarm - wylacz tez zarzenie
		if (start == (((BIP - 2) + FUH + 2)))
		{
			if (stop != 0)
				SPKON;
		}
		if (start == (((BIP - 3) + FUH + 2)))
		{
			SPKOFF;
		}
		if (start == ((FUH + 1)))
		{
			uhset = ihset = 0;
			if (currentLampNum == 0)
			{
				currentLampData.uhdef = currentLampData.ihdef = 0;
			}
		}
		if (start == ((1)))
		{
			err = 0;
			zersrk();
		}
		lcdBlink(dusk0 == DMAX);

		if (start == (FUH + 2))
		{
			if (stop == 0)
				start = (FUH + 1);
		}
		else
		{
			if (start > 0)
			{
				start--;
			}
		}
	}
}

void ioSetup(void){
    //***** IO config ***********************************
	//   7   6   5   4   3   2   1   0
	// UG1 IG2 UG2  IA  UA  UH  IH REZ
	DDRA = 0x00;  //   0   0   0   0   0   0   0   0
	PORTA = 0x00; //   0   0   0   0   0   0   0   0
				  //  D7  D6  D5  K1  UH CKG SEL RNG
	DDRB = 0x0f;  //   0   0   0   0   1   1   1   1
	PORTB = 0xf0; //   1   1   1   1   0   0   0   0
				  //  D7  D6  D5  D4  ENA RS SDA SCL
	DDRC = 0xfc;  //   1   1   1   1   1   1   0   0
	PORTC = 0x03; //   0   0   0   0   0   0   1   1
				  // SPK DIR UG2  UA CLK  K0 TXD RXD
	DDRD = 0xb2;  //   1   0   1   1   0   0   1   0
	PORTD = 0x4f; //   0   1   0   0   1   1   1   1

	//***** Processor configuration ********************************
	ACSR = BIT(ACD); // wylacz komparator

	//***** Timer configuration **********************************
	TCCR2 = BIT(FOC2) | BIT(WGM21) | BIT(CS22); // FOC2/CTC,XTAL/64
	OCR2 = MS1;									// 1ms

	//***** PWM output configuration ********************************
	TCCR0 = BIT(WGM01) | BIT(WGM00) | BIT(CS00); // FAST,XTAL, OC0 odlaczone
	TCCR1A = BIT(COM1A1) | BIT(COM1B1); // PWM
	TCCR1B = BIT(WGM13) | BIT(CS10);	// PHASE+FREQ,ICR1,XTAL

	//***** ADC configuration *************************
	ADMUX = ADRUG1;
	ADCSRA = BIT(ADEN) | BIT(ADSC) | BIT(ADATE) | BIT(ADIF) | BIT(ADIE) | BIT(ADPS2) | BIT(ADPS1) | BIT(ADPS0);

	//***** Serial port configuration ************************

	UBRRL = RATE; // ustaw szybkosc
	UCSRB = BIT(RXCIE) | BIT(RXEN) | BIT(TXCIE) | BIT(TXEN);
	UCSRC = BIT(URSEL) | BIT(UCSZ1) | BIT(UCSZ0);

	//***** WDT *******************************

	WDTCR = BIT(WDE);
	WDTCR = BIT(WDE) | BIT(WDP2) | BIT(WDP1);
	WDR;

	//***** Interrupt init ********************************

	MCUCR = BIT(ISC11); // INT1 na zboczu
	TIMSK = BIT(OCIE2); // T2 CTC
	GIFR = BIT(INTF1);	// skasuj znacznik INT1

}

int main(void)
{
	ioSetup();
	//***** Inicjalizacja zmiennych *******************************

	vref = 509;				  // vref >= 480
	TOPPWM = 61 * vref / 100; // okres PWM Ua i Ug2

	ug1set = ug1zer = ug1ref = liczug1(240);
	; // -24.0V
	currentLampData.ug1def = 240;

	cwartmax = (ELAMP + FLAMP - 1);								 // wszystkie lampy
	cwart = &currentLampNum;											 // wskaz na Typ
	currentLampNum = eeprom_read_byte((const unsigned char *)&poptyp); // ustaw na ostatnio aktywny
	ADMUX = ADRIH;

	SEI; // wlacz przerwania

	//***** Inicjalizacja wyswietlacza LCD ************************

	delay(30); // 30ms
	cmd2lcd(0, 0x28);
	cmd2lcd(0, 0x06);
	cmd2lcd(0, 0x0c);
	cmd2lcd(0, 0x01);
	cmd2lcd(0, 0x40);

	SPKON;
	gotoxy( 0, 0 ); cstr2lcd( 0, (unsigned char*)"VTTester 1.16" );
	gotoxy( 0, 1 ); cstr2lcd( 0, (unsigned char*)"AVT 5229" );
	gotoxy( 0, 2 ); cstr2lcd( 0, (unsigned char*)"Adam Tomasz");
	gotoxy( 0, 3 ); cstr2lcd( 0, (unsigned char*)"Tatus Gumny");
	for( i = 0; i < 2; i++ ) { 
		WDR; 
		delay( 100 ); 
	};
	SPKOFF;
	gotoxy( 0, 0 ); cstr2lcd( 0, (unsigned char*)"             G     V" );
	gotoxy( 0, 1 ); cstr2lcd( 0, (unsigned char*)"H=    V A=    G2=   " );
	gotoxy( 0, 2 ); cstr2lcd( 0, (unsigned char*)"      mA            " );


	// 01234567890123456789
	// ====================
	// 01 ECC83_J29 G=24.0V
	// H=12.6V A=300 G2=300
	// T 2550mA 199.9 19.99
	// S=99.9 u=99.9 R=99.9
	// ====================

	cstr2rs("\r\nPress <ESC> to get LCD copy\r\nNr Type Uh[V] Ih[mA] -Ug[V] Ua[V] Ia[mA] Ug2[V] Ig2[mA] S[mA/V] R[k] K[V/V]");

	//***** Glowna petla programu *********************************

	while (1)
	{
		WDR;
		if (sync == 1)
		{
			sync = 0;
			//***** Wyswietlenie zawartosci bufora ************************

			gotoxy(0, 0); // Numer
			str2lcd(((adr == 0) && (start != (FUH + 2))), &buf[0]);

			gotoxy(3, 0); // name
			for (i = 0; i < 9; i++)
				char2lcd((((adr - 1) == i) || ((i == 7) && (start == (FUH + 2)) && ((buf[10] == '1') || (buf[10] == '2')))), buf[i + 3]);

			gotoxy(14, 0);
			char2lcd((adr == 10), '-');
			str2lcd((adr == 10), &buf[23]); // Ug1

			gotoxy(2, 1);
			str2lcd((adr == 11), &buf[13]); // Uh

			gotoxy(0, 2);
			errcode = ' ';
			if ((err & OVERIH) == OVERIH)
				errcode = 'H';
			if ((err & OVERIA) == OVERIA)
				errcode = 'A';
			if ((err & OVERIG) == OVERIG)
				errcode = 'G';
			if ((err & OVERTE) == OVERTE)
				errcode = 'T';
			char2lcd(0, errcode); // Err

			gotoxy(2, 2);
			str2lcd((adr == 12), &buf[18]); // Ih

			gotoxy(10, 1);
			str2lcd((adr == 13), &buf[28]); // Ua

			gotoxy(9, 2);
			str2lcd((adr == 14), &buf[32]); // Ia

			gotoxy(17, 1);
			str2lcd((adr == 15), &buf[38]); // Ug2

			gotoxy(15, 2);
			str2lcd((adr == 16), &buf[42]); // Ig2

			gotoxy(0, 3);
			if (currentLampNum > 1)
			{
				gotoxy(0, 3);
				cstr2lcd(0, (const unsigned char *)"S=");
				str2lcd((adr == 17), &buf[48]); // S

				gotoxy(6, 3);
				cstr2lcd(0, (const unsigned char *)" R=");
				str2lcd((adr == 18), &buf[53]); // R

				gotoxy(13, 3);
				if ((start <= (FUH + 2)) || (dusk0 == DMAX))
				{
					cstr2lcd(0, (const unsigned char *)" K=");
					str2lcd((adr == 19), &buf[58]); // K
				}
				else
				{
					cstr2lcd(0, (const unsigned char *)" T=");
					int2asc(start >> 2, ascii);

					if (ascii[2] != '0')
					{
						char2lcd(0, ascii[2]);
						char2lcd(0, ascii[1]);
					}
					else
					{
						char2lcd(0, ' ');
						if (ascii[1] != '0')
						{
							char2lcd(0, ascii[1]);
						}
						else
						{
							char2lcd(0, ' ');
						}
					}
					char2lcd(0, ascii[0]);
					char2lcd(0, 's');
				}
			}
			else
			{
				cstr2lcd(0, (const unsigned char *)"* OVERHEAT Warning *");
			}
		}
		GICR = BIT(INT1); // wlacz INT1
		if ((err != 0) && ((start == 0) || (start > (TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2))))
		{
			stop = 1;
			start = (TMAR + FUG2 + FUA + FUG + (BIP - 0) + FUH + 2);
		} // awaryjne wylaczenie

		//***** Wyswietlanie Numeru ***********************************
		if (adr == 0) // ustawianie numeru
		{
			int2asc(currentLampNum, ascii);
			buf[0] = ascii[1];
			buf[1] = ascii[0];
			//***** Pobieranie nowej Nazwy ************************************
			if (currentLampNum < FLAMP)
			{
				readPgmLampData(currentLampNum, &currentLampData);
				tuh = (currentLampData.name[8] - '0') * 240; // 240 = 1min
				for (i = 0; i < 9; i++)
					buf[i + 3] = (unsigned char)currentLampData.name[i];
			}
			else
			{
				eeprom_read_block(&lampeep[currentLampNum - FLAMP], &currentLampData, sizeof(katalog_t));
				tuh = (currentLampData.name[8] - 27) * 240; // 240 = 1min
				for (i = 0; i < 9; i++)
					buf[i + 3] = AZ[(unsigned char)currentLampData.name[i]];
			}
			nowa = 1; // zaktualizowano nazwe
		}

		//***** Edit Name ******************************************
		if ((adr > 0) && (adr < 10)) // edycja nazwy
		{
			//***** Displaying the edited Name *************************
			if (czytaj == 1)
				eeprom_read_block(&lampeep[currentLampNum - FLAMP].name, currentLampData.name, 7);
			for (i = 0; i < 9; i++)
				buf[3 + i] = AZ[(unsigned char)currentLampData.name[i]];
			czytaj = 0;
			if (dusk0 == DMAX)
				zapisz = 1;
			if ((nodus == DMIN) && (zapisz == 1))
			{
				eeprom_write_word((unsigned int *)&(lampeep[currentLampNum - FLAMP].name[adr - 1]), currentLampData.name[adr - 1]);
				zapisz = 0;
			}
		}
		else
			czytaj = 1;
		//***** Ustawianie Ug1 ****************************************

		ug1ref = liczug1(currentLampData.ug1def); // przelicz Ug1

		temp = mug1adc;
		temp *= 725;
		licz = 40960000;
		if (licz > temp)
		{
			licz -= temp;
		}
		else
			licz = 0;
		licz >>= 16; //   /= 65536;
		licz *= vref;
		licz /= 1000;
		ug1 = (unsigned int)licz;
		if (start == (FUH + 2))
			licz = ug1lcd;
		if ((adr == 0) || (adr == 10))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.ug1def; // 0..250..300
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 10) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum == 0) // SUPPLY
				{
					ug1set = ug1ref;
				}
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word((unsigned int *)&(lampeep[currentLampNum - FLAMP].ug1def), currentLampData.ug1def);
				}
			}
		}
		//***** Wyswietlanie Ug1 **************************************
		int2asc((unsigned int)licz, ascii);
		buf[23] = (ascii[2] != '0') ? ascii[2] : ' ';
		buf[24] = ascii[1];
		buf[25] = '.';
		buf[26] = ascii[0];
		//***** Ustawianie Uh *****************************************
		licz = muhadc;
		licz *= vref;
		licz >>= 14;   //  /= 16384;                      // 0..200
		temp = mihadc; // poprawka na spadek napiecia na boczniku
		temp *= vref;
		temp >>= 16; //    /= 65536;
		if (licz > temp)
		{
			licz -= temp;
		}
		else
			licz = 0;
		licz /= 10;
		uh = (unsigned int)licz;
		if (start == (FUH + 2))
			licz = uhlcd;
		if ((adr == 0) || (adr == 11))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.uhdef;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 11) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum == 0) // SUPPLY
				{
					uhset = currentLampData.uhdef;
					ihset = currentLampData.ihdef = 0;
				}
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word((unsigned int *)&(lampeep[currentLampNum - FLAMP].uhdef), currentLampData.uhdef);
				}
			}
		}
		//***** Wyswietlanie Uh ***************************************
		int2asc((unsigned int)licz, ascii);
		buf[13] = (ascii[2] != '0') ? ascii[2] : ' ';
		buf[14] = ascii[1];
		buf[15] = '.';
		buf[16] = ascii[0];
		//***** Ustawianie Ih *****************************************
		licz = mihadc;
		licz *= vref;
		licz >>= 15; //   /= 32768;                  // 0..250
		ih = (unsigned int)licz;
		if (start == (FUH + 2))
			licz = ihlcd;
		if ((adr == 0) || (adr == 12))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.ihdef;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 12) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum == 0) // SUPPLY
				{
					ihset = currentLampData.ihdef;
					uhset = currentLampData.uhdef = 0;
				}
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word((unsigned int *)&(lampeep[currentLampNum - FLAMP].ihdef), currentLampData.ihdef);
				}
			}
		}
		//***** Wyswietlanie Ih ***************************************
		int2asc((unsigned int)licz, ascii);
		if (ascii[2] != '0')
		{
			buf[18] = ascii[2];
			buf[19] = ascii[1];
			buf[20] = ascii[0];
		}
		else
		{
			buf[18] = ' ';
			if (ascii[1] != '0')
			{
				buf[19] = ascii[1];
				buf[20] = ascii[0];
			}
			else
			{
				buf[19] = ' ';
				buf[20] = (ascii[0] != '0') ? ascii[0] : ' ';
			}
		}
		buf[21] = '0';
		//***** Ustawianie Ua *****************************************
		licz = muaadc;
		licz *= vref;
		licz /= 107436;
		ua = (unsigned int)licz;
		if (start == (FUH + 2))
			licz = ualcd;
		if ((adr == 0) || (adr == 13))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.uadef;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 13) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum == 0) // SUPPLY
				{
					uaset = currentLampData.uadef;
				}
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word(&lampeep[currentLampNum - FLAMP].uadef, currentLampData.uadef);
				}
			}
		}
		//***** Wyswietlanie Ua ***************************************
		int2asc((unsigned int)licz, ascii);
		if (ascii[2] != '0')
		{
			buf[28] = ascii[2];
			buf[29] = ascii[1];
		}
		else
		{
			buf[28] = ' ';
			buf[29] = (ascii[1] != '0') ? ascii[1] : ' ';
		}
		buf[30] = ascii[0];
		//***** Ustawianie Ia ***************************************
		licz = miaadc;
		licz *= vref;
		licz >>= 14; //    /= 16384;
		temp = muaadc;
		temp *= vref;
		if (range == 0)
		{
			temp *= 10;
		}
		temp /= 4369064;
		if (licz > temp)
		{
			licz -= temp;
		}
		else
			licz = 0;
		ia = (unsigned int)licz;
		if (start == (FUH + 2))
			licz = ialcd;
		rangedef = 0;
		if ((adr == 0) || (adr == 14))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.iadef;
				rangedef = 1;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 14) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word(&lampeep[currentLampNum - FLAMP].iadef, currentLampData.iadef);
				}
			}
		}
		//***** Wyswietlanie Ia ***************************************
		int2asc((unsigned int)licz, ascii);
		if ((rangedef == 0) && (((start != (FUH + 2)) && (range == 0)) || ((start == (FUH + 2)) && (rangelcd == 0))))
		{
			buf[32] = (ascii[3] != '0') ? ascii[3] : ' ';
			buf[33] = ascii[2];
			buf[34] = '.';
			buf[35] = ascii[1];
			buf[36] = ascii[0];
		}
		else
		{
			if (ascii[3] != '0')
			{
				buf[32] = ascii[3];
				buf[33] = ascii[2];
			}
			else
			{
				buf[32] = ' ';
				buf[33] = (ascii[2] != '0') ? ascii[2] : ' ';
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
		if (start == (FUH + 2))
			licz = ug2lcd;
		if ((adr == 0) || (adr == 15))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.ug2def;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 15) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum == 0) // SUPPLY
				{
					ug2set = currentLampData.ug2def;
				}
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word(&lampeep[currentLampNum - FLAMP].ug2def, currentLampData.ug2def);
				}
			}
		}
		//***** Wyswietlanie Ug2 **************************************
		int2asc((unsigned int)licz, ascii);
		if (ascii[2] != '0')
		{
			buf[38] = ascii[2];
			buf[39] = ascii[1];
		}
		else
		{
			buf[38] = ' ';
			buf[39] = (ascii[1] != '0') ? ascii[1] : ' ';
		}
		buf[40] = ascii[0];
		//***** Ustawianie Ig2 **************************************
		licz = mig2adc;
		licz *= vref;
		licz >>= 13; //  /8192(40mA)  /16384(20mA);
		temp = mug2adc;
		temp *= vref;
		temp *= 10;
		temp /= 4369064;
		if (licz > temp)
		{
			licz -= temp;
		}
		else
			licz = 0;
		ig2 = (unsigned int)licz;
		if (start == (FUH + 2))
			licz = ig2lcd;
		if ((adr == 0) || (adr == 16))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.ig2def;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 16) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word(&lampeep[currentLampNum - FLAMP].ig2def, currentLampData.ig2def);
				}
			}
		}
		//***** Wyswietlanie Ig2 **************************************
		int2asc((unsigned int)licz, ascii);
		buf[42] = (ascii[3] != '0') ? ascii[3] : ' ';
		buf[43] = ascii[2];
		buf[44] = '.';
		buf[45] = ascii[1];
		buf[46] = ascii[0];
		//***** Ustawianie S ****************************************
		licz = s;
		if (start == (FUH + 2))
			licz = slcd;
		if ((adr == 0) || (adr == 17))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.sdef;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 17) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word(&lampeep[currentLampNum - FLAMP].sdef, currentLampData.sdef);
				}
			}
		}
		//***** Wyswietlanie S ****************************************
		int2asc(licz, ascii);
		buf[48] = (ascii[2] != '0') ? ascii[2] : ' ';
		buf[49] = ascii[1];
		buf[50] = '.';
		buf[51] = ascii[0];
		//***** Ustawianie R ****************************************
		licz = r;
		if (start == (FUH + 2))
			licz = rlcd;
		if ((adr == 0) || (adr == 18))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.rdef;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 18) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word(&(lampeep[currentLampNum - FLAMP].rdef), (unsigned int)(currentLampData.rdef));
				}
			}
		}
		//***** Wyswietlanie R ****************************************
		int2asc(licz, ascii);
		buf[53] = (ascii[2] != '0') ? ascii[2] : ' ';
		buf[54] = ascii[1];
		buf[55] = '.';
		buf[56] = ascii[0];
		//***** Ustawianie K ****************************************
		licz = k;
		if (start == (FUH + 2))
			licz = klcd;
		if ((adr == 0) || (adr == 19))
		{
			if (dusk0 == DMAX)
			{
				licz = currentLampData.kdef;
				zapisz = 1;
			}
			if ((nodus == DMIN) && (adr == 19) && (zapisz == 1))
			{
				zapisz = 0;
				if (currentLampNum >= FLAMP) // ELAMP
				{
					eeprom_write_word(&lampeep[currentLampNum - FLAMP].kdef, currentLampData.kdef);
				}
			}
		}
		//***** Wyswietlanie K ****************************************
		int2asc(licz, ascii);
		buf[58] = (ascii[2] != '0') ? ascii[2] : ' ';
		buf[59] = ascii[1];
		buf[60] = '.';
		buf[61] = ascii[0];
		//***** Wyslanie pomiarow do PC *******************************
		if (txen)
		{
			eeprom_write_word(&poptyp, currentLampNum);
			cstr2rs("\r\n");
			for (i = 0; i < 62; i++)
			{
				if (buf[i] != '\0')
					char2rs(buf[i]);
				else
					cstr2rs("  ");
			}
			txen = 0;
		}
	}
}
