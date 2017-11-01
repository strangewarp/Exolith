
// Check the SD-card filesystem for correct formatting, or throw an error if incorrect
void checkFilesystem() {
	for (byte i = 0; i < 48; i++) { // For every savefile slot...
		char name[7];
		getFilename(name, i); // Get the name of a given savefile
		if ( // If...
			(!file.exists(name)) // The savefile doesn't exist...
			|| (file.fileSize() != FILE_BYTES) // Or the file isn't the expected size...
		) { // Then...
			lc.setRow(0, 2, B11101110); // Throw a visible error-message: "FS" for "filesystem"
			lc.setRow(0, 3, B10001000);
			lc.setRow(0, 4, B11101110);
			lc.setRow(0, 5, B10000010);
			lc.setRow(0, 6, B10000010);
			lc.setRow(0, 7, B10001110);
			// Intentionally hang the program in an infinite loop, to stop the bad filesystem from being used
			while (1) { }
		}
	}
}

// Get the name of a target song-slot's savefile (format: "00.DAT" etc.)
void getFilename(char source[], byte fnum) {
	source[0] = char(floor(fnum / 10)) + 48;
	source[1] = char((fnum % 10) + 48);
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

	// Put the header-bytes from the savefile into the global BPM and STATS vars
	file.open(name, O_RDWR);
	BPM = file.read();
	file.read(STATS, 48);

	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value

	SONG = slot; // Set the currently-active SONG-position to the given save-slot

	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}
