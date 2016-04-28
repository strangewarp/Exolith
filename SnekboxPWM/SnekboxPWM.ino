

/*

    SnekboxPWM is a PWM LED controller for the "Snekbox" hardware.
    It runs on the device's secondary ATMEGA8 microcontroller.
    Copyright (C) 2016-onward, Christian D. Madsen (sevenplagues@gmail.com).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    
*/


// Timing vars
unsigned long ABSOLUTETIME = 0;
unsigned long ELAPSED = 0;

// Idle-animation vars
const unsigned long WAITFOR PROGMEM = 3000000;
int WAITPULSE = 0;

// LED-brightness vars for PWM usage
// Format:
//    0 = green (MIDI CHANNEL)
//    1 = red (MIDI BYTE 1 [e.g. note pitch])
//    2 = blue (MIDI BYTE 2 [e.g. note velocity])
byte PWM[3] = {0, 0, 0};

// MIDI-tracking vars
byte COUNT = 0;
boolean CATCH = false;
boolean IGNORE = false;


// Write the contents of the PWM array to the corresponding PWM-pin LEDs
void writePWM() {
  analogWrite(3, PWM[0]);
  analogWrite(5, PWM[1]);
  analogWrite(6, PWM[2]);
}


void setup() {
  Serial.begin(31250); // MIDI rate
  writePWM(); // Clear PWM LEDs on startup
}


void loop() {

  // Get any incoming MIDI commands
  while (Serial.available() > 0) {

    byte b = Serial.read();

    if ((b == 244) || (b == 240)) { // SYSEX command: engage MIDI IGNORE latch
      IGNORE = true;
    } else if (b == 247) { // END SYSEX command: disengage MIDI IGNORE latch, skip remainder of loop
      IGNORE = false;
      continue;
    } else if (b == 248) { // MIDI CLOCK tick: write PWM states, and then apply decay to them
      writePWM();
      for (byte i = 0; i <= 2; i++) {
        PWM[i] = max(0, PWM[i] - (8 << (3 - i)));
      }
    }

    // Catch MIDI-NOTE commands, and set the PWM values to reflect their contents
    if (!IGNORE) { // If the IGNORE-latch isn't set...
      ELAPSED = 0; // Unset the ELAPSED idle-timer
      WAITPULSE = 0; // Reset the idle-pulse counter
      PWM[0] = 0; // Clear PWM values
      PWM[1] = 0;
      PWM[2] = 0;
      if ((b >= 144) && (b <= 159)) { // On a MIDI NOTE-ON...
        COUNT = 0; // Reset the byte-counting value
        CATCH = true; // Toggle the byte-catching flag
        PWM[0] = (b % 16) * 16; // Set PWM LED 1 to reflect the MIDI CHAN value
      } else { // On all non-NOTE-ON bytes...
        if (CATCH) { // If the byte-catching flag has been set by a NOTE-ON...
          COUNT++; // Increment the byte-counter
          PWM[COUNT] = b * 2; // Set the RGB LED according to the MIDI byte value
          if (COUNT == 2) { // If this is the last byte in the MIDI NOTE command...
            CATCH = false; // Unset the byte-catching flag
            writePWM(); // Write the PWM values to the LED pins
          }
        }
      }
    }
  }

  // Get the current time in microseconds;
  // then calculate the time that's elapsed since the last MIDI command
  // (while compensating for overflows on the timer's unsigned long data);
  // then set the absolute-time to the current time.
  unsigned long micr = micros();
  if (micr < ABSOLUTETIME) {
    ELAPSED += micr + (4294967295 - ABSOLUTETIME);
  } else {
    ELAPSED += micr - ABSOLUTETIME;
  }
  ABSOLUTETIME = micr;

  // If no MIDI commands have been received for WAITFOR microseconds,
  // then pulse the PWM pins of the RGB LED in an idling animation.
  if (ELAPSED >= WAITFOR) {
    ELAPSED = WAITFOR; // stop ELAPSED from overflowing during long inactivity periods
    WAITPULSE += 1;
    unsigned int abspulse = abs(WAITPULSE);
    PWM[1] = abspulse >> 8;
    PWM[0] = 255 - PWM[1];
    PWM[2] = (abspulse >> 4) % 256;
    writePWM();
  }

}

