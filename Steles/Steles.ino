

/*

		Steles is a MIDI sequencer for the "Tegne" hardware.
		THIS CODE IS UNDER DEVELOPMENT AND DOESN'T DO ANYTHING!
		Copyright (C) 2016-2017, C.D.M. Rørmose (sevenplagues@gmail.com).

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
#define FILE_BYTES 393265UL // Number of bytes in each seq-file
#define SCANRATE 6000 // Amount of time between keystroke-scans, in microseconds



// Program-space (PROGMEM) library
#include <avr/pgmspace.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>



// Scrolling glyph: logo to display on startup
const unsigned long LOGO[] PROGMEM = {
	4294934211,
	4294967267,
	415286259,
	419414235,
	419418063,
	415286215,
	419430339,
	419397571
};

// Glyph: BASENOTE
const byte GLYPH_BASENOTE[] PROGMEM = {B11101001, B10101101, B11001111, B10101011, B11101001};

// Glyph: BPM
const byte GLYPH_BPM[] PROGMEM = {B01010001, B01011011, B01010101, B11010001, B11010001};

// Glyph: CHAN
const byte GLYPH_CHAN[] PROGMEM = {B11101000, B10101000, B10001100, B10101010, B11101010};

// Glyph: CLOCK-MASTER
const byte GLYPH_CLOCKMASTER[] PROGMEM = {B11101000, B10101000, B10001010, B10101100, B11101010};

// Glyph: CONTROL-CHANGE
const byte GLYPH_CONTROLCHANGE[] PROGMEM = {B11101110, B10101010, B10001000, B10101010, B11101110};

// Glyph: DURATION
const byte GLYPH_DURATION[] PROGMEM = {B11000000, B10100000, B10101110, B10101000, B11001000};

// Glyph: "ERASING"
const byte GLYPH_ERASE[] PROGMEM = {B11101010, B10001010, B11101010, B10000000, B11101010};

// Glyph: HUMANIZE
const byte GLYPH_HUMANIZE[] PROGMEM = {B10100000, B10100000, B11101010, B10101010, B10101110};

// Glyph: LISTEN-CHAN
const byte GLYPH_LISTEN[] PROGMEM = {B10001110, B10001010, B10001000, B10001010, B11101110};

// Glyph: LOAD
const byte GLYPH_LOAD[] PROGMEM = {B10001010, B10001010, B10001010, B10000000, B11101010};

// Glyph: OCTAVE
const byte GLYPH_OCTAVE[] PROGMEM = {B11100000, B10100000, B10101110, B10101000, B11101110};

// Glyph: PLAY/STOP (PLAYING)
const byte GLYPH_PLAYSTOP_ARMED[] PROGMEM = {B10001010, B11001010, B11101010, B11000000, B10001010};

// Glyph: PLAY/STOP (STOPPED)
const byte GLYPH_PLAYSTOP[] PROGMEM = {B10000000, B11000000, B11101010, B11000100, B10001010};

// Glyph: QUANTIZE
const byte GLYPH_QUANTIZE[] PROGMEM = {B01000000, B10100000, B10100101, B10100101, B01110111};

// Glyph: "RECORDING"
const byte GLYPH_RECORDING[] PROGMEM = {B11100000, B10100000, B11000000, B10100000, B10100000};

// Glyph: "RECORDING NOW!"
const byte GLYPH_RECORDING_ARMED[] PROGMEM = {B11101010, B10101010, B11001010, B10100000, B10101010};

// Glyph: "SWITCH RECORDING-SEQUENCE"
const byte GLYPH_RSWITCH[] PROGMEM = {B11101110, B10101000, B11000100, B10100010, B10101110};

// Glyph: "SD-CARD"
const byte GLYPH_SD[] PROGMEM = {B11101100, B10001010, B01001010, B00101010, B11101100};

// Glyph: "SEQ-SIZE"
const byte GLYPH_SIZE[] PROGMEM = {B11101110, B10000010, B01000100, B00101000, B11101110};

// Glyph: VELO
const byte GLYPH_VELO[] PROGMEM = {B10010000, B10010000, B01010000, B00110000, B00010000};



// UI vars
unsigned long BUTTONS = 0; // Tracks which of the 30 buttons are currently pressed; each button has an on/off bit
byte PAGE = 0; // Tracks currently-active page of sequences
byte TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
unsigned long ELAPSED = 0; // Time elapsed since last tick
word KEYELAPSED = 0; // Time elapsed since last keystroke-scan
unsigned long TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Recording vars
byte RECORDMODE = 0; // Tracks whether RECORD MODE is active
byte RECORDNOTES = 0; // Tracks whether notes are being recorded into the RECORDSEQ-sequence or not
byte ERASENOTES = 0; // Tracks whether notes are being erased from the RECORDSEQ-sequence or not
byte RECORDSEQ = 0; // Sequence currently being recorded into
byte BASENOTE = 0; // Baseline pitch-offset value for RECORD-mode notes
byte OCTAVE = 3; // Octave-offset value for RECORD-mode notes
byte VELO = 127; // Baseline velocity-value for RECORD-mode notes
byte HUMANIZE = 0; // Maximum velocity-humanize value for RECORD-mode notes
byte CHAN = 0; // MIDI-CHAN for RECORD-mode notes
byte LISTEN = 0; // Channel to listen to in RECORD mode, for recording from external MIDI sources
byte QUANTIZE = B00000100; // Time-quantize value for RECORD-mode notes: bits 0-3: 1, 2, 4, 8 (16th-notes)
byte DURATION = B00001000; // Duration value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)

// Sequencing vars
byte SONG = 0; // Current song-slot whose data-files are being played
byte PLAYING = 1; // Controls whether the sequences and global tick-counter are iterating
byte DUMMYTICK = 0; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
byte CLOCKMASTER = 1; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
byte BPM = 120; // Beats-per-minute value: one beat is 96 tempo-ticks
byte TICKCOUNT = 5; // Current global tick, bounded within the size of a 16th-note
byte CUR16 = 127; // Current global sixteenth-note (bounded to 128, or 8 beats)

// Beat-scattering flags, one per seq.
// bits 0-2: scatter chance
// bits 3-6: count of notes since last scatter
// bit 7: flag for "seq's read-head is in scatter mode"
byte SCATTER[49];

// Cued-command flags, one per seq.
// bit 0: TURN OFF
// bit 1: TURN ON
// bits 2-4: slice 4, 2, 1;
// bits 5-7: cue 4, 2, 1;
byte CMD[49];

// Holds 48 seqs' sizes and activity-flags
// bits 0-6: 1, 2, 4, 8, 16, 32, 64 (= size, in beats)
// bit 7: on/off flag
byte STATS[49];

// Holds the 48 seqs' internal tick-positions
// bits 0-15: current 16th-note position
word POS[49];

// Holds up to 8 MIDI notes from a given tick,
// (format: MOUT[n*3] = command, pitch, velocity)
// to be sent in a batch at the tick's end
byte MOUT[25];
byte MOUT_COUNT = 0; // Counts the current number of note entries in MOUT

// Note-Sustain data storage
// (format: SUST[n*3] = duration, channel, pitch)
byte SUST[25];
byte SUST_COUNT = 0; // Counts the current number of sustained notes

// Keeps a record of the most recent note-pitch sent to each MIDI channel
// 255 = "no qualifying note has been played in this context yet"
byte RECENT[17] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

// MIDI-IN vars
byte INBYTES[4]; // Buffer for incoming MIDI commands
byte INCOUNT = 0; // Number of MIDI bytes received from current incoming command
byte INTARGET = 0; // Number of expected incoming MIDI bytes
byte SYSIGNORE = 0; // Ignores SYSEX messages when toggled



SdFat sd; // Initialize SdFat object
SdFile file; // Initialize an SdFile File object, to control default data read/write processes

LedControl lc = LedControl(5, 7, 6, 1); // Initialize the object that controls the MAX7219's LED-grid



void setup() {

	// Set all the keypad's row-pins to INPUT_PULLUP mode, and all its column-pins to OUTPUT mode
	DDRC = 0;
	PORTC = 255;
	DDRD |= B00011100;
	DDRB |= B00000011;

	// Power up ledControl to full brightness
	lc.shutdown(0, false);
	lc.setIntensity(0, 15);

	// Initialize the SD-card at full speed, or throw a visible error message if no SD-card is inserted
	if (!sd.begin(10, SPI_FULL_SPEED)) {
		lc.setRow(0, 2, B11101110);
		lc.setRow(0, 3, B10001001);
		lc.setRow(0, 4, B11101001);
		lc.setRow(0, 5, B00101001);
		lc.setRow(0, 6, B00101010);
		lc.setRow(0, 7, B11101100);
		sd.initErrorHalt();
	}

	startupAnimation(); // Display startup-animation

	loadSong(SONG); // Load the default song, or create its savefile if it doesn't exist

	// Start serial comms at the MIDI baud rate
	Serial.begin(31250);
	
}


void loop() {

	//parseRawMidi();

	updateTimer();

	updateGUI();

}

