
// Load a given song, or create its files if they don't exist
void loadSong(byte song) {

	// Display an "L" while loading data
	displayLetter("L");

	// Get the names of the target song-slot's savefile and tempfile
	string name = string(song) + ".dat";
	string name2 = string(song) + ".tmp";

	// Create a small buffer for data-transfer between the savefile and tempfile
	byte buf[33];

	if (!file.exists(name)) { // If the savefile doesn't exist...
		file.open(name, O_CREAT | O_APPEND | O_WRITE); // Open a data-file with the given song's number
		// Fill file with body dummy-bytes totalling:
		// 3 (bytes) * 96 (ticks) * 4 (beats) * 16 (measures) * 32 (seqs) = 589824
		for (long i = 0; i < 589824; i++) {
			file.write(0); // Empty dummy-bytes for all sequence contents
		}
		// Fill remainder of file with footer dummy-bytes totalling:
		// (32 (exclude) + 32 (random) + 32 (scatter) + 32 (size) + 5 (active slice-seqs) + 1 (bpm))
		for (byte i = 0; i < 96; i++) {
			file.write(0); // Misc seq-parameter bytes
		}
		for (byte i = 0; i < 32; i++) {
			file.write(B01000000); // Seq-size bytes
		}
		file.write(DEFAULT_POS_BPM, 6); // Write the active-slice-seq bytes and the BPM byte
	}
	file.close();

	// Recreate the song's corresponding tempfile as an empty file
	file.open(name2, O_CREAT | O_TRUNC);
	file.close();

	// Copy the savefile's body-bytes into the tempfile
	for (long b = 0; b < 589824; b += 32) {
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
	file.seekSet(589824);
	file.read(SEQ_EXCLUDE, 32);
	file.read(SEQ_RAND, 32);
	file.read(SEQ_SCATTER, 32);
	file.read(SEQ_SPOS, 32);
	file.read(SLICE_ROW, 5);
	file.read(BPM);
	file.close();
	file.open(name2, O_WRITE | O_APPEND);
	file.write(SEQ_EXCLUDE, 32);
	file.write(SEQ_RAND, 32);
	file.write(SEQ_SCATTER, 32);
	file.write(SEQ_SPOS, 32);
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
	for (long b = 0; b < 589824; b += 32) {
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
	file.write(SEQ_EXCLUDE, 32);
	file.write(SEQ_RAND, 32);
	file.write(SEQ_SCATTER, 32);
	file.write(SEQ_SPOS, 32);
	file.write(SLICE_ROW, 5);
	file.write(BPM);
	file.close();

	// Set the currently-active SONG-position to the given save-slot
	SONG = slot;

}

// TODO change size of savefiles to compensate for 4-part polyphony per sequence, plus 4-byte note values
