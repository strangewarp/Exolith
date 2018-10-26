

// Parse a new keypress in the arpeggiation-system
void arpPress(byte col, byte row) {
	if (
      REPEAT // If REPEAT is currently enabled...
      && (DURATION < 129) // And DURATION isn't in manual-mode...
   ) {
   	byte note = modKeyPitch(col, row); // Get a note-value for the new ARP slot
   	if (ARPCOUNT == ARPEG_KEYS_1) { // If the last ARP-slot is already filled...
   		memmove(ARP, ARP + 1, ARPEG_KEYS_0); // Shift each ARP-slot one space toward index 0, overwriting the oldest one
   		ARP[ARPEG_KEYS_0] = note; // Fill the last ARP index with the flagged note-value
   		ARPNEXT -= !!ARPNEXT; // If ARPNEXT isn't in ARP-slot 0, then reduce it by 1, to compensate for the movement of ARP's contents
   	} else { // Else, if the last ARP-slot is not already filled...
   		ARP[ARPCOUNT] = note; // Fill the highest ARP-slot with the new note
   		ARPCOUNT++; // Increase the count of currently-filled ARP-slots by 1
   	}
   }
}

// Parse a new key-release in the arpeggiation-system
void arpRelease() {
	if (
      REPEAT // If REPEAT is currently enabled...
      && (DURATION < 129) // And DURATION isn't in manual-mode...
      && ARPCOUNT // And any notes are currently being arpeggiated...
   ) {
		byte amin = ARPCOUNT - 1; // Get a 0-indexed version of the ARP-notes-counting var
		for (byte i = 0; i <= ARPEG_KEYS_0; i++) { // For every currently-active ARP note...
			if (ARP[i] == modKeyPitch(col, row)) { // If that note's pitch-value matches this key-release's pitch-value...
				if (i < amin) { // If this isn't the last note in the arpeggiation-array...
					memmove(ARP + i, ARP + i + 1, ARPEG_KEYS_0 - i); // Move all higher-up notes downward by one index
				}
				ARPNEXT -= ARPNEXT > amin; // If ARPNEXT is above the array-shift-point, then reduce it by 1, to keep arpeggiating without stutters
				break; // Stop looking for matching ARP notes, since one has already been found
			}
		}
		ARPCOUNT--; // Reduce the ARP-note-tracking var by 1, since we have removed one of the arpeggiation notes
	}
}

// Clear the arpeggiation-system's contents
void arpClear() {
	if (REPEAT) { // If REPEAT is currently enabled...
		ARPCOUNT = 0; // Clear the counter for the number of values in the ARP array
		ARPNEXT = 0; // Clear the tracking-var for the currently-active ARP index
	}
}
