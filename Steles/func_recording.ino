
// Check whether a given RECORD-MODE keychord is NOTE-RECORDING compatible
byte isRecCompatible(byte ctrl) {
	// Return 1 if:
	return (!ctrl) // No command-buttons are held...
		|| (ctrl == B00110000) // Or INTERVAL-DOWN is held...
		|| (ctrl == B00101000) // Or INTERVAL-UP is held...
		|| (ctrl == B00100100) // Or CONTROL-CHANGE is held...
		|| (ctrl == B00100010) // Or PROGRAM-CHANGE is held...
		|| (ctrl == B00110100) // Or INTERVAL-DOWN-RANDOM is held...
		|| (ctrl == B00101100); // Or INTERVAL-UP-RANDOM is held
	// ...Otherwise, return 0
}

// Prime a newly-entered RECORD-MODE sequence for editing
void primeRecSeq() {
	resetSeq(RECORDSEQ); // If the most-recently-touched seq is already playing, reset it to prepare for timing-wrapping
	SCATTER[RECORDSEQ] = 0; // Unset the most-recently-touched seq's SCATTER values before starting to record
	word seqsize = word(STATS[RECORDSEQ] & B00111111) << 4; // Get sequence's size in 16th-notes
	POS[RECORDSEQ] = (seqsize > 127) ? CUR16 : (CUR16 % seqsize); // Wrap sequence around to global cue-point
	STATS[RECORDSEQ] |= 128; // Set the sequence to active
}

// Erase all commands that match the global CHAN, in the current tick of the current RECORDSEQ
void eraseTick(byte buf[]) {

	// Get the tick's position in the savefile
	unsigned long tpos = (49UL + (POS[RECORDSEQ] * 8)) + (8192UL * RECORDSEQ);

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

}

// Parse all of the possible actions that signal the recording of commands
void processRecAction(byte ctrl, byte key) {

	// Get the interval-buttons' activity specifically,
	// while weeding out any false-positives from commands that don't hold the INTERVAL button (B00000001)
	byte ibs = (ctrl & 7) * ((ctrl <= 7) && (ctrl & 1));

	// Get a note that corresponds to the key, organized from bottom-left to top-right, with all modifiers applied
	byte pitch = (OCTAVE * 12) + BASENOTE + ((23 - key) ^ 3);
	while (pitch > 127) { // If the pitch is above 127, which is the limit for MIDI notes...
		pitch -= 12; // Reduce it by 12 until it is at or below 127
	}

	// Get the note's velocity, with a random humanize-offset
	byte velo = VELO - min(VELO - 1, byte(GLOBALRAND % (HUMANIZE + 1)));

	byte rchan = CHAN & 15; // Strip the chan of any special-command flag bits

	if (ibs) { // If any INTERVAL-keys are held...
		// Make a composite INTERVAL command, as it would appear in data-storage,
		// and apply the INTERVAL command to the channel's most-recent pitch
		ibs = ((ibs & 1) << 7) | ((ibs & 2) << 5) | ((ibs & 4) << 3);
		pitch = applyIntervalCommand(ibs | key, RECENT[rchan]);
	}

	if (ibs || (rchan == CHAN)) { // If INTERVAL keys are held, or this is a normal NOTE...
		RECENT[rchan] = pitch; // Update the channel's most-recent note to the pitch-value
	}

	if (PLAYING && RECORDNOTES) { // If notes are being recorded into a playing sequence...
		// Record the note, either natural or modified by INTERVAL, into the current RECORDSEQ slot;
		// and if this is an INTERVAL-command, then clip off any virtual CC-info from the chan-byte 
		recordToSeq(
			POS[RECORDSEQ] - (POS[RECORDSEQ] % QUANTIZE), // Get a rounded-down insert-point, based on QUANTIZE
			ibs ? (CHAN % 16) : CHAN,
			pitch,
			velo
		);
		// Incerase the record-position by QUANTIZE amount, wrapping around the end of the sequence if applicable
		TO_UPDATE |= 254; // Flag the 7 bottommost LED-rows for an update
	}

	if (CHAN & 16) { return; } // If this was a virtual CC command, forego all SUSTAIN operations

	// Update SUSTAIN buffer in preparation for playing the user-pressed note
	clipBottomSustain(); // If the bottommost SUSTAIN is filled, send its NOTE-OFF prematurely
	memmove(SUST + 3, SUST, SUST_COUNT * 3); // Move all sustains one space downward
	SUST[0] = 128 + CHAN; // Fill the topmost sustain-slot with the user-pressed note
	SUST[1] = pitch; // ^
	SUST[2] = DURATION; // ^
	SUST_COUNT++; // Increase the number of active sustains, to accurately reflect the new note
	byte buf[4] = {byte(144 + CHAN), pitch, velo}; // Build a correctly-formatted NOTE-ON for the user-pressed note
	Serial.write(buf, 3); // Send the user-pressed note to MIDI-OUT immediately, without buffering

	TO_UPDATE |= 2; // Flag the sustain-row for a GUI update

}

// Record a given MIDI command into the tempdata-file of the current RECORDSEQ sequence
void recordToSeq(word pstn, byte chan, byte b1, byte b2) {

	byte buf[9]; // SD-card read/write buffer

	// Get the position of the first of this tick's bytes in the data-file
	unsigned long tickstart = (49UL + (8192UL * RECORDSEQ)) + (((unsigned long)pstn) * 8);

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

