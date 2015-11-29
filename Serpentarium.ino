
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

// All note-ons in the active tick, to be stored as sustain-values via SDfat.
// Format: noteOns[channel][index] = {pitch, duration}
byte noteOns[16][4][2];

// Scale types for the snap-to-scale feature.
// Format: scales[index][note] = (distance from previous note)
//     or: scales[index] = {1, 2, 3, 4, 5, 6, 7, 8}
const byte scales[15][7] PROGMEM = {
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

// Activity values for 64 sequences, crammed into 8 bytes as binary 0/1 values.
byte active[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Binary values per seq: active data for both CUE and SWAP commands.
// Format: 00(cue) 000000(swap)
byte cueSwap[64];

// Binary values per seq: the portions of their loops to be skipped.
// Format: 0(1) 0(2) 0(3) 0(4) 0(5) 0(6) 0(7) 0(8)
byte skip[64];

// Binary values per 2 seqs: scale-quantize type.
// Format: 0000(scale type)
byte scale[32];

// Binary values per seq: pitch offset.
// Format: 0(negative or positive) 0000000(pitch distance)
byte pitch[64];

// Binary values per seq: chance of directional wandering.
// Format: 0000(chance) 0(up) 0(left) 0(right) 0(down)
byte wander[64];

// Binary values per seq: chance of note-event scattering.
// Format: 0000(chance) 00(left dist) 00(right dist)
byte scatter[64];

// Binary values per seq: scatter loop-resolution random seeds.
// Format: 00000000(seed = int(n << 8))
byte scatterSeed[64];

// Binary values per seq: scatter note-resolution random values.
// Format: 00000000(current rand value)
byte scatterRand[64];


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

