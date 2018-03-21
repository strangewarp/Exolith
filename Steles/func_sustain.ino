

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

// If the bottommost SUSTAIN is filled, send its NOTE-OFF prematurely
// (Note: this function does not empty out the bottommost sustain's data;
// that should be done by other code after this function is called,
// depending on what is specifically being done.)
void clipBottomSustain() {
	if (SUST_COUNT < 8) { return; } // If the SUSTAIN buffer isn't full, exit the function
	byte obuf[4] = {SUST[21], SUST[22], 127}; // Make a premature NOTE-OFF for the oldest active sustain
	Serial.write(obuf, 3); // Send the premature NOTE-OFF immediately
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
			byte buf[4] = {SUST[n], SUST[n + 1], 127, 0}; // Create a buffer holding a correctly-formed NOTE-OFF
			Serial.write(buf, 3); // Send a NOTE-OFF for the sustain
			if (n < 21) { // If this isn't the bottom sustain-slot...
				memmove(SUST + n, SUST + n + 3, 24 - (n + 3)); // Move all lower sustains one slot upwards
			}
			SUST_COUNT--; // Reduce the sustain-count
			// n doesn't need to be increased here, since the next sustain occupies the same index now
			memset(SUST + (SUST_COUNT * 3), 0, 3); // Empty out the bottommost sustain's former position
			TO_UPDATE |= 2 * (!RECORDMODE); // If RECORD-MODE isn't active, flag the sustain-row for a GUI update
		}
	}
}

