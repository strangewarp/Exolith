
// Arm or disarm RECORDNOTES
void armCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	RECORDNOTES ^= 1; // Arm or disarm the RECORDNOTES flag
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse a BASENOTE press
void baseCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	BASENOTE = applyChange(BASENOTE, change, 0, 11); // Modify the BASENOTE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CHAN press
void chanCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	CHAN = (CHAN & 48) | applyChange(CHAN & 15, change, 0, 15); // Modify the CHAN value, while preserving CC & PC flags
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CLEAR NOTES press
void clearCmd(byte col, byte row) {

	byte len = STATS[RECORDSEQ] & 63; // Get the RECORDSEQ's length, in beats
	word flen = word(len) * 16; // Get the RECORDSEQ's length, in 16th-notes

	// Convert a column and row into a CHANGE value (positive values only), bounded to the seq's size (in 16th-notes)
	word amount = min(abs(toChange(col, row)), len) * 16;

	byte buf[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Create a blanked-out buffer for use with eraseTick()

	word pos = (POS[RECORDSEQ] >> 4) << 4; // Get the bottom-point of the current beat in RECORDSEQ
	while (amount) { // While there is still some amount of space left to clear...
		eraseTick(buf, pos); // Erase all notes within this tick that correspond to the global CHAN
		pos = (pos + 1) % flen; // Increase the tick-position by 1, wrapped around the RECORDSEQ's size
		amount--; // Reduce the amount-of-ticks-remaining-value by 1
	}

	BLINK = 255; // Start an LED-BLINK that is ~16ms long
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates

}

// Parse a CLOCK-MASTER press
void clockCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	CLOCKMASTER ^= 1; // Toggle the CLOCK-MASTER value
	ABSOLUTETIME = micros(); // Set the ABSOLUTETIME-tracking var to now
	ELAPSED = 0; // Set the ELAPSED value to show that no time has elapsed since the last tick-check
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CONTROL-CHANGE press
void controlChangeCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	CHAN ^= 16; // Flip the CHAN bit that turns all NOTE command-entry into CC command-entry
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a COPY press
void copyCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	COPYPOS = POS[RECORDSEQ] - (POS[RECORDSEQ] % 16); // Set the COPY-position to the start of the beat
	COPYSEQ = RECORDSEQ; // Set the COPY-seq to the curent RECORD-seq
	BLINK = 255; // Start an LED-BLINK that is ~16ms long
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse a DURATION press
void durationCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	DURATION = applyChange(DURATION, change, 0, 255); // Modify the DURATION value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse all of the possible actions that signal the recording of commands
void genericCmd(byte col, byte row) {
	if (REPEAT) { return; } // If REPEAT is held, don't send any commands instantly
	byte key = col + (row * 4); // Get the key at the intersection of the column and row
	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity
	processRecAction(ctrl, key); // Parse all of the possible actions that signal the recording of commands
}

// Parse a HUMANIZE press
void humanizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	HUMANIZE = applyChange(HUMANIZE, change, 0, 127); // Modify the HUMANIZE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// All INTERVAL commands go directly to the generic note-handler command,
// and only exist independently because the KEYTAB array holds unique identifiers for each of them for GUI use.
void iDownCmd(byte col, byte row) { genericCmd(col, row); }
void iDownRandCmd(byte col, byte row) { genericCmd(col, row); }
void iUpCmd(byte col, byte row) { genericCmd(col, row); }
void iUpRandCmd(byte col, byte row) { genericCmd(col, row); }

// Parse an OCTAVE press
void octaveCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	OCTAVE = applyChange(OCTAVE, change, 0, 10); // Modify the OCTAVE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a PASTE press
void pasteCmd(__attribute__((unused)) byte col, byte row) {

	byte buf[33]; // Create a quarter-note-sized data buffer...
	memset(buf, 0, 32); // ...And clear it of any junk data

	unsigned long copybase = 49UL + (word(COPYSEQ) * 4096); // Get the literal position of the copy-seq
	unsigned long pastebase = 49UL + (word(RECORDSEQ) * 4096); // Get the literal position of the paste-seq

	byte clen = 128 >> row; // Get the length of the copy/paste, in quarter-notes

	byte csize = STATS[COPYSEQ] & 63; // Get the copy-seq's size, in beats
	byte psize = STATS[RECORDSEQ] & 63; // Get the paste-seq's size, in beats

	byte pstart = (POS[RECORDSEQ] - (POS[RECORDSEQ] % 16)) >> 4; // Get the start of the paste-location, in beats

	for (byte filled = 0; filled < clen; filled++) { // For every quarter-note in the copypaste's area...

		byte cpos = (COPYPOS + filled) % csize; // Wrap the copy-range around the end of its seq
		byte ppos = (pstart + filled) % psize; // ^ Same, for paste-range

		file.seekSet(copybase + (word(cpos) * 32)); // Navigate to the current copy-position
		file.read(buf, 32); // Read the copy-data
		file.seekSet(pastebase + (word(ppos) * 32)); // Navigate to the current paste-position
		file.write(buf, 32); // Write the paste-data

		file.sync(); // Sync the SD-card's data after every 32 bytes of read/write

	}

	BLINK = 255; // Start an LED-BLINK that is ~16ms long
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates

}

// Parse a SHIFT CURRENT POSITION press
void posCmd(byte col, byte row) {

	char change = toChange(col, row); // Convert a column and row into a CHANGE value

	CUR16 = (((int(CUR16) + change) % 128) + 128) % 128; // Shift the global cue-point, wrapping in either direction

	for (byte seq = 0; seq < 48; seq++) { // For each seq...
		if (STATS[seq] & 128) { // If the seq is playing...
			word size = (STATS[seq] & 63) * 16; // Get the seq's size, in 16th-notes
			// Shift the seq's position, wrapping in either direction
			POS[seq] = (((int(POS[seq]) + change) % size) + size) % size;
		}
	}

	BLINK = 255; // Start an LED-BLINK that is ~16ms long
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates

}

// Parse a PROGRAM-CHANGE press
void programChangeCmd(byte col, byte row) {

	(void)(col); // todo remove these after writing the function
	(void)(row);



}

// Parse a QUANTIZE press
void quantizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	QUANTIZE = applyChange(QUANTIZE, change, 1, 16); // Modify the QUANTIZE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a REPEAT press
void repeatCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	REPEAT ^= 1; // Arm or disarm the NOTE-REPEAT flag
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse a SEQ-SIZE press
void sizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	// Get the new value for the currently-recording-seq's size
	byte newsize = applyChange(STATS[RECORDSEQ] & 63, change, 1, 32);
	STATS[RECORDSEQ] = (STATS[RECORDSEQ] & 128) | newsize; // Modify the currently-recording seq's size
	updateFileByte(RECORDSEQ + 1, newsize); // Update the seq's size-byte in the song's savefile
	POS[RECORDSEQ] %= word(newsize) << 4; // Wrap around the resized seq's 16th-note-position
	TO_UPDATE = 3; // Flag top two LED-rows for updating
}

// Parse a SWITCH RECORDSEQ press
void switchCmd(byte col, byte row) {
	TO_UPDATE |= 4 >> (RECORDSEQ >> 2); // Flag the previous seq's corresponding LED-row for updating
	resetSeq(RECORDSEQ); // Reset the current record-seq, which is about to become inactive
	RECORDSEQ = (PAGE * 24) + (col + (row * 4)); // Switch to the seq that corresponds to the key-position on the active page
	primeRecSeq(); // Prime the newly-entered RECORD-MODE sequence for editing
	TO_UPDATE |= 1 | (4 << row); // Flag the top row, and the new seq's corresponding LED-row, for updating
}

// Parse a BPM-press
void tempoCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	BPM = applyChange(BPM, change, 16, 200); // Change the BPM rate
	updateFileByte(0, BPM); // Update the BPM-byte in the song's savefile
	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a VELOCITY press
void veloCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	VELO = applyChange(VELO, change, 0, 127); // Modify the VELO value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

