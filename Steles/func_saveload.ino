
// Set a savefile or tempfile's header bytes to their default contents.
// Note: file must already be open before this function is called
void makeDefaultHeader() {
	file.seekSet(0); // Set write-position to the first byte
	file.write(112); // Write a default BPM byte at the start of the file
	for (uint8_t i = 1; i < 49; i++) { // For each seq's SIZE-byte in the header...
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
	uint8_t buf[33]; // Create a buffer of blank bytes to insert
	memset(buf, 0, sizeof(buf) - 1); // Ensure that the blank bytes are all zeroed out
	for (uint32_t i = 49; i < FILE_BYTES; i += 32) { // For every 32 bytes in the file...
		file.seekSet(i); // Go to the start of those 32 bytes
		file.write(EMPTY_TICK, 32); // Fill them with 32 empty bytes
	}
	file.close(); // Cose the file
}

// Load a given song, or create its files if they don't exist
void loadSong(uint8_t slot) {

	// Display an "L" while loading data
	lc.setRow(0, 0, 0);
	lc.setRow(0, 1, 0);
	lc.setRow(0, 2, 0);
	lc.setRow(0, 3, 64);
	lc.setRow(0, 4, 64);
	lc.setRow(0, 5, 64);
	lc.setRow(0, 6, 112);
	lc.setRow(0, 7, 0);

	haltAllSustains(); // Clear all currently-sustained notes
	resetAllSeqs(); // Reset all seqs' internal activity variables of all kinds

	// Get the names of the target song-slot's savefile and tempfile
	char name[7] = "XX.dat";
	char name2[7] = "XX.tmp";
	name[0] = char(floor(slot / 10));
	name[1] = slot % 10;
	name2[0] = name[0];
	name2[1] = name[1];

	// Create a small buffer for data-transfer between the savefile and tempfile
	uint8_t buf[17];

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
	file.read(BPM);
	temp.write(BPM);
	file.seekSet(1);
	temp.seekSet(1);
	file.read(STATS, 48);
	temp.write(STATS, 48);

	// Copy the savefile's body-bytes into the tempfile
	for (uint32_t b = 49; b < FILE_BYTES; b += 32) {
		file.seekSet(b);
		temp.seekSet(b);
		file.read(buf, 16);
		temp.write(buf, 16);
	}
	file.close();
	temp.close();

	// Set the currently-active SONG-position to the given save-slot
	SONG = slot;

	// Clear all metadata from the currently-loaded seqs
	resetAllSeqs();

	ABSOLUTETIME = micros(); // Adjust the absolute-time, to prevent backlogs of tempo-ticks from the saveload-lag
	CUR16 = 127; // Reset the global 16th-note position
	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}

// Save the active song's tempfile contents into a given song-slot's perma-files
void saveSong(uint8_t slot) {

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

	// Get the names of the target song-slot's savefile and tempfile
	int8_t name[7] = string(slot) + ".dat";
	int8_t name2[7] = string(SONG) + ".tmp";

	// Create a small buffer for data-transfer between the savefile and tempfile
	uint8_t buf[17];

	initializeSavefile(name); // Make sure the savefile exists, and is the correct size

	SdFile tempfile; // Create a second SdFile object for data-transfer

	file.open(name, O_WRITE); // Open the song's savefile
	tempfile.open(name2, O_WRITE); // Open the tempfile too

	// Write the program's header-data into both the savefile and tempfile
	file.seekSet(0);
	tempfile.seekSet(0);
	file.write(BPM);
	tempfile.write(BPM);
	file.seekSet(1);
	tempfile.seekSet(1);
	file.write(STATS, 48);
	tempfile.write(STATS, 48);

	tempfile.close(); // Close the tempfile (necessary to hop out of write-mode and into read-mode)

	tempfile.open(name2, O_READ); // Open the tempfile for reading

	// Copy the current tempfile's body-bytes into the given savefile slot
	for (uint32_t b = 49; b < 393265; b += 16) {
		file.seekSet(b);
		tempfile.seekSet(b);
		tempfile.read(buf, 16);
		file.write(buf, 16);
	}

	file.close(); // Close savefile
	tempfile.close(); // Close tempfile

	SONG = slot; // Set the currently-active SONG-position to the given save-slot

	ABSOLUTETIME = micros(); // Adjust the absolute-time, to prevent backlogs of tempo-ticks from the saveload-lag
	CUR16 = 127; // Reset the global 16th-note position
	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}
