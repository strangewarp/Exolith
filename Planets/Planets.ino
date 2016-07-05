
/*

		Planets is a physics-simulating MIDI delay for the "Snekbox" hardware.
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

// Versioning vars
PROGMEM char FIRMWAREVERSION[17] = "BS_PLANETS_00001"; // Program name, plus version number, to be matched with the EEPROM contents

// GUI vars
byte ROWUPDATE = 0; // Track which rows of LEDs to update on a given iteration of the main loop. 1 = row 0; 2 = row 1; 4 = row 2; ... 128 = row 7
word PANELTIMER = 0; // Controls whether the MIDI-CHANNEL panel is displayed in the left half of the LED-grid: 0 = no; >0 = yes
byte BINARYLEDS[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Stores each LED-row's full-on and full-off LED values

// Sequencing vars
boolean PLAYING = false; // Controls whether the gravity system responds to MIDI CLOCK signals
byte CHANS[4] = {0, 0, 0, 0}; // Holds every MIDI channel: 0 in all = OMNI

// MIDI-CATCH vars
byte CATCHBYTES[3] = {0, 0, 0};
byte CATCHCOUNT = 0;
byte CATCHTARGET = 0;
boolean SYSIGNORE = false;

// MIDI-OUT SUSTAIN vars
// Format: SUSTAIN[n] = {channel, pitch, remaining duration in ticks}
// Note: "255" in byte n[0] means "empty slot"
byte SUSTAIN[8][3];

// MIDI-IN SUSTAIN vars
// Format: INSUSTAIN[n] = {channel, pitch, velocity, elapsed duration in ticks, target planet}
// Note: "255" in byte n[0] means "empty slot"
// Note: these are not cleared in haltAllSustains, as proper behavior of upstream devices is assumed
byte INSUSTAIN[8][5];

// PLANET ENTITY byte-vars
// Format: PLANETBYTE[n] = {channel, pitch, velocoity, remaining duration}
// Note: "255" in byte n[0] means "empty slot"
byte PLANETBYTE[16][4];

// PLANET ENTITY float-vars
// Format: PLANETFLOAT[n] = {mass, x position, y position}
//	 -- byte 0: mass: 0.01 to 1.0: describes how much weight the object has
//	 -- byte 1: x position: 0.0 to 1.0: planet's x-position on the virtual canvas
//	 -- byte 2: y position: 0.0 to 1.0: as above, for y
//   -- byte 3: x acceleration: -1.0 to 1.0: planet's x-acceleration rate per tick
//   -- byte 4: y acceleration: -1.0 to 1.0: as above, for y
// Note: "-3.0" in float n[0] means "empty slot"
float PLANETFLOAT[16][5];

// ATTRACTOR ENTITY byte-vars
// These hold the presence-values of attractors, which correspond to the right 4x4 group of buttons, and affect every planet's gravity
// Format: ATTRACTORS[n] = B00001111 (position values: bits 0-3 are unused; bits 4-7 are 0 for empty, or 1 for filled)
byte ATTRACTORS[4] = {0, 0, 0, 0};


// Update every ledControl LED-row flagged for an update
void updateLEDs() {
	for (byte i = 0; i < 8; i++) { // For every LED row...
		byte bval = 1 << i; // Get the row's corresponding bitwise value
		if ((ROWUPDATE & bval) > 0) { // If the row is flagged for a GUI update...
			lc.setRow(0, i, BINARYLEDS[i]); // Update the ledControl row that corresponds to the given row's BINARYLEDS illumination-value
			ROWUPDATE ^= bval; // Unset this row's ROWUPDATE flag
		}
	}
}

// Increment the timer that's used for tracking overlay-panel visibility
void incrementTimer() {
	unsigned long mill = millis(); // Get the current millisecond-timer value
	word diff = 0; // Variable that will hold the difference between the current time and the previous time
	if (mill < ABSOLUTETIME) { // If the millis-value has wrapped around its finite counting-space to be less than last loop's absolute-time position...
		diff = (4294967295 - ABSOLUTETIME) + mill; // Put the wrapped-around milliseconds into the elapsed-blink-time value
	} else if (mill > ABSOLUTETIME) { // Else, if the millis-value is greater than last loop's absolute-time position...
		diff = mill - ABSOLUTETIME; // Add the difference between the current time and the previous time to the elapsed-blink-time value
	}
	ABSOLUTETIME = mill; // Set the absolute-time to the current time-value
	if (PANELTIMER > 0) { // If the panel-view timer is active, then apply the timing difference to it...
		if ((PANELTIMER - diff) <= 0) { // If the panel-view timer has fewer milliseconds remaining than are contained in the current time-difference...
			PANELTIMER = 0; // Set the panel-view timer to 0 (off)
			ROWUPDATE |= 15; // Flag the MIDI-CHANNEL-panel rows for a GUI update
		} else { // Else, if the panel-view timer has more milliseconds than are contained in the current time-difference...
			PANELTIMER -= diff; // Subtract the time-difference from the panel-view timer
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
		PANELTIMER = 500; // Make the MIDI-CHANNEL panel visible for 500 milliseconds
		CHANS[row] |= 1 << (3 - col); // Toggle the bit in the row's channel-value flag that corresponds to the column's bitwise value
		EEPROM.write(16 + row, CHANS[row]); // Write the new channel value to EEPROM memory, for storage across shutdowns
		ROWUPDATE |= 15; // Flag the MIDI-CHANNEL-panel rows for a GUI update
	} else { // Else, if the keystroke was in the right half of the button-grid...
		PANELTIMER = 0; // Make the MIDI-CHANNEL panel invisible
		ATTRACTORS[row] |= 1 << col; // Activate an ATTRACTOR object in the physics system, whose position corresponds to the button pressed
	}
}

// Interpret a key-release according to whatever command-mode is active
void unassignCommandAction(byte col, byte row) {
	if (col <= 3) { // If this key-release was in the left half of the button-grid...
		PANELTIMER = 250; // Set the panel-view timer to 250
	} else { // Else, if this key-release was in the right half of the button-grid...
		ATTRACTORS[row] ^= 1 << col; // Remove an ATTRACTOR object in the physics system, whose position corresponds to the button pressed
	}
}

// Update the physics system by a single frame
void updatePhysics() {

	for (byte i = 0; i < 16; i++) {

		if (PLANETBYTE[i][0] == 255) {
			break;
		}

		byte currow = byte(round(PLANETFLOAT[i][2] / 8));
		byte curcol = byte(round(PLANETFLOAT[i][1] / 8));
		byte nextrow = currow;
		byte nextcol = curcol;

		float pull[16][2]; // Will contain each ATTRACTOR's combined x,y gravity values for the current PLANET
		byte pullcount = 0; // Will contain the number of active ATTRACTORS, to be used for later averaging-division of summed x,y gravity values
		float pulltotal[2]; // Will contain the composite x,y gravity value to modulate the current PLANET

		// Get the composite ATTRACTOR modifiers for the current planet's position
		for (byte y = 0; y < 4; y++) {
			for (byte x = 0; x < 4; x++) {
				byte pindex = x + (y * 4); // Get an index for the pull-array that corresponds to this ATTRACTOR slot's x,y coordinates
				if ((ATTRACTORS[y] & (1 << x)) > 0) { // If this x,y position's ATTRACTOR slot is filled...
					float attx = 8 / ((2 * x) + 1); // Get the attractor's x,y positions, expressed in a 0.0-to-1.0 range
					float atty = 8 / ((2 * y) + 1);
					float distx = PLANETFLOAT[i][1] - attx; // Get the planet's x,y distances from the attractor, in -1.0 to 1.0
					float disty = PLANETFLOAT[i][2] - atty;

					float maxdist = max(abs(distx), abs(disty));

					float accelx = (distx / maxdist) * PLANETFLOAT[i][0] * PLANETFLOAT[i][3];
					float accely = (disty / maxdist) * PLANETFLOAT[i][0] * PLANETFLOAT[i][4];

					PLANETFLOAT[i][3] = min(1, max(-1, PLANETFLOAT[i][3] - accelx));
					PLANETFLOAT[i][4] = min(1, max(-1, PLANETFLOAT[i][4] - accely));


					//pull[pindex][0] = 
					//pull[pindex][1] = 

					pullcount++;
				} else {
					pull[pindex][0] = 0.0;
					pull[pindex][1] = 0.0;
				}
			}
		}

		//for (byte)



		//PLANETFLOAT[i][1] =
		//PLANETFLOAT[i][2] =

		//ROWUPDATE |= 

	}
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
			for (byte j = 16; j < 20; j++) { // For every MIDI-channel storage byte in the EEPROM...
				EEPROM.write(j, 0); // Write default MIDI-channel values
			}
			CHANS[0] = 0; // Set all MIDI-CHANNEL slots to the default value of 0
			CHANS[1] = 0;
			CHANS[2] = 0;
			CHANS[3] = 0;
			match = false; // Since the EEPROM and the current firmware-name didn't match, set the match-flag to false
			break; // As a match has been disproven, there is no need to check any further, so break from the outer for loop
		}
	}
	if (match) { // If the program-name in EEPROM was found to match the current firmware-name...
		CHANS[0] = EEPROM.read(16); // Set all MIDI-CHANNEL slots to their EEPROM-stored values
		CHANS[1] = EEPROM.read(17);
		CHANS[2] = EEPROM.read(18);
		CHANS[3] = EEPROM.read(19);
	}

	// Tune the Keypad settings
	kpd.setDebounceTime(1); // Set an extremely minimal keypad-debounce time, since we're using clicky buttons
	kpd.setHoldTime(0); // Set no gap between a keypress' PRESS and HOLD states

	// Initialize the ledControl system
	lc.shutdown(0, false); // Turn on the ledControl object, using an inverse shutdown command (weird!)
	lc.setIntensity(0, 15); // Set the ledControl brightness to maximum intensity

	// Populate various global arrays
	for (byte i = 0; i < 8; i++) { // For every global array with 8 slots...
		SUSTAIN[i][0] = 255; // Fill the SUSTAIN array with empty-slot-signifying values
		SUSTAIN[i][1] = 0;
		SUSTAIN[i][2] = 0;
		INSUSTAIN[i][0] = 255; // Fill the INSUSTAIN array with empty-slot-signifying values
		INSUSTAIN[i][1] = 0;
		INSUSTAIN[i][2] = 0;
		INSUSTAIN[i][3] = 0;
		INSUSTAIN[i][4] = 0;
	}
	for (byte i = 0; i < 16; i++) { // For every global array with 16 slots...
		PLANETBYTE[i][0] = 255; // Fill the PLANETBYTE array with empty-slot-signifying values
		PLANETBYTE[i][1] = 0;
		PLANETBYTE[i][2] = 0;
		PLANETBYTE[i][3] = 0;
		PLANETFLOAT[i][0] = -3.0; // Fill the PLANETFLOAT array with empty-slot-signifying values
		PLANETFLOAT[i][1] = 0.0;
		PLANETFLOAT[i][2] = 0.0;
		PLANETFLOAT[i][3] = 0.0;
		PLANETFLOAT[i][4] = 0.0;
	}

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
				incrementInSustains(); // Increment all currently-externally-sustained notes
				incrementSustains(); // Increment all currently-internally-sustained notes
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
						CATCHBYTES[0] = 0; // Empty out the byte-catching array
						CATCHBYTES[1] = 0;
						CATCHBYTES[2] = 0;
						CATCHCOUNT = 0; // Empty out the byte-counting value
						CATCHTARGET = 0; // Empty out the target-bytes-to-catch value
					}
				}
			}
		}

	}

	updateLEDs(); // Update whatever rows of LEDs have been flagged for GUI-updates

}
