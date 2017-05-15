
// Update the GUI based on update-flags that have been set by the current tick's events
void updateGUI() {

	// TODO write this

	// Note: LED-row 2 applies to both RECORD and PLAY modes
	if (TO_UPDATE & 2) { // If the second row is slated for a GUI update...
		byte r2 |= 1 << (7 - PAGE); // Get the page-location, translated to its LED-row value
		r2 |= ((CURTICK % 24) < 12) ? 16 : 0; // Metronome on or off, depending on time since last quarter-note
		for (byte i = 0; i < 8; i++) { // For every sustain-slot...
			if (SUSTAIN[i][1] == 255) { break; } // If the slot is empty, stop checking sustain-slots
			r2++; // Else, increase the sustain-value in this LED-row
		}
		lc.setRow(0, 1, r2); // Set the LED-row
	}

	if (RECORDMODE) { // If RECORD MODE is active...

		if (TO_UPDATE & 1) { // If the top row is slated for a GUI update...

			if (CTRL == 2) { // If the second-from-bottom CTRL button is held...
				if (BUTHELD == 0) { // If a BASE-NOTE keystroke is present...
					lc.setRow(0, 0, BASENOTE); // Display the BASENOTE on the top row
				} else if (BUTHELD == 1) { // Else, if an OCTAVE keystroke is present...
					lc.setRow(0, 0, OCTAVE); // Display the OCTAVE on the top row
				} else if (BUTHELD <= 3) { // Else, if a VELOCITY keystroke is present...
					lc.setRow(0, 0, VELO); // Display the VELOCITY on the top row
				} else if (BUTHELD <= 5) { // Else, if a HUMANIZE keystroke is present...
					lc.setRow(0, 0, HUMANIZE); // Display the HUMANIZE value on the top row
				}
			} else if (CTRL == 4) { // If the third-from-bottom CTRL button is held...
				if (BUTHELD == 0) {

				} else if (BUTHELD == 1) {

				} else if (BUTHELD <= 3) {

				} else if (BUTHELD <= 5) {

				}
				
			} else if (CTRL == 8) { // If the third-from-top CTRL button is held...
				
			} else if (CTRL == 16) { // If the second-from-top CTRL button is held...
				
			} else if (CTRL == 32) { // If the top CTRL button is held...
				
			} else { // If no CTRL buttons are hald, or an unknown or irrelevant combination of CTRL buttons are held...
				lc.setRow(0, 0, 128 >> byte(floor(CURTICK / 96))); // Display the global-gate position
			}

		}

		// TO_UPDATE & 2 is handled earlier in the function, because it applies to both RECORD and PLAY modes

		if (TO_UPDATE & 4) { // If the third row is slated for a GUI update...



		}





	} else {



	}

	if (TO_UPDATE & 2) {
		byte r2 = (CURTICK % 24) ? 0 : 16;
		r2 |= (2 - PAGE) << 5;
		for (byte i = 0; i < 8; i++) {
			if (SUSTAIN[i][1] == 255) { break; }
			r2++;
		}
	}






}
