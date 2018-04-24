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
#include <string.h>
#include <stdio.h>

extern keymap_config_t keymap_config;

#define SOLENOID_DEFAULT_DWELL 12 
#define SOLENOID_MAX_DWELL 100
#define SOLENOID_MIN_DWELL 4
#define SOLENOID_PIN F7

#define randadd 53
#define randmul 181
#define randmod 167

#define SONG_COUNT 6
void song0(void);
void song1(void);
void song2(void);
void song3(void);
void song4(void);
void song5(void);

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

#define SPEECH_QUEUE_SIZE 16
#define MAX_SPEECH_LINE 256

int speech_read = 0;
int speech_write = 0;
int speech_count = 0;

void speech_read_inc(void) {
  speech_read = (speech_read < SPEECH_QUEUE_SIZE - 1 ? speech_read + 1 : 0); 
}

void speech_write_inc(void) {
  speech_write = (speech_write < SPEECH_QUEUE_SIZE - 1 ? speech_write + 1 : 0); 
}

void speech_read_dec(void) {
  speech_read = (speech_read > 0 ? speech_read - 1 : SPEECH_QUEUE_SIZE - 1); 
}

void speech_write_dec(void) {
  speech_write = (speech_write > 0 ? speech_write - 1 : SPEECH_QUEUE_SIZE - 1); 
}

//char speech_queue[SPEECH_QUEUE_SIZE][MAX_SPEECH_LINE];
void * speech_queue[SPEECH_QUEUE_SIZE];

void queue(char * line) {
  //strcpy(speech_queue[speech_write],line);
  speech_queue[speech_write] = (void *)line;
  speech_write_inc();
  speech_count++;  
} 

void say(const char *);

void service_queue(void) {
  char * local_line;
  if(speech_count > 0) {
    local_line = (char *)speech_queue[speech_read]; 
    say(local_line);
    speech_read_inc();
    speech_count--;
  }
}

void sing_random_song(void) {
  uint8_t local_random;
  local_random = myrandom(SONG_COUNT);
  
  switch(local_random) {
    case 0:
      song0();
    break;
    case 1:
      song1();
    break;
    case 2:
      song2();
    break;
    case 3:
      song3();
    break;
    case 4:
      song4();
    break;
    case 5:
      song5();
    break;
/*
    case 6:
      song6();
    break;
    case 7:
      song7();
    break;
    case 8:
      song8();
    break;
    case 9:
      song9();
    break;
    case 10:
      song10();
    break;
    case 11:
      song11();
    break;
    case 12:
      song12();
    break;
    case 13:
      song13();
    break;
    case 14:
      song14();
    break;
    case 15:
      song15();
    break;
    case 16:
      song16();
    break;
    case 17:
      song17();
    break;
*/
  }

}

//Just write a string without any newline or anything
void serial_write_string(char * buff) {
  int i;
  for (i=0; i<strlen(buff) && i < MAX_SPEECH_LINE; i++) {
    Serial1_write(buff[i]);
  }
}

void flush_all(void) {
  Serial1_flush();
  while (Serial1_read() != -1); 
} 

void waitfor(char token) {
  char localtoken;
  do { 
    localtoken = Serial1_read();
  } while (localtoken != token);
}

void say(const char * line) {
  char buff[MAX_SPEECH_LINE];
  flush_all();
  serial_write_string("\n");
  waitfor(':');
  sprintf(buff, "S%s[:n0]\n", line);
  serial_write_string(buff);
  waitfor(':');
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
  serial_write_string("\n");
  waitfor(':');

  flush_all();
  serial_write_string("V18\n");
  serial_write_string("P0\n");

  queue("[:rate 200][:n0][:dv ap 90 pr 0] All your base are belong to us.");
}

void matrix_scan_user(void) {
  solenoid_check();

  service_queue(); 
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

//Spooky Scary Skeketons:
void song0(void) { 
  queue("[spuh<300,19>kiy<300,19>skeh<300,18>riy<300,18>skeh<300,11>lleh<175,14>tih<200,11>ns]"); 
  queue("[seh<300,11>nd][shih<100,19>ver<500,19>sdaw<300,18>nyur<300,18>spay<300,11>n]"); 
  queue("[shriy<300,19>kiy<300,19>ng][skow<300,18>swih<300,18>ll][shah<300,11>kyur<300,14>sow<300,11>ll]"); 
  queue("[siy<300,14>llyur<300,16>duh<300,13>mtuh<300,14>nay<300,11>t]"); 
}

//My Little Pony:
void song1(void) { 
  queue("[Mah<250,31>][Rih<250,34>][Tuh<250,31>][Pow<499,29>][Niy<799,34>]"); 
  queue("[Mah<250,27>][Rih<250,29>][Tuh<250,27>][Pow<499,26>][Niy<799,22>]"); 
  queue("[aa<499,24>][aa<499,26>][aa<499,27>][aa<499,29>][aa<900,27>]"); 

}

//Call Me Maybe:
void song2(void) { 
  queue("[_<1,20>]hey[_<400,12>]i[_<1,15>]just[_<1,20>]met[_<1,15>]yewwww[_<800,15>]and[_<1,12>]this[_<1,15>]is[_<1,24>]cray[_<1,20>]zee"); 
  queue("[_<1,20>]but[_<1,24>]here's[_<1,25>]my[_<1,24>]num[_<1,20>]ber[_<800,20>]so[_<1,24>]call[_<1,22>]me mey[_<1,20>]be"); 
  queue("[_<1,20>]and[_<200,12>]all[_<1,15>]the[_<1,20>]other[_<1,15>]boys[_<800,12>]try[_<1,15>]to[_<1,24>]chayyys[_<1,20>]me"); 
  queue("[_<1,20>]but[_<1,24>]here's[_<1,25>]my[_<1,24>]num[_<1,20>]ber[_<800,20>]so[_<1,24>]call[_<1,22>]me mey[_<1,20>]be"); 

}

/*
//Trololo:
void song3(void) { 
  queue("[llao<1600,25>][llao<350,20>][llao<300,18>][llao<1600,20>]"); 
  queue("[llao<350,13>][llao<300,15>][llao<1200,17>][llao<1200,20>][llao<600,17>][llao<1400,15>]"); 
  queue("[llao<130,20>][llao<130,22>][llao<130,23>][llao<130,24>][llao<1600,25>][llao<350,20>][llao<300,18>][llao<1600,20>]"); 
  queue("[llao<350,13>][llao<300,15>][llao<1200,17>][llao<1200,17>][llao<600,15>][llao<1000,13>]"); 
  queue("[llao<130,13>][llao<130,17>][llao<130,20>][llao<130,25>][llao<350,24>][llao<130,24>][llao<130,20>][llao<350,22>]"); 
  queue("[llao<130,22>][llao<130,18>][llao<620,20>][llao<130,8>][llao<130,12>][llao<130,15>][llao<130,18>][llao<1600,17>]"); 
  queue("[llao<130,13>][llao<130,17>][llao<130,20>][llao<130,25>][llao<350,24>][llao<130,24>][llao<130,20>][llao<350,22>]"); 
  queue("[llao<130,22>][llao<130,18>][llao<620,20>][llao<130,20>][llao<130,22>][llao<130,23>][llao<130,24>][llao<1600,25>]"); 
  queue("[llao<200,29>][llao<200,27>][llao<200,25>][llao<200,24>][llao<200,22>][llao<300,20>][llao<130,17>][llao<130,18>][llao<1200,20>]"); 
  queue("[llao<350,13>][llao<300,15>][llao<1200,17>][llao<1200,17>][llao<600,15>][llao<1000,13>]"); 

}
*/

/*
//Star Wars: Imperial March
void song4(void) { 
  queue("[dah<600,20>][dah<600,20>][dah<600,20>][dah<500,16>][dah<130,23>][dah<600,20>][dah<500,16>][dah<130,23>][dah<600,20>]"); 
  queue("[dah<600,27>][dah<600,27>][dah<600,27>][dah<500,28>][dah<130,23>][dah<600,19>][dah<500,15>][dah<130,23>][dah<600,20>]"); 
  queue("[dah<600,32>][dah<600,20>][dah<600,32>][dah<600,31>][dah<100,30>][dah<100,29>][dah<100,28>][dah<300,29>]"); 
  queue("[dah<150,18>][dah<600,28>][dah<600,27>][dah<100,26>][dah<100,25>][dah<100,24>][dah<100,26>]"); 
  queue("[dah<150,15>][dah<600,20>][dah<600,16>][dah<150,23>][dah<600,20>][dah<600,20>][dah<150,23>][dah<600,27>]"); 
  queue("[dah<600,32>][dah<600,20>][dah<600,32>][dah<600,31>][dah<100,30>][dah<100,29>][dah<100,28>][dah<300,29>]"); 
  queue("[dah<150,18>][dah<600,28>][dah<600,27>][dah<100,26>][dah<100,25>][dah<100,24>][dah<100,26>]"); 
  queue("[dah<150,15>][dah<600,20>][dah<600,16>][dah<150,23>][dah<600,20>][dah<600,16>][dah<150,23>][dah<600,20>]"); 

}
*/ 

//Friday:
void song3(void) { 
  queue("[fray<400,15>dey<400,12>][fray<400,15>dey<400,12>][gah<99,14>t][daw<200,14>n<99,14>aa<150,12>n]"); 
  queue("[fray<400,15>dey<400,12>][fray<400,15>dey<400,12>]"); 

}

//MLP Theme:
void song4(void) { 
  queue("[Mah<500,31>][Rih<500,34>][Tuh<500,31>][Pow<999,29>][Niy<1099,34>]"); 
  queue("[Mah<500,27>][Rih<500,29>][Tuh<500,27>][Pow<999,26>][Niy<1099,22>]"); 
  queue("[ah<999,24>][ah<999,26>][ah<999,27>][ah<999,29>]"); 

}

/*
//Happy Birthday John Madden:
void song7(void) { 
  queue("[hxae<300,10>piy<300,10>brr<600,12>th<100>dey<600,10>tuw<600,15>yu<1200,14>_<120>]"); 
  queue("[hxae<300,10>piy<300,10>brr<600,12>th<100>dey<600,10>tuw<600,17>yu<1200,15>_<120>]"); 
  queue("[hxae<300,10>piy<300,10>brr<600,22>th<100>dey<600,19>jh<100>aa<600,15>n<100>m<100>ae<600,14>d<50>dih<600,12>n] "); 
  queue("[hxae<300,20>piy<300,20>brr<600,19>th<100>dey<600,15>tuw<600,17>yu<1200,15>_<120>]"); 

}
*/

/*
//Batman:
void song8(void) { 
  queue("[nae<99,20>nae<99,20>nae<99,19>nae<99,19>nae<99,18>nae<99,18>nae<99,19>nae<99,19>bae<140,25>ttmae<600,25>nn]"); 

}
//Scooby Doo:
void song9(void) { 
  queue("[skuw<200,24>biy<200,24>duw<200,22>biy<200,22>duw<600,20>_<200>weh<200,22>rraa<400,24>rryu<600,17>]"); 

}
*/

/*
//Taps:
void song10(void) { 
  queue("[pr<600,18>][pr<200,18>][pr<1800,23>_>pr<600,18>][pr<300,23>][pr<1800,27>]"); 
  queue("[pr<600,18>][pr<300,23>][pr<1200,27>][pr<600,18>][pr<300,23>][pr<1200,27>][pr<600,18>][pr<300,23>][pr<1800,27>]"); 
  queue("[pr<600,23>][pr<300,27>][pr<1800,30>][pr<900,27>][pr<900,23>][pr<1800,18>]"); 
  queue("[pr<600,18>][pr<200,18>][pr<1800,23>]"); 

}
*/

/*
//2001 Space Odyssey Theme:
void song11(void) { 
  queue("[bah<1500,13>][bah<1500,20>][bah<1500,25>][bah<800,28>][bah<800,27>]"); 
  queue("[bah<200,8>][bah<200,13>][bah<200,8>][bah<200,13>][bah<200,8>][bah<200,13>] [bah<200,8>][bah<800,1>]"); 
  queue("[bah<1500,13>][bah<1500,20>][bah<1500,25>][bah<800,28>][bah<800,29>]"); 
  queue("[bah<200,8>][bah<200,13>][bah<200,8>][bah<200,13>][bah<200,8>][bah<200,13>] [bah<200,8>][bah<800,1>]"); 
  queue("[bah<1500,13>][bah<1500,20>][bah<1500,25>][bah<400,32>][bah<800,34>]"); 
  queue("[bah<400,22>][bah<400,24>][bah<1500,27>][bah<400,24>][bah<400,26>][bah<400,27>][bah<1600,29>]"); 
  queue("[bah<400,27>][bah<400,29>][bah<1600,31>][bah<1600,33>][bah<1600,34>]"); 

}
*/

/*

//John Madden Beautiful Choir
void song12(void) { 
  queue("[jhah<800,13>nmae<800,15>deh<800,17>]n[_<800,17>][jhah<800,17>nmae<800,18>deh<800,20>n]"); 
  queue("[jhah<400,20>ah<800,25>ah<400,24>ah<400,25>ah<800,20>ah<400,18>ah<800,17>nmae<800,15>deh<800,13>n]"); 

}

//A-team
void song13(void) { 
  queue("[dah<300,30>][dah<60,30>][dah<200,25>][dah<1000,30>][dah<200,23>][dah<400,25>][dah<700,18>]"); 

}

//Tetris tune
void song14(void) { 
  queue("[:t 430,500][:t 320,250][:t 350,250][:t 390,500][:t 350,250][:t 330,250][:t 290,500][:t 290,250][:t 350,250][:t 430,500]"); 

}

//Funky Town
void song15(void) { 
  queue("[:t 520,250][:t 520,250][:t 460,250][:t 520,500][:t 390,500][:t 390,250][:t 520,250][:t 700,250][:t 660,250][:t 520,500]"); 

}

//Mario song
void song16(void) { 
  queue("[:tone 165,200][:tone 165,400][:tone 165,400][:tone 131,200][:tone 165,400][:tone 196,800][:tone 98,1000]"); 

}

*/

//The whaling tune
void song5(void) { 
  queue("[_<1,13>]we're[_<1,18>]whalers[_<1,17]on[_<1,18>]the[_<1,20>]moon[_<400,13>]we[_<1,20>]carry[_<1,18>]a[_<1,20>]har[_<1,22>]poon"); 
  queue("[_<1,22>]but there[_<1,23>]aint no[_<1,15>]whales[_<1,23>]so we[_<1,22>]tell tall[_<1,18>]tales and"); 
  queue("[_<1,20>]sing our[_<1,18>]whale[_<1,17>]ing[_<1,18>]tune"); 

}
