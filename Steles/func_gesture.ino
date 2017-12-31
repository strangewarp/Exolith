
// Create a new entry in the gesture-tracking array
void newGestureEntry(byte row) {
	memmove(GESTURE + 1, GESTURE, 3); // Move all previous GESTURE values downward
	GESTURE[0] = row | B10110000; // Insert the new GESTURE entry, with a ~1-second expiration timer
}

// Check the gesture-tracking array for matches to any gesture-commands
void checkForGestures() {
	word all = 0; // Will get filled with a sequence of GESTURE-related bits for easy comparison
	for (byte i = 0; i < 4; i++) { // For each entry in the GESTURE-tracking array...
		if (!(GESTURE[i] & 128)) { return; } // If no GESTURE entry exists for this slot, exit the function
		all |= (GESTURE[i] & 7) << (i * 3); // Add the GESTURE-entry's button-location to the composite var
	}
	if (all == 2824) { // If this completed a LOAD gesture...
		LOADMODE ^= 1; // Toggle or untoggle LOAD-MODE override
		TO_UPDATE |= 253; // Flag LED-rows 0 and 2-7 for updating
	} else if (all == 101) { // Else, if this completed a GLOBAL PLAY/STOP gesture...
		toggleMidiClock(1); // Toggle the MIDI clock, with "1" for "the user did this, not a device"
	} else if (all == 325) { // Else, if this is a completed TOGGLE RECORD-MODE gesture...
		resetSeq(RECORDSEQ); // If the most-recently-touched seq is already playing, reset it to prepare for timing-wrapping
		SCATTER[RECORDSEQ] = 0; // Unset the most-recently-touched seq's SCATTER values before starting to record
		word seqsize = (STATS[RECORDSEQ] & B00111111) * 16; // Get sequence's size in 16th-notes
		POS[RECORDSEQ] = (seqsize > 255) ? CUR16 : (CUR16 % seqsize); // Wrap sequence around to global cue-point
		STATS[RECORDSEQ] |= 128; // Set the sequence to active
		RECORDMODE = 1; // Toggle RECORD-MODE
		TO_UPDATE |= 253; // Flag LED-rows 0 and 2-7 for updating
	} else { // Else, if no matching gesture was found...
		return; // Exit the function
	}
	// At this point in the function, a command was definitely found, so...
	memset(GESTURE, 0, 4); // Empty out the gesture-button memory
	RECORDNOTES = 0; // Disable note-recording, to avoid recording new notes automatically on a future toggle cycle
	ERASENOTES = 0; // Disable note-erasing, to avoid erasing new notes automatically on a future toggle cycle
}

// Update the tracking-info for all active gesture-keys
void updateGestureKeys() {
	while (GESTELAPSED >= GESTDECAY) { // While the gesture-decay timer has elapsed by a step or more...
		for (byte i = 0; i < 4; i++) { // For each active gesture-event...
			if (!(GESTURE[i] & 128)) { break; } // If the gesture-slot is empty, stop checking
			byte gest = (GESTURE[i] & 127) >> 3; // Get the gesture-entry's remaining time
			if (gest) { // If the gesture has time remaining...
				GESTURE[i] = (gest - 1) | (GESTURE[i] & B10000111); // Reduce its time by 1
			} else { // Else, if the gesture exists but has no time remaining...
				GESTURE[i] = 0; // Clear its now-outdated contents
			}
		}
		GESTELAPSED -= GESTDECAY; // Reduce the elapsed gesture-check time by a single unit of gesture-decay
	}
}
