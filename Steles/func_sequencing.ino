
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

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	if (!PLAYING) { return; } // If the sequencer isn't currently toggled to PLAYING mode, ignore this iteration

	CURTICK = (CURTICK + 1) % 384; // Advance the global current-tick, bounded to the global slice-size
	TO_UPDATE |= 1 >> (CURTICK % 96); // If the global cue-timer advanced by a chunk, cue the global-tick-row for a GUI update

	byte note[4]; // Var to hold incoming 4-byte note values from the tempfile: dur, chan, pitch, velo

	file.open(string(SONG) + ".tmp", O_READ); // Open the tempfile, for reading note data

	for (byte i = 0; i < 32; i++) { // For every currently-loaded sequence...

		if ( // If...
			SEQ_CMD[i] // The seq has cued commands...
			&& (!(CURTICK % (pow(SEQ_CMD[i] & 3, 3) * 96))) // And the new global tick corresponds to the seq's CUE command, or lack thereof...
		) { // Then apply the commands that are cued





		}

		if (SEQ_PSIZE[i] & 128) { // If the seq's "currently playing" bit is filled...
			byte smask = (SEQ_PSIZE[i] & 127) + 1; // Get the seq's size modifier, without its playing-bit
			word size = 24 * smask; // Get the seq's absolute size, in ticks
			word csize = word(size / 8); // Get the number of ticks within each of the seq's slices
			byte oldchunk = byte(SEQ_POS[i] / csize); // Get the slice-chunk that the sequence's old tick occupied
			SEQ_POS[i] = (SEQ_POS[i] + 1) % size; // Increase the seq's position by 1, wrapping at its upper size boundary
			byte chunk = byte(SEQ_POS[i] / csize); // Get the slice-chunk that the sequence's active tick occupies
			while ((1 << chunk) & SEQ_EXCLUDE[i]) { // While the current slice-chunk is excluded...
				SEQ_POS[i] = (SEQ_POS[i] + csize) % size; // Skip directly to the next slice-chunk
				chunk = (chunk + 1) % 8; // Change the chunk var to reflect that the active slice-chunk has shifted
			}
			file.seekSet((i * 55296) + SEQ_POS[i]); // Prepare to start reading from the tick's location in the tempfile
			file.read(note, 3); // Read the data of the tick's first note
			if (note[0]) { // If the first note exists...
				playNote(note); // Play the note
				file.read(note, 3); // Read the data of the tick's second note
				if (note[0]) { // If the second note exists...
					playNote(note); // Play the note
				}
			}
			note[0] ? playNote(note) : break; // If the note exists, play it, else break from this read-loop
			if (oldchunk != chunk) { // If the slice-chunk-position has visibly changed...
				TO_UPDATE |= 248; // Queue a GUI update for all slicing-canvas rows
			}
		}

	}

	file.close(); // Close the tempfile, having done all reads for all seqs on this tick

}
