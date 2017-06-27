
// Parse a slice-keypress that was generated while in SLICE or OVERRVIEW mode
void parsePlayPress(byte col, byte row) {

	byte seq = col + (row * 4) + (PAGE * 24); // Get the sequence that corresponds to the given column and row
	byte nums = CTRL & B00011100; // Get the NUMBER commands that are being held
	byte cue = CTRL & B00000010; // Get the CUE command that's being held
	byte off = CTRL & B00100000; // Get the OFF command that's being held

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
			word chunk = (SEQ_STATS[seq] * 96) >> 3; // Get the number of ticks in one of the seq's slice-chunks
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

	uint8_t key = col + (row * 4); // Get the button-key that corresponds to the given column and row

	if (!CTRL) { // If no CTRL buttons are held...

		// Get a note that corresponds to the key, organized from bottom-left to top-right, with all modifiers applied;
		// And also get the note's velocity with a random humanize-offset
		uint8_t pitch = min(127, (OCTAVE * 12) + BASENOTE + ((23 - key) ^ 3));
		uint8_t velo = min(127, max(1, VELOCITY + (((int8_t)(HUMANIZE / 2)) - rand(HUMANIZE))));

		uint8_t play = 1; // Track whether a note should be played in response to this keystroke

		if (RECORDNOTES) { // If notes are being recorded...

			// Make a time-offset for the RECORDSEQ's current 16th-note, based on the QUANTIZE value;
			// this will be fed to the recordToSeq command to achieve the correct insert-position
			uint8_t down = SEQ_POS[RECORDSEQ] % QUANTIZE;
			uint8_t up = QUANTIZE - down;
			int8_t offset = (down <= up) ? (-down) : up;

			recordToSeq(offset, DURATION, 144 + CHAN, pitch, velo); // Record the note into the current RECORDSEQ slot

			if (offset >= 1) { // If the tick was inserted at an offset after the current position...
				return; // Exit the function without playing the note, because it will be played momentarily
			}

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

	} else if (CTRL == B00010000) { // If the BASENOTE button is held...
		BASENOTE = min(12, BASENOTE ^ (8 >> col)); // Modify the BASENOTE value
	} else if (CTRL == B00001000) { // If the OCTAVE button is held...
		OCTAVE = min(10, OCTAVE ^ (8 >> col)); // Modify the OCTAVE value
	} else if (CTRL == B00000100) { // If the DURATION button is held...
		DURATION ^= 128 >> (key % 8); // Modify the DURATION value
	} else if (CTRL == B00000010) { // If the SWITCH RECORDING SEQUENCE button is held...
		RECORDSEQ = (PAGE * 24) + key; // Switch to the seq that corresponds to the key-position on the active page
	} else if (CTRL == B00011000) { // If the VELOCITY button is held...
		VELOCITY = min(127, max(1, VELOCITY ^ (128 >> (key % 8)))); // Modify the VELOCITY value
	} else if (CTRL == B00010100) { // If the HUMANIZE button is held...
		HUMANIZE = min(127, max(1, HUMANIZE ^ (128 >> (key % 8)))); // Modify the HUMANIZE value
	} else if (CTRL == B00001100) { // If the TIME-QUANTIZE button is held...
		QUANTIZE = 8 >> col; // Modify the TIME-QUANTIZE value
	} else if (CTRL == B00010010) { // If the CHANNEL button is held...
		CHAN ^= 8 >> col; // Modify the CHANNEL value
	} else if (CTRL == B00001010) { // If the SEQ-SIZE button is held...
		SEQ_STATS[RECORDSEQ] ^= 63 & (128 >> (key % 8)); // Modify the currently-recording-seq's size
	} else if (CTRL == B00000110) { // If the BPM command is held...
		BPM = max(16, BPM ^ (128 >> (key % 8))); // Change the BPM rate
	} else if (CTRL == B00011100) { // If the SAVE command is held...
		saveSong(key); // Save the current tempfile into a savefile slot
	} else if (CTRL == B00011010) { // If the CLOCK-MASTER command is held...
		CLOCKMASTER ^= 1; // Toggle the CLOCK-MASTER value
	} else if (CTRL == B00010110) { // If the CHAN-LISTEN command is held...
		LISTEN ^= 8 >> col; // Modify the CHAN-LISTEN value
	} else if (CTRL == B00001110) { // If the LOAD command is held...
		loadSong(key); // Load a save-slot's contents into its tempfile, and start using that file
	}

}


// Interpret an incoming keystroke, using a given button's row and column
void assignKey(byte col, byte row) {

	if (col == 0) { // If the keystroke is in the leftmost column...

		CTRL |= 1 << row; // Add this button to control-tracking

		// Commands that apply to both modes:
		if (CTRL == B00001111) { // GLOBAL PLAY/STOP special command...
			toggleMidiClock(1); // Toggle the MIDI clock, with "1" for "the user did this, not a device"
		} else if (CTRL == B00111111) { // TOGGLE RECORD-MODE special command...
			RECORDMODE ^= 1; // Toggle/untoggle the mode's tracking-variable
			RECORDNOTES = 255; // Disable note-recording, to avoid recording new notes automatically on a future toggle cycle
			ERASENOTES = 0; // Disable note-erasing, to avoid erasing new notes automatically on a future toggle cycle
			return; // No need to go through the rest of the function at this point
		}

		if (RECORDMODE) { // If RECORD-MODE is active...
			if (CTRL == B00000001) { // ERASE WHILE HELD special command...
				ERASENOTES = 1; // As the button is held down, start erasing notes
			}
		} else { // Else, if PLAY MODE is active...
			if (CTRL == B00010001) { // PAGE A special command...
				PAGE = 0; // Toggle to page A
			} else if (CTRL == B00001001) { // PAGE B special command...
				PAGE = 1; // Toggle to page B
			} //else if (CTRL == B00000101) { // PAGE C special command...
				//PAGE = 2; // Toggle to page C
			//}
		}

		TO_UPDATE |= 252; // Flag the lower 6 LED-rows for updating

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
			if (CTRL == B00000001) { // If ERASE WHILE HELD was held...
				ERASENOTES = 0; // Stop erasing notes
			}
			TO_UPDATE |= 252; // Flag the lower 6 LED-rows for updating
		}
	}
	CTRL &= ~(1 << row); // Remove this button from control-tracking
}

// Parse any incoming keystrokes in the Keypad grid
void parseKeystrokes() {
	if (!kpd.getKeys()) { return; } // If no keys are pressed, exit the function
	for (byte i = 0; i < 10; i++) { // For every keypress slot...
		if (!kpd.key[i].stateChanged) { continue; } // If the key's state hasn't just changed, check the next key
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
