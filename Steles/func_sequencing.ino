
// Update the internal tick-size (in microseconds) to match the new BPM value
void updateTickSize() {
	TICKSIZE = round(60000000L / (BPM * 96));
}

// Reset the "most recent note by channel" array
void clearRecentNotes() {
	memset(RECENT, 255, 16);
}

// Reset every sequence, including SCATTER values
void resetAllSeqs() {
	memset(CMD, 0, sizeof(CMD) - 1);
	memset(POS, 0, sizeof(POS) - 1);
	memset(SCATTER, 0, sizeof(SCATTER) - 1);
	resetAllPlaying();
}

// Reset all seqs' PLAYING flags in the STATS array
void resetAllPlaying() {
	for (byte i = 0; i < 48; i++) {
		STATS[i] &= 127;
	}
}

// Reset a seq's cued-commands, playing-byte, and tick-position
void resetSeq(byte s) {
	CMD[s] = 0;
	POS[s] = 0;
	STATS[s] &= 127;
	//SCATTER[s] &= 7; // Wipe all of the seq's scatter-counting and scatter-flagging bits, but not its scatter-chance bits
}

// Send MIDI-OFF commands for all currently-sustained notes
void haltAllSustains() {
	for (byte i = 0; i < (SUST_COUNT * 3); i++) { // For every active sustain...
		Serial.write(SUST[i + 1]); // Send a premature NOTE-OFF for the sustain
		Serial.write(SUST[i + 2]); // ^
		Serial.write(127); // ^
	}
	SUST_COUNT = 0;
}

// Process one 16th-note worth of duration for all notes in the SUSTAIN system
void processSustains() {
	byte n = 0; // Value to iterate through each sustain-position
	while (n < (SUST_COUNT * 3)) { // For every sustain-entry in the SUST array...
		if (SUST[n]) { // If any duration remains...
			SUST[n]--; // Reduce the duration by 1
			n += 3; // Move on to the next sustain-position normally
		} else { // Else, if the remaining duration is 0...
			Serial.write(SUST[n + 1]); // Send a NOTE-OFF for the sustain
			Serial.write(SUST[n + 2]); // ^
			Serial.write(127); // ^
			if (n < 21) { // If this isn't the lowest sustain-slot...
				memmove(SUST + n, SUST + n + 3, 24 - (n + 3)); // Move all lower sustains one slot upwards
			}
			SUST_COUNT--; // Reduce the sustain-count
			// n doesn't need to be increased here, since the next sustain occupies the same index now
		}
	}
}

// Send all outgoing MIDI commands in a single burst
void flushNoteOns() {
	if (!MOUT_COUNT) { return; } // If there are no commands in the NOTE-ON buffer, exit the function
	Serial.write(MOUT, MOUT_COUNT * 3); // Send all outgoing MIDI-command bytes at once
	MOUT_COUNT = 0; // Clear the MIDI buffer's counting-byte
}

// Compare a seq's CUE-commands to the global CUE-point, and parse them if the timing is correct
void parseCues(byte s, word size) {

	if (
		(!CMD[s]) // If the seq has no cued commands...
		|| (CUR16 != ((CMD[s] & B11100000) >> 1)) // Or the global 16th-note doesn't correspnd to the seq's cue-point...
	) { return; } // ...Exit the function

	// Enable or disable the sequence's playing-bit
	STATS[s] = (STATS[s] & B01111111) | ((CMD[s] & 2) << 6);

	// Set the sequence's internal tick to a position based on the incoming SLICE bits
	POS[s] = size * ((CMD[s] & B00011100) >> 1);

	// Flag the sequence's corresponding LED-row for an update
	TO_UPDATE |= 4 >> ((s % 24) >> 2);

}


// Get the notes from the current tick in a given seq, and add them to the MIDI-OUT buffer
void getTickNotes(byte s) {

	byte buf[9]; // Buffer for reading note-events from the datafile

	if (MOUT_COUNT == 8) { return; } // If the MIDI-OUT queue is full, exit the function

	byte readpos = POS[s]; // Get the default read-position for this tick

	if (SCATTER[s] & 128) { // If the seq's SCATTER is flagged as active...

		// Get a byte-sized truncation of ABSOLUTETIME, for faster(?) bitwise operations
		byte ashort = ABSOLUTETIME & 255;

		// Get bits from ABSOLUTETIME, to use for pseudo-random values
		char rnd = ashort & 3; // 1st random: 2 bits
		char rnd2 = rnd ? ((ashort >> 2) & 3) : 0; // 2nd random: 2 bits, or 0 if rnd is 0
		char rnd3 = (ashort >> 4) & 1; // 3rd random: 1 bit
		rnd2 &= rnd2 >> 1; // Ensure that rnd2 is 1 a quarter of the time, and 0 the rest of the time

		// Add a random scatter-distance (positive or negative) to the read-position,
		// favoring eighth-notes, quarter-notes, and half-notes
		readpos = (readpos + (((((char) 2) | rnd2) << rnd) | (128 * rnd3))) % (STATS[s] & 127);

	}

	file.seekSet(49 + readpos + (s * 8192)); // Navigate to the note's absolute position
	file.read(buf, 8); // Read the data of the tick's notes

	if (!buf[3]) { return; } // If no notes are present, exit the function

	// For either one or both note-slots on this tick, depending on how many are filled...
	for (byte bn = 0; bn < (((!!buf[7]) + 1) << 2); bn += 4) {

		if (MOUT_COUNT == 8) { return; } // If the MIDI buffer is full, exit the function

		byte bn1 = bn + 1; // Get note's CHANNEL index
		byte bn2 = bn + 2; // Get note's PITCH/SPECIAL index

		if (buf[bn2] == 224) { // If this is a special REPEAT command...
			if (RECENT[buf[bn1]] == 255) { continue; } // If no note has played on this channel yet, this command does nothing
			buf[bn2] = RECENT[buf[bn1]]; // Change this command's PITCH byte to the most-recent pitch for this command's cannel
		} else { // Else, if this is a regular NOTE command...
			RECENT[buf[bn1]] = buf[bn2]; // Store the note's pitch, indexed by its channel
		}

		byte pos = MOUT_COUNT * 3; // Get the lowest empty MOUT location

		memcpy(MOUT + pos, buf + bn1, 3); // Copy the note into the MIDI buffer
		MOUT_COUNT++; // Increase the counter that tracks the number of bytes in the MIDI buffer

		if (SUST_COUNT == 8) { // If the SUSTAIN buffer is already full...
			Serial.write(SUST[23]); // Send a premature NOTE-OFF for the oldest active sustain
			Serial.write(SUST[24]); // ^
			Serial.write(127); // ^
			SUST_COUNT--; // Reduce the number of active sustains by 1
		}
		memmove(SUST, SUST + 3, (SUST_COUNT * 3)); // Move all sustains one space downward
		memcpy(SUST, buf + bn, 3); // Create a new sustain corresponding to this note
		SUST[1] ^= 16; // Turn the new sustain's NOTE-ON into a NOTE-OFF preemptively
		SUST_COUNT++; // Increase the number of active sustains by 1

	}

	// If the function hasn't exited by this point, then that means this tick contained a note. So...

	if (SCATTER[s] >= 128) { // If this was a SCATTER note...
		SCATTER[s] &= 7; // Remove the seq's scatter-activity flag & note-counting bits
	} else { // Else, if this wasn't a SCATTER note...
		byte chance = SCATTER[s] & B00000111; // Get the seq's scatter-chance bits, which are set by the user
		byte count = (SCATTER[s] & B01111000) >> 3; // Get the note-count since last scatter-event
		if (chance) { // If this seq has scatter-chance bits...
			// Have a random chance of setting the scatter-active flag,
			// made more likely if it has been a long time since the last scatter-event (counted by the "count" bits)
			SCATTER[s] |= ((chance + count) > random(31)) ? 128 : 0;
			if (count < 15) { // If the count-value hasn't reached its maximum...
				// Increment it by 1 and put it back into the seq's SCATTER count-bits
				SCATTER[s] = (SCATTER[s] & B10000111) | ((count + 1) << 3);
			}
		}
	}

}

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	processSustains(); // Process one 16th-note's worth of duration for all sustained notes

	if (PLAYING) { // If the sequencer is currently in PLAYING mode...

		byte changed = 0;

		for (byte i = 47; i != 255; i--) { // For every loaded sequence, in reverse order...

			word size = STATS[i] & 127; // Get seq's absolute size, in beats

			parseCues(i, size); // Parse a sequence's cued commands, if any

			// If the seq isn't currently playing, go to the next seq's iteration-routine
			if (!(STATS[i] & 128)) { continue; }

			// If RECORD MODE is active, and the ERASE-NOTES command is being held,
			// and notes are being recorded into this seq...
			if (RECORDMODE && ERASENOTES && (RECORDNOTES == i)) {
				byte buf[9] = {0, 0, 0, 0, 0, 0, 0, 0};
				file.seekSet(49 + POS[i] + (i * 8192)); // Set position to start of tick's first note
				file.write(buf, 8); // Write in an entire empty tick's worth of bytes
				changed = 1;
			} else { // Else, if any other combination of states applies...
				getTickNotes(i); // Get the notes from this tick in a given seq, and add them to the MIDI-OUT buffer
			}

			// Increase the seq's 16th-note position by one increment, wrapping it around its top limit
			POS[i] = (POS[i] + 1) % (size << 4);

		}

		if (changed) { // If any changes have been queued for the tempfile's data...
			file.sync(); // Apply all changes to the tempfile
		}

	}

	if (MOUT_COUNT) { // If any notes are in the MIDI-OUT buffer...
		flushNoteOns(); // Send them all at once
	}

}
