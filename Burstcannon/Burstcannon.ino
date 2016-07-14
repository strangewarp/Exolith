
/*

		Burstcannon is a MIDI burst-generator for the "Snekbox" hardware.
		THIS CODE IS UNDER DEVELOPMENT AND DOESN'T DO ANYTHING!
		Copyright (C) 2016-onward, Christian D. Madsen (sevenplagues@gmail.com).

		This program is free software; you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation; either version 2 of the License, or
		(at your option) any later version.

		This program is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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

// EEPROM data-storage library
#include <EEPROM.h>

// Initialize the object that controls the MAX7221's LED-grid
LedControl lc = LedControl(2, 3, 4, 1);

// Initialize the object that controls the Keypad buttons.
const byte ROWS PROGMEM = 5;
const byte COLS PROGMEM = 9;
char KEYS[ROWS][COLS] = {
	{'0', '1', '2', '3', '4', '5', '6', '7'},
	{'8', '9', ':', ';', '<', '=', '>', '?'},
	{'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G'},
	{'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'}
};
byte rowpins[ROWS] = {5, 6, 7, 8};
byte colpins[COLS] = {9, 10, 14, 15, 16, 17, 18, 19};
Keypad kpd(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS);

// Versioning vars
const char FIRMWAREVERSION[17] PROGMEM = "BS_BURSTCN_00001"; // Program name, plus version number, to be matched with the EEPROM contents

// GUI vars
byte ROWUPDATE = 0; // Track which rows of LEDs to update on a given iteration of the main loop. 1 = row 0; 2 = row 1; 4 = row 2; ... 128 = row 7
boolean BLINKING = false; // Track whether the rows that light up during a burst are lit

// Burst vars
byte CHAN = 0; // Channel on which burst-notes are sent
byte PITCH = 40; // Base-pitch for burst-notes
byte VELOCITY = 0; // Base burst-note velocity
byte DENSITYINTERVAL[3] = {0, 0}; // Stores flags for density-interval intersection points that correspond to the right 4x4 button section

// Sequencing vars
boolean PLAYING = false; // Controls whether the MIDI bursts must adhere to external MIDI CLOCK signals
byte CURTICK = 1; // Current global tick

// MIDI-CATCH vars
boolean SYSIGNORE = false; // Flags whether an incoming SYSEX command is currently being ignored

// MIDI-OUT SUSTAIN vars
// Format: SUSTAIN = {channel, pitch, remaining duration in ticks}
// Note: "255" in byte 0 means "empty"
byte SUSTAIN[4] = {255, 0, 0};


// Update every ledControl LED-row that's flagged for an update
void updateLEDs() {
	randomSeed(i + CHAN + PITCH + VELOCITY); // Set a random-seed based on current internal values, in case of blink-row activity
	for (byte i = 0; i < 8; i++) { // For every LED row...
		byte bval = 1 << i; // Get the row's corresponding bitwise value
		if ((ROWUPDATE & bval) > 0) { // If the row is flagged for a GUI update...
			if (i == 0) { // If this is the first row...
				lc.setRow(0, i, (CHAN << 4) | (PLAYING ? 15 : 0)); // Display the MIDI CHANNEL value, and whether a MIDI CLOCK is currently being followed
			} else if (i == 1) { // If this is the second row...
				lc.setRow(0, i, PITCH); // Display the pitch-value
			} else if (i == 2) { // If this is the third row...
				lc.setRow(0, i, VELOCITY); // Display the velocity-value
			} else if (i == 7) { // If this is the eighth row...
				lc.setRow(0, i, 1 << (7 - byte(floor(CURTICK / 12)))); // Display the position in the current beat
			} else { // Else, if this is a blink-display row...
				lc.setRow(0, i, (BLINKTIMER > 0) ? ((~CURTICK) & random(127, 255)) : 0); // If a blink is active, display a pseudorandom value generated from internal variables
			}
			ROWUPDATE ^= bval; // Unset this row's ROWUPDATE flag
		}
	}
}

// Parse any incoming keystrokes in the Keypad grid
void parseKeystrokes() {
	if (kpd.getKeys()) { // If any keys are pressed...
		for (byte i = 0; i < 10; i++) { // For every keypress slot...
			if (kpd.key[i].stateChanged) { // If the key's state has just changed...
				byte keynum = byte(kpd.key[i].kchar) - 48; // Convert the key's unique ASCII character into a number that reflects its position
				byte kcol = keynum & 7; // Get the key's column
				byte krow = keynum >> 3; // Get the key's row
				if (kpd.key[i].kstate == PRESSED) { // If the button is pressed...
					assignCommandAction(kcol, krow); // Interpret the keystroke
				} else if (kpd.key[i].kstate == RELEASED) { // Else, if the button is released...
					unassignCommandAction(kcol, krow); // Interpret the key-release
				}
			}
		}
	}
}

// Interpret an incoming keystroke, using a given button's row and column
void assignCommandAction(byte col, byte row) {
	if (col <= 3) { // If this keystroke was in the left half of the button-grid...
		if (row == 0) { // If this keystroke was on the first row...
			CHAN ^= 1 << (3 - col); // Toggle the bit in the channel-value that corresponds to the column's bitwise value
			EEPROM.write(16, CHAN); // Write the new channel value to EEPROM memory, for storage across shutdowns
		} else if (row == 3) { // Else, if this keystroke was on the fourth row...
      VELOCITY = ((VELOCITY & 7) > 0) ? 0 : VELOCITY; // If the velocity is at a special value of 127 or 1, set it to 0 in preparation for a left-shifted bit
			VELOCITY ^= 8 << (3 - col); // Toggle the bit in the velocity-value that corresponds to the column's bitwise value, left-shifted by 3 to (64-32-16-8)
      VELOCITY |= (VELOCITY == 120) ? 7 : ((VELOCITY == 0) ? 1 : 0); // Set the velocity to 127 or 1 if it is filled or emptied
		} else { // Else, if this keystroke was in one of the middle rows...
			if ((row == 1) && (col == 0)) { // If this was the first button of the second row...
				PITCH = max(0, min(127, PITCH + (((random(0, 1) * 2) - 1) * random(2, 5)))); // Change the global base-pitch by a random likely-to-be-consonant interval
			} else { // Else, if this was any other button...
				PITCH ^= max(1, (2 - row) << 4) << (3 - col); // Modify the PITCH value by a binary value corresponding to a button in the middle two rows
			}
		}
		ROWUPDATE |= 1 << row; // Flag the button-row's corresponding LED-row for a GUI update
	} else { // Else, if the keystroke was in the right half of the button-grid...
		DENSITYINTERVAL[row >> 1] |= 1 << ((col & 3) | ((row & 1) << 2)); // Flag the button's corresponding density-interval coordinates
		ROWUPDATE |= B01110000; // Flag the burst-activity LED-rows for a GUI update
	}
}

// Interpret a key-release according to whatever command-mode is active
void unassignCommandAction(byte col, byte row) {
	if (col >= 4) { // If this key-release was in the right half of the button-grid...
    DENSITYINTERVAL[row >> 1] ^= 1 << ((col & 3) | ((row & 1) << 2)); // Unflag the button's corresponding density-interval coordinates
    ROWUPDATE |= B01110000; // Flag the burst-activity LED-rows for a GUI update
	}
}

// Play a note based on currently-active burst commands, if they correspond to the current global MIDI CLOCK tick
void playBurst() {

  byte mod[9]; // Density-values, arranged by their corresponding interval
  byte locs[9]; // List of interval-locations in the "mod" array with any current amount of burst-density
  byte pressed = 0; // Number of interval-types, represented by buttons, that are currently being held down

  for (byte i = 0; i < 8; i++) { // For every interval entry in the DENSITYINTERVAL array...
    byte ibin = 1 << i; // Get the bitwise position of this entry
    mod[i] = min(1, DENSITYINTERVAL[0] & ibin) << 2) * (1 - min(1, CURTICK % 6)); // If 1/6-density key is pressed, and the global tick matches, store the value
    mod[i] |= (min(1, DENSITYINTERVAL[1] & ibin) << 1) * (1 - min(1, CURTICK % 3)); // If 1/3-density key is pressed, and the global tick matches, store the value
    if (mod[i] == 6) { // If both the 1/6-density and 1/3-density keys are pressed for a given interval...
      mod[i] = 1; // Set the interval's density to 1, aka "every tick"
    }
    if (mod[i] > 0) { // If the current interval has any density from button-presses...
      locs[pressed] = i; // Store this interval in the "locations with any amount of density" array
      pressed++; // Increase the "pressed" value to track the number of interval-types that are pressed
    }
  }

  byte combo = random(1, min(2, pressed)); // Number of intervals to apply to global PITCH for this burst press: either 1 or 2, based on keys currently held
  byte outpitch = PITCH; // The output-pitch, to be modified by intervals in the user's keystrokes
  byte interval = 0; // The current interval-type
  byte lastused = 255; // The previous interval-type, for checking other interval-types against
  byte applied = 0; // Number of intervals applied to the outgoing pitch
  while (applied < combo) { // While fewer buttons have been applied than the number required for the interval-combination...
    do { // Continually get an interval...
      interval = mod[locs[random(1, pressed)]]; // ...From a random position among user-pressed interval types...
    } while (interval == lastused); // ...Until that interval doesn't match the previous interval
    lastused = interval; // This interval will be treated as the "previous" interval, on the next loop
    outpitch = min(127, max(0, outpitch + interval)); // Apply the interval to the out-pitch, bounded to the minimum and maximum MIDI pitch values
    applied++; // Increase the "applied" value to track the number of intervals that have been applied to the outgoing pitch
  }

  sendNoteOn(CHAN, outpitch, VELOCITY, 1); // Send a note on the currently-active MIDI channel, with the outgoing pitch

  ROWUPDATE |= B01110000; // Flag the burst-activity LED-rows for a GUI update

}

// Increment the internal sustain-value
void incrementSustain() {
  if (SUSTAIN[0] == 255) { return; } // If the sustain is empty, do nothing
  if (SUSTAIN[2] == 0) { // If the sustain's duration is over...
    sendNoteOff(SUSTAIN[0], SUSTAIN[1]); // Call the note-off function, which will also clear the current sustain's contents
  } else { // Else, if the sustain's duration is still ongoing...
    SUSTAIN[2]--; // Decrease the sustain's remaining duration by 1 tick
  }
}

// Add a sustain-note to the internal sustain-tracking system
void addSustain(byte chan, byte pitch, byte dur) {
  if (SUSTAIN[0] <= 15) { // If the sustain-system is already tracking a note...
    sendNoteOff(SUSTAIN[0], SUSTAIN[1]); // Call the note-off function, which will also clear the current sustain's contents
  }
  SUSTAIN[0] = chan; // Put the given note-data into the sustain-note array
  SUSTAIN[1] = pitch;
  SUSTAIN[2] = dur;
}

// Remove the current sustain value, if it corresponds to the given channel and pitch values
void removeSustain(byte chan, byte pitch) {
  if (SUSTAIN[0] == 255) { return; } // If the sustain is empty, do nothing
  if ((SUSTAIN[0] == chan) && (SUSTAIN[1] == pitch)) { // If the currently-active sustain matches the given note values...
    SUSTAIN[0] = 255; // Empty out the sustain-note array
    SUSTAIN[1] = 0;
    SUSTAIN[2] = 0;
  }
}

// Play a given MIDI NOTE-ON, start tracking its sustain status, and send its data to the MIDI-OUT TX line
void sendNoteOn(byte chan, byte pitch, byte velo, byte dur) {
  addSustain(chan, pitch, dur); // Add a sustain for the note
  sendMidi(144 + chan, pitch, velo); // Send the NOTE-ON command onward to the TX MIDI-OUT line
}

// Play a given NOTE-OFF, remove its corresponding sustain-command (if applicable), and send its data to the MIDI-OUT TX line
void sendNoteOff(byte chan, byte pitch) {
  removeSustain(chan, pitch); // Remove the note's corresponding sustain, if one exists
  sendMidi(128 + chan, pitch, 127); // Send the NOTE-OFF command onward to the TX MIDI-OUT line
}

// Send a 3-byte MIDI command on the TX MIDI-OUT line
void sendMidi(byte b1, byte b2, byte b3) {
  Serial.write(b1); // Send the command-byte
  Serial.write(b2); // Send the pitch-byte / second data-byte
  Serial.write(b3); // Send the velocity-byte / third data-byte
}

// Setup function: called exactly once on launch, after the initial global variable declarations but before the main loop.
void setup() {

	// Check EEPROM for a firmware match, and either overwrite it or set local variables to stored variables
	boolean match = true; // Set the match-flag to true by default
	for (byte i = 15; i >= 0; i--) { // For every byte of the program's name in EEPROM...
		char nchar = char(EEPROM.read(i)); // Get that byte, translated into a char
		if (nchar != FIRMWAREVERSION[i]) { // If the char from EEPROM doesn't match the corresponding char in the current firmware-name...
			for (byte j = 0; j < 16; j++) { // For the 16-char-wide program-name space in the EEPROM...
				EEPROM.write(j, FIRMWAREVERSION[j]); // Fill it with the program's name
			}
			EEPROM.write(16, 0); // Write default MIDI-channel value
			CHAN = 0; // Set MIDI-CHANNEL to the default value of 0
			match = false; // Since the EEPROM and the current firmware-name didn't match, set the match-flag to false
			break; // As a match has been disproven, there is no need to check any further, so break from the outer for loop
		}
	}
	if (match) { // If the program-name in EEPROM was found to match the current firmware-name...
		CHAN = EEPROM.read(16); // Set MIDI-CHANNEL to its EEPROM-stored value
	}

	// Tune the Keypad settings
	kpd.setDebounceTime(1); // Set an extremely minimal keypad-debounce time, since we're using clicky buttons
	kpd.setHoldTime(0); // Set no gap between a keypress' PRESS and HOLD states

	// Initialize the ledControl system
	lc.shutdown(0, false); // Turn on the ledControl object, using an inverse shutdown command (weird!)
	lc.setIntensity(0, 15); // Set the ledControl brightness to maximum intensity

	// Start sending/receiving serial data
	Serial.begin(31250); // Run the sketch at a bitrate of 31250, which is what MIDI-over-TX/RX requires
	
}

// Loop function: called continuously, starting immediately after the only call to the setup() function.
void loop() {

	parseKeystrokes(); // Check for any incoming keystrokes

	// Parse all incoming MIDI bytes
	while (Serial.available() > 0) { // While new MIDI bytes are available to read from the MIDI-IN port...

		byte b = Serial.read(); // Get the frontmost incoming byte

		if (SYSIGNORE) { // If we are currently ignoring a SYSEX command...
			Serial.write(b); // Send the current byte to MIDI-OUT immediately, without processing it
			if (b == 247) { // If this was an END SYSEX byte...
				SYSIGNORE = false; // Stop ignoring incoming bytes
			}
		} else { // Else, if we aren't currently ignoring a SYSEX command...
			byte cmd = b & 240; // Get the command-type of any given non-SYSEX command
			if (b == 252) { // If this is a STOP command...
				Serial.write(b); // Send it onward to MIDI-OUT
				PLAYING = false; // Set the PLAYING var to false, to stop acting on any MIDI CLOCKs that might still be received
				haltSustain(); // Halt the currently-sustained MIDI NOTE
			} else if (b == 251) { // If this is a CONTINUE command...
				Serial.write(b); // Send it onward to MIDI-OUT
				PLAYING = true; // Set the PLAYING var to true, to begin acting on any incoming MIDI CLOCK commands
			} else if (b == 250) { // If this is a START command...
				Serial.write(b); // Send it onward to MIDI-OUT
				PLAYING = true; // Set the PLAYING var to true, to begin acting on any incoming MIDI CLOCK commands
        CURTICK = 0; // Set the current tick to the start-point
			} else if (b == 248) { // If this is a TIMING CLOCK command...
				Serial.write(b); // Send it onward to MIDI-OUT
        CURTICK = (CURTICK + 1) % 96; // Increment the global tick-counter, bounded by the number of ticks in a beat
        incrementSustain(); // Increment the currently-sustained note
        playBurst(); // Play notes based on whatever burst-keys are being held
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
      } else { // Else, if this is any other byte...
        Serial.write(b); // Send it onward to MIDI-OUT
      }

	}

	updateLEDs(); // Update whatever rows of LEDs have been flagged for GUI-updates

}
