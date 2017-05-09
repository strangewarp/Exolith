
// Record a given MIDI command into the tempdata-file of the current RECORDSEQ sequence
void recordToSeq(int offset, byte cmd, byte chan, byte b1, byte b2) {

	// Create a virtual tick-position that compensates for any given tick-offset
	word tick = ((SEQ_POS[RECORDSEQ] & B0111111111111111) + offset) % (SEQ_SIZE[RECORDSEQ] * 96);

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
