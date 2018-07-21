

// Initialize communications with the MAX72** chip,
// on regular digital pins.
void maxInitialize() {

	DDRD |= B11100000; // Set the Data, Clk, and CS pins as outputs

	PORTD &= B10111111; // Set the CS pin low (data latch)
	shiftOut(5, 7, MSBFIRST, 15); // Conduct a display-test
	shiftOut(5, 7, MSBFIRST, 0); // ^
	shiftOut(5, 7, MSBFIRST, 11); // Set scanlimit to max
	shiftOut(5, 7, MSBFIRST, 7); // ^
	shiftOut(5, 7, MSBFIRST, 9); // Decode is done in source
	shiftOut(5, 7, MSBFIRST, 0); // ^
	shiftOut(5, 7, MSBFIRST, 12); // Shutown mode: startup
	shiftOut(5, 7, MSBFIRST, 1); // ^
	shiftOut(5, 7, MSBFIRST, 10); // LED intensity: maximum
	shiftOut(5, 7, MSBFIRST, 15); // ^
	PORTD |= B01000000; // Set the CS pin high (data latch)

	// Clear all rows
	sendRow(0, 0);
	sendRow(1, 0);
	sendRow(2, 0);
	sendRow(3, 0);
	sendRow(4, 0);
	sendRow(5, 0);
	sendRow(6, 0);
	sendRow(7, 0);

	PORTD |= B01000000; // Set the CS pin high (data latch)

}

// Update the LED-data of a given row on the MAX72** chip
void sendRow(volatile byte row, volatile byte d) {
	PORTD &= B10111111; // Set the CS pin low (data latch)
	shiftOut(5, 7, MSBFIRST, row + 1); // Send the row's opcode
	shiftOut(5, 7, MSBFIRST, d); // Send its 1-byte LED-data payload
	PORTD |= B01000000; // Set the CS pin high (data latch)
}

// Display the number of a newly-loaded savefile
void displayLoadNumber() {
	byte c1 = SONG % 10; // Get the ones digit
	byte c2 = byte(floor(SONG / 10)) % 10; // Get the tens digit
	byte c3 = SONG >= 100; // Get the hundreds digit (only 0 or 1)
	sendRow(0, 0); // Blank out top row
	sendRow(1, 0); // Blank out second row
	for (byte i = 0; i < 6; i++) { // For each of the bottom 6 rows of the GUI...
		sendRow(
			i + 2, // Set this row...
			(pgm_read_byte_near(NUMBER_GLYPHS + (c1 * 6) + i) >> 5) // To a composite of all three digits, shifted in position accordingly
				| (pgm_read_byte_near(NUMBER_GLYPHS + (c2 * 6) + i) >> 1)
				| (pgm_read_byte_near(NUMBER_GLYPHS + (c3 * 6) + i) * c3)
		);
	}
	delay(750); // Pause the program for 3/4 of a second while the number is displayed
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
	return ((!!(SCATTER[ib] & 15)) << 7) // Return the row's SCATTER info, from both pages, as a row's worth of bits
		| ((!!(SCATTER[ib + 1] & 15)) << 6)
		| ((!!(SCATTER[ib + 2] & 15)) << 5)
		| ((!!(SCATTER[ib + 3] & 15)) << 4)
		| ((!!(SCATTER[ib + 24] & 15)) << 3)
		| ((!!(SCATTER[ib + 25] & 15)) << 2)
		| ((!!(SCATTER[ib + 26] & 15)) << 1)
		| (!!(SCATTER[ib + 27] & 15));
}

// Update the first row of LEDs
void updateFirstRow(byte ctrl) {
	if (RECORDMODE) { // If this is RECORD-MODE...
		if (ctrl == B00100000) { // If ARM RECORDING is pressed...
			sendRow(0, RECORDSEQ); // Display currently-recording sequence (in binary)
		} else if (ctrl == B00010000) { // If QUANTIZE is pressed...
			sendRow(0, QUANTIZE); // Display QUANTIZE value
		} else if (ctrl == B00001000) { // Else, if OCTAVE is pressed...
			sendRow(0, OCTAVE); // Display OCTAVE value
		} else if (ctrl == B00000100) { // Else, if VELO is pressed...
			sendRow(0, VELO); // Display VELO value
		} else if (ctrl == B00000010) { // Else, if TRACK is pressed...
			sendRow(0, 240 >> (TRACK * 4)); // Display TRACK value (illuminate left side for track 1, right for track 2)
		} else if (ctrl == B00110000) { // Else, if SEQ-SIZE is pressed...
			sendRow(0, STATS[RECORDSEQ] & 63); // Display current record-seq's SIZE value
		} else if (ctrl == B00011000) { // Else, if DURATION is pressed...
			sendRow(0, DURATION); // Display DURATION value
		} else if (ctrl == B00001100) { // Else, if HUMANIZE is pressed...
			sendRow(0, HUMANIZE); // Display HUMANIZE value
		} else if ((ctrl == B00101000) || (ctrl == B00100100)) { // Else, if CHAN or UPPER CHAN BITS is pressed...
			sendRow(0, CHAN); // Display CHAN value
		} else if (ctrl == B00100010) { // Else, if BPM is pressed...
			sendRow(0, BPM); // Display BPM value
		} else if (ctrl == B00100001) { // Else, if SWITCH RECORDING-SEQUENCE is held...
			sendRow(0, RECORDSEQ); // Show the currently-active seq
		} else if (ctrl == B00010001) { // Else, if QUANTIZE-RESET is held...
			sendRow(0, QRESET); // Display QRESET value
		} else if (ctrl == B00001001) { // Else, if CLOCK-MASTER is pressed...
			sendRow(0, CLOCKMASTER * 255); // Light up row if CLOCKMASTER mode is active
		} else { // Else...
			// If ARM-RECORDING or ERASE WHILE HELD is held...
			// Or some other unassigned button-combination is held...
			// Or no control-buttons are held...
			sendRow(0, 128 >> (CUR16 >> 4)); // Display the global cue's current beat
		}
	} else { // Else, if this isn't RECORD-MODE...
		if ((!LOADMODE) && (ctrl == B00100010)) { // If this isn't LOAD MODE (then this is PLAY MODE), and if BPM is held...
			sendRow(0, BPM); // Display BPM value
		} else { // Else, if this is LOAD MODE, or if this is PLAY MODE and a BPM command isn't held...
			sendRow(0, 128 >> (CUR16 >> 4)); // Display the global cue's current beat
		}
	}
}

// Update the second row of LEDs
void updateSecondRow() {
	if (RECORDMODE) { // If RECORDMODE is active...
		byte beat = POS[RECORDSEQ] >> 4; // Get the current beat in the RECORDSEQ
		byte b2 = beat % 8; // Get the current beat, wrapped by 8
		byte join = (2 << (POS[RECORDSEQ] >> 7)) - 1; // Light a number of LEDs equal to the number of times the beat has wrapped around
		sendRow(1, (join << (7 - b2)) | (join >> (b2 + 1))); // Display the RECORDSEQ's spatially-wrapped beat-value
	} else { // Else, if RECORDMODE isn't active...
		if ((BUTTONS & B00000011) == 3) { // If any SCATTER-shaped command is held...
			sendRow(1, SCATTER[RECORDSEQ]); // Display the most-recently-touched seq's SCATTER value
		} else { // Else, if a SCATTER-shaped command isn't being held...
			// Display the current number of sustains (0-8), with illumination inverted depending on which page is active
			sendRow(1, (255 >> SUST_COUNT) ^ (255 * (!PAGE)));
		}
	}
}

// Update the bottom LED-rows for LOAD-MODE
void updateLoadBottomRows(byte ctrl) {
	byte ind = ctrlToButtonIndex(ctrl); // Get the number that corresponds to the currently-held LOADPAGE-button
	for (byte i = 0; i < 6; i++) { // For each of the bottom 6 GUI rows...
		if (!(TO_UPDATE & (4 << i))) { continue; } // If the row is not flagged for an update, continue to the next row
		sendRow( // Set the LED-row based on the current display-row:
			i, // LED-row corresponding to the current row...
			pgm_read_byte_near( // Get LED data from PROGMEM:
				NUMBER_GLYPHS // Get a number-glyph,
				+ (6 * ind) // based on the highest pressed control-button,
				+ i // whose contents correspond to the current row
			)
			>> 3 // Shifted 3 LEDs to the right from its default left-border-hugging position
		);
	}
}

// Update the bottom LED-rows for RECORD-MODE
void updateRecBottomRows(byte ctrl) {

	// Get a key that will be used to match the ctrl-row buttons to a glyph in the GUI-GLYPHS table
	word kt = pgm_read_byte_near(KEYTAB + ctrl) * 6;

	byte row = 0; // Will hold the binary value to be sent to the row's LEDs

	for (byte i = 0; i < 6; i++) { // For each of the bottom 6 GUI rows...

		if (!(TO_UPDATE & (4 << i))) { continue; } // If the row is not flagged for an update, continue to the next row

		if (BLINK) { // If a BLINK is active...
			row = 240 >> (TRACK * 4); // Fill in the LED-row based on the currently-active TRACK
		} else if (!ctrl) { // Else, if no control-buttons are held...
			row = getRowSeqVals(i); // Get the row's standard SEQ values
		} else if (ctrl == B00111100) { // Else, if ERASE NOTES is held...
			// Read the given row in the ERASE NOTES glyph, with exclamation-points if RECORD NOTES is active
			row = pgm_read_byte_near(GLYPH_ERASE + i) & (240 | (RECORDNOTES * 15));
		} else { // Else, if control-buttons are held...
			row = pgm_read_byte_near(GLYPHS + kt + i); // Read the given row, in the given glyph
			if (ctrl == B00100000) { // If RECORDNOTES is held...
				row |= 15 * RECORDNOTES; // Add a blaze to the RECORDNOTES glyph, if RECORDNOTES is ARMED
			} else if (ctrl == B00000001) { // If REPEAT is held...
				row |= 15 * REPEAT; // Add a blaze to the REPEAT glyph, if REPEAT is ARMED
			}
		}

		sendRow(i + 2, row); // Set the LED-row based on the current display-row

	}

}

// Update the bottom LED-rows for PLAY-MODE
void updatePlayBottomRows(byte ctrl) {
	byte heldsc = (ctrl & B00000011) == B00000011; // Make sure this is, indeed, a command with SCATTER shape
	for (byte i = 2; i < 8; i++) { // For each of the bottom 6 GUI rows...
		if (TO_UPDATE & (1 << i)) { // If the row is flagged for an update...
			if (BLINK) { // If a BLINK is active within PLAY-mode...
				sendRow(i, 255); // Illuminate the entire row
				continue; // Skip to the next row
			}
			if (ctrl == B00100010) { // If a BPM command is held...
				sendRow(i, pgm_read_byte_near(GLYPHS + 96 + (i - 2))); // Display a line from the BPM glyph
			} else { // Else, if a regular PLAY MODE command is held...
				// If a SCATTER-related command is held, display a row of SCATTER info; else display a row of SEQ info
				sendRow(i, heldsc ? getRowScatterVals(i - 2) : getRowSeqVals(i - 2));
			}
		}
	}
}

// Update the 6 bottom rows of LEDs
void updateBottomRows(byte ctrl) {
	if (LOADMODE) { // If LOAD MODE is active...
		updateLoadBottomRows(ctrl); // Update the bottom LED-rows for LOAD-MODE
	} else if (RECORDMODE) { // If RECORD MODE is active...
		updateRecBottomRows(ctrl); // Update the bottom-rows for RECORD-MODE
	} else { // Else, if PLAYING MODE is actve...
		updatePlayBottomRows(ctrl); // Update the bottom LED-rows for PLAYING-MODE
	}
}

// Update the GUI based on update-flags that have been set by the current tick's events
void updateGUI() {

	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity

	if (TO_UPDATE & 1) { // If the first row is flagged for a GUI update...
		updateFirstRow(ctrl); // Update the first row of LEDs
	}

	if (TO_UPDATE & 2) { // If the second row is flagged for a GUI update...
		updateSecondRow(); // Update the second row of LEDs
	}

	if (TO_UPDATE & 252) { // If any of the bottom 6 rows are flagged for a GUI update...
		updateBottomRows(ctrl); // Update the 6 bottom rows of LEDs
	}

	TO_UPDATE = 0; // Clear all TO_UPDATE flags

}

