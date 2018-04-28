

// Helper function for writeData:
// Write a number of buffered bytes to the file, based on the value of an iterator,
//     and how many bytes it's been since the last matching byte
void wdSince(
	unsigned long pos, // Base position for the write
	byte &since, // Number of bytes that have elapsed since the last matching-byte
	byte i, // Current iterator value
	byte b[] // Buffer of bytes to write
) {
	if (since) { // If it has been at least 1 byte since the last matching byte...
		byte wbot = i - since; // Get the bottom byte for the write-position
		file.seekSet(pos + wbot); // Navigate to the byte-cluster's position in the savefile
		file.write(b + wbot, since); // Write the data-bytes into their corresponding savefile positions
		since = 0; // Reset the bytes-since-last-matching-byte variable
	}
}

// Write a given series of bytes into a given point in the savefile,
// only committing actual writes when the data doesn't match, in order to reduce SD-card wear.
void writeData(
	unsigned long pos, // Note's bitwise position in the savefile (NOTE: (pos + amt) must be <= bytes in savefile!)
	byte amt, // Amount of data to read for comparison
	byte b[], // Array of bytes to compare against the data, and write into the file where discrepancies are found
	byte onchan // Flag: only replace a given note if its channel matches the global CHAN
) {

	byte* buf = NULL; // Create a pointer to a dynamically-allocated buffer...
	buf = new byte[amt + 1]; // Allocate that buffer's memory based on the given size...
	memset(buf, 0, amt); // ...And clear it of any junk-data

	file.seekSet(pos); // Navigate to the data's position
	file.read(buf, amt); // Read the already-existing data from the savefile into the data-buffer

	byte since = 0; // Will count the number of bytes that have been checked since the last matching-byte
	for (byte i = 0; i < amt; i++) { // For every byte in the data...

		if (
			onchan // If replace-on-chan-match is flagged...
			&& (!(i % 4)) // And this is a note's first byte...
			&& ((buf[i] & 15) != (CHAN & 15)) // And the channels don't match...
		) {
			wdSince(pos, since, i, b); // Write bytes to the file, based on iterator and since-bytes
			i += 3; // Increment the iterator to skip to the start of the next note in the data
			continue; // Skip this iteration of the loop
		}

		if (buf[i] == b[i]) { // If the byte in the savefile is the same as the given byte...
			wdSince(pos, since, i, b); // Write bytes to the file, based on iterator and since-bytes
		} else { // Else, if the byte in the savefile doesn't match the given byte...
			since++; // Flag that it has been an additional byte since any matching bytes were found
			if (i == (amt - 1)) { // If this is the last buffer-byte, and it didn't match its corresponding savefile-byte...
				file.seekSet(pos + i); // Go to the savefile-byte's position
				file.write(b[i]); // Replace it with the buffer-byte
			}
		}

	}

	file.sync(); // Sync the data immediately, to prevent any unusual behavior between multiple conflicting note-writes

	delete [] buf; // Free the buffer's dynamically-allocated memory
	buf = NULL; // Unset the buffer's pointer

}

// Record a given MIDI command into the tempdata-file of the current RECORDSEQ sequence
void recordToSeq(word pstn, byte chan, byte b1, byte b2) {

	// Get the position of the first of this tick's bytes within the active track in the data-file
	unsigned long tickstart = (49UL + (4096UL * RECORDSEQ)) + (((unsigned long)pstn) * 8) + (TRACK * 4);

	// Construct a virtual MIDI command, with an additional DURATION value
	byte note[5] = {
		chan, // Channel-byte
		b1, // Pitch-byte
		byte(b2 * ((CHAN % 224) <= 191)), // Velocity-byte, or 0 if this is a 2-byte command
		byte((chan <= 15) * DURATION) // Duration-byte, or 0 if this is a non-NOTE-ON command
	};

	writeData(tickstart, 4, note, 0); // Write the note to its corresponding savefile position

	file.sync(); // Force the write-process to be committed to the savefile immediately

}

// Parse all of the possible actions that signal the recording of commands
void processRecAction(byte key) {

	// Get a note that corresponds to the key, organized from bottom-left to top-right, with all modifiers applied
	byte pitch = (OCTAVE * 12) + ((23 - key) ^ 3);
	while (pitch > 127) { // If the pitch is above 127, which is the limit for MIDI notes...
		pitch -= 12; // Reduce it by an octave until it is at or below 127
	}

	// Get the note's velocity, with a random humanize-offset
	byte velo = VELO - min(VELO - 1, byte(GLOBALRAND & 255) % (HUMANIZE + 1));

	if (PLAYING && RECORDNOTES) { // If notes are being recorded into a playing sequence...
		word qp = POS[RECORDSEQ]; // Will hold a QUANTIZE-modified insertion point (defaults to the seq's current position)
		if (!REPEAT) { // If REPEAT isn't toggled...
			char down = POS[RECORDSEQ] % QUANTIZE; // Get distance to previous QUANTIZE point
			byte up = QUANTIZE - down; // Get distance to next QUANTIZE point
			qp += (down <= up) ? (-down) : up; // Make the shortest distance into an offset for the note-insertion point
			qp %= word(STATS[RECORDSEQ] & 63) * 16; // Wrap the insertion-point around the seq's length
		}
		recordToSeq(qp, CHAN, pitch, velo); // Record the note into the current RECORDSEQ slot
		TO_UPDATE |= 252; // Flag the 6 bottommost LED-rows for an update
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

