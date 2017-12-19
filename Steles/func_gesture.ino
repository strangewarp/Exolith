
// Create a new entry in the gesture-tracking array
void newGestureEntry(byte row) {
	memmove(GESTURE + 1, GESTURE, 5); // Move all previous GESTURE values downward
	GESTURE[0] = row | B11010000; // Insert the new GESTURE entry, with a 2-second expiration timer
}

// Check the gesture-tracking array for matches to any gesture-commands
void checkForGestures() {

	// Track whether the current GESTURE-pattern matches a gesture-command
	// Format: 1 = LOAD; 2 = GLOBAL PLAY/STOP
	byte track = 3;

	for (byte i = 0; i < 6; i++) { // For every GESTURE-entry...
		if (!(GESTURE[i] & 128)) { return; } // If no GESTURE entry exists for this slot, exit the function
		// If the gesture doesn't follow the LOAD or GLOBAL PLAY/STOP pattern, empty that part of its tracking-flag
		track &= ((GESTURE[i] == i) << 1) | (GESTURE[i] == (5 - i));
		if (!track) { return; } // If the gesture-tracking flag is throwing a "no-gesture" result, exit the function
	}

	// At this point in the function, a command was definitely found, so empty out the gesture-button memory
	memset(GESTURE, 0, 6);

	if (track & 1) { // If this completed a LOAD gesture...
		LOADMODE = 1; // Toggle LOAD-MODE override
		RECORDNOTES = 0; // Disable note-recording, to avoid recording new notes automatically on a future toggle cycle
		ERASENOTES = 0; // Disable note-erasing, to avoid erasing new notes automatically on a future toggle cycle
		TO_UPDATE |= 253; // Flag LED-rows 0 and 2-7 for updating
	} else { // Else, if this completed a GLOBAL PLAY/STOP gesture...
		toggleMidiClock(1); // Toggle the MIDI clock, with "1" for "the user did this, not a device"
	}

}
