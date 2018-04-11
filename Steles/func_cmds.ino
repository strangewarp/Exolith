

// Arm or disarm RECORDNOTES
void armCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	RECORDNOTES ^= 1; // Arm or disarm the RECORDNOTES flag
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
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

	// Convert a column and row into a CHANGE value (positive values only), bounded to the seq-data's size (in quarter-notes)
	byte amount = min(abs(toChange(col, row)), len) * 4;

	byte buf[33]; // Create a quarter-note-sized data buffer...
	memset(buf, 0, 32); // ...And clear it of any junk-data

	word pos = (POS[RECORDSEQ] >> 4) << 4; // Get the bottom-point of the current beat in RECORDSEQ

	while (amount) { // While there is still some amount of space left to clear...
		writeData( // Write data to the sequence...
			(49UL + (pos * 32)) + (4096UL * RECORDSEQ), // Current quarter-note's data position
			32, // Size of the data to replace
			b, // The empty data-buffer
			1 // Only clear notes that match the global CHAN
		);
		pos = (pos + 1) % flen; // Increase the tick-position by 1, wrapped around the RECORDSEQ's size
		amount--; // Reduce the amount-of-quarter-notes-remaining-value by 1
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
	if (REPEAT) { return; } // If REPEAT is held, exit the function, since no commands should be sent instantly
	processRecAction((row * 4) + col); // Parse the key as a recording-action
}

// Parse a HUMANIZE press
void humanizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	HUMANIZE = applyChange(HUMANIZE, change, 0, 127); // Modify the HUMANIZE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse an OCTAVE press
void octaveCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	OCTAVE = applyChange(OCTAVE, change, 0, 10); // Modify the OCTAVE value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a PASTE press
void pasteCmd(__attribute__((unused)) byte col, byte row) {

	byte b1[33]; // Create a quarter-note-sized copy-data buffer...
	memset(b1, 0, 32); // ...And clear it of any junk-data

	unsigned long copybase = 49UL + (word(COPYSEQ) * 4096); // Get the literal position of the copy-seq
	unsigned long pastebase = 49UL + (word(RECORDSEQ) * 4096); // Get the literal position of the paste-seq

	byte clen = 128 >> row; // Get the length of the copy/paste, in quarter-notes

	byte csize = STATS[COPYSEQ] & 63; // Get the copy-seq's size, in beats
	byte psize = STATS[RECORDSEQ] & 63; // Get the paste-seq's size, in beats

	byte pstart = (POS[RECORDSEQ] - (POS[RECORDSEQ] % 16)) >> 4; // Get the start of the paste-location, in beats

	for (byte filled = 0; filled < clen; filled++) { // For every quarter-note in the copypaste's area...

		file.seekSet(copybase + (word((COPYPOS + filled) % csize) * 32)); // Navigate to the current copy-position
		file.read(b1, 32); // Read the copy-data

		writeData( // Write this chunk of copy-data to the corresponding paste-area of the file
			pastebase + (word((pstart + filled) % psize) * 32), // Bitwise file-position for quarter-note's paste-location
			32, // Size of the data-buffer, in bytes
			b1, // The data-buffer itself
			0 // Apply this to notes on all channels
		);

	}

	BLINK = 255; // Start an LED-BLINK that is ~16ms long
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates

}

// Parse a SHIFT CURRENT POSITION press
void posCmd(byte col, byte row) {

	int change = int(toChange(col, row)) * 16; // Convert a column and row into a CHANGE value, in 16th-notes

	CUR16 = byte(word(((int(CUR16) + change) % 128) + 128) % 128); // Shift the global cue-point, wrapping in either direction

	for (byte seq = 0; seq < 48; seq++) { // For each seq...
		if (STATS[seq] & 128) { // If the seq is playing...
			word size = (STATS[seq] & 63) * 16; // Get the seq's size, in 16th-notes
			// Shift the seq's position, wrapping in either direction
			POS[seq] = word(((long(POS[seq]) + change) % size) + size) % size;
		}
	}

	/*
	BLINK = 255; // Start an LED-BLINK that is ~16ms long
	TO_UPDATE = 255; // Flag all rows for LED updates
	*/

	TO_UPDATE |= 3; // Flag the top two rows for LED updates

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
	STATS[RECORDSEQ] = 128 | newsize; // Modify the currently-recording seq's size
	updateFileByte(RECORDSEQ + 1, newsize); // Update the seq's size-byte in the song's savefile
	resetAllTiming(); // Reset the timing of all seqs and the global cue-point
	TO_UPDATE |= 3; // Flag top two rows for updating
}

// Parse a SWITCH RECORDSEQ press
void switchCmd(byte col, byte row) {
	TO_UPDATE |= 4 >> (RECORDSEQ >> 2); // Flag the previous seq's corresponding LED-row for updating
	resetSeq(RECORDSEQ); // Reset the current record-seq, which is about to become inactive
	RECORDSEQ = (PAGE * 24) + col + (row * 4); // Switch to the seq that corresponds to the key-position on the active page
	resetAllTiming(); // Reset the timing of all seqs and the global cue-point
	TO_UPDATE |= 3; // Flag the top two rows for updating
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

