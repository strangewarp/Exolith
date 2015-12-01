
/*

    The Serpentarium is a MIDI sequencer for ATMEGA328P-based platforms.
    THIS CODE IS UNDER DEVELOPMENT AND DOESN'T DO ANYTHING!
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

// EEPROM data-storage library
#include <EEPROM.h>

// SD-card data-storage library
//#include <SdFat.h>


// Initialize the object that controls the MAX72**'s LED-grid
LedControl lc = LedControl(2, 3, 4, 1);

// Initialize the object that controls the MCP23017's digital pins,
// in a manner that drives a button-grid identical to those used by the Keypad library.
const byte ROWS PROGMEM = 6;
const byte COLS PROGMEM = 8;
const char KEYS[ROWS][COLS] PROGMEM = {
  {'0', '1', '2', '3', '4', '5', '6', '7'},
  {'8', '9', ':', ';', '<', '=', '>', '?'},
  {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G'},
  {'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'},
  {'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W'},
  {'X', 'Y', 'Z', '[', '\\', ']', '^', '_'}
};
byte rowpins[ROWS] = {0, 1, 2, 3, 4, 5};
byte colpins[COLS] = {8, 9, 10, 11, 12, 13, 14, 15};
Keypad_MC17 kpd(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS, 0x27);

// Scale types for the snap-to-scale feature.
// Format: SCALES[index][note] = (distance from previous note)
//     or: SCALES[index] = {1, 2, 3, 4, 5, 6, 7, 8}
const byte SCALES[15][7] PROGMEM = {
  // Scales 1-3 can be rotated into any of the most-consonant 7-note 12-tone scales:
  {2, 2, 1, 2, 2, 2, 1},  // Major scale
  {2, 2, 1, 2, 1, 2, 2},  // Major-minor scale
  {2, 2, 1, 1, 2, 2, 2},  // Major Locrian scale
  // Scales 4-8 are equal divisions of the octave, of various sizes:
  {2, 2, 2, 2, 2, 2, 0},  // EDO whole-tone scale
  {3, 3, 3, 3, 0, 0, 0},  // EDO tetratonic scale (Diminished seventh chord)
  {4, 4, 4, 0, 0, 0, 0},  // EDO triphonic scale (Augmented chord)
  {6, 6, 0, 0, 0, 0, 0},  // EDO tritone
  {12, 0, 0, 0, 0, 0, 0}, // Octave
  // Scales 9-15 are assorted useful chords:
  {4, 3, 5, 0, 0, 0, 0},  // Major chord
  {3, 4, 5, 0, 0, 0, 0},  // Minor chord
  {3, 3, 6, 0, 0, 0, 0},  // Diminished chord
  {4, 3, 3, 2, 0, 0, 0},  // Major seventh chord
  {3, 4, 3, 2, 0, 0, 0},  // Minor seventh chord
  {3, 3, 4, 2, 0, 0, 0},  // Half-diminished seventh chord
  {7, 5, 0, 0, 0, 0, 0}   // Power chord
};


// Potentiometer values (TODO: keeping these values in memory may not be necessary)
int potA = 0;
int potB = 0;


// Testing vars (TODO: remove these)
const unsigned long INTERVAL PROGMEM = 500;
unsigned long last = 0;
byte onoff = 0;


void setup() {

  // Clear old temp-variables from EEPROM storage
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
  }

  Serial.begin(31250);
  
  kpd.begin();
  kpd.setDebounceTime(1);

  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

}

void loop() {

  // MIDI testing code
  unsigned long mill = millis();
  if ((mill - last) >= INTERVAL) {
    last = mill;
    Serial.write((onoff == 1) ? 128 : 144);
    Serial.write(32);
    Serial.write(127);
    onoff = 1 - onoff;
  }

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

