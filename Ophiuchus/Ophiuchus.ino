

/*

    The Ophiuchus is a MIDI control program for the "Snekbox" hardware.
    It is a controller for every downstream "Serpentarium" unit.
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
#include <Key.h>
#include <Keypad.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>

// Initialize SdFat and SdFile objects, for filesystem manipulation on the SD-card
SdFat sd;
SdFile file;

// Initialize the object that controls the MAX7221's LED-grid
LedControl lc = LedControl(2, 3, 4, 1);

// Initialize the object that controls the Keypad buttons.
const byte ROWS PROGMEM = 4;
const byte COLS PROGMEM = 8;
char KEYS[ROWS][COLS] = {
  {'0', '1', '2', '3', '4', '5', '6', '7'},
  {'8', '9', ':', ';', '<', '=', '>', '?'},
  {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G'},
  {'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'}
};
byte rowpins[ROWS] = {5, 6, 7, 8};
byte colpins[COLS] = {9, 10, 14, 15, 16, 17, 18, 19};
Keypad kpd(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS);


// Control vars


// Timing vars
unsigned long TICKSIZE = 100000; // Size of 
unsigned long ELAPSED = 0;
unsigned long ABSOLUTETIME = 0;


void setup() {

  Serial.begin(31250);
  //Serial.begin(38400);//testing purposes
  
  kpd.setDebounceTime(1);

  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

}

void loop() {

  unsigned long micr = micros(); // Get the current time in microseconds
  if (micr < ABSOLUTETIME) { // If the microsecond-counter has overflowed its finite storage space and wrapped around to a small value...
    ELAPSED += micr + (4294967295 - ABSOLUTETIME); // Increase elapsed-ticks by said value, plus the amount between the previous tick and the overflow point
  } else { // Else, if the microsecond-counter hasn't overflowed...
    ELAPSED += micr - ABSOLUTETIME; // Increase the elapsed-ticks by the time since the previous tick
  }
  ABSOLUTETIME = micr; // Set the absolute-time to the current time, for use in the next main-loop cycle as a "previous tick" value

  // On every MIDI tick...
  if (ELAPSED >= TICKSIZE) {

    ELAPSED -= TICKSIZE; // Subtract the tick-size from the time elapsed since last tick, to compensate for overdraft

    


  }

  // Get keypad keys and launch commands accordingly
  if (kpd.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if (kpd.key[i].stateChanged) {
        if (kpd.key[i].kstate == PRESSED) {
          int kc = int(kpd.key[i].kchar) - 48;

        } else if (kpd.key[i].kstate == RELEASED) {

        }
      }
    }
  }

  // TODO: put MIDI-THRU code here



}
