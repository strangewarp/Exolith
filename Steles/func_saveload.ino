
// Get the name of a target song-slot's savefile or tempfile,
// either ".dat" or ".tmp" depending on whether isdt is 1 or 0
void getFilename(char source[], byte fnum, byte isdt) {
	byte rem = fnum % 10;
	source[0] = char(rem + 48);
	source[1] = char((fnum - rem) + 48);
	source[2] = 46;
	source[3] = isdt ? 100 : 116;
	source[4] = isdt ? 97 : 109;
	source[5] = isdt ? 116 : 112;
}

// Set a savefile or tempfile's header bytes to their default contents.
// Note: file must already be open before this function is called
void makeDefaultHeader() {
	file.seekSet(0); // Set write-position to the first byte
	file.write(112); // Write a default BPM byte at the start of the file
	for (byte i = 1; i < 49; i++) { // For each seq's SIZE-byte in the header...
		file.seekSet(i); // Go to the next byte location
		file.write(8); // Set the seq's default size to 8 beats
	}
}

// Make sure a given savefile exists, and is the correct size
void initializeSavefile(char name[7]) {
	if ( // If...
		(!file.exists(name)) // The savefile doesn't exist...
		|| (file.fileSize() != FILE_BYTES) // Or the file isn't the expected size...
	) { // Then...
		file.createContiguous(name, FILE_BYTES); // Recreate and open a blank datafile
		file.close(); // Close file after creating it
		file.open(name, O_WRITE); // Reopen the newly-created file in write-mode
		makeDefaultHeader(); // Fill the file's header-bytes with their default contents
		file.close(); // Close the file
	}
}

// Zero out the contents of an already-existing file, and give it default header contents
void clearFile(char name[7]) {
	file.open(name, O_WRITE);
	makeDefaultHeader(); // Fill the file's header-bytes with their default contents
	byte buf[33]; // Create a buffer of blank bytes to insert
	memset(buf, 0, sizeof(buf) - 1); // Ensure that the blank bytes are all zeroed out
	for (unsigned long i = 49; i < FILE_BYTES; i += 32) { // For every 32 bytes in the file...
		file.seekSet(i); // Go to the start of those 32 bytes
		file.write(buf, 32); // Fill them with 32 empty bytes
	}
	file.close(); // Cose the file
}

// Load a given song, or create its files if they don't exist
void loadSong(byte slot) {

	haltAllSustains(); // Clear all currently-sustained notes
	resetAllSeqs(); // Reset all seqs' internal activity variables of all kinds

	// Display an "L" while loading data
	lc.setRow(0, 0, 0);
	lc.setRow(0, 1, 0);
	lc.setRow(0, 2, 0);
	lc.setRow(0, 3, 64);
	lc.setRow(0, 4, 64);
	lc.setRow(0, 5, 64);
	lc.setRow(0, 6, 112);
	lc.setRow(0, 7, 0);

	// Get the names of the target song-slot's savefile and tempfile
	char name[7];
	char name2[7];
	getFilename(name, slot, 1);
	getFilename(name2, slot, 0);

	// Create a small buffer for data-transfer between the savefile and tempfile
	byte buf[17];

	if (file.isOpen()) { // If a tempfile is still open and in use...
		file.close(); // Close it
	}

	initializeSavefile(name); // Make sure the savefile exists, and is the correct size

	// Recreate the song's corresponding tempfile:
	if (file.exists(name2)) { // If an old tempfile exists...
		clearFile(name2); // Empty out the old tempfile's contents
	} else { // Else, if a tempfile doesn't exist for this save-slot...
		initializeSavefile(name2); // Create this save-slot's tempfile
	}

	SdFile temp; // Create a temporary second SdFile object for data-transfer

	// Open both the savefile and the tempfile
	file.open(name, O_READ);
	temp.open(name2, O_WRITE);

	// Put the header-bytes from the savefile into the program RAM, and then into the tempfile
	file.seekSet(0);
	temp.seekSet(0);
	BPM = file.read();
	temp.write(BPM);
	file.seekSet(1);
	temp.seekSet(1);
	file.read(STATS, 48);
	temp.write(STATS, 48);

	// Copy the savefile's body-bytes into the tempfile
	for (unsigned long b = 49; b < FILE_BYTES; b += 32) {
		file.seekSet(b);
		temp.seekSet(b);
		file.read(buf, 16);
		temp.write(buf, 16);
	}

	file.close();
	temp.close();

	// Open the song's tempfile, and leave it open, for all read/write operations during general use
	file.open(name2, O_RDWR);

	// Set the currently-active SONG-position to the given save-slot
	SONG = slot;

	// Clear all metadata from the currently-loaded seqs
	resetAllSeqs();

	ABSOLUTETIME = micros(); // Adjust the absolute-time, to prevent backlogs of tempo-ticks from the saveload-lag
	CUR16 = 127; // Reset the global 16th-note position
	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}

// Save the active song's tempfile contents into a given song-slot's perma-files
void saveSong(byte slot) {

	// Display an "S" while saving data
	lc.setRow(0, 0, 0);
	lc.setRow(0, 1, 0);
	lc.setRow(0, 2, B01100000);
	lc.setRow(0, 3, B10010000);
	lc.setRow(0, 4, B01000000);
	lc.setRow(0, 5, B00100000);
	lc.setRow(0, 6, B10010000);
	lc.setRow(0, 7, B01100000);

	haltAllSustains(); // Clear all currently-sustained notes
	resetAllSeqs(); // Reset all seqs' internal activity variables of all kinds

	char name[7];
	char name2[7];
	getFilename(name, slot, 1);
	getFilename(name2, SONG, 0);

	// Create a small buffer for data-transfer between the savefile and tempfile
	byte buf[17];

	initializeSavefile(name); // Make sure the savefile exists, and is the correct size

	SdFile temp; // Create a second SdFile object for data-transfer

	file.open(name, O_WRITE); // Open the song's savefile
	temp.open(name2, O_WRITE); // Open the tempfile too

	// Write the program's header-data into both the savefile and tempfile
	file.seekSet(0);
	temp.seekSet(0);
	file.write(BPM);
	temp.write(BPM);
	file.seekSet(1);
	temp.seekSet(1);
	file.write(STATS, 48);
	temp.write(STATS, 48);

	temp.close(); // Close the tempfile (necessary to hop out of write-mode and into read-mode)

	temp.open(name2, O_READ); // Open the tempfile for reading

	// Copy the current tempfile's body-bytes into the given savefile slot
	for (unsigned long b = 49; b < 393265; b += 16) {
		file.seekSet(b);
		temp.seekSet(b);
		temp.read(buf, 16);
		file.write(buf, 16);
	}

	file.close(); // Close savefile
	temp.close(); // Close tempfile

	SONG = slot; // Set the currently-active SONG-position to the given save-slot

	ABSOLUTETIME = micros(); // Adjust the absolute-time, to prevent backlogs of tempo-ticks from the saveload-lag
	CUR16 = 127; // Reset the global 16th-note position
	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}
