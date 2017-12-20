
// Toggle whether the MIDI CLOCK is playing, provided that this unit is in CLOCK MASTER mode
void toggleMidiClock(byte usercmd) {

	PLAYING = !PLAYING; // Toggle/untoggle the var that tracks whether the MIDI CLOCK is playing

	clearRecentNotes(); // Reset the "most recent note by channel" array

	if (CLOCKMASTER) { // If in CLOCK MASTER mode...
		if (PLAYING) { // If playing has just been enabled...
			ELAPSED = TICKSIZE; // Cue the next tick-update to occur on the next timer-check
			TICKCOUNT = 5; // Set the next 16th-note to be cued on the next timer-check
			CUR16 = 127; // Ensure that the global 16th-note position will reach 0 on the next timer-check
			ABSOLUTETIME = micros(); // Set the ABSOLUTETIME to the current time, to prevent changes to ELAPSED in the next timer-check
			Serial.write(250); // Send a MIDI CLOCK START command
			Serial.write(248); // Send a MIDI CLOCK TICK (dummy-tick immediately following START command, as per MIDI spec)
		} else { // Else, if playing has just been disabled...
			haltAllSustains(); // Halt all currently-broadcasting MIDI sustains
			Serial.write(252); // Send a MIDI CLOCK STOP command
			resetAllSeqs(); // Reset the internal pointers of all sequences
		}
	} else { // If in CLOCK FOLLOW mode...
		if (PLAYING) { // If PLAYING has just been enabled...
			TICKCOUNT = 5; // Set the next 16th-note to be cued on the next clock-tick
			CUR16 = 127; // Set the global 16th-note to a position where it will wrap around to 0
			if (!usercmd) { // If this toggle was sent by an external device...
				DUMMYTICK = true; // Flag the sequencer to wait for an incoming dummy-tick, as per MIDI spec
			}
		} else { // Else, if playing has just been disabled...
			haltAllSustains(); // Halt all currently-broadcasting MIDI sustains
			if (!usercmd) {
				resetAllSeqs(); // Reset the internal pointers of all sequences
			}
		}
	}

}

// Update the internal timer system, and trigger various events
void updateTimer() {

	unsigned long micr = micros(); // Get the current microsecond-timer value

	if (micr < ABSOLUTETIME) { // If the micros-value has wrapped around its finite counting-space to be less than the last absolute-time position...
		unsigned long offset = (4294967295UL - ABSOLUTETIME) + micr; // Get an offset representing time since last check, wrapped around the unsigned long's limit
		GESTELAPSED += offset; // Put the wrapped-around microseconds into the elapsed-time-for-gesture-decay value
		KEYELAPSED += offset; // Put the wrapped-around microseconds into the elapsed-time-for-keypad-checks value
		ELAPSED += offset; // Put the wrapped-around microseconds into the elapsed-time value
	} else { // Else, if the micros-value is greater than or equal to last loop's absolute-time position...
		unsigned long offset = micr - ABSOLUTETIME; // Get an offset representing time since last check
		GESTELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time-for-gesture-decay value
		KEYELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time-for-keypad-checks value
		ELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time value
	}
	ABSOLUTETIME = micr; // Set the absolute-time to the current time-value

	updateGestureKeys(); // Update the tracking-info for all active gesture-keys

	if (KEYELAPSED >= SCANRATE) { // If the next keypad-check time has been reached...
		scanKeypad(); // Scan the keypad for changes in keystroke values
		KEYELAPSED = 0; // Reset the keypad-check timer
	}

	// If not in CLOCKMASTER mode, or if the next tick hasn't been reached, exit the function
	if ((!CLOCKMASTER) || (ELAPSED < TICKSIZE)) { return; }

	ELAPSED -= TICKSIZE; // Subtract the tick-delay length from the elapsed-time variable

	if (!PLAYING) { return; } // If the sequencer isn't in PLAYING mode, exit the function

	TICKCOUNT = (TICKCOUNT + 1) % 6; // Increase the value that tracks ticks, bounded to the size of a 16th-note
	Serial.write(248); // Send a MIDI CLOCK pulse

	if (TICKCOUNT) { return; } // If the current tick doesn't fall on a 16th-note, exit the function

	// Since we're sure we're on a new 16th-note, increase the global current-16th-note variable
	// Also note: since CUR16 is a byte, it wraps around to 0 automatically after reaching 255
	CUR16++;

	if (!(CUR16 % 16)) { // If the global 16th-note is the first within a global cue-section...
		TO_UPDATE |= 1; // Flag the global-cue row of LEDs for an update
	}

	iterateAll(); // Iterate through a step of each active sequence

}
