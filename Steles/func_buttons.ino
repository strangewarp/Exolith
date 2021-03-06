

// Parse a slice-keypress that was generated while in PLAY mode
void parsePlayPress(byte col, byte row) {

	byte seq = col + (row * 4) + (PAGE * 24); // Get the sequence that corresponds to the given column and row
	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity
	byte pg = ctrl & B00000001; // Get the PAGE button's status
	byte cue = ctrl & B00010000; // Get the CUE command's status
	byte nums = ctrl & B00001110; // Get the NUMBER commands' status
	byte off = ctrl & B00100000; // Get the OFF command's status

	if (!pg) { // If neither a PAGE-OFF, SCATTER, SCATTER UNSET, PAGE SCATTER UNSET, PAGE, SHIFT, or BPM command is held...
		RECORDSEQ = seq; // Set the most-recent-seq to the seq whose button was pressed, regardless of command
	}

	// Flip the number-values into the expected format for CUE/SLICE operations
	// (1 in lowest bit, 2 next, then 4)
	nums = ((nums & 8) >> 3) | ((nums & 4) >> 1) | ((nums & 2) << 1);

	if (ctrl == B00000001) { // If PAGE is held, and a regular button-press was made to signal intent...
		PAGE ^= 1; // Toggle between page A and page B
		TO_UPDATE |= 2; // Flag the second LED-row for updates
	} else if (ctrl == B00000011) { // If SHIFT is held...
		posCmd(col, row); // Shift the CUR32 value by calling posCmd(), which works in both PLAY MODE and RECORD MODE
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

			word whole = (STATS[seq] & 31) + 1; // Get the number of whole-notes in the seq
			byte slice = whole * 4; // Get the number of 32nd-notes in one of the seq's slice-chunks

			// Set the seq's given slice-position to correspond with the start of the curent global beat
			POS[seq] = ((slice * nums) + ((CUR32 % 32) % min(slice, 32)) + 1) % (whole * 32);

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
		TO_UPDATE |= 1; // Flag the top LED-row for an update
	}
	TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
}

// Interpret an incoming keystroke, using a given button's row and column
void assignKey(byte col, byte row, byte oldcmds) {

	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity

	//byte key = col + (row * 4); // Get the button-key that corresponds to the given column and row

	if (col == 0) { // If the keystroke is in the leftmost column...

		arpClear(); // Clear the arpeggiation-system's variables

		if (KEYFLAG) { // If this button's note is currently held in manual-mode...
			recordHeldNote(); // Record the currently-held-note-key's note, to prevent bad conflicts with other commands
		}

		if (ctrl == B00111111) { // If this was the final keystroke in a TOGGLE RECORD-MODE command...
			toggleRecordMode(); // Toggle into, or out of, RECORD-MODE
		} else if (RECORDMODE) { // Else, if TOGGLE RECORD-MODE wasn't pressed, and if RECORD-MODE is active...
			if (ctrl == B00111100) { // If ERASE NOTES is held...
				// Erase a tick's worth of notes from the RECORDSEQ at the current QUANTIZE-QRESET-OFFSET position
				eraseTick(applyOffset(1, applyQuantize(POS[RECORDSEQ])));
			}
			TO_UPDATE |= 253; // Flag the top LED-row, and bottom 6 LED-rows, for updating
		} else { // Else, if this is PLAY MODE...
			refreshPlayCustoms(ctrl, oldcmds); // If any custom PLAY MODE commands were held, flag their GUI to be refreshed
		}

	} else { // Else, if the keystroke is in any of the other columns...

		if (RECORDMODE) { // If RECORD-MODE is active...
			// Get a key that will be used to match the ctrl-row buttons to a function in the COMMANDS table
			byte kt = pgm_read_byte_near(KEYTAB + (BUTTONS & 63));
			if (!kt) { return; } // If the key from the key-table was not assigned to a function, exit this function
			((CmdFunc) pgm_read_word_near(&COMMANDS[kt])) (col - 1, row); // Run a function that corresponds to the keypress
		} else { // Else, if PLAY MODE is active...
			if (ctrl == B00110011) { // If this is a LOAD (SONG-PAGE 1) press...
				loadSong((col - 1) + (row * 4)); // Load a new song from SONG-PAGE 1
			} else if (ctrl == B00110111) { // If this is a LOAD (SONG-PAGE 2) press...
				loadSong((col - 1) + (row * 4) + 24); // Load a new song from SONG-PAGE 2
			} else if (ctrl == B00001111) { // If a RESET TIMING command is held...
				resetAllTiming(); // Reset all timing of all seqs and the global cue-point, and send a SONG-POSITION POINTER
			} else { // Else, if none of the aforementioned commands are held...
				parsePlayPress(col - 1, row); // Parse the PLAY-MODE button-press
			}
		}

	}

}

// Interpret a key-release according to whatever command-mode is active
void unassignKey(byte col, byte row, byte oldcmds) {

	if (RECORDMODE) { // If RECORD-MODE is active...

		if (col == 0) { // If this up-keystroke was in the leftmost column...

			if ((oldcmds == B00111100) || (oldcmds == B00011110)) { // If this was an ERASE NOTES or ERASE INVERSE NOTES press...
				file.sync(); // Sync any stored unwritten savefile-data
			}

			TO_UPDATE |= 253; // Flag the top LED-row, and bottom 6 LED-rows, for updating

		} else { // Else, if this was a regular button-press...

			arpRelease(); // Check whether this was the last released arp-note

			if (KEYFLAG && (modKeyPitch(col - 1, row) == KEYNOTE)) { // If this button's note is currently held in manual-mode...
				recordHeldNote(); // Record the released-key's note
			}

			if ((!oldcmds) && (!BUTTONS)) { // If no command-buttons are held, and no notes are remaining...
				file.sync(); // Sync any stored unwritten savefile-data (this includes data from recordHeldNote() calls)
			}

			TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for updating

		}

		RECPRESS = 0; // Regardless of which note was released, clear the "a note is being pressed" flag

	} else { // Else, if this is PLAY MODE...
		byte ctrl = BUTTONS & B00111111; // Get the currently-held control-buttons
		refreshPlayCustoms(ctrl, oldcmds); // If any custom PLAY MODE commands were held, flag their GUI to be refreshed
	}

}
