
// Toggle whether the MIDI CLOCK is playing, provided that this unit is in CLOCK MASTER mode
void toggleMidiClock(boolean usercmd) {
	PLAYING = !PLAYING; // Toggle/untoggle the var that tracks whether the MIDI CLOCK is playing
	if (CLOCKMASTER) { // If in CLOCK MASTER mode...
		if (PLAYING) { // If playing has just been enabled...
			Serial.write(250); // Send a MIDI CLOCK START command
			Serial.write(248); // Send a MIDI CLOCK TICK (dummy-tick immediately following START command, as per MIDI spec)
		} else { // Else, if playing has just been disabled...
			haltAllSustains(); // Halt all currently-broadcasting MIDI sustains
			Serial.write(252); // Send a MIDI CLOCK STOP command
			if (usercmd) { // If this toggle was sent by a user-command...
				resetAllSeqsInSlice(); // Reset each sequence to the start of its currently-active slice
			} else { // Else, if this toggle was sent by an external device...
				resetAllSeqs(); // Reset the internal pointers of all sequences
			}
		}
	} else { // If in CLOCK FOLLOW mode...
		if (PLAYING) { // If PLAYING has just been enabled...
			CURTICK = 1; // Set the global sequencing tick to 1
			if (!usercmd) { // If this toggle was sent by an external device...
				DUMMYTICK = true; // Flag the sequencer to wait for an incoming dummy-tick, as per MIDI spec
			}
		} else { // Else, if playing has just been disabled...
			haltAllSustains(); // Halt all currently-broadcasting MIDI sustains
			if (!usercmd) {
				resetAllSeqs(); // Reset the internal pointers of all sequences
			}
		}
	}
}

// Parse a given MIDI command
void parseMidiCommand() {
	if (RECORDMODE) { // If RECORDING-mode is active...
		byte cmd = INBYTES[0] & 240; // Get the command-type
		if (cmd == 144) { // If this is a NOTE-ON command...
			RECPITCHES = (RECPITCHES << 4) | (INBYTES[1] % 12); // Shift the RECENT-PITCHES leftward, and add the note's pitch
		}
		if (RECORDNOTES) { // If notes are currently being recorded...
			recordToSeq(0, cmd, INBYTES[0] & 15, INBYTES[1], INBYTES[2]); // Record the incoming MIDI command
		}
	}
	for (byte i = 0; i < INTARGET; i++) { // Having parsed the command, send its bytes onward to MIDI-OUT
		Serial.write(INBYTES[i]);
	}
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
					INBYTES[0] = 0; // Reset incoming-command-tracking vars
					INBYTES[1] = 0;
					INBYTES[2] = 0;
					INCOUNT = 0;
				}
			} else { // Else, if this is either a single-byte command, or a multi-byte command's first byte...

				byte cmd = b & 240; // Get the command-type of any given non-SYSEX command

				if (b == 248) { // TIMING CLOCK command
					Serial.write(b); // Send the byte to MIDI-OUT
					if (!CLOCKMASTER) { // If the clock is in MIDI FOLLOW mode...
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
					|| (cmd == 208) // CHANNEL PRESSURE (AFTERTOUCH) command
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
