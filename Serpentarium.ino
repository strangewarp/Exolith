
/*

    The Serpentarium is a MIDI sequencer for ATMEGA328P-based platforms.
    Copyright (C) 2015 Christian D. Madsen (sevenplagues@gmail.com).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    
*/

// MCP23017 keypad libraries
#include <Wire.h>
//#include <Key.h>
#include <Keypad.h>
#include <Keypad_MC17.h>

// MAX72** library
#include <LedControl.h>

// SD-card library
//#include <SdFat.h>


// Initialize the object that controls the MAX72**'s LED-grid
LedControl lc = LedControl(2, 3, 4, 1);

// Initialize the object that controls the MCP23017's digital pins,
// in a manner that drives a button-grid identical to those used by the Keypad library.
const byte ROWS = 6;
const byte COLS = 8;
char keys[ROWS][COLS] = {
  {'0', '1', '2', '3', '4', '5', '6', '7'},
  {'8', '9', ':', ';', '<', '=', '>', '?'},
  {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G'},
  {'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'},
  {'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W'},
  {'X', 'Y', 'Z', '[', '\\', ']', '^', '_'}
};
byte rowpins[ROWS] = {0, 1, 2, 3, 4, 5};
byte colpins[COLS] = {8, 9, 10, 11, 12, 13, 14, 15};
Keypad_MC17 kpd(makeKeymap(keys), rowpins, colpins, ROWS, COLS, 0x27);

// Array for tracking MIDI-sustains on every channel.
// Data format:
// sustain[chan][index] = pitch
// sustick[chan][index] = ticks
//   ...where "chan" is MIDI channel, "index" is in order of most recent note,
//            "pitch" is note-pitch, and "ticks" is the remaining ticks before note-off.
//   ...additionally, a "sustain" value of "255" means its sustain-slot is inactive.
byte sustain[16][4] = {
  {255, 255, 255, 255}, {255, 255, 255, 255},
  {255, 255, 255, 255}, {255, 255, 255, 255},
  {255, 255, 255, 255}, {255, 255, 255, 255},
  {255, 255, 255, 255}, {255, 255, 255, 255},
  {255, 255, 255, 255}, {255, 255, 255, 255},
  {255, 255, 255, 255}, {255, 255, 255, 255},
  {255, 255, 255, 255}, {255, 255, 255, 255},
  {255, 255, 255, 255}, {255, 255, 255, 255}
};
unsigned int sustick[16][4] = {
  {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
  {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
  {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
  {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}
};

byte active[32] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Potentiometer values (TODO: keeping these values in memory may not be necessary)
int potA = 0;
int potB = 0;

void setup() {

  Serial.begin(31250);
  
  kpd.begin();
  kpd.setDebounceTime(1);

  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

}

void loop() {

  // potentiometer testing code
  int oldA = potA;
  int oldB = potB;
  potA = analogRead(0);
  potB = analogRead(1);
  if (oldA != potA) {
    lc.setRow(0, 2, potA >> 8);
    lc.setRow(0, 4, potA & 255);
  }
  if (oldB != potB) {
    lc.setRow(0, 3, potB >> 8);
    lc.setRow(0, 5, potB & 255);
  }

  // keypad testing code
  if (kpd.getKeys()) {
    lc.clearDisplay(0);
    for (byte i = 0; i < LIST_MAX; i++) {
      if (kpd.key[i].stateChanged) {
        if (kpd.key[i].kstate == PRESSED) {
          byte kc = kpd.key[i].kchar - 48;
          lc.setRow(0, 0, 128 >> (kc % 8));
          lc.setRow(0, 1, 128 >> byte(floor(kc / 8)));
        }
      }
    }
  }

}

