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
#include "i2c.h"

#include "matrix.h"

#ifndef DEBOUNCE
#   define DEBOUNCE	5
#endif

static uint8_t debouncing = DEBOUNCE;

static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];

void matrix_set_row_status(uint8_t row);
uint8_t bit_reverse(uint8_t x);

#define MCP23018_TWI_ADDRESS 0b0100000
#define TW_READ		1
#define TW_WRITE	0
#define TWI_ADDR_WRITE ( (MCP23018_TWI_ADDRESS<<1) | TW_WRITE )
#define TWI_ADDR_READ  ( (MCP23018_TWI_ADDRESS<<1) | TW_READ  )

#define MCP_ROWS_START	8

/*
uint8_t mcp23018_init(void);

uint8_t mcp23018_init(void) {
	uint8_t ret;
	uint8_t data[3];
	// set pin direction
	// - unused  : input  : 1
	// - input   : input  : 1
	// - driving : output : 0
	data[0] = IODIRA;
	data[1] = 0b00000000;  // IODIRA
	data[2] = (0b11111111);  // IODIRB

	ret = i2cMasterSendNI(TWI_ADDR_WRITE, 3, (u08 *)data);
	if (ret) goto out;  // make sure we got an ACK
	// set pull-up
	// - unused  : on  : 1
	// - input   : on  : 1
	// - driving : off : 0
	data[0] = GPPUA;
	data[1] = 0b00000000;  // IODIRA
	data[2] = (0b11111111);  // IODIRB

	ret = i2cMasterSendNI(TWI_ADDR_WRITE, 3, (u08 *)data);
	if (ret) goto out;  // make sure we got an ACK

	// set logical value (doesn't matter on inputs)
	// - unused  : hi-Z : 1
	// - input   : hi-Z : 1
	// - driving : hi-Z : 1
	data[0] = OLATA;
	data[1] = 0b11111111;  // IODIRA
	data[2] = (0b11111111);  // IODIRB

	ret = i2cMasterSendNI(TWI_ADDR_WRITE, 3, (u08 *)data);

out:
	return ret;
}
*/

void matrix_init(void) {
    // all outputs for rows high
    DDRB = 0xFF;
    PORTB = 0xFF;

    // all inputs for columns
    DDRA = 0x00;
    DDRC &= ~(0x111111<<2);
    DDRD &= ~(1<<PIND7);

    // all columns are pulled-up
    PORTA = 0xFF;
    PORTC |= (0b111111<<2);
    PORTD |= (1<<PIND7);

    // initialize matrix state: all keys off
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        matrix[row] = 0x00;
        matrix_debouncing[row] = 0x00;
    }

    // Initialize the other half of the keyboard 
    matrix_init_quantum();
}

uint8_t matrix_scan(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        matrix_set_row_status(row);
        _delay_us(5);

        matrix_row_t cols = (
            // cols 0..7, PORTA 0 -> 7
            (~PINA) & 0xFF
        ) | (
            // cols 8..13, PORTC 7 -> 0
            bit_reverse((~PINC) & 0xFF) << 8
        ) | (
            // col 14, PORTD 7
            ((~PIND) & (1 << PIND7)) << 7
        );

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

    matrix_scan_quantum();

    return 1;
}

// declarations
void matrix_set_row_status(uint8_t row) {
    DDRB = (1 << row);
    PORTB = ~(1 << row);

    //Set the row on the slave half
      
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
