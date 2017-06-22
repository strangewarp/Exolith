
// UI vars
uint8_c CTRL = 0; // Tracks left-column control-button presses in bits 0 thru 5
uint8_c TO_UPDATE = 0; // Tracks which rows of LEDs should be updated at the end of a given tick

// Timing vars
uint32_c ABSOLUTETIME = 0; // Absolute time elapsed: wraps around after reaching its limit
uint32_c ELAPSED = 0; // Time elapsed since last tick
uint32_c TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Recording vars
uint8_c RECORDMODE = 0; // Tracks whether RECORD MODE is active
uint8_c RECORDNOTES = 0; // Tracks whether notes are being recorded into the RECORDSEQ-sequence or not
uint8_c RECORDSEQ = 0; // Sequence currently being recorded into
uint8_c BASENOTE = 0; // Baseline pitch-offset value for RECORD-mode notes
uint8_c OCTAVE = 3; // Octave-offset value for RECORD-mode notes
uint8_c VELOCITY = 127; // Baseline velocity-value for RECORD-mode notes
uint8_c HUMANIZE = 0; // Maximum velocity-humanize value for RECORD-mode notes
uint8_c CHANNEL = 0; // MIDI-CHANNEL for RECORD-mode notes
uint8_c LISTEN = 0; // Channel to listen to in RECORD mode, for recording from external MIDI sources
uint8_c QUANTIZE = B00000100; // Time-quantize value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)
uint8_c DURATION = B00001000; // Duration value for RECORD-mode notes: bits 0-7: 1, 3, 6, 12, 24, 48, 96, 192 (ticks)

// Sequencing vars
uint8_c SONG = 0; // Current song-slot whose data-files are being played
uint8_c PLAYING = 0; // Controls whether the sequences are iterating through their contents
uint8_c DUMMYTICK = 0; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences
uint8_c CLOCKMASTER = 1; // Toggles whether to generate MIDI CLOCK ticks, or respond to incoming CLOCK ticks from an external device
uint8_c BPM = 120; // Beats-per-minute value: one beat is 96 tempo-ticks
uint8_c TICKCOUNT = 5; // Current global tick, bounded within the size of a 16th-note
uint8_c CUR16 = 127; // Current global sixteenth-note (bounded to 128, or 8 beats)

// Cued-command flags, one per seq.
// bit 0: TURN OFF
// bit 1: TURN ON
// bits 2-4: slice 4, 2, 1;
// bits 5-7: cue 4, 2, 1;
uint8_c SEQ_CMD[49];

// Holds 48 seqs' sizes and activity-flags
// bits 0-6: 1, 2, 4, 8, 16, 32, 64 (= size, in beats)
// bit 7: on/off flag
uint8_c SEQ_STATS[49];

// Holds the 48 seqs' internal tick-positions
// bits 0-7: current 16th-note position
uint8_c SEQ_POS[49];

// Holds up to 8 MIDI notes from a given tick,
// (format: MOUT[n*3] = command, pitch, velocity)
// to be sent in a batch at the tick's end
uint8_c MOUT[25];
uint8_c MOUT_COUNT = 0; // Counts the current number of note entries in MOUT

// Note-Sustain data storage
// (format: SUST[n*3] = duration, channel, pitch)
uint8_c SUST[25];
uint8_c SUST_COUNT = 0; // Counts the current number of sustained notes

// MIDI-IN vars
uint8_c INBYTES[4]; // Buffer for incoming MIDI commands
uint8_c INCOUNT = 0; // Number of MIDI bytes received from current incoming command
uint8_c INTARGET = 0; // Number of expected incoming MIDI bytes
uint8_c SYSIGNORE = 0; // Ignores SYSEX messages when toggled


