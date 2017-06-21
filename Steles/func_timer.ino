
// Update the internal timer system, and trigger various events
void updateTimer() {
	if (!CLOCKMASTER) { return; } // If not in CLOCKMASTER mode, exit the function
	unsigned long micr = micros(); // Get the current microsecond-timer value
	if (micr < ABSOLUTETIME) { // If the micros-value has wrapped around its finite counting-space to be less than the last absolute-time position...
		ELAPSEDTIME = (4294967295 - ABSOLUTETIME) + micr; // Put the wrapped-around microseconds into the elapsed-time value
	} else { // Else, if the micros-value is greater than or equal to last loop's absolute-time position...
		ELAPSEDTIME += micr - ABSOLUTETIME; // Add the difference between the current time and the previous time to the elapsed-time value
	}
	ABSOLUTETIME = micr; // Set the absolute-time to the current time-value
	if (ELAPSEDTIME < TICKSIZE) { return; } // If the next tick hasn't been reached, exit the function
	ELAPSEDTIME -= TICKSIZE; // Subtract the tick-delay length from the elapsed-time variable
	if (!PLAYING) { return; } // If the sequencer isn't in PLAYING mode, exit the function
	TICKCOUNT = (TICKCOUNT + 1) % 6; // Increase the value that tracks ticks, bounded to the size of a 16th-note
	Serial.write(248); // Send a MIDI CLOCK pulse
	if (TICKCOUNT) { return; } // If the current tick doesn't fall on a 16th-note, exit the function
	CUR16 = (CUR16 + 1) % 64; // Since we're sure we're on a new 16th-note, increase the current-16th-note variable
	if (!(CUR16 % 8)) { // If the global 16th-note is the first within a global cue-section...
		TO_UPDATE |= 1; // Flag the global-cue row of LEDs for an update
	}
	iterateAll(); // Iterate through a step of each active sequence
}
