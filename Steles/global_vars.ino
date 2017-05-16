
// UI vars
byte CTRL = 0; // Tracks left-column control-button presses in bits 0 thru 5
byte TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick

// Timing vars
unsigned long ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
unsigned long ELAPSED = 0; // Time elapsed since last tick
unsigned long TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Recording vars
boolean RECORDMODE = false; // Tracks whether RECORD MODE is active
boolean RECORDNOTES = false; // Tracks whether notes are being recorded into the RECORDSEQ-sequence or not
byte RECORDSEQ = 0; // Sequence currently being recorded into
byte BASENOTE = 0; // Baseline pitch-offset value for RECORD-mode notes
byte OCTAVE = 3; // Octave-offset value for RECORD-mode notes
byte VELOCITY = 127; // Baseline velocity-value for RECORD-mode notes
byte HUMANIZE = 0; // Maximum velocity-humanize value for RECORD-mode notes
byte CHANNEL = 0; // MIDI-CHANNEL for RECORD-mode notes
byte QUANTIZE = B00000100; // Time-quantize value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)
byte DURATION = B00001000; // Duration value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)

// Channels to listen to in RECORD mode, for recording using external MIDI sources
byte LISTENS[4] = {1, 2, 3};

// Sequencing vars
byte SONG = 0; // Current song-slot whose data-files are being played
boolean PLAYING = false; // Controls whether the sequences are iterating through their contents
unsigned int CURTICK = 1; // Current global sequencing tick
boolean DUMMYTICK = false; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
boolean CLOCKMASTER = true; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
byte BPM = 120; // Beats-per-minute value: one beat is 96 tempo-ticks

// Cued-command flags, one per seq.
// bits 0-2: cue 4, 2, 1;
// bits 3-5: slice 4, 2, 1;
// bit 6: TURN ON
// bit 7: TURN OFF
byte SEQ_CMD[73];

// Holds 72 seqs' sizes
// bit 0: reserved
// bits 1-7: 1, 2, 4, 8, 16, 32, 64 (* 24 * 4 = size, in ticks)
byte SEQ_SIZE[73];

// Holds 72 seqs' internal tick-positions and playing-statuses
// bit 0: playing
// bits 1-15: current tick
word SEQ_POS[73];

// MIDI-IN vars
byte INBYTES[4] = {0, 0, 0};
byte INCOUNT = 0;
byte INTARGET = 0;
boolean SYSIGNORE = false;

// MIDI SUSTAIN vars
// Format: SUSTAIN[n] = {channel, pitch, remaining duration in ticks}
byte SUSTAIN[9][4] = {
	{255, 255, 255}, {255, 255, 255},
	{255, 255, 255}, {255, 255, 255},
	{255, 255, 255}, {255, 255, 255},
	{255, 255, 255}, {255, 255, 255}
};
