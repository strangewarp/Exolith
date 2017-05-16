
// Update the GUI based on update-flags that have been set by the current tick's events
void updateGUI() {

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
				if (ROWHELD == 0) { // If BASE-NOTE pressed...
					lc.setRow(0, 0, BASENOTE); // Display BASENOTE value
				} else if (ROWHELD == 1) { // Else, if OCTAVE pressed...
					lc.setRow(0, 0, OCTAVE); // Display OCTAVE value
				} else if (ROWHELD <= 3) { // Else, if VELOCITY pressed...
					lc.setRow(0, 0, VELO); // Display VELOCITY value
				} else if (ROWHELD <= 5) { // Else, if HUMANIZE pressed...
					lc.setRow(0, 0, HUMANIZE); // Display HUMANIZE value
				}
			} else if (CTRL == 4) { // If the third-from-bottom CTRL button is held...
				if (ROWHELD == 0) { // If CHANNEL pressed...
					lc.setRow(0, 0, CHANNEL); // Display CHANNEL value
				} else if (ROWHELD == 1) { // If QUANTIZE pressed...
					lc.setRow(0, 0, QUANTIZE); // Display QUANTIZE value
				} else if (ROWHELD <= 3) { // If DURATION pressed...
					lc.setRow(0, 0, DURATION); // Display DURATION value
				} else if (ROWHELD <= 5) { // If SEQ-SIZE pressed...
					lc.setRow(0, 0, SEQ_SIZE[RECORDSEQ]); // Display current record-seq's SIZE value
				}
			} else if (CTRL == 8) { // If the third-from-top CTRL button is held...
				if (ROWHELD == 0) { // If CLOCK-MODE pressed...
					lc.setRow(0, 0, CLOCKMASTER ? 255 : 0); // Display MIDI CLOCK MASTER/FOLLOWER status
				} else if (ROWHELD <= 3) { // If CHAN-LISTEN pressed...
					lc.setRow(0, 0, LISTENS[ROWHELD - 1]); // Display the row's CHAN-LISTEN value
				} else if (ROWHELD <= 5) { // If BPM pressed...
					lc.setRow(0, 0, BPM); // Display BPM value
				}
			} else { // If no CTRL buttons are held, or an unknown or irrelevant combination of CTRL buttons are held...
				lc.setRow(0, 0, 128 >> byte(floor(CURTICK / 96))); // Display the global-gate position
			}

		}

		// TO_UPDATE & 2 is handled earlier in the function, because it applies to both RECORD and PLAY modes





		if (TO_UPDATE & 252) { // If any of the bottom six rows are slated for a GUI update...

			byte glyph[7];

			if (CTRL <= 1) { // If NOTE-RECORDING is being toggled, or no CTRL button is held...
				if (RECORDNOTES) { // If RECORDING, then...
					memcpy_P(glyph, GLYPH_RECORDING, 6); // Slate the RECORDING-glyph for display
				} else { // Else, if not RECORDING...

				}
			} else if (CTRL == 2) { // If second-from-bottom CTRL button is held...
				if (ROWHELD == 0) { // If BASE-NOTE row is held...
					memcpy_P(glyph, GLYPH_BASENOTE, 6);
				} else if (ROWHELD == 1) { // If OCTAVE row is held...
					memcpy_P(glyph, GLYPH_OCTAVE, 6);
				} else if (ROWHELD <= 3) { // If VELOCITY rows are held...
					memcpy_P(glyph, GLYPH_VELO, 6);
				} else if (ROWHELD <= 5) { // If HUMANIZE rows are held...
					memcpy_P(glyph, GLYPH_HUMANIZE, 6);
				} else { // Else, if no row-button is being held on this page...
					glyph[0] = 96 | BASENOTE;
					glyph[1] = 144 | OCTAVE;
					glyph[2] = 240 | ((VELOCITY & 240) >> 4);
					glyph[3] = 144 | (VELOCITY & 15);
					glyph[4] = 144 | ((HUMANIZE & 240) >> 4);
					glyph[5] = 144 | (HUMANIZE & 15);
				}
			} else if (CTRL == 4) {

			} else if (CTRL == 8) {

			} else if (CTRL == 16) {

			} else if (CTRL == 32) {

			}

			for (byte i = 0; i < 6; i++) {
				lc.setRow(0, i + 2, glyph[i]);
			}


		}





		if (TO_UPDATE & 4) { // If the third row is slated for a GUI update...


			if (CTRL == 2) { // 

			} else if (CTRL == 4) {

			} else if (CTRL == 8) {

			} else if (CTRL == 16) {

			} else if (CTRL == 32) {

			}



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
