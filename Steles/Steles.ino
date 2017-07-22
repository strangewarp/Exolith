

/*

		Steles is a MIDI sequencer for the "Tegne" hardware.
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



// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>



// Number of bytes in each savefile
const unsigned long FILE_BYTES PROGMEM = 393265;

// Buffer of empty bytes, for writing into a newly-emptied tick en masse
const byte EMPTY_TICK[9] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0};

// Glyph: BASENOTE
const byte GLYPH_BASENOTE[7] PROGMEM = {B11100010, B10010010, B11100010, B10010110, B10010110, B11100000};

// Glyph: BPM
const byte GLYPH_BPM[7] PROGMEM = {B01111110, B11001011, B10011001, B10011001, B11000011, B01111110};

// Glyph: CHAN
const byte GLYPH_CHAN[7] PROGMEM = {B11101001, B10101001, B10001111, B10001001, B10101001, B11101001};

// Glyph: CLOCK-MASTER
const byte GLYPH_CLOCKMASTER[7] PROGMEM = {B11101001, B10101010, B10001100, B10001010, B10101001, B11101001};

// Glyph: DURATION
const byte GLYPH_DURATION[7] PROGMEM = {B11001110, B10101001, B10101110, B10101011, B10101001, B11001001};

// Glyph: "ERASING"
const byte GLYPH_ERASE[7] PROGMEM = {B11110101, B10000101, B11110101, B10000101, B10000000, B11110101
};

// Glyph: HUMANIZE
const byte GLYPH_HUMANIZE[7] PROGMEM = {B10101001, B10101001, B11101001, B10101001, B10101001, B10100110};

// Glyph: LISTEN-CHAN
const byte GLYPH_LISTEN[7] PROGMEM = {B10001110, B10001010, B10001000, B10001000, B10001010, B11101110};

// Glyph: LOAD
const byte GLYPH_LOAD[7] PROGMEM = {B10000101, B10000101, B10000101, B10000101, B10000000, B11110101};

// Glyph: OCTAVE
const byte GLYPH_OCTAVE[7] PROGMEM = {B01100111, B10010101, B10010100, B10010100, B10010101, B01100111};

// Glyph: QUANTIZE
const byte GLYPH_QUANTIZE[7] PROGMEM = {B01100010, B10010010, B10010010, B10010110, B01100110, B00110000};

// Glyph: "RECORDING"
const byte GLYPH_RECORDING[7] PROGMEM = {B11100000, B10010000, B11100000, B10010000, B10010000, B10010000};

// Glyph: "RECORDING NOW!"
const byte GLYPH_RECORDING_ARMED[7] PROGMEM = {B11100101, B10010101, B11100101, B10010101, B10010000, B10010101};

// Glyph: "SWITCH RECORDING-SEQUENCE"
const byte GLYPH_RSWITCH[7] PROGMEM = {B11100111, B10010100, B11100111, B10010001, B10010001, B10010111};

// Glyph: "SAVE"
const byte GLYPH_SAVE[7] PROGMEM = {B01110101, B10000101, B01100101, B00010101, B00010000, B11100101};

// Glyph: "SD-CARD"
const byte GLYPH_SD[7] PROGMEM = {B11101110, B10001001, B11101001, B00101001, B00101010, B11101100};

// Glyph: "SEQ-SIZE"
const byte GLYPH_SIZE[7] PROGMEM = {B11101111, B10000001, B11100010, B00100100, B00101000, B11101111};

// Glyph: VELO
const byte GLYPH_VELO[7] PROGMEM = {B10010111, B10010100, B10010111, B10100100, B11000100, B10000111};



// REG, BIT, and SUBNUM consts:
// REG describes which port-register the pin is in (0 = PORTD; 1 = PORTB; 2 = PORTC)
// BIT holds the positional value for the pin's place in its port-register
// SUBNUM holds the numeric value for the pin's place in its port-register
const byte ROW_SUBNUM[7] PROGMEM = {5, 6, 7, 0, 2, 3};
const byte ROW_BIT[7] PROGMEM = {
	B00100000, // row 0: pin 5  = 5
	B01000000, // row 1: pin 6  = 6
	B10000000, // row 2: pin 7  = 7
	B00000001, // row 3: pin 8  = 0
	B00000100, // row 4: pin 18 = 2
	B00001000  // row 5: pin 19 = 3
};
const byte ROW_REG[7] PROGMEM = {
	B00000000, // row 0: pin 5  = 0
	B00000000, // row 1: pin 6  = 0
	B00000000, // row 2: pin 7  = 0
	B00000001, // row 3: pin 8  = 1
	B00000010, // row 4: pin 18 = 2
	B00000010  // row 5: pin 19 = 2
};
const byte COL_SUBNUM[6] PROGMEM = {1, 6, 7, 0, 1};
const byte COL_BIT[6] PROGMEM = {
	B00000010, // col 0: pin 9  = 1
	B01000000, // col 1: pin 14 = 6
	B10000000, // col 2: pin 15 = 7
	B00000001, // col 3: pin 16 = 0
	B00000010  // col 4: pin 17 = 1
};
const byte COL_REG[6] PROGMEM = {
	B00000001, // col 0: pin 9  = 1
	B00000001, // col 1: pin 14 = 1
	B00000001, // col 2: pin 15 = 1
	B00000010, // col 3: pin 16 = 2
	B00000010  // col 4: pin 17 = 2
};



// UI vars
unsigned long BUTTONS = 0; // Tracks which of the 30 buttons are currently pressed; each button has an on/off bit
byte PAGE = 0; // Tracks currently-active page of sequences
byte TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
unsigned long ELAPSED = 0; // Time elapsed since last tick
word KEYELAPSED = 0; // Time elapsed since last keystroke-scan
const word SCANRATE PROGMEM = 7000; // Amount of time between keystroke-scans, in microseconds
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
byte PLAYING = 0; // Controls whether the sequences are iterating through their contents
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

// MIDI-IN vars
byte INBYTES[4]; // Buffer for incoming MIDI commands
byte INCOUNT = 0; // Number of MIDI bytes received from current incoming command
byte INTARGET = 0; // Number of expected incoming MIDI bytes
byte SYSIGNORE = 0; // Ignores SYSEX messages when toggled



SdFat sd; // Initialize SdFat object
SdFile file; // Initialize an SdFile File object, to control default data read/write processes

// Initialize the object that controls the MAX7219's LED-grid
LedControl lc = LedControl(2, 3, 4, 1);



void setup() {

	// Power up ledControl to full brightness
	lc.shutdown(0, false);
	lc.setIntensity(0, 15);

	// Initialize the SD-card at full speed, or throw a visible error message if no SD-card is inserted
	if (!sd.begin(10, SPI_FULL_SPEED)) {
		for (byte i = 2; i < 8; i++) {
			lc.setRow(0, i, GLYPH_SD[i - 2]);
		}
		sd.initErrorHalt();
	}

	// Set all the keypad's row-pins to INPUT_PULLUP mode, and all its column-pins to OUTPUT mode
	for (byte i = 0; i < 6; i++) {
		pinMode((ROW_REG[i] << 3) + ROW_SUBNUM[i], INPUT_PULLUP);
		if (i == 5) { break; }
		pinMode((COL_REG[i] << 3) + COL_SUBNUM[i], OUTPUT);
	}

	// Load the default song, or create its folder and files if they don't exist
	loadSong(SONG);

	// Start serial comms at the MIDI baud rate
	Serial.begin(31250);
	
}


void loop() {

	parseRawMidi();

	updateTimer();

	updateGUI();

}

