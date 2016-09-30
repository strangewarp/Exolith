
// Toggle whether the MIDI CLOCK is playing, provided that this unit is in CLOCK MASTER mode
void toggleMidiClock() {
	if (CLOCKMASTER) { // If in CLOCK MASTER mode...
		PLAYING = !PLAYING; // Toggle/untoggle the var that tracks whether the MIDI CLOCK is playing
		if (PLAYING) { // If playing has just been enabled...
			Serial.write(250); // Send a MIDI CLOCK START command
			Serial.write(248); // Send a MIDI CLOCK TICK (dummy-tick immediately following START command, as per MIDI spec)
		} else { // Else, if playing has just been disabled...
			haltAllSustains(); // Halt all currently-broadcasting MIDI sustains
			Serial.write(252); // Send a MIDI CLOCK STOP command
			resetSeqs(); // Reset the internal pointers of all sequences
		}
	}
}


void parseSysex() {

	// testing code TODO remove
	/*
	lc.setRow(0, 0, 128 >> min(128, SYSBYTES[0]));
	lc.setRow(0, 1, 128 >> min(128, SYSBYTES[1]));
	lc.setRow(0, 2, 128 >> min(128, SYSBYTES[2]));
	lc.setRow(0, 3, 128 >> min(128, SYSBYTES[3]));
	lc.setRow(0, 4, 128 >> min(128, SYSBYTES[4]));
	lc.setRow(0, 5, 128 >> min(128, SYSBYTES[5]));
	lc.setRow(0, 6, 128 >> min(128, SYSBYTES[6]));
	lc.setRow(0, 7, 128 >> min(128, SYSBYTES[7]));
	*/


	// Send SYSEX command to MIDI-OUT, acting as a SOFT THRU;
	// and also clear the SYSBYTES array
	for (byte i = 0; i < 11; i++) {
		Serial.write(SYSBYTES[i]);
		SYSBYTES[i] = 0;
	}

}


//testing code TODO remove
byte TESTTICK = 95;
byte TESTCOUNT = 1;

void parseMidi() {

	// testing code TODO remove
	//lc.setRow(0, 2, INBYTES[0] >> 7);
	//lc.setRow(0, 3, INBYTES[0] % 128);
	lc.setRow(0, TESTCOUNT, INBYTES[1]);
	TESTCOUNT = max(1, (TESTCOUNT % 8) + 1);
	//lc.setRow(0, 7, INBYTES[2]);


	// TODO: write the actual version of this function:
	//				* Record commands when in Record Mode
	//				* Change internal state based on Ophiuchus SYSEX commands - each containing a checksum byte tacked onto its command byte

	// TODO: adapt this
	/*
	if (b == 252) { // STOP command
		PLAYING = false;
	} else if (b == 251) { // CONTINUE command
		PLAYING = true;
	} else if (b == 250) { // START command
		PLAYING = true;
		CURTICK = 0;
		//iterateTick();
	} else if (b == 248) { // TIMING CLOCK command
		CURTICK = (CURTICK + 1) % 768;
		//iterateTick();
	*/


}
