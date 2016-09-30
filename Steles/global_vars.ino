
// Timing vars
unsigned long ABSOLUTETIME = 0;
unsigned long ELAPSED = 0;
unsigned long TICKSIZE = 100000; // Size of the current tick, in microseconds; tick = 60000000 / (bpm * 96)

// Sequencing vars
boolean PLAYING = false; // Controls whether the sequences are iterating through their contents
unsigned int CURTICK = 1; // Current global sequencing tick
boolean DUMMYTICK = false; // Tracks whether to expect a dummy MIDI CLOCK tick before starting to iterate through sequences

// UI vars
byte LEFTCTRL = 0; // Tracks left-column control-buttons in bits 1 thru 5
byte BOTCTRL = 0; // Tracks bottom-row control-buttons in bits 1 thru 5
boolean RECORDING = false; // Tracks whether RECORD MODE is active

// Cued-command flags, one per sequence
// Format:
// bit 1 - CUE [8]
// bit 2 - CUE [1]
// bit 3 - SEQ COLUMN [4]
// bit 4 - SEQ COLUMN [2]
// bit 5 - SEQ COLUMN [1]
// bit 6 - reserved
// bit 7 - reserved
// bit 8 - OFF
byte CMD[51];

// Sequence-status flags, one per sequence
// Format:
// byte 1 - misc data
//    bit 1-2: play speed (2 1) [0 = off; 1 = 1; 2 = 1/2; 3 = 1/4]
//    bit 3-4: play-speed tick delay counter (2 1)
//    bit 5-7: wander chance (4 2 1) [0 = none; 1 = 14.5%; ... 7 = 100%]
//    bit 8: location by tick (512)
// byte 2 - location by tick (256 128 64 32 16 8 4 2 1)
// byte 3 - wander permissions
//    bit 1: up-left allowed
//    bit 2: up allowed
//    bit 3: up-right allowed
//    bit 4: left allowed
//    bit 5: right allowed
//    bit 6: down-left allowed
//    bit 7: down allowed
//    bit 8: down-right allowed
byte SEQ[51][4];

// MIDI-IN vars
byte INBYTES[4] = {0, 0, 0};
byte INCOUNT = 0;
byte INTARGET = 0;
byte SYSBYTES[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte SYSCOUNT = 0;
boolean SYSIGNORE = false;
boolean SYSLATCH = false;

// MIDI SUSTAIN vars
// Format: SUSTAIN[n] = {channel, pitch, remaining duration in ticks}
byte SUSTAIN[9][4] = {
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0},
	{0, 0, 0}, {0, 0, 0}
};

// Initialize SdFat and SdFile objects, for filesystem manipulation on the SD-card
SdFat sd;
SdFile file;

// Initialize the object that controls the MAX7221's LED-grid
LedControl lc = LedControl(2, 3, 4, 1);

// Initialize the object that controls the Keypad buttons.
const byte ROWS PROGMEM = 6;
const byte COLS PROGMEM = 5;
char KEYS[ROWS][COLS] = {
	{'0', '1', '2', '3', '4'},
	{'5', '6', '7', '8', '9'},
	{':', ';', '<', '=', '>'},
	{'?', '@', 'A', 'B', 'C'},
	{'D', 'E', 'F', 'G', 'H'},
	{'I', 'J', 'K', 'L', 'M'}
};
byte rowpins[ROWS] = {5, 6, 7, 8, 18, 19};
byte colpins[COLS] = {9, 14, 15, 16, 17};
Keypad kpd(makeKeymap(KEYS), rowpins, colpins, ROWS, COLS);
