

// Create a new entry in the gesture-tracking array
void newGestureEntry(byte row) {
	memmove(GESTURE, GESTURE + 1, 3); // Move all previous GESTURE values downward
	GESTURE[3] = row | B10110000; // Insert the new GESTURE entry, with a ~1-second expiration timer
}

// Check the gesture-tracking array for matches to any gesture-commands
void checkForGestures() {
	word all = 0; // Will get filled with a sequence of GESTURE-related bits for easy comparison
	for (byte i = 3; i != 255; i--) { // For each entry in the GESTURE-tracking array...
		if (!(GESTURE[i] & 128)) { return; } // If no GESTURE entry exists for this slot, exit the function
		all |= (GESTURE[i] & 7) << (i * 3); // Add the GESTURE-entry's button-location to the composite var
	}
	if (all == 2824) { // If this completed a LOAD gesture...
		LOADMODE ^= 1; // Toggle or untoggle LOAD-MODE override
		TO_UPDATE |= 253; // Flag LED-rows 0 and 2-7 for updating
	} else if (all == 101) { // Else, if this completed a GLOBAL PLAY/STOP gesture...
		toggleMidiClock(1); // Toggle the MIDI clock, with "1" for "the user did this, not a device"
	} else if (all == 325) { // Else, if this is a completed TOGGLE RECORD-MODE gesture...
		if (RECORDMODE) { // If RECORDMODE is about to be untoggled...
			writePrefs(); // Write the current relevant global vars into PRF.DAT
		} else { // Else, if RECORD-MODE is about to be toggled...
			primeRecSeq(); // Prime the newly-entered RECORD-MODE sequence for editing
		}
		RECORDMODE ^= 1; // Toggle or untoggle RECORD-MODE
		TO_UPDATE |= 255; // Flag all LED-rows for updating
	} else { // Else, if no matching gesture was found...
		return; // Exit the function
	}
	// At this point in the function, a command was definitely found, so...
	memset(GESTURE, byte(0), 4); // Empty out the gesture-button memory
	RECORDNOTES = 0; // Clear the ARMED RECORDING flag
}

// Update the tracking-info for all active gesture-keys
void updateGestureKeys() {
	if (GESTELAPSED < GESTDECAY) { return; } // If gesture-decay timer hasn't elapsed by a step or more, exit function
	for (byte i = 0; i < 4; i++) { // For each active gesture-event...
		if (!(GESTURE[i] & 128)) { break; } // If the gesture-slot is empty, stop checking
		byte gest = (GESTURE[i] & 127) >> 3; // Get the gesture-entry's remaining time
		if (gest) { // If the gesture has time remaining...
			GESTURE[i] = (gest - 1) | (GESTURE[i] & B10000111); // Reduce its time by 1
		} else { // Else, if the gesture exists but has no time remaining...
			GESTURE[i] = 0; // Clear its now-outdated contents
		}
	}
	GESTELAPSED %= GESTDECAY; // Reduce the elapsed gesture-check time by n units of gesture-decay
}

