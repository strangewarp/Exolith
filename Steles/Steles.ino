

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



// Digital-pin keypad library
#include <Key.h>
#include <Keypad.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>



// Number of uint8_ts in each savefile
const uint32_t FILE_BYTES PROGMEM = 393265;

// Buffer of empty uint8_ts, for writing into a newly-emptied tick en masse
const uint8_t EMPTY_TICK[9] PROGMEM = {0, 0, 0, 0, 0, 0, 0, 0};

// Glyph: BASENOTE
const uint8_t GLYPH_BASENOTE[7] PROGMEM = {B11100010, B10010010, B11100010, B10010110, B10010110, B11100000};

// Glyph: BPM
const uint8_t GLYPH_BPM[7] PROGMEM = {B01111110, B11001011, B10011001, B10011001, B11000011, B01111110};

// Glyph: CHAN
const uint8_t GLYPH_CHAN[7] PROGMEM = {B11101001, B10101001, B10001111, B10001001, B10101001, B11101001};

// Glyph: CLOCK-MASTER
const uint8_t GLYPH_CLOCKMASTER[7] PROGMEM = {B11101001, B10101010, B10001100, B10001010, B10101001, B11101001};

// Glyph: DURATION
const uint8_t GLYPH_DURATION[7] PROGMEM = {B11001110, B10101001, B10101110, B10101011, B10101001, B11001001};

// Glyph: "ERASING"
const uint8_t GLYPH_ERASE[7] PROGMEM = {B11110101, B10000101, B11110101, B10000101, B10000000, B11110101
};

// Glyph: HUMANIZE
const uint8_t GLYPH_HUMANIZE[7] PROGMEM = {B10101001, B10101001, B11101001, B10101001, B10101001, B10100110};

// Glyph: LISTEN-CHAN
const uint8_t GLYPH_LISTEN[7] PROGMEM = {B10001110, B10001010, B10001000, B10001000, B10001010, B11101110};

// Glyph: LOAD
const uint8_t GLYPH_LOAD[7] PROGMEM = {B10000101, B10000101, B10000101, B10000101, B10000000, B11110101};

// Glyph: OCTAVE
const uint8_t GLYPH_OCTAVE[7] PROGMEM = {B01100111, B10010101, B10010100, B10010100, B10010101, B01100111};

// Glyph: QUANTIZE
const uint8_t GLYPH_QUANTIZE[7] PROGMEM = {B01100010, B10010010, B10010010, B10010110, B01100110, B00110000};

// Glyph: "RECORDING"
const uint8_t GLYPH_RECORDING[7] PROGMEM = {B11100000, B10010000, B11100000, B10010000, B10010000, B10010000};

// Glyph: "RECORDING NOW!"
const uint8_t GLYPH_RECORDING_ARMED[7] PROGMEM = {B11100101, B10010101, B11100101, B10010101, B10010000, B10010101};

// Glyph: "SWITCH RECORDING-SEQUENCE"
const uint8_t GLYPH_RSWITCH[7] PROGMEM = {B11100111, B10010100, B11100111, B10010001, B10010001, B10010111};

// Glyph: "SAVE"
const uint8_t GLYPH_SAVE[7] PROGMEM = {B01110101, B10000101, B01100101, B00010101, B00010000, B11100101};

// Glyph: "SD-CARD"
const uint8_t GLYPH_SD[7] PROGMEM = {B11101110, B10001001, B11101001, B00101001, B00101010, B11101100};

// Glyph: "SEQ-SIZE"
const uint8_t GLYPH_SIZE[7] PROGMEM = {B11101111, B10000001, B11100010, B00100100, B00101000, B11101111};

// Glyph: VELO
const uint8_t GLYPH_VELO[7] PROGMEM = {B10010111, B10010100, B10010111, B10100100, B11000100, B10000111};



// UI vars
uint8_t CTRL = 0; // Tracks left-column control-button presses in bits 0 thru 5
uint8_t PAGE = 0; // Tracks currently-active page of sequences
uint8_t TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick

// Timing vars
uint32_t ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
uint32_t ELAPSED = 0; // Time elapsed since last tick
uint32_t TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Recording vars
uint8_t RECORDMODE = 0; // Tracks whether RECORD MODE is active
uint8_t RECORDNOTES = 0; // Tracks whether notes are being recorded into the RECORDSEQ-sequence or not
uint8_t ERASENOTES = 0; // Tracks whether notes are being erased from the RECORDSEQ-sequence or not
uint8_t RECORDSEQ = 0; // Sequence currently being recorded into
uint8_t BASENOTE = 0; // Baseline pitch-offset value for RECORD-mode notes
uint8_t OCTAVE = 3; // Octave-offset value for RECORD-mode notes
uint8_t VELO = 127; // Baseline velocity-value for RECORD-mode notes
uint8_t HUMANIZE = 0; // Maximum velocity-humanize value for RECORD-mode notes
uint8_t CHAN = 0; // MIDI-CHAN for RECORD-mode notes
uint8_t LISTEN = 0; // Channel to listen to in RECORD mode, for recording from external MIDI sources
uint8_t QUANTIZE = B00000100; // Time-quantize value for RECORD-mode notes: bits 0-3: 1, 2, 4, 8 (16th-notes)
uint8_t DURATION = B00001000; // Duration value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)

// Sequencing vars
uint8_t SONG = 0; // Current song-slot whose data-files are being played
uint8_t PLAYING = 0; // Controls whether the sequences are iterating through their contents
uint8_t DUMMYTICK = 0; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
uint8_t CLOCKMASTER = 1; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
uint8_t BPM = 120; // Beats-per-minute value: one beat is 96 tempo-ticks
uint8_t TICKCOUNT = 5; // Current global tick, bounded within the size of a 16th-note
uint8_t CUR16 = 127; // Current global sixteenth-note (bounded to 128, or 8 beats)

// Beat-scattering flags, one per seq.
// bits 0-2: scatter chance
// bits 3-6: count of notes since last scatter
// bit 7: flag for "seq's read-head is in scatter mode"
uint8_t SCATTER[49];

// Cued-command flags, one per seq.
// bit 0: TURN OFF
// bit 1: TURN ON
// bits 2-4: slice 4, 2, 1;
// bits 5-7: cue 4, 2, 1;
uint8_t CMD[49];

// Holds 48 seqs' sizes and activity-flags
// bits 0-6: 1, 2, 4, 8, 16, 32, 64 (= size, in beats)
// bit 7: on/off flag
uint8_t STATS[49];

// Holds the 48 seqs' internal tick-positions
// bits 0-15: current 16th-note position
uint16_t POS[49];

// Holds up to 8 MIDI notes from a given tick,
// (format: MOUT[n*3] = command, pitch, velocity)
// to be sent in a batch at the tick's end
uint8_t MOUT[25];
uint8_t MOUT_COUNT = 0; // Counts the current number of note entries in MOUT

// Note-Sustain data storage
// (format: SUST[n*3] = duration, channel, pitch)
uint8_t SUST[25];
uint8_t SUST_COUNT = 0; // Counts the current number of sustained notes

// MIDI-IN vars
uint8_t INBYTES[4]; // Buffer for incoming MIDI commands
uint8_t INCOUNT = 0; // Number of MIDI bytes received from current incoming command
uint8_t INTARGET = 0; // Number of expected incoming MIDI bytes
uint8_t SYSIGNORE = 0; // Ignores SYSEX messages when toggled



// Initialize the object that controls the Keypad buttons
const uint8_t ROWS PROGMEM = 6;
const uint8_t COLS PROGMEM = 5;
int8_t KEYS[ROWS][COLS] = {
	{'0', '1', '2', '3', '4'},
	{'5', '6', '7', '8', '9'},
	{':', ';', '<', '=', '>'},
	{'?', '@', 'A', 'B', 'C'},
	{'D', 'E', 'F', 'G', 'H'},
	{'I', 'J', 'K', 'L', 'M'}
};
uint8_t rowpins[ROWS] = {5, 6, 7, 8, 18, 19};
uint8_t colpins[COLS] = {9, 14, 15, 16, 17};
Keypad kpd(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS);

SdFat sd; // Initialize SdFat FAT32 Volume object
SdFile file;// Initialize an SdFile File object, to control default data read/write processes

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

	// Set a short debounce time (8ms), but not so short as to lag the other functions with constant checks
	kpd.setDebounceTime(8);

	// Load the default song, or create its folder and files if they don't exist
	loadSong(SONG);

	// Start serial comms at the MIDI baud rate
	Serial.begin(31250);
	
}


void loop() {

	parseRawMidi();

	updateTimer();

	parseKeystrokes();

	updateGUI();

}

