

// Reset every sequence, including SCATTER values
void resetAllSeqs() {
	memset(CMD, 0, 48);
	memset(POS, 0, 96);
	memset(SCATTER, 0, 48);
	for (byte i = 0; i < 48; i++) {
		STATS[i] &= 63; // Reset all seqs' PLAYING flags
	}
}

// Reset a seq's cued-commands, playing-flag, and tick-position
void resetSeq(byte s) {
	CMD[s] = 0;
	POS[s] = 0;
	STATS[s] &= 63;
}

// Send a MIDI-CLOCK reset command to MIDI-OUT
void sendClockReset() {
	byte spos[6] = { // Create a series of commands to stop, adjust, and restart any devices downstream:
		252, // STOP
		242, 0, 0, // SONG-POSITION POINTER: beginning of every sequence
		250, // START
		0 // (Empty array entry)
	};
	Serial.write(spos, 5); // Send the series of commands to the MIDI-OUT circuit
	BLINKL = 255; // Start a relatively-long LED-blink
	BLINKR = 255; // ^
}

// Reset all timing of all seqs and the global cue-point, and send a SONG-POSITION POINTER
void resetAllTiming() {
	TICKCOUNT = 2; // Set the TICK-counter to lapse into a new 32nd-note on the next tick
	CUR32 = 255; // Reset the global cue-point
	memset(POS, 0, 96); // Reset each seq's internal tick-position
	sendClockReset(); // Send a MIDI-CLOCK reset command to MIDI-OUT
}

// Compare a seq's CUE-commands to the global CUE-point, and parse them if the timing is correct
void parseCues(byte s, byte size) {

	if (CMD[s]) { // If the sequence has any cued commands...

		if (CUR32 == (CMD[s] & B11100000)) { // If the global 32nd-note corresponds to the seq's cue-point...

			// Enable or disable the sequence's playing-bit
			STATS[s] = (STATS[s] & 63) | ((CMD[s] & 2) << 6);

			// Set the sequence's internal tick to a position based on the incoming SLICE bits
			POS[s] = word(size) * (CMD[s] & B00011100);

			CMD[s] = 0; // Clear the sequence's CUED-COMMANDS byte

		} else if ((CUR32 % 8) >= 2) { // Else, the cue-command is still dormant. So if the global tick isn't on an eighth-note or the 32nd-note immediately afterward...
			return; // Exit the function without flagging any TO_UPDATE rows
		}

		TO_UPDATE |= 4 << ((s % 24) >> 2); // Flag the sequence's corresponding LED-row for an update

	}

}

// Read a tick from a given position, with an offset applied
void readTick(byte s, byte offset, byte buf[]) {
	word loc = POS[s]; // Get the sequence's current internal tick-location
	if (offset) { // If an offset was given...
		// Apply the offset to the tick-location, wrapping around the sequence's size-boundary
		loc = (loc + offset) % (word(STATS[s] & 63) * 32);
	}
	// Navigate to the note's absolute position,
	// and compensate for the fact that each tick contains 8 bytes
	file.seekSet((FILE_BODY_START + (loc * 8)) + (FILE_SEQ_BYTES * s));
	file.read(buf, 8); // Read the data of the tick's notes
}

// Parse the contents of a given tick, and add them to the MIDI-OUT buffer and SUSTAIN buffer as appropriate
void parseTickContents(byte s, byte buf[]) {

	for (byte bn = 0; bn < 8; bn += 4) { // For both event-slots on this tick...

		if (!buf[bn]) { continue; } // If this slot doesn't contain a command, then check the next slot or exit the loop

		// If this is the RECORDSEQ, in RECORD MODE, and RECORDNOTES isn't armed, and a command is present on this tick...
		if ((RECORDMODE) && (RECORDSEQ == s) && (!RECORDNOTES)) {
			BLINKL |= (!bn) * 192; // Start or continue a TRACK-activity-linked LED-BLINK that is ~12ms long
			BLINKR |= bn * 48; // ^
			TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
		}

		if (buf[bn] == 112) { // If this is a BPM-CHANGE command...
			BPM = buf[bn + 1]; // Set the global BPM to the new BPM-value
			updateTickSize(); // Update the global tick-size to reflect the new BPM value
			continue; // Either check the next command-slot or exit the loop
		}

		// If the MIDI-OUT queue is full, then ignore this command, since we're now sure that it's a MIDI command
		// (This is checked here, instead of in iterateAll(), and using "continue" instead of "return",
		//  because all BPM-CHANGE commands need to be caught, regardless of how full MOUT_COUNT gets)
		if (MOUT_COUNT == 8) { continue; }

		memcpy(MOUT + (MOUT_COUNT * 3), buf + bn, 3); // Copy the event into the MIDI buffer's lowest empty MOUT location
		MOUT_COUNT++; // Increase the counter that tracks the number of bytes in the MIDI buffer

		if ((buf[bn] & 240) != 144) { continue; } // If this wasn't a NOTE-ON, forego any sustain mechanisms

		// If the function hasn't exited, then we're dealing with a NOTE-ON. So...

		// Send NOTE-OFFs for any duplicate SUSTAINs, and remove duplicate MOUT entries
		removeDuplicates(buf[bn], buf[bn + 1]);

		buf[bn] -= 16; // Turn the NOTE-ON into a NOTE-OFF
		buf[bn + 2] = buf[bn + 3]; // Move DURATION to the next byte over, for the 3-byte SUST-array note-format

		clipBottomSustain(); // If the bottommost SUSTAIN-slot is full, send its NOTE-OFF prematurely
		memmove(SUST + 3, SUST, SUST_COUNT * 3); // Move all sustains one space downward
		memcpy(SUST, buf + bn, 3); // Create a new sustain corresponding to this note
		SUST_COUNT++; // Increase the number of active sustains by 1

		TO_UPDATE |= 2 * (!RECORDMODE); // If we're not in RECORDMODE, flag the sustain-row for a GUI update

	}

}

// Parse any active SCATTER values that might be associated with a given seq
void parseScatter(byte s, byte didscatter) {

	if (didscatter) { // If this tick was a SCATTER-tick...
		SCATTER[s] &= 15; // Unset the "distance" side of this seq's SCATTER byte
	} else { // Else, if this wasn't a SCATTER-tick...

		// If this tick's random-value doesn't exceed the seq's SCATTER-chance value, exit the function
		if ((GLOBALRAND & 15) >= (SCATTER[s] & 15)) { return; }

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
void processRepeats(byte ctrl) {
	if (
		(!ctrl) // If no command-button is currently held...
		&& (getInsertionPoint(distFromQuantize()) == POS[RECORDSEQ]) // And this tick occupies an insertion-point (as modified by QUANTIZE, QRESET, and OFFSET)...
	) { // Then this is a valid REPEAT position. So...
		arpAdvance(); // Advance the arpeggio-position
		processRecAction(modPitch(ARPPOS & 31)); // Put the current raw repeat-note or arpeggiation-note into the current track
		RPTVELO = applyChange(RPTVELO, char(int(RPTSWEEP) - 128), 0, 127); // Change the stored REPEAT-VELOCITY by the REPEAT-SWEEP amount
	}
}

// Get the notes from the current tick in a given seq, and add them to the MIDI-OUT buffer
void getTickNotes(byte ctrl, byte s, byte buf[]) {

	byte didscatter = 0; // Flag that tracks whether this tick has had a SCATTER effect

	if (RECORDMODE) { // If RECORD MODE is active...
		if (RECORDSEQ == s) { // If this is the current RECORDSEQ...
			if (REPEAT && ARPPOS) { // If REPEAT is toggled, and any note-buttons are being held...
				processRepeats(ctrl); // Process a REPEAT-RECORD action
			} else if (RECORDNOTES && (ctrl == B00111100)) { // Else, if RECORDNOTES is armed, and ERASE-NOTES is held...
				byte b[5]; // Make a buffer the size of a note...
				memset(b, 0, 4); // ...And clear it of any junk-data
				writeCommands( // Write the blank note to the savefile
					// Write at the RECORDSEQ's current position
					(FILE_BODY_START + (POS[RECORDSEQ] * 8)) + (FILE_SEQ_BYTES * RECORDSEQ) + (TRACK * 4),
					4, // Write 4 bytes of data
					b, // Use the empty buffer that was just created
					1 // Only overwrite notes that match the global CHAN
				);
				// note: file.sync() is called elsewhere, for efficiency reasons
			}
		}
		readTick(s, 0, buf); // Read the tick with no offset
	} else { // Else, if RECORD MODE is inactive...
		// Read the tick with SCATTER-offset (>> 2, not 3 or 4, because we want a minimum of an 8th-note granularity)
		readTick(s, (SCATTER[s] & 240) >> 2, buf);
		didscatter = 1;
	}

	// If the function hasn't exited by this point, then that means this tick contained a note. So...

	parseTickContents(s, buf); // Check the tick's contents, and add them to MIDI-OUT and SUSTAIN where appropriate
	parseScatter(s, didscatter); // Parse any active SCATTER values that might be associated with the seq

}

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity

	byte buf[9]; // Buffer for reading note-events from the datafile

	for (byte i = 47; i != 255; i--) { // For every sequence in the song, in reverse order...

		byte size = STATS[i] & 63; // Get seq's absolute size, in whole-notes

		parseCues(i, size); // Parse a sequence's cued commands, if any

		// If the seq isn't currently playing, go to the next seq's iteration-routine
		if (!(STATS[i] & 128)) { continue; }

		memset(buf, 0, 8); // Clear the buffer of junk data

		// Get the notes from this tick in a given seq, and add them to the MIDI-OUT buffer
		getTickNotes(ctrl, i, buf);

		// Increase the seq's 32nd-note position by one increment, wrapping it around its top limit
		POS[i] = (POS[i] + 1) % (word(size) * 32);

	}

	if (MOUT_COUNT) { // If there are any commands in the MIDI-command buffer...
		byte m3 = MOUT_COUNT * 3; // Get the number of outgoing bytes
		for (byte i = 0; i < m3; i += 3) { // For every command in the MIDI-OUT queue...
			Serial.write(MOUT + i, 2 + ((MOUT[i] % 224) <= 191)); // Send that command's number of bytes
		}
		memset(MOUT, 0, m3); // Clear the MOUT array of now-obsolete data
		MOUT_COUNT = 0; // Clear the MIDI buffer's command-counter
	}

	processSustains(); // Process one 32nd-note's worth of duration for all sustained notes

}
