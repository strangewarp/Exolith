
// Reset every sequence
void resetAllSeqs() {
	memset(SEQ_CMD, 0, sizeof(SEQ_CMD));
	memset(SEQ_POS, 0, sizeof(SEQ_POS));
}

// Reset a seq's cued-commands, playing-byte, and tick-position
void resetSeq(byte s) {
	SEQ_CMD[s] = 0;
	SEQ_POS[s] = 0;
}

// Send MIDI-OFF commands for all currently-sustained notes
void haltAllSustains() {
	for (byte i = 0; i < 8; i++) {
		byte i3 = i * 3;
		if (SUSTAIN[i3 + 2] > 0) {
			Serial.write(128 + SUSTAIN[i3]);
			Serial.write(SUSTAIN[i3 + 1]);
			Serial.write(127);
			memset(SUSTAIN[i3], 0, 3);
		}
	}
}

// Halt a single sustain in the sustain-system
void haltSustain(byte num) {

	// Send a NOTE-OFF to the MIDI-OUT port
	byte n = num << 1;
	Serial.write(128 + SUSTAIN[n]);
	Serial.write(SUSTAIN[n + 1]);
	Serial.write(127);

	// Shift every sustain-note underneath this note upwards by one slot
	if (num < 7) {
		byte n2 = 3 * (num + 1);
		memmove(SUSTAIN + n, SUSTAIN + n2, 24 - n);
	}
	memset(SUSTAIN + 21, 255, 3); // Clear the bottommost sustain-slot regardless

}

// Process one 16th-note worth of duration for all notes in the SUSTAIN system
void processSustains() {
	for (byte n = 0; n < 16; n += 2) { // For every sustain-entry in the SUST array...
		if (SUST[n + 1] == 255) { break; } // If this sustain-slot is empty, finish checking
		byte dur = SUST[n] & 15; // Get the remaining-duration-in-16th-notes for this sustain
		if (dur) { // If any duration remains...
			SUST[n] &= (dur - 1) | 240; // Reduce the duration by 1, and add it back to its chan-dur composite byte
		} else { // Else, if the remaining duration is 0...
			haltSustain(n); // Halt the sustain
		}
	}
}

// Play a given MIDI note, adding a corresponding note to the SUSTAIN system if applicable
void playNote(byte dur, byte size, byte b1, byte b2, byte b3) {

	// Split the MIDI CHANNEL off from its command-type
	byte chan = b1 % 16;
	byte cmd = b1 - chan;

	if (cmd == 144) { // If this is a NOTE-ON cmmand...

		// If the lowest sustain-slot is filled, then halt its note and empty it
		if (SUSTAIN[21] < 255) {
			haltSustain(7);
		}

		// For every currently-filled sustain-slot, move its contents downward by one slot
		memmove(SUSTAIN + 3, SUSTAIN, 21);

		// Put the note's sustain-value in the top sustain-slot
		SUSTAIN[0] = chan;
		SUSTAIN[1] = b2;
		SUSTAIN[2] = dur;

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

	if (!(CURTICK % 6)) { // If this tick falls on a 16th-note...

		processSustains(); // Process one 16th-note's worth of duration for all sustained notes
		flushMidiOut(); // Flush the MIDI-OUT array, which may now contain sustains' OFF-commands

		if (PLAYING) { // If the sequencer is currently in PLAYING mode...

			byte buf[7]; // Buffer for reading note-events from the datafile

			for (byte i = 0; i < 48; i++) { // For every loaded sequence...

				byte s16 = (SEQ_STATS[i] & 31) << 4; // Get seq's absolute size, in 16th-notes
				byte c16 = s16 >> 3; // Get seq's chunk-size, in 16th-notes

				if (SEQ_CMD[i]) { // If the seq has cued commands...
					byte slice = (SEQ_CMD[i] & B00011100) >> 2; // Get slice-point from incoming command
					if (CURTICK == ((((SEQ_CMD[i] & B11100000) >> 5) * 96) % 768)) { // If the global tick corresponds to the seq's cue-point...

						// Enable or disable the sequence's playing-bit
						SEQ_STATS[i] = (SEQ_STATS[i] & B00011111) | (128 * ((SEQ_CMD[i] & 3) >> 1));

						// Set the sequence's internal tick to a position based on the incoming SLICE bits
						SEQ_POS[i] = c16 * slice;

						// Flag the sequence's LED-row for an update
						TO_UPDATE |= (i % 24) >> 2;

					}
				}

				// If the seq isn't currently playing, go to the next seq's iteration-routine
				if (!(SEQ_STATS[i] & 128)) { continue; }

				// If RECORD MODE is active, and the ERASE-NOTES command is being held,
				// and notes are being recorded into this seq...
				if (RECORDMODE && ERASENOTES && (RECORDNOTES == i)) {
					file.seekSet(49 + SEQ_POS[i] + (i * 6144)); // Set position to start of tick's first note
					file.write(EMPTY_TICK, 6) // Write in an entire empty tick's worth of bytes
					continue; // Skip the remaining read-and-play routines for this sequence
				}

				if (MOUT_COUNT < 8) { // If the MIDI-OUT queue isn't full...

					file.seekSet(49 + SEQ_POS[i] + (i * 2048)); // Navigate to the note's absolute position
					file.read(buf, 6); // Read the data of the tick's notes
					if (buf[2]) { // If the note exists...
						byte bnum = (buf[5] && (MOUT_COUNT < 7)) ? 2 : 1; // Check if a second note exists, and if there's a MOUT space for it
						memcpy(MOUT + (MOUT_COUNT * 3), buf, 3 * bnum); // Copy note-data to the MIDI-OUT buffer
						MOUT_COUNT += bnum; // Increase the count of notes contained within the MIDI-OUT buffer
					}

				}

				// Increase the seq's 16th-note position by one increment, wrapping it around its top limit
				SEQ_POS[i] = (SEQ_POS[i] + 1) % s16;

			}



		}

	}


	// note: needs file.open at start of function??




	//ICOUNT = (ICOUNT + 1) % 6;


	file.close(); // Close the tempfile, having done all reads for all seqs on this tick

	CURTICK = (CURTICK + 1) % 768; // Advance the global current-tick, bounded to the global slice-size

	TO_UPDATE |= 1 >> (CURTICK % 96); // If the global cue-timer advanced by a chunk, cue the global-tick-row for a GUI update



	if (MOUT_COUNT) { // If any notes are in the MIDI-OUT buffer...
		flushMidiOut(); // Send them all at once
	}

}
