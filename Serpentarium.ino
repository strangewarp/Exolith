

/*

    The Serpentarium is a MIDI sequencer for ATMEGA1280/2560-based platforms.
    THIS CODE IS UNDER DEVELOPMENT AND DOESN'T DO ANYTHING!
    Copyright (C) 2016-onward, Christian D. Madsen (sevenplagues@gmail.com).

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


// Digital-pin keypad library
//#include <Key.h>
//#include <Keypad.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>

// testing TODO remove
//SdFat sd;
//SdFile myFile;

// Initialize the object that controls the MAX7221's LED-grid
LedControl lc = LedControl(26, 27, 28, 1);

// Initialize the object that controls the MCP23017's digital pins,
// in a manner that drives a button-grid identical to those used by the Keypad library.
/*const byte ROWS PROGMEM = 8;
const byte COLS PROGMEM = 8;
const char KEYS[ROWS][COLS] PROGMEM = {
  {'0', '1', '2', '3', '4', '5', '6', '7'},
  {'8', '9', ':', ';', '<', '=', '>', '?'},
  {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G'},
  {'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'},
  {'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W'},
  {'X', 'Y', 'Z', '[', '\\', ']', '^', '_'},
  {'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g'},
  {'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o'}
};
byte rowpins[ROWS] = {38, 39, 40, 41, 42, 43, 44, 45};
byte colpins[COLS] = {46, 47, 48, 49, 50, 51, 52, 53};
Keypad kpd = Keypad(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS);
*/
// Scale types for the snap-to-scale feature.
// Format: SCALES[index][note] = (distance from previous note)
//     or: SCALES[index] = {1, 2, 3, 4, 5, 6, 7, 8}
const byte SCALES[41][7] PROGMEM = {

  // 1-3: Heptatonia prima, secunda, and tertia scales
  {2, 2, 1, 2, 2, 2, 1},  // C  D  E  F  G  A  B  C: Prima root scale (major scale)
  {2, 1, 2, 2, 2, 2, 1},  // C  D  Eb F  G  A  B  C: Secunda scale (melodic minor)
  {1, 2, 2, 2, 2, 2, 1},  // C  Db Eb F  G  A  B  C: Tertia scale (whole-tone hop)

  // 4-24: All unique heptatonic scales with one 3-semitone interval
  {2, 2, 2, 1, 1, 3, 1},  // Doric minor roma scale
  {2, 2, 1, 2, 1, 3, 1},  // Harmonic major scale
  {2, 2, 1, 1, 2, 3, 1},
  {2, 2, 1, 1, 1, 3, 2},
  {2, 1, 2, 2, 1, 3, 1},  // Harmonic minor scale
  {2, 1, 2, 1, 2, 3, 1},
  {2, 1, 2, 1, 1, 3, 2},
  {2, 1, 1, 2, 2, 3, 1},
  {2, 1, 1, 2, 1, 3, 2},
  {2, 1, 1, 1, 2, 3, 2},
  {1, 2, 2, 2, 1, 3, 1},  // Neapolitan minor scale
  {1, 2, 2, 1, 2, 3, 1},
  {1, 2, 2, 1, 1, 3, 2},
  {1, 2, 1, 2, 2, 3, 1},
  {1, 2, 1, 2, 1, 3, 2},  // Romanian major scale
  {1, 2, 1, 1, 2, 3, 2},
  {1, 1, 2, 2, 2, 3, 1},
  {1, 1, 2, 2, 1, 3, 2},
  {1, 1, 2, 1, 2, 3, 2},
  {1, 1, 1, 2, 2, 3, 2},

  // 25-27: EDO scales
  {2, 2, 2, 2, 2, 2, 0},  // EDO whole-tone scale
  {3, 3, 3, 3, 0, 0, 0},  // EDO tetratonic scale (Diminished seventh chord)
  {4, 4, 4, 0, 0, 0, 0},  // EDO triphonic scale (Augmented chord)

  // 28-33: 2-note chords
  {1, 11, 0, 0, 0, 0, 0},
  {2, 10, 0, 0, 0, 0, 0},
  {3, 9, 0, 0, 0, 0, 0},
  {4, 8, 0, 0, 0, 0, 0},
  {5, 7, 0, 0, 0, 0, 0},
  {6, 6, 0, 0, 0, 0, 0},  // Tritone

  // 34-34: 1-note unities
  {12, 0, 0, 0, 0, 0, 0}, // Octave

  // 35-63: Assorted useful chords:
  {7, 5, 0, 0, 0, 0, 0},   // Power chord
  {4, 3, 5, 0, 0, 0, 0},  // Major chord
  {3, 4, 5, 0, 0, 0, 0},  // Minor chord
  {3, 3, 6, 0, 0, 0, 0},  // Diminished chord
  {4, 3, 3, 2, 0, 0, 0},  // Major seventh chord
  {3, 4, 3, 2, 0, 0, 0},  // Minor seventh chord
  {3, 3, 4, 2, 0, 0, 0}  // Half-diminished seventh chord

};


// Most recent microsecond at which a MIDI CLOCK tick occurred
unsigned long lastmicros = 0;

// Controls whether the sequences are iterating through their contents
boolean playing = false;

// Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
boolean dummytick = false;

// Tracks the current global tick, and the next global tick, for sequence-playing
unsigned int curtick = 1;
unsigned int nexttick = 1;

// Potentiometer values (TODO: keeping these values in memory may not be necessary)
const byte P_TOTAL PROGMEM = 2;
const byte P_SMOOTH PROGMEM = 64;
byte pPins[P_TOTAL] = {0, 1}; // Analog-in pins connected to the potentiometers
int pTot[P_TOTAL];
int pLastAvg[P_TOTAL];
byte pState[P_TOTAL];
int pBuf[P_TOTAL][P_SMOOTH];
byte pIndex[P_TOTAL];
byte pMetaIndex = 0;
unsigned long pTime = 0;

void initializePotentiometers() {
  for (int i = 0; i < P_TOTAL; i++) {
    pTot[i] = 0;
    pLastAvg[i] = 0;
    pState[i] = 0;
    pIndex[i] = 0;
    for (int j = 0; j < P_SMOOTH; j++) {
      pBuf[i][j] = 0;
    }
  }
}

void updateAllPotentiometers() {
  updatePotentiometer(pMetaIndex);
  pIndex[pMetaIndex] = (pIndex[pMetaIndex] + 1) % P_SMOOTH;
  pMetaIndex = (pMetaIndex + 1) % P_TOTAL;
}

void updatePotentiometer(byte n) {
  pTot[n] -= pBuf[n][pIndex[n]];
  int r = analogRead(pPins[n]);
  pTot[n] += r;
  pBuf[n][pIndex[n]] = r;
  int avg = pTot[n] / P_SMOOTH;
  int change = abs(avg - pLastAvg[n]);
  if (change >= 8) {
    pLastAvg[n] = avg;
    if (avg < 0) {
      avg = -512 - avg;
    } else {
      avg = 512 - avg;
    }
    avg += 512;
    Serial.println(avg);//debugging
    pLastAvg[n] = avg;
    pState[n] = min(127, floor(avg / 8));
    lc.setRow(0, 0 + n, pState[n] & 255);
  }
}



// Testing vars (TODO: remove these)
const unsigned long INTERVAL PROGMEM = 500;
unsigned long last = 0;
byte onoff = 0;


void setup() {

  initializePotentiometers();
  
  //Serial.begin(31250);
  Serial.begin(38400);//testing purposes
  
//  kpd.setDebounceTime(2);

  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

}

void loop() {

  // MIDI testing code
/*
  unsigned long mill = millis();
  if ((mill - last) >= INTERVAL) {
    last = mill;
    Serial.write((onoff == 1) ? 128 : 144);
    Serial.write(32);
    Serial.write(127);
    onoff = 1 - onoff;
  }
*/

  updateAllPotentiometers();

  // keypad testing code
/*
  if (kpd.getKeys()) {
    lc.clearDisplay(0);
    for (byte i = 0; i < LIST_MAX; i++) {
      if (kpd.key[i].stateChanged) {
        if (kpd.key[i].kstate == PRESSED) {
          byte kc = kpd.key[i].kchar - 48;
          lc.setRow(0, 0, 128 >> (kc % 8));
          lc.setRow(0, 1, 128 >> (kc >> 3));
        }
      }
    }
  }
*/

}

void iterateSeqs() {

  

}
