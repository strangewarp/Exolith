
// Parse a slice-keypress that was generated while in SLICE or OVERRVIEW mode
void parsePlayPress(byte col, byte row) {

	byte seq = col + (row * 4) + (PAGE * 24);
	byte nums = CTRL & B00001110;
	byte cue = CTRL & B00010000;
	byte off = CTRL & B00000001;

	if ((CTRL & B00100001) == B00100001) { // If this is a PAGE SEQS OFF command, which requires an additional seq-key strike to activate...
		byte pstart = 24 * PAGE; // Get the location of the first sequence on the active page
		if (cue | nums) { // If there is a CUE or any NUMBERS present...
			for (byte i = pstart; i < pstart + 24; i++) { // For each seq on the active page...
				SEQ_CMD[i] = (nums << 4) | 1; // Cue an OFF command for the global cue-point that corresponds to the given NUMBERS
			}
		} else { // Else, if neither a CUE or NUMBERS are present, then this is a pure ENTIRE-PAGE-OFF command, so...
			for (byte i = pstart; i < pstart + 24; i++) { // For each seq on the active page...
				resetSeq(i); // Reset the sequence immediately
			}
		}
	} else if (cue | off) { // Else, if either a CUE or OFF command is present...
		if (nums | cue) { // If there is a CUE or any NUMBERS...
			SEQ_CMD[seq] = (nums << 4) | ((off ^ 1) << 1) | off; // Cue an on-or-off command at the given global time
		} else { // Else, if there is neither CUE nor NUMBERS, that means this is an un-cued OFF command, so...
			resetSeq(seq); // Reset the sequence
		}
	} else { // Else, if neither a CUE nor OFF command is present...
		if (SEQ_CMD[seq] & B00000010) { // If the sequence already contains a cued ON value...
			SEQ_CMD[seq] = (SEQ_CMD[seq] & B11100011) | (nums << 1); // Add a slice-position to the cued ON command
		} else { // Else, if the sequence has no cued ON value, then this is a pure beatslice, so...
			word chunk = (SEQ_SIZE[seq] * 96) >> 3; // Get the number of ticks in one of the seq's slice-chunks
			SEQ_POS[seq] = 32768 | ( // Set the sequence to playing...
				(chunk * nums) + ( // With a base position equal to the slice-numbers being held...
					( // Added to an additional tick-offset inside of the slice itself, calculated as follows:
						(SEQ_POS[seq] & 32768) ? // Depending on whether the sequence is already playing...
						SEQ_POS[seq] : // Either take the sequence's current tick...
						CURTICK // Or take the global absolute tick...
					) % chunk // And get its remainder when divided by the seq's chunk-size
				)
			);
		}
	}

}

// Parse a note-keypress that was generated while in RECORDMODE mode
void parseRecPress(byte col, byte row) {

	// TODO update all of this... all of it



	byte slice = col + (4 * (LEFTCTRL & B00000001)); // Get the slice that corresponds to this column, plus 4 if RIGHT-SIDE is held

	if (BOTCTRL == B00000010) { // If the CHANNEL button is held...
		CHANNEL = max(0, min(15, CHANNEL + ((col + (col >> 1)) - 2))); // Shift the CHANNEL value by -2, -1, 1, or 2, depending on keystroke's column
	} else if (BOTCTRL == B00000100) { // If the OCTAVE button is held...
		OCTAVE = max(0, min(9, OCTAVE + ((col + (col >> 1)) - 2))); // Shift the OCTAVE value by -2, -1, 1, or 2, depending on keystroke's column
	} else if (BOTCTRL == B00001000) { // If the VELOCITY button is held...
		VELOCITY = max(1, min(127, VELOCITY + pow(((col + (col >> 1)) - 2), 4))); // Shift the VELOCITY value by -16, -1, 1, or 16, depending on keystroke's column
	} else if (BOTCTRL == B00010000) { // If the HUMANIZE button is held...
		HUMANIZE ^= 1 << slice; // Toggle this slice-button's HUMANIZE-bit
	} else if (BOTCTRL == B00000011) { // If the TIME-QUANTIZE chord is held...
		QUANTIZE ^= 1 << slice; // Toggle this slice-button's QUANTIZE-bit
	} else if (BOTCTRL == B00000101) { // If the SEQ-SIZE chord is held...
		resizeSeq(SLICE_SLOT[0]); // Resize the sequence in edit-position, using the given row modifier
	} else if (BOTCTRL == B00001001) { // If the NOTE-DURATION chord is held...
		DURATION ^= 1 << slice; // Toggle this slice-button's DURATION-bit
	} else if (BOTCTRL == B00010001) { // If the CLOCK MASTER/FOLLOW chord is held...
		if (CLOCKMASTER && PLAYING) { // If this was previously in MASTER mode, and currently PLAYING...
			toggleMidiClock(true); // Toggle the MIDI CLOCK off
		}
		CLOCKMASTER != CLOCKMASTER; // Only toggle the MIDI-CLOCK-MASTER boolean when chording its command with a regular key, to prevent chord collisions
		if (CLOCKMASTER && (!PLAYING)) { // If this is now in MASTER mode, and not PLAYING...
			toggleMidiClock(true); // Toggle the MIDI CLOCK on
		}
	} else if (BOTCTRL == B00000111) { // If the SAVE chord is held...
		saveData(slice + (row * 8)); // Save this song to a save-slot corresponding to the slice keypress
	} else if (BOTCTRL == B00011100) { // If the LOAD chord is held...
		loadData(slice + (row * 8)); // Load a song from the save-slot corresponding to the slice keypress
	} else { // Else, if the keystroke was within the note-keys...
		PITCHOFFSET = (LEFTCTRL & 3) * 12; // Shift pitch-offset by 1, 2, or 3 octaves, based on the currently-held octave-keys
		PITCHOFFSET += (LEFTCTRL & 4) ? 5 : 0; // Shift pitch-offset by interval 5, if pressed
		PITCHOFFSET += (LEFTCTRL & 8) ? 3 : 0; // Interval 3, if pressed
		PITCHOFFSET += (LEFTCTRL & 16) ? 2 : 0; // Interval 2, if pressed
		byte pitch = ((4 - row) * 4) + (col - 1); // Get a pitch-value corresponding to the keystroke location
		byte velo = // Get a velocity value...
			max(1, min(127, // Bounded between 1 and 127, to make a valid MIDI NOTE-ON velocity-byte...
				VELOCITY // Of the stored global VELOCITY-value for RECORD-mode notes...
				+ ( // Plus...
					random(0, HUMANIZE) // A random HUMANIZE value within the user-defined range...
					- byte(floor(HUMANIZE / 2)) // Minus half of the HUMANIZE range, to create an even negative-to-positive spread
				)
			));
		byte note[4] = { // Create a composite note-array
			(DURATION << 4) | CHANNEL, // Merge duration and channel values into a composite byte
			(OCTAVE * 12) + PITCHOFFSET + pitch, // Offset the pitch by various interval and octave modifiers
			velo // And then also velocity
		};
		if (RECORDNOTES) { // If RECORD-NOTES is active, and notes are being recorded...
			recordToTopSlice(note[0], note[1], note[2]); // Record the note into the sequence in edit-position
		}
		//playNote(DURATION, 3, note[0], note[1], note[2]); // Play the note, with the constructed pitch and velocity values
	}

}


// Interpret an incoming keystroke, using a given button's row and column
void assignKey(byte col, byte row) {

	if (col == 0) { // If the keystroke is in the leftmost column...

		CTRL |= 1 << row; // Add this button to control-tracking

		// Commands that apply to both modes:
		if (CTRL == B00111100) { // GLOBAL PLAY/STOP special command...
			toggleMidiClock(true); // Toggle the MIDI clock, with "true" for "the user did this, not a device"
		} else if (CTRL == B00111111) { // TOGGLE RECORD-MODE special command...
			RECORDMODE = !RECORDMODE; // Toggle/untoggle the mode's tracking-variable
			RECORDNOTES = 255; // Disable note-recording, to avoid recording new notes automatically on a future toggle cycle
			ERASENOTES = false; // Disable note-erasing, to avoid erasing new notes automatically on a future toggle cycle
			return; // No need to go through the rest of the function at this point
		}

		if (RECORDMODE) { // If RECORD-MODE is active...

			if (CTRL == B00100000) { // ERASE WHILE HELD special command...
				ERASENOTES = true; // As the button is held down, start erasing notes
			}

			TO_UPDATE = 255; // Slate the entire UI for updating

		} else { // Else, if PLAY MODE is active...

			if (CTRL == B00100010) { // PAGE A special command...
				PAGE = 0; // Toggle to page A
				TO_UPDATE |= 252; // Slate the UI for updating
			} else if (CTRL == B00100100) { // PAGE B special command...
				PAGE = 1; // Toggle to page B
				TO_UPDATE |= 252; // Slate the UI for updating
			} else if (CTRL == B00101000) { // PAGE C special command...
				PAGE = 2; // Toggle to page C
				TO_UPDATE |= 252; // Slate the UI for updating
			}

		}

	} else { // Else, if the keystroke is in any of the other columns...

		if (RECORDMODE) { // If RECORD-MODE is active...
			parseRecPress(col - 1, row); // Parse the RECORD-mode button-press
		} else { // Else, if PLAY MODE is active...
			parsePlayPress(col - 1, row); // Parse the PLAY-MODE button-press
		}

	}

}

// Interpret a key-release according to whatever command-mode is active
void unassignKey(byte col, byte row) {
	if (col == 0) { // If this up-keystroke was in the leftmost column...
		if (RECORDMODE) { // If RECORD MODE is active...
			if (CTRL == B00100000) { // If ERASE WHILE HELD was held...
				ERASENOTES = false; // Stop erasing notes
			}
			TO_UPDATE |= 255; // Toggle all GUI rows to update
		}
	}
	CTRL &= ~(1 << row); // Remove this button from control-tracking
}

// Parse any incoming keystrokes in the Keypad grid
void parseKeystrokes() {
	if (kpd.getKeys()) { // If any keys are pressed...
		for (byte i = 0; i < 10; i++) { // For every keypress slot...
			if (kpd.key[i].stateChanged) { // If the key's state has just changed...
				byte keynum = byte(kpd.key[i].kchar) - 48; // Convert the key's unique ASCII character into a number that reflects its position
				byte kcol = keynum % 5; // Get the key's column
				byte krow = byte((keynum - kcol) / 5); // Get the key's row
				if (kpd.key[i].kstate == PRESSED) { // If the button is pressed...
					assignKey(kcol, krow); // Interpret the keystroke
				} else if (kpd.key[i].kstate == RELEASED) { // Else, if the button is released...
					unassignKey(kcol, krow); // Interpret the key-release
				}
			}
		}
	}
}
