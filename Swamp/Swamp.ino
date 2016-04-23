


/*

    Swamp is a MIDI modulation program for the ATMEGA328P.
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


// EEPROM data-storage library
#include <EEPROM.h>

// Digital-pin keypad library
#include <Key.h>
#include <Keypad.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Initialize the object that controls the MAX7221's LED-grid
LedControl lc = LedControl(26, 27, 28, 1);

// Initialize the object that controls the Keypad buttons.
const byte ROWS PROGMEM = 4;
const byte COLS PROGMEM = 8;
char KEYS[ROWS][COLS] = {
  {'0', '1', '2', '3', '4', '5', '6', '7'},
  {'8', '9', ':', ';', '<', '=', '>', '?'},
  {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G'},
  {'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'}
};
byte rowpins[ROWS] = {34, 35, 36, 37};
byte colpins[COLS] = {40, 41, 42, 43, 44, 45, 46, 47};
Keypad kpd(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS);


// Active lane, 0-7
byte activelane = 0;

// Each lane's assigned channel (0-15 = 1-16)
byte lanechan[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Tracks whether a given lane sends duplicate notes from its assigned channel directly to THRU
boolean lanethru[8] = {false, false, false, false, false, false, false, false};

// Each lane's assigned duration (binary: 1 2 4 8 = 6 12 24 48; 0 means "1")
byte lanedur[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// All notes move one space for every n CLOCK ticks (binary: 1 2 4 8 = 6 12 24 48; 0 means "only move when forced")
byte moveticks = 0;

// Holds all 8 boolean dam states
boolean[8] dam = {false, false, false, false, false, false, false, false};

// Holds all notes currently in the swamp, with duration-values from the time of their creation.
// Format: swamp[row][column] = {chan, pitch, velocity, duration}
byte swamp[8][8][4];

// Holds up to 8 active sustains per MIDI channel.
// Format: sustain[chan][slot] = {pitch, ticks-remaining}
// Also: {255, n} means "empty"
byte sustain[16][8][2];


// If an incoming note is a swamp-channel note command, throw it into the swamp;
// otherwise, send it to regular soft-thru.
void swampCheck(byte[4] note) {
	byte chan = note[0] % 16;
	byte cmd = note[0] - chan;
	if (cmd == 144) {
		for (byte i = 0; i <= 7; i++) {
			if (lanechan[i] == chan) {
        if (lanethru[i]) { noteSend(note); } // If the lane is set to duplicate-THRU, then send a copy of the note to output
				swampIn(i, chan, note[1], note[2]); // Throw the note into the swamp
			}
		}
	} else {
		noteSend(note);
	}
}

// Throw a note into the swamp, shifting other notes out of the way if necessary.
void swampIn(byte lane, byte chan, byte pitch, byte velo) {
	if (swamp[lane][0][0] < 17) {
		swampShift(lane, 0);
	}
	swamp[lane][0] = {chan, pitch, velo, max(1, 6 * lanedur[lane])};
}

// Shift a swamp lane at position "pos", and if any notes reach the sinkhole, send them.
void swampShift(byte lane, byte pos) {

	// If this swamp slot is empty, do nothing
	if (swamp[lane][pos][0] >= 17) { return; }

	// Either move to the right (66% chance), or down (33% chance), bounded by the swamp's borders.
	// If a given random move is invalid, just move toward the sinkhole.
	byte dir = random(3) % 2;
	if (pos == 7) { dir = 0; }
	if (lane == 7) { dir = 1; }
	byte newpos = pos;
	byte newlane = lane;
	if (dir == 0) {
    newpos++;
  } else {
		newlane++;
	}

  // If there's a dam, or the note is in the rightmost column...
  if ((pos == 7) || dam[newlane]) { // (keep the clauses in this order; must check for position before checking dam[newlane])

    // Check whether all rows within this column are full
    boolean upfull = true;
    boolean downfull = true;
    if (lane > 0) {
      for (int i = 0; i <= (lane - 1); i++) {
        if (swamp[i][pos][0] >= 17) {
          upfull = false;
          break;
        }
      }
    }
    if (lane < 7) {
      for (int i = (lane + 1); i <= 7; i++) {
        if (swamp[i][pos][0] >= 17) {
          downfull = false;
          break;
        }
      }
    }

    // If there are any empty rows in this column, move toward that slot; else, spill over the dam
    if (!downfull) {
      newpos = pos;
      newlane = lane + 1;
    } else if (!upfull) {
      newpos = pos;
      newlane = lane - 1;
    } else {
      newpos = pos + 1;
      newlane = lane;
    }

  }

  // If the note's new position is beyond the rightmost column, send it to MIDI-OUT
	if (newpos > 7) {
    byte[4] note = swamp[lane][pos];
    note[0] += 144;
		noteSend(note);
    addSustain(swamp[lane][pos]);
	} else { // Else, if the note is in a valid column...
    // If a note is already in the new position, shift that note ahead
    if (swamp[newlane][newpos][0] < 17) {
		  swampShift(newlane, newpos);
    }
    // Move the current note into the new position
    swamp[newlane][newpos] = swamp[lane][pos];
	}

  // Set the now-unused position to hold a "position unused" dummy-note
	swamp[lane][pos] = {255, 0, 0};

}

// Send a note, or other MIDI command, via the MIDI-OUT port
void noteSend(byte[3] note) {

  // If any of the MIDI values are invalid, do nothing
  if ((note[0] <= 127) || (note[1] >= 128) || (note[2] >= 128)) {
    return;
  }

  // Send two bytes for two-byte commands, or three bytes for three-byte commands
  if (note[0] <= 239) {
    Serial.print(note[0]);
    Serial.print(note[1]);
    if ((note[0] <= 191) || (note[0] >= 224)) {
      Serial.print(note[2]);
    }
  }

}

// Add a sustain to the internal sustain-tracking system
void addSustain(byte[4] note) {

  byte chan = note[0];
  byte pitch = note[1];
  byte velo = note[2];
  byte dur = note[3];

  boolean match = false;

  // Look for a matching pitch in the channel's sustain array
  for (int i = 0; i <= 7; i++) {
    if (sustain[chan][i][0] == pitch) { // If the pitch is already present...
      match = true; // Flag that the pitch was matched
      sustain[chan][i] = {255, 0}; // Empty the pitch's slot
      noteSend({128 + chan, pitch, 127}); // Send a note-off for the pitch's old sustain, which has to be cut short
      // If this wasn't the bottommost sustain-slot, then move all lower filled sustains upwards
      for (int j = i; j <= 6; j++) {
        if (sustain[chan][j][0] == 255) { break 2; }
        sustain[chan][j] = sustain[chan][j + 1];
      }
      break;
    } else if (sustain[chan][i][0] == 255) { // If this slot is empty, then all slots below it are empty too, so stop searching
      break;
    }
  }

  // If no matching pitch was found in the active sustains...
  if (!match) {
    // If this channel's bottommost sustain-slot is filled, send its corresponding noteoff and empty it
    if (sustain[chan][7][0] <= 15) {
      noteSend({chan + 128, sustain[chan][7][0], 127});
      sustain[chan][7] = {255, 0};
    }
    // Move all of this channel's sustain-slots one position downwards
    for (int i = 6; i >= 0; i--) {
      sustain[chan][i + 1] = sustain[chan][i];
    }
    // Put the pitch in the channel's top sustain-slot
    sustain[chan][0] = {pitch, dur};
  }

  noteSend({144 + chan, pitch, velo}); // Send a note-on for the pitch's new sustain

}

// Reduce the sustain-times on all currently-active sustains, and send note-offs for every sustain that expires
void iterateSustains() {

  for (int i = 0; i <= 15; i++) {
    for (int j = 0; j <= 7; j++) {
      // If this sustain-slot is empty, break from the channel's sustain-checking loop
      if (sustain[i][j][0] == 255) {
        break;
      } else { // Else, if this sustain-slot is full...
        sustain[i][j][1]--; // Reduce the sustain's remaining duration by 1
        if (sustain[i][j][1] <= 0) { // If the sustain has no remaining duration...
          noteSend(128 + i, sustain[i][j][0], 127); // Send the note-off
          // If this wasn't the channel's bottommost sustain-slot, move all lower sustain-slots one position upwards
          for (int k = j; k <= 6; k++) {
            sustain[i][k] = sustain[i][k + 1];
          }
          // Empty the bottommost slot, which now contains either the outdated note or a duplicate entry
          sustain[i][7] = {255, 0};
        }
      }
    }
  }

}


void setup() {

  Serial.begin(31250);
  
  kpd.setDebounceTime(1);

  lc.shutdown(0, false);
  lc.setIntensity(0, 15);

  // Fill swamp array with "this slot is unused" values
  for (int i = 0; i <= 7; i++) {
    for (int j = 0; j <= 7; j++) {
      swamp[i][j] = {255, 0, 0, 0};
    }
  }

  // Fill sustain-array with "this slot is unused" values
  for (int i = 0; i <= 15; i++) {
  	for (int j = 0; j <= 7; j++) {
  		sustain[i][j] = {255, 255};
  	}
  }

}

void loop() {

  // Catch all keypad events
  if (kpd.getKeys()) {
    for (int i = 0; i < LIST_MAX; i++) {
      if (kpd.key[i].stateChanged) {
        if (kpd.key[i].kstate == PRESSED) {

        	// TODO: put keychord-addition and keycatching here

        } else if (kpd.key[i].kstate == RELEASED) {

        	// TODO: put keychord-removal here

        }
      }
    }
  }

  // Catch incoming MIDI events


}

