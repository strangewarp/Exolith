

// Advance the arpeggiator's position, based on the current ARPMODE
void arpAdvance() {

	ARPPOS &= 127; // Mask the "ARPPOS is active" bit out of ARPPOS

   unsigned long nbuts = BUTTONS >> 6; // Get the note-button bits only, with the control-button bits shifted out of the value

	if (ARPMODE < 2) { // If the current ARP MODE is either "up" or "down"...

		char i = ARPPOS; // Set the iterator's start-position to the current ARP POSITION button
		byte times = 1; // Keep a tally of how many buttons will have been searched (this intentionally skips the current button, whose value is in ARPPOS)
		while (times < 23) { // While the other 23 buttons have not yet been searched...
			if (nbuts & (1 << i)) { // If this button is currently pressed...
				ARPPOS = i; // Set the new ARP POSITION to this button's position
				break; // Stop looking for the next held button, since one has been found
			}
			i = (i + (ARPMODE ? -1 : 1) + 24) % 24; // Advance the button-search var by either -1 or 1, depending on whether ARP MODE is "up" or "down", then wrap it
			times++; // Increase the "number of buttons searched" counter
		}

	} else { // Else, if the current ARP MODE is "repeating-random"...

		byte found[25]; // Temp array that will hold all held-button locations
		byte f = 0; // Counts the number of button-locations that will be found and placed into the temp array
      byte rpos = 0; // Will hold the random-slice-position, from counting held butons only, that is used as the basis for grabbing a random value from ARPRAND

		for (byte i = 0; i < 24; i++) { // For every button-location...
			if (nbuts & (1 << i)) { // If this button is currently pressed...
            if (ARPPOS == i) { // If this is the current ARP POSITION button...
               rpos = f; // Set the random-slice-position to the current button-count (before increasing the count)
            }
				found[f] = i; // Put the button-location into the temp-array
				f++; // Increase the "number of buttons found" counter
			}
		}

		char dist = (ARPRAND >> (rpos % 4)) & 15; // Get a slice of the ARP RANDOM value that corresponds to the button's position among held-buttons
      dist -= 8 - (dist >= 8); // Change the random-value into an offset of -8 to +8, without any possibility of landing on 0
      dist = ((dist % f) + 128) % f; // Wrap the dist-value within the bounds of 0-to-f, even if dist is negative
      ARPPOS = found[byte(dist)]; // Set the new ARPPOS value to the next button within the repeating-random system

	}

	ARPPOS |= 128; // Add the "ARPPOS is active" bit back to ARPPOS

}

// Parse a new keypress in the arpeggiation-system
void arpPress() {

	if (
      REPEAT // If REPEAT is currently enabled...
      && (DURATION < 129) // And DURATION isn't in manual-mode...
   ) {

      if (!ARPPOS) { // If the var that tracks which note was most recently arpeggiated hasn't yet been filled...

         unsigned long nbuts = BUTTONS >> 6; // Get the note-button bits only, with the control-button bits shifted out of the value

         if (ARPMODE < 2) { // If the current ARP MODE is either 0 (up) or 1 (down)...

            ARPPOS = 0; // Reset ARPPOS to 0, to prepare to fill it with the highest currently-held note-value
            byte most = 0; // Will end up holding the highest held-note for "down" mode, or the lowest held-note for "up" mode
            for (byte i = 0; i < 24; i++) { // For every note-button...
               if (nbuts & (1 << i)) { // If this note-button is held...
                  byte realnote = pgm_read_byte_near(GRIDS + (GRIDCONFIG * 24) + i); // Get the note that corresponds to the button according to the current GRIDCONFIG
                  if (
                     (ARPMODE && (most < realnote)) // If this is "down" mode, and a higher realnote has been found,
                     || ((!ARPMODE) && (most > realnote)) // Or if this is "up" mode, and a lower realnote has been found...
                  ) {
                     most = realnote; // Set the note-comparison value to the new realnote
                     ARPPOS = i; // Set ARPPOS to the note's button-value
                  }
               }
            }
            ARPPOS |= 128; // Flag ARPPOS as "filled"

         } else { // Else, if the current ARP MODE is "repeating random"...

            byte b[25]; // Temp array to store the notes from found-buttons
            byte held = 0; // Var to track the number of held-buttons
            for (byte i = 0; i < 24; i++) { // For every note-button...
               if (nbuts & (1 << i)) { // If this note-button is held...
                  b[held] = i; // Temporarily store the button-value
                  held++; // Increment the index-number in which to store the next found-button
               }
            }
            ARPPOS = 128 | b[GLOBALRAND % held]; // Set ARPPOS to a random filled button-value
            xorShift(ARPRAND); // Re-randomize the arp's repeating-random value

         }

      }

   }

}

// Parse a new key-release in the arpeggiation-system (and check whether it was the last held arp-note)
void arpRelease() {
	if (
      (DURATION < 129) // If DURATION isn't in manual-mode...
      && ARPPOS // And notes are currently being arpeggiated...
      && (!(BUTTONS >> 6)) // And the last note-button has been released...
   ) {
      arpClear(); // Clear the arpeggiation-system's contents
	}
}

// Clear the arpeggiation-system's contents
void arpClear() {
	if (REPEAT) { // If REPEAT is currently enabled...
      ARPPOS = 0; // Clear the var that tracks the current arpeggiation-position
      ARPRAND = 0; // Clear the arpeggiator's repeating-random value
	}
}
