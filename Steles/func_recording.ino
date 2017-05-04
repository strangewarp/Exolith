
// Record a given MIDI command into the tempdata-file of the seq in the topmost slice-row
void recordToSeq(boolean usercmd, byte cmd, byte chan, byte b1, byte b2) {

	if (usercmd) { // If this is a user-command (as opposed to a command from an external MIDI device)...

		// Create a virtual tick that adheres to the time-quantize value;
		// this is the tick-position where the command will be placed in the sequence
		byte tick = SEQ_POS[RECORDSEQ] & B0111111111111111;
		byte qmod = max(1, (QUANTIZE >> 1) * 3);
		byte down = tick % qmod;
		byte up = qmod - down;
		tick += (down <= up) ? -down : up;

		// If this is a NOTE-ON command, then apply note/octave/velocity/humanize modifiers
		if (cmd == 144) {



		}



	}

	byte buf[5];
	byte tochange = 255;
	unsigned long tickstart = 73 + (98304 * RECORDSEQ) + (tick * 16);

	file.open(string(SONG) + ".tmp", O_RDWR);

	file.seekSet(tickstart);

	for (byte i = 0; i < 4; i++) {
		file.read(buf, 4);
		if (buf[1] < 255) {
			tochange = i;
			break;
		}
		file.seekCur(4);
	}

	if (tochange == 255) {
		file.seekSet(tickstart + 16);
		for (byte i = 0; i < 3; i++) {
			file.seekCur(-8);
			file.read(buf, 4);
			file.seekCur(4);
			file.write(buf, 4);
		}
		tochange = 0;
	}

	file.seekSet(tickstart + (tochange * 4));

	buf = {DURATION, cmd | chan, b1, b2};

	file.write(buf, 4);

	file.close();


}
