
/*

    Lossyloop is a realtime lossy MIDI looper for the "Snekbox" hardware.
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

// LED pseudo-PWM vars
const byte BLINKLENGTH PROGMEM = 64; // Length, in main-loop cycles, of each pseudo-PWM blink-cycle
byte BLINKELAPSED = 0; // Amount of time elapsed within the current blink-cycle
byte BLINKVAL[3] = {0, 0, 0}; // All persistent visibility states for dimmed LEDs. 0 = off; 1 = dimmed
byte BLINKVISIBLE[3] = {0, 0, 0}; // All current PWM states for all LEDs in rows 0, 2, and 4, stored as individual bits. 0 = PWM low; 1 = PWM high

// Sequencing vars
boolean PLAYING = false; // Controls whether the sequences are iterating through their contents
word CURTICK = 1; // Current global sequencing tick

// MIDI-CATCH vars
byte CATCHBYTES[3] = {0, 0, 0};
byte CATCHCOUNT = 0;
byte CATCHTARGET = 0;
boolean SYSIGNORE = false;

// MIDI-OUT SUSTAIN vars
// Format: SUSTAIN[n] = {channel, pitch, remaining duration in ticks}
// Note: "255" in bytes 1 and 2 means "empty slot"
byte SUSTAIN[8][3] = {
  {255, 255, 0}, {255, 255, 0},
  {255, 255, 0}, {255, 255, 0},
  {255, 255, 0}, {255, 255, 0},
  {255, 255, 0}, {255, 255, 0}
};

// MIDI-IN SUSTAIN vars
// Format: INSUSTAIN[n] = {channel, pitch, elapsed duration in ticks}
// Note: "255" in bytes 1 and 2 means "empty slot"
// Note: these are not cleared in haltAllSustains, as proper behavior of upstream devices is assumed;
//       however, individual INSUSTAIN entries are concluded and recorded after a beat's worth of ticks have elapsed.
byte INSUSTAIN[3][3] = { {255, 255, 0}, {255, 255, 0}, {255, 255, 0} };

// Contains all currently-existing notes in each lane
// Format: LANE[n] = {channel, pitch, velocity, duration}
// Note: "255" in bytes 0 and 1 means "empty slot"
// Note: If byte 0 is between 128 and 255, that means "latent note"
byte LANE[3][8][4];

// Contains all metadata for each lane
// Format: LANEMETA[n] = {ACTIVE, PITCH, VELO, DUR, VANISH, BLEED, LEN, CHAN}
// Key:
//   [0] ACTIVE: Toggles whether the lane is currently recording and playing notes, and whether it is incremented by note-ins or the global clock
//                 Bit 0 (ACTIVITY):    off/on
//                 Bit 1 (CLOCK TRIG):  note/clock
//                 Bits 2-7 (reserved): (reserved)
//   [1] PITCH:  Toggles possible pitch-modulation distances.
//                 Bits 0-3: -5, -4, -3, -2
//                 Bits 4-7: 2, 3, 4, 5
//   [2] VELO:   Toggles possible velocity-modulation distances.
//                 Bits 0-3: -100, -50, -25, -10
//                 Bits 4-7: 10, 25, 50, 100
//   [3] DUR:    Toggles possible duration-modulation distances.
//                 Bits 0-3: -48, -24, -12, -6
//                 Bits 4-7: 6, 12, 24, 48
//   [4] VANISH: Toggles note-vanish likelihood, and bit-rot likelihood.
//                 Bits 0-3 (VANISH):  1, 2, 4, 8 (15 means "absolutely turn latent after next note")
//                 Bits 4-7 (BIT ROT): 1, 2, 4, 8 (15 means "absolutely simulate bit-rot on next note")
//   [5] BLEED:  Toggles horizontal and vertical note-bleed likelihood.
//                 Bits 0-3 (HORIZONTAL): 1, 2, 4, 8 (15 means "absolutely bleed a bit horizontally on next note")
//                 Bits 4-7 (VERTICAL):   1, 2, 4, 8 (15 means "absolutely bleed a bit vertically on next note")
//   [6] LEN:    Toggles lane's loop-length, and note-repeat likelihood.
//                 Bits 0-3 (LOOP LENGTH): 1, 2, 4, 8 (clamped to a maximum of 8)
//                 Bits 4-7 (NOTE REPEAT): 1, 2, 4, 8 (15 means "absolutely repeat the current note")
//   [7] CHAN:   Toggles lane's MIDI channel, and division of main beat-length
//                 Bits 0-3 (MIDI CHAN A):   1, 2, 4, 8
//                 Bits 4-7 (BEAT DIVISION): 2, 3, 4, 8
byte LANEMETA[3][8] = {
  {0, 0, 0, 0, 8, 0, 8, 0},
  {0, 0, 0, 0, 8, 0, 8, 0},
  {0, 0, 0, 0, 8, 0, 8, 0}
};

byte LANETICK[3] = {0, 0, 0};


void updateRowLEDs(byte row) { // TODO: comments that explain what these lines do
  if (row == 6) { // If this is the global beat-tracking row...
    lc.setRow(0, 6, 1 << (7 - byte(floor(CURTICK / 96)))); // Set the row to display the current global beat
  } else if (row == 7) { // Else, if this is the control-button keypress row...
    lc.setRow(0, 7, 1 << (((8 - CMDMODE) % 8) - 1)); // Light up an LED representing the active command-mode, which is controlled by the bottom row of buttons
  } else { // Else, if this is some other row...
    byte rs = row >> 1; // Get a shifted value that yields indices combining 0,1, 2,3, and 4,5
    if (CMDMODE == 0) { // If this is the default command-mode...
      lc.setRow(0, row, DISPLAY[row] | (((row & 1) == 0) ? BLINKVISIBLE[rs] : 0)); // Combine the note-row with the blink-values of latent-notes
    } else { // Else, if this is any other command-mode...
      lc.setRow(0, row, LANEMETA[rs][CMDMODE - 1]); // Display the lanes' meta-values that correspond to various command-modes
    }
  }
}

// Update every LED in the ledControl grid
void updateAllLEDs() {
  for (byte i = 0; i < 8; i++) { // For every LED row...
    updateRowLEDs(i); // Update that row of LEDs
  }
}


void parseKeystroke(KeypadEvent key) {

  KeyState statetype = getState(key); // Get the type of event occurring on the key that has changed state
  byte keynum = byte(key) - 48; // Convert the key's unique ASCII character into a number that reflects its position
  byte kcol = keynum % 8; // Get the key's column
  byte krow = keynum >> 4; // Get the key's row

  if (statetype == PRESSED) { // If the button is pressed...
    if (krow < 3) { // If the button is in one of the three upper rows...
      assignCommandAction(kcol, krow); // Interpret the keystroke under whichever command-mode is active
    } else { // Else, if the button is in the bottom row...
      CMDMODE = keynum - 23; // Set the command-mode to 1 through 8, one ahead of the raw button column (0 is default)
    }
  } else if (statetype == RELEASED) { // Else, if the button is released...
    if (keynum >= 24) { // If the released key was on the bottom row...
      CMDMODE = 0; // Set the display-mode to the main screen
      for (byte i = 31; i >= 24; i--) { // Check whether any other keys on the bottom row are held, from right to left
        if (getState(char(i + 48)) == HOLD) { // If another key on the bottom row is still held...
          CMDMODE = i - 23; // Set that key's corresponding display-mode to be displayed
          break; // ...And exit the loop that checks for held keys on the bottom-row
        }
      }
    }
  }

}

// Interpret a keystroke according to whichever command-mode is active
// 0 = PLAY LATENT NOTE (DEFAULT BEHAVIOR)
// 1 = TOGGLE LANE ACTIVITY (LEFTMOST CONTROL-ROW BUTTON)
// 2 = PITCH DEGRADE AMOUNT
// 3 = VELOCITY DEGRADE AMOUNT
// 4 = DURATION DEGRADE AMOUNT
// 5 = BIT ROT AMOUNT / VANISH LIKELIHOOD
// 6 = HORIZONTAL/VERTICAL BIT BLEED AMOUNT
// 7 = LOOP LENGTH / NOTE REPEAT
// 8 = MIDI CHANNEL / BEAT DIVISION (RIGHTMOST CONTROL-ROW BUTTON)
void assignCommandAction(byte col, byte row) {

  // Note: "row" represents the row of the button pressed, not the row of LEDs it represents

  if (CMDMODE == 0) { // If we are in the default mode...
    playStoredNote(col, row); // Play the note that was last present in the button's corresponding loop-slot
  } else if (CMDMODE == 1) { // If we are in the lane-activity-toggle mode...
    if (col == 0) { // If this is the leftmost column...
      LANEMETA[row][0] ^= 128; // Toggle the lane's activity status on or off (this is effectively a mute)
    } else if (col == 1) { // If this is the second-leftmost column...
      LANEMETA[row][0] ^= 64; // Toggle whether the lane is incremented by global MIDI CLOCK or its channel's MIDI NOTES
    } else if (col == 6) { // Else, if this is the second-rightmost column...
      latentLane(row); // Nullify the lane's note contents, rendering them all latent
    } else if (col == 7) { // Else, if this is the rightmost column...
      clearLane(row); // Clear the lane's note contents, rendering them all empty
    } else { // Else, if this is any of the 6 middle buttons...
      shiftLaneNotes(col, row); // Shift the lane's items by a binary value corresponding to the button's position
    }
  } else if (CMDMODE == 8) { // If we are in the MIDI CHAN / BEAT DIVISION mode...
    if (col <= 3) { // If this is in the left half of the button-grid...
      LANEMETA[row][7] ^= 1 << (7 - col); // Toggle one of the bytes of the lane's MIDI CHANNEL value, as normal
    } else { // Else, if this is in the right half of the button-grid...
      LANEMETA[row][7] = (LANEMETA[row][7] & 240) ^ (1 << (7 - col)); // Toggle between individual bits in the lane's BEAT DIVISION value
    }
  } else { // Else, for all other command-modes...
    LANEMETA[row][CMDMODE - 1] ^= 1 << (7 - col); // Toggle the lane-metadata bit that corresponds to the row and column, under the present command-mode
  }

}

// Play the note that was last present in the button's corresponding loop-slot
void playStoredNote(byte col, byte row) {
  if (LANE[row][col][0] < 255) { // If the note is either active or latent, and not empty...
    playNote(LANE[row][col][0] % 16, LANE[row][col][1], LANE[row][col][2], LANE[row][col][3]); // Play the note, with a corrected MIDI CHANNEL byte
  }
}

// Turn all notes in a given lane into latent-but-present notes
void latentLane(byte row) {
  for (byte i = 0; i < 8; i++) { // For every note-slot in the lane...
    if (LANE[row][i][0] < 255) { // If the note-slot isn't empty...
      LANE[row][i][0] = (LANE[row][i][0] % 16) + 128; // Set the note-slot's channel byte to "latent" state
    }
  }
}

// Clear all notes from a given lane
void clearLane(byte row) {
  for (byte i = 0; i < 8; i++) { // For each note-slot...
    memcpy(LANE[row][i], {255, 255, 0, 0}, 4); // Copy an empty dummy-note into the note-slot
  }
}

// Shift the notes in lane "row" by a "column" number of spaces, in negative or positive binary increments
void shiftLaneNotes(byte col, byte row) {
  byte amt = (col - 4) + ((col + 1) >> 3); // Get a binary value that fans out from the center line and increases with distance
  int dir = (col <= 3) ? -1 : 1; // Get the direction in which to rotate the lane
  byte temp[4]; // Create an array that will hold displaced note-values temporarily
  for (byte i = 0; i < abs(amt); i++) { // For every required rotation of the lane...
    for (byte j = 0; j < 8; j++) { // For every note within the lane...
      byte j2 = (j - dir) % 8; // Get the next-indexed note in the inverse of the direction of rotation, wrapping around the array boundaries
      memcpy(temp, LANE[row][j], 4); // Copy the current note into temporary storage
      memcpy(LANE[row][j], LANE[row][j2], 4); // Copy the next-indexed note into the current note's slot
      memcpy(LANE[row][j2], temp, 4); // Copy the temporarily-stored note into the next-indexed note's slot
    }
  }
}

// Increment the global tick, each lane's local tick (if applicable), and trigger notes if any lanes lapse into their next section
void incrementTicks() {
  CURTICK = (CURTICK + 1) % 768; // Increment the global tick, wrapping it to an 8-beat window
  for (byte i = 0; i <= 3; i++) { // For each lane...
    if (((LANEMETA[i][0] >> 7) % 2) == 1) { // If the lane is set to clock-trigger mode, not note-trigger mode...
      byte beatdiv = max(1, 8 - (LANEMETA[i][7] & 15)); // Get the lane's beat-division value as it's stored - 1, 2, 4, or 8
      beatdiv += min(1, beatdiv & 3); // Modify the beat-division value to reflect the state its stored value represents - 2, 3, 4, or 8
      byte looplen = max(1, 8 - (LANEMETA[i][6] >> 4)); // Get the loop's total length, in beats
      word persect = (96 * looplen) / beatdiv; // Get the number of ticks per note-section
      word ticktotal = persect * looplen; // Get the total number of ticks in the lane
      word nexttick = (LANETICK[i] % persect) + 1; // Get a dummy-value that simulates the next tick's unwrapped position
      LANETICK[i] = (LANETICK + 1) % ticktotal; // Increment the lane's local tick value, wrapping to its local tick-total
      if (nexttick >= persect) { // If the next tick's unwrapped position is greater than the number of ticks per local note-slot...
        byte p = word(floor(LANETICK[i] / persect)); // Get the section that the lane's updated tick now resides in
        if (LANE[i][p][0] < 16) { // If the lane-note within the new section is an active note...
          playNote(LANE[i][p][0], LANE[i][p][1], LANE[i][p][2], LANE[i][p][3]); // Play that note
        }
      }
    }
  }
}

// Reset the global tick-pointer, and all lanes' local tick-pointers
void resetTickPointers() {
  CURTICK = 0; // Reset global tick-pointer
  memcpy(LANETICK, {0, 0, 0}, 3); // Reset all lanes' local tick-pointers
}

// Send NOTE-OFFs for all currently-sustained notes, and remove them from the SUSTAIN array
void haltAllSustains() {
  for (byte i = 0; i < 8; i++) { // For every item in the SUSTAIN array...
    if (SUSTAIN[i][0] < 255) { // If the sustain-slot isn't empty...
      Serial.write(128 + SUSTAIN[i][0]); // Send a corresponding NOTE-OFF's command-byte
      Serial.write(SUSTAIN[i][1]); // Send a corresponding NOTE-OFF's pitch-byte
      Serial.write(127); // Send a corresponding NOTE-OFF's velocity-byte
      memcpy(SUSTAIN[i], {255, 255, 0}, 3); // Empty out the SUSTAIN-slot
    }
  }
}


void playNote(byte chan, byte pitch, byte velo, byte dur) {



}


void parseMidi() {

  // testing code TODO remove
  //lc.setRow(0, 2, CATCHBYTES[0] >> 7);
  //lc.setRow(0, 3, CATCHBYTES[0] % 128);
  lc.setRow(0, TESTCOUNT, CATCHBYTES[1]);
  TESTCOUNT = max(1, (TESTCOUNT % 8) + 1);
  //lc.setRow(0, 7, CATCHBYTES[2]);




}

// Setup function: runs once when the program starts
void setup() {

  Serial.begin(31250);
  
  kpd.setDebounceTime(1); // Set an extremely minimal keypad-debounce time, since we're using clicky buttons

  lc.shutdown(0, false); // Turn on the ledControl object, using an inverse shutdown command (weird!)
  lc.setIntensity(0, 15); // Set the ledControl brightness to maximum intensity

  // Populate the LANE array with dummy-values, which will be treated as empty
  for (byte a = 0; a <= 3; a++) {
    for (byte b = 0; b <= 8; b++) {
      memcpy(LANE[a][b], {255, 255, 0, 0}, 4);
    }
  }

  kpd.addEventListener(parseKeystroke); // Whenever a keypad event occurs, launch the parseKeystroke function
}

// Loop function: runs continuously for as long as the device is powered on
void loop() {

  BLINKELAPSED++; // Increase the blink-time-counting variable
  if (BLINKELAPSED >= BLINKLENGTH) { // If the elapsed blink-time has reached the blink-length limit...
    BLINKELAPSED == 0; // Reset the blink-time-counting variable
    memcpy(BLINKVISIBLE, BLINKVAL, 3); // Copy the lit-LED values from the absolute-blink-value array to the visible-blink array
  } else if (BLINKELAPSED == (BLINKLENGTH >> 1)) { // Else, if the elapsed blink-time has reached the middle of the blink-cycle...
    memcpy(BLINKVISIBLE, {0, 0, 0}, 3); // Place all unlit values into the visible-blink array
  }

  while (Serial.available() > 0) { // While new MIDI bytes are available to read from the MIDI-IN port...

    byte b = Serial.read(); // Get the frontmost incoming byte

    if (SYSIGNORE) { // If we are currently ignoring a SYSEX command...
      Serial.write(b); // Send the current byte to MIDI-OUT immediately, without processing it
      if (b == 247) { // If this was an END SYSEX byte...
        SYSIGNORE = false; // Stop ignoring incoming bytes
      }
    } else { // Else, if we aren't currently ignoring a SYSEX command...
      byte cmd = b - (b % 16); // Get the command-type of any given non-SYSEX command
      if (b == 252) { // If this is a STOP command...
        Serial.write(b); // Send it onward to MIDI-OUT
        PLAYING = false; // Set the PLAYING var to false, to stop acting on any MIDI CLOCKs that might still be received
        haltAllSustains(); // Halt all currently-sustained MIDI NOTES
      } else if (b == 251) { // If this is a CONTINUE command...
        Serial.write(b); // Send it onward to MIDI-OUT
        PLAYING = true; // Set the PLAYING var to true, to begin acting on any incoming MIDI CLOCK commands
      } else if (b == 250) { // If this is a START command...
        Serial.write(b); // Send it onward to MIDI-OUT
        PLAYING = true; // Set the PLAYING var to true, to begin acting on any incoming MIDI CLOCK commands
        resetTickPointers(); // Reset the global tick, and all lanes' local ticks
      } else if (b == 248) { // If this is a TIMING CLOCK command...
        Serial.write(b); // Send it onward to MIDI-OUT
        incrementTicks(); // Increment global and lane-based ticks, and launch related functions when appropriate
      } else if (b == 247) { // If this is an END SYSEX MESSAGE command...
        // If you're dealing with an END-SYSEX command while SYSIGNORE is inactive,
        // then that implies you've received either an incomplete or corrupt command,
        // and so nothing should be done here aside from sending it onward.
        Serial.write(b);
      } else if ( // If the byte represents a...
        (b == 244) // MISC SYSEX command,
        || (b == 240) // or a START SYSEX MESSAGE command,
      ) { // then...
        Serial.write(b); // Send it onward to MIDI-OUT
        SYSIGNORE = true; // Begin ignoring an incoming arbitrarily-sized SYSEX message
      } else if ( // If the byte represents a...
        (b == 242) // SONG POSITION POINTER command,
        || (cmd == 224) // PITCH BEND command,
        || (cmd == 176) // CONTROL CHANGE command,
        || (cmd == 160) // POLY-KEY PRESSURE command,
        || (cmd == 144) // NOTE-ON command,
        || (cmd == 128) // or a NOTE-OFF command...
      ) { // then anticipate an incoming 3-byte message...
        CATCHBYTES[0] = b; // Set the initial caught-byte to the incoming byte
        CATCHCOUNT = 1; // Increment the byte-catching counter to reflect that one byte has been caught
        CATCHTARGET = 3; // Set the catch-target to expect a 3-byte command
      } else if ( // If the byte represents a...
        (b == 243) // SONG SELECT command,
        || (b == 241) // MIDI TIME CODE QUARTER FRAME command,
        || (cmd == 208) // CHANNEL PRESSURE (AFTERTOUCH) command,
        || (cmd == 192) // or a PROGRAM CHANGE command,
      ) { // then anticipate an incoming 2-byte message...
        CATCHBYTES[0] = b; // Set the initial caught-byte to the incoming byte
        CATCHCOUNT = 1; // Increment the byte-catching counter to reflect that one byte has been caught
        CATCHTARGET = 2; // Set the catch-target to expect a 2-byte command
      } else { // Else, if this is a body-byte from a given non-SYSEX MIDI command...
        if (CATCHBYTES[0] == 0) { // If a command-byte wasn't previously received...
          continue; // Ignore the data, and move on to the next byte, if any
        } else { // Else, if a command-byte was previously received...
          CATCHBYTES[CATCHCOUNT] = b; // Store the incoming body-byte
          CATCHCOUNT++; // Increase the index at which the next body-byte would be stored
          if (CATCHCOUNT == CATCHTARGET) { // If all the command's bytes have been received...
            parseMidi(); // Parse the fully-received command
          }
        }
      }
    }

  }

}


