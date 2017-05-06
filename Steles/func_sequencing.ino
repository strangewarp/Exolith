
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

// Empty out any pending cued commands, and reset every sequence-slot's playing-byte and tick-position
void resetSeqs() {
	for (byte i = 0; i < 72; i++) {
		SEQ_CMD[i] = 0;
		SEQ_POS[i] = (SEQ_SIZE[i] * 96) - 1;
	}
}

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	if (!PLAYING) { return; } // If the sequencer isn't currently toggled to PLAYING mode, ignore this iteration

	CURTICK = (CURTICK + 1) % 768; // Advance the global current-tick, bounded to the global slice-size
	TO_UPDATE |= 1 >> (CURTICK % 96); // If the global cue-timer advanced by a chunk, cue the global-tick-row for a GUI update

	byte note[5]; // Var to hold incoming 4-byte note values from the tempfile: dur, chan, pitch, velo

	file.open(string(SONG) + ".tmp", O_READ); // Open the tempfile, for reading note data

	for (byte i = 0; i < 72; i++) { // For every currently-loaded sequence...

		word size = SEQ_SIZE[i] * 96; // Get seq's absolute size, in ticks
		word csize = size >> 3; // Get the number of ticks within each of the seq's slices

		if (SEQ_CMD[i]) { // If the seq has cued commands...

			byte slice = (SEQ_CMD[i] & B00011100) >> 2; // Get seq's slice-point

			// If the global tick corresponds to the seq's cue-point...
			if (CURTICK == ((((SEQ_CMD[i] & B11100000) >> 5) * 96) % 768)) {
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

		for (byte n = 0; n < 4; n++) { // For every note-slot in the tick...
			file.seekSet(73 + maskpos + (n * 4) + (i * 98304)); // Read the note from its file-position
			file.read(note, 4); // Read the data of the tick's first note
			if (note[1]) { // If the note exists...
				playNote(note); // Play the note
			}
		}

	}

	file.close(); // Close the tempfile, having done all reads for all seqs on this tick

}
