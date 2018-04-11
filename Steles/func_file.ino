

// Check whether savefiles exist, and create whichever ones don't exist
void createFiles() {

	char name[8]; // Will hold a given filename

	for (byte i = 0; i < 168; i++) { // For every song-slot...
		//lc.setRow(0, 0, i); // Display how many files have been created so far, if any
		if (!(i % 21)) { // After a certain amount of files, switch to the next logo letter
			for (byte row = 0; row < 7; row++) { // For each row in the 7-row-tall logo text...
				// Set the corresponding row to the corresponding letter slice
				lc.setRow(0, row + 1, pgm_read_byte_near(LOGO + (row * 4) + ((i / 21) % 4)));
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
	source[0] = char(fnum >= 100) + 48;
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

// Helper function for writeData:
// Write a number of buffered bytes to the file, based on the value of an iterator,
//     and how many bytes it's been since the last matching byte
byte wdSince(
	unsigned long pos, // Base position for the write
	byte since, // Number of bytes that have elapsed since the last matching-byte
	byte i, // Current iterator value
	byte b[] // Buffer of bytes to write
) {
	if (since) { // If it has been at least 1 byte since the last matching byte...
		byte wbot = i - since; // Get the bottom byte for the write-position
		file.seekSet(pos + wbot); // Navigate to the byte-cluster's position in the savefile
		file.write(b + wbot, since + 1); // Write the data-bytes into their corresponding savefile positions
		return 0; // Return a reset since-flag, since it has been 0 bytes since the last data-replacement
	}
	return since; // Return the same since-value
}

// Write a given series of bytes into a given point in the savefile,
// only committing actual writes when the data doesn't match, in order to reduce SD-card wear.
void writeData(
	unsigned long pos, // Note's bitwise position in the savefile (NOTE: (pos + amt) must be <= bytes in savefile!)
	byte amt, // Amount of data to read for comparison (NOTE: must be in the range of 1 to 32)
	byte b[], // Array of bytes to compare against the data, and write into the file where discrepancies are found
	byte onchan // Flag: only replace a given note if its channel matches the global CHAN
) {

	byte* buf = NULL; // Create a pointer to a dynamically-allocated buffer...
	buf = new byte[amt + 1]; // Allocate that buffer's memory based on the given size...
	memset(buf, 0, amt); // ...And clear it of any junk-data

	file.seekSet(pos); // Navigate to the data's position
	file.read(buf, amt); // Read the already-existing data from the savefile into the data-buffer

	byte since = 0; // Will count the number of bytes that have been checked since the last matching-byte
	for (byte i = 0; i < amt; i++) { // For every byte in the data...

		// If replace-on-chan-match is flagged, and this is a note's first byte, and the channels don't match...
		if (onchan && (!(i % 4)) && ((buf[i] & 15) != CHAN)) {
			since = wdSince(pos, since, i, b); // Write bytes to the file, based on iterator and since-bytes
			i += 3; // Increment the iterator to skip to the next note in the data
			continue; // Skip this iteration of the loop
		}

		if (buf[i] == b[i]) { // If the byte in the savefile is the same as the given byte...
			since = wdSince(pos, since, i, b); // Write bytes to the file, based on iterator and since-bytes
		} else { // Else, if the byte in the savefile doesn't match the given byte...
			since++; // Flag that it has been an additional byte since any matching bytes were found
		}

		if (since && (i == (amt - 1))) { // If this is the last buffer-byte, and it didn't match its corresponding savefile-byte...
			file.seekSet(pos + i); // Go to the savefile-byte's position
			file.write(b[i]); // Replace it with the buffer-byte
		}

	}

	file.sync(); // Sync the data immediately, to prevent any unusual behavior between multiple conflicting note-writes

	delete [] buf; // Free the buffer's dynamically-allocated memory
	buf = NULL; // Unset the buffer's pointer

}

// Put the current prefs-related global vars into a given buffer
void makePrefBuf(byte buf[]) {
	buf[0] = PAGE;
	buf[1] = OCTAVE;
	buf[2] = VELO;
	buf[3] = HUMANIZE;
	buf[4] = CHAN;
	buf[5] = QUANTIZE;
	buf[6] = DURATION;
	buf[7] = COPYPOS;
	buf[8] = COPYSEQ;
	buf[9] = SONG;
	buf[10] = CLOCKMASTER;
}

// Get the prefs-file's filename out of PROGMEM
void getPrefsFilename(char pn[]) {
	for (byte i = 0; i < 6; i++) { // For each letter in the filename...
		pn[i] = pgm_read_byte_near(PREFS_FILENAME + i); // Get that letter out of PROGMEM
	}
}

// Write the current relevant global vars into PRF.DAT
void writePrefs() {

	file.close(); // Temporarily close the current song-file

	byte buf[12]; // Make a buffer that will contain all the prefs
	makePrefBuf(buf); // Fill it with all the relevant pref values

	char pn[6]; // Will contain the prefs-file's filename
	getPrefsFilename(pn); // Get the prefs-file's filename out of PROGMEM

	file.open(pn, O_WRITE); // Open the prefs-file in write-mode
	file.seekSet(0); // Ensure we're on the first byte of the file
	file.write(buf, 11); // Write the pref vars into the file
	file.close(); // Close the prefs-file

	char name[8];
	getFilename(name, SONG); // Get the name of the current song-file

	file.open(name, O_RDWR); // Reopen the current song-file before exiting the function

}

// Load the contents of the preferences file ("PRF.DAT"), and put its contents into global variables
void loadPrefs() {

	byte buf[13]; // Make a buffer that will contain all the prefs

	char pn[6]; // Will contain the prefs-file's filename
	getPrefsFilename(pn); // Get the prefs-file's filename out of PROGMEM

	if (!sd.exists(pn)) { // If the prefs-file doesn't exist, create the file

		makePrefBuf(buf); // Fill the buffer with all the relevant pref values

		file.createContiguous(sd.vwd(), pn, 12); // Create a contiguous prefs-file
		file.close(); // Close the newly-created file

		file.open(pn, O_WRITE); // Create a prefs file and open it in write-mode
		file.write(buf, 11); // Write the pref vars into the file

	} else { // Else, if the prefs-file does exist...

		file.open(pn, O_READ); // Open the prefs-file in read-mode
		file.read(buf, 11); // Read all the prefs-bytes into the buffer

		// Assign the buffer's-worth of pref-bytes to their respective global-vars
		PAGE = buf[0];
		OCTAVE = buf[1];
		VELO = buf[2];
		HUMANIZE = buf[3];
		CHAN = buf[4];
		QUANTIZE = buf[5];
		DURATION = buf[6];
		COPYPOS = buf[7];
		COPYSEQ = buf[8];
		SONG = buf[9];
		CLOCKMASTER = buf[10];

	}

	file.close(); // Close the prefs file, regardless of whether it already exists or was just created

}

// Load a given song, or create its savefile if it doesn't exist
void loadSong(byte slot) {

	haltAllSustains(); // Clear all currently-sustained notes
	resetAllSeqs(); // Reset all seqs' internal activity variables of all kinds

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
	file.seekSet(1); // Go to the start of the SEQ-SIZE block
	file.read(STATS, 48);

	updateTickSize(); // Update the internal tick-size (in microseconds) to match the new BPM value

	SONG = slot; // Set the currently-active SONG-position to the given save-slot

	CUR16 = 127; // Set the global cue-position to arrive at 1 immediately
	ELAPSED = 0; // Reset the elapsed-time-since-last-16th-note counter

	displayLoadNumber(); // Display the number of the newly-loaded savefile

	TO_UPDATE = 255; // Flag entire GUI for an LED-update

}

