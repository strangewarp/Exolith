

// Toggle whether the MIDI CLOCK is playing, provided that this unit is in CLOCK MASTER mode
void toggleMidiClock(byte usercmd) {

	PLAYING = !PLAYING; // Toggle/untoggle the var that tracks whether the MIDI CLOCK is playing

	if (CLOCKMASTER) { // If in CLOCK MASTER mode...
		if (PLAYING) { // If playing has just been enabled...
			ELAPSED = TICKSZ2; // Cue the next tick-update to occur on the next timer-check
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

// Update the internal tick-sizes (in microseconds) to match a new BPM and/or SWING AMOUNT value
void updateTickSize() {

	float sw; // Will hold a multiplier that corresponds to the current SWING AMOUNT
	if (SAMOUNT <= 64) { // If this is a left-swing or a no-swing...
		sw = 1.0 - pgm_read_float_near(SWING_TABLE + (64 - SAMOUNT)); // Flip the SAMOUNT index, and subtract its SWING value from 1
	} else { // Else, if this is a right-swing...
		sw = 1.0 + pgm_read_float_near(SWING_TABLE + (SAMOUNT - 64)); // Reduce SAMOUNT by 64, and add its SWING value to 1
	}

	// Get a micros-per-tick value that corresponds to the current BPM (with each "beat" being a quarter-note long)
	float mcpb = pgm_read_float_near(MCS_TABLE + (BPM - BPM_LIMIT_LOW));

	// Modify the micros-per-tick-value by the SWING-multiplier, to get the left-tick's new absolute size
	float mcmod = mcpb * sw;

	TICKSZ1 = word(round(mcmod)); // Set the leftmost SWING-tick size
	TICKSZ2 = word(round((mcpb * 2) - mcmod)); // Get and set the rightmost SWING-tick size

}

// Engage all timed elements of a 16th-note-sized granularity
void activateStep() {

	updateSwingPart(); // Update the SWING-PART var based on the current SWING GRANULARITY and CUR16 tick

	if (!(CUR16 % 16)) { // It we're on the first tick within a global cue...
		TO_UPDATE |= 1; // Flag the top LED-row for updating
	}

	if (RECORDMODE) { // If we're in RECORD-MODE...
		if (!(POS[RECORDSEQ] % 16)) { // If at the start of a new beat of RECORDSEQ...
			TO_UPDATE |= 2; // Flag the second LED-row for updating
		}
		if (RECORDNOTES && (!distFromQuantize())) { // If we are recording notes, and on a QUANTIZE or QRESET tick...
			BLINKR = 63 | (192 * TRACK); // Cue either a short or long RIGHT-BLINK, depending on which TRACK is active
			BLINKL = 63 | (192 * (!TRACK)); // ^ Same, but for LEFT-BLINK
			TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
		}
	}

	iterateAll(); // Iterate through a step of each active sequence

}

// Advance the concrete tick, and all its associated sequencing mechanisms
void advanceTick() {

	Serial.write(248); // Send a MIDI CLOCK pulse

	TICKCOUNT = (TICKCOUNT + 1) % 6; // Increase the value that tracks ticks, bounded to the size of a 16th-note

	if (TICKCOUNT) { return; } // If the current tick doesn't fall on a 16th-note, exit the function

	// Since we're sure we're on a new 16th-note, advance the global current-16th-note variable
	CUR16 = (CUR16 + 1) % 128;

	if (KEYFLAG) { // If a note is currently being recorded in manual-duration-mode...
		KEYCOUNT++; // Increase the note's tick-counter by 1
		if (KEYCOUNT == 128) { // If the note's tick-counter has reached a length of 2 beats...
			recordHeldNote(); // Record the note, since it's reached the maximum reasonable size
		}
	}

	activateStep(); // Engage all timed elements of a 16th-note-sized granularity

}

// Check whether timing should currently be advanced, and if so, start applying internal timing changes
void advanceOwnTick() {

	word tlen = SPART ? TICKSZ2 : TICKSZ1; // Get the length of the current SWING-section's ticks

	// If the next tick hasn't been reached within the current SWING-section's tick-length, exit the function
	if (ELAPSED < tlen) { return; }

	ELAPSED -= tlen; // Subtract the current tick-length from the elapsed-time variable

	if (!PLAYING) { return; } // If the sequencer isn't in PLAYING mode, exit the function

	advanceTick(); // Advance the concrete tick, and all its associated sequencing mechanisms

}

// Decay any currrently-active BLINKL/BLINKR counters
void blinkDecay(word omod) {

	if (BLINKL || BLINKR) { // If an LED-BLINK is active...

		omod |= !omod; // If the bit-shifted offset value is empty, set it to 1

		if (BLINKL) { // If BLINK-LEFT is active...
			if (BLINKL > omod) { // If the BLINKL-counter is greater than the bit-shifted offset value...
				BLINKL -= omod; // Subtract the offset from the BLINKL-counter
			} else { // Else, if the BLINKL-counter is lower than the offset...
				BLINKL = 0; // Clear the BLINKL-counter
				TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
			}
		}

		if (BLINKR) { // If BLINK-RIGHT is active...
			if (BLINKR > omod) { // If the BLINKR-counter is greater than the bit-shifted offset value...
				BLINKR -= omod; // Subtract the offset from the BLINKR-counter
			} else { // Else, if the BLINKR-counter is lower than the offset...
				BLINKR = 0; // Clear the BLINKR-counter
				TO_UPDATE |= 252; // Flag the bottom 6 rows for LED updates
			}
		}

	}

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
	GESTELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time-for-gesture-decay value
	KEYELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time-for-keypad-checks value
	ELAPSED += offset; // Add the difference between the current time and the previous time to the elapsed-time value

	ABSOLUTETIME = micr; // Set the absolute-time to the current time-value

	if (LOADHOLD) { // If a just-loaded file's file-number is currently being held onscreen...
		LOADHOLD -= max(1, (offset >> 8)); // Reduce LOADHOLD by an offset-based value
		if (LOADHOLD <= 0) { // If LOADHOLD has just this moment finished reducing all the way to 0...
			LOADHOLD = 0; // Set LOADHOLD to 0, so boolean checks will parse it as "false"
			TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
		}
	}

	// Send a bit-shifted version of the offset value to blinkDecay(), to decrement the BLINK counters correctly
	blinkDecay(word(offset >> 6)); // Decay any currrently-active BLINKL/BLINKR counters

	updateGestureKeys(); // Update the tracking-info for all active gesture-keys

	scanKeypad(); // Scan the keypad for changes in keystroke values

	// If CLOCK-FOLLOW mode is active, then exit the function without updating the sequencing mechanisms
	if (!CLOCKMASTER) { return; }

	advanceOwnTick(); // Check all timing elements of a tick-sized granularity (1/6 of a 16th note), and advance the tick-counter

}
