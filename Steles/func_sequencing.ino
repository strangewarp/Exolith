

// Reset every sequence, including SCATTER values
void resetAllSeqs() {
	memset(CMD, 0, 48);
	memset(POS, 0, 96);
	memset(SCATTER, 0, 48);
	for (byte i = 0; i < 48; i++) {
		STATS[i] &= 63; // Reset all seqs' PLAYING and CHAIN IGNORE flags
	}
}

// Reset a seq's cued-commands, playing-flag, chain-ignore flag, and tick-position
void resetSeq(byte s) {
	CMD[s] = 0;
	POS[s] = 0;
	STATS[s] &= 63;
}

// Send a MIDI-CLOCK reset command to MIDI-OUT
void sendClockReset() {

	byte spos[6] = { // Create a series of commands to stop, adjust, and restart any devices downstream:
		252, // STOP
		242, 0, 0, // SONG-POSITION POINTER: beginning of every sequence
		250, // START
		0 // (Empty array entry)
	};

	Serial.write(spos, 5); // Send the series of commands to the MIDI-OUT circuit

	setBlink(0, 0, 0, 0); // Blink both sides of the LED-screen
	setBlink(1, 0, 0, 0); // ^

}

// Reset all timing of all seqs and the global cue-point, and send a SONG-POSITION POINTER
void resetAllTiming() {
	TICKCOUNT = 2; // Set the TICK-counter to lapse into a new 32nd-note on the next tick
	CUR32 = 255; // Reset the global cue-point
	memset(POS, 0, 96); // Reset each seq's internal tick-position
	sendClockReset(); // Send a MIDI-CLOCK reset command to MIDI-OUT
}

// Flush the MIDI-OUT buffer by sending all of its MIDI commands
void flushMIDI() {
	if (MOUT_COUNT) { // If there are any commands in the MIDI-command buffer...
		byte m3 = MOUT_COUNT * 3; // Get the number of outgoing bytes
		for (byte i = 0; i < m3; i += 3) { // For every command in the MIDI-OUT queue...
			Serial.write(MOUT + i, 2 + ((MOUT[i] % 224) <= 191)); // Send that command's number of bytes
		}
		memset(MOUT, 0, m3); // Clear the MOUT array of now-obsolete data
		MOUT_COUNT = 0; // Clear the MIDI buffer's command-counter
	}
}

// Compare a seq's CUE-commands to the global CUE-point, and parse them if the timing is correct
void parseCues(byte s, byte size) {

	if (CMD[s]) { // If the seq has any cued commands...

		if (CUR32 == (CMD[s] & B11100000)) { // If the global 32nd-note corresponds to the seq's cue-point...

			// Enable or disable the sequence's playing-bit
			STATS[s] = (STATS[s] & 127) | ((CMD[s] & 2) << 6);

			// Set the sequence's internal tick to a position based on the incoming SLICE bits.
			// NOTE: For cued OFF commands, a SLICE value can still be stored, but otherwise the seq's position will be set to 0
			POS[s] = word(size) * (CMD[s] & B00011100);

			CMD[s] = 0; // Clear the sequence's CUED-COMMANDS byte

		} else if ((CUR32 % 8) >= 2) { // Else, the cue-command is still dormant. So if the global tick isn't on an eighth-note or the 32nd-note immediately afterward...
			return; // Exit the function without flagging any TO_UPDATE rows
		}

		flagSeqRow(s); // Flag the sequence's corresponding LED-row for an update

	}

}

// Read a tick from a given position, with an offset applied
void readTick(byte s, byte offset, byte buf[]) {
	word loc = POS[s]; // Get the sequence's current internal tick-location
	if (offset) { // If an offset was given...
		// Apply the offset to the tick-location, wrapping around the sequence's size-boundary
		loc = (loc + offset) % (word(STATS[s] & 63) * 32);
	}
	// Navigate to the note's absolute position,
	// and compensate for the fact that each tick contains 8 bytes
	file.seekSet((FILE_BODY_START + (loc * 8)) + (FILE_SEQ_BYTES * s));
	file.read(buf, 8); // Read the data of the tick's notes
}

// Parse the contents of a given tick, and add them to the MIDI-OUT buffer and SUSTAIN buffer as appropriate
void parseTickContents(byte s, byte buf[], byte dglyph) {

	for (byte bn = 0; bn < 8; bn += 4) { // For both event-slots on this tick...

		if (!buf[bn]) { continue; } // If this slot doesn't contain a command, then check the next slot or exit the loop

		// If this is the RECORDSEQ, in RECORD MODE, and RECORDNOTES isn't armed, and a command is present on this tick...
		if ((RECORDMODE) && (RECORDSEQ == s) && (!RECORDNOTES)) {
			byte bnbool = !!bn; // Get the boolean version of bn - "0 or 1" instead of "0 or 4"
			if (buf[bn] && !(dglyph && (bnbool == TRACK))) { // If this note-slot contains a command, and no GLYPH is already stored from a user REPEAT command...
				setBlink(bnbool, buf[bn], buf[bn + 1], buf[bn + 2]); // Set a BLINK-GLYPH according to the command-type in this TRACK
			}
		}

		if (buf[bn] == 112) { // If this is a BPM-CHANGE command...
			BPM = buf[bn + 1]; // Set the global BPM to the new BPM-value
			updateTickSize(); // Update the global tick-size to reflect the new BPM value
			continue; // Either check the next command-slot or exit the loop
		}

		// If the MIDI-OUT queue is full, then ignore this command, since we're now sure that it's a MIDI command
		// (This is checked here, instead of in iterateAll(), and using "continue" instead of "return",
		//  because all BPM-CHANGE commands need to be caught, regardless of how full MOUT_COUNT gets)
		if (MOUT_COUNT == 8) { continue; }

		memcpy(MOUT + (MOUT_COUNT * 3), buf + bn, 3); // Copy the event into the MIDI buffer's lowest empty MOUT location
		MOUT_COUNT++; // Increase the counter that tracks the number of bytes in the MIDI buffer

		if ((buf[bn] & 240) != 144) { continue; } // If this wasn't a NOTE-ON, forego any sustain mechanisms

		// If the function hasn't exited, then we're dealing with a NOTE-ON. So...

		// Send NOTE-OFFs for any duplicate SUSTAINs, and remove duplicate MOUT entries
		removeDuplicates(buf[bn], buf[bn + 1]);

		buf[bn] -= 16; // Turn the NOTE-ON into a NOTE-OFF
		buf[bn + 2] = buf[bn + 3]; // Move DURATION to the next byte over, for the 3-byte SUST-array note-format

		clipBottomSustain(); // If the bottommost SUSTAIN-slot is full, send its NOTE-OFF prematurely
		memmove(SUST + 3, SUST, SUST_COUNT * 3); // Move all sustains one space downward
		memcpy(SUST, buf + bn, 3); // Create a new sustain corresponding to this note
		SUST_COUNT++; // Increase the number of active sustains by 1

		TO_UPDATE |= 2 * (!RECORDMODE); // If we're not in RECORDMODE, flag the sustain-row for a GUI update

	}

}

// Parse any active SCATTER values that might be associated with a given seq
void parseScatter(byte s, byte dscatter) {

	if (dscatter) { // If this tick was a SCATTER-tick...
		SCATTER[s] &= 15; // Unset the "distance" side of this seq's SCATTER byte
	} else { // Else, if this wasn't a SCATTER-tick...

		// If this tick's random-value doesn't exceed the seq's SCATTER-chance value, exit the function
		if ((GLOBALRAND & 15) >= (SCATTER[s] & 15)) { return; }

		byte rnd = (GLOBALRAND >> 4) & 255; // Get 8 random bits

		if (!(rnd & 224)) { // If a 1-in-4 chance occurs...
			SCATTER[s] |= 128; // Set the seq's SCATTER-distance to be a whole-note
			return; // Exit the function
		}

		// Set the seq's SCATTER-distance to contain:
		// Half-note: 1/2 of the time
		// Quarter-note: 1/2 of the time
		// Eighth-note: 1/4 of the time
		byte dist = (rnd & 6) | (!(rnd & 24));

		if (!dist) { // If the new SCATTER-distance doesn't contain anything...
			SCATTER[s] |= 32; // Set the SCATTER-distance to a quarter-note
		} else { // Else, if the new SCATTER-distance contains something...
			SCATTER[s] |= dist << 4; // Make it into the seq's stored SCATTER-distance value
		}

	}

}

// Process a REPEAT, REPEAT-RECORD, or REPEAT-ERASE command
void processRepeat(byte ctrl, byte &dglyph) {

	if (REPEAT) { // If REPEAT is toggled...

		if (isInsertionPoint()) { // If this is the current insertion-point...

			if (RECORDNOTES && (ctrl == B00111100)) { // If RECORDNOTES is armed, and ERASE-NOTES is held...
				eraseTick(applyOffset(1, applyQuantize(POS[RECORDSEQ]))); // Erase any notes within the RECORDSEQ's current QUANTIZE-QRESET-OFFSET-tick
			} else if (ARPPOS && (!ctrl)) { // Else, if any note-buttons and no control-buttons are being held (regardless of whether RECORDNOTES is armed or not)...
				arpAdvance(); // Advance the arpeggio-position
				processRecAction(modPitch(ARPPOS & 31)); // Put the current raw repeat-note or arpeggiation-note into the current track
				RPTVELO = applyChange(RPTVELO, char(int(RPTSWEEP) - 128), 0, 127); // Change the stored REPEAT-VELOCITY by the REPEAT-SWEEP amount
				dglyph = 1; // Flag that a REPEAT-GLYPH is present, and should not be overridden by other notes on this tick in this TRACK
			}

		} else { // Else, if this isn't the current insertion point...

			if (RECORDNOTES && (ctrl == B00011110)) { // If RECORDNOTES is armed, and ERASE-INVERSE-NOTES is held...
				eraseTick(POS[RECORDSEQ]); // Erase any notes that do not occupy QUANTIZE-QRESET-OFFSET ticks
			}

		}

	}

}

// Get the notes from the current tick in a given seq, and add them to the MIDI-OUT buffer
void getTickNotes(byte ctrl, byte s, byte buf[]) {

	byte didglyph = 0; // Tracks whether this tick has had a user-generated REPEAT-GLYPH
	byte didscatter = 0; // Tracks whether this tick has had a SCATTER effect

	if (RECORDMODE) { // If RECORD MODE is active...
		if (RECORDSEQ == s) { // If this is the current RECORDSEQ...
			processRepeat(ctrl, didglyph); // Process any REPEAT, REPEAT-RECORD, or REPEAT-ERASE commands that might be occurring
		}
		readTick(s, 0, buf); // Read the tick with no offset
	} else { // Else, if RECORD MODE is inactive...
		// Read the tick with SCATTER-offset (>> 2, not 3 or 4, because we want a minimum of an 8th-note granularity)
		readTick(s, (SCATTER[s] & 240) >> 2, buf);
		didscatter = 1;
	}

	// If the function hasn't exited by this point, then that means this tick contained a note. So...

	parseTickContents(s, buf, didglyph); // Check the tick's contents, and add them to MIDI-OUT and SUSTAIN where appropriate
	parseScatter(s, didscatter); // Parse any active SCATTER values that might be associated with the seq

}

// Translate a CHAIN-direction into the key for the target-sequence that sits in that direction,
// also compensating for all vertical and horizontal wrapping, and keeping on the same PAGE as the parent-seq
byte chainDirToSeq(byte dir, byte parent) {

	// Get the offset that should be applied for a given CHAIN-direction, and add the parent-seq's position to this value
	dir = parent + pgm_read_byte_near(CHAIN_OFFSET + dir);

	byte pcol = parent % 4; // Get the parent-seq's button-column
	byte dcol = dir % 4; // ^ Same, for target-seq
	if ((pcol == 3) && (!dcol)) { // If this would wrap around the right side, compensate so that it remains centered on the same row
		dir -= 4;
	} else if ((!pcol) && (dcol == 3)) { // ^ Same, but for left side
		dir += 4;
	}

	// Wrap the target-seq's location onto a PAGE-sized numerical space,
	// and put it onto either the first or second PAGE depending on the parent-seq's PAGE-location
	return (dir % 24) + ((parent > 23) * 24);

}

// Iterate through all CHAIN-values, before parsing anything else on the tick
void iterateChains(byte buf[]) {

	if (RECORDMODE || DIDTOGGLE) { return; } // If RECORDMODE is active, or PLAY-MODE was just toggled, don't activate the CHAIN system at all

	for (byte i = 0; i < 48; i++) { // For every sequence in the song...

		if (
			CHAIN[i] // If this seq has any CHAIN DIRECTIONs set...
			&& (!POS[i]) // And it just looped back around to its first tick, on the previous iteration...
			&& ((STATS[i] & 192) == 128) // And it is currently playing, but hasn't been flagged by the CHAIN system on this tick...
		) {

			// Strip away the seq's "ON" flag, and its "CHAIN IGNORE" flag (though its CHAIN IGNORE flag should already be empty).
			// NOTE: We're leaving its CMD the same, because CHAINs shouldn't override their parent-seqs' cued-commands.
			STATS[i] &= B00111111;

			byte dircount = 0; // Will count the number of CHAIN-directions the seq has

			for (byte j = 0; j < 8; j++) { // For every possible CHAIN-direction...
				if (CHAIN[i] & (1 << j)) { // If that direction is filled in this seq's CHAIN entry...
					buf[dircount] = j; // Add the CHAIN entry to the buffer
					dircount++; // Increase the var that tracks the number of CHAIN-directions
				}
			}

			// Get a random chain-direction, if the seq has multiple possible directions to choose from,
			// and then get the target-seq's numerical key from that chain-direction
			byte target = chainDirToSeq(buf[byte(GLOBALRAND & 255) % dircount], i);

			// Set the ON flag, and the JUST CHAINED flag, for the target-seq.
			// NOTE: We don't need to change the target-seq's POS,
			//       because it will have been set to 0 by the seq's most-recent OFF or CUE-OFF command, or by savefile-loading;
			//       or it will have been set to the desired slice-value by a CUE-OFF-SLICE command.
			STATS[target] |= B1100000;

			flagSeqRow(i); // Flag the LED-row of the base seq, and the LED-row of its chain seq, for an update
			flagSeqRow(target); // ^

		}

	}

}

// Iterate all seqs, processing their CUEs, and processing any commands within their current ticks
void iterateSeqs(byte buf[]) {

	byte ctrl = BUTTONS & B00111111; // Get the control-row buttons' activity

	for (byte i = 47; i != 255; i--) { // For every sequence in the song, in reverse order...

		byte size = STATS[i] & 63; // Get seq's absolute size, in whole-notes

		parseCues(i, size); // Parse a sequence's cued commands, if any

		if (!(STATS[i] & 128)) { continue; } // If the seq isn't currently playing, go to the next seq's iteration-routine

		memset(buf, 0, 8); // Clear the buffer of junk data

		// Get the notes from this tick in a given seq, and add them to the MIDI-OUT buffer
		getTickNotes(ctrl, i, buf);

		POS[i] = (POS[i] + 1) % (word(size) * 32); // Increase the seq's 32nd-note position by one increment, wrapping it around its top limit

		if (STATS[i] & 64) { // If the seq was flagged as JUST CHAINED...
			STATS[i] &= B10111111; // Remove that flag, because its position has now been moved forward
		}

	}

}

// Advance global tick, and iterate through all currently-active sequences
void iterateAll() {

	byte buf[9]; // Buffer for: 1. collecting chain-directions, and 2. reading note-events from the datafile. (not at the same time)

	iterateChains(buf); // Iterate through all CHAIN-values, before parsing anything else on the tick

	iterateSeqs(buf); // Iterate all seqs, processing their CUEs, and processing any commands within their current ticks

	flushMIDI(); // Flush the MIDI-OUT buffer by sending all of its MIDI commands

	processSustains(); // Process one 32nd-note's worth of duration for all sustained notes

}
