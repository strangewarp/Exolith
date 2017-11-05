
// Clamp input v between the low and high values, without any wrap-around, and then return it
byte clamp(byte low, byte high, byte v) {
	return min(high, max(low, v));
}
char clamp(byte low, byte high, char v) {
	return min(high, max(low, v));
}
int clamp(byte low, byte high, int v) {
	return min(high, max(low, v));
}
