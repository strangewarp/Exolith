

// Advance the arpeggiator's position, based on the current ARPMODE
void arpAdvance() {

	ARPPOS &= 127; // Mask the "ARPPOS is active" bit out of ARPPOS

   unsigned long nbuts = BUTTONS >> 6; // Get the note-button bits only, with the control-button bits shifted out of the value

	if (ARPMODE < 2) { // If the current ARP MODE is either "up" or "down"...

      byte high = ARPPOS; // Highest found-note (holds ARPPOS by defaut, for when no notes are found)
      byte low = ARPPOS; // Lowest found-note (holds ARPPOS by default, for when no notes are found)
      byte dist = 255; // Distance to closest found-note (255 by default, larger than any note-distance could possibly be)
      byte closest = 0; // Closest found-note's note value (0 by default, and will either be filled or remain unused)

      for (byte i = 0; i < 24; i++) { // For every raw-note-button slot...
         if (nbuts & (1UL << i)) { // If this note-button is held...

            byte note = pgm_read_byte_near(GRIDS + (GRIDCONFIG * 24) + i); // Get the button's note, according to the current GRIDCONFIG

            if (note == ARPPOS) { continue; } // If this is the current ARPPOS-note, ignore it

            high = max(high, note); // If this is the highest found-note, save it
            low = min(low, note); // If this is the lowest found-note, save it

            byte newdist = abs(ARPPOS - note); // Get this note's distance from ARPPOS

            if (
               (newdist < dist) // If the new distance is smaller than the old distance...
               && ( // And...
                  (ARPMODE && (note < ARPPOS)) // Either, [ARPMODE is "down", and the note is lower than ARPPOS]...
                  || ((!ARPMODE) && (note > ARPPOS)) // Or, [ARPMODE is "up", and the note is higher than ARPPOS]...
               )
            ) {
               closest = note; // Update "closest" to hold the new closest note
               dist = newdist; // Update "dist" to hold the new closest note's distance
            }

         }
      }

      if (
         (!ARPLATCH) // If this is the first ARP-note to be played in the current keystroke-cluster...
         || (dist == 255) // Or dist is still 255, and no nearby note candidate has been found...
      ) {
         ARPPOS = ARPMODE ? high : low; // Set the new ARPPOS to the highest note in "down"-mode, or the lowest note in "up"-mode
      } else { // Else, if a nearby-note has been found...
         ARPPOS = closest; // Put it into ARPPOS
      }

	} else { // Else, if the current ARP MODE is "random"...

		byte found[25]; // Temp array that will hold all held-button locations
		byte f = 0; // Counts the number of button-locations that will be found and placed into the temp array

		for (byte i = 0; i < 24; i++) { // For every button-location...
			if (nbuts & (1UL << i)) { // If this button is currently pressed...
				found[f] = i; // Put the button-location into the temp-array
				f++; // Increase the "number of buttons found" counter
			}
		}

      // Out of all the currently-pressed buttons, select one at random, and get its note-value via the current GRIDCONFIG
      ARPPOS = pgm_read_byte_near(GRIDS + (GRIDCONFIG * 24) + found[GLOBALRAND % f]);

	}

	ARPPOS |= 128; // Add the "ARPPOS is active" bit back to ARPPOS

   ARPLATCH = 1; // Set a flag that signifies: "notes have played during the current keypress-cluster"

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

            for (byte i = 0; i < 24; i++) { // For every raw button-position...
               if (nbuts & (1UL << i)) { // If this note-button is held...
                  ARPPOS = 128 | pgm_read_byte_near(GRIDS + (GRIDCONFIG * 24) + i); // Get the ARPPOS that corresponds to this button's GRIDCONFIG entry
                  break; // An initial ARPPOS has been found, so don't search for any more
               }
            }

         } else { // Else, if the current ARP MODE is "random"...

            arpAdvance(); // Advance the arpeggiator's position (in a specifically random way, since the "random" ARPMODE is active)

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
      ARPLATCH = 0; // Clear the "notes have played during the current keystroke-cluster" flag
	}
}
