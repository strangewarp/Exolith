
// Update the internal timer system, and if Exolith is in MIDI MASTER mode, trigger various events
void updateTimer() {
	unsigned long micr = micros(); // Get the current microsecond-timer value
	if (micr < ABSOLUTETIME) { // If the micros-value has wrapped around its finite counting-space to be less than the last absolute-time position...
		ELAPSEDTIME = (4294967295 - ABSOLUTETIME) + micr; // Put the wrapped-around microseconds into the elapsed-time value
	} else { // Else, if the micros-value is greater than or equal to last loop's absolute-time position...
		ELAPSEDTIME += micr - ABSOLUTETIME; // Add the difference between the current time and the previous time to the elapsed-time value
	}
	ABSOLUTETIME = micr; // Set the absolute-time to the current time-value
	if (ELAPSEDTIME >= TICKDELAY) { // If the elapsed-time has reached the tick-delay length...
		ELAPSEDTIME -= TICKDELAY; // Subtract the tick-delay length from the elapsed-time variable
		iterateAll(); // Iterate through one step of all sequencing processes
	}
}
