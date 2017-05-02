
// Digital-pin keypad library
#include <Key.h>
#include <Keypad.h>

// MAX7219/MAX7221 LED-matrix library
#include <LedControl.h>

// Serial Peripheral Interface library
#include <SPI.h>

// SD-card data-storage library (requires SPI.h)
#include <SdFat.h>

// Initialize the object that controls the Keypad buttons
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

// Initialize SdFat and SdFile objects, for filesystem manipulation on the SD-card
SdFat sd;
SdFile file;

// Initialize the object that controls the MAX7219's LED-grid
LedControl lc = LedControl(2, 3, 4, 1);
