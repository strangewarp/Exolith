

// Apply a negative or positive "change" value to a given byte,
// clamping it between the given low and high values.
byte applyChange(byte n, char change, byte low, byte high) {
	int n2 = int(n) + change;
	return (n2 < low) ? low : ((n2 > high) ? high : byte(n2));
}

// Apply an offset to a given position in the current RECORDSEQ, applying either a negative or positive chirality to the OFFSET value
word applyOffset(char chiral, word p) {
	word len = (STATS[RECORDSEQ] & 63) * 32; // Get the RECORDSEQ's length, in 32nd-notes
	// Apply the offset to the given position (with positive or negative chirality), and wrap its new value (even if negative)
	return word((int(p) + (OFFSET * chiral)) + len) % len;
}

// Apply QUANTIZE and QRESET to a given point in the current RECORDSEQ
word applyQuantize(word p) {

	word len = (STATS[RECORDSEQ] & 63) * 32; // Get the RECORDSEQ's length, in 32nd-notes

	byte down = (POS[RECORDSEQ] % (QRESET ? QRESET : 65535)) % QUANTIZE; // Get the distance from the previous QUANTIZE-point
	byte up = min(QRESET, QUANTIZE) - down; // Get distance to next QUANTIZE-point, compensating for QRESET

	return (p + ((down <= up) ? (-down) : up)) % len; // Return the QUANTIZE-and-QRESET-adjusted insertion-point

}

// Return a button-number that corresponds to the most-significant pressed control-button's bitwise value
byte ctrlToButtonIndex(byte ctrl) {
	return (ctrl & 32) ? 6 : (
		(ctrl & 16) ? 5 : (
			(ctrl & 8) ? 4 : (
				(ctrl & 4) ? 3 : (
					(ctrl & 2) ? 2 : (ctrl & 1)
				)
			)
		)
	);
}

// Check whether the current point in RECORDSEQ is an insertion-point, when adjusted for QUANTIZE, QRESET, and OFFSET
byte isInsertionPoint() {
	return !((applyOffset(-1, POS[RECORDSEQ]) % (QRESET ? QRESET : 65535)) % QUANTIZE);
}

// Get a CHANGE-value from a given keystroke, and apply that change to the given global-var
void simpleChange(byte col, byte row, byte &v, byte low, byte high, byte update) {
	v = applyChange(v, toChange(col, row), low, high); // Apply the keystroke's CHANGE-value to the given global var
	TO_UPDATE |= update; // Flag the given rows for updating
}

// Convert a column and row into a CHANGE value,
// which is used when global vars are being changed by the user.
char toChange(byte col, byte row) {
	return char(32 >> row) * (char(col & 2) - 1);
}

// Set GLOBALRAND to contain a semi-random number, by using a xorshift algorithm on the smallest digits of ABSOLUTETIME
void xorShift() {
	GLOBALRAND = ABSOLUTETIME % 65536;
	GLOBALRAND ^= GLOBALRAND << 2;
	GLOBALRAND ^= GLOBALRAND >> 7;
	GLOBALRAND ^= GLOBALRAND << 7;
}
