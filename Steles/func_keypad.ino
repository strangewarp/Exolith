

// Scan all buttons in a given column, checking for changes in state and then reacting to any that are found
void scanColumn(byte col) {

	for (byte row = 0; row < 6; row++) { // For each keypad row...

		byte bpos = (col * 6) + row; // Get the button's bit-position for the BUTTONS data
		byte bstate = !(PINC & (32 >> row)); // Get the pin's input-state from bank C (low = ON; high = OFF)

		if (bstate == ((BUTTONS >> bpos) & 1)) { continue; } // If the button's state hasn't changed, skip to the next button

		byte oldcmds = byte(BUTTONS & B00111111); // Get the old COMMAND-row values
		BUTTONS ^= 1UL << bpos; // Flip the button's activity-bit into its new state

		if (bstate) { // If the button's new state is ON...
			assignKey(col, row, oldcmds); // Assign any key-commands that might be based on this keystroke
		} else { // Else, the button's new state is OFF, so...
			unassignKey(col, oldcmds); // Unassign any key-commands that might be based on this release-keystroke
		}

	}

}

// Scan all keypad rows and columns, checking for changes in state
void scanKeypad() {

	byte cr[6] = {0, 0, 0, 1, 1}; // Temp var: port-registers for column-pins: 2 = 0; 3 = 0; 4 = 0; 8 = 1; 9 = 1
	byte cb[6] = { // Temp var: port-bit-locations for column-pins:
		B00000100, // col 0: pin 2 = bit 2
		B00001000, // col 1: pin 3 = bit 3
		B00010000, // col 2: pin 4 = bit 4
		B00000001, // col 3: pin 8 = bit 0
		B00000010  // col 4: pin 9 = bit 1
	};

	for (byte col = 0; col < 5; col++) { // For each keypad column...
		if (!cr[col]) { // If this col's register is in PORTD...
			PORTD &= ~cb[col]; // Pulse the column's pin low, in port-bank D
			scanColumn(col); // Scan the column for keystrokes
			PORTD |= cb[col]; // Raise the column's pin high, in port-bank D
		} else if (cr[col] == 1) { // Else, if this col's register is in PORTB...
			PORTB &= ~cb[col]; // Pulse the column's pin low, in port-bank B
			scanColumn(col); // Scan the column for keystrokes
			PORTB |= cb[col]; // Raise the column's pin high, in port-bank B
		}
	}

}

