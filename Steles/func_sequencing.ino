
// Update the internal tick-size (in microseconds) to match the new BPM value
void updateTickSize() {
	TICKSIZE = round(60000000L / (BPM * 96));
}

// Reset the "most recent note by channel" array
void clearRecentNotes() {
	memset(RECENT, 60, 16);
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
	//SCATTER[s] &= 15; // Wipe all of the seq's scatter-counting and scatter-flagging bits, but not its scatter-chance bits
}

// Apply an interval-command to a given pitch, within a given CHANNEL
byte applyIntervalCommand(byte cmd, byte pitch) {

	byte interv = cmd & 31; // Get the pitch-based interval value
	char offset = interv; // Default offset: the interval itself

	cmd &= B11100000; // Reduce the INTERVAL-command to its flags, now that its interval has been removed

	if (cmd & B00100000) { // If this is a RANDOM-flavored command...
		// Get a random offset within the bounds of the interval-value
		offset = char(GLOBALRAND % (interv + 1));
	}

	if (cmd & B01000000) { // If this is a DOWN-flavored command...
		offset = -offset; // Point the offset downwards
	}

	int shift = int(pitch) + offset; // Get a shifted version of the given pitch
	while (shift < 0) { shift += 12; } // Increase too-low shift-values
	while (shift > 127) { shift -= 12; } // Decrease too-high shift-values

	return byte(shift); // Return a MIDI-style pitch-byte

}

// Compare a seq's CUE-commands to the global CUE-point, and parse them if the timing is correct
void parseCues(byte s, word size) {

	if (
		(!CMD[s]) // If the seq has no cued commands...
		|| (CUR16 != ((CMD[s] & B11100000) >> 1)) // Or the global 16th-note doesn't correspond to the seq's cue-point...
	) { return; } // ...Exit the function

	// Enable or disable the sequence's playing-bit
	STATS[s] = (STATS[s] & B01111111) | ((CMD[s] & 2) << 6);

	// Set the sequence's internal tick to a position based on the incoming SLICE bits
	POS[s] = size * ((CMD[s] & B00011100) >> 1);

	CMD[s] = 0; // Clear the sequence's CUED-COMMANDS byte

	// Flag the sequence's corresponding LED-row for an update
	TO_UPDATE |= 4 << ((s % 24) >> 2);

}

// Get the notes from the current tick in a given seq, and add them to the MIDI-OUT buffer
void getTickNotes(byte s) {

	if (MOUT_COUNT == 8) { return; } // If the MIDI-OUT queue is full, exit the function

	byte buf[9]; // Buffer for reading note-events from the datafile

	byte didscatter = 0; // Flag that tracks whether this tick has had a SCATTER effect

	// If notes are being recorded into this seq,
	// and RECORD MODE is active, and RECORD-NOTES is armed,
	// and the ERASE-NOTES command is being held...
	if ((RECORDSEQ == s) && RECORDMODE && RECORDNOTES && ERASENOTES) {

		unsigned long tpos = (49UL + (POS[s] * 8)) + (8192UL * s); // Get the tick's position in the savefile

		file.seekSet(tpos); // Navigate to the note's absolute position
		file.read(buf, 8); // Read the data of the tick's notes

		byte full = buf[0] || buf[2]; // Check if the tick's top note is filled
		if (!full) { return; } // If no notes or CCs are present, exit the function

		// Check if the first and/or second note matches the global CHAN
		byte pos1 = full && ((buf[0] & 15) == CHAN);
		byte pos2 = (buf[4] || buf[6]) && ((buf[4] & 15) == CHAN);

		if (pos1 || pos2) { // If either note matches the global chan...
			byte outbuf[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Make a tick-sized buffer to send blank data
			if (!pos1) { // If the first note didn't match global CHAN...
				file.seekSet(tpos + 4); // Set the write-position to the tick's bottom-note
			}
			file.write(outbuf, (pos1 + pos2) * 4); // Clear all ticks in the byte that contain notes matching the global CHAN
			file.sync(); // Apply changes to the savefile immediately
			if (pos1 && pos2) { // If both notes matched global CHAN...
				return; // Exit the function, since there's nothing left to do for this tick of this seq
			} else { // Else, if only the first or second note matched global CHAN...
				if (pos1) { // If only the first note matched global CHAN...
					memmove(buf, buf + 4, 4); // Move the buffer's bottom note to its top slot
				}
				memset(buf + 4, 0, 4); // Empty the buffer's bottom note-slot in either case
			}
		}

	} else { // Else, if ERASE-NOTES isn't currently happening...

		unsigned long tpos = POS[s]; // Get the tick's position in the savefile

		byte sc = (SCATTER[s] & 240) >> 3; // Get the seq's SCATTER-read-distance
		if (sc) { // If the seq has an active SCATTER-read-distance...
			didscatter = 1; // Flag this tick as having had a SCATTER effect
			// Add that distance to the read-point, wrapping around the end of the seq
			tpos = (tpos + sc) % ((STATS[s] & 127) * 16);
		}

		// Navigate to the SCATTER-note's absolute position,
		// and compensate for the fact that each tick contains 8 bytes
		file.seekSet(49UL + (tpos * 8) + (8192UL * s));
		file.read(buf, 8); // Read the data of the tick's notes

	}

	if (!(buf[0] || buf[2])) { return; } // If the tick contains no commands, then exit the function

	// For either one or both note-slots on this tick, depending on how many are filled...
	for (byte bn = 0; bn < ((buf[4] || buf[6]) + 4); bn += 4) {

		if (MOUT_COUNT == 8) { return; } // If the MIDI buffer is full, exit the function

		byte bn1 = bn + 1; // Get note's PITCH index

		if (buf[bn1] & 128) { // If this is an INTERVAL command...
			buf[bn] &= 15; // Ensure that this command's CHAN byte is in NOTE format
			// Apply the byte's INTERVAL command to the channel's most-recent pitch,
			// and then act like the command's byte was always the resulting pitch.
			buf[bn1] = applyIntervalCommand(buf[bn1], RECENT[buf[bn]]);
		}

		if (buf[bn] <= 15) { // If this is a NOTE command...
			RECENT[buf[bn]] = buf[bn1]; // Set the channel's most-recent note to this command's pitch
		}

		// Convert the note's CHANNEL byte into either a NOTE or CC command
		buf[bn] = 144 + (buf[bn] & 15) + ((buf[bn] & 16) << 1);

		memcpy(MOUT + (MOUT_COUNT * 3), buf + bn, 3); // Copy the note into the MIDI buffer's lowest empty MOUT location
		MOUT_COUNT++; // Increase the counter that tracks the number of bytes in the MIDI buffer

		if (buf[bn] >= 176) { continue; } // If this was a proto-MIDI-CC command, forego any sustain mechanisms

		buf[bn] -= 16; // Turn the NOTE-ON into a NOTE-OFF
		buf[bn + 2] = buf[bn + 3]; // Move DURATION to the next byte over, for the 3-byte sustain-storage format

		clipBottomSustain(); // If the bottommost SUSTAIN-slot is full, send its NOTE-OFF prematurely
		memmove(SUST + 3, SUST, SUST_COUNT * 3); // Move all sustains one space downward
		memcpy(SUST, buf + bn, 3); // Create a new sustain corresponding to this note
		SUST_COUNT++; // Increase the number of active sustains by 1

		TO_UPDATE |= 2 * (!RECORDNOTES); // If ARMED STEP-RECORD isn't active, flag the sustain-row for a GUI update

	}

	// If the function hasn't exited by this point, then that means this tick contained a note. So...

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

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	if (PLAYING) { // If the sequencer is currently in PLAYING mode...

		for (byte i = 47; i != 255; i--) { // For every loaded sequence, in reverse order...

			word size = STATS[i] & 127; // Get seq's absolute size, in beats

			parseCues(i, size); // Parse a sequence's cued commands, if any

			// If the seq isn't currently playing, go to the next seq's iteration-routine
			if (!(STATS[i] & 128)) { continue; }

			getTickNotes(i); // Get the notes from this tick in a given seq, and add them to the MIDI-OUT buffer

			// Increase the seq's 16th-note position by one increment, wrapping it around its top limit
			POS[i] = (POS[i] + 1) % (size << 4);

		}

	}

	if (MOUT_COUNT) { // If there are any commands in the NOTE-ON buffer...
		Serial.write(MOUT, MOUT_COUNT * 3); // Send all outgoing MIDI-command bytes at once
		MOUT_COUNT = 0; // Clear the MIDI buffer's counting-byte
	}

	processSustains(); // Process one 16th-note's worth of duration for all sustained notes

}
