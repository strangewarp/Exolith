
// Parse a slice-keypress that was generated while in SLICE or OVERRVIEW mode
void parsePlayPress(byte col, byte row) {

	uint8_t seq = col + (row * 4) + (PAGE * 24); // Get the sequence that corresponds to the given column and row
	uint8_t pg = CTRL & B00100000; // Get the PAGE button's status
	uint8_t cue = CTRL & B00010000; // Get the CUE command's status
	uint8_t nums = CTRL & B00001110; // Get the NUMBER commands' status
	uint8_t off = CTRL & B00000001; // Get the OFF command's status

	if (CTRL == B00100001) { // If PAGE-OFF is held, and a regular button-press was made to signal intent...
		uint8_t ptop = PAGE * 24; // Get the position of the first seq on the current page
		for (uint8_t i = ptop; i < (ptop + 24); i++) { // For every seq on this page...
			resetSeq(i); // Reset the seq's contents
		}
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
	} else if (CTRL == B00110000) { // If SCATTER UNSET is held...
		SCATTER[seq] = 0; // Unset the seq's SCATTER flags
		TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
	} else if (CTRL == B00110001) { // If PAGE-SCATTER-UNSET is held, and a regular button-press was made to signal intent...
		uint8_t ptop = PAGE * 24; // Get the position of the first seq on the current page
		for (uint8_t i = ptop; i < (ptop + 24); i++) { // For every seq on this page...
			SCATTER[i] = 0; // Unset the seq's SCATTER flags
		}
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
	} else if (pg && cue && nums && (!off)) { // If this is a SCATTER command of any kind...
		SCATTER[seq] = nums >> 1; // Turn this command's numeric value into the seq's SCATTER-chance flags
		TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
	} else if (cue | off) { // If either a CUE or OFF command is present...
		if (cue | nums) { // If there is a CUE or any NUMBERS...
			CMD[seq] = (nums << 4) | ((off ^ 1) << 1) | off; // Cue an on-or-off command at the given global time
		} else { // Else, if there is neither CUE nor NUMBERS, that means this is an un-cued OFF command, so...
			if (STATS[seq] & 128) { // If the sequence is playing...
				TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
			}
			resetSeq(seq); // Reset the sequence
		}
	} else { // Else, if no special command is present...
		if (CMD[seq] & B00000010) { // If the sequence already contains a cued ON value...
			CMD[seq] = (CMD[seq] & B11100011) | (nums << 1); // Add a slice-position to the cued ON command
		} else { // Else, if the sequence has no cued ON value, then this is a pure beatslice, so...

			// Get the number of 16th-notes in one of the seq's slice-chunks
			uint8_t csize = ((STATS[seq] & 127) * 16) >> 3;

			// Set the sequence's position to:
			//   A base-position equal to the slice-numbers being held,
			//   with an additional tick-offset inside the slice itself,
			//   (which, depending on whether the sequence is playing,
			//   either takes the sequence's current tick, or the global absolute tick,
			//   and wraps said tick to the size of a single chunk)
			POS[seq] = (csize * (nums >> 1)) + (((STATS[seq] & 128) ? POS[seq] : CUR16) % csize);

			if (!(STATS[seq] & 128)) { // If the seq was previously turned off...
				TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
			}

			STATS[seq] |= 128; // Set the sequence to playing

		}
	}

}

// Parse a note-keypress that was generated while in RECORDMODE mode
void parseRecPress(byte col, byte row) {

	uint8_t key = col + (row * 4); // Get the button-key that corresponds to the given column and row

	if (!CTRL) { // If no CTRL buttons are held...

		// Get a note that corresponds to the key, organized from bottom-left to top-right, with all modifiers applied;
		// And also get the note's velocity with a random humanize-offset
		uint8_t pitch = min(127, (OCTAVE * 12) + BASENOTE + ((23 - key) ^ 3));
		uint8_t velo = min(127, max(1, VELO + ((HUMANIZE >> 1) - random(HUMANIZE + 1))));

		uint8_t play = 1; // Track whether a note should be played in response to this keystroke

		if (RECORDNOTES) { // If notes are being recorded...

			// Make a time-offset for the RECORDSEQ's current 16th-note, based on the QUANTIZE value;
			// this will be fed to the recordToSeq command to achieve the correct insert-position
			uint8_t down = POS[RECORDSEQ] % QUANTIZE;
			uint8_t up = QUANTIZE - down;
			int8_t offset = (down <= up) ? (-down) : up;

			recordToSeq(offset, DURATION, 144 + CHAN, pitch, velo); // Record the note into the current RECORDSEQ slot

			// If the tick was inserted at an offset after the current position,
			// exit the function without playing the note, because it will be played momentarily
			if (offset >= 1) { return; }

		}

		// Update SUSTAIN buffer in preparation for playing the user-pressed note
		if (SUST_COUNT == 8) { // If the SUSTAIN buffer is already full...
			Serial.write(SUST[23]); // Send a premature NOTE-OFF for the oldest active sustain
			Serial.write(SUST[24]); // ^
			Serial.write(127); // ^
			SUST_COUNT--; // Reduce the number of active sustains by 1
		}
		memmove(SUST, SUST + 3, (SUST_COUNT * 3)); // Move all sustains one space downward
		SUST[0] = DURATION; // Fill the topmost sustain-slot with the user-pressed note
		SUST[1] = 128 + CHAN; // ^
		SUST[2] = pitch; // ^
		Serial.write(144 + CHAN); // Send the user-pressed note to MIDI-OUT immediately, without buffering
		Serial.write(pitch); // ^
		Serial.write(velo); // ^

	} else if (CTRL == B00000010) { // If the BASENOTE button is held...
		BASENOTE = min(12, BASENOTE ^ (8 >> col)); // Modify the BASENOTE value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00000100) { // If the OCTAVE button is held...
		OCTAVE = min(10, OCTAVE ^ (8 >> col)); // Modify the OCTAVE value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00001000) { // If the DURATION button is held...
		DURATION = min(64, DURATION ^ (128 >> (key % 8))); // Modify the DURATION value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00010000) { // If the SWITCH RECORDING SEQUENCE button is held...
		TO_UPDATE |= 4 >> (RECORDSEQ >> 2); // Flag the previous seq's corresponding LED-row for updating
		RECORDSEQ = (PAGE * 24) + key; // Switch to the seq that corresponds to the key-position on the active page
		TO_UPDATE |= 4 >> row; // Flag the new seq's corresponding LED-row for updating
	} else if (CTRL == B00000110) { // If the VELO button is held...
		VELO = min(127, max(1, VELO ^ (128 >> (key % 8)))); // Modify the VELO value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00001010) { // If the HUMANIZE button is held...
		HUMANIZE = min(127, HUMANIZE ^ (128 >> (key % 8))); // Modify the HUMANIZE value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00001100) { // If the TIME-QUANTIZE button is held...
		QUANTIZE = 8 >> col; // Modify the TIME-QUANTIZE value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00010010) { // If the CHAN button is held...
		CHAN ^= 8 >> col; // Modify the CHAN value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00010100) { // If the SEQ-SIZE button is held...
		// Modify the currently-recording-seq's size
		STATS[RECORDSEQ] = min(64, max(1, STATS[RECORDSEQ] ^ (128 >> (key % 8))));
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00011000) { // If the BPM command is held...
		BPM = max(16, min(200, BPM ^ (128 >> (key % 8)))); // Change the BPM rate
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00001110) { // If the SAVE command is held...
		saveSong((PAGE * 24) + key); // Save the current tempfile into a savefile slot
	} else if (CTRL == B00010110) { // If the CLOCK-MASTER command is held...
		CLOCKMASTER ^= 1; // Toggle the CLOCK-MASTER value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00011010) { // If the CHAN-LISTEN command is held...
		LISTEN ^= 8 >> col; // Modify the CHAN-LISTEN value
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
	} else if (CTRL == B00011100) { // If the LOAD command is held...
		loadSong((PAGE * 24) + key); // Load a save-slot's contents into its tempfile, and start using that file
	}

}


// Interpret an incoming keystroke, using a given button's row and column
void assignKey(byte col, byte row) {

	if (col == 0) { // If the keystroke is in the leftmost column...

		TO_UPDATE |= 1; // Flag the top LED-row for updating

		CTRL |= 1 << row; // Add this button to control-tracking

		// Commands that apply to both modes:
		if (CTRL == B00111111) { // TOGGLE RECORD-MODE special command...
			RECORDMODE ^= 1; // Toggle/untoggle the mode's tracking-variable
			RECORDNOTES = 255; // Disable note-recording, to avoid recording new notes automatically on a future toggle cycle
			ERASENOTES = 0; // Disable note-erasing, to avoid erasing new notes automatically on a future toggle cycle
			TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
			return; // No need to go through the rest of the function at this point
		}

		if (RECORDMODE) { // If RECORD-MODE is active...
			if (CTRL == B00100000) { // ERASE WHILE HELD special command...
				ERASENOTES = 1; // As the button is held down, start erasing notes
			}
			TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
		} else { // Else, if PLAY MODE is active...
			if (CTRL == B00100010) { // If this is a PAGE A special command...
				PAGE = 0; // Toggle to page A
				TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
			} else if (CTRL == B00100100) { // If this is a PAGE B special command...
				PAGE = 1; // Toggle to page B
				TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
			} //else if (CTRL == B00101000) { // If this is a PAGE C special command...
				//PAGE = 2; // Toggle to page C
				//TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating
			//}
		}

	} else { // Else, if the keystroke is in any of the other columns...

		if (CTRL == B00110011) { // GLOBAL PLAY/STOP special command, with a regular button-press to signal intent...
			toggleMidiClock(1); // Toggle the MIDI clock, with "1" for "the user did this, not a device"
			return; // Exit the function, so the keystroke isn't also interpreted as a note or slice command
		}

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
		TO_UPDATE |= 1; // Flag the top LED-row for updating
		if (RECORDMODE) { // If RECORD MODE is active...
			if (CTRL == B00100000) { // If ERASE WHILE HELD was held...
				ERASENOTES = 0; // Stop erasing notes
			}
			TO_UPDATE |= 252; // Flag the lower 6 LED-rows for updating
		}
	}
	CTRL &= ~(1 << row); // Remove this button from control-tracking
	TO_UPDATE |= 1; // Flag the top LED-row for updating
}

// Parse any incoming keystrokes in the Keypad grid
void parseKeystrokes() {
	if (!kpd.getKeys()) { return; } // If no keys are pressed, exit the function
	for (uint8_t i = 0; i < 10; i++) { // For every keypress slot...
		if (!kpd.key[i].stateChanged) { continue; } // If the key's state hasn't just changed, check the next key
		uint8_t keynum = (uint8_t)(kpd.key[i].kchar - 48); // Convert the key's unique ASCII character into a number that reflects its position
		uint8_t kcol = keynum % 5; // Get the key's column
		uint8_t krow = (uint8_t)((keynum - kcol) / 5); // Get the key's row
		if (kpd.key[i].kstate == PRESSED) { // If the button is pressed...
			assignKey(kcol, krow); // Interpret the keystroke
		} else if (kpd.key[i].kstate == RELEASED) { // Else, if the button is released...
			unassignKey(kcol, krow); // Interpret the key-release
		}
	}
}
