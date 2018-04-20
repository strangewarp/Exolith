

// Update the internal tick-size (in microseconds) to match the new BPM value
void updateTickSize() {
	TICKSIZE = (unsigned long) round(60000000L / (word(BPM) * 96));
}

// Reset every sequence, including SCATTER values
void resetAllSeqs() {
	memset(CMD, 0, sizeof(CMD) - 1);
	memset(POS, 0, sizeof(POS) - 1);
	memset(SCATTER, 0, sizeof(SCATTER) - 1);
	for (byte i = 0; i < 48; i++) {
		STATS[i] &= 63; // Reset all seqs' PLAYING flags
	}
}

// Reset a seq's cued-commands, playing-byte, and tick-position
void resetSeq(byte s) {
	CMD[s] = 0;
	POS[s] = 0;
	STATS[s] &= 63;
	//SCATTER[s] &= 15; // Wipe all of the seq's scatter-counting and scatter-flagging bits, but not its scatter-chance bits
}

// Reset the timing of all seqs and the global cue-point
void resetAllTiming() {
	CUR16 = 127; // Reset the global cue-point
	memset(POS, 0, 96); // Reset each seq's internal tick-position
}

// Compare a seq's CUE-commands to the global CUE-point, and parse them if the timing is correct
void parseCues(byte s, byte size) {

	if (
		(!CMD[s]) // If the seq has no cued commands...
		|| (CUR16 != ((CMD[s] & B11100000) >> 1)) // Or the global 16th-note doesn't correspond to the seq's cue-point...
	) { return; } // ...Exit the function

	// Enable or disable the sequence's playing-bit
	STATS[s] = (STATS[s] & B00111111) | ((CMD[s] & 2) << 6);

	// Set the sequence's internal tick to a position based on the incoming SLICE bits
	POS[s] = word(size) * ((CMD[s] & B00011100) >> 1);

	CMD[s] = 0; // Clear the sequence's CUED-COMMANDS byte

	BLINK = 127; // Start an ~8ms LED-blink
	TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
	//TO_UPDATE |= 4 << ((s % 24) >> 2); // Flag the sequence's corresponding LED-row for an update

}

// Read a tick from a given position, with an offset applied
void readTick(byte s, byte offset, byte buf[]) {
	word loc = POS[s]; // Get the sequence's current internal tick-location
	if (offset) { // If an offset was given...
		// Apply the offset to the tick-location, wrapping around the sequence's size-boundary
		loc = (loc + offset) % (word(STATS[s] & 63) * 16);
	}
	// Navigate to the note's absolute position,
	// and compensate for the fact that each tick contains 8 bytes
	file.seekSet((49UL + (loc * 8)) + (4096UL * s));
	file.read(buf, 8); // Read the data of the tick's notes
}

// Parse the contents of a given tick, and add them to the MIDI-OUT buffer and SUSTAIN buffer as appropriate
void parseTickContents(byte buf[]) {

	// For either one or both note-slots on this tick, depending on how many are filled...
	for (byte bn = 0; bn < ((buf[4] || buf[6]) + 4); bn += 4) {

		if (MOUT_COUNT == 8) { return; } // If the MIDI buffer is full, exit the function

		// Convert the note's CHANNEL byte into either a NOTE, CC, or PC command
		buf[bn] = 144 + (buf[bn] & 15) + ((buf[bn] & 48) << 1);

		memcpy(MOUT + (MOUT_COUNT * 3), buf + bn, 3); // Copy the note into the MIDI buffer's lowest empty MOUT location
		MOUT_COUNT++; // Increase the counter that tracks the number of bytes in the MIDI buffer

		if (buf[bn] >= 176) { continue; } // If this was a CC or PC command, forego any sustain mechanisms

		buf[bn] -= 16; // Turn the NOTE-ON into a NOTE-OFF
		buf[bn + 2] = buf[bn + 3]; // Move DURATION to the next byte over, for the 3-byte sustain-storage format

		clipBottomSustain(); // If the bottommost SUSTAIN-slot is full, send its NOTE-OFF prematurely
		memmove(SUST + 3, SUST, SUST_COUNT * 3); // Move all sustains one space downward
		memcpy(SUST, buf + bn, 3); // Create a new sustain corresponding to this note
		SUST_COUNT++; // Increase the number of active sustains by 1

		if (!RECORDMODE) { // If we're in PLAY-MODE...
			TO_UPDATE |= 2; // Flag the sustain-row for a GUI update
		}

	}

}

// Parse any active SCATTER values that might be associated with a given seq
void parseScatter(byte s, byte didscatter) {

	if (didscatter) { // If this tick was a SCATTER-tick...
		SCATTER[s] &= 15; // Unset the "distance" side of this seq's SCATTER byte
	} else { // Else, if this wasn't a SCATTER-tick...

		// If this tick's random-value doesn't exceed the seq's SCATTER-chance value, exit the function
		if ((GLOBALRAND & 15) >= SCATTER[s]) { return; }

		byte rnd = (GLOBALRAND >> 4) & 255; // Get 8 random bits

		if (!(rnd & 224)) { // If a 1-in-4 chance occurs...
			SCATTER[s] |= 128; // Set the seq's SCATTER-distance to be a whole-note
			return; // Exit the function
		}

		// Set the seq's SCATTER-distance to contain:
		// Half-note: 1/2 of the time
		// Quarter-note: 1/2 of the time
		// Eighth-note: 1/4 of the time
		byte dist = (rnd & 6) | (!(rnd & 24));
		if (!dist) { // If the new SCATTER-distance doesn't contain anything...
			SCATTER[s] |= 32; // Set the SCATTER-distance to a quarter-note
		} else { // Else, if the new SCATTER-distance contains something...
			SCATTER[s] |= dist << 4; // Make it into the seq's stored SCATTER-distance value
		}

	}

}

// Process a REPEAT or REPEAT-RECORD command, and send its resulting button-key to processRecAction()
void processRepeats(byte ctrl, unsigned long held, byte s) {

	if (
		(!(POS[s] % QUANTIZE)) // If this tick's position is a multiple of the QUANTIZE value...
		&& isRecCompatible(ctrl) // And the current keychord signifies a command is to be recorded...
	) { // Then this is a valid REPEAT position. So...

		for (byte row = 0; row < 6; row++) { // For every row...
			byte i = row; // Create a new value that will check all BUTTONS-formatted button-bits in order
			for (byte col = 0; col < 4; col++) { // For every column...
				if (held & (1UL << i)) { // If the internal BUTTONS-value for this note is held...
					processRecAction((row * 4) + col); // Process the note-key
				}
				i += 6; // Increment this row-loop's BUTTON-bit counter to the next note
			}
		}

	}

}

// Get the notes from the current tick in a given seq, and add them to the MIDI-OUT buffer
void getTickNotes(byte ctrl, unsigned long held, byte s, byte buf[]) {

	byte didscatter = 0; // Flag that tracks whether this tick has had a SCATTER effect

	if (RECORDMODE) { // If RECORD MODE is active...
		if (RECORDSEQ == s) { // If this is the current RECORDSEQ...
			if (REPEAT && held) { // If REPEAT is toggled, and a note-button is being held...
				processRepeats(ctrl, held, s); // Process a REPEAT-RECORD
			} else if (RECORDNOTES && (ctrl == B00111100)) { // Else, if RECORDNOTES is armed, and ERASE-NOTES is held...
				byte b[5]; // Make a buffer the size of a tick...
				memset(b, 0, 4); // ...And clear it of any junk-data
				writeData( // Write the blank note to the savefile
					(49UL + (POS[RECORDSEQ] * 8)) + (4096UL * RECORDSEQ) + (TRACK * 4), // Write at the RECORDSEQ's current position
					4, // Write 4 bytes of data
					b, // Use the empty buffer that was just created
					1 // Only overwrite notes that match the global CHAN
				);
			}
		}
		readTick(s, 0, buf); // Read the tick with no offset
	} else { // Else, if RECORD MODE is inactive...
		readTick(s, (SCATTER[s] & 240) >> 3, buf); // Read the tick with SCATTER-offset
		didscatter = 1;
	}

	// If the MIDI-OUT queue is full, or the tick contains no commands, then exit the function
	if ((MOUT_COUNT == 8) || (!(buf[0] || buf[2]))) { return; }

	// If the function hasn't exited by this point, then that means this tick contained a note. So...
	parseTickContents(buf); // Check the tick's contents, and add them to MIDI-OUT and SUSTAIN where appropriate
	parseScatter(s, didscatter); // Parse any active SCATTER values that might be associated with the seq

}

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	if (PLAYING) { // If the sequencer is currently in PLAYING mode...

		byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity
		unsigned long held = BUTTONS >> 6; // Get the note-buttons that are currently held

		byte buf[9]; // Buffer for reading note-events from the datafile

		for (byte i = 47; i != 255; i--) { // For every loaded sequence, in reverse order...

			byte size = STATS[i] & 63; // Get seq's absolute size, in beats

			parseCues(i, size); // Parse a sequence's cued commands, if any

			// If the seq isn't currently playing, go to the next seq's iteration-routine
			if (!(STATS[i] & 128)) { continue; }

			memset(buf, 0, 8); // Clear the buffer's RAM of junk data

			// Get the notes from this tick in a given seq, and add them to the MIDI-OUT buffer
			getTickNotes(ctrl, held, i, buf);

			// Increase the seq's 16th-note position by one increment, wrapping it around its top limit
			POS[i] = (POS[i] + 1) % (word(size) << 4);

		}

	}

	if (MOUT_COUNT) { // If there are any commands in the NOTE-ON buffer...
		Serial.write(MOUT, MOUT_COUNT * 3); // Send all outgoing MIDI-command bytes at once
		MOUT_COUNT = 0; // Clear the MIDI buffer's counting-byte
	}

	processSustains(); // Process one 16th-note's worth of duration for all sustained notes

}

