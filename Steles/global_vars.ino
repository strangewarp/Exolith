
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
byte LISTEN = 0; // Channel to listen to in RECORD mode, for recording from external MIDI sources
byte QUANTIZE = B00000100; // Time-quantize value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)
byte DURATION = B00001000; // Duration value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)

// Sequencing vars
byte SONG = 0; // Current song-slot whose data-files are being played
boolean PLAYING = false; // Controls whether the sequences are iterating through their contents
unsigned int CURTICK = 1; // Current global sequencing tick
boolean DUMMYTICK = false; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
boolean CLOCKMASTER = true; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
byte BPM = 120; // Beats-per-minute value: one beat is 96 tempo-ticks

// Cued-command flags, one per seq.
// bit 0: TURN OFF
// bit 1: TURN ON
// bits 2-4: slice 4, 2, 1;
// bits 5-7: cue 4, 2, 1;
byte SEQ_CMD[49];

// Holds 48 seqs' sizes and activity-flags
// bits 0-6: 1, 2, 4, 8, 16, 32, 64 (= size, in beats)
// bit 7: on/off flag
byte SEQ_STATS[49];

// Holds the 48 seqs' internal tick-positions
// bits 0-7: current 16th-note position
byte SEQ_POS[49];

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
byte INBYTES[4];
byte INCOUNT = 0;
byte INTARGET = 0;
boolean SYSIGNORE = false;

