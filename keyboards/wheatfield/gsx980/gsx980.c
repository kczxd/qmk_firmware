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

#include "gsx980.h"
#include "rgblight.h"

#include <avr/pgmspace.h>

#include "action_layer.h"

#define I2C_TIMEOUT     10

#define LED_INDICATORS

// Use the backlight as caps lock indicator?
// Best used if you only solder an LED under caps lock
#define BL_AS_CAPSLOCK 

__attribute__ ((weak))
void matrix_init_keymap(void) {}

__attribute__ ((weak))
void matrix_scan_keymap(void) {}

__attribute__ ((weak))
bool process_record_keymap(uint16_t keycode, keyrecord_t *record) {
  return true;
}
__attribute__ ((weak))
uint32_t layer_state_set_keymap (uint32_t state) {
  return state;
}
__attribute__ ((weak))
void led_set_keymap(uint8_t usb_led) {}

void matrix_init_user(void) {
  matrix_init_keymap();
}

void matrix_scan_user(void) {
  matrix_scan_keymap();
}

uint8_t led0 = 0, led1 = 0, led2 = 0;

void indicator_init(void) {
    // D0, D1, and D6 are the indicators and D4 is the backlight
    DDRD |= 0x53;
    PORTD |= 0x53;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
  return process_record_keymap(keycode, record);
}

#if defined(LED_INDICATORS)
void led_set_user(uint8_t usb_led) {

  if (IS_LED_ON(usb_led, USB_LED_NUM_LOCK)) {
    writePinHigh(D0);
  }
  else {
    writePinLow(D0);
  }

  if (IS_LED_ON(usb_led, USB_LED_CAPS_LOCK)) {
    writePinHigh(D1);
#if defined(BL_AS_CAPSLOCK)
    writePinHigh(D4);
#endif
  }
  else {
    writePinLow(D1);
#if defined(BL_AS_CAPSLOCK)
    writePinLow(D4);
#endif
  }

  if (IS_LED_ON(usb_led, USB_LED_SCROLL_LOCK)) {
    writePinHigh(D6);
  }
  else {
    writePinLow(D6);
  }
  led_set_keymap(usb_led);
}
#endif
