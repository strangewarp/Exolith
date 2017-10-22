
// Get the name of a target song-slot's savefile
void getFilename(char source[], byte fnum) {
	byte rem = fnum % 10;
	source[0] = char(rem + 48);
	source[1] = char((fnum - rem) + 48);
	source[2] = 46;
	source[3] = 68;
	source[4] = 65;
	source[5] = 84;
	source[6] = 0;
}

// Update a given byte within a given song-file.
// This is used to update BPM and SEQ-SIZE bytes in the header
void updateFileByte(byte pos, byte b) {
	file.seekSet(pos); // Go to the given insert-point
	file.write(b); // Write the new byte into the file
	file.sync(); // Make sure the changes are recorded
}

// Make sure a given savefile exists, and is the correct size
void initializeFile(char name[7]) {
	if ( // If...
		(!file.exists(name)) // The savefile doesn't exist...
		|| (file.fileSize() != FILE_BYTES) // Or the file isn't the expected size...
	) { // Then...
		file.createContiguous(name, FILE_BYTES); // Recreate and open a blank datafile
		file.close(); // Close file after creating it
		//file.timestamp( // Do some retrocomputing magic
		//	T_ACCESS | T_CREATE | T_WRITE,
		//	random(1981, 1993),
		//	random(1, 13),
		//	random(1, 29),
		//	random(0, 24),
		//	random(0, 60),
		//	random(0, 60)
		//);
		file.open(name, O_WRITE); // Reopen the newly-created file in write-mode
		file.seekSet(0); // Set write-position to the first byte
		file.write(112); // Write a default BPM byte at the start of the file
		for (byte i = 1; i < 49; i++) { // For each seq's SIZE-byte in the header...
			file.seekSet(i); // Go to the next byte location
			file.write(8); // Set the seq's default size to 8 beats
		}
		file.close(); // Close the file
	}
}

// Load a given song, or create its savefile if it doesn't exist
void loadSong(byte slot) {

	haltAllSustains(); // Clear all currently-sustained notes
	resetAllSeqs(); // Reset all seqs' internal activity variables of all kinds
	clearRecentNotes(); // Reset the "most recent note by channel" array

	// Display a fully-lit screen while loading data
	lc.setRow(0, 0, 255);
	lc.setRow(0, 1, 255);
	lc.setRow(0, 2, 255);
	lc.setRow(0, 3, 255);
	lc.setRow(0, 4, 255);
	lc.setRow(0, 5, 255);
	lc.setRow(0, 6, 255);
	lc.setRow(0, 7, 255);
	delay(10); // Wait for a long enough time for the screen-flash to be visible

	if (file.isOpen()) { // If a savefile is already open...
		file.close(); // Close it
	}

	char name[7];
	getFilename(name, slot); // Get the name of the target song-slot's savefile
	initializeFile(name); // Make sure the savefile exists, and is the correct size

	// Put the header-bytes from the savefile into the global BPM and STATS vars
	file.open(name, O_RDWR);
	BPM = file.read();
	file.read(STATS, 48);

	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value

	SONG = slot; // Set the currently-active SONG-position to the given save-slot

	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}
