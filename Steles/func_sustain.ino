

// Send MIDI-OFF commands for all currently-sustained notes
void haltAllSustains() {
	if (!SUST_COUNT) { return; } // If there are no active sustains, exit the function
	byte scb = SUST_COUNT * 3; // Get the number of filled sustain-bytes
	for (byte i = 2; i < scb; i += 3) { // For every duration-byte in the sustain-data...
		SUST[i] = 127; // Replace the duration with a velocity-value (this shouldn't be necessary, but some synths act weird without it)
	}
	Serial.write(SUST, scb); // Send the now-fully-formed chain of NOTE-OFF commands
	memset(SUST, 0, scb); // Clear the SUST array's now-obsolete data
	SUST_COUNT = 0; // Set the sustain-count to 0
}

// Send NOTE-OFFs for any duplicate SUSTAINs and MOUTs
void removeDuplicates(byte cmd, byte pitch) {
	byte i = 0;
	while (i < (SUST_COUNT * 3)) { // For every SUSTAIN-slot...
		if ((SUST[i + 1] == pitch) && ((SUST[i] + 16) == cmd)) { // If the pitch and channel match...
			endSustain(i); // End the sustain in a given SUST-slot
		} else { // Else, if this wasn't a match...
			i += 3; // Set "i" forward, since no SUSTAIN-entries were removed
		}
	}
	i = 0; // Reset the iteration var
	while (i < (MOUT_COUNT * 3)) { // For every MOUT-slot...
		if ((MOUT[i + 1] == pitch) && (MOUT[i] == cmd)) { // If the MOUT pitch and channel match...
			arrayRemoveEntry(MOUT, i, MOUT_COUNT); // Remove the duplicate MOUT-entry
		} else { // Else, if this wasn't a match...
			i += 3; // Set "i" forward, since no MOUT-entries were removed
		}
	}
}

// Remove an entry from the SUST or MOUT array, and then decrement its counter
void arrayRemoveEntry(byte a[], byte pos, byte &count) {
	if (pos < 21) { // If this isn't the array's bottom note-slot...
		memmove(a + pos, a + pos + 3, 24 - (pos + 3)); // Move all lower notes one slot upwards
	}
	count--; // Reduce the array's note-counter
	memset(a + (count * 3), 0, 3); // Empty out the bottommost note-slot's former position
}

// Create and send a NOTE-OFF for an entry within the given SUST/MOUT array
void arrayNoteOff(byte a[], byte pos) {
	byte buf[4] = {a[pos], a[pos + 1], 127, 0}; // Create a buffer holding a correctly-formed NOTE-OFF
	Serial.write(buf, 3); // Send a NOTE-OFF for the sustain
}

// End the sustain in a given SUST-slot
void endSustain(byte n) {
	arrayNoteOff(SUST, n); // Send a NOTE-OFF for the given SUST entry 
	arrayRemoveEntry(SUST, n, SUST_COUNT); // Remove the duplicate SUST-entry
	TO_UPDATE |= 2 * (!RECORDMODE); // If RECORD-MODE isn't active, flag the sustain-row for a GUI update
}

// If the bottommost SUSTAIN is filled, send its NOTE-OFF prematurely
// (Note: this function does not empty out the bottommost sustain's data;
// that should be done by other code after this function is called,
// depending on what is specifically being done.)
void clipBottomSustain() {
	if (SUST_COUNT < 8) { return; } // If the SUSTAIN buffer isn't full, exit the function
	arrayNoteOff(SUST, 21); // Send a premature NOTE-OFF for the oldest active sustain
	SUST_COUNT--; // Reduce the number of active sustains by 1
}

// Process one 16th-note worth of duration for all notes in the SUSTAIN system
void processSustains() {
	byte n = 0; // Value to iterate through each sustain-position
	while (n < (SUST_COUNT * 3)) { // For every sustain-entry in the SUST array (if there are any)...
		byte n2 = n + 2; // Get the key of the sustain's DURATION byte
		if (SUST[n2]) { // If any duration remains...
			SUST[n2]--; // Reduce the duration by 1
			n += 3; // Move on to the next sustain-position normally
		} else { // Else, if the remaining duration is 0...
			endSustain(n);
			// n doesn't need to be increased here, since the next sustain occupies the same index now
		}
	}
}

