/* Copyright 2017 Zach White <skullydazed@gmail.com>
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
#include "2x1800.h"
#include "audio.h"
#include "song_list.h"
#include "pincontrol.h"
#include "TTSWrapper.h"

bool tts_enable = false;

#define TTS_BUFSIZE 1024
#define PITCHCOUNT 8

int pitchindex = 2;
int tts_read = 0;
int tts_write = 0;
char tts_storage[TTS_BUFSIZE];
char tts_sentence[TTS_BUFSIZE + 1];

const byte PitchesP[] = { 1, 2, 4, 6, 8, 10, 13, 16 };

void tts_add(char *c) {
  int x;
  for (x=0; x < strlen(c); x++) {
    tts_storage[tts_write] = c[x];
    tts_write = (tts_write == TTS_BUFSIZE - 1 ? 0 : tts_write + 1);
    if (tts_read == tts_write)
      tts_read = (tts_read == TTS_BUFSIZE - 1 ? 0 : tts_read + 1);
  }
}

void tts_higher(void) {
  if (pitchindex >= PITCHCOUNT - 1) return;
  pitchindex++;
  setPitch(PitchesP[pitchindex]);
}

void tts_lower(void) {
  if (pitchindex < 1) return;
  pitchindex--;
  setPitch(PitchesP[pitchindex]);
}

void tts_recite(void) {
  int x;
  for (x=0; x<TTS_BUFSIZE && tts_read != tts_write; x++){
    tts_sentence[x] = tts_storage[tts_read];
    tts_read = (tts_read == TTS_BUFSIZE - 1 ? 0 : tts_read + 1);
  }
  tts_sentence[x] = 0;

  tts_read = 0;
  tts_write = 0;

  sayText(tts_sentence);
}

enum keyboard_layers {
  LAYER_QWERTY = 0,
  LAYER_UPPER,
  LAYER_LOWER,
  LAYER_ADJUST,
};

void tts_toggle(void){
  tts_enable = !tts_enable;
  if (!tts_enable) {
    tts_read = 0;
    tts_write = 0;
  }
}

enum keyboard_macros {
  MACRO_QWERTY = 0,
  MACRO_UPPER,
  MACRO_LOWER,
  MACRO_FUNCTION,
  MACRO_MOUSE,
  MACRO_TIMBRE_1,
  MACRO_TIMBRE_2,
  MACRO_TIMBRE_3,
  MACRO_TIMBRE_4,
  MACRO_TEMPO_U,
  MACRO_TEMPO_D,
  MACRO_TONE_DEFAULT,
  MACRO_MUSIC_TOGGLE,
  MACRO_AUDIO_TOGGLE,
  MACRO_INC_VOICE,
  MACRO_DEC_VOICE,
  MACRO_BACKLIGHT,
  MACRO_BREATH_TOGGLE,
  MACRO_BREATH_SPEED_INC,
  MACRO_BREATH_SPEED_DEC,
  MACRO_BREATH_DEFAULT,
  MACRO_MOUSE_MOVE_UL,
  MACRO_MOUSE_MOVE_UR,
  MACRO_MOUSE_MOVE_DL,
  MACRO_MOUSE_MOVE_DR,
  MACRO_HELP_1,
  MACRO_HELP_2,
  MACRO_HELP_3,
  MACRO_HELP_4,
  MACRO_HELP_5,
  MACRO_HELP_6,
  MACRO_HELP_7,
  MACRO_HELP_8,
  MACRO_HELP_9,
};

enum my_keycodes {
  TTS_TOGGLE = SAFE_RANGE,
  TTS_HIGHER,
  TTS_LOWER,
  TTS_RECITE,
  RAISE,
  LOWER
};

#define M_UPPER             M(MACRO_UPPER)
#define M_LOWER             M(MACRO_LOWER)
#define M_FUNCT             M(MACRO_FUNCTION)
#define M_MOUSE             M(MACRO_MOUSE)
#define TIMBR_1             M(MACRO_TIMBRE_1)
#define TIMBR_2             M(MACRO_TIMBRE_2)
#define TIMBR_3             M(MACRO_TIMBRE_3)
#define TIMBR_4             M(MACRO_TIMBRE_4)
#define TMPO_UP             M(MACRO_TEMPO_U)
#define TMPO_DN             M(MACRO_TEMPO_D)
#define TMPO_DF             M(MACRO_TONE_DEFAULT)
#define M_BACKL             M(MACRO_BACKLIGHT)
#define M_BRTOG             M(MACRO_BREATH_TOGGLE)
#define M_BSPDU             M(MACRO_BREATH_SPEED_INC)
#define M_BSPDD             M(MACRO_BREATH_SPEED_DEC)
#define M_BDFLT             M(MACRO_BREATH_DEFAULT)
#define M_MS_UL             M(MACRO_MOUSE_MOVE_UL)
#define M_MS_UR             M(MACRO_MOUSE_MOVE_UR)
#define M_MS_DL             M(MACRO_MOUSE_MOVE_DL)
#define M_MS_DR             M(MACRO_MOUSE_MOVE_DR)
#define M_HELP1             M(MACRO_HELP_1)
#define M_HELP2             M(MACRO_HELP_2)
#define M_HELP3             M(MACRO_HELP_3)
#define M_HELP4             M(MACRO_HELP_4)
#define M_HELP5             M(MACRO_HELP_5)
#define M_HELP6             M(MACRO_HELP_6)
#define M_HELP7             M(MACRO_HELP_7)
#define M_HELP8             M(MACRO_HELP_8)
#define M_HELP9             M(MACRO_HELP_9)


#define VC_UP               M(MACRO_INC_VOICE)
#define VC_DOWN             M(MACRO_DEC_VOICE)


#define SC_UNDO             LCTL(KC_Z)
#define SC_REDO             LCTL(KC_Y)
#define SC_CUT              LCTL(KC_X)
#define SC_COPY             LCTL(KC_C)
#define SC_PSTE             LCTL(KC_V)
#define SC_SELA             LCTL(KC_A)
#define SC_SAVE             LCTL(KC_S)
#define SC_OPEN             LCTL(KC_O)
#define SC_ACLS             LALT(KC_F4)
#define SC_CCLS             LCTL(KC_F4)

#define TG_NKRO             MAGIC_TOGGLE_NKRO
#define OS_SHFT             KC_FN0

#define _______             KC_TRNS
#define XXXXXXX             KC_NO
#define ________________    _______, _______
#define XXXXXXXXXXXXXXXX    XXXXXXX, XXXXXXX


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[LAYER_QWERTY] = LAYOUT_4U_SPACE(
      KC_HOME, KC_END,  KC_PGUP, KC_PGDN,       KC_ESC,    KC_F1,   KC_F2,   KC_F3,   KC_F4,     KC_F5,   KC_F6,   KC_F7,   KC_F8,     KC_F9,   KC_F10,   KC_F11,  KC_F12,        KC_PSCR, KC_SLCK, KC_PAUS, KC_INS,  \
                                                                                                                                                                                                                      \
      KC_PMNS, KC_NLCK, KC_PSLS, KC_PAST,      KC_GRV,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,   KC_BSPC,     KC_NLCK, KC_PSLS, KC_PAST, KC_PMNS, \
      KC_PPLS, KC_P9,   KC_P8,   KC_P7,        KC_TAB,    KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P, KC_LBRC, KC_RBRC, KC_BSLS,       KC_P7, KC_P8, KC_P9, KC_PSLS,       \
               KC_P6,   KC_P5,   KC_P4,        KC_CAPS,     KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,    KC_QUOT, KC_ENT,         KC_P4, KC_P5, KC_P6,                \
      KC_PENT, KC_P3,   KC_P2,   KC_P1,     KC_UP,  KC_LSFT,    KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M, KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,      KC_UP,      KC_P1, KC_P2, KC_P3, KC_PENT,       \
               KC_PDOT,   KC_P0,  KC_LEFT, KC_DOWN, KC_RGHT,  KC_LCTL, KC_LGUI, KC_LALT,                 KC_SPC,         RAISE, KC_RGUI, KC_APP, KC_RCTL,  KC_LEFT, KC_DOWN, KC_RGHT,  KC_P0, KC_PDOT               \
),


[LAYER_UPPER] = LAYOUT_4U_SPACE(
      KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS,       KC_TRNS,    KC_TRNS,   KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,   KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,   KC_TRNS,   KC_TRNS,  KC_TRNS,        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  \
                                                                                                                                                                                                                      \
     TTS_LOWER, KC_TRNS, KC_TRNS, TTS_TOGGLE,      KC_TRNS,  KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS,   KC_TRNS,     KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, \
      TTS_HIGHER, KC_TRNS,   KC_TRNS,   KC_TRNS,        KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, RGB_TOG,       KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       \
               KC_TRNS,   KC_TRNS,   KC_TRNS,        KC_TRNS,     KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, AU_TOG,         KC_TRNS, KC_TRNS, KC_TRNS,                \
      TTS_RECITE, KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,  KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,   KC_TRNS,      KC_TRNS,      KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       \
               KC_TRNS,   KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,                 KC_TRNS,         KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  RGB_RMOD, KC_TRNS, RGB_MOD,  KC_TRNS, KC_TRNS              \
),

[LAYER_LOWER] = LAYOUT_4U_SPACE(
      KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS,       KC_TRNS,    KC_TRNS,   KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,   KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,   KC_TRNS,   KC_TRNS,  KC_TRNS,        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  \
                                                                                                                                                                                                                      \
      KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,      KC_TRNS,  KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS,   KC_TRNS,     KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, \
      KC_TRNS, KC_TRNS,   KC_TRNS,   KC_TRNS,        KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       \
               KC_TRNS,   KC_TRNS,   KC_TRNS,        KC_TRNS,     KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS,         KC_TRNS, KC_TRNS, KC_TRNS,                \
      KC_TRNS, KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,  KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,   KC_TRNS,      KC_TRNS,      KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       \
               KC_TRNS,   KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,                 KC_TRNS,         KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS              \
),

[LAYER_ADJUST] = LAYOUT_4U_SPACE(
      KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS,       KC_TRNS,    KC_TRNS,   KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,   KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,   KC_TRNS,   KC_TRNS,  KC_TRNS,        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  \
                                                                                                                                                                                                                      \
      KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,      KC_TRNS,  KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS,   KC_TRNS,     KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, \
      KC_TRNS, KC_TRNS,   KC_TRNS,   KC_TRNS,        KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       \
               KC_TRNS,   KC_TRNS,   KC_TRNS,        KC_TRNS,     KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS,         KC_TRNS, KC_TRNS, KC_TRNS,                \
      KC_TRNS, KC_TRNS,   KC_TRNS,   KC_TRNS,     KC_TRNS,  KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,   KC_TRNS,      KC_TRNS,      KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,       \
               KC_TRNS,   KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,                 KC_TRNS,         KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS,  KC_TRNS, KC_TRNS              \
)

};

void led_set_user(uint8_t usb_led)
{
    static uint8_t old_usb_led = 0;

    _delay_ms(10); // gets rid of tick

    if ((usb_led & (1<<USB_LED_CAPS_LOCK)) && !(old_usb_led & (1<<USB_LED_CAPS_LOCK)))
    {
            // If CAPS LK LED is turning on...
    }
    else if (!(usb_led & (1<<USB_LED_CAPS_LOCK)) && (old_usb_led & (1<<USB_LED_CAPS_LOCK)))
    {
            // If CAPS LK LED is turning off...
    }
    else if ((usb_led & (1<<USB_LED_NUM_LOCK)) && !(old_usb_led & (1<<USB_LED_NUM_LOCK)))
    {
            // If NUM LK LED is turning on...
    }
    else if (!(usb_led & (1<<USB_LED_NUM_LOCK)) && (old_usb_led & (1<<USB_LED_NUM_LOCK)))
    {
            // If NUM LED is turning off...
    }
    else if ((usb_led & (1<<USB_LED_SCROLL_LOCK)) && !(old_usb_led & (1<<USB_LED_SCROLL_LOCK)))
    {
            // If SCROLL LK LED is turning on...
    }
    else if (!(usb_led & (1<<USB_LED_SCROLL_LOCK)) && (old_usb_led & (1<<USB_LED_SCROLL_LOCK)))
    {
            // If SCROLL LED is turning off...
    }
    old_usb_led = usb_led;
}

void startup_user()
{
    _delay_ms(10); // gets rid of tick

    //Set both pins to output
    DDRB |= _BV(PORTB7);
    DDRC |= _BV(PORTC4);

    //Set C4 to low and leave it there
    digitalWrite(C4, PinLevelLow);

    //Init the TTS engine
    TTSInit();

}

void shutdown_user()
{
    _delay_ms(2000);
}

void subprocess_record(char *code, uint16_t keycode, keyrecord_t *record) {
  if (record->event.pressed) {
    tts_add(code);
    if(tts_enable) {
      sayText(code);
    }
  }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {

  switch (keycode) {

    case LOWER:
      if (record->event.pressed) {
        layer_on(LAYER_LOWER);
        update_tri_layer(LAYER_LOWER, LAYER_UPPER, LAYER_ADJUST);
      } else {
        layer_off(LAYER_LOWER);
        update_tri_layer(LAYER_LOWER, LAYER_UPPER, LAYER_ADJUST);
      }
      return false;
      break;
    case RAISE:
      if (record->event.pressed) {
        layer_on(LAYER_UPPER);
        update_tri_layer(LAYER_LOWER, LAYER_UPPER, LAYER_ADJUST);
      } else {
        layer_off(LAYER_UPPER);
        update_tri_layer(LAYER_LOWER, LAYER_UPPER, LAYER_ADJUST);
      }
      return false;
      break;

    case TTS_LOWER:
      if (record->event.pressed) {
        tts_lower();
      }
      return false;
      break;
    case TTS_HIGHER:
      if (record->event.pressed) {
        tts_higher();
      }
      break;

    case TTS_TOGGLE:
      if (record->event.pressed) {
        tts_toggle();
      }
      return false;
      break;
    case TTS_RECITE:
      if (record->event.pressed) {
        tts_recite();
      }
      break;
    case KC_SPACE:
      if (record->event.pressed) {
        tts_add(" ");
        if(tts_enable) {
          sayText("Space");
        }
      }
      break;
    case KC_ENTER:
      if (record->event.pressed) {
        tts_add(" ");
        if(tts_enable) {
          sayText("Enter");
        }
      }
      break;
    case KC_A:
      subprocess_record("a", keycode, record);
      break;
    case KC_B:
      subprocess_record("b", keycode, record);
      break;
    case KC_C:
      subprocess_record("c", keycode, record);
      break;
    case KC_D:
      subprocess_record("d", keycode, record);
      break;
    case KC_E:
      subprocess_record("e", keycode, record);
      break;
    case KC_F:
      subprocess_record("f", keycode, record);
      break;
    case KC_G:
      subprocess_record("g", keycode, record);
      break;
    case KC_H:
      subprocess_record("h", keycode, record);
      break;
    case KC_I:
      subprocess_record("i", keycode, record);
      break;
    case KC_J:
      subprocess_record("j", keycode, record);
      break;
    case KC_K:
      subprocess_record("k", keycode, record);
      break;
    case KC_L:
      subprocess_record("l", keycode, record);
      break;
    case KC_M:
      subprocess_record("m", keycode, record);
      break;
    case KC_N:
      subprocess_record("n", keycode, record);
      break;
    case KC_O:
      subprocess_record("o", keycode, record);
      break;
    case KC_P:
      subprocess_record("p", keycode, record);
      break;
    case KC_Q:
      subprocess_record("q", keycode, record);
      break;
    case KC_R:
      subprocess_record("r", keycode, record);
      break;
    case KC_S:
      subprocess_record("s", keycode, record);
      break;
    case KC_T:
      subprocess_record("t", keycode, record);
      break;
    case KC_U:
      subprocess_record("u", keycode, record);
      break;
    case KC_V:
      subprocess_record("v", keycode, record);
      break;
    case KC_W:
      subprocess_record("w", keycode, record);
      break;
    case KC_X:
      subprocess_record("x", keycode, record);
      break;
    case KC_Y:
      subprocess_record("y", keycode, record);
      break;
    case KC_Z:
      subprocess_record("z", keycode, record);
      break;
    case KC_1:
      subprocess_record("1", keycode, record);
      break;
    case KC_2:
      subprocess_record("2", keycode, record);
      break;
    case KC_3:
      subprocess_record("3", keycode, record);
      break;
    case KC_4:
      subprocess_record("4", keycode, record);
      break;
    case KC_5:
      subprocess_record("5", keycode, record);
      break;
    case KC_6:
      subprocess_record("6", keycode, record);
      break;
    case KC_7:
      subprocess_record("7", keycode, record);
      break;
    case KC_8:
      subprocess_record("8", keycode, record);
      break;
    case KC_9:
      subprocess_record("9", keycode, record);
      break;
    case KC_0:
      subprocess_record("0", keycode, record);
      break;
  }

  return true;
}

