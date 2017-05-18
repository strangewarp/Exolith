

/*

		Steles is a MIDI sequencer for the "Exolith" hardware.
		THIS CODE IS UNDER DEVELOPMENT AND DOESN'T DO ANYTHING!
		Copyright (C) 2016-onward, Christian D. Madsen (sevenplagues@gmail.com).

		This program is free software; you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation; either version 2 of the License, or
		(at your option) any later version.

		This program is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License along
		with this program; if not, write to the Free Software Foundation, Inc.,
		51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
		
*/


void setup() {

    // Initialize empty arrays
    resetAllSeqs();
    memset(SEQ_SIZE, 0, sizeof(SEQ_SIZE));

    // Initialize the SD-card at full speed
    if (!sd.begin(10, SPI_FULL_SPEED)) {
        sd.initErrorHalt();
    }

    // Set an extremely minimal debounce time, since the device uses clicky buttons
	kpd.setDebounceTime(1);

    // Power up ledControl to full brightness
	lc.shutdown(0, false);
	lc.setIntensity(0, 15);

    // Load the default song, or create its folder and files if they don't exist
    loadSong(SONG);

    // Start serial comms at the MIDI baud rate
	Serial.begin(31250);
	
}


void loop() {

    parseRawMidi();

    parseKeystrokes();

    if (CLOCKMASTER) {
        updateTimer();
    }

    updateMarquee();

    updateGUI();

}

