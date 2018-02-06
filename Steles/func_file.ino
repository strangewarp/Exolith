
// Check whether savefiles exist, and create whichever ones don't exist
void createFiles() {

	char name[8]; // Will hold a given filename

	for (byte i = 0; i < 168; i++) { // For every song-slot...
		lc.setRow(0, 0, i); // Display how many files have been created so far, if any
		if (!(i % 32)) { // After a certain amount of files, switch to the next logo letter
			for (byte row = 0; row < 7; row++) { // For each row in the 7-row-tall logo text...
				// Set the corresponding row to the corresponding letter slice
				lc.setRow(0, row + 1, pgm_read_byte_near(LOGO + (row * 4) + (i >> 5)));
			}
		}
		getFilename(name, i); // Get the filename that corresponds to this song-slot
		if (sd.exists(name)) { continue; } // If the file exists, skip the file-creation process for this filename
		file.createContiguous(sd.vwd(), name, FILE_BYTES); // Create a contiguous file 
		file.close(); // Close the newly-created file
		file.open(name, O_WRITE); // Open the file explicitly in WRITE mode
		file.seekSet(0); // Go to byte 0
		file.write(byte(80)); // Write a default BPM value of 80
		for (byte j = 1; j < 49; j++) { // For every header-byte...
			file.seekSet(j); // Go to that byte's position
			file.write(byte(8)); // Write a default sequence-size value of 8
		}
		file.close(); // Close the file
	}

}

// Get the name of a target song-slot's savefile (format: "000.DAT" etc.)
void getFilename(char source[], byte fnum) {
	source[0] = char(floor(fnum / 100)) + 48;
	source[1] = (char(floor(fnum / 10)) % 10) + 48;
	source[2] = char((fnum % 10) + 48);
	source[3] = 46;
	source[4] = 68;
	source[5] = 65;
	source[6] = 84;
	source[7] = 0;
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

	char name[8];
	getFilename(name, slot); // Get the name of the target song-slot's savefile

	// Put the header-bytes from the savefile into the global BPM and STATS vars
	file.open(name, O_RDWR);
	BPM = file.read();
	file.read(STATS, 48);

	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value

	SONG = slot; // Set the currently-active SONG-position to the given save-slot

	CUR16 = 127; // Set the global cue-position to arrive at 1 immediately
	ELAPSED = 0; // Reset the elapsed-time-since-last-16th-note counter

	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}

