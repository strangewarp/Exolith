

// Update the internal tick-size (in microseconds) to match a new BPM value
void updateTickSize() {

	// Get a micros-per-tick value that corresponds to the current BPM (with each "beat" being a quarter-note long)
	TICKSIZE = pgm_read_float_near(MCS_TABLE + (BPM - BPM_LIMIT_LOW));

}

// Reduce the given BLINK-counter (left or right), and if it has reached 0, flag an LED-update fro its associated rows
void blinkReduce(byte &b, word omod) {
	if (b) { // If the given BLINK var is active...
		if (b > omod) { // If the BLINK-counter is greater than the bit-shifted offset value...
			b -= omod; // Subtract the offset from the BLINK-var
		} else { // Else, if the BLINK-counter is lower than the offset...
			b = 0; // Clear the BLINK-counter
			TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
		}
	}
}

// Decay any currrently-active BLINKL/BLINKR counters
void blinkDecay(word omod) {
	if (BLINKL || BLINKR) { // If an LED-BLINK is active...
		omod |= !omod; // If the bit-shifted offset value is empty, set it to 1
		blinkReduce(BLINKL, omod); // Update the BLINKL counter
		blinkReduce(BLINKR, omod); // Update the BLINKR counter
	}
}

// Engage all timed elements of a 32nd-note-sized granularity
void activateStep() {

	if (!(CUR32 % 32)) { // It we're on the first tick within a global cue-point...
		TO_UPDATE |= 1; // Flag the top LED-row for updating
	}

	if (RECORDMODE) { // If we're in RECORD-MODE...

		if (!(POS[RECORDSEQ] % 8)) { // If this is the start of a new quarter-note...
			TO_UPDATE |= 2; // Flag the second LED-row for updating
		}

		if (RECORDNOTES) { // If RECORD-NOTES is currently armed...
			if (!applyOffset(distFromQuantize())) { // If we are on an OFFSET-modified insertion-point tick...
				BLINKR = 255 * TRACK; // Cue a long RIGHT-BLINK, if TRACK 2 is active
				BLINKL = 255 * (!TRACK); // ^ Same, but for LEFT-BLINK and TRACK 1
				TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
			}
		}

	}

	iterateAll(); // Iterate through a step of each active sequence

}

// Advance the concrete tick, and all its associated sequencing mechanisms
void advanceTick() {

	// If the next tick hasn't been reached within the current tick-length, exit the function
	if (ELAPSED < TICKSIZE) { return; }

	ELAPSED -= TICKSIZE; // Subtract the current tick-length from the elapsed-time variable

	Serial.write(248); // Send a MIDI CLOCK pulse

	TICKCOUNT = (TICKCOUNT + 1) % 3; // Increase the value that tracks ticks, bounded to the size of a 32nd-note

	if (TICKCOUNT) { return; } // If the current tick doesn't fall on a 32nd-note, exit the function

	// Since we're sure we're on a new 32nd-note, advance the global current-32nd-note variable;
	// it will wrap around after reaching 255, limiting it to 256 32nd-notes, or 8 whole-notes
	CUR32++;

	if (KEYFLAG) { // If a note is currently being recorded in manual-duration-mode...
		KEYCOUNT++; // Increase the note's tick-counter by 1
		if (KEYCOUNT == 128) { // If the note's tick-counter has reached a length of 4 whole-notes...
			recordHeldNote(); // Record the note, since it's reached the maximum reasonable size
		}
	}

	activateStep(); // Engage all timed elements of a 32nd-note-sized granularity

}

// Update the internal timer system, and trigger various events
void updateTimer() {

	unsigned long micr = micros(); // Get the current microsecond-timer value
	unsigned long offset; // Will hold the time-offset since the last ABSOLUTETIME point

	if (micr < ABSOLUTETIME) { // If the micros-value has wrapped around its finite counting-space to be less than the last absolute-time position...
		offset = (4294967295UL - ABSOLUTETIME) + micr; // Get an offset representing time since last check, wrapped around the unsigned long's limit
	} else { // Else, if the micros-value is greater than or equal to last loop's absolute-time position...
		offset = micr - ABSOLUTETIME; // Get an offset representing time since last check
	}
	KEYELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time-for-keypad-checks value
	ELAPSED += float(offset); // Add the difference between the current time and the previous time to the elapsed-time value

	ABSOLUTETIME = micr; // Set the absolute-time to the current time-value

	if (LOADHOLD) { // If a just-loaded file's file-number is currently being held onscreen...
		LOADHOLD -= max(1, (offset >> 8)); // Reduce LOADHOLD by an offset-based value
		if (LOADHOLD <= 0) { // If LOADHOLD has just this moment finished reducing all the way to 0...
			LOADHOLD = 0; // Set LOADHOLD to 0, so boolean checks will parse it as "false"
			TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
		}
	}

	// Send a bit-shifted version of the offset value to blinkDecay(), to decrement the BLINK counters correctly
	blinkDecay(max(1, offset >> 6)); // Decay any currrently-active BLINKL/BLINKR counters

	scanKeypad(); // Scan the keypad for changes in keystroke values

	advanceTick(); // Check all timing elements of a tick-sized granularity (1/3 of a 32nd note), and advance the tick-counter

}
