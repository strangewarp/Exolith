
// Clamp input v between the low and high values, without any wrap-around, and then return it
byte clamp(byte low, byte high, byte v) {
	return min(high, max(low, v));
}
char clamp(char low, char high, char v) {
	return min(high, max(low, v));
}
int clamp(int low, int high, int v) {
	return min(high, max(low, v));
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