
// UI vars
byte LEFTCTRL = 0; // Tracks left-column control-button presses in bits 1 thru 5
byte BOTCTRL = 0; // Tracks bottom-row control-button presses in bits 1 thru 5
byte TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick's computations

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
unsigned long ELAPSED = 0; // Time elapsed since last tick
unsigned long TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Recording vars
boolean RECORDMODE = false; // Tracks whether RECORD MODE is active
boolean RECORDNOTES = false; // Tracks whether notes are being recorded into the top slice-sequence or not
byte BASENOTE = 0; // Baseline pitch-offset value for RECORD-mode notes
byte OCTAVE = 3; // Octave-offset value for RECORD-mode notes
byte VELOCITY = 127; // Baseline velocity-value for RECORD-mode notes
byte HUMANIZE = 0; // Maximum velocity-humanize value for RECORD-mode notes
byte CHANNEL = 0; // MIDI-CHANNEL for RECORD-mode notes
byte QUANTIZE = B00000100; // Time-quantize value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)
byte DURATION = B00001000; // Duration value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)

// Holds the 8 previous RECORD-mode note-pitches as 4-bit note-values of 0 to 11,
// unmodified by any BASENOTE value.
// A value of "1111" is "empty"
unsigned long RECPITCHES = 4294967295;

// Harmonization type for RECORD-mode notes
// bits 0-1: undertone strength (0-3)
// bits 2-3: overtone strength (0-3)
// bits 4-7: number of previous RECORD-mode pitches to analyze while coming up with harmonies (0-15)
byte HARMONIZE = 0;
byte HARMODDS = 0; // Harmonize-chance: 0 (0%) to 255 (100%)
byte HARMCHAN = 0; // Harmonize-channel: 0 to 15

// Harmonize-octave difference:
// bits 0-3: Down difference (0-11)
// bits 4-7: Up difference (0-11)
char HARMDIFF = 1;

// Flex type to use when storing RECORD-mode notes:
// A flex-note will be random, within its given directions and intervals, whenever it activates
// bits 0-1: 0 = move upward; 1 = move downward;
// bits 0-7: 2 = whole tone; 3 = minor third; 4 = major third; 5 = perfect fourth;
// 6 = perfect fifth; 7 = major sixth
byte FLEX = 0;

// Scatter-storage type for RECORD-mode notes
// bits 0-3: possible scatter delay times: 0 = half beat; 1 = quarter beat; 2 = eighth beat; 3 = sixteenth beat
// bits 4-7: chance that note will be scattered: 4 = 50%; 5 = 25%; 6 = 12.5%; 7 = 6.25%
byte SCATTER = 0;

// Channels to listen to in RECORD mode, for recording using external MIDI sources
// Each of the 16 bits corresponds to a MIDI channel
word LISTEN = 0;

// Sequencing vars
byte SONG = 0; // Current song-slot whose data-files are being played
boolean PLAYING = false; // Controls whether the sequences are iterating through their contents
unsigned int CURTICK = 1; // Current global sequencing tick
boolean DUMMYTICK = false; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
boolean CLOCKMASTER = true; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
byte BPM = 120; // Beats-per-minute value: one beat is 96 tempo-ticks

// Cued-command flags, one per seq.
// bits 0-1: cue 1, cue 8;
// bits 2-4: slice 1, 2, 4;
// bits 5-6: reserved;
// bit 7: on/off (1/0)
byte SEQ_CMD[73];

// Holds 72 seqs' sizes
// bits 0-6: 1, 2, 4, 8, 16, 32, 64 (* 24 * 4 = size, in ticks)
byte SEQ_PSIZE[73];

// Holds 72 seqs' internal tick-positions, slice-positions, and playing-statuses
// bit 0: playing
// bits 1-3: current slice (0-7 in binary)
// bits 4-15: current tick within active slice
word SEQ_POS[73];


// MIDI-IN vars
byte INBYTES[4] = {0, 0, 0};
byte INCOUNT = 0;
byte INTARGET = 0;
boolean SYSIGNORE = false;

// MIDI SUSTAIN vars
// Format: SUSTAIN[n] = {channel, pitch, remaining duration in ticks}
byte SUSTAIN[9][4] = {
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0}
};
