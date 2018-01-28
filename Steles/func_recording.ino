
// Prime a newly-entered RECORD-MODE sequence for editing
void primeRecSeq() {
	resetSeq(RECORDSEQ); // If the most-recently-touched seq is already playing, reset it to prepare for timing-wrapping
	SCATTER[RECORDSEQ] = 0; // Unset the most-recently-touched seq's SCATTER values before starting to record
	word seqsize = word(STATS[RECORDSEQ] & B00111111) << 4; // Get sequence's size in 16th-notes
	POS[RECORDSEQ] = (seqsize > 127) ? CUR16 : (CUR16 % seqsize); // Wrap sequence around to global cue-point
	STATS[RECORDSEQ] |= 128; // Set the sequence to active
	RECPOS = 0; // Reset the STEP-EDIT position
}

// Record a given MIDI command into the tempdata-file of the current RECORDSEQ sequence
void recordToSeq(word pos, byte chan, byte b1, byte b2) {

	byte buf[9] = {0, 0, 0, 0, 0, 0, 0, 0}; // SD-card read/write buffer

	// Get the position of the first of this tick's bytes in the data-file
	unsigned long tickstart = (49UL + (8192UL * RECORDSEQ)) + (((unsigned long)pos) * 8);

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
	file.seekSet(tickstart + (((!!buf[3]) ^ (!!buf[7])) << 2));

	// Construct a virtual MIDI command, with an additional DURATION value, in the write buffer
	buf[0] = chan;
	buf[1] = b1;
	buf[2] = b2;
	buf[3] = (chan <= 15) * DURATION; // Only include the DURATION if this isn't a flagged special-command

	file.write(buf, 4); // Write the note to the current tempdata

	file.sync(); // Force this function's write processes to be committed to the savefile immediately

}
