
// Reset every sequence
void resetSeqs() {
	for (byte i = 0; i < 72; i++) {
		resetSeq(i);
	}
}

// Reset a seq's cued-commands, playing-byte, and tick-position
void resetSeq(byte s) {
	SEQ_CMD[s] = 0;
	SEQ_POS[s] = 0;
}

// Send MIDI-OFF commands for all currently-sustained notes
void haltAllSustains() {
	for (byte i = 0; i < 8; i++) {
		if (SUSTAIN[i][2] > 0) {
			Serial.write(128 + SUSTAIN[i][0]);
			Serial.write(SUSTAIN[i][1]);
			Serial.write(127);
			SUSTAIN[i][0] = 0;
			SUSTAIN[i][1] = 0;
			SUSTAIN[i][2] = 0;
		}
	}
}

// Halt a single sustain in the sustain-system
void haltSustain(byte num) {

	// Send a NOTE-OFF to the MIDI-OUT port
	Serial.write(128 + SUSTAIN[num][0]);
	Serial.write(SUSTAIN[num][1]);
	Serial.write(127);

	// Shift every sustain-note underneath this note upwards by one slot
	for (byte i = num; i < 7; i++) {
		byte i2 = i + 1;
		if (SUSTAIN[i2][0] == 255) { // If the next slot is empty...
			SUSTAIN[i][0] = 255; // Empty the current slot
			SUSTAIN[i][1] = 255;
			SUSTAIN[i][2] = 255;
			break; // Stop checking for filled sustains, as the rest are empty
		}
		memcpy(SUSTAIN[i], SUSTAIN[i2], 3); // Copy the next-lowest slot into the current slot
	}

}

// Process one tick's worth of duration for all notes in the SUSTAIN system
void processSustains() {
	for (byte n = 0; n < 8; n++) { // For each sustain-slot...
		if (SUSTAIN[n][0] == 255) { break; } // If the slot is empty, then don't check any lower slots
		if (SUSTAIN[n][2] == 0) { // If the sustain-slot's note has a remaining duration of 0...
			haltSustain(n); // Halt the slot's sustain
			n--; // Compensate for the shifting-upwards of subsequent sustain-slots that haltSustain() performs
			continue; // Skip to the loop's next iteration
		}
		SUSTAIN[n][2]--; // Reduce the sustain-slot's duration-value by 1 tick
	}
}

// Play a given MIDI note, adding a corresponding note to the SUSTAIN system if applicable
void playNote(byte dur, byte size, byte b1, byte b2, byte b3) {

	// Split the MIDI CHANNEL off from its command-type
	byte chan = b1 % 16;
	byte cmd = b1 - chan;

	if (cmd == 144) { // If this is a NOTE-ON cmmand...

		// If the lowest sustain-slot is filled, then halt its note and empty it
		if (SUSTAIN[7][0] < 255) {
			haltSustain(7);
		}

		// For every currently-filled sustain-slot, move its contents downward by one slot
		for (byte i = 0; i < 7; i++) {
			if (SUSTAIN[i][0] < 255) { break; } // If the current slot is empty, stop shifting slots downward
			memcpy(SUSTAIN[i + 1], SUSTAIN[i], 3);
		}

		// Put the note's sustain-value in the top sustain-slot
		SUSTAIN[0][0] = chan;
		SUSTAIN[0][1] = b2;
		SUSTAIN[0][2] = dur;

	}

	// Send the MIDI command to the MIDI-OUT port
	Serial.write(b1);
	Serial.write(b2);
	if (size == 3) { // If the command contains three bytes...
		Serial.write(b3); // Write the third byte
	}


}

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	processSustains(); // Process one tick's worth of duration for all sustained notes

	if (!PLAYING) { return; } // If the sequencer isn't currently toggled to PLAYING mode, ignore this iteration

	byte note[5]; // Var to hold incoming 4-byte note values from the tempfile: dur, chan, pitch, velo

	file.open(string(SONG) + ".tmp", O_RDRW); // Open the tempfile, for reading and writing

	for (byte i = 0; i < 72; i++) { // For every currently-loaded sequence...

		word size = SEQ_SIZE[i] * 96; // Get seq's absolute size, in ticks
		word csize = size >> 3; // Get the number of ticks within each of the seq's slices

		if (SEQ_CMD[i]) { // If the seq has cued commands...
			byte slice = (SEQ_CMD[i] & B00011100) >> 2; // Get seq's slice-point
			if (CURTICK == ((((SEQ_CMD[i] & B11100000) >> 5) * 96) % 768)) { // If the global tick corresponds to the seq's cue-point...
				// Enable or disable the sequence's playing-bit, and set the sequence's internal tick to a position, based on the incoming ON/OFF bit and SLICE bits
				SEQ_POS[i] =
					(32768 | (csize * slice))
					* ((SEQ_CMD[i] & 3) >> 1);
			}
		}

		// If the seq isn't currently playing, go to the next seq's iteration-routine
		if (SEQ_POS < 32768) { continue; }

		// Get a version of the seq's tick-position, without its activity-bit,
		// increase the position by 1, wrapping at its upper size boundary;
		// and then set that to the sequence's global position variable,
		// along with an active playing-bit.
		word maskpos = ((SEQ_POS[i] & 32767) + 1) % size;
		SEQ_POS[i] = 32768 | maskpos;

		// If RECORD MODE is active, and the ERASE-NOTES command is being held,
		// and notes are being recorded into this seq...
		if (RECORDMODE && ERASENOTES && (RECORDNOTES == i)) {
			file.seekSet(73 + maskpos + (i * 98304)); // Set position to start of tick's first note
			file.write(EMPTY_TICK, 16) // Write in an entire empty tick's worth of bytes
			continue; // Skip the remaining read-and-play routines for this sequence
		}

		for (byte n = 0; n < 4; n++) { // For every note-slot in the tick...
			file.seekSet(73 + maskpos + (n * 4) + (i * 98304)); // Read the note from its file-position
			file.read(note, 4); // Read the data of the tick's first note
			if (note[1]) { // If the note exists...
				playNote(note[0], 3, note[1], note[2], note[3]); // Play the note
			}
		}

	}

	file.close(); // Close the tempfile, having done all reads for all seqs on this tick

	CURTICK = (CURTICK + 1) % 768; // Advance the global current-tick, bounded to the global slice-size
	TO_UPDATE |= 1 >> (CURTICK % 96); // If the global cue-timer advanced by a chunk, cue the global-tick-row for a GUI update

}
