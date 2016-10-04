
// UI vars
byte LEFTCTRL = 0; // Tracks left-column control-buttons in bits 1 thru 5
byte BOTCTRL = 0; // Tracks bottom-row control-buttons in bits 1 thru 5

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
unsigned long ELAPSED = 0; // Time elapsed since last tick
unsigned long TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Recording vars
boolean RECORDMODE = false; // Tracks whether RECORD MODE is active
boolean RECORDNOTES = false; // Tracks whether notes are being recorded into the top slice-sequence or not
byte PITCHOFFSET = 0; // Baseline pitch-offset value for RECORD-mode notes
byte CHANNEL = 0; // MIDI-CHANNEL for RECORD-mode notes
byte OCTAVE = 3; // Octave-offset value for RECORD-mode notes
byte VELOCITY = 127; // Baseline velocity-value for RECORD-mode notes
byte HUMANIZE = 0; // Maximum velocity-humanize value for RECORD-mode notes
byte QUANTIZE = B00000100; // Time-quantize value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)
byte DURATION = B00001000; // Duration value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)

// Sequencing vars
boolean PLAYING = false; // Controls whether the sequences are iterating through their contents
unsigned int CURTICK = 1; // Current global sequencing tick
boolean DUMMYTICK = false; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
boolean CLOCKMASTER = true; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
byte BPM = 120; // Beats-per-minute value: one beat is 96 tempo-ticks
byte SEQ_CMD[33]; // Cued-command flags, one per sequence. bits 1-2 = cue 1, 8; bits 3-5 = slice 1, 2, 4; bits 6-7: unused; bit 8: on/off (1/0)
byte SEQ_EXCLUDE[33]; // Holds a seq's EXCLUDE values: each bit corresponds to a slice. 1 = include; 0 = skip
byte SEQ_OFFSET[17]; // Holds 32 4-bit values for SCATTER-based tick-position offset: 0 = neg/pos; 1-3 = eighth/quarter/half note
byte SEQ_PLAYING[5]; // Holds 32 bits that describe whether a given sequence is currently playing
word SEQ_POS[33]; // Holds the current tick-position for each sequence
byte SEQ_RAND[33]; // Holds randomize-slice and randomize-velocity values: (0 to 3) = binary rand-slice chance; (4 to 7) = binary rand-velocity variance
byte SEQ_SCATTER[33]; // Holds a seq's SCATTER values: bits 0-3 are "intervals", bits 4-7 are "chance"
byte SEQ_SIZE[17]; // Holds 32 4-bit values for each sequence's size: (1 to 15; 0 = 16) * 2 = beats in seq
byte SLICE_ROW[6]; // Holds the indexes of the sequences currently on the slicing-page

// MIDI-IN vars
byte INBYTES[4] = {0, 0, 0};
byte INCOUNT = 0;
byte INTARGET = 0;
byte SYSBYTES[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte SYSCOUNT = 0;
boolean SYSIGNORE = false;
boolean SYSLATCH = false;

// MIDI SUSTAIN vars
// Format: SUSTAIN[n] = {channel, pitch, remaining duration in ticks}
byte SUSTAIN[9][4] = {
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0}
};
