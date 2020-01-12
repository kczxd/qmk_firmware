/*
Copyright 2017 Luiz Ribeiro <luizribeiro@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <util/delay.h>
#include <i2c_master.h>
#include "matrix.h"
#include "gsx980.h"
#include "pincontrol.h"

#include <string.h>
#include <stdio.h>

#ifndef DEBOUNCE
#define DEBOUNCE    10
#endif

#if defined(DEBOUNCE)
static uint8_t debouncing = DEBOUNCE;
#endif

extern uint8_t led0, led1, led2;

static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

void matrix_set_row_status(uint8_t row);
uint8_t bit_reverse(uint8_t x);

static void set_led_val(uint8_t val) {
    led0 = val & 0x1;
    led1 = (val & 0x2) >> 1;
    led2 = (val & 0x4) >> 2;
}

static uint16_t globalval = 0;
static void inc_led_val(void) {
    globalval++;
    set_led_val(globalval / 8192);
}

void matrix_init(void) {
    // Init indicator LEDs
    indicator_init();

    // Set rows as output starting high
    DDRB |= 0x7F;
    PORTB |= 0x7F;

    // Set columns as inputs with pull-up enabled
    DDRA &= 0x00;
    PORTA |= 0xFF;
    DDRC &= 0x3;
    PORTC |= 0xFC;
    DDRD &= 0x7F;
    PORTD |= 0x80;

    // initialize matrix state: all keys off
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        matrix[row] = 0x00;
        matrix_debouncing[row] = 0x00;
    }

    // Initialize i2c communication
    i2c_init();

    matrix_init_quantum();
    (void)inc_led_val;
    (void)set_led_val;
}

uint8_t matrix_scan(void) {
    matrix_row_t col[3] = {0, 0, 0};
    matrix_row_t cols = 0;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        cols = 0;
	//Select the row to scan
        matrix_set_row_status(row);

        _delay_us(5);

	//Set the local row
        col[0] = ((~(PINA | 0x00)) & 0xFF);
        col[1] = ((~( bit_reverse(PINC | 0x3) )) & 0x3F);
        col[2] = ((~( bit_reverse(PIND | 0x7F) )) & 0x1);

        cols = col[0] | col[1] << 8 | col[2] << 14;

#if defined(DEBOUNCE)
        if (matrix_debouncing[row] != cols) {
            matrix_debouncing[row] = cols;
            debouncing = DEBOUNCE;
        }
    }

    if (debouncing) {
        if (--debouncing) {
            _delay_ms(1);
        } else {
            for (uint8_t i = 0; i < MATRIX_ROWS; i++) {
                matrix[i] = matrix_debouncing[i];
            }
        }
    }
#else
        matrix[row] = cols;
    }
#endif

    matrix_scan_quantum();

    return 1;
}

void matrix_set_row_status(uint8_t row) {
    //Set the local row on port B
    DDRB = (1 << row);
    PORTB = ~(1 << row);
}

uint8_t bit_reverse(uint8_t x) {
    x = ((x >> 1) & 0x55) | ((x << 1) & 0xaa);
    x = ((x >> 2) & 0x33) | ((x << 2) & 0xcc);
    x = ((x >> 4) & 0x0f) | ((x << 4) & 0xf0);
    return x;
}

inline matrix_row_t matrix_get_row(uint8_t row) {
    return matrix[row];
}

void matrix_print(void) {
}
