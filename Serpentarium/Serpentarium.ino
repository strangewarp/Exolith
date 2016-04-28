

/*

    The Serpentarium is a MIDI sequencer for the "Snekbox" hardware.
    It runs on the device's primary ATMEGA328 microcontroller.
    To function fully, it must be connected to an upstream "Ophiuchus" unit.
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


// Timing vars
unsigned long ABSOLUTETIME = 0;
unsigned long ELAPSED = 0;
unsigned long TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Sequencing vars
boolean PLAYING = false; // Controls whether the sequences are iterating through their contents
unsigned int CURTICK = 1; // Current global sequencing tick
boolean DUMMYTICK = false; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences

// Cued-command flags, one per sequence
// Format:
// bit 1 - CUE [8]
// bit 2 - CUE [1]
// bit 3 - SEQ COLUMN [4]
// bit 4 - SEQ COLUMN [2]
// bit 5 - SEQ COLUMN [1]
// bit 6 - reserved
// bit 7 - reserved
// bit 8 - OFF
byte CMD[32] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Sequence-status flags, one per sequence
// Format:
// byte 1 - misc data
//    bit 1-2: play speed (2 1) [0 = off; 1 = 1; 2 = 1/2; 3 = 1/4]
//    bit 3-4: play-speed tick delay counter (2 1)
//    bit 5-7: wander chance (4 2 1) [0 = none; 1 = 14.5%; ... 7 = 100%]
//    bit 8: location by tick (512)
// byte 2 - location by tick (256 128 64 32 16 8 4 2 1)
// byte 3 - wander permissions
//    bit 1: up-left allowed
//    bit 2: up allowed
//    bit 3: up-right allowed
//    bit 4: left allowed
//    bit 5: right allowed
//    bit 6: down-left allowed
//    bit 7: down allowed
//    bit 8: down-right allowed
byte SEQ[32][3] = {
  {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
  {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
  {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
  {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}
};

// MIDI-IN vars
byte INBYTES[3] = {0, 0, 0};
byte INCOUNT = 0;
byte INTARGET = 0;
byte SYSBYTES[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte SYSCOUNT = 0;
boolean SYSIGNORE = false;
boolean SYSLATCH = false;

// MIDI SUSTAIN vars
// Format: SUSTAIN[n] = {channel, pitch, remaining duration in ticks}
byte SUSTAIN[8][3] = {
  {0, 0, 0}, {0, 0, 0},
  {0, 0, 0}, {0, 0, 0},
  {0, 0, 0}, {0, 0, 0},
  {0, 0, 0}, {0, 0, 0}
};


void parseSysex() {

  // testing code TODO remove
  /*
  lc.setRow(0, 0, 128 >> min(128, SYSBYTES[0]));
  lc.setRow(0, 1, 128 >> min(128, SYSBYTES[1]));
  lc.setRow(0, 2, 128 >> min(128, SYSBYTES[2]));
  lc.setRow(0, 3, 128 >> min(128, SYSBYTES[3]));
  lc.setRow(0, 4, 128 >> min(128, SYSBYTES[4]));
  lc.setRow(0, 5, 128 >> min(128, SYSBYTES[5]));
  lc.setRow(0, 6, 128 >> min(128, SYSBYTES[6]));
  lc.setRow(0, 7, 128 >> min(128, SYSBYTES[7]));
  */


  // Send SYSEX command to MIDI-OUT, acting as a SOFT THRU;
  // and also clear the SYSBYTES array
  for (byte i = 0; i < 11; i++) {
    Serial.write(SYSBYTES[i]);
    SYSBYTES[i] = 0;
  }

}


//testing code TODO remove
byte TESTTICK = 95;
byte TESTCOUNT = 1;

void parseMidi() {

  // testing code TODO remove
  //lc.setRow(0, 2, INBYTES[0] >> 7);
  //lc.setRow(0, 3, INBYTES[0] % 128);
  lc.setRow(0, TESTCOUNT, INBYTES[1]);
  TESTCOUNT = max(1, (TESTCOUNT % 8) + 1);
  //lc.setRow(0, 7, INBYTES[2]);


  // TODO: write the actual version of this function:
  //        * Record commands when in Record Mode
  //        * Change internal state based on Ophiuchus SYSEX commands - each containing a checksum byte tacked onto its command byte

  // TODO: adapt this
  /*
  if (b == 252) { // STOP command
    PLAYING = false;
  } else if (b == 251) { // CONTINUE command
    PLAYING = true;
  } else if (b == 250) { // START command
    PLAYING = true;
    CURTICK = 0;
    //iterateTick();
  } else if (b == 248) { // TIMING CLOCK command
    CURTICK = (CURTICK + 1) % 768;
    //iterateTick();
  */


}


void haltAllSustains() {
  for (byte i = 0; i < 8; i++) {
    if (SUSTAIN[i][2] > 0) {
      Serial.write(128 + SUSTAIN[i][0]);
      Serial.write(SUSTAIN[i][1]);
      Serial.write(127);
      SUSTAIN[i][0] = 0;
      SUSTAIN[i][1] = 0;
      SUSTAIN[i][2] = 0;
    }
  }
}


void resetSeqs() {
  for (byte i = 0; i < 32; i++) {
    CMD[i] = 0;
  }
}


void iterateSeqs() {

  // TODO write this part

}


void setup() {

  Serial.begin(31250);
  //Serial.begin(38400);//testing purposes
  
  kpd.setDebounceTime(1);

  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

}

void loop() {





  // Get keypad keys and launch commands accordingly
  if (kpd.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if (kpd.key[i].stateChanged) {
        if (kpd.key[i].kstate == PRESSED) {
          int kc = int(kpd.key[i].kchar) - 48;
          lc.setRow(0, 2, 128 >> (kc % 8));
          lc.setRow(0, 3, 128 >> (kc >> 3));

          //testing code TODO remove
          Serial.write(144);
          Serial.write(byte((floor(kc / 8)) * 8) + (kc >> 3));
          Serial.write(127);
          Serial.write(128);
          Serial.write(byte((floor(kc / 8) * 8)) + (kc >> 3));
          Serial.write(127);

        } else if (kpd.key[i].kstate == RELEASED) {
          lc.setRow(0, 2, 0);
          lc.setRow(0, 3, 0);
        }
      }
    }
  }

  // While new MIDI bytes are available to read from the MIDI-IN port...
  while (Serial.available() > 0) {

    // Get the frontmost incoming byte
    byte b = Serial.read();

    // TODO: Stress-test SYSEX processing with a Pd send-receive script & a USB-MIDI cable

    if (SYSIGNORE) { // If this is an ignoreable SYSEX command, send its bytes onward
      Serial.write(b);
      if (b == 247) { // If this was an END SYSEX byte, clear all SYSEX-tracking flags and counters
        SYSCOUNT = 0;
        SYSLATCH = false;
        SYSIGNORE = false;
      }
    } else if (SYSLATCH) { // Else, if we don't know whether this SYSEX command can be ignored, accumulate it
      SYSCOUNT++;
      if (b == 247) { // If this SYSEX command fell within the size limit, then parse it to see whether it's a Basilisk command
        parseSysex();
        SYSLATCH = false;
        SYSCOUNT = 0;
      } else if (SYSCOUNT > 11) { // Else, if this command went over the size limit, send it onward, and toggle SYSIGNORE for its remaining bytes
        SYSIGNORE = true;
        for (byte i = 0; i < 11; i++) {
          Serial.write(SYSBYTES[i]);
          SYSBYTES[i] = 0;
        }
      }
    } else { // Else, accumulate arbitrary commands...

      byte cmd = b - (b % 16); // Get the command-type of any given non-SYSEX command

      if (b == 252) { // STOP command
        Serial.write(b);
        PLAYING = false;
        haltAllSustains();
      } else if (b == 251) { // CONTINUE command
        Serial.write(b);
        PLAYING = true;
      } else if (b == 250) { // START command
        Serial.write(b);
        resetSeqs();
        PLAYING = true;

        //testing code TODO remove
        TESTTICK = 95;
        
      } else if (b == 248) { // TIMING CLOCK command
        Serial.write(b);
        iterateSeqs();

        //testing code TODO remove
        TESTTICK = (TESTTICK + 1) % 96;
        lc.setRow(0, 0, TESTTICK);
        
      } else if (b == 247) { // END SYSEX MESSAGE command
        // If you're dealing with an END-SYSEX command while SYSIGNORE and SYSLATCH are inactive,
        // then that implies you've received either an incomplete or corrupt SYSEX message,
        // and so nothing should be done here aside from sending it onward.
        Serial.write(b);
      } else if (
        (b == 244) // MISC SYSEX command
        || (b == 240) // START SYSEX MESSAGE command
      ) { // Anticipate an incoming arbitrarily-sized SYSEX message
        SYSLATCH = true;
        SYSBYTES[0] = b;
        SYSCOUNT = 1;
      } else if (
        (b == 242) // SONG POSITION POINTER command
        || (cmd == 224) // PITCH BEND command
        || (cmd == 176) // CONTROL CHANGE command
        || (cmd == 160) // POLY-KEY PRESSURE command
        || (cmd == 144) // NOTE-ON command
        || (cmd == 128) // NOTE-OFF command
      ) { // Anticipate an incoming 3-byte message
        INBYTES[0] = b;
        INCOUNT = 1;
        INTARGET = 3;
      } else if (
        (b == 243) // SONG SELECT command
        || (b == 241) // MIDI TIME CODE QUARTER FRAME command
        || (cmd == 208) // CHANNEL PRESSURE (AFTERTOUCH) command
        || (cmd == 192) // PROGRAM CHANGE command
      ) { // Anticipate an incoming 2-byte message
        INBYTES[0] = b;
        INCOUNT = 1;
        INTARGET = 2;
      } else { // Else, if this is a body-byte from a given non-SYSEX MIDI command...
        if (INBYTES[0] == 0) { // Ignore all body-data without proper command-bytes
          continue;
        } else { // Parse body-data if a proper command-byte was received
          INBYTES[INCOUNT] = b;
          INCOUNT++;
          if (INCOUNT == INTARGET) { // If all the command's bytes have been received, parse the command
            parseMidi();
          }
        }
      }

    }

  }

}

