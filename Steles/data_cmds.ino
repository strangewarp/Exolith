

// Store pointers to RECORD-MODE functions in PROGMEM
const CmdFunc COMMANDS[] PROGMEM = {
	genericCmd,       //  0: Unused duplicate pointer to genericCmd
	armCmd,           //  1: TOGGLE RECORDNOTES
	chanCmd,          //  2: CHANNEL
	clearCmd,         //  3: CLEAR NOTES
	clockCmd,         //  4: CLOCK-MASTER
	copyCmd,          //  5: COPY
	durationCmd,      //  6: DURATION
	genericCmd,       //  7: All possible note-entry commands
	humanizeCmd,      //  8: HUMANIZE
	octaveCmd,        //  9: OCTAVE
	pasteCmd,         // 10: PASTE
	posCmd,           // 11: SHIFT CURRENT POSITION
	quantizeCmd,      // 12: QUANTIZE
	repeatCmd,        // 13: TOGGLE REPEAT
	sizeCmd,          // 14: SEQ-SIZE
	switchCmd,        // 15: SWITCH RECORDING-SEQUENCE
	tempoCmd,         // 16: BPM
	trackCmd,         // 17: TRACK
	upperBitsCmd,     // 18: UPPER CHAN BITS
	veloCmd           // 19: VELOCITY
};

// Matches control-row binary button-values to the decimal values that stand for their corresponding functions in COMMANDS
// Note: These binary values are flipped versions of what is displayed in button-key.txt
const byte KEYTAB[] PROGMEM = {
	7,  //  0, 000000:  7, genericCmd (REGULAR NOTE)
	13, //  1, 000001: 13, repeatCmd
	17, //  2, 000010: 17, trackCmd
	11, //  3, 000011: 11, posCmd
	19, //  4, 000100: 19, veloCmd
	0,  //  5, 000101:  0, ignore
	0,  //  6, 000110:  0, ignore
	0,  //  7, 000111:  0, ignore
	9,  //  8, 001000:  9, octaveCmd
	4,  //  9, 001001:  4, clockCmd
	10, // 10, 001010: 10, pasteCmd
	0,  // 11, 001011:  0, ignore
	8,  // 12, 001100:  8, humanizeCmd
	0,  // 13, 001101:  0, ignore
	0,  // 14, 001110:  0, ignore
	3,  // 15, 001111:  3, clearCmd
	12, // 16, 010000: 12, quantizeCmd
	0,  // 17, 010001:  0, ignore
	0,  // 18, 010010:  0, ignore
	0,  // 19, 010011:  0, ignore
	5,  // 20, 010100:  5, copyCmd
	0,  // 21, 010101:  0, ignore
	0,  // 22, 010110:  0, ignore
	0,  // 23, 010111:  0, ignore
	6,  // 24, 011000:  6, durationCmd
	0,  // 25, 011001:  0, ignore
	0,  // 26, 011010:  0, ignore
	0,  // 27, 011011:  0, ignore
	0,  // 28, 011100:  0, ignore
	0,  // 29, 011101:  0, ignore
	0,  // 30, 011110:  0, ignore
	0,  // 31, 011111:  0, ignore
	1,  // 32, 100000:  1, armCmd
	15, // 33, 100001: 15, switchCmd
	16, // 34, 100010: 16, tempoCmd
	0,  // 35, 100011:  0, ignore
	18, // 36, 100100: 18, upperBitsCmd
	0,  // 37, 100101:  0, ignore
	0,  // 38, 100110:  0, ignore
	0,  // 39, 100111:  0, ignore
	2,  // 40, 101000:  2, chanCmd
	0,  // 41, 101001:  0, ignore
	0,  // 42, 101010:  0, ignore
	0,  // 43, 101011:  0, ignore
	0,  // 44, 101100:  0, ignore
	0,  // 45, 101101:  0, ignore
	0,  // 46, 101110:  0, ignore
	0,  // 47, 101111:  0, ignore
	14, // 48, 110000: 14, sizeCmd
	0,  // 49, 110001:  0, ignore
	0,  // 50, 110010:  0, ignore
	0,  // 51, 110011:  0, ignore
	0,  // 52, 110100:  0, ignore
	0,  // 53, 110101:  0, ignore
	0,  // 54, 110110:  0, ignore
	0,  // 55, 110111:  0, ignore
	0,  // 56, 111000:  0, ignore
	0,  // 57, 111001:  0, ignore
	0,  // 58, 111010:  0, ignore
	0,  // 59, 111011:  0, ignore
	0,  // 60, 111100:  0, ignore (ERASE-WHILE-HELD is handled by other routines)
	0,  // 61, 111101:  0, ignore
	0,  // 62, 111110:  0, ignore
	0,  // 63, 111111:  0, ignore
};

