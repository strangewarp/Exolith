
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

// Sequencing vars
word SEQ_POS[51]; // Holds the current tick-position for each sequence
byte SEQ_SIZE[26]; // Holds 50 4-bit values describing each sequence's size
byte SEQ_PLAYING[8]; // Holds 50 bits that describe whether a given sequence is currently playing
byte SLICE_ROW[6]; // Holds the indexes of the sequences in each slice-row

// Cued-command flags, one per sequence
// Format:
// bit 1 - CUE [1]
// bit 2 - CUE [8]
// bit 3 - SEQ COLUMN [1]
// bit 4 - SEQ COLUMN [2]
// bit 5 - SEQ COLUMN [4]
// bit 6 - reserved
// bit 7 - reserved
// bit 8 - ON/OFF (1/0)
byte SEQ_CMD[51];

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
