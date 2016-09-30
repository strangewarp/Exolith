
// Parse a slice-keypress that was generated while in SLICE or OVERRVIEW mode
void slicePress(byte col, byte row, byte seq, byte slice) {

    boolean rightheld = (LEFTCTRL & B00000001) > 0;
    boolean flingheld = (LEFTCTRL & B00010000) > 0;

    if (flingheld) { // If the FLING button is held...
        if (FLINGSEQ == 255) { // If the FLINGSEQ slot hasn't been filled by a FLING keypress yet...
            FLINGSEQ = seq + (rightheld ? 25 : 0); // Put the given sequence into the FLINGSEQ slot, plus 25 if the RIGHT-SIDE-key is held
        } else { // Else, if the FLINGSEQ slot is filled...
            SLICE_SLOT[col] = FLINGSEQ; // Fill the SLICE_SLOT corresponding to the key's column with the seq in the FLINGSEQ slot
            FLINGSEQ = 255; // Set the FLINGSEQ slot to an empty dummy-value
        }
    } else if (BOTCTRL == B00000001) { // If only the OVERVIEW button is held...
        SEQ_POS[seq] = 0; // Set the seq's position to correspond with the first slice-point, which is 0
        SEQ_PLAYING[seq >> 3] |= 1 << (seq & 7); // Set the seq's PLAYING-bit to 1
    } else if (BOTCTRL == B00000011) { // If the OVERVIEW-OFF chord is held...
        SEQ_POS[seq] = 0; // Reset the seq's position
        SEQ_PLAYING[seq >> 3]
    } else if (BOTCTRL == B00001001) { // If the OVERVIEW-CUE-1 chord is held...

    } else if (BOTCTRL == B00010001) { // If the OVERVIEW-CUE-8 chord is held...

    } else if (BOTCTRL == B00000111) { // If the OVERVIEW-OFF-SCATTER chord is held...

    } else if (BOTCTRL == B00001011) { // If the OVERVIEW-OFF-CUE-1 chord is held...

    } else if (BOTCTRL == B00010011) { // If the OVERVIEW-OFF-CUE-8 chord is held...

    } else if (BOTCTRL == B00000010) { // If the OFF button is held...

    } else if (BOTCTRL == B00000100) { // If the SCATTER button is held...

    } else if (BOTCTRL == B00001000) { // If the CUE-1 button is held...

    } else if (BOTCTRL == B00010000) { // If the CUE-8 button is held...


    } else if (LEFTCTRL == B00000010) {




    } else if ((BOTCTRL == 0) && ((LEFTCTRL & 30) == 0)) { // If no control-buttons are being held (aside from, optionally, the RIGHT-SIDE button)...
        SEQ_POS[seq] = (96 * SEQ_LEN[seq]) * ((slice - 1) / 8); // Set the seq's position to correspond with the given slice-point
    }



}

// Parse a note-keypress that was generated while in RECORDING mode
void recPress(byte col, byte row) {



}


// Interpret an incoming keystroke, using a given button's row and column
void assignCommandAction(byte col, byte row) {
	if (row <= 4) { // If keystroke is in the top 5 rows...
		if (col == 0) { // If keystroke is in column 1...
            LEFTCTRL |= 1 << row; // Add the button-row's corresponding bit to the LEFTCTRL byte
		} else { // If keystroke is right of column 1...
			if (RECORDING) { // If RECORDING mode is active...
                recPress(col - 1, row); // Parse the RECORDING-mode button-press
			} else if ((BOTCTRL & B00010000) > 0) { // Else, if OVERVIEW MODE is active...
				slicePress(col, row, (5 * row) + col, 1); // Send a keypress for slice 1 of the sequence that corresponds to this overview-button
			} else { // Else, if SLICE MODE is active...
				slicePress(col, row, SLICE_ROW[row], (col + (4 * SLICE_SIDE[row])) - 1); // Send a slice-keypress for the seq's selected column
			}
		}
	} else { // Else, if keystroke is in the bottom row...
        BOTCTRL |= 1 << col; // Add the button-column's corresponding bit to the BOTCTRL byte
        if (BOTCTRL == B00001111) { // If a RECORD-MODE-toggling command has been activated...
            RECORDING = !RECORDING; // Toggle/untoggle RECORD MODE
        } else if (BOTCTRL == B00010101) { // If a GLOBAL PLAY/STOP command has been activated...
            toggleMidiClock(); // Toggle the MIDI clock, if applicable
        }
	}
}

// Interpret a key-release according to whatever command-mode is active
void unassignCommandAction(byte col, byte row) {
    if (row == 5) { // If this up-keystroke was in the bottommost row...
        if (col == 0) { // If this is the leftmost button in said row...
            FLINGSEQ = 255; // Put an empty dummy-value into the FLINGSEQ slot, to prevent old partially-complete FLINGs from affecting later behavior
        }
        BOTCTRL ^= 1 << col; // Remove keystroke's corresponding bit from the BOTCTRL byte
    } else if (col == 0) { // Else, if this up-keystroke was in the leftmost column...
        LEFTCTRL ^= 1 << row; // Remove keystroke's corresponding bit from the LEFTCTRL byte
    }
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
					assignCommandAction(kcol, krow); // Interpret the keystroke
				} else if (kpd.key[i].kstate == RELEASED) { // Else, if the button is released...
					unassignCommandAction(kcol, krow); // Interpret the key-release
				}
			}
		}
	}
}
