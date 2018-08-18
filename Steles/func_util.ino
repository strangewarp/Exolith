

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
	)	)	)	);
}

// Get the distance from a seq's previous QUANTIZE-or-QRESET point,
// or 0 if such a point is currently occupied.
byte distFromQuantize() {
	if (QRESET) { // If there is an active QRESET amount...
		// Return the QRESET-adjusted QUANTIZE distance
		return (POS[RECORDSEQ] % QRESET) % QUANTIZE;
	}
	// Otherwise, return the regular QUANTIZE distance
	return POS[RECORDSEQ] % QUANTIZE;
}

// Create a global semirandom number, which will stay the same until the function is called again
void updateGlobalRand() {
	GLOBALRAND = ABSOLUTETIME % 65536;
	GLOBALRAND ^= GLOBALRAND << 2;
	GLOBALRAND ^= GLOBALRAND >> 7;
	GLOBALRAND ^= GLOBALRAND << 7;
}

