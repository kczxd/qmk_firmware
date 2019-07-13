#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hal.h"
#include "timer.h"
#include "wait.h"
#include "print.h"
#include "matrix.h"
#include "action.h"
#include "mikebench.h"

/*
 * Matt3o's WhiteFox
 * Column pins are input with internal pull-down. Row pins are output and strobe with high.
 * Key is high or 1 when it turns on.
 *
 *     col: { PTD0, PTD1, PTD4, PTD5, PTD6, PTD7, PTC1, PTC2 }
 *     row: { PTB2, PTB3, PTB18, PTB19, PTC0, PTC8, PTC9, PTC10, PTC11 }
 */
/* matrix state(1:on, 0:off) */
static matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_debouncing[MATRIX_ROWS];
static bool debouncing = false;
static uint16_t debouncing_time = 0;

#define SOLENOID_DEFAULT_DWELL 12 
#define SOLENOID_MAX_DWELL 100
#define SOLENOID_MIN_DWELL 4
#define SOLENOID_MAX 4
#define SOLENOID_COUNT 1

static bool solenoid_enabled[SOLENOID_MAX] = {false, false, false};
static bool solenoid_is_on[SOLENOID_MAX] = {false, false, false};
static bool solenoid_buzz[SOLENOID_MAX] = {false, false, false}; 
static bool solenoid_buzzing[SOLENOID_MAX] = {false, false, false};
volatile static uint32_t solenoid_start_time[SOLENOID_MAX] = {0, 0, 0};
static uint8_t solenoid_dwell[SOLENOID_MAX] = {SOLENOID_DEFAULT_DWELL, SOLENOID_DEFAULT_DWELL, SOLENOID_DEFAULT_DWELL};
static uint8_t solenoid_index = 0;

void solenoid_buzz_on(void) {
  for(int i=0;i<SOLENOID_COUNT;i++)
    solenoid_buzz[i] = true;
}

void solenoid_buzz_off(void) {
  for(int i=0;i<SOLENOID_COUNT;i++)
    solenoid_buzz[i] = false;
}

void solenoid_dwell_minus(void) {
  for(int i=0;i<SOLENOID_COUNT;i++)
    if (solenoid_dwell[i] > 0) solenoid_dwell[i]--;
}

void solenoid_dwell_plus(void) {
  for(int i=0;i<SOLENOID_COUNT;i++)
    if (solenoid_dwell[i] < SOLENOID_MAX_DWELL) solenoid_dwell[i]++;
}

void solenoid_toggle(void) {
  for(int i=0;i<SOLENOID_COUNT;i++)
    solenoid_enabled[i] = !solenoid_enabled[i];
}

void solenoid_off(int i) {
  if (!solenoid_enabled[i]) return;
  switch (i) {
    case 0: palClearPad(GPIOB, 0); break;
    case 1: palClearPad(GPIOB, 1); break;
    case 2: palClearPad(GPIOD, 2); break;
    case 3: palClearPad(GPIOD, 3); break;
  }
}
 
void solenoid_on(int i) {
  if (!solenoid_enabled[i]) return;
  switch (i) {
    case 0: palSetPad(GPIOB, 0); break;
    case 1: palSetPad(GPIOB, 1); break;
    case 2: palSetPad(GPIOD, 2); break;
    case 3: palSetPad(GPIOD, 3); break;
  }
}

void solenoid_stop(int i) {
  solenoid_off(i);
  solenoid_is_on[i] = false;
  solenoid_buzzing[i] = false;
}

void solenoid_start(int i) {
  solenoid_is_on[i] = true;
  solenoid_buzzing[i] = true;
  solenoid_start_time[i] = timer_read32();
  solenoid_on(i);
}

void solenoid_fire(void) {
  if (!solenoid_enabled[solenoid_index]) return;
  if (!solenoid_buzz[solenoid_index] && solenoid_is_on[solenoid_index]) return;
  if (solenoid_buzz[solenoid_index] && solenoid_buzzing[solenoid_index]) return;
  solenoid_start(solenoid_index);
  solenoid_index = ((solenoid_index + 1) % SOLENOID_COUNT);
}

void solenoid_check(void) {
  uint16_t elapsed = 0;

  for (int i=0; i<SOLENOID_COUNT; i++) {
    if (!solenoid_is_on[i]) continue;

    elapsed = timer_elapsed32(solenoid_start_time[i]);

    //Check if it's time to finish this solenoid click cycle 
    if (elapsed > solenoid_dwell[i]) {
      solenoid_stop(i);
      return;
    }

    //Check whether to buzz the solenoid on and off
    if (solenoid_buzz[i]) {
      if (elapsed / SOLENOID_MIN_DWELL % 2 == 0){
        if (!solenoid_buzzing[i]) {
          solenoid_buzzing[i] = true;
          solenoid_on(i); 
        }
      }
      else {
        if (solenoid_buzzing[i]) {
          solenoid_buzzing[i] = false;
          solenoid_off(i);
        }
      }
    }
  }
}

void solenoid_init(void)
{
    palSetPadMode(GPIOB, 0,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 1,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOD, 2,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOD, 3,  PAL_MODE_OUTPUT_PUSHPULL);

    timer_init();
}

void matrix_init(void)
{
//debug_matrix = true;
    /* Column(sense) */
    palSetPadMode(GPIOD, 0,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOD, 1,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOD, 4,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOD, 5,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOD, 6,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOD, 7,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOC, 1,  PAL_MODE_INPUT_PULLDOWN);
    palSetPadMode(GPIOC, 2,  PAL_MODE_INPUT_PULLDOWN);

    /* Row(strobe) */
    palSetPadMode(GPIOB, 2,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 3,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 18, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOB, 19, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOC, 0,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOC, 8,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOC, 9,  PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOC, 10, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOC, 11, PAL_MODE_OUTPUT_PUSHPULL);

    memset(matrix, 0, MATRIX_ROWS * sizeof(matrix_row_t));
    memset(matrix_debouncing, 0, MATRIX_ROWS * sizeof(matrix_row_t));

    solenoid_init();

    matrix_init_quantum();
}

uint8_t matrix_scan(void)
{
    for (int row = 0; row < MATRIX_ROWS; row++) {
        matrix_row_t data = 0;

        // strobe row
        switch (row) {
            case 0: palSetPad(GPIOB, 2);    break;
            case 1: palSetPad(GPIOB, 3);    break;
            case 2: palSetPad(GPIOB, 18);   break;
            case 3: palSetPad(GPIOB, 19);   break;
            case 4: palSetPad(GPIOC, 0);    break;
            case 5: palSetPad(GPIOC, 8);    break;
            case 6: palSetPad(GPIOC, 9);    break;
            case 7: palSetPad(GPIOC, 10);   break;
            case 8: palSetPad(GPIOC, 11);   break;
        }

        wait_us(20); // need wait to settle pin state

        // read col data: { PTD0, PTD1, PTD4, PTD5, PTD6, PTD7, PTC1, PTC2 }
        data = ((palReadPort(GPIOC) & 0x06UL) << 5) |
               ((palReadPort(GPIOD) & 0xF0UL) >> 2) |
                (palReadPort(GPIOD) & 0x03UL);

        // un-strobe row
        switch (row) {
            case 0: palClearPad(GPIOB, 2);  break;
            case 1: palClearPad(GPIOB, 3);  break;
            case 2: palClearPad(GPIOB, 18); break;
            case 3: palClearPad(GPIOB, 19); break;
            case 4: palClearPad(GPIOC, 0);  break;
            case 5: palClearPad(GPIOC, 8);  break;
            case 6: palClearPad(GPIOC, 9);  break;
            case 7: palClearPad(GPIOC, 10); break;
            case 8: palClearPad(GPIOC, 11); break;
        }

        if (matrix_debouncing[row] != data) {
            matrix_debouncing[row] = data;
            debouncing = true;
            debouncing_time = timer_read();
        }
    }

    if (debouncing && timer_elapsed(debouncing_time) > DEBOUNCE) {
        for (int row = 0; row < MATRIX_ROWS; row++) {
            matrix[row] = matrix_debouncing[row];
        }
        debouncing = false;
    }
    matrix_scan_quantum();
    solenoid_check();
    return 1;
}

bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & (1<<col));
}

matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

void matrix_print(void)
{
    xprintf("\nr/c 01234567\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        xprintf("%X0: ", row);
        matrix_row_t data = matrix_get_row(row);
        for (int col = 0; col < MATRIX_COLS; col++) {
            if (data & (1<<col))
                xprintf("1");
            else
                xprintf("0");
        }
        xprintf("\n");
    }
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
  if (record->event.pressed) {
    solenoid_fire();
  }

  return process_record_user(keycode, record);
}

