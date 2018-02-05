
// Arm or disarm RECORDNOTES
void armCmd(byte col, byte row) { armCmd(); }
void armCmd() {
	RECORDNOTES ^= 1; // Arm or disarm the RECORDNOTES flag
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse a BASENOTE press
void baseCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	BASENOTE = clamp(0, 11, char(BASENOTE) + change); // Modify the BASENOTE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CHAN press
void chanCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	CHAN = (CHAN & 48) | clamp(0, 15, char(CHAN & 15) + change); // Modify the CHAN value, while preserving CC & PC flags
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CLOCK-MASTER press
void clockCmd(byte col, byte row) { clockCmd(); }
void clockCmd() {
	CLOCKMASTER ^= 1; // Toggle the CLOCK-MASTER value
	ABSOLUTETIME = micros(); // Set the ABSOLUTETIME-tracking var to now
	ELAPSED = 0; // Set the ELAPSED value to show that no time has elapsed since the last tick-check
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CONTROL-CHANGE press
void controlChangeCmd(byte col, byte row) { controlChangeCmd(); }
void controlChangeCmd() {
	CHAN ^= 16; // Flip the CHAN bit that turns all NOTE command-entry into CC command-entry
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a COPY press
void copyCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	COPYPOS = POS[RECORDSEQ] - (POS[RECORDSEQ] % 16); // Set the COPY-position to the start of the beat
	COPYSEQ = RECORDSEQ; // Set the COPY-seq to the curent RECORD-seq
	COPYSIZE = abs(change); // Set the COPY-size to 1, 2, 4, 8, 16, or 32 beats
	BLINK = 255; // Start an LED-BLINK that is ~16ms long
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse a DURATION press
void durationCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	DURATION = clamp(0, 255, int(DURATION) + change); // Modify the DURATION value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse all of the possible actions that signal the recording of commands
void genericCmd(byte col, byte row) {
	byte key = col + (row * 4); // Get the key at the intersection of the column and row
	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity
	processRecAction(ctrl, key); // Parse all of the possible actions that signal the recording of commands
}

// Parse a HUMANIZE press
void humanizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	HUMANIZE = clamp(0, 127, int(HUMANIZE) + change); // Modify the HUMANIZE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse an OCTAVE press
void octaveCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	OCTAVE = clamp(0, 10, char(OCTAVE) + change); // Modify the OCTAVE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a PASTE press
void pasteCmd(byte col, byte row) {

}

// Parse a SHIFT CURRENT POSITION press
void posCmd(byte col, byte row) {

}

// Parse a PROGRAM-CHANGE press
void programChangeCmd(byte col, byte row) {

}

// Parse a QUANTIZE press
void quantizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	QUANTIZE = clamp(0, 16, abs(change)); // Modify the QUANTIZE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a REPEAT press
void repeatCmd(byte col, byte row) { repeatCmd(); }
void repeatCmd() {
	REPEAT ^= 1; // Arm or disarm the NOTE-REPEAT flag
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse a SEQ-SIZE press
void sizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	// Get the new value for the currently-recording-seq's size
	byte newsize = byte(clamp(0, 63, char(STATS[RECORDSEQ] & 63) + change));
	STATS[RECORDSEQ] = (STATS[RECORDSEQ] & 128) | newsize; // Modify the currently-recording seq's size
	updateFileByte(RECORDSEQ + 1, STATS[RECORDSEQ]); // Update the seq's size-byte in the song's savefile
	POS[RECORDSEQ] %= newsize << 4; // Wrap around the currently-recording seq's 16th-note-position
	TO_UPDATE = 255; // Flag all LED-rows for updating
}

// Parse a SWITCH RECORDSEQ press
void switchCmd(byte col, byte row) {
	TO_UPDATE |= 4 >> (RECORDSEQ >> 2); // Flag the previous seq's corresponding LED-row for updating
	resetSeq(RECORDSEQ); // Reset the current record-seq, which is about to become inactive
	RECORDSEQ = (PAGE * 24) + (col + (row * 4)); // Switch to the seq that corresponds to the key-position on the active page
	primeRecSeq(); // Prime the newly-entered RECORD-MODE sequence for editing
	TO_UPDATE |= 1 | (4 >> row); // Flag the top row, and the new seq's corresponding LED-row, for updating
}

// Parse a BPM-press
void tempoCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	BPM = clamp(16, 200, int(BPM) + change); // Change the BPM rate
	updateFileByte(0, BPM); // Update the BPM-byte in the song's savefile
	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a VELOCITY press
void veloCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	VELO = clamp(0, 127, int(VELO) + change); // Modify the VELO value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Store pointers to RECORD-MODE functions in PROGMEM
const CmdFunc COMMANDS[] PROGMEM = {
	genericCmd,       //  0: Unused duplicate pointer to genericCmd
	armCmd,           //  1: TOGGLE RECORDNOTES
	baseCmd,          //  2: BASENOTE
	chanCmd,          //  3: CHANNEL
	clockCmd,         //  4: CLOCK-MASTER
	controlChangeCmd, //  5: CONTROL-CHANGE
	copyCmd,          //  6: COPY
	durationCmd,      //  7: DURATION
	genericCmd,       //  8: All possible note-entry commands
	humanizeCmd,      //  9: HUMANIZE
	octaveCmd,        // 10: OCTAVE
	pasteCmd,         // 11: PASTE
	posCmd,           // 12: SHIFT CURRENT POSITION
	programChangeCmd, // 13: PROGRAM-CHANGE
	quantizeCmd,      // 14: QUANTIZE
	repeatCmd,        // 15: TOGGLE REPEAT
	sizeCmd,          // 16: SEQ-SIZE
	switchCmd,        // 17: SWITCH RECORDING-SEQUENCE
	tempoCmd,         // 18: BPM
	veloCmd           // 19: VELOCITY
};

