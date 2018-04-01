

// Store pointers to RECORD-MODE functions in PROGMEM
const CmdFunc COMMANDS[] PROGMEM = {
	genericCmd,       //  0: Unused duplicate pointer to genericCmd
	armCmd,           //  1: TOGGLE RECORDNOTES
	chanCmd,          //  2: CHANNEL
	clearCmd,         //  3: CLEAR NOTES
	clockCmd,         //  4: CLOCK-MASTER
	controlChangeCmd, //  5: CONTROL-CHANGE
	copyCmd,          //  6: COPY
	durationCmd,      //  7: DURATION
	genericCmd,       //  8: All possible note-entry commands
	humanizeCmd,      //  9: HUMANIZE
	octaveCmd,        // 10: OCTAVE
	pasteCmd,         // 11: PASTE
	posCmd,           // 12: SHIFT CURRENT POSITION
	programChangeCmd, // 13: PROGRAM-CHANGE
	quantizeCmd,      // 14: QUANTIZE
	repeatCmd,        // 15: TOGGLE REPEAT
	sizeCmd,          // 16: SEQ-SIZE
	switchCmd,        // 17: SWITCH RECORDING-SEQUENCE
	tempoCmd,         // 18: BPM
	veloCmd           // 19: VELOCITY
};

// Matches control-row binary button-values to the decimal values that stand for their corresponding functions in COMMANDS
// Note: These binary values are flipped versions of what is displayed in button-key.txt
const byte KEYTAB[] PROGMEM = {
	8,  //  0, 000000:  8, genericCmd (REGULAR NOTE)
	15, //  1, 000001: 15, repeatCmd
	2,  //  2, 000010:  2, chanCmd
	12, //  3, 000011: 12, posCmd
	19, //  4, 000100: 19, veloCmd
	0,  //  5, 000101:  0, ignore
	18, //  6, 000110: 18, tempoCmd
	0,  //  7, 000111:  0, ignore
	10, //  8, 001000: 10, octaveCmd
	4,  //  9, 001001:  4, clockCmd
	11, // 10, 001010: 11, pasteCmd
	0,  // 11, 001011:  0, ignore
	9,  // 12, 001100:  9, humanizeCmd
	0,  // 13, 001101:  0, ignore
	0,  // 14, 001110:  0, ignore
	3,  // 15, 001111:  3, clearCmd
	14, // 16, 010000: 14, quantizeCmd
	0,  // 17, 010001:  0, ignore
	13, // 18, 010010: 13, programChangeCmd
	0,  // 19, 010011:  0, ignore
	6,  // 20, 010100:  6, copyCmd
	0,  // 21, 010101:  0, ignore
	0,  // 22, 010110:  0, ignore
	0,  // 23, 010111:  0, ignore
	7,  // 24, 011000:  7, durationCmd
	0,  // 25, 011001:  0, ignore
	0,  // 26, 011010:  0, ignore
	0,  // 27, 011011:  0, ignore
	0,  // 28, 011100:  0, ignore
	0,  // 29, 011101:  0, ignore
	0,  // 30, 011110:  0, ignore
	0,  // 31, 011111:  0, ignore
	1,  // 32, 100000:  1, armCmd
	17, // 33, 100001: 17, switchCmd
	0,  // 34, 100010:  0, ignore
	0,  // 35, 100011:  0, ignore
	5,  // 36, 100100:  5, controlChangeCmd
	0,  // 37, 100101:  0, ignore
	0,  // 38, 100110:  0, ignore
	0,  // 39, 100111:  0, ignore
	0,  // 40, 101000:  0, ignore
	0,  // 41, 101001:  0, ignore
	0,  // 42, 101010:  0, ignore
	0,  // 43, 101011:  0, ignore
	0,  // 44, 101100:  0, ignore
	0,  // 45, 101101:  0, ignore
	0,  // 46, 101110:  0, ignore
	0,  // 47, 101111:  0, ignore
	16, // 48, 110000: 16, sizeCmd
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

