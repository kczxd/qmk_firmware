/* Copyright 2015-2017 Jack Humbert
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "contra.h"
#include "action_layer.h"
#include <avr/io.h>
#include <timer.h>
#include "pincontrol.h"
#include "serial_wrapper.h"
//#include "dectalk_songs.h"

extern keymap_config_t keymap_config;

#define SOLENOID_DEFAULT_DWELL 12 
#define SOLENOID_MAX_DWELL 100
#define SOLENOID_MIN_DWELL 4
#define SOLENOID_PIN F7

#define randadd 53
#define randmul 181
#define randmod 167

bool solenoid_enabled = false;
bool solenoid_on = false;
bool solenoid_buzz = false;
bool solenoid_buzzing = false;
uint16_t solenoid_start = 0;
uint8_t solenoid_dwell = SOLENOID_DEFAULT_DWELL;

static uint16_t random_value = 157;

uint8_t myrandom(uint8_t howbig) {
  uint8_t clockbyte=0;
  clockbyte = TCNT1 % 256;
  uint8_t rval;

  random_value = ((random_value + randadd) * randmul) % randmod;
  rval = (random_value ^ clockbyte) % howbig;

  return rval;
}

const char ** get_random_song(void) {
  return (const char **)NULL;
}

/*
const char ** get_random_song(void) {
  uint8_t local_random;
  const char ** result = NULL;

  local_random = myrandom(SONG_COUNT);

  switch(local_random) {
    case 0:
      result = song0;
    break;
    case 1:
      result = song1;
    break;
    case 2:
      result = song2;
    break;
    case 3:
      result = song3;
    break;
    case 4:
      result = song4;
    break;
    case 5:
      result = song5;
    break;
    case 6:
      result = song6;
    break;
    case 7:
      result = song7;
    break;
    case 8:
      result = song8;
    break;
    case 9:
      result = song9;
    break;
    case 10:
      result = song10;
    break;
    case 11:
      result = song11;
    break;
    case 12:
      result = song12;
    break;
    case 13:
      result = song13;
    break;
    case 14:
      result = song14;
    break;
    case 15:
      result = song15;
    break;
    case 16:
      result = song16;
    break;
    case 17:
      result = song17;
    break;
  }

  return result;
}
*/

void sing_random_song(void) {
  uint8_t i = 0;

  Serial1_println("STesting 123!\n");
  return;

  const char ** random_song = get_random_song();
  const char * random_line = random_song[0];
 
  while (random_line[0] != '\0') {
    Serial1_println((char *)random_line);
    i++;
    random_line = random_song[i];
  }
}

void solenoid_buzz_on(void) {
  solenoid_buzz = true;
}

void solenoid_buzz_off(void) {
  solenoid_buzz = false;
}

void solenoid_dwell_minus(void) {
  if (solenoid_dwell > 0) solenoid_dwell--;
}

void solenoid_dwell_plus(void) {
  if (solenoid_dwell < SOLENOID_MAX_DWELL) solenoid_dwell++;
}

void solenoid_toggle(void) {
  solenoid_enabled = !solenoid_enabled;
} 

void solenoid_stop(void) {
  digitalWrite(SOLENOID_PIN, PinLevelLow);
  solenoid_on = false;
  solenoid_buzzing = false;
}

void solenoid_fire(void) {
  if (!solenoid_enabled) return;

  if (!solenoid_buzz && solenoid_on) return;
  if (solenoid_buzz && solenoid_buzzing) return;

  solenoid_on = true;
  solenoid_buzzing = true;
  solenoid_start = timer_read(); 
  digitalWrite(SOLENOID_PIN, PinLevelHigh);
}

void solenoid_check(void) {
  uint16_t elapsed = 0;

  if (!solenoid_on) return;

  elapsed = timer_elapsed(solenoid_start);

  //Check if it's time to finish this solenoid click cycle 
  if (elapsed > solenoid_dwell) {
    solenoid_stop();
    return;
  }

  //Check whether to buzz the solenoid on and off
  if (solenoid_buzz) {
    if (elapsed / SOLENOID_MIN_DWELL % 2 == 0){
      if (!solenoid_buzzing) {
        solenoid_buzzing = true;
        digitalWrite(SOLENOID_PIN, PinLevelHigh);
      }
    }
    else {
      if (solenoid_buzzing) {
        solenoid_buzzing = false;
        digitalWrite(SOLENOID_PIN, PinLevelLow);
      }
    }
  }
  
}

void solenoid_setup(void) {
  pinMode(SOLENOID_PIN, PinDirectionOutput);
}

void matrix_init_user(void) {
  solenoid_setup();
  Serial1_begin(9600);
  Serial1_println("V18");
}

void matrix_scan_user(void) {
  solenoid_check();
}

enum planck_layers {
  _QWERTY,
  _LOWER,
  _RAISE,
  _ADJUST
};

enum planck_keycodes {
  QWERTY = SAFE_RANGE,
  LOWER,
  RAISE,
  BACKLIT,
  SOLENOID_TOG,
  SOLENOID_DWELL_MINUS,
  SOLENOID_DWELL_PLUS,
  SOLENOID_BUZZ_ON,
  SOLENOID_BUZZ_OFF,
  SING_RANDOM_SONG,
  EXT_PLV
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[_QWERTY] = {
  {KC_ESC,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC},
  {KC_TAB,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT},
  {KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_ENT },
  {KC_LCTL, KC_LGUI, KC_LALT, KC_DEL, LOWER, KC_SPC,  KC_SPC,  RAISE,   KC_LEFT, KC_RGHT, KC_DOWN, KC_UP}
},

[_LOWER] = {
  {KC_TILD, KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC, KC_CIRC, KC_AMPR,    KC_ASTR,    KC_LPRN, KC_RPRN, KC_UNDERSCORE},
  {KC_CAPSLOCK,  KC_F1,   KC_F2,   KC_F3,   KC_INSERT,   KC_HOME,   KC_PGUP,   KC_LEFT_CURLY_BRACE, KC_RIGHT_CURLY_BRACE, KC_PLUS, _______, KC_PIPE},
  {_______, KC_F4,   KC_F5,   KC_F6,   KC_DEL,  KC_END,  KC_PGDOWN, KC_SCOLON, KC_LT, KC_GT, KC_DOUBLE_QUOTE, KC_PSCREEN},
  {_______, KC_F7,   KC_F8,   KC_F9, _______, _______, _______, _______, KC_HOME, KC_END, KC_PGDOWN, KC_PGUP}
},

[_RAISE] = {
  {KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINUS},
  {KC_SCROLLLOCK,  KC_F1,   KC_F2,   KC_F3,   KC_INS,  KC_HOME, KC_PGUP, KC_LBRACKET, KC_RBRACKET,  KC_EQUAL, _______, KC_BSLS},
  {_______, KC_F4,   KC_F5,   KC_F6,   KC_DEL,  KC_END,  KC_PGDOWN, KC_COLON, KC_NUBS, _______, KC_QUOTE, KC_PAUSE},
  {_______, KC_F7,   KC_F8,   KC_F9, _______, _______, _______, _______, _______, KC_F10, KC_F11, KC_F12}
},

[_ADJUST] = {
  {SING_RANDOM_SONG, RESET,   DEBUG,    RGB_TOG, RGB_MOD, RGB_HUI, RGB_HUD, RGB_SAI, RGB_SAD, RGB_VAI, RGB_VAD, KC_DEL },
  {_______, _______, MU_MOD,  AU_ON,   AU_OFF,  AG_NORM, AG_SWAP, QWERTY,  _______, _______,  _______,  _______},
  {_______, MUV_DE,  MUV_IN,  MU_ON,   MU_OFF,  MI_ON,   MI_OFF,  TERM_ON, TERM_OFF, RGB_VAD, RGB_VAI, SOLENOID_TOG},
  {_______, _______, _______, _______, _______, _______, _______, _______, SOLENOID_BUZZ_OFF, SOLENOID_BUZZ_ON, SOLENOID_DWELL_MINUS, SOLENOID_DWELL_PLUS}
}


};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  
  if (record->event.pressed) {
    solenoid_fire(); 
  }

  switch (keycode) {
    case SING_RANDOM_SONG:
      if (record->event.pressed) {
        sing_random_song();
      }
      break;
    case LOWER:
      if (record->event.pressed) {
        layer_on(_LOWER);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      } else {
        layer_off(_LOWER);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      }
      return false;
      break;
    case RAISE:
      if (record->event.pressed) {
        layer_on(_RAISE);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      } else {
        layer_off(_RAISE);
        update_tri_layer(_LOWER, _RAISE, _ADJUST);
      }
      return false;
      break;
    case BACKLIT:
      if (record->event.pressed) {
        register_code(KC_RSFT);
        #ifdef BACKLIGHT_ENABLE
          backlight_step();
        #endif
        PORTE &= ~(1<<6);
      } else {
        unregister_code(KC_RSFT);
        PORTE |= (1<<6);
      }
      return false;
      break;
    case SOLENOID_TOG:
      if (record->event.pressed) {
        solenoid_toggle();
      }
      break;
    case SOLENOID_DWELL_MINUS:
      if (record->event.pressed) {
        solenoid_dwell_minus();
      }
      break;
    case SOLENOID_DWELL_PLUS:
      if (record->event.pressed) {
        solenoid_dwell_plus();
      }
      break;
    case SOLENOID_BUZZ_ON:
      if (record->event.pressed) {
        solenoid_buzz_on();
      }
      break;
    case SOLENOID_BUZZ_OFF:
      if (record->event.pressed) {
        solenoid_buzz_off();
      }
      break;
  }
  return true;
}
