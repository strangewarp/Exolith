

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


// Digital-pin keypad library
#include <Key.h>
#include <Keypad.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>



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


void resetSeqs() {
	for (byte i = 0; i < 50; i++) {
		CMD[i] = 0;
	}
}

void resetSeqFlags() {
	for (byte i = 0; i < 50; i++) {
		SEQ[i][0] = 0;
		SEQ[i][1] = 0;
		SEQ[i][2] = 0;
	}
}


void iterateSeqs() {

	// TODO write this part

}


void setup() {

	resetSeqs();
	resetSeqFlags();

	kpd.setDebounceTime(1);

	lc.shutdown(0, false);
	lc.setIntensity(0, 15);

	Serial.begin(31250);
	//Serial.begin(38400);//testing purposes
	
}

void loop() {





	// Get keypad keys and launch commands accordingly
	if (kpd.getKeys()) {
		for (int i = 0; i < LIST_MAX; i++) {
			if (kpd.key[i].stateChanged) {
				if (kpd.key[i].kstate == PRESSED) {
					int kc = int(kpd.key[i].kchar) - 48;
					lc.setRow(0, 2, 128 >> (kc % 8));
					lc.setRow(0, 3, 128 >> (kc >> 3));

					//testing code TODO remove
					Serial.write(144);
					Serial.write(byte((floor(kc / 8)) * 8) + (kc >> 3));
					Serial.write(127);
					Serial.write(128);
					Serial.write(byte((floor(kc / 8) * 8)) + (kc >> 3));
					Serial.write(127);

				} else if (kpd.key[i].kstate == RELEASED) {
					lc.setRow(0, 2, 0);
					lc.setRow(0, 3, 0);
				}
			}
		}
	}

	// While new MIDI bytes are available to read from the MIDI-IN port...
	while (Serial.available() > 0) {

		// Get the frontmost incoming byte
		byte b = Serial.read();

		// TODO: Stress-test SYSEX processing with a Pd send-receive script & a USB-MIDI cable

		if (SYSIGNORE) { // If this is an ignoreable SYSEX command, send its bytes onward
			Serial.write(b);
			if (b == 247) { // If this was an END SYSEX byte, clear all SYSEX-tracking flags and counters
				SYSCOUNT = 0;
				SYSLATCH = false;
				SYSIGNORE = false;
			}
		} else if (SYSLATCH) { // Else, if we don't know whether this SYSEX command can be ignored, accumulate it
			SYSCOUNT++;
			if (b == 247) { // If this SYSEX command fell within the size limit, then parse it to see whether it's a Basilisk command
				parseSysex();
				SYSLATCH = false;
				SYSCOUNT = 0;
			} else if (SYSCOUNT > 11) { // Else, if this command went over the size limit, send it onward, and toggle SYSIGNORE for its remaining bytes
				SYSIGNORE = true;
				for (byte i = 0; i < 11; i++) {
					Serial.write(SYSBYTES[i]);
					SYSBYTES[i] = 0;
				}
			}
		} else { // Else, accumulate arbitrary commands...

			byte cmd = b - (b % 16); // Get the command-type of any given non-SYSEX command

			if (b == 252) { // STOP command
				Serial.write(b);
				PLAYING = false;
				haltAllSustains();
			} else if (b == 251) { // CONTINUE command
				Serial.write(b);
				PLAYING = true;
			} else if (b == 250) { // START command
				Serial.write(b);
				resetSeqs();
				PLAYING = true;

				//testing code TODO remove
				TESTTICK = 95;
				
			} else if (b == 248) { // TIMING CLOCK command
				Serial.write(b);
				iterateSeqs();

				//testing code TODO remove
				TESTTICK = (TESTTICK + 1) % 96;
				lc.setRow(0, 0, TESTTICK);
				
			} else if (b == 247) { // END SYSEX MESSAGE command
				// If you're dealing with an END-SYSEX command while SYSIGNORE and SYSLATCH are inactive,
				// then that implies you've received either an incomplete or corrupt SYSEX message,
				// and so nothing should be done here aside from sending it onward.
				Serial.write(b);
			} else if (
				(b == 244) // MISC SYSEX command
				|| (b == 240) // START SYSEX MESSAGE command
			) { // Anticipate an incoming arbitrarily-sized SYSEX message
				SYSLATCH = true;
				SYSBYTES[0] = b;
				SYSCOUNT = 1;
			} else if (
				(b == 242) // SONG POSITION POINTER command
				|| (cmd == 224) // PITCH BEND command
				|| (cmd == 176) // CONTROL CHANGE command
				|| (cmd == 160) // POLY-KEY PRESSURE command
				|| (cmd == 144) // NOTE-ON command
				|| (cmd == 128) // NOTE-OFF command
			) { // Anticipate an incoming 3-byte message
				INBYTES[0] = b;
				INCOUNT = 1;
				INTARGET = 3;
			} else if (
				(b == 243) // SONG SELECT command
				|| (b == 241) // MIDI TIME CODE QUARTER FRAME command
				|| (cmd == 208) // CHANNEL PRESSURE (AFTERTOUCH) command
				|| (cmd == 192) // PROGRAM CHANGE command
			) { // Anticipate an incoming 2-byte message
				INBYTES[0] = b;
				INCOUNT = 1;
				INTARGET = 2;
			} else { // Else, if this is a body-byte from a given non-SYSEX MIDI command...
				if (INBYTES[0] == 0) { // Ignore all body-data without proper command-bytes
					continue;
				} else { // Parse body-data if a proper command-byte was received
					INBYTES[INCOUNT] = b;
					INCOUNT++;
					if (INCOUNT == INTARGET) { // If all the command's bytes have been received, parse the command
						parseMidi();
					}
				}
			}

		}

	}

}

