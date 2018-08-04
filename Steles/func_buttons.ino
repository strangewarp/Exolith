

// Parse a slice-keypress that was generated while in PLAY mode
void parsePlayPress(byte col, byte row) {

	byte seq = col + (row * 4) + (PAGE * 24); // Get the sequence that corresponds to the given column and row
	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity
	byte pg = ctrl & B00000001; // Get the PAGE button's status
	byte cue = ctrl & B00010000; // Get the CUE command's status
	byte nums = ctrl & B00001110; // Get the NUMBER commands' status
	byte off = ctrl & B00100000; // Get the OFF command's status

	RECORDSEQ = seq; // Set the most-recent-seq to the seq whose button was pressed, regardless of command

	// Flip the number-values into the expected format for CUE/SLICE operations
	// (1 in lowest bit, 2 next, then 4)
	nums = ((nums & 8) >> 3) | ((nums & 4) >> 1) | ((nums & 2) << 1);

	if (ctrl == B00000001) { // If PAGE is held, and a regular button-press was made to signal intent...
		PAGE ^= 1; // Toggle between page A and page B
		TO_UPDATE |= 2; // Flag the second LED-row for updates
	} else if (ctrl == B00000011) { // If SHIFT is held...
		// Shift the CUR16 value by a number of GLOBAL CUE slices equal to the column of the keystroke (-2, -1, +1, +2)
		CUR16 = byte(((int(CUR16) + (((char(col) - 2) + (col >= 2)) * 16)) + 128) % 128);
		TO_UPDATE |= 1; // Flag the top LED-row for updates
	} else if (ctrl == B00000101) { // If BPM is held...
		tempoCmd(col, row); // Cue a global BPM modification command, using the same function as in RECORD MODE
	} else if (ctrl == B00100001) { // If PAGE-OFF is held, and a regular button-press was made to signal intent...
		byte ptop = (col >> 1) * 24; // Get the position of the first seq on the user-selected page
		for (byte i = ptop; i < (ptop + 24); i++) { // For every seq on this page...
			resetSeq(i); // Reset the seq's contents
		}
	} else if (ctrl == B00110001) { // If PAGE-SCATTER-UNSET is held, and a regular button-press was made to signal intent...
		byte ptop = (col >> 1) * 24; // Get the position of the first seq on the user-selected page
		for (byte i = ptop; i < (ptop + 24); i++) { // For every seq on this page...
			SCATTER[i] = 0; // Unset the seq's SCATTER flags
		}
	} else if (pg && cue && (!off)) { // If this is a SCATTER command of any kind...
		SCATTER[seq] = nums; // Turn this command's numeric value into the seq's SCATTER-chance flags
	} else if (cue || off) { // If all other command-types are ruled out, and either a CUE or OFF command is present...
		if (cue || nums) { // If there is a CUE or any NUMBERS...
			if (!(off && (!(STATS[seq] & 128)))) { // If we aren't trying to apply an OFF cmd to a seq that is already off...
				CMD[seq] = (nums << 5) | ((!off) << 1) | (!!off); // Cue an on-or-off command at the given global time
			}
		} else { // Else, if there is neither CUE nor OFF+NUMBERS, that means this is an un-cued OFF command, so...
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

	if (ctrl && (ctrl != B00000101) && (ctrl != B00000011)) { // If this was a non-BPM, non-SHIFT, control-command...
		TO_UPDATE |= 4 << ((seq % 24) >> 2); // Flag the sequence's corresponding LED-row for an update
	}

}

// If any custom PLAY MODE commands were held, flag their GUI to be refreshed
void refreshPlayCustoms(byte ctrl, byte oldcmds) {
	if (
		(ctrl == B00000101) // If a BPM command is active...
		|| (oldcmds == B00000101) // Or the previous command was a BPM command...
	) {
		TO_UPDATE |= 253; // Flag the top LED-row and bottom 6 LED-rows for an update, to clear the BPM glyph
	} else if ( // Else...
		(ctrl == B00000011) // If a SHIFT command is active...
		|| (oldcmds == B00000011) // Or the previous command was SHIFT...
	) {
		TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
	}
}

// Interpret an incoming keystroke, using a given button's row and column
void assignKey(byte col, byte row, byte oldcmds) {

	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity

	//byte key = col + (row * 4); // Get the button-key that corresponds to the given column and row

	if (col == 0) { // If the keystroke is in the leftmost column...

		newGestureEntry(row); // Create a new entry in the GESTURE-tracking system for this keypress
		checkForGestures(); // Check whether any gestures have just been completed

		if (RECORDMODE) { // If RECORD-MODE is active...
			TO_UPDATE |= 253; // Flag the top LED-row, and bottom 6 LED-rows, for updating
		} else { // Else, if this is PLAY MODE...
			refreshPlayCustoms(ctrl, oldcmds); // If any custom PLAY MODE commands were held, flag their GUI to be refreshed
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
		byte ctrl = BUTTONS & B00111111; // Get the currently-held control-buttons
		refreshPlayCustoms(ctrl, oldcmds); // If any custom PLAY MODE commands were held, flag their GUI to be refreshed
	}
}

