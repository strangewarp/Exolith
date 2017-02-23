
// Load a given song, or create its folder and files if they don't exist
void loadSong(byte song) {

	// Display an "L" while loading data
	displayLetter("L");

	// Check whether song-slot's folder exists; if not, then create it



	// Get the names of this sequence's savefile and tempfile
	string name = string(SONG) + ".dat";
	string name2 = string(SONG) + ".tmp";

	// Open a data-file with the given song's number, and if it is newly-created, fill it with default data
	file.open(name, O_CREAT | O_APPEND | O_RDWR);
	if (file.size == 0) { // If the file is newly-created...
		// Fill it with dummy-bytes totalling:
		// (32 (exclude) + 16 (offset) + 32 (random) + 32 (scatter) + 16 (size) + 5 (active slice-seqs) + 1 (bpm))
		// + (3 (bytes) * 96 (ticks) * 4 (beats) * 16 (measures) * 32 (seqs))
		for (byte i = 0; i < 112; i++) {
			file.write(0); // Misc seq-parameter bytes
		}
		for (byte i = 0; i < 16; i++) {
			file.write(B01000100); // Seq-size bytes
		}
		file.write(B00000000); // Active slice-seq bytes
		file.write(B00000001);
		file.write(B00000010);
		file.write(B00000011);
		file.write(B00000100);
		file.write(120); // BPM byte
		for (long i = 0; i < 589824; i++) {
			file.write(0);
		}
	}
	file.close();

	// Recreate the song's corresponding tempfile as an empty file
	file.open(name2, O_CREAT | O_TRUNC);
	file.close();

	// Create a small buffer for data-transfer between the savefile and tempfile
	byte buf[97];

	// Get the seq-parameters from the file's header, and copy them into both the tempfile and program-vars



	// Copy the savefile's body-bytes into the tempfile
	for (long b = 0; b < 589824; b += 96) {
		file.open(name, O_CREAT | O_READ);
		file.seekSet(b + 134);
		file.read(buf, 96);
		file.close();
		file.open(name2, O_APPEND | O_WRITE);
		file.write(buf, 96);
		file.close();
	}

}

// Save the active song's tempfile contents into a given song-slot's perma-files
void saveSong(byte slot) {



}
