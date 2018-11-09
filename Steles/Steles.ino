

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


/////////////////////////////
// Start define statements //
/////////////////////////////

// These values may need to be changed in the course of programming/debugging,
// but will always stay the same at runtime.

#define FILE_BYTES 196659UL // Number of bytes in each savefile

// Location of bytes in a given savefile's header-block:
#define FILE_BPM_BYTE 0 // BPM byte
#define FILE_SGRAN_BYTE 1 // SWING GRANULARITY byte
#define FILE_SAMOUNT_BYTE 2 // SWING AMOUNT byte
#define FILE_SQS_START 3 // Start-byte of the seq-size-values block
#define FILE_SQS_END 50 // End-byte of the seq-size-values block

#define FILE_BODY_START 51UL // Start of the file's body-block (UL because large values get added to this)

#define FILE_SEQ_BYTES 4096UL // Bytes within each sequence (UL because large values get added to this)

#define BPM_LIMIT_LOW 60 // Limits to the range of valid BPM values
#define BPM_LIMIT_HIGH 255 // ^

#define UPPER_BITS_LOW 96 // Limits to the range of valid UPPER COMMAND BITS values
#define UPPER_BITS_HIGH 224 // ^

#define PREFS_ITEMS_1 17 // Number of items in the PREFS-file (1-indexed)
#define PREFS_ITEMS_2 18 // ^ (1-indexed + 1, for buffer assembly)

#define DEFAULT_BPM 130 // Default BPM-value, for cases where a coherent BPM value is not available

#define GRID_TOTAL 5 // Number of GRIDCONFIG grids within the GRIDS[] array (this value is 0-indexed!!!)

#define SCANRATE 7000 // Amount of time between keystroke-scans, in microseconds

#define GESTDECAY 250000UL // Amount of time between gesture-decay ticks, in microseconds

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
byte GESTURE[5]; // Tracks currently-active button-gesture events
byte PAGE = 0; // Tracks currently-active page of sequences
byte BLINKL = 0; // When filled, this will count down to 0, illuminating the left side of the LEDs
byte BLINKR = 0; // ^ This, but right side
int LOADHOLD = 0; // Track how long to hold a just-loaded savefile's file-number onscreen
byte TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
unsigned long ELAPSED = 0; // Time elapsed since last tick
word KEYELAPSED = 0; // Time elapsed since last keystroke-scan
unsigned long GESTELAPSED = 0; // Time elapsed since last gesture-decay
word TICKSZ1 = 19231; // Current size of pre-SWING-modified ticks
word TICKSZ2 = 19231; // Current size of post-SWING-modified ticks

// Mode flag vars
byte LOADMODE = 0; // Tracks whether LOAD MODE is active
byte RECORDMODE = 0; // Tracks whether RECORD MODE is active
byte RECORDSEQ = 0; // Sequence currently being recorded into
byte RECORDNOTES = 0; // Tracks whether notes are currently being recorded into a sequence
byte TRACK = 0; // Current track within the active sequence to edit with RECORDMODE actions
byte REPEAT = 0; // Toggles whether held-down note-buttons should repeat a NOTE-ON every QUANTIZE ticks, in RECORD-MODE

// Arpeggiation vars
byte ARPMODE = 0; // Arp system's current mode: 0 = up, 1 = down, 2 = repeating random
byte ARPPOS = 0; // Holds the most-recent bit in the BUTTONS array that the arpeggiator has acted upon (0 = none)
byte ARPLATCH = 0; // Tracks whether any ARP-notes have played yet within the current cluster of keystrokes
byte ARPREFRESH = 0; // Controls how RPTVELO gets refreshed (0 = only on note-keystrokes where ARPLATCH is false; 1 = on every new note-keystroke)

// Recording vars
byte GRIDCONFIG = 0; // Current rotation of the note-key grid in RECORD-MODE (0 = chromatic row-based, 1 = chromatic column-based)
byte RPTSWEEP = 128; // Amount to modify RPTVELO by on each held-REPEAT-tick (0-127 = minus, 128-255 = plus)
byte RPTVELO = 127; // Stored RPTSWEEP-modified VELOCITY value for the current REPEAT step (refreshed on every new REPEAT-note press)
byte OCTAVE = 4; // Octave-offset value for RECORD MODE notes
byte VELO = 127; // Baseline velocity-value for RECORD MODE notes
byte HUMANIZE = 0; // Maximum velocity-humanize value for RECORD MODE notes
byte CHAN = 144; // MIDI-COMMAND byte (including current CHANNEL) for RECORD MODE notes
byte QUANTIZE = 4; // Time-quantize value for RECORD MODE notes (1 to 16)
byte QRESET = 0; // Tracks how many beats must elapse within RECORDSEQ before the QUANTIZE anchor gets reset (0 = whole sequence)
byte DURATION = 1; // Duration value for RECORD MODE notes, in 32nd-notes (0 to 128; and 129 = "manually-held durations mode")
byte COPYPOS = 0; // Copy-position within the copy-sequence
byte COPYSEQ = 0; // Sequence from which to copy data

// Manual-note-recording vars
byte KEYFLAG = 0; // Flags whether a note is currently being recorded in manual-DURATION-mode
word KEYPOS = 0; // Holds the QUANTIZE-adjusted insert-point for the current recording-note
byte KEYNOTE = 0; // Holds the recording-note's pitch-value
byte KEYVELO = 0; // Holds the recording-note's velocity-value
byte KEYCOUNT = 0; // Holds the number of ticks for which a recording-note has been held

// Sequencing vars
byte SONG = 0; // Current song-slot whose data-files are being played
byte PLAYING = 1; // Controls whether the sequences and global tick-counter are iterating
byte DUMMYTICK = 0; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
byte CLOCKMASTER = 1; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
byte BPM = DEFAULT_BPM; // Beats-per-minute value: one beat is 96 tempo-ticks
byte TICKCOUNT = 2; // Current global tick, bounded within the size of a 32nd-note (3 ticks, 0-indexed)
byte CUR32 = 127; // Current global 32nd-note (bounded to 128, or 16 beats, beats being quarter-notes)
word GLOBALRAND = 12345; // Global all-purpose semirandom value; gets changed on every tick

// Swing vars
byte SGRAN = 1; // Current SWING granularity (1 = 16th; 2 = 8th; 3 = quarter note; 4 = half note; 5 = whole note)
byte SAMOUNT = 64; // Current SWING amount (0 = full negative swing; 128 = full positive swing; 64 = no swing)
byte SPART = 0; // Tracks which section of the SWING is currently active

// Beat-scattering flags, one per seq.
// bits 0-3: scatter chance
// bits 4-7: scatter distance (0=off; 1,2,4 = 8th,4th,half [these can stack with each other])
byte SCATTER[49];

// Cued-command flags, one per seq.
// bit 0: TURN OFF
// bit 1: TURN ON
// bits 2-4: slice 4, 2, 1;
// bits 5-7: cue 4, 2, 1;
byte CMD[49];

// Holds 48 seqs' sizes and activity-flags
// bits 0-5: 1, 2, 4, 8, 16, 32 (= size, in half-notes, where each quarter-note is a "beat") (range limits: 1 to 32)
// bit 6: reserved
// bit 7: on/off flag
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



// Manual extern reference to the writeCommands function,
//     because the Arduino compiler's linking system gets broken by some functions that are called by the CmdFunc construct,
//     which, in practical terms, means that functions in func_cmds.ino that call writeCommands() won't get linked to it,
//     unless writeCommands() is linked manually with an extern command right here.
//extern void writeCommands(unsigned long pos, byte amt, byte b[], byte onchan);



// Typedef for a generic function that takes "column" and "row" arguments.
// This will be used to select RECORD-MODE functions based on which keychords are active.
typedef void (*CmdFunc) (byte col, byte row);



SdFat sd; // Initialize SdFat object
SdFile file; // Initialize an SdFile File object, to control default data read/write processes



void setup() {

	// Ensure that the global arrays don't contain junk-data
	memset(SCATTER, 0, 49);
	memset(CMD, 0, 49);
	memset(STATS, 0, 49);
	memset(POS, 0, 98);
	memset(MOUT, 0, 25);
	memset(SUST, 0, 25);
	memset(INBYTES, 0, 4);

	// Set all the keypad's row-pins to INPUT_PULLUP mode, and all its column-pins to OUTPUT mode
	DDRC = 0;
	PORTC = 255;
	DDRD |= B00011100;
	DDRB |= B00000011;

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

	loadSong(SONG); // Load whatever song-slot was in P.DAT, or the default song-slot if PRF.DAT didn't exist

	ABSOLUTETIME = micros(); // Make sure ABSOLUTETIME matches the current time, so tempo doesn't have an initial jerk

}


void loop() {

	parseRawMidi(); // Parse incoming MIDI

	updateTimer(); // Update the global timer

	updateGUI(); // Update the GUI, if applicable

	xorShift(GLOBALRAND); // Update the global semirandom value

}
