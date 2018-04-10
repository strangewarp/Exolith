

// Check whether a given RECORD-MODE keychord is NOTE-RECORDING compatible
byte isRecCompatible(byte ctrl) {
	// Return 1 if:
	return (!ctrl) // No command-buttons are held...
		|| (ctrl == B00100100) // Or CONTROL-CHANGE is held...
		|| (ctrl == B00100010); // Or PROGRAM-CHANGE is held
	// ...Otherwise, return 0
}

// Prime a newly-entered RECORD-MODE sequence for editing
void primeRecSeq() {

	resetSeq(RECORDSEQ); // If the most-recently-touched seq is already playing, reset it to prepare for timing-wrapping
	SCATTER[RECORDSEQ] = 0; // Unset the most-recently-touched seq's SCATTER values before starting to record

	// Set the seq's position to correspond with the current global 16th-note,
	// wrapped to either the seq's size, or the global cue's size, whichever is smaller
	POS[RECORDSEQ] = (CUR16 + 1) % (word(min(STATS[RECORDSEQ] & 63, 8)) * 16);

	STATS[RECORDSEQ] |= 128; // Set the sequence to active, if it isn't already

}

// Erase all commands that match the global CHAN, in a given tick-position within the RECORDSEQ
void eraseTick(byte buf[], byte p) {

	// Get the tick's position in the savefile
	unsigned long tpos = (49UL + (p * 8)) + (4096UL * RECORDSEQ);

	file.seekSet(tpos); // Navigate to the note's absolute position
	file.read(buf, 8); // Read the data of the tick's notes

	byte full = buf[0] || buf[2]; // Check if the tick's top note is filled
	if (!full) { return; } // If no notes or CCs are present, exit the function

	// Check if the first and/or second note matches the global CHAN
	byte pos1 = full && ((buf[0] & 15) == CHAN);
	byte pos2 = (buf[4] || buf[6]) && ((buf[4] & 15) == CHAN);

	if (pos1 || pos2) { // If either note matches the global chan...
		byte outbuf[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // Make a tick-sized buffer to send blank data
		file.seekSet(tpos + ((!pos1) * 4)); // Set the write-position to whichever note matched
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

}

// Parse all of the possible actions that signal the recording of commands
void processRecAction(byte key) {

	// Get a note that corresponds to the key, organized from bottom-left to top-right, with all modifiers applied
	byte pitch = (OCTAVE * 12) + ((23 - key) ^ 3);
	while (pitch > 127) { // If the pitch is above 127, which is the limit for MIDI notes...
		pitch -= 12; // Reduce it by 12 until it is at or below 127
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

		// Record the note into the current RECORDSEQ slot
		recordToSeq(qp, CHAN, pitch, velo);

		TO_UPDATE |= 252; // Flag the 6 bottommost LED-rows for an update

	}

	// If this wasn't a CC or PC command,
	// update SUSTAIN buffer in preparation for playing the user-pressed note
	if (CHAN < 16) {
		clipBottomSustain(); // If the bottommost SUSTAIN is filled, send its NOTE-OFF prematurely
		memmove(SUST + 3, SUST, SUST_COUNT * 3); // Move all sustains one space downward
		SUST[0] = 128 + CHAN; // Fill the topmost sustain-slot with the user-pressed note
		SUST[1] = pitch; // ^
		SUST[2] = DURATION; // ^
		SUST_COUNT++; // Increase the number of active sustains, to accurately reflect the new note
	}

	byte offc = CHAN >> 4; // Get an offset-value for the CHAN byte
	offc = (offc + (!!offc)) * 16; // Convert the offset into a CC or PC offset, or no offset for a NOTE

	// Build a correctly-formatted NOTE-ON for the user-pressed note;
	// or a correctly-formatted CC or PC command, if applicable.
	byte buf[4] = {byte(144 + CHAN + offc), pitch, velo, 0};
	Serial.write(buf, 3); // Send the user-pressed note to MIDI-OUT immediately, without buffering

}

// Record a given MIDI command into the tempdata-file of the current RECORDSEQ sequence
void recordToSeq(word pstn, byte chan, byte b1, byte b2) {

	byte buf[9]; // SD-card read/write buffer

	// Get the position of the first of this tick's bytes in the data-file
	unsigned long tickstart = (49UL + (4096UL * RECORDSEQ)) + (((unsigned long)pstn) * 8);

	// Get the tick's note-slots, and check whether any of them are empty
	file.seekSet(tickstart);
	file.read(buf, 8);

	// If none of the note-slots are empty, then shift their contents downward by one note, deleting the lowest item
	if (buf[7]) {
		file.seekSet(tickstart + 4);
		file.write(buf, 4);
	}

	// Set the insert-point to the highest unfilled note in the tick;
	// or to the first note in the tick, if the contents were shifted downward
	file.seekSet(tickstart + (((!!(buf[0] | buf[2])) ^ (!!(buf[4] | buf[6]))) * 4));

	// Construct a virtual MIDI command, with an additional DURATION value, in the write buffer
	buf[0] = chan;
	buf[1] = b1;
	buf[2] = b2;
	buf[3] = (chan <= 15) * DURATION; // Only include the DURATION if this isn't a flagged special-command

	file.write(buf, 4); // Write the note to the current tempdata

	file.sync(); // Force this function's write processes to be committed to the savefile immediately

}

