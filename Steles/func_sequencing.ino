
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

// Advance the global sequencing-tick, and toggle over the activity-windows of every active sequence along with it
void advanceGlobalTick() {



}

// Iterate through all currently-active sequences by one tick
void iterateSeqs() {

	// If the sequencer isn't currently toggled to PLAYING mode, ignore this iteration-command
	if (!PLAYING) { return; }

	byte note[4]; // Var to hold incoming 4-byte note values from the tempfile: dur, chan, pitch, velo

	file.open(string(SONG) + ".tmp", O_READ); // Open the tempfile, for reading note data

	for (byte i = 0; i < 32; i++) { // For every currently-loaded sequence...

		if (SEQ_SPOS[i] & 1) { // If the sequence's "currently playing" bit is filled...
			word tick = ((SEQ_SPOS & 14) * 96) + (CURTICK % 96); // Get the sequence's current active tick
			file.seekSet((i * 98304) + (tick * 16)); // Prepare to start reading from the tick's location in the tempfile
			for (byte p = 0; p < 4; p++) { // For each of the 4 potential notes on this tick...
				file.read(note, 4); // Read the note's data
				note[0] ? playNote(note) : break; // If the note exists, play it, else break from this read-loop
			}



		}

	}

	file.close(); // Close the tempfile, having done all reads for all seqs on this tick

}
