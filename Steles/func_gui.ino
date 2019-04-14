

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

// Flag the sequence's corresponding LED-row for an update
void flagSeqRow(byte s) {
	TO_UPDATE |= 4 << ((s % 24) >> 2);
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

// Fill the BLINK-values that are related to a given TRACK
void fillBlinkVals(byte glyph[], word &blink, byte cmd, byte pitch, byte velo) {
	if (cmd) {
		glyph[0] = cmd; // Store the MIDI-command for later use in the GUI-display functions
		if (cmd == 144) { // If this represents a NOTE command...
			glyph[1] = pitch % 12; // Put the note's pitch-value into the given GLYPH-var
		} else { // Else, if this represents a non-note MIDI-command...
			glyph[1] = pitch; // Store the MIDI-cmd's second and third bytes, for later display
			glyph[2] = velo; // ^
		}
		blink = 5000; // Start a track-linked long-LED-BLINK
	} else { // Else, if this is a "full-blink"...
		glyph[0] = 0; // Flag the glyph to display a "full-blink"
		blink = 200; // Start a track-linked short-LED-BLINK
	}
}

// Flag the LEDs that correspond to the given TRACK to blink, in various ways depending on whether they represent a NOTE, a special-command, or a "full-blink"
void setBlink(byte trk, byte cmd, byte pitch, byte velo) {
	if (trk) { // If TRACK 2 was given...
		fillBlinkVals(GLYPHR, BLINKR, cmd, pitch, velo); // Fill the BLINK-values that are related to the right-side TRACK
	} else { // Else, if TRACK 1 was given...
		fillBlinkVals(GLYPHL, BLINKL, cmd, pitch, velo); // Fill the BLINK-values that are related to the left-side TRACK
	}
	TO_UPDATE |= 252; // Flag the bottom 6 LED-rows for an update
}

// Get the LED-value for a single seq, adjusted based on whether it's active and whether it has a dormant cue-command
byte getSeqVal(byte s) {
	byte active = STATS[s] & 128;
	return (CMD[s] && (!(CUR32 % 8))) ? (((CUR32 + (!active)) % 2) * 128) : active;
}

// Get the SEQUENCE-ACTIVITY LED-values for a given GUI row
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

// Get the SEQUENCE-ACTIVITY LED-values for a given half-GUI-row
byte getHalfRowSeqVals(byte r, byte shift) {
	byte ib = (r << 2) + (shift * 6); // Get the row's global-array position, shifted by 24 if this represents PAGE 2
	return ( // Return the half-row's PLAYING info, as a shifted nibble of bits
			getSeqVal(ib)
			| (getSeqVal(ib + 1) >> 1)
			| (getSeqVal(ib + 2) >> 2)
			| (getSeqVal(ib + 3) >> 3)
		) >> shift;
}

// Get the SCATTER-ACTIVITY LED-values for a given GUI row
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

// Get a row that corresponds to a slice of the given BLINK's data
byte getBlinkRow(word b, byte g[], byte cmd, byte shift, byte i) {
	if (b) { // If the given BLINK is active...
		if (g[0]) { // If the stored GLYPH represents a MIDI-command or virtual-MIDI-command...
			if (cmd == 144) { // If the stored GLYPH represents a NOTE-ON or a virtual-NOTE-ON...
				// Return a slice of a letter-glyph that corresponds to the note's pitch-byte and to the given row and shift-position
				return pgm_read_byte_near(GLYPH_BLINK + ((g[1] % 12) * 5) + i) >> shift;
			} else { // Else, if the stored GLYPH represents another type of MIDI-command...
				return g[i >> 1] >> shift; // Return a slice of the stored command-GLYPH's data that corresponds to the given row and shift-position
			}
		} else { // Else, if the stored GLYPH represents a "full-BLINK"...
			return B11110000 >> shift; // Return a value that will fill 4 LED-bits in the current shift-position
		}
	}
	return getHalfRowSeqVals(i, shift); // Get the activity-values from a half-row of seqs, on PAGE 1 for left-side or 2 for right-side
}

// Display the contents of the GLYPHL/GLYPHR arrays, based on which BLINKs are active
void displayBlink() {
	byte lcmd = GLYPHL[0] & 240; // Get the left BLINK-GLYPH's MIDI-command status
	byte rcmd = GLYPHR[0] & 240; // ^ This, but right
	for (byte i = 0; i < 5; i++) { // For each one of the 6 bottom LED-rows...
		// Send a row made from a composite of the current LEFT-BLINK and RIGHT-BLINK rows, if they are active
		sendRow(i + 2, getBlinkRow(BLINKL, GLYPHL, lcmd, 0, i) | getBlinkRow(BLINKR, GLYPHR, rcmd, 4, i));
	}
	// If either of the GLYPHs is for a MIDI-NOTE, then display its/their CHANNELs as binary values on the bottom-row
	sendRow(7, ((BLINKL && (lcmd == 144)) ? ((GLYPHL[0] % 16) << 4) : 0) | ((BLINKR && (rcmd == 144)) ? (GLYPHR[0] % 16) : 0));
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
		} else if (ctrl == B00000110) { // Else, if DURATION-HUMANIZE is pressed...
			sendRow(0, DURHUMANIZE); // Display DURATION-HUMANIZE value
		} else if (ctrl == B00010001) { // Else, if REPEAT-SWEEP is pressed...
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
			// If ARM-RECORDING, ERASE WHILE HELD, or CHAIN DIRECTION is held...
			// Or some other unassigned button-combination is held...
			// Or no control-buttons are held...
			sendRegularCueRow(); // Send a regular GLOBAL CUE value to the top LED-row
		}
	} else { // Else, if this isn't RECORD-MODE...
		if (ctrl == B00000101) { // If a BPM command is held...
			sendRow(0, BPM); // Display BPM value
		} else { // Else, if a BPM command isn't held...
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

// Update the bottom LED-rows for RECORD-MODE
void updateRecBottomRows(byte ctrl) {

	// Get a key that will be used to match the ctrl-row buttons to a glyph in the GUI-GLYPHS table
	word kt = pgm_read_byte_near(KEYTAB + ctrl) * 6;

	byte row = 0; // Will hold the binary value to be sent to the row's LEDs

	for (byte i = 0; i < 6; i++) { // For each of the bottom 6 GUI rows...

		if (!(TO_UPDATE & (4 << i))) { continue; } // If the row is not flagged for an update, continue to the next row

		if (ctrl == B00111100) { // If ERASE NOTES is held...
			// Read the given row in the ERASE NOTES glyph, with exclamation-points if RECORD NOTES is active
			row = pgm_read_byte_near(GLYPH_ERASE + i) & (240 | (RECORDNOTES * 15));
		} else if (ctrl == B00011110) { // Else, if ERASE INVERSE NOTES is held...
			// Read the given row in the ERASE INVERSE NOTES glyph, with exclamation-points if RECORD NOTES is active
			row = pgm_read_byte_near(GLYPH_ERASE_INVERSE + i) & (15 | ((RECORDNOTES && REPEAT) * 240));
		} else if (ctrl == B00111000) { // Else, if CHAIN DIRECTION is held...
			byte left = 0;
			byte mid = 0;
			byte right = 0;
			if (i < 2) { // If this is in the top 2 glyph-rows...
				left = CHAIN[RECORDSEQ] & 1; // Left chunk gets CHAIN-bit 0
				mid = CHAIN[RECORDSEQ] & 2; // Middle chunk gets CHAIN-bit 1
				right = CHAIN[RECORDSEQ] & 4; // Right chunk gets CHAIN-bit 2
			} else if (i < 4) { // Else, if this is in the middle 2 glyph-rows...
				left = CHAIN[RECORDSEQ] & 8; // Left chunk gets CHAIN-bit 3
				mid = 1; // Middle chunk will be lit regardless, since it visually represents the sequence itself
				right = CHAIN[RECORDSEQ] & 16; // Right chunk gets CHAIN-bit 4
			} else { // Else, if this is in the bottom 2 glyph-rows...
				left = CHAIN[RECORDSEQ] & 32; // Left chunk gets CHAIN-bit 5
				mid = CHAIN[RECORDSEQ] & 64; // Middle chunk gets CHAIN-bit 6
				right = CHAIN[RECORDSEQ] & 128; // Right chunk gets CHAIN-bit 7
			}
			row = (96 * (!!left)) | (24 * (!!mid)) | (6 * (!!right)); // Make a composite out of the row-chunks
		} else if (ctrl) { // Else, if control-buttons are held...
			row = pgm_read_byte_near(GLYPHS + kt + i); // Read the given row, in the given glyph
			if (ctrl == B00100000) { // If RECORDNOTES is held...
				row |= 15 * RECORDNOTES; // Add a blaze to the RECORDNOTES glyph, if RECORDNOTES is ARMED
			} else if (ctrl == B00000001) { // If REPEAT is held...
				row |= 15 * REPEAT; // Add a blaze to the REPEAT glyph, if REPEAT is ARMED
			}
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
	for (byte i = 2; i < 8; i++) { // For each of the bottom 6 GUI rows...
		if (TO_UPDATE & (1 << i)) { // If the row is flagged for an update...
			row = 0; // Clear the previous loop's row value
			if (ctrl == B00000101) { // If a BPM command is held...
				row |= pgm_read_byte_near(GLYPHS + 118 + i); // Display a line from the BPM glyph
			} else if (ctrl == B00000011) { // Else, if a SHIFT POSITION command is held...
				row |= pgm_read_byte_near(GLYPHS + 76 + i); // Display a line from the SHIFT POSITION glyph
			} else if (ctrl == B00001111) { // Else, if a RESET TIMING command is held...
				row |= pgm_read_byte_near(GLYPH_RESET_TIMING + (i - 2)); // Display a line from the RESET TIMING glyph
			} else if ((ctrl == B00110011) || (ctrl == B00110111)) { // Else, if a LOAD command is held...
				// Display a line from the LOAD glyph, combined with a line from either the "1" or "2" glyph, based on the held LOAD-command
				row |= pgm_read_byte_near(GLYPH_LOAD + i) | (pgm_read_byte_near(NUMBER_GLYPHS + i + (6 << (!!(ctrl & 4)))) >> 4);
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
	} else if (RECORDMODE) { // If RECORD MODE is active...
		updateRecBottomRows(ctrl); // Update the bottom-rows for RECORD-MODE
	} else { // Else, if PLAY MODE is actve...
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
		if (ctrl || (!(BLINKL || BLINKR))) { // If any ctrl-buttons are held, or no BLINKs are active...
			updateBottomRows(ctrl); // Update the 6 bottom rows of LEDs
		} else { // Else, if BLINKS are active and no ctrl-buttons are held...
			displayBlink(); // Display the contents of the GLYPHL/GLYPHR arrays, based on which BLINKs are active
		}
	}

	TO_UPDATE = 0; // Clear all TO_UPDATE flags

}
