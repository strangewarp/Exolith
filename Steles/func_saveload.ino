
// Load a given song, or create its files if they don't exist
void loadSong(byte song) {

	// Display an "L" while loading data
	displayLetter("L");

	// Get the names of the target song-slot's savefile and tempfile
	string name = string(song) + ".dat";
	string name2 = string(song) + ".tmp";

	// Create a small buffer for data-transfer between the savefile and tempfile
	byte buf[33] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	if ( // If...
		(!file.exists(name)) // The savefile doesn't exist...
		|| (file.fileSize() != 1769472) // Or the file isn't the expected size...
	) { // Then recreate a blank datafile...
		file.open(name, O_CREAT | O_TRUNC | O_APPEND | O_WRITE); // Open a data-file with the given song's number
		// Fill file with body dummy-bytes totalling:
		// 3 (bytes) * 24 (ticks) * 128 (chunks) * 32 (seqs) * 2 (notes per tick) * 3 (bytes per note) = 1769472
		for (long i = 0; i < 1769472; i += 32) {
			file.write(buf, 32); // Empty dummy-bytes for all sequence contents
		}
		// Fill remainder of file with footer dummy-bytes totalling:
		// (32 (size) + 32 (exclude) + 32 (rand) + 32 (scatter) + 5 (active slice-seqs) + 1 (bpm))
		for (byte i = 0; i < 32; i++) {
			file.write(B01000000); // Seq-size bytes
		}
		for (byte i = 0; i < 96; i += 32) {
			file.write(buf, 32); // Misc seq-parameter bytes
		}
		file.write(DEFAULT_POS_BPM, 6); // Write the active-slice-seq bytes and the BPM byte
	}
	file.close();

	// Recreate the song's corresponding tempfile as an empty file
	file.open(name2, O_CREAT | O_TRUNC);
	file.close();

	// Copy the savefile's body-bytes into the tempfile
	for (long b = 0; b < 1769472; b += 32) {
		file.open(name, O_READ);
		file.seekSet(b);
		file.read(buf, 32);
		file.close();
		file.open(name2, O_WRITE | O_AT_END);
		file.write(buf, 32);
		file.close();
	}

	// Get the seq-parameters from the file's footer, and copy them into both the tempfile and program-vars
	file.open(name, O_READ);
	file.seekSet(1769472);
	file.read(SEQ_PSIZE, 32);
	file.read(SEQ_EXCLUDE, 32);
	file.read(SEQ_RAND, 32);
	file.read(SEQ_SCATTER, 32);
	file.read(SLICE_ROW, 5);
	file.read(BPM);
	file.close();
	file.open(name2, O_WRITE | O_APPEND);
	file.write(SEQ_PSIZE, 32);
	file.write(SEQ_EXCLUDE, 32);
	file.write(SEQ_RAND, 32);
	file.write(SEQ_SCATTER, 32);
	file.write(SLICE_ROW, 5);
	file.write(BPM);
	file.close();

	// Set the currently-active SONG-position to the given save-slot
	SONG = slot;

}

// Save the active song's tempfile contents into a given song-slot's perma-files
void saveSong(byte slot) {

	// Display an "S" while saving data
	displayLetter("S");

	string name = string(slot) + ".dat";
	string name2 = string(SONG) + ".tmp";
	byte buf[33];

	// Mask out the playing-and-position bits from each sequence's SIZE-AND-POS byte
	for (byte i = 0; i < 32; i++) {
		SEQ_SPOS[i] &= B11110000;
	}

	// Wipe the old savefile in the slot
	file.open(name, O_CREAT | O_WRITE | O_TRUNC);
	file.close();

	// Copy the current tempfile's body-bytes into the given savefile slot
	for (long b = 0; b < 1769472; b += 32) {
		file.open(name2, O_READ);
		file.seekSet(b);
		file.read(buf, 32);
		file.close();
		file.open(name, O_WRITE | O_AT_END);
		file.write(buf, 32);
		file.close();
	}

	// Create a new footer based on current sequence-variables
	file.open(name, O_WRITE | O_APPEND);
	file.write(SEQ_PSIZE, 32);
	file.write(SEQ_EXCLUDE, 32);
	file.write(SEQ_RAND, 32);
	file.write(SEQ_SCATTER, 32);
	file.write(SLICE_ROW, 5);
	file.write(BPM);
	file.close();

	// Set the currently-active SONG-position to the given save-slot
	SONG = slot;

}
