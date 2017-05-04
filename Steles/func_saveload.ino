
// Load a given song, or create its files if they don't exist
void loadSong(byte song) {

	// Display an "L" while loading data
	displayLetter("L");

	// Clear all currently-sustained notes
	haltAllSustains();

	// Get the names of the target song-slot's savefile and tempfile
	string name = string(song) + ".dat";
	string name2 = string(song) + ".tmp";

	// Create a small buffer for data-transfer between the savefile and tempfile
	byte buf[17] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	if ( // If...
		(!file.exists(name)) // The savefile doesn't exist...
		|| (file.fileSize() != FILE_BYTES) // Or the file isn't the expected size...
	) { // Then...
		file.createContiguous(name, FILE_BYTES); // Recreate and open a blank datafile
		file.seekSet(0); // Set write-position to the first byte
		file.write(120); // Write a default BPM byte at the start of the file
		file.close(); // Close the file
	}

	// Recreate the song's corresponding tempfile as an empty file
	file.open(name2, O_CREAT | O_TRUNC);
	file.close();

	// Copy the savefile's body-bytes into the tempfile
	for (unsigned long b = 73; b < FILE_BYTES; b += 16) {
		file.open(name, O_READ);
		file.seekSet(b);
		file.read(buf, 16);
		file.close();
		file.open(name2, O_WRITE | O_AT_END);
		file.write(buf, 16);
		file.close();
	}

	// Get the seq-parameters from the file's header, and copy them into both the tempfile and program-vars
	file.open(name, O_READ);
	file.read(BPM);
	file.read(SEQ_SIZE, 72);
	file.close();
	file.open(name2, O_WRITE | O_APPEND);
	file.write(BPM);
	file.write(SEQ_SIZE, 72);
	file.close();

	// Set the currently-active SONG-position to the given save-slot
	SONG = slot;

	// Clear all metadata from the currently-loaded seqs
	resetSeqs();

	// Adjust the absolute-time, to prevent backlogs of tempo-ticks from the saveload-lag
	ABSOLUTETIME = micros();

}

// Save the active song's tempfile contents into a given song-slot's perma-files
void saveSong(byte slot) {

	// Display an "S" while saving data
	displayLetter("S");

	// Clear all currently-sustained notes
	haltAllSustains();

	string name = string(slot) + ".dat";
	string name2 = string(SONG) + ".tmp";
	byte buf[33];

	// Create a new savefile, with a new header based on current sequence-variables
	file.open(name, O_CREAT | O_WRITE | O_APPEND);

	// Write the header bytes
	file.write(BPM);
	file.write(SEQ_SIZE, 72);

	// Do some time magic
	file.timestamp(T_CREATE, 1987, 5, 12, 4, 20, 7);

	// Close the new savefile
	file.close();

	// Copy the current tempfile's body-bytes into the given savefile slot
	for (unsigned long b = 73; b < 7077961; b += 32) {
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

	// Adjust the absolute-time, to prevent backlogs of tempo-ticks from the saveload-lag
	ABSOLUTETIME = micros();

}
