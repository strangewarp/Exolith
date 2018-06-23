

// Parse a slice-keypress that was generated while in PLAY mode
void parsePlayPress(byte col, byte row) {

	byte seq = col + (row * 4) + (PAGE * 24); // Get the sequence that corresponds to the given column and row
	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity
	byte pg = ctrl & B00000001; // Get the PAGE button's status
	byte cue = ctrl & B00000010; // Get the CUE command's status
	byte nums = ctrl & B00011100; // Get the NUMBER commands' status
	byte off = ctrl & B00100000; // Get the OFF command's status

	RECORDSEQ = seq; // Set the most-recent-seq to the seq whose button was pressed, regardless of command

	// Flip the number-values into the expected format for CUE/SLICE operations
	// (1 in lowest bit, 2 next, then 4)
	nums = (((nums & 16) >> 2) | ((nums & 4) << 2) | (nums & 8)) >> 2;

	if (ctrl == B00000001) { // If PAGE is held, and a regular button-press was made to signal intent...
		PAGE ^= 1; // Toggle between page A and page B
		BLINK = 255; // Cue a ~16ms LED-blink
		TO_UPDATE = 253; // Flag LED-rows 0 and 2-7 for updates
	} else if (ctrl == B00100010) { // If BPM is held...
		tempoCmd(col - 1, row); // Cue a global BPM modification command, using the same function as in RECORD MODE
		BLINK = 255; // Cue a ~16ms LED-blink
		TO_UPDATE |= 253; // Flag the top LED-row, and bottom 6 LED-rows, for an update
	} else if (ctrl == B00100001) { // If PAGE-OFF is held, and a regular button-press was made to signal intent...
		byte ptop = (col >> 1) * 24; // Get the position of the first seq on the user-selected page
		for (byte i = ptop; i < (ptop + 24); i++) { // For every seq on this page...
			resetSeq(i); // Reset the seq's contents
		}
		BLINK = 255; // Cue a ~16ms LED-blink
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
	} else if (ctrl == B00000011) { // If SCATTER UNSET is held...
		SCATTER[seq] = 0; // Unset the seq's SCATTER flags
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
		//TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
	} else if (ctrl == B00100011) { // If PAGE-SCATTER-UNSET is held, and a regular button-press was made to signal intent...
		byte ptop = (col >> 1) * 24; // Get the position of the first seq on the user-selected page
		for (byte i = ptop; i < (ptop + 24); i++) { // For every seq on this page...
			SCATTER[i] = 0; // Unset the seq's SCATTER flags
		}
		BLINK = 255; // Cue a ~16ms LED-blink
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
	} else if (pg && cue && nums && (!off)) { // If this is a SCATTER command of any kind...
		SCATTER[seq] = nums; // Turn this command's numeric value into the seq's SCATTER-chance flags
		TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
	} else if (cue | off) { // If either a CUE or OFF command is present...
		if (cue | nums) { // If there is a CUE or any NUMBERS...
			CMD[seq] = (nums << 5) | ((!off) << 1) | (!!off); // Cue an on-or-off command at the given global time
		} else { // Else, if there is neither CUE nor NUMBERS, that means this is an un-cued OFF command, so...
			if (STATS[seq] & 128) { // If the sequence is playing...
				TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
			}
			resetSeq(seq); // Reset the sequence
		}
	} else { // Else, if no special command is present...
		if (CMD[seq] & B00000010) { // If the sequence already contains a cued ON value...
			CMD[seq] = (CMD[seq] & B11100011) | (nums << 2); // Add a slice-position to the cued ON command
		} else { // Else, if the sequence has no cued ON value, then this is a pure beatslice, so...

			word beats = STATS[seq] & 63; // Get the number of beats in the seq
			byte slice = beats * 2; // Get the number of 16th-notes in one of the seq's slice-chunks

			// Set the seq's given slice-position to correspond with the start of the curent global beat
			POS[seq] = ((slice * nums) + ((CUR16 % 16) % min(slice, 16)) + 1) % (beats * 16);

			if (!(STATS[seq] & 128)) { // If the seq was previously turned off...
				TO_UPDATE |= 4 << row; // Flag the seq's corresponding LED-row for an update
			}

			STATS[seq] |= 128; // Set the sequence to playing

		}
	}

}

// Interpret an incoming keystroke, using a given button's row and column
void assignKey(byte col, byte row) {

	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity

	//byte key = col + (row * 4); // Get the button-key that corresponds to the given column and row

	if (col == 0) { // If the keystroke is in the leftmost column...

		newGestureEntry(row); // Create a new entry in the GESTURE-tracking system for this keypress
		checkForGestures(); // Check whether any gestures have just been completed

		if (RECORDMODE) { // If RECORD-MODE is active...
			TO_UPDATE |= 253; // Flag the top LED-row, and bottom 6 LED-rows, for updating
		} else { // Else, if this is PLAY MODE...
			if (ctrl == B00100010) { // And a BPM command is newly-held...
				TO_UPDATE |= 1; // Flag the top LED-row for an update
			}
		}

	} else { // Else, if the keystroke is in any of the other columns...

		if (LOADMODE) { // If LOAD-MODE is active...
			RECORDMODE = 0; // Toggle out of RECORD-MODE to prevent timing-mismatch bugs
			// Load a song whose file corresponds to the button that was pressed,
			// on a page that corresponds to the highest control-button that is being held
			loadSong((col - 1) + (row * 4) + (24 * ctrlToButtonIndex(ctrl)));
			LOADMODE = 0; // Exit LOAD-MODE automatically
		} else if (RECORDMODE) { // If RECORD-MODE is active...
			// Get a key that will be used to match the ctrl-row buttons to a function in the COMMANDS table
			byte kt = pgm_read_byte_near(KEYTAB + (BUTTONS & 63));
			if (!kt) { return; } // If the key from the key-table was invalid, exit the function
			((CmdFunc) pgm_read_word(&COMMANDS[kt])) (col - 1, row); // Run a function that corresponds to the keypress
		} else { // Else, if PLAY MODE is active...
			parsePlayPress(col - 1, row); // Parse the PLAY-MODE button-press
		}

	}

}

// Interpret a key-release according to whatever command-mode is active
void unassignKey(byte col, byte oldcmds) {
	if (RECORDMODE) { // If RECORD-MODE is active...
		if (col == 0) { // If this up-keystroke was in the leftmost column...
			if (oldcmds == B00111100) { // If this was an ERASE WHILE HELD press...
				file.sync(); // Sync any stored unwritten savefile-data
			}
			TO_UPDATE |= 253; // Flag the top LED-row, and bottom 6 LED-rows, for updating
		} else { // Else, if this was a regular button-press...
			if ((!oldcmds) && (!BUTTONS)) { // If no command-buttons are held, and no notes are remaining...
				file.sync(); // Sync any stored unwritten savefile-data
			}
		}
	} else { // Else, if this is PLAY MODE...
		if ((BUTTONS & B00111111) == B00100010) { // If a BPM command is revealed by the key-release...
			TO_UPDATE |= 1; // Flag the top LED-row for an update
		}
	}
}

