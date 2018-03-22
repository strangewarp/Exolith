

// Store pointers to RECORD-MODE functions in PROGMEM
const CmdFunc COMMANDS[] PROGMEM = {
	genericCmd,       //  0: Unused duplicate pointer to genericCmd
	armCmd,           //  1: TOGGLE RECORDNOTES
	baseCmd,          //  2: BASENOTE
	chanCmd,          //  3: CHANNEL
	clearCmd,         //  4: CLEAR NOTES
	clockCmd,         //  5: CLOCK-MASTER
	controlChangeCmd, //  6: CONTROL-CHANGE
	copyCmd,          //  7: COPY
	durationCmd,      //  8: DURATION
	genericCmd,       //  9: All possible note-entry commands
	humanizeCmd,      // 10: HUMANIZE
	octaveCmd,        // 11: OCTAVE
	pasteCmd,         // 12: PASTE
	posCmd,           // 13: SHIFT CURRENT POSITION
	programChangeCmd, // 14: PROGRAM-CHANGE
	quantizeCmd,      // 15: QUANTIZE
	repeatCmd,        // 16: TOGGLE REPEAT
	sizeCmd,          // 17: SEQ-SIZE
	switchCmd,        // 18: SWITCH RECORDING-SEQUENCE
	tempoCmd,         // 19: BPM
	veloCmd           // 20: VELOCITY
};

// Matches control-row binary button-values to the decimal values that stand for their corresponding functions in COMMANDS
// Note: These binary values are flipped versions of what is displayed in button-key.txt
const byte KEYTAB[] PROGMEM = {
	9,  //  0, 000000:  9, genericCmd (REGULAR NOTE)
	16, //  1, 000001: 16, repeatCmd
	3,  //  2, 000010:  3, chanCmd
	13, //  3, 000011: 13, posCmd
	20, //  4, 000100: 20, veloCmd
	0,  //  5, 000101:  0, ignore
	10, //  6, 000110: 10, humanizeCmd
	0,  //  7, 000111:  0, ignore
	11, //  8, 001000: 11, octaveCmd
	5,  //  9, 001001:  5, clockCmd
	12, // 10, 001010: 12, pasteCmd
	0,  // 11, 001011:  0, ignore
	15, // 12, 001100: 15, quantizeCmd
	0,  // 13, 001101:  0, ignore
	0,  // 14, 001110:  0, ignore
	4,  // 15, 001111:  4, clearCmd
	2,  // 16, 010000:  2, baseCmd
	0,  // 17, 010001:  0, ignore
	19, // 18, 010010: 19, tempoCmd
	0,  // 19, 010011:  0, ignore
	7,  // 20, 010100:  7, copyCmd
	0,  // 21, 010101:  0, ignore
	0,  // 22, 010110:  0, ignore
	0,  // 23, 010111:  0, ignore
	8,  // 24, 011000:  8, durationCmd
	0,  // 25, 011001:  0, ignore
	0,  // 26, 011010:  0, ignore
	0,  // 27, 011011:  0, ignore
	0,  // 28, 011100:  0, ignore
	0,  // 29, 011101:  0, ignore
	0,  // 30, 011110:  0, ignore
	0,  // 31, 011111:  0, ignore
	1,  // 32, 100000:  1, armCmd
	18, // 33, 100001: 18, switchCmd
	14, // 34, 100010: 14, programChangeCmd
	0,  // 35, 100011:  0, ignore
	6,  // 36, 100100:  6, controlChangeCmd
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
	17, // 48, 110000: 17, sizeCmd
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

