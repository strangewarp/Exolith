

// Scan all buttons in a given column, checking for changes in state and then reacting to any that are found
void scanColumn(byte col, byte rb[]) {

	for (byte row = 0; row < 6; row++) { // For each keypad row...

		byte bpos = (col * 6) + row; // Get the button's bit-position for the BUTTONS data
		byte bstate = !(((row <= 2) ? PIND : PINC) & rb[row]); // Get the pin's input-state (low = ON; high = OFF)

		if (bstate == ((BUTTONS >> bpos) & 1)) { continue; } // If the button's state hasn't changed, skip to the next button

		byte oldcmds = byte(BUTTONS & B00111111); // Get the old COMMAND-row values
		BUTTONS ^= 1UL << bpos; // Flip the button's activity-bit into its new state

		if (bstate) { // If the button's new state is ON...
			assignKey(col, row, oldcmds); // Assign any key-commands that might be based on this keystroke
		} else { // Else, the button's new state is OFF, so...
			unassignKey(col, row, oldcmds); // Unassign any key-commands that might be based on this release-keystroke
		}

	}

}

// Scan all keypad rows and columns, checking for changes in state
void scanKeypad() {

	if (KEYELAPSED < SCANRATE) { return; } // If the next keypad-check time hasn't been reached, exit the function

	KEYELAPSED = 0.0; // Reset the keypad-check timer

	byte cb[6] = { // Temp var: port-bit-locations for column-pins:
		B00000100, // col 0: pin 25 = bit 2
		B00000010, // col 1: pin 24 = bit 1
		B00000001, // col 2: pin 23 = bit 0
		B00000010, // col 3: pin 15 = bit 1
		B00000001  // col 4: pin 14 = bit 0
	};

	byte rb[7] = { // Temp var: raw port-bit-values for row-pins:
		16, // row 0: pin 6  = bit 4 (16)
		8,  // row 1: pin 5  = bit 3 (8)
		4,  // row 2: pin 4  = bit 2 (4)
		32, // row 3: pin 28 = bit 5 (32)
		16, // row 4: pin 27 = bit 4 (16)
		8   // row 3: pin 26 = bit 3 (8)
	};

	for (byte col = 0; col < 5; col++) { // For each keypad column...
		if (col >= 3) { // If this col's register is in PORTB...
			PORTB &= ~cb[col]; // Pulse the column's pin low, in port-bank B
			scanColumn(col, rb); // Scan the column for keystrokes
			PORTB |= cb[col]; // Raise the column's pin high, in port-bank B
		} else { // Else, if this col's register is in PORTC...
			PORTC &= ~cb[col]; // Pulse the column's pin low, in port-bank C
			scanColumn(col, rb); // Scan the column for keystrokes
			PORTC |= cb[col]; // Raise the column's pin high, in port-bank C
		}
	}

}
