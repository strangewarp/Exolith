

/*

		Steles is a MIDI sequencer firmware for the "Tine" hardware.
		THIS CODE IS UNDER DEVELOPMENT AND DOESN'T DO ANYTHING!
		Copyright (C) 2016-2019, C.D.M. Rørmose (sevenplagues@gmail.com).

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


/////////////////////////////
// Start define statements //
/////////////////////////////

// These values may need to be changed in the course of programming/debugging,
// but will always stay the same at runtime.

#define FILE_BYTES 393411UL // Number of bytes in each savefile

// Location of bytes in a given savefile's header-block:
#define FILE_BPM_BYTE 0 // BPM byte
#define FILE_CHAIN_START 3 // Start-byte of the seq-chain-directions block
#define FILE_CHAIN_END 50 // ^ End-byte
#define FILE_SQS_START 51 // Start-byte of the seq-size-values block
#define FILE_SQS_END 98 // ^ End-byte
#define FILE_ONLOAD_START 99 // Start-byte of the SEND ON LOAD block
#define FILE_ONLOAD_END 146 // ^ End-byte
#define FILE_ONEXIT_START 147 // Start-byte of the SEND ON EXIT block
#define FILE_ONEXIT_END 194 // ^ End-byte

#define FILE_BODY_START 147UL // Start of the file's body-block (UL because large values will be added to this)

#define FILE_SEQ_BYTES 8192UL // Bytes within each sequence (UL because large values will be added to this)

#define BPM_LIMIT_LOW 32 // Limits to the range of valid BPM values
#define BPM_LIMIT_HIGH 255 // ^

#define UPPER_BITS_LOW 112 // Limits to the range of valid UPPER COMMAND BITS values
#define UPPER_BITS_HIGH 224 // ^

#define PREFS_ITEMS_1 17 // Number of items in the PREFS-file (1-indexed)
#define PREFS_ITEMS_2 18 // ^ (1-indexed + 1, for buffer assembly)

#define DEFAULT_BPM 130 // Default BPM-value, for cases where a coherent BPM value is not available

#define GRID_TOTAL 5 // Number of GRIDCONFIG grids within the GRIDS[] array (this value is 0-indexed!!!)

#define SCANRATE 7000 // Amount of time between keystroke-scans, in microseconds

///////////////////////////
// End define statements //
///////////////////////////



// Program-space (PROGMEM) library
#include <avr/pgmspace.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>



// UI vars
unsigned long BUTTONS = 0; // Tracks which of the 30 buttons are currently pressed; each button has an on/off bit
byte PAGE = 0; // Tracks currently-active page of sequences
word BLINKL = 0; // When filled, this will count down to 0, illuminating the left side of the LEDs
word BLINKR = 0; // ^ This, but right side
byte GLYPHL[4]; // Holds a TRACK-linked MIDI-command that will be assigned a glyph once the GUI is processed
byte GLYPHR[4]; // ^ This, but right side
int LOADHOLD = 0; // Track how long to hold a just-loaded savefile's file-number onscreen
byte TO_UPDATE = 255; // Tracks which rows of LEDs should be updated at the end of a given tick (initially set to 255, to make sure the entire screen is refreshed)

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
float ELAPSED = 0.0; // Time elapsed since last tick (must be a float for accuracy, since float-based TICKSIZE values will be being subtracted from it)
unsigned long KEYELAPSED = 0; // Time elapsed since last keystroke-scan
float TICKSIZE = 31250.0; // Current tick-size, in microseconds

// Mode flag vars
byte RECORDMODE = 0; // Tracks whether RECORD MODE is active
byte RECORDSEQ = 0; // Sequence currently being recorded into
byte RECORDNOTES = 0; // Tracks whether notes are currently being recorded into a sequence
byte TRACK = 0; // Current track within the active sequence to edit with RECORDMODE actions
byte REPEAT = 0; // Toggles whether held-down note-buttons should repeat a NOTE-ON every QUANTIZE ticks, in RECORD-MODE
byte AUTOCURSOR = 0; // Tracks which AUTOCMD position is currently active, for [insertion/deletion] of AUTO ON LOAD or AUTO ON EXIT commands

// Arpeggiation vars
byte ARPMODE = 0; // Arp system's current mode: 0 = up, 1 = down, 2 = repeating random
byte ARPPOS = 0; // Holds the most-recent bit in the BUTTONS array that the arpeggiator has acted upon (0 = none)
byte ARPLATCH = 0; // Tracks whether any ARP-notes have played yet within the current cluster of keystrokes
byte ARPREFRESH = 0; // Controls how RPTVELO gets refreshed (0 = only on note-keystrokes where ARPLATCH is false; 1 = on every new note-keystroke)

// Recording vars
byte GRIDCONFIG = 0; // Current rotation of the note-key grid in RECORD-MODE (0 = chromatic row-based, 1 = chromatic column-based)
byte RECPRESS = 0; // Tracks whether a note is currently being pressed in RECORD-MODE
byte RECNOTE = 0; // Tracks the latest pressed-note within RECORD-MODE
byte RPTSWEEP = 128; // Amount to modify RPTVELO by on each held-REPEAT-tick (0-127 = minus, 128-255 = plus)
byte RPTVELO = 127; // Stored RPTSWEEP-modified VELOCITY value for the current REPEAT step (refreshed on every new REPEAT-note press)
byte OCTAVE = 4; // Octave-offset value for RECORD MODE notes
byte VELO = 127; // Baseline velocity-value for RECORD MODE notes
byte HUMANIZE = 0; // Maximum velocity-humanize value for RECORD MODE notes
byte DURHUMANIZE = 0; // Maximum duration-humanize value for RECORD MODE notes
byte CHAN = 144; // MIDI-COMMAND byte (including current CHANNEL) for RECORD MODE notes
char OFFSET = 0; // The number of 32nd-notes by which the base QUANTIZE-point gets offset in RECORD-MODE (-31 to 31)
byte QUANTIZE = 4; // Time-quantize value for RECORD MODE notes (1 to 16)
byte QRESET = 0; // Tracks how many beats must elapse within RECORDSEQ before the QUANTIZE anchor gets reset (0 = whole sequence)
byte DURATION = 1; // Duration value for RECORD MODE notes, in 32nd-notes (0 to 128; and 129 = "manually-held durations mode")

// Manual-note-recording vars
byte KEYFLAG = 0; // Flags whether a note is currently being recorded in manual-DURATION-mode
word KEYPOS = 0; // Holds the QUANTIZE-adjusted insert-point for the current recording-note
byte KEYNOTE = 0; // Holds the recording-note's pitch-value
byte KEYVELO = 0; // Holds the recording-note's velocity-value
byte KEYCOUNT = 0; // Holds the number of ticks for which a recording-note has been held

// Sequencing vars
byte SONG = 0; // Current song-slot whose data-files are being played
byte BPM = DEFAULT_BPM; // Beats-per-minute value: one beat is 96 tempo-ticks
byte TICKCOUNT = 2; // Current global tick, bounded within the size of a 32nd-note (3 ticks, 0-indexed) (must be set to 2 initially, to lapse into the first note)
byte CUR32 = 255; // Current global 32nd-note (bounded to 256, or 8 whole-notes)
word GLOBALRAND = 12345; // Global all-purpose semirandom value; gets changed on every tick

// Pattern-chain vars, one per seq.
// These indicate that another adjacent seq should be activated, and the current seq deactivated, when this seq ends.
// If multiple bits are present, then one of the seq-positions they represent will be chosen randomly.
// bit 0: "up-left"
// bit 1: "up"
// bit 2: "up-right"
// bit 3: "left"
// bit 4: "right"
// bit 5: "down-left"
// bit 6: "down"
// bit 7: "down-right"
byte CHAIN[49];

// Beat-scattering flags, one per seq.
// bits 0-3: scatter chance
// bits 4-7: scatter distance (0=off; 1,2,4 = 8th,4th,half [these can stack with each other])
byte SCATTER[49];

// Cued-command flags, one per seq.
// bit 0: TURN OFF
// bit 1: TURN ON
// bits 2-4: slice 4, 2, 1
// bits 5-7: cue 4, 2, 1
byte CMD[49];

// Holds 48 seqs' sizes and activity-flags
// bits 0-4: SEQ SIZE: 0 to 31 (+ 1 = seq size in whole-notes)
// bit 5: JUST CHAINED: this seq was just the target of a CHAIN operation on the current tick.
// bit 6: ACTIVE ON LOAD: this seq will start playing when the savefile is loaded.
// bit 7: ON/OFF: this seq is either playing or not playing.
byte STATS[49];

// Holds the 48 seqs' internal tick-positions
// bits 0-9: current 32nd-note position (0-1023)
// bits 10-15: reserved
word POS[49];

// Holds up to 8 MIDI notes from a given tick,
// (format: MOUT[n*3] = command, pitch, velocity)
// to be sent in a batch at the tick's end
byte MOUT[25];
byte MOUT_COUNT = 0; // Counts the number of outgoing MIDI-commands

// Note-Sustain data storage
// (format: SUST[n*3] = channel, pitch, duration)
byte SUST[25];
byte SUST_COUNT = 0; // Counts the current number of sustained notes

// MIDI-IN vars
byte INBYTES[4]; // Buffer for incoming MIDI commands
byte INCOUNT = 0; // Number of MIDI bytes received from current incoming command
byte INTARGET = 0; // Number of expected incoming MIDI bytes
byte SYSIGNORE = 0; // Ignores SYSEX messages when toggled



// Typedef for a generic function that takes "column" and "row" arguments.
// This will be used to select RECORD-MODE functions based on which keychords are active.
typedef void (*CmdFunc) (byte col, byte row);



SdFat sd; // Initialize SdFat object
SdFile file; // Initialize an SdFile File object, to control default data read/write processes



void setup() {

	// Delay the startup process, in case Tine is connected to a bunch of power-draining MIDI splitters or something,
	//   which could cause a brownout on powerup, and garble SD comms, data-loading, memset calls, etc.
	delay(1000);

	// Ensure that the global arrays don't contain junk-data
	memset(CHAIN, 0, 49);
	memset(SCATTER, 0, 49);
	memset(CMD, 0, 49);
	memset(STATS, 0, 49);
	memset(POS, 0, 98);
	memset(MOUT, 0, 25);
	memset(SUST, 0, 25);
	memset(INBYTES, 0, 4);
	memset(GLYPHL, 0, 4);
	memset(GLYPHR, 0, 4);

	// Set all the keypad's row-pins to INPUT_PULLUP mode, and all its column-pins to OUTPUT mode:
	// B bits 0, 1 = OUTPUT
	// C bits 0, 1, 2 = OUTPUT
	// C bits 3, 4, 5 = INPUT_PULLUP
	// D bits 2, 3, 4 = INPUT_PULLUP
	DDRB |= B00000011; // Set B(0, 1) as outputs
	DDRC = B00000111; // Set C(0, 1, 2) as outputs, and C(3, 4, 5) as inputs. C(6, 7) are unused
	DDRD &= B11100011; // Set D(2, 3, 4) as inputs
	PORTC |= B00111000; // Set pullup resistors for input-bits C(3, 4, 5)
	PORTD |= B00011100; // Set pullup resistors for input-bits D(2, 3, 4)

	maxInitialize(); // Initialize the MAX72** chip's LED system

	// Initialize the SD-card at full speed, or throw a visible error message if no SD-card is inserted
	if (!sd.begin(10, SPI_FULL_SPEED)) {
		sendRow(2, B11101110);
		sendRow(3, B10001001);
		sendRow(4, B11101001);
		sendRow(5, B00101001);
		sendRow(6, B00101010);
		sendRow(7, B11101100);
		sd.initErrorHalt();
	}

	Serial.begin(31250); // Start serial comms at the MIDI baud rate

	midiPanic(); // Send NOTE-OFFs to every MIDI note on every MIDI channel

	createFiles(); // Check whether the savefiles exist, and if they don't, then create them

	loadPrefs(); // Load whatever prefsarein P.DAT, or create PRF.DAT if it doesn't exist yet

	loadSong(SONG); // Load whatever song-slot was in P.DAT, or the default song-slot if P.DAT didn't exist
	// loadSong() also performs an updateTickSize(), since it's loading the song's BPM as well,
	// so updateTickSize() doesn't need to be explicitly called on startup

	sendClockReset(); // Send a MIDI CLOCK reset command to MIDI-OUT

	ABSOLUTETIME = micros(); // Make sure ABSOLUTETIME matches the current time, so tempo doesn't have an initial jerk

}


void loop() {

	parseRawMidi(); // Parse incoming MIDI

	updateTimer(); // Update the global timer

	updateGUI(); // Update the GUI, if applicable

	xorShift(); // Update the global semirandom value

}
