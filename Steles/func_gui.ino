
// Display the startup-animation
void startupAnimation() {
	for (byte i = 0; i < 5; i++) {
		for (byte row = 0; row < 8; row++) {
			lc.setRow(0, row, (pgm_read_dword_near(LOGO + row) >> ((i == 4) ? 16 : (8 * (3 - i)))) & 255);
		}
		delay(150);
		lc.clearDisplay(0);
		delay(35);
	}
}

// Get the SEQUENCE-ACTIVITY LED-values for a given GUI row within the lower 6 rows
byte getRowSeqVals(byte r) {
	byte ib = r << 2; // Get the row's left-side's global-array position
	return (STATS[ib] & 128) // Return the row's PLAYING info, from both pages, as a row's worth of bits
		| ((STATS[ib + 1] & 128) >> 1)
		| ((STATS[ib + 2] & 128) >> 2)
		| ((STATS[ib + 3] & 128) >> 3)
		| ((STATS[ib + 24] & 128) >> 4)
		| ((STATS[ib + 25] & 128) >> 5)
		| ((STATS[ib + 26] & 128) >> 6)
		| ((STATS[ib + 27] & 128) >> 7);
}

// Get the SCATTER-ACTIVITY LED-values for a given GUI row within the lower 6 rows
byte getRowScatterVals(byte r) {
	byte ib = r << 2; // Get the row's left-side's global-array position
	return ((!!(SCATTER[ib] & 7)) << 7) // Return the row's SCATTER info, from both pages, as a row's worth of bits
		| ((!!(SCATTER[ib + 1] & 7)) << 6)
		| ((!!(SCATTER[ib + 2] & 7)) << 5)
		| ((!!(SCATTER[ib + 3] & 7)) << 4)
		| ((!!(SCATTER[ib + 24] & 7)) << 3)
		| ((!!(SCATTER[ib + 25] & 7)) << 2)
		| ((!!(SCATTER[ib + 26] & 7)) << 1)
		| (!!(SCATTER[ib + 27] & 7));
}

// Update the GUI based on update-flags that have been set by the current tick's events
void updateGUI() {

	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity

	if (TO_UPDATE & 1) { // If the first row is flagged for a GUI update...
		if (RECORDMODE && ctrl) { // If this is RECORD-MODE, and any ctrl-buttons are held...
			if (ctrl == B00100000) { // If TOGGLE NOTE-RECORDING is pressed...
				lc.setRow(0, 0, RECORDNOTES * 255); // Light up row if notes are being recorded
			} else if (ctrl == B00010000) { // Else, if BASENOTE is pressed...
				lc.setRow(0, 0, BASENOTE); // Display BASENOTE value
			} else if (ctrl == B00001000) { // Else, if OCTAVE is pressed...
				lc.setRow(0, 0, OCTAVE); // Display OCTAVE value
			} else if (ctrl == B00000100) { // Else, if DURATION is pressed...
				lc.setRow(0, 0, DURATION); // Display DURATION value
			} else if (ctrl == B00000010) { // Else, if CHAN is pressed...
				lc.setRow(0, 0, CHAN); // Display CHAN value
			} else if (ctrl == B00101000) { // Else, if CLOCK-MASTER is pressed...
				lc.setRow(0, 0, CLOCKMASTER * 255); // Light up row if CLOCKMASTER mode is active
			} else if (ctrl == B00100100) { // Else, if QUANTIZE is pressed...
				lc.setRow(0, 0, QUANTIZE); // Display QUANTIZE value
			} else if (ctrl == B00100001) { // Else, if CONTROL-CHANGE is pressed...
				lc.setRow(0, 0, (CHAN >> 4) * 255); // Light up row if the CONTROL-CHANGE flag is active
			} else if (ctrl == B00011000) { // Else, if VELO is pressed...
				lc.setRow(0, 0, VELO); // Display VELO value
			} else if (ctrl == B00010100) { // Else, if HUMANIZE is pressed...
				lc.setRow(0, 0, HUMANIZE); // Display HUMANIZE value
			} else if (ctrl == B00010010) { // Else, if SEQ-SIZE is pressed...
				lc.setRow(0, 0, STATS[RECORDSEQ] & 127); // Display current record-seq's SIZE value
			} else if (ctrl == B00010001) { // Else, if BPM is pressed...
				lc.setRow(0, 0, BPM); // Display BPM value
			} else if (ctrl == B00000110) { // Else, if CHAN-LISTEN is pressed...
				lc.setRow(0, 0, LISTEN); // Display the CHAN-LISTEN value
			} else { // Else...
				// B00110000: If SWITCH RECORDING-SEQUENCE is held...
				// Or some other unassigned button-combination is held...
				lc.setRow(0, 0, 0); // Clear this row
			}
		} else { // Else, if this isn't RECORD-MODE, OR no ctrl-buttons are held...
			byte beat = CUR16 >> 4; // Get the current global beat
			byte quart = (CUR16 >> 2) & 3; // Get the current global quarter-note
			// Display the global beat in the 4 leftmost LEDs,
			// and the global quarter-note in the 4 rightmost LEDs
			lc.setRow(0, 0, ((beat >= 4) ? (~(255 >> (beat - 3))) : (128 >> beat)) | (8 >> quart));
		}
	}

	if (TO_UPDATE & 2) { // If the second row is flagged for a GUI update...
		// Display the current number of sustains (0-8)
		lc.setRow(0, 1, 128 >> SUST_COUNT);
	}

	if (TO_UPDATE & 252) { // If any of the bottom 6 rows are flagged for a GUI update...

		if (LOADMODE) { // If LOAD MODE is active...

			for (byte i = 0; i < 6; i++) { // For each of the bottom 6 GUI rows...

				// If the row is not flagged for an update, continue to the next row
				if ((~TO_UPDATE) & (4 << i)) { continue; }

				lc.setRow( // Set the LED-row based on the current display-row:
					0, // LED-grid 0 (only LED-grid used)...
					i, // LED-row corresponding to the current row...
					pgm_read_byte_near( // Get LED data from PROGMEM:
						MULTIGLYPH_NUMBERS // Get a number-glyph,
						+ (6 * ctrlToButtonIndex(ctrl)) // based on the highest pressed control-button,
						+ i // whose contents correspond to the current row
					)
				);

			}

		} else if (RECORDMODE) { // If RECORD MODE is active...

			for (byte i = 0; i < 6; i++) { // For each of the bottom 6 GUI rows...

				// If the row is not flagged for an update, continue to the next row
				if ((~TO_UPDATE) & (4 << i)) { continue; }

				// Holds the LED-row's contents, which will be assembled based on which commands are held
				byte row = 0;

				if (!ctrl) { // If no command-buttons are held...
					row = getRowSeqVals(i); // Get the row's standard SEQ values
				} else if (ctrl == B00100000) { // If TOGGLE NOTE-RECORDING is held...
					// Grab a section of the RECORDING-glyph for display, or its ARMED variant, if notes are being recorded
					row = pgm_read_byte_near((RECORDNOTES ? GLYPH_RECORDING_ARMED : GLYPH_RECORDING) + i);
				} else if (ctrl == B00010000) { // If BASENOTE command is held...
					row = pgm_read_byte_near(GLYPH_BASENOTE + i); // Grab a section of the BASENOTE glyph for display
				} else if (ctrl == B00001000) { // If OCTAVE command is held...
					row = pgm_read_byte_near(GLYPH_OCTAVE + i); // Grab a section of the OCTAVE glyph for display
				} else if (ctrl == B00000100) { // If DURATION command is held...
					row = pgm_read_byte_near(GLYPH_DURATION + i); // Grab a section of the DURATION glyph for display
				} else if (ctrl == B00000010) { // If CHAN command is held...
					row = pgm_read_byte_near(GLYPH_CHAN + i); // Grab a section of the CHAN glyph for display
				} else if (ctrl == B00000001) { // If INTERVAL-UP is held...
					row = pgm_read_byte_near(GLYPH_UP + i); // Grab a section of the UP-glyph for display
				} else if (ctrl == B00110000) { // If SWITCH-SEQ command is held...
					row = pgm_read_byte_near(GLYPH_RSWITCH + i); // Grab a section of the SWITCH-SEQ-glyph for display
				} else if (ctrl == B00101000) { // If CLOCK-MASTER command is held...
					row = pgm_read_byte_near(GLYPH_CLOCKMASTER + i); // Grab a section of the CLOCK-MASTER glyph for display
				} else if (ctrl == B00100100) { // If QUANTIZE command is held...
					row = pgm_read_byte_near(GLYPH_QUANTIZE + i); // Grab a section of the QUANTIZE glyph for display
				} else if (ctrl == B00100001) { // If CONTROL-CHANGE command is held...
					row = pgm_read_byte_near(GLYPH_CONTROLCHANGE + i); // Grab a section of the CONTROL-CHANGE glyph for display
				} else if (ctrl == B00011000) { // If VELO command is held...
					row = pgm_read_byte_near(GLYPH_VELO + i); // Grab a section of the VELO glyph for display
				} else if (ctrl == B00010100) { // If HUMANIZE command is held...
					row = pgm_read_byte_near(GLYPH_HUMANIZE + i); // Grab a section of the HUMANIZE glyph for display
				} else if (ctrl == B00010010) { // If SEQ-SIZE command is held...
					row = pgm_read_byte_near(GLYPH_SIZE + i); // Grab a section of the SEQ-SIZE glyph for display
				} else if (ctrl == B00010001) { // If BPM command is held...
					row = pgm_read_byte_near(GLYPH_BPM + i); // Grab a section of the BPM glyph for display
				} else if (ctrl == B00000110) { // If CHAN-LISTEN command is held...
					row = pgm_read_byte_near(GLYPH_LISTEN + i); // Grab a section of the CHAN-LISTEN glyph for display
				} else if (ctrl == B00001111) { // If ERASE WHILE HELD command is held...
					row = pgm_read_byte_near(GLYPH_ERASE + i); // Grab a section of the ERASE-glyph for display
				} else if (ctrl == B00000111) { // If INTERVAL-DOWN-RANDOM is held...
					// Grab a composite of the DOWN-glyph and RANDOM-glyph for display
					row = pgm_read_byte_near(GLYPH_DOWN + i) | pgm_read_byte_near(GLYPH_RANDOM + i);
				} else if (ctrl == B00000101) { // If INTERVAL-UP-RANDOM is held...
					// Grab a composite of the UP-glyph and RANDOM-glyph for display
					row = pgm_read_byte_near(GLYPH_UP + i) | pgm_read_byte_near(GLYPH_RANDOM + i);
				} else if (ctrl == B00000011) { // If INTERVAL-DOWN is held...
					row = pgm_read_byte_near(GLYPH_DOWN + i); // Grab a section of the DOWN-glyph for display
				}
				// If any ctrl-buttons are held in an unknown configuration,
				// then no action is taken, leaving "row" set to 0 (all row-LEDs off).

				lc.setRow(0, i + 2, row); // Set the LED-row based on the current display-row

			}

		} else { // Else, if PLAYING MODE is actve...
			// Check whether a SCATTER-related command is held:
			byte heldsc = (ctrl != B00111111) // Eliminate TOGGLE RECORD MODE-based false-positives
				&& ((ctrl & B00000011) == B00000011); // Make sure this is, indeed, a command with SCATTER shape
			for (byte i = 2; i < 8; i++) { // For each of the bottom 6 GUI rows...
				if (TO_UPDATE & (1 << i)) { // If the row is flagged for an update...
					// If a SCATTER-related command is held, display a row of SCATTER info; else display a row of SEQ info
					lc.setRow(0, i, heldsc ? getRowScatterVals(i - 2) : getRowSeqVals(i - 2));
				}
			}
		}

	}

	TO_UPDATE = 0; // Unset the GUI-row-update flags

}
