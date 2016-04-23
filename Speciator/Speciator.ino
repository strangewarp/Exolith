
byte chanin[8] = {1, 2, 3, 4, 5, 6, 7, 8}; // Lane in-channel
byte chanout[8] = {1, 2, 3, 4, 5, 6, 7, 8}; // Lane out-channel

byte species[8] = {7, 7, 7, 7, 7, 7, 7, 7}; // Lane k-species (1 to 12)
byte consonance[8] = {100, 100, 100, 100, 100, 100, 100, 100}; // Lane target consonance (0 to 100%) [0 to 100]
byte consovar[8] = {10, 10, 10, 10, 10, 10, 10, 10}; // Lane consonance variation (0 to 100%) [0 to 100]
short humanize[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Lane velocity humanize (-127 to -1, 0 to 255, +1 to +127) [-127 to 383]

byte adepth[8] = {7, 7, 7, 7, 7, 7, 7, 7}; // Lane analysis-depth (0 to 16)
byte aframes[8] = {2, 2, 2, 2, 2, 2, 2, 2}; // Lane analysis-frames (1 to 4)
byte aframesize[8] = {5, 5, 5, 5, 5, 5, 5, 5}; // Lane analysis-frame size (1 to 16)
byte aframeoverlap[8] = {3, 3, 3, 3, 3, 3, 3, 3}; // Lane analysis-frame overlap (0 to 16)
byte anotes[8][8] = { // Lanes' most recent notes (0 to 127)
  {40, 40, 42, 44, 45, 47, 49, 51},
  {40, 40, 42, 44, 45, 47, 49, 51},
  {40, 40, 42, 44, 45, 47, 49, 51},
  {40, 40, 42, 44, 45, 47, 49, 51},
  {40, 40, 42, 44, 45, 47, 49, 51},
  {40, 40, 42, 44, 45, 47, 49, 51},
  {40, 40, 42, 44, 45, 47, 49, 51},
  {40, 40, 42, 44, 45, 47, 49, 51}
};

byte basenote[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // Default scale's base-note (none; C to B) [0 to 12]
byte basescale[8] = {1, 1, 1, 1, 1, 1, 1, 1}; // Default scale type (none; scale pool) [0 to 352]
byte baserot[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Default scale rotation (0 to 11)
byte aclear[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Clear notes every x beats (none; 1 to 16) [0 to 16]

// 0=single tick; 1=32nd note; 2=16th note; 3=16th note dot; 4=8th note; 5=8th note dot;
// 6=quarter note; 7=quarter note dot; 8=half note; 9=half note dot; 10=whole note
byte quantin[8] = {2, 2, 2, 2, 2, 2, 2, 2}; // Input quantization
byte quantout[8] = {7, 7, 7, 7, 7, 7, 7, 7}; // Output quantization
byte durout[8] = {4, 4, 4, 4, 4, 4, 4, 4}; // Output note-duration
byte offsetout[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Output offset-amount

byte botnote =
byte topnote =
byte wraptype =
byte sendmode =


// Note self: store some things in SD-card memory?????


byte active[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Lane activity/muting

word scales[352]; // All possible scales (to be filled by populateScales())


void setup() {

  String programversion = "0.1"; // Firmware version (for checking whether EEPROM should be cleared)

  populateScales();

  // TESTING CODE //
  word testword = rotateScale(2584, 5);
  Serial.println(testword);
  // END TESTING CODE //

  // Set serial communications to MIDI baud rate
  Serial.begin(31250);

}

void loop() {


}
