

// Arm or disarm RECORDNOTES
void armCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	RECORDNOTES ^= 1; // Arm or disarm the RECORDNOTES flag
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse an ARPEGGIATOR MODE press
void arpModeCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	ARPMODE = (ARPMODE + 1) % 3; // Toggle the ARPEGGIATOR MODE
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CHAN press
void chanCmd(byte col, byte row) {
	// Modify the CHAN value, keeping it within the range of valid/supported commands
	CHAN = min(239, (CHAN & 240) | applyChange(CHAN & 15, toChange(col, row), 0, 15));
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a CLEAR NOTES press:
// Clear a given number of beats' worth of notes, from the currently-active track in the current RECORDSEQ.
void clearCmd(byte col, byte row) {

	byte buf[5]; // Create a note-sized data buffer...
	memset(buf, 0, 4); // ...And clear it of any junk-data

	byte len = STATS[RECORDSEQ] & 63; // Get the RECORDSEQ's length, in beats
	word blen = word(len) * 128; // Get the RECORDSEQ's length, in bytes

	unsigned long rspos = FILE_BODY_START + (FILE_SEQ_BYTES * RECORDSEQ); // Get the RECORDSEQ's absolute data-position
	word pos = (POS[RECORDSEQ] >> 4) << 4; // Get the bottom-point of the current beat in RECORDSEQ
	byte t4 = TRACK * 4; // Get a bitwise offset based on whether track 1 or 2 is active

	for (word i = 0; i < (word(min(abs(toChange(col, row)), len)) * 128); i += 8) { // For every 32nd-note in the clear-area...
		writeCommands(rspos + ((pos + i + t4) % blen), 4, buf, 1); // Overwrite it if it matches the TRACK and CHAN
	}

	file.sync(); // Sync any still-buffered data to the savefile

	BLINKL = (!TRACK) * 255; // Start a TRACK-linked LED-BLINK that is ~16ms long
	BLINKR = TRACK * 255; // ^
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates

}

// Parse a CLOCK-MASTER press
void clockCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	CLOCKMASTER ^= 1; // Toggle the CLOCK-MASTER value
	ABSOLUTETIME = micros(); // Set the ABSOLUTETIME-tracking var to now
	ELAPSED = 0; // Set the ELAPSED value to show that no time has elapsed since the last tick-check
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a COPY press
void copyCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	COPYPOS = POS[RECORDSEQ] - (POS[RECORDSEQ] % 16); // Set the COPY-position to the start of the beat
	COPYSEQ = RECORDSEQ; // Set the COPY-seq to the curent RECORD-seq
	BLINKL = 255; // Start an LED-BLINK that is ~16ms long
	BLINKR = 255; // ^
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
}

// Parse a DURATION press
void durationCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	DURATION = applyChange(DURATION, change, 0, 129); // Modify the DURATION value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse all of the possible actions that signal the recording of commands
void genericCmd(byte col, byte row) {
	arpPress(); // Parse a new keypress in the arpeggiation-system
	if (REPEAT) { // If REPEAT is active...
		RPTVELO = VELO; // Reset the REPEAT-VELOCITY tracking var to be equal to the user-defined VELOCITY amount
		return; // Exit the function, since no commands should be sent instantly
	} else { // Else, if REPEAT isn't active...
		if (DURATION == 129) { // If DURATION is in manual-mode...
			if (KEYFLAG) { // If another key is already held in manual-mode...
				recordHeldNote(); // Record that held key's note
			}
			setKeyNote(col, row); // Set a held-recording-note for the given button-position
		} else { // Else, if DURATION is in auto-mode...
			processRecAction(modKeyPitch(col, row)); // Parse the key as a recording-action into the current TRACK
		}
	}
}

// Parse a GRID-CONFIG press
void gridConfigCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	GRIDCONFIG = applyChange(GRIDCONFIG, change, 0, GRID_TOTAL); // Modify the GRIDCONFIG value
	TO_UPDATE |= 1; // Flag the topmost row for updating
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

	unsigned long copybase = FILE_BODY_START + (word(COPYSEQ) * FILE_SEQ_BYTES); // Get the literal position of the copy-seq
	unsigned long pastebase = FILE_BODY_START + (word(RECORDSEQ) * FILE_SEQ_BYTES); // Get the literal position of the paste-seq

	byte clen = 128 >> row; // Get the length of the copy/paste, in quarter-notes

	byte csize = STATS[COPYSEQ] & 63; // Get the copy-seq's size, in beats
	byte psize = STATS[RECORDSEQ] & 63; // Get the paste-seq's size, in beats

	byte pstart = (POS[RECORDSEQ] - (POS[RECORDSEQ] % 16)) >> 4; // Get the start of the paste-location, in beats

	for (byte filled = 0; filled < clen; filled++) { // For every quarter-note in the copypaste's area...

		file.seekSet(copybase + (word((COPYPOS + filled) % csize) * 32)); // Navigate to the current copy-position
		file.read(b1, 32); // Read the copy-data

		writeCommands( // Write this chunk of copy-data to the corresponding paste-area of the file
			pastebase + (word((pstart + filled) % psize) * 32), // Bitwise file-position for quarter-note's paste-location
			32, // Size of the data-buffer, in bytes
			b1, // The data-buffer itself
			0 // Apply this to notes on all channels
		);

	}

	file.sync(); // Sync any still-buffered data to the savefile

	BLINKL = 255; // Start an LED-BLINK that is ~16ms long
	BLINKR = 255; // ^
	TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates

}

// Parse a SHIFT RECORDING POSITION press
void posCmd(byte col, byte row) {

	int change = int(toChange(col, row)) * 16; // Convert a column and row into a CHANGE value, in 32nd-notes

	CUR32 = byte(word(((int(CUR32) + change) % 128) + 128) % 128); // Shift the global cue-point, wrapping in either direction

	for (byte seq = 0; seq < 48; seq++) { // For each seq...
		if (STATS[seq] & 128) { // If the seq is playing...
			word size = (STATS[seq] & 63) * 16; // Get the seq's size, in 32nd-notes
			// Shift the seq's position, wrapping in either direction
			POS[seq] = word(((long(POS[seq]) + change) % size) + size) % size;
		}
	}

	TO_UPDATE |= 3; // Flag the top two rows for LED updates

}

// Parse a QRESET press
void qrstCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	QRESET = applyChange(QRESET, change, 0, 128); // Modify the QRESET value
	TO_UPDATE |= 1; // Flag the topmost row for updating
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

// Parse a REPEAT-SWEEP press
void rSweepCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	RPTSWEEP = applyChange(RPTSWEEP, change, 0, 255); // Modify the RPTSWEEP value by the CHANGE amount
	TO_UPDATE |= 253; // Flag the top row, and bottom 6 rows, for LED updates
}

// Parse a SEQ-SIZE press
void sizeCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	// Get the new value for the currently-recording-seq's size
	byte newsize = applyChange(STATS[RECORDSEQ] & 63, change, 1, 32);
	STATS[RECORDSEQ] = 128 | newsize; // Modify the currently-recording seq's size
	resetAllTiming(); // Reset the timing of all seqs and the global cue-point
	TO_UPDATE |= 3; // Flag top two rows for updating
}

// Parse a SWING AMOUNT press
void swAmtCmd(byte col, byte row) {
	SAMOUNT = applyChange(SAMOUNT, toChange(col, row), 0, 128); // Apply the column-and-row's CHANGE value to the SAMOUNT value
	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a SWING GRANULARITY press
void swGranCmd(byte col, byte row) {
	SGRAN = applyChange(SGRAN, toChange(col, row), 1, 4); // Apply the column-and-row's CHANGE value to the SGRAN value
	updateSwingPart(); // Update the SWING-PART var based on the current SWING GRANULARITY and CUR32 tick
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a SWITCH RECORDSEQ press
void switchCmd(byte col, byte row) {
	updateSeqSize(); // Update the current seq's size-byte in the savefile, if applicable
	TO_UPDATE |= 3 | (4 >> (RECORDSEQ >> 2)); // Flag the old seq's LED-row for updating, plus the top two rows
	resetSeq(RECORDSEQ); // Reset the current record-seq, which is about to become inactive
	RECORDSEQ = (PAGE * 24) + col + (row * 4); // Switch to the seq that corresponds to the key-position on the active page
	resetAllTiming(); // Reset the timing of all seqs and the global cue-point
}

// Parse a BPM-press
void tempoCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	BPM = applyChange(BPM, change, BPM_LIMIT_LOW, BPM_LIMIT_HIGH); // Change the BPM rate
	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a TRACK-press
void trackCmd(__attribute__((unused)) byte col, __attribute__((unused)) byte row) {
	TRACK = !TRACK; // Toggle between tracks 1 and 2
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse an UPPER COMMAND BITS press
void upperBitsCmd(byte col, byte row) {
	// Convert col/row into a CHANGE val, bounded to +/- 16, 32, 64
	char change = max(-8, min(8, toChange(col, row))) << 4;
	// Apply that change-interval to the CHAN value, bounded to valid commands
	CHAN = (CHAN & 15) | applyChange(CHAN & 240, change, UPPER_BITS_LOW, UPPER_BITS_HIGH);
	TO_UPDATE |= 1; // Flag the topmost row for updating
}

// Parse a VELOCITY press
void veloCmd(byte col, byte row) {
	char change = toChange(col, row); // Convert a column and row into a CHANGE value
	VELO = applyChange(VELO, change, 0, 127); // Modify the VELO value
	TO_UPDATE |= 1; // Flag the topmost row for updating
}
