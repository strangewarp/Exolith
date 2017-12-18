
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
