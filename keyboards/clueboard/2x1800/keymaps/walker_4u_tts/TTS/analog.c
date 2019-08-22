#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include "progmem.h"
#include "pincontrol.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

/* Delay for the given number of microseconds.  Assumes a 8 or 16 MHz clock. */
void delayMicroseconds(unsigned int us)
{
        // calling avrlib's delay_us() function with low values (e.g. 1 or
        // 2 microseconds) gives delays longer than desired.
        //delay_us(us);
#if F_CPU >= 20000000L
        // for the 20 MHz clock on rare Arduino boards

        // for a one-microsecond delay, simply wait 2 cycle and return. The overhead
        // of the function call yields a delay of exactly a one microsecond.
        __asm__ __volatile__ (
                "nop" "\n\t"
                "nop"); //just waiting 2 cycle
        if (--us == 0)
                return;

        // the following loop takes a 1/5 of a microsecond (4 cycles)
        // per iteration, so execute it five times for each microsecond of
        // delay requested.
        us = (us<<2) + us; // x5 us

        // account for the time taken in the preceeding commands.
        us -= 2;

#elif F_CPU >= 16000000L
        // for the 16 MHz clock on most Arduino boards

        // for a one-microsecond delay, simply return.  the overhead
        // of the function call yields a delay of approximately 1 1/8 us.
        if (--us == 0)
                return;

        // the following loop takes a quarter of a microsecond (4 cycles)
        // per iteration, so execute it four times for each microsecond of
        // delay requested.
        us <<= 2;

        // account for the time taken in the preceeding commands.
        us -= 2;
#else
        // for the 8 MHz internal clock on the ATmega168

        // for a one- or two-microsecond delay, simply return.  the overhead of
        // the function calls takes more than two microseconds.  can't just
        // subtract two, since us is unsigned; we'd overflow.
        if (--us == 0)
                return;
        if (--us == 0)
                return;

        // the following loop takes half of a microsecond (4 cycles)
        // per iteration, so execute it twice for each microsecond of
        // delay requested.
        us <<= 1;

        // partially compensate for the time taken by the preceeding commands.
        // we can't subtract any more than this or we'd overflow w/ small delays.
        us--;
#endif
        // busy wait
        __asm__ __volatile__ (
                "1: sbiw %0,1" "\n\t" // 2 cycles
                "brne 1b" : "=w" (us) : "0" (us) // 2 cycles
        );
}

void analogWriteMike(uint8_t pin, int val)
{
	// We need to make sure the PWM output is enabled for those pins
	// that support it, as we turn it off when digitally reading or
	// writing with them.  Also, make sure the pin is in output mode
	// for consistenty with Wiring, which doesn't require a pinMode
	// call for the analog output pins.
	pinMode(pin, PinDirectionOutput);
	if (val == 0)
	{
		digitalWrite(pin, PinLevelLow);
	}
	else if (val == 255)
	{
		digitalWrite(pin, PinLevelHigh);
	}
	else
	{
		switch(pin)
		{
			case C6:
				sbi(TCCR3A, COM3A1);
				OCR3A = val;
				break;
			case C5:
				sbi(TCCR3A, COM3B1);
				OCR3B = val;
				break;

			case C4:
				sbi(TCCR3A, COM3C1);
				OCR3C = val;
				break;

			case B5:
				sbi(TCCR1A, COM1A1);
				OCR1A = val;
				break;
			case B6:
				sbi(TCCR1A, COM1B1);
				OCR1B = val;
				break;
			case B7:
				sbi(TCCR1A, COM1C1);
				OCR1C = val;
				break;

			default:
				break;
		}
	}
}

