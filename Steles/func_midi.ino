

// Parse a given MIDI command
void parseMidiCommand() {
	if (RECORDMODE) { // If RECORD-MODE is active...
		byte cmd = INBYTES[0] & 240; // Get the command-type
		byte chn = INBYTES[0] & 15; // Get the MIDI channel
		if ((BUTTONS & B00111111) == B00100000) { // If notes are currently being recorded...
			if ((CHAN & 15) == chn) { // If this command is within the currently-selected CHANNEL...
				if ((cmd >= 144) && (cmd <= 239)) { // If this is a valid command...
					recordToSeq(POS[RECORDSEQ], chn, INBYTES[1], INBYTES[2]); // Record the incoming MIDI command
				}
			}
		}
	}
	Serial.write(INBYTES, INTARGET); // Having parsed the command, send its bytes onward to MIDI-OUT
}

// Parse all incoming raw MIDI bytes
void parseRawMidi() {

	// While new MIDI bytes are available to read from the MIDI-IN port...
	while (Serial.available() > 0) {

		byte b = Serial.read(); // Get the frontmost incoming byte

		if (SYSIGNORE) { // If this is an ignoreable SYSEX command...
			if (b == 247) { // If this was an END SYSEX byte, clear SYSIGNORE and stop ignoring new bytes
				SYSIGNORE = false;
			}
		} else { // Else, accumulate arbitrary commands...

			if (INCOUNT >= 1) { // If a command's first byte has already been received...
				INBYTES[INCOUNT] = b;
				INCOUNT++;
				if (INCOUNT == INTARGET) { // If all the command's bytes have been received...
					parseMidiCommand(); // Parse the command
					memset(INBYTES, 0, sizeof(INBYTES) - 1); // Reset incoming-command-tracking vars
					INCOUNT = 0;
				}
			} else { // Else, if this is either a single-byte command, or a multi-byte command's first byte...

				byte cmd = b & 240; // Get the command-type of any given non-SYSEX command

				if (b == 248) { // TIMING CLOCK command
					Serial.write(b); // Send the byte to MIDI-OUT
					if (!CLOCKMASTER) { // If the clock is in MIDI FOLLOW mode...
						CUR16++; // Since we're sure we're on a new 16th-note, increase the current-16th-note variable
						if (!(CUR16 % 4)) { // If the global 16th-note is the first within a global quarter-note...
							TO_UPDATE |= 1; // Flag the global-cue row of LEDs for an update
						}
						iterateAll(); // Iterate through one step of all sequencing processes
					}
				} else if (b == 250) { // START command
					Serial.write(b); // Send the byte to MIDI-OUT
					if (!CLOCKMASTER) { // If the clock is in MIDI FOLLOW mode...
						resetAllSeqs(); // Reset sequences to their initial positions
						toggleMidiClock(false); // Toggle the MIDI clock, with an "external command" flag
					}
				} else if (b == 251) { // CONTINUE command
					Serial.write(b); // Send the byte to MIDI-OUT
					if (!CLOCKMASTER) { // If the clock is in MIDI FOLLOW mode...
						toggleMidiClock(false); // Toggle the MIDI clock, with an "external command" flag
					}
				} else if (b == 252) { // STOP command
					Serial.write(b); // Send the byte to MIDI-OUT
					if (!CLOCKMASTER) { // If the clock is in MIDI FOLLOW mode...
						haltAllSustains(); // Halt all currently-sustained notes
						toggleMidiClock(false); // Toggle the MIDI clock, with an "external command" flag
					}
				} else if (b == 247) { // END SYSEX MESSAGE command
					// If you're dealing with an END-SYSEX command while SYSIGNORE is inactive,
					// then that implies you've received either an incomplete or corrupt SYSEX message,
					// and so nothing should be done here.
				} else if (
					(b == 244) // MISC SYSEX command
					|| (b == 240) // START SYSEX MESSAGE command
				) { // Anticipate an incoming SYSEX message
					SYSIGNORE = true;
				} else if (
					(b == 242) // SONG POSITION POINTER command
					|| (cmd == 224) // PITCH BEND command
					|| (cmd == 176) // CONTROL CHANGE command
					|| (cmd == 160) // POLY-KEY PRESSURE command
					|| (cmd == 144) // NOTE-ON command
					|| (cmd == 128) // NOTE-OFF command
				) { // Anticipate an incoming 3-byte message
					INBYTES[0] = b;
					INCOUNT = 1;
					INTARGET = 3;
				} else if (
					(b == 243) // SONG SELECT command
					|| (b == 241) // MIDI TIME CODE QUARTER FRAME command
					|| (cmd == 208) // CHAN PRESSURE (AFTERTOUCH) command
					|| (cmd == 192) // PROGRAM CHANGE command
				) { // Anticipate an incoming 2-byte message
					INBYTES[0] = b;
					INCOUNT = 1;
					INTARGET = 2;
				}

			}
		}

	}

}

