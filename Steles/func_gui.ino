

// Initialize communications with the MAX72** chip,
// on regular digital pins.
void maxInitialize() {

	DDRD |= B11100000; // Set the Data, Clk, and CS pins as outputs

	sendMaxCmd(15, 0); // Conduct a display-test
	sendMaxCmd(11, 7); // Set scanlimit to max
	sendMaxCmd(9, 0); // Decode is done in source

	// Clear all rows
	sendRow(0, 0);
	sendRow(1, 0);
	sendRow(2, 0);
	sendRow(3, 0);
	sendRow(4, 0);
	sendRow(5, 0);
	sendRow(6, 0);
	sendRow(7, 0);

	sendMaxCmd(12, 0); // Shutown mode: shutdown
	sendMaxCmd(12, 1); // Shutown mode: startup
	sendMaxCmd(10, 15); // LED intensity: maximum

}

// Update the LED-data of a given row on the MAX72** chip
void sendRow(volatile byte row, volatile byte d) {
	sendMaxCmd(row + 1, d); // Send the row's opcode and data byte
}

// Send a command to the MAX72** chip
void sendMaxCmd(volatile byte cmd, volatile byte d) {
	PORTD &= B10111111; // Set the CS pin low (data latch)
	shiftOut(5, 7, MSBFIRST, cmd); // Send the command's opcode
	shiftOut(5, 7, MSBFIRST, d); // Send the command's 1-byte LED-data payload
	PORTD |= B01000000; // Set the CS pin high (data latch)
}

// Send a blinking glyph that signifies an invalid BPM has been loaded
void sendInvalidBPMGlyph() {
	sendRow(0, 0); // Clear the top 3 LED-rows
	sendRow(1, 0);
	sendRow(2, 0);
	for (byte i = 1; i < 21; i++) { // Iterate through 20 on-or-off blink-states...
		byte i2 = !!(i % 4); // Get whether this is an on-blink or an off-blink
		// Depending on blink-status, either send an "invalid BPM" glyph to the screen, or totally clear the screen
		sendRow(3, i2 * B01010001);
		sendRow(4, i2 * B01001010);
		sendRow(5, i2 * B01000100);
		sendRow(6, i2 * B11001010);
		sendRow(7, i2 * B11010001);
		delay(250); // Hold the glyph's current state for a quarter-second
	}
}

// Display the number of a newly-loaded savefile
void displayLoadNumber() {
	byte c1 = SONG % 10; // Get the ones digit
	byte c2 = byte(floor(SONG / 10)) % 10; // Get the tens digit
	byte c3 = SONG >= 100; // Get the hundreds digit (only 0 or 1)
	//sendRow(0, 0); // Blank out top row
	sendRow(1, 0); // Blank out second row
	for (byte i = 0; i < 6; i++) { // For each of the bottom 6 rows of the GUI...
		sendRow(
			i + 2, // Set this row...
			(pgm_read_byte_near(NUMBER_GLYPHS + (c1 * 6) + i) >> 5) // To a composite of all three digits, shifted in position accordingly
				| (pgm_read_byte_near(NUMBER_GLYPHS + (c2 * 6) + i) >> 1)
				| (pgm_read_byte_near(NUMBER_GLYPHS + (c3 * 6) + i) * c3)
		);
	}
}

// Flag the LEDs that correspond to the current TRACK to blink for ~12ms
void recBlink() {
	BLINKL |= (!TRACK) * 192; // Start a given-track-linked LED-BLINK that is ~12ms long
	BLINKR |= TRACK * 192; // ^
	TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
}

// Get the LED-value for a single seq, adjusted based on whether it's active and whether it has a dormant cue-command
byte getSeqVal(byte s) {
	return (CMD[s] && (!(CUR32 % 8))) ? (((CUR32 + (!(STATS[s] & 128))) % 2) * 128) : (STATS[s] & 128);
}

// Get the SEQUENCE-ACTIVITY LED-values for a given GUI row within the lower 6 rows
byte getRowSeqVals(byte r) {
	byte ib = r << 2; // Get the row's left-side's global-array position
	return getSeqVal(ib) // Return the row's PLAYING info, from both pages, as a row's worth of bits
		| (getSeqVal(ib + 1) >> 1)
		| (getSeqVal(ib + 2) >> 2)
		| (getSeqVal(ib + 3) >> 3)
		| (getSeqVal(ib + 24) >> 4)
		| (getSeqVal(ib + 25) >> 5)
		| (getSeqVal(ib + 26) >> 6)
		| (getSeqVal(ib + 27) >> 7);
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

// Send a virtual char-value to the top LED-row, for "byte" values whose contents represent a virtual negative-to-positive range
void sendVirtualCharRow(byte n, byte mid) {
	if (n < mid) { // If the value is less than the range's midpoint...
		sendRow(0, 128 | (mid - n)); // Send an inverted version of the byte's contents, combined with a "negative" indicator-LED
	} else { // Else, if the value is at or above the range's midpoint...
		sendRow(0, n - mid); // Send an offset version of the byte's contents, without any "negative" LED-indication
	}
}

// Send a regular GLOBAL CUE value to the top LED-row
void sendRegularCueRow() {
	sendRow(0, 128 >> (CUR32 >> 5)); // Display the global cue's current beat
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
			sendRow(0, (DURATION == 129) ? 255 : DURATION); // Display DURATION value, or light the entire row for "manual duration" mode
		} else if (ctrl == B00001100) { // Else, if HUMANIZE is pressed...
			sendRow(0, HUMANIZE); // Display HUMANIZE value
		} else if (ctrl == B00000110) { // Else, if REPEAT-SWEEP is pressed...
			sendVirtualCharRow(RPTSWEEP, 128); // Send a virtual char-value to the top LED-row
		} else if ((ctrl == B00101000) || (ctrl == B00100100)) { // Else, if CHAN or UPPER CHAN BITS is pressed...
			sendRow(0, CHAN); // Display CHAN value
		} else if (ctrl == B00000101) { // Else, if BPM is pressed...
			sendRow(0, BPM); // Display BPM value
		} else if (ctrl == B00100001) { // Else, if SWITCH RECORDING-SEQUENCE is held...
			sendRow(0, RECORDSEQ); // Show the currently-active seq
		} else if (ctrl == B00100010) { // Else, if OFFSET is pressed...
			sendRow(0, (128 * (OFFSET < 0)) | abs(OFFSET)); // Show the current OFFSET value, with a "below-zero" marker if it's negative
		} else if (ctrl == B00001001) { // Else, if GRID-CONFIG is pressed...
			sendRow(0, GRIDCONFIG); // Display GRIDCONFIG value
		} else if (ctrl == B00010010) { // Else, if QUANTIZE-RESET is pressed...
			sendRow(0, QRESET); // Display QRESET value
		} else if (ctrl == B00010100) { // Else, if ARP-MODE is pressed...
			sendRow(0, ~(15 << (ARPMODE * 4))); // Display ARP-MODE value (0 = left side on; 1 = right side on; 2 = both sides on)
		} else if (ctrl == B00001010) { // Else, if ARP-REFRESH is pressed...
			sendRow(0, 255 * ARPREFRESH); // Display ARPREFRESH value (0 = empty row; 1 = full row)
		} else { // Else...
			// If ARM-RECORDING or ERASE WHILE HELD is held...
			// Or some other unassigned button-combination is held...
			// Or no control-buttons are held...
			sendRegularCueRow(); // Send a regular GLOBAL CUE value to the top LED-row
		}
	} else { // Else, if this isn't RECORD-MODE...
		if ((!LOADMODE) && (ctrl == B00000101)) { // If this isn't LOAD MODE (then this is PLAY MODE), and if BPM is held...
			sendRow(0, BPM); // Display BPM value
		} else { // Else, if this is LOAD MODE, or if this is PLAY MODE and a BPM command isn't held...
			sendRegularCueRow(); // Send a regular GLOBAL CUE value to the top LED-row
		}
	}
}

// Update the second row of LEDs
void updateSecondRow() {
	if (RECORDMODE) { // If RECORDMODE is active...
		byte braw = POS[RECORDSEQ] >> 5; // Get the seq's current whole-note
		byte b2 = braw % 8; // Get the seq's current whole-note, wrapped by 8
		byte join = (2 << (POS[RECORDSEQ] >> 8)) - 1; // Get a series of illuminated LEDs that correspond to the number of times the seq's display has wrapped around
		// Display the RECORDSEQ's spatially-wrapped whole-note, alternating its blinking-activity on every quarter-note
		sendRow(1, (!((POS[RECORDSEQ] & 8) >> 3)) * ((join << (7 - b2)) | (join >> (b2 + 1))));
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

		if (BLINKL || BLINKR) { // If a BLINK is active...
			row = (240 * (!!BLINKL)) | (15 * (!!BLINKR)); // Fill in the LED-row based on which BLINK-sides are active
		} else if (ctrl == B00111100) { // Else, if ERASE NOTES is held...
			// Read the given row in the ERASE NOTES glyph, with exclamation-points if RECORD NOTES is active
			row = pgm_read_byte_near(GLYPH_ERASE + i) & (240 | (RECORDNOTES * 15));
		} else if (ctrl) { // Else, if control-buttons are held...
			row = pgm_read_byte_near(GLYPHS + kt + i); // Read the given row, in the given glyph
			if (ctrl == B00100000) { // If RECORDNOTES is held...
				row |= 15 * RECORDNOTES; // Add a blaze to the RECORDNOTES glyph, if RECORDNOTES is ARMED
			} else if (ctrl == B00000001) { // If REPEAT is held...
				row |= 15 * REPEAT; // Add a blaze to the REPEAT glyph, if REPEAT is ARMED
			}
		} else if (RECPRESS && (BUTTONS >> 6)) { // Else, if control-buttons aren't held, but a recently-pressed note-button is held...
			byte rmod = RECNOTE % 12; // Get a number that corresponds to the keypress's in-octave note-value
			row =
				pgm_read_byte_near(GLYPH_PITCHES + (rmod * 6) + i) // Display a pitch-letter that corresponds to the currently-held note-button...
				| ((i == 5) ? min(9, byte(floor(RECNOTE / 12))) : 0); // ...Combined with the binary value of the note's octave
		} else { // Else, if no control-buttons or note-buttons are held...
			row = getRowSeqVals(i); // Get the row's standard SEQ values
		}

		sendRow(i + 2, row); // Set the LED-row based on the current display-row

	}

}

// Update the bottom LED-rows for PLAY-MODE
void updatePlayBottomRows(byte ctrl) {

	// Check whether a command with SCATTER-shape is held, and chec that it's not a RECORD-TOGGLE command
	byte heldsc = ((ctrl & B00010001) == B00010001) && (ctrl != B00111111);

	byte row = 0; // Will hold the binary value to be sent to the row's LEDs
	byte blnk = (240 * (!!BLINKL)) | (15 * (!!BLINKR)); // If there are any active BLINKs, create a row-template for them

	for (byte i = 2; i < 8; i++) { // For each of the bottom 6 GUI rows...
		if (TO_UPDATE & (1 << i)) { // If the row is flagged for an update...
			row = blnk; // If a BLINK is active, add it to the row's contents (also: this line clears the previous loop's row value)
			if (ctrl == B00000101) { // If a BPM command is held...
				row |= pgm_read_byte_near(GLYPHS + 112 + i); // Display a line from the BPM glyph
			} else if (ctrl == B00000011) { // Else, if a SHIFT POSITION command is held...
				row |= pgm_read_byte_near(GLYPHS + 70 + i); // Display a line from the SHIFT POSITION glyph
			} else if (ctrl == B00110111) { // Else, if a RESET TIMING command is held...
				row |= pgm_read_byte_near(GLYPH_RESET_TIMING + (i - 2)); // Display a line from the RESET TIMING glyph
			} else { // Else, if a regular PLAY MODE command is held...
				// If a SCATTER-related command is held, display a row of SCATTER info; else display a row of SEQ info
				row |= heldsc ? getRowScatterVals(i - 2) : getRowSeqVals(i - 2);
			}
			sendRow(i, row); // Send the composite row
		}
	}

}

// Update the 6 bottom rows of LEDs
void updateBottomRows(byte ctrl) {
	if (LOADHOLD) { // If a number from a just-loaded savefile is being held on screen...
		displayLoadNumber(); // Display the number of the newly-loaded savefile
	} else if (LOADMODE) { // If LOAD MODE is active...
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
