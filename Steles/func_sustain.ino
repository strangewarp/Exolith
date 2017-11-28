
// Send MIDI-OFF commands for all currently-sustained notes
void haltAllSustains() {
	if (!SUST_COUNT) { return; } // If there are no active sustains, exit the function
	byte scb = SUST_COUNT * 3; // Get the number of filled sustain-bytes
	memmove(SUST, SUST + 1, 23); // Move the sustain-bytes upward by 1, to start converting into a chain of MIDI commands
	for (byte i = 2; i < scb; i += 3) { // For every new velocity-byte in the transitional sustain-data...
		SUST[i] = 127; // Set the OFF-velocity to maximum (this shouldn't be necessary, but some synths act weird without it)
	}
	Serial.write(SUST, scb); // Send the now-fully-formed chain of NOTE-OFF commands
	memset(SUST, 0, scb); // Clear the SUST array's now-obsolete data
	SUST_COUNT = 0; // Set the sustain-count to 0
}

// Process one 16th-note worth of duration for all notes in the SUSTAIN system
void processSustains() {
	byte n = 0; // Value to iterate through each sustain-position
	while (n < (SUST_COUNT * 3)) { // For every sustain-entry in the SUST array (if there are any)...
		if (SUST[n]) { // If any duration remains...
			SUST[n]--; // Reduce the duration by 1
			n += 3; // Move on to the next sustain-position normally
		} else { // Else, if the remaining duration is 0...
			byte buf[4] = {SUST[n + 1], SUST[n + 2], 127}; // Create a buffer holding a correctly-formed NOTE-OFF
			Serial.write(buf, 3); // Send a NOTE-OFF for the sustain
			if (n < 21) { // If this isn't the bottom sustain-slot...
				memmove(SUST + n, SUST + n + 3, 24 - (n + 3)); // Move all lower sustains one slot upwards
			}
			SUST_COUNT--; // Reduce the sustain-count
			// n doesn't need to be increased here, since the next sustain occupies the same index now
			memset(SUST + (SUST_COUNT * 3), 0, 3); // Empty out the bottommost sustain's former position
			TO_UPDATE |= 2; // Flag the second LED-row for a GUI update
		}
	}
}
