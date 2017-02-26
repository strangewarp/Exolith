
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
		// Fill it with dummy-bytes totalling:
		// (32 (exclude) + 32 (random) + 32 (scatter) + 16 (size) + 5 (active slice-seqs) + 1 (bpm))
		// + (3 (bytes) * 96 (ticks) * 4 (beats) * 16 (measures) * 32 (seqs))
		for (byte i = 0; i < 96; i++) {
			file.write(0); // Misc seq-parameter bytes
		}
		for (byte i = 0; i < 16; i++) {
			file.write(B01000100); // Seq-size bytes
		}
		file.write(DEFAULT_POS_BPM, 6); // Write the active-slice-seq bytes and the BPM byte
		for (long i = 0; i < 589824; i++) {
			file.write(0); // Empty dummy-bytes for all sequence contents
		}
	}
	file.close();

	// Recreate the song's corresponding tempfile as an empty file
	file.open(name2, O_CREAT | O_TRUNC);
	file.close();

	// Get the seq-parameters from the file's header, and copy them into both the tempfile and program-vars
	file.open(name, O_READ);
	file.read(SEQ_EXCLUDE, 32);
	file.read(SEQ_RAND, 32);
	file.read(SEQ_SCATTER, 32);
	file.read(SEQ_SIZE, 16);
	file.read(SLICE_ROW, 5);
	file.read(BPM);
	file.close();
	file.open(name2, O_WRITE | O_APPEND);
	file.write(SEQ_EXCLUDE, 32);
	file.write(SEQ_RAND, 32);
	file.write(SEQ_SCATTER, 32);
	file.write(SEQ_SIZE, 16);
	file.write(SLICE_ROW, 5);
	file.write(BPM);
	file.close();

	// Copy the savefile's body-bytes into the tempfile
	for (long b = 118; b < 589942; b += 32) {
		file.open(name, O_READ);
		file.seekSet(b);
		file.read(buf, 32);
		file.close();
		file.open(name2, O_WRITE | O_AT_END);
		file.write(buf, 32);
		file.close();
	}

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

	// Wipe the old savefile in the slot, and create a new header based on current sequence-variables
	file.open(name, O_WRITE | O_TRUNC | O_APPEND);
	file.write(SEQ_EXCLUDE, 32);
	file.write(SEQ_RAND, 32);
	file.write(SEQ_SCATTER, 32);
	file.write(SEQ_SIZE, 16);
	file.write(SLICE_ROW, 5);
	file.write(BPM);
	file.close();

	// Copy the current tempfile's body-bytes into the given savefile slot
	for (long b = 118; b < 589942; b += 32) {
		file.open(name2, O_READ);
		file.seekSet(b);
		file.read(buf, 32);
		file.close();
		file.open(name, O_WRITE | O_AT_END);
		file.write(buf, 32);
		file.close();
	}

	// Set the currently-active SONG-position to the given save-slot
	SONG = slot;

}
