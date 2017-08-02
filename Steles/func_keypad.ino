
// Scan all buttons in a given column, checking for changes in state and then reacting to any that are found
void scanColumn(byte col) {

	for (byte row = 0; row < 6; row++) { // For each keypad row...

		byte butpos = (row * 5) + col; // Get the button's bit-position for the BUTTONS data
		byte bstate = !(PINC & (32 >> row)); // Get the pin's input-state from bank C (low = ON; high = OFF)

		if (bstate == bitRead(BUTTONS, butpos)) { continue; } // If the button's state hasn't changed, skip to the next button

		bitWrite(BUTTONS, butpos, bstate); // Put the button's new state into the BUTTON data

		if (bstate) { // If the button's new state is ON...
			assignKey(col, row); // Assign any key-commands that might be based on this keystroke
		} else { // Else, the button's new state is OFF, so...
			unassignKey(col); // Assign any key-commands that might be based on this release-keystroke
		}

	}

}

// Scan all keypad rows and columns, checking for changes in state
void scanKeypad() {
	byte cr[6] = {0, 0, 0, 1, 1}; // Temp var: port-registers for column-pins: 2 = 0; 3 = 0; 4 = 0; 8 = 1; 9 = 1
	byte cb[6] = { // Temp var: port-bit-locations for column-pins:
		B00000100, // col 0: pin 2 = 2
		B00001000, // col 1: pin 3 = 3
		B00010000, // col 2: pin 4 = 4
		B00000001, // col 3: pin 8 = 0
		B00000010  // col 4: pin 9 = 1
	};
	for (byte col = 0; col < 5; col++) { // For each keypad column...
		//byte cr = (col + 1) >> 2; // Get port-register corresponding to column's pin (0=2=0, 1=3=0, 2=4=0, 3=8=1, 4=9=1)
		//byte cb = 1 << ((col + 2) % 5);
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
