
// Digital-pin keypad library
#include <Key.h>
#include <Keypad.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// K-species 8: 9 interesting scales
const word SCALE[10] PROGMEM = {
	B0000110110110110, // (consonance 20.2)
	B0000101101110110, // (consonance 15.2)
	B0000111011011100, // (consonance 14.9)
	B0000110111011100, // (consonance 14.6)
	B0000111011101100, // (consonance 14.6)
	B0000101110110110, // (consonance 14.2)
	B0000101101101110, // (consonance 14.2)
	B0000101011101110, // (consonance 13.6)
	B0000101110101110 // (consonance 12.9)
	//{0, 1, 3, 4, 6, 7, 9, 10},
	//{0, 2, 3, 5, 6, 7, 9, 10},
	//{0, 1, 2, 4, 5, 7, 8, 9},
	//{0, 1, 3, 4, 5, 7, 8, 9},
	//{0, 1, 2, 4, 5, 6, 8, 9},
	//{0, 2, 3, 4, 6, 7, 9, 10},
	//{0, 2, 3, 5, 6, 8, 9, 10},
	//{0, 2, 4, 5, 6, 8, 9, 10},
	//{0, 2, 3, 4, 6, 8, 9, 10}
};

// Commands and args that correspond to each button's position
const byte CMD_COORDS[7][6][3] PROGMEM = {
	{{0, 1}, {1, 11}, {1, 0}, {1, 1}, {0, 3}},
	{{1, 10}, {2, 7}, {2, 0}, {2, 1}, {1, 2}},
	{{1, 9}, {2, 6}, {4, 0}, {2, 2}, {1, 3}},
	{{1, 8}, {2, 5}, {2, 4}, {2, 3}, {1, 4}},
	{{0, 0}, {1, 7}, {1, 6}, {1, 5}, {0, 2}},
	{{3, 0}, {3, 1}, {3, 2}, {3, 3}, {3, 4}}
};

// Directions of pitch-vectors in n=12 space, plotted onto the smallest possible granular grid
const char VECTOR[13][3] PROGMEM = {
	{0, -2}, {1, -2}, {2, -1},
	{2, 0}, {2, 1}, {1, 2},
	{0, 2}, {-1, 2}, {-2, 1},
	{-2, 0}, {-2, -1}, {-1, -2}
};

// MIDI-IN vars
byte INBYTES[4] = {0, 0, 0};
byte INCOUNT = 0;
byte INTARGET = 0;
boolean SYSIGNORE = false;

// Note-sustain variables
byte SUSTAIN[9][5]; // Holds all currently-sustained user notes
byte SUSEMPTY[5] PROGMEM = {255, 255, 255, 255}; // Dummy-byte for easy memcpy operations when clearing sustain-entries

// Direction the glyph is moving in:
// 0 = stick-to-smallest-interval;
// 1 = repelled-by-smallest-interval;
// 2 = stationary;
// 3 = random
byte DIRECTION = 2;

word SCALEBIN = B0000110110110110; // Binary representation of the current rotation of the currently-active scale
byte CURSCALE = 0; // Currently-active 8-note scale
byte ROTSCALE = 0; // Current rotation of the active 8-note scale
byte BASENOTE = 0; // Current base-note
byte CHAN = 0; // Currently-active MIDI channel
int OCTAVE = 2; // Current octave
byte VELO = 127; // Current default velocity
byte HUMANIZE = 24; // Current humanization of velocity values

// Channels to store tonal data from
unsigned int CHAN_LISTEN = B0000000010001000;

// Time-ordered incoming and glyph-based pitch-bytes, for use in GUI-drawing & predictive harmony
byte LATEST_IN[17];
byte GLYPH[17];

// Position of the current point, in glyph-space
int XPOS = 0;
int YPOS = 0;

// Initialize the object that controls the Keypad buttons
const byte ROWS PROGMEM = 6;
const byte COLS PROGMEM = 5;
char KEYS[ROWS][COLS] = {
	{'0', '1', '2', '3', '4'},
	{'5', '6', '7', '8', '9'},
	{':', ';', '<', '=', '>'},
	{'?', '@', 'A', 'B', 'C'},
	{'D', 'E', 'F', 'G', 'H'},
	{'I', 'J', 'K', 'L', 'M'}
};
byte rowpins[ROWS] = {5, 6, 7, 8, 18, 19};
byte colpins[COLS] = {9, 14, 15, 16, 17};
Keypad kpd(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS);

// Initialize the object that controls the MAX7219's LED-grid
LedControl lc = LedControl(2, 3, 4, 1);


word updateBinScale() {
	SCALEBIN = ((SCALE[CURSCALE] >> ROTSCALE) | ((SCALE[CURSCALE] << (12 - ROTSCALE)) & 4096);
}


// Add a new note to the sustain-tracking array
void addSustain(byte[4] note) {
	if (SUSTAIN[7][0] != 255) { // If the last sustain-tracking slot isn't empty...
		removeSustain(7); // Remove that sustain
	}
	for (byte i = 7; i >= 1; i--) { // For every sustain-slot except the topmost one...
		memcpy(SUSTAIN[i], SUSTAIN[i - 1], 4); // Copy the immediately-above sustain-slot's contents downward into the current slot
	}
	memcpy(SUSTAIN[i], note, 4); // Put the sustain-note into the now-cleared topmost sustain-slot
}

// Iterate each sustain's duration-tracking byte by a single tempo-tick
void iterateSustains() {
	for (byte i = 7; i >= 0; i--) { // For each sustain-slot, in reverse order...
		if (SUSTAIN[i][3] == 0) { // If the sustain has run to the end of its duration...
			removeSustain(i); // Remove the sustain from the current slot
		} else { // Else, if the sustain is still running...
			SUSTAIN[i][3]--; // Decrease the sustain's delay-byte by 1 tempo-tick
		}
	}
}

// Remove a currently-sustained note from the sustain-tracking array
void removeSustain(byte loc) {
	memcpy(SUSTAIN[loc], SUSEMPTY, 4); // Put an empty dummy-byte into the note's sustain-slot
	Serial.write(128 + SUSTAIN[loc]][0]); // Send a NOTE-OFF command for the note, on its given MIDI channel
	Serial.write(SUSTAIN[loc][1]); // ^
	Serial.write(127); // ^
	for (int i = loc; i < 7; i++) { // For every sustain-slot underneath the removed sustain's slot...
		memcpy(SUSTAIN[i], SUSTAIN[i + 1], 4); // Copy the sustain-slot upwards by one slot
	}
}

// Add a node to the glyph, based on a given note
void addGlyphNode(byte n) {
	if (GLYPH[15] != 255) { // If the oldest glyph-node isn't empty...
		XPOS -= VECTOR[GLYPH[15]][0]; // Remove the oldest glyph-node's x-y vector distances from the current position
		YPOS -= VECTOR[GLYPH[15]][1]; // ^
	}
	for (int i = 15; i >= 1; i--) { // For every node entry, except for the topmost one...
		GLYPH[i] = GLYPH[i - 1]; // Copy the next-most-recent node into the current node-slot
	}
	GLYPH[0] = n; // Put the given note into the topmost glyph-node slot
	XPOS += VECTOR[n][0]; // Add the newest node's x-y vector distances to the current position
	YPOS += VECTOR[n][1]; // ^
}

// Process a chromatic note, and send it to MIDI-OUT
void playChromaticNote(byte n) {
	n += BASENOTE; // Add the base pitch-offset to the note's button-related value
	addGlyphNode(n % 12); // Add the note's pitch-offset value to the glyph system, bounded to an octave
	n += 12 * OCTAVE; // Add the current octave's worth of pitch-shifting to the note-value
	while (n > 127) { // While the note is beyond the acceptable MIDI NOTE pitch limit...
		n -= 12; // Remove an octave from the note-value
	}
	int hum = random(HUMANIZE + 1); // Get the amount by which the note's velocity should be humanized
	hum -= byte(round(hum / 2)); // Offset the humanize-value, to give it equal chance of becoming quieter or louder
	byte note[5] = { // Construct an outgoing MIDI note:
		CHAN, // MIDI channel (currently-selected MIDI channel for playing)
		n, // Processed note-value
		max(1, min(127, VELO + hum)), // Velocity, plus humanize-factor, clamped to a 1-127 range
		DURATION // Global note-duration value
	};
	addSustain(note); // Add a sustain for the outgoing note-value
	Serial.write(note[0]); // Send the note to MIDI-OUT
	Serial.write(note[1]); // ^
	Serial.write(note[2]); // ^
}

// Play whatever note leads to the most efficient step toward the glyph's origin
void stepToOrigin() {

	word best = 65535; // Holds the position-value of the best step for getting back to origin
	byte sel = 0; // Holds the index of the best step

	// Check whether any of the note-vectors' absolute positions reach the origin outright. If so, play that note.
	// Otherwise, find the note-vector that brings the current position closest to origin, and play that note instead.
	for (byte i = 0; i < 12; i++) { // For each chromatic note...
		if (!(SCALEBIN & (1 << (11 - i)))) { continue; } // If the note is inactive in the current scale, then go to the next iteration
		int xi = XPOS + VECTOR[i][0]; // Get a note's x-y coordinates, based on the current position
		int yi = YPOS + VECTOR[i][1]; // ^
		byte test = abs(xi) + abs(yi); // Get the note-position's absolute distance from origin
		if (test < best) { // If the new distance-from-origin is smaller than the current-best-one...
			best = test; // Set the best distance to the new distance
			sel = i; // Set the selected-key to that distance's note-position
			if (!best) { break; } // If the best distance is 0, then the search is over, so break the loop
		}
		for (byte j = 0; j < 12; j++) { // For each chromatic note (in addition to the chromatic note already in var "i")...
			if ( // If...
				(SCALEBIN & (1 << (11 - j))) // The note is active in the current scale...
				&& (!((xi + VECTOR[j][0]) || (yi + VECTOR[j][1]))) // And both notes' offsets, combined, lead to origin...
			) { // Then...
				sel = i; // Set the selected-key to the first of the two note-positions
				break 2; // The ideal key has been found, so break from both search-loops
			}
		}
	}

	playChromaticNote(sel); // Play the note that tacks closest to the origin-point

}

// Change the spin-type for octave transitions
void changeSpinType(byte kind) {



}


void ctrlCmd(byte c) {

}

void parseKeystrokes() {

}

// Parse a given MIDI command
void parseMidiCommand() {
	if (
		(CHAN_LISTEN & (1 << (INBYTES[0] % 16))) // If the command's channel is among the listened-to channels...
		&& ((INBYTES[0] & 240) == 144) // And this command is a NOTE-ON...
	) { // Then...
		for (int i = 15; i > 0; i--) { // Shift all LATEST_IN note entries downward by 1 slot
			LATEST_IN[i] = LATEST_IN[i - 1];
		}
		LATEST_IN[0] = INBYTES[1] % 12; // Put the note-pitch's chromatic position into the LATEST_IN array
	}
	for (byte i = 0; i < INTARGET; i++) { // Having parsed the command, send its bytes onward to MIDI-OUT
		Serial.write(INBYTES[i]);
	}
}

// Parse all incoming raw MIDI bytes
void parseRawMidi() {

	// While new MIDI bytes are available to read from the MIDI-IN port...
	while (Serial.available() > 0) {

		byte b = Serial.read(); // Get the frontmost incoming byte

		if (SYSIGNORE) { // If this is an ignoreable SYSEX command...
			if (b == 247) { // If this was an END SYSEX byte, clear SYSIGNORE and stop ignoring new bytes
				SYSIGNORE = false;
			}
		} else { // Else, accumulate arbitrary commands...

			if (INCOUNT >= 1) { // If a command's first byte has already been received...
				INBYTES[INCOUNT] = b;
				INCOUNT++;
				if (INCOUNT == INTARGET) { // If all the command's bytes have been received...
					parseMidiCommand(); // Parse the command
					INBYTES[0] = 0; // Reset incoming-command-tracking vars
					INBYTES[1] = 0;
					INBYTES[2] = 0;
					INCOUNT = 0;
				}
			} else { // Else, if this is either a single-byte command, or a multi-byte command's first byte...

				byte cmd = b - (b % 16); // Get the command-type of any given non-SYSEX command

				if ( // If this is a TIMING CLOCK, START, CONTINUE, or STOP command...
					(b == 248) || (b == 250)
					|| (b == 251) || (b == 252)
				) { // Then send the byte onward to MIDI-OUT
					Serial.write(b);
				} else if (b == 247) { // END SYSEX MESSAGE command
					// If you're dealing with an END-SYSEX command while SYSIGNORE is inactive,
					// then that implies you've received either an incomplete or corrupt SYSEX message,
					// and so nothing should be done here.
				} else if (
					(b == 244) // MISC SYSEX command
					|| (b == 240) // START SYSEX MESSAGE command
				) { // Anticipate an incoming SYSEX message
					SYSIGNORE = true;
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
				}

			}
		}

	}

}


void updateGUI() {

}

// Convert a keystroke to its corresponding command and variable
void keyToCmd(byte x, byte y) {

	byte cmd = CMD_COORDS[y][x][0]; // Get the keystroke's command
	byte arg = CMD_COORDS[y][x][1]; // Get the keystroke's arg

	if (CTRL_CMD && (cmd != 3)) { // If a control-command is being pressed, and the current keystroke isn't a control-button...
		parseCtrlSubCmd(x, y); // Parse the control-mode's subcommand
		return; // End the function, as the remainder of its contents are irrelevant here
	}

	if (cmd == 0) { // If this is a SPIN command...
		changeSpinType(arg); // Change the spin type in the given way
	} else if (cmd == 1) { // If this is a CHROMATIC NOTE command...
		playChromaticNote(arg); // Play the given note
	} else if (cmd == 2) { // If this is a SCALE NOTE command...
		playScaleNote(arg); // Play the given scale-note
	} else if (cmd == 3) { // If this is a CONTROL command...
		parseCtrlCmd(arg); // Parse that control-key
	} else { // Else, if this is the ORIGIN button...
		stepToOrigin(); // Take a step toward the glyph's origin
	}

}


void setup() {

    // Set an extremely minimal debounce time, since the device uses clicky buttons
	kpd.setDebounceTime(1);

    // Power up ledControl to full brightness
	lc.shutdown(0, false);
	lc.setIntensity(0, 15);

	// Fill arrays with dummy-values
	for (byte i = 0; i <= 7; i++) {
		memcpy(SUSTAIN[i], SUSEMPTY, 4);
	}
	for (byte i = 0; i <= 15; i++) {
		GLYPH[i] = 255;
		LATEST_IN[i] = 255;
	}

    // Start serial comms at the MIDI baud rate
	Serial.begin(31250);
	
}


void loop() {

    parseRawMidi();

    parseKeystrokes();

    updateGUI();

}



