
// Send MIDI-OFF commands for all currently-sustained notes
void haltAllSustains() {
	for (byte i = 0; i < 8; i++) {
		if (SUSTAIN[i][2] > 0) {
			Serial.write(128 + SUSTAIN[i][0]);
			Serial.write(SUSTAIN[i][1]);
			Serial.write(127);
			SUSTAIN[i][0] = 0;
			SUSTAIN[i][1] = 0;
			SUSTAIN[i][2] = 0;
		}
	}
}


void iterateSeqs() {

	// TODO write this part

}
