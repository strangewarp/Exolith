

/*

		Steles is a MIDI sequencer firmware for the "Tine" hardware.
		THIS CODE IS UNDER DEVELOPMENT AND DOESN'T DO ANYTHING!
		Copyright (C) 2016-2018, C.D.M. Rørmose (sevenplagues@gmail.com).

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



// Define statements:
//   These values may need to be changed in the course of programming/debugging,
//   but will always stay the same at runtime.
#define FILE_BYTES 196608UL // Number of bytes in each seq-file
#define SCANRATE 7000 // Amount of time between keystroke-scans, in microseconds
#define GESTDECAY 250000UL // Amount of time between gesture-decay ticks, in microseconds



// Program-space (PROGMEM) library
#include <avr/pgmspace.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>



// UI vars
unsigned long BUTTONS = 0; // Tracks which of the 30 buttons are currently pressed; each button has an on/off bit
byte GESTURE[5]; // Tracks currently-active button-gesture events
byte PAGE = 0; // Tracks currently-active page of sequences
byte BLINK = 0; // When an LED-BLINK is active, this will count down to 0
byte TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
unsigned long ELAPSED = 0; // Time elapsed since last tick
word KEYELAPSED = 0; // Time elapsed since last keystroke-scan
unsigned long GESTELAPSED = 0; // Time elapsed since last gesture-decay
unsigned long TICKSIZE = 6250; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Recording vars
byte LOADMODE = 0; // Tracks whether LOAD MODE is active
byte RECORDMODE = 0; // Tracks whether RECORD MODE is active
byte RECORDSEQ = 0; // Sequence currently being recorded into
byte RECORDNOTES = 0; // Tracks whether notes are currently being recorded into a sequence
byte TRACK = 0; // Current track within the active sequence to edit with RECORDMODE actions
byte REPEAT = 0; // Toggles whether held-down note-buttons should repeat a NOTE-ON every QUANTIZE ticks, in RECORD-MODE
byte OCTAVE = 3; // Octave-offset value for RECORD MODE notes
byte VELO = 127; // Baseline velocity-value for RECORD MODE notes
byte HUMANIZE = 0; // Maximum velocity-humanize value for RECORD MODE notes
byte CHAN = 151; // MIDI-COMMAND byte (including current CHANNEL) for RECORD MODE notes
byte QUANTIZE = B00000100; // Time-quantize value for RECORD MODE notes (1 to 16)
byte QRESET = 0; // Tracks how many beats must elapse within RECORDSEQ before the QUANTIZE anchor gets reset (0 = whole sequence)
byte DURATION = 4; // Duration value for RECORD MODE notes, in 16th-notes
byte COPYPOS = 0; // Copy-position within the copy-sequence
byte COPYSEQ = 0; // Sequence from which to copy data

// Sequencing vars
byte SONG = 0; // Current song-slot whose data-files are being played
byte PLAYING = 1; // Controls whether the sequences and global tick-counter are iterating
byte DUMMYTICK = 0; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
byte CLOCKMASTER = 1; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
byte BPM = 80; // Beats-per-minute value: one beat is 96 tempo-ticks
byte TICKCOUNT = 5; // Current global tick, bounded within the size of a 16th-note
byte CUR16 = 127; // Current global sixteenth-note (bounded to 128, or 8 beats)
word GLOBALRAND = 12345; // Global all-purpose semirandom value; gets changed on every tick

// Beat-scattering flags, one per seq.
// bits 0-3: scatter chance
// bits 4-7: scatter distance (0=off; 1,2,4,8 = 8th,4th,half,whole [these can stack with each other])
byte SCATTER[49];

// Cued-command flags, one per seq.
// bit 0: TURN OFF
// bit 1: TURN ON
// bits 2-4: slice 4, 2, 1;
// bits 5-7: cue 4, 2, 1;
byte CMD[49];

// Holds 48 seqs' sizes and activity-flags
// bits 0-5: 1, 2, 4, 8, 16, 32 (= size, in beats) (range limits: 1 to 32)
// bit 6: reserved
// bit 7: on/off flag
byte STATS[49];

// Holds the 48 seqs' internal tick-positions
// bits 0-9: current 16th-note position (0-1023)
// bits 10-15: reserved
word POS[49];

// Holds up to 8 MIDI notes from a given tick,
// (format: MOUT[n*3] = command, pitch, velocity)
// to be sent in a batch at the tick's end
byte MOUT[25];
byte MOUT_BYTES = 0; // Counts the number of bytes used by notes in the MOUT buffer

// Note-Sustain data storage
// (format: SUST[n*3] = channel, pitch, duration)
byte SUST[25];
byte SUST_COUNT = 0; // Counts the current number of sustained notes

// MIDI-IN vars
byte INBYTES[4]; // Buffer for incoming MIDI commands
byte INCOUNT = 0; // Number of MIDI bytes received from current incoming command
byte INTARGET = 0; // Number of expected incoming MIDI bytes
byte SYSIGNORE = 0; // Ignores SYSEX messages when toggled



// Manual extern reference to the writeCommands function,
//     because the Arduino compiler's linking system gets broken by some functions that are called by the CmdFunc construct,
//     which, in practical terms, means that functions in func_cmds.ino that call writeCommands() won't get linked to it,
//     unless writeCommands() is linked manually with an extern command right here.
extern void writeCommands(unsigned long pos, byte amt, byte b[], byte onchan);



// Typedef for a generic function that takes "column" and "row" arguments.
// This will be used to select RECORD-MODE functions based on which keychords are active.
typedef void (*CmdFunc) (byte col, byte row);

extern const CmdFunc COMMANDS[]; // Use the list of RECORD-MODE commands from data_cmds.ino
extern const byte KEYTAB[]; // Use the list of binary-to-decimal function-keys from data_cmds.ino

extern const byte LOGO[]; // Use the logo from data_gui.ino
extern const byte GLYPH_ERASE[]; // Use the ERASE-glyph from data_gui.ino
extern const byte NUMBER_GLYPHS[]; // Use the number-multiglyph from data_gui.ino



const char PREFS_FILENAME[] PROGMEM = {80, 46, 68, 65, 84, 0}; // Filename of the preferences-file, in PROGMEM to save RAM



SdFat sd; // Initialize SdFat object
SdFile file; // Initialize an SdFile File object, to control default data read/write processes



void setup() {

	// Ensure that the global arrays don't contain junk-data
	memset(SCATTER, 0, 49);
	memset(CMD, 0, 49);
	memset(STATS, 0, 49);
	memset(POS, 0, 49);
	memset(MOUT, 0, 25);
	memset(SUST, 0, 25);
	memset(INBYTES, 0, 4);

	// Set all the keypad's row-pins to INPUT_PULLUP mode, and all its column-pins to OUTPUT mode
	DDRC = 0;
	PORTC = 255;
	DDRD |= B00011100;
	DDRB |= B00000011;

	// Initialize the SD-card at full speed, or throw a visible error message if no SD-card is inserted
	if (!sd.begin(10, SPI_FULL_SPEED)) {
		PORTD &= B10111111; // Set the MAX chip's CS pin low (data latch)
		sendRow(2, B11101110);
		sendRow(3, B10001001);
		sendRow(4, B11101001);
		sendRow(5, B00101001);
		sendRow(6, B00101010);
		sendRow(7, B11101100);
		PORTD |= B01000000; // Set the MAX chip's CS pin high (data latch)
		sd.initErrorHalt();
	}

	createFiles(); // Check whether the savefiles exist, and if they don't, then create them

	loadPrefs(); // Load whatever prefsarein PRF.DAT, or create PRF.DAT if it doesn't exist yet

	loadSong(SONG); // Load whatever song-slot was in PRF.DAT, or the default song-slot if PRF.DAT didn't exist

	Serial.begin(31250); // Start serial comms at the MIDI baud rate
	
	ABSOLUTETIME = micros(); // Make sure ABSOLUTETIME matches the current time, so tempo doesn't have an initial jerk

}


void loop() {

	parseRawMidi(); // Parse incoming MIDI

	updateTimer(); // Update the global timer

	updateGUI(); // Update the GUI, if applicable

	updateGlobalRand(); // Update the global semirandom value

}

