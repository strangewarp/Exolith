

// Apply a negative or positive "change" value to a given byte,
// clamping it between the given low and high values.
byte applyChange(byte n, char change, byte low, byte high) {
	int n2 = int(n) + change;
	return (n2 < low) ? low : ((n2 > high) ? high : byte(n2));
}

// Convert a column and row into a CHANGE value,
// which is used when global vars are being changed by the user.
char toChange(byte col, byte row) {
	return char(32 >> row) * (char(col & 2) - 1);
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

// Apply an offset to a given position in the current RECORDSEQ
word applyOffset(word p) {
	word len = (STATS[RECORDSEQ] & 63) * 32; // Get the RECORDSEQ's length, in 32nd-notes
	return word((((int(p) + OFFSET) % len) + len) % len); // Apply the offset to the given position, and wrap its new value (even if negative)
}
word applyOffset(byte p) { return applyOffset(word(p)); } // Overload function: compensates for when a byte is received instead of a word

// Apply QUANTIZE and QRESET to a given point in the sequence
word applyQuantize(word p) {

	int len = word(STATS[RECORDSEQ] & 63) * 32; // Get the seq's length, in 32nd-notes (this is int, because it may go negative in subsequent lines)

	byte down = distFromQuantize(); // Get the distance from the previous QUANTIZE-point
	byte up = min(QRESET - down, QUANTIZE - down); // Get distance to next QUANTIZE-point, compensating for QRESET

	return (p + ((down <= up) ? (-down) : up)) % len; // Return the QUANTIZE-and-QRESET-adjusted insertion-point

}

// Get the distance from a seq's previous QUANTIZE-or-QRESET point, or 0 if such a point is currently occupied.
byte distFromQuantize() {
	return (POS[RECORDSEQ] % (QRESET ? QRESET : 65535)) % QUANTIZE; // Return the QRESET-adjusted QUANTIZE distance, or just QUANTIZE-adjusted if QRESET is 0
}

// Set a given var to contain a semi-random number, by using a xorshift algorithm on the smallest digits of ABSOLUTETIME
void xorShift() {
	GLOBALRAND = ABSOLUTETIME % 65536;
	GLOBALRAND ^= GLOBALRAND << 2;
	GLOBALRAND ^= GLOBALRAND >> 7;
	GLOBALRAND ^= GLOBALRAND << 7;
}
