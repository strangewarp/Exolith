
// Update the internal timer system, and trigger various events
void updateTimer() {

	unsigned long micr = micros(); // Get the current microsecond-timer value

	unsigned long offset = 0; // Will hold the time-offset between the previous and current timer-check times
	if (micr < ABSOLUTETIME) { // If the micros-value has wrapped around its finite counting-space to be less than the last absolute-time position...
		offset = (4294967295 - ABSOLUTETIME) + micr; // Get an offset representing time since last check, wrapped around the unsigned long's limit
	} else { // Else, if the micros-value is greater than or equal to last loop's absolute-time position...
		offset = micr - ABSOLUTETIME; // Get an offset representing time since last check
	}
	KEYELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time-for-keypad-checks value
	ELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time value
	ABSOLUTETIME = micr; // Set the absolute-time to the current time-value

	if (KEYELAPSED >= SCANRATE) { // If the next keypad-check time has been reached...
		scanKeypad(); // Scan the keypad for changes in keystroke values
		KEYELAPSED = 0; // Reset the keypad-check timer
	}

	if (!CLOCKMASTER) { return; } // If not in CLOCKMASTER mode, exit the function

	if (ELAPSED < TICKSIZE) { return; } // If the next tick hasn't been reached, exit the function

	ELAPSED -= TICKSIZE; // Subtract the tick-delay length from the elapsed-time variable

	if (!PLAYING) { return; } // If the sequencer isn't in PLAYING mode, exit the function

	TICKCOUNT = (TICKCOUNT + 1) % 6; // Increase the value that tracks ticks, bounded to the size of a 16th-note
	Serial.write(248); // Send a MIDI CLOCK pulse
	if (TICKCOUNT) { return; } // If the current tick doesn't fall on a 16th-note, exit the function
	CUR16 = (CUR16 + 1) % 128; // Since we're sure we're on a new 16th-note, increase the current-16th-note variable
	if (!(CUR16 % 16)) { // If the global 16th-note is the first within a global cue-section...
		TO_UPDATE |= 2; // Flag the global-cue row of LEDs for an update
	}

	iterateAll(); // Iterate through a step of each active sequence

}
