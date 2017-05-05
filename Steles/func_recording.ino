
// Record a given MIDI command into the tempdata-file of the seq in the topmost slice-row
void recordToSeq(boolean usercmd, byte cmd, byte chan, byte b1, byte b2) {

	// Create a virtual tick-position that may or may not be changed before the note-insert process
	word tick = SEQ_POS[RECORDSEQ] & B0111111111111111;

	if (usercmd) { // If this is a user-command (as opposed to a command from an external MIDI device)...

		// Make the tick adhere to the time-quantize value;
		// this will be the tick-position where the command gets placed in the sequence
		word qmod = max(1, (QUANTIZE >> 1) * 3);
		word down = tick % qmod;
		word up = qmod - down;
		tick += (down <= up) ? -down : up;

		// If this is a NOTE-ON command, then apply basenote/octave/humanize modifiers
		if (cmd == 144) {

			// Apply base-note offset, and octave offset, to the note's pitch-byte
			b1 = min(127, b1 + BASENOTE + (OCTAVE * 12));

			// Apply a random humanize-offset to the note's velocity-byte
			b2 = min(127, max(1, b2 + (byte(HUMANIZE / 2) - rand(HUMANIZE))));

		}

	}

	byte buf[17]; // SD-card read/write buffer
	byte toinsert = 255; // Tracks which empty slot to insert the note into

	// Get the position of the start of this tick's notes, in the data-file
	unsigned long tickstart = 73 + (98304 * RECORDSEQ) + (tick * 16);

	// Open the current tempdata for reading and writing
	file.open(string(SONG) + ".tmp", O_RDWR);

	// Get the tick's note-slots, and check whether any of them are empty
	file.seekSet(tickstart);
	file.read(buf, 16);
	for (byte i = 0; i < 4; i++) {
		if (buf[(i << 2) + 1] == 255) {
			toinsert = i;
			break;
		}
	}

	// If none of the note-slots are empty, then shift their contents downward by one note, deleting the lowest item
	if (toinsert == 255) {
		file.seekSet(tickstart + 4);
		file.write(buf, 12);
		toinsert = 0;
	}

	// Set the insert-point to the highest unfilled note
	file.seekSet(tickstart + (toinsert * 4));

	// Construct a MIDI note, with an additional DURATION value, in the write buffer
	buf = {DURATION, cmd | chan, b1, b2};

	// Write the note to the current tempdata
	file.write(buf, 4);

	// Close the file, to finish the write process
	file.close();

}
