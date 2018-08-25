

// Write a given series of commands into a given point in the savefile,
// only committing actual writes when the data doesn't match, in order to reduce SD-card wear.
void writeCommands(
	unsigned long pos, // Note's bitwise position in the savefile (NOTE: (pos + amt) must be <= bytes in savefile!)
	byte amt, // Number of bytes to compare-and-write (must be a multiple of 4)
	byte b[], // Array of command-bytes to compare against the data, and write into the file where discrepancies are found
	byte onchan // Flag: only replace a given note if its channel matches the global CHAN
) {

	byte* buf = NULL; // Create a pointer to a dynamically-allocated buffer...
	buf = new byte[amt + 1]; // Allocate that buffer's memory based on the given size

	file.seekSet(pos); // Navigate to the data's position
	file.read(buf, amt); // Read the already-existing data from the savefile into the data-buffer

	for (byte i = 0; i < amt; i += 4) { // For every byte in the data...
		if (
			( // If the command doesn't match...
				(buf[i] != b[i])
				|| (buf[i + 1] != b[i + 1])
				|| (buf[i + 2] != b[i + 2])
				|| (buf[i + 3] != b[i + 3])
			) && ( // And...
				(
					!onchan // Replace-on-chan-match isn't flagged...
				) || ( // Or...
					onchan // Replace-on-chan-match is flagged...
					&& ((buf[i] & 15) == (CHAN & 15)) // And the channels match...
				) || ( // Or...
					(buf[i] == 112) // The command-to-be-replaced is a channel-independent internal BPM command...
					&& (buf[i + 1] != b[i + 1]) // And its value doesn't match the given BPM-value...
				) || ( // Or...
					(buf[i] == 96) // The command-to-be-replaced is a channel-independent internal SWING command...
					&& ( // And...
						(buf[i + 1] != b[i + 1]) // Its first byte doesn't match the given SGRAN-value...
						|| (buf[i + 2] != b[i + 2]) // Or its second byte doesn't match the given SAMOUNT-value...
					)
				)
			)
		) {
			file.seekSet(pos + i); // Go to the command's position in the savefile
			file.write(b + i, 4); // Write the given command into that position
		}
	}

	delete [] buf; // Free the buffer's dynamically-allocated memory
	buf = NULL; // Unset the buffer's pointer

}

// Record a given MIDI command into the tempdata-file of the current RECORDSEQ sequence
void recordToSeq(word pstn, byte chan, byte b1, byte b2, byte trk) {

	// Get the position of the first of this tick's bytes within the active track in the data-file
	unsigned long tickstart = (FILE_BODY_START + (FILE_SEQ_BYTES * RECORDSEQ)) + (((unsigned long)pstn) * 8) + (trk * 4);

	byte cstrip = chan & 240; // Get the command-type, stripped of any MIDI CHANNEL information

	// Construct a virtual MIDI command, with an additional DURATION value
	byte note[5] = {
		(cstrip <= 112) ? cstrip : chan, // Channel-byte (stripped of CHAN-info for any internal BPM or SWING commands)
		byte((cstrip == 112) ? max(BPM_LIMIT_LOW, min(BPM_LIMIT_HIGH, b1 + b2)) : b1), // Pitch-byte, or BPM-byte in a BPM-CHANGE command
		byte(b2 * (((cstrip % 224) <= 176) && (cstrip != 112))), // Velocity-byte, or 0 if this is a 2-byte command
		byte((cstrip == 144) * DURATION) // Duration-byte, or 0 if this is a non-NOTE-ON command
	};

	writeCommands(tickstart, 4, note, 0); // Write the note to its corresponding savefile position

	// note: file.sync() is called elsewhere, for efficiency reasons

}

// Parse all of the possible actions that signal the recording of commands
void processRecAction(byte col, byte row, byte trk) {

	// Get a note that corresponds to the key, organized from bottom-left to top-right, with all modifiers applied
	byte pitch = (OCTAVE * 12) + ((23 - ((row * 4) + col)) ^ 3);
	while (pitch > 127) { // If the pitch is above 127, which is the limit for MIDI notes...
		pitch -= 12; // Reduce it by an octave until it is at or below 127
	}

	// Get the note's velocity, with a random humanize-offset
	byte velo = VELO - min(VELO - 1, byte(GLOBALRAND & 255) % (HUMANIZE + 1));

	if (PLAYING && RECORDNOTES) { // If notes are being recorded into a playing sequence...

		word qp = POS[RECORDSEQ]; // Will hold a QUANTIZE-modified insertion point (defaults to the seq's current position)

		if (!REPEAT) { // If REPEAT isn't toggled...
			char down = distFromQuantize(); // Get distance to previous QUANTIZE point
			byte up = min(QRESET - down, QUANTIZE - down); // Get distance to next QUANTIZE point, compensating for QRESET
			qp += (down <= up) ? (-down) : up; // Make the shortest distance into an offset for the note-insertion point
			qp %= word(STATS[RECORDSEQ] & 63) * 16; // Wrap the insertion-point around the seq's length
		}

		recordToSeq(qp, CHAN, pitch, velo, trk); // Record the note into the current RECORDSEQ slot

		// If the quantize-point fell on this tick,
		// then exit the function without sending the command, and without flagging GUI-updates,
		// since the command will be sent immediately after this anyway by a readTick() call.
		if (qp == POS[RECORDSEQ]) { return; }

	}

	// Flag GUI elements in the middle of the function, as the function may exit early
	BLINKL |= (!TRACK) * 192; // Start a TRACK-linked LED-BLINK that is ~12ms long
	BLINKR |= TRACK * 192; // ^
	TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update

	// If this was a SWING-CHANGE command, change the local SWING values immediately to reflect its contents
	if (CHAN == 96) {
		SGRAN = col + 1; // Derive SWING GRANULARITY from the keystroke's column
		SAMOUNT = velo; // Derive SWING AMOUNT from the HUMANIZE-modified VELOCITY
		updateTickSize(); // Update the global tick-size to reflect the new SWING values
		updateSwingPart(); // Update the SWING-PART var based on the current SWING GRANULARITY and CUR16 tick
		return; // Exit the function, since SWING-CHANGE commands require neither a SUSTAIN nor a MIDI-OUT message
	}

	// If this was a BPM-CHANGE command, change the local BPM immediately to reflect its contents
	if (CHAN == 112) {
		// Add the modified-pitch and velo, and clamp them into the new BPM value
		BPM = max(BPM_LIMIT_LOW, min(BPM_LIMIT_HIGH, pitch + velo));
		updateTickSize(); // Update the global tick-size to reflect the new BPM value
		return; // Exit the function, since BPM-CHANGE commands require neither a SUSTAIN nor a MIDI-OUT message
	}

	// If this was a NOTE-ON command, update the SUSTAIN buffer in preparation for playing the user-pressed note
	if ((CHAN & 240) == 144) {
		clipBottomSustain(); // If the bottommost SUSTAIN is filled, send its NOTE-OFF prematurely
		memmove(SUST + 3, SUST, SUST_COUNT * 3); // Move all sustains one space downward
		SUST[0] = CHAN - 16; // Fill the topmost sustain-slot with the user-pressed note's corresponding OFF command
		SUST[1] = pitch; // ^
		SUST[2] = DURATION; // ^
		SUST_COUNT++; // Increase the number of active sustains, to accurately reflect the new note
	}

	byte buf[4] = {CHAN, pitch, velo, 0}; // Build a correctly-formatted MIDI command
	Serial.write(buf, 2 + ((CHAN % 224) <= 191)); // Send the command to MIDI-OUT, with the correct number of bytes

}

