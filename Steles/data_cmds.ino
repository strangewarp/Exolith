


const char PREFS_FILENAME[] PROGMEM = {80, 46, 68, 65, 84, 0}; // Filename of the preferences-file, in PROGMEM to save RAM

// Store pointers to RECORD-MODE functions in PROGMEM
const CmdFunc COMMANDS[] PROGMEM = {
	genericCmd,       //  0: Unused duplicate pointer to genericCmd
	armCmd,           //  1: TOGGLE RECORDNOTES
	arpModeCmd,       //  2: ARPEGGIATOR MODE
	arpRefCmd,        //  3: ARPEGGIATOR REFRESH
	chanCmd,          //  4: CHANNEL
	clearCmd,         //  5: CLEAR NOTES
	durationCmd,      //  6: DURATION
	genericCmd,       //  7: All possible note-entry commands
	gridConfigCmd,    //  8: GRID-CONFIG
	humanizeCmd,      //  9: HUMANIZE
	octaveCmd,        // 10: OCTAVE
	offsetCmd,        // 11: OFFSET
	posCmd,           // 12: SHIFT CURRENT POSITION
	qrstCmd,          // 13: QUANTIZE-RESET
	quantizeCmd,      // 14: QUANTIZE
	repeatCmd,        // 15: TOGGLE REPEAT
	rSweepCmd,        // 16: REPEAT-SWEEP
	sizeCmd,          // 17: SEQ-SIZE
	switchCmd,        // 18: SWITCH RECORDING-SEQUENCE
	tempoCmd,         // 19: BPM
	trackCmd,         // 20: TRACK
	upperBitsCmd,     // 21: UPPER CHAN BITS
	veloCmd           // 22: VELOCITY
};

// Matches control-row binary button-values to the decimal values that stand for their corresponding functions in COMMANDS
// Note: These binary values are flipped versions of what is displayed in button-key.txt
const byte KEYTAB[] PROGMEM = {
	7,  //  0, 000000:  7, genericCmd (REGULAR NOTE)
	15, //  1, 000001: 15, repeatCmd
	20, //  2, 000010: 20, trackCmd
	12, //  3, 000011: 12, posCmd
	22, //  4, 000100: 22, veloCmd
	19, //  5, 000101: 19, tempoCmd
	16, //  6, 000110: 16, rSweepCmd
	0,  //  7, 000111:  0, ignore
	10, //  8, 001000: 10, octaveCmd
	8,  //  9, 001001:  9, gridConfigCmd
	3,  // 10, 001010:  3, arpRefCmd
	0,  // 11, 001011:  0, ignore
	9,  // 12, 001100:  9, humanizeCmd
	0,  // 13, 001101:  0, ignore
	0,  // 14, 001110:  0, ignore
	5,  // 15, 001111:  5, clearCmd
	14, // 16, 010000: 14, quantizeCmd
	0,  // 17, 010001:  0, ignore
	13, // 18, 010010: 13, qrstCmd
	0,  // 19, 010011:  0, ignore
	2,  // 20, 010100:  2, arpModeCmd
	0,  // 21, 010101:  0, ignore
	0,  // 22, 010110:  0, ignore
	0,  // 23, 010111:  0, ignore
	6,  // 24, 011000:  6, durationCmd
	0,  // 25, 011001:  0, ignore
	0,  // 26, 011010:  0, ignore
	0,  // 27, 011011:  0, ignore
	0,  // 28, 011100:  0, ignore
	0,  // 29, 011101:  0, ignore
	0,  // 30, 011110:  0, ignore
	0,  // 31, 011111:  0, ignore
	1,  // 32, 100000:  1, armCmd
	18, // 33, 100001: 18, switchCmd
	11, // 34, 100010: 11, offsetCmd
	0,  // 35, 100011:  0, ignore
	21, // 36, 100100: 21, upperBitsCmd
	0,  // 37, 100101:  0, ignore
	0,  // 38, 100110:  0, ignore
	0,  // 39, 100111:  0, ignore
	4,  // 40, 101000:  3, chanCmd
	0,  // 41, 101001:  0, ignore
	0,  // 42, 101010:  0, ignore
	0,  // 43, 101011:  0, ignore
	0,  // 44, 101100:  0, ignore
	0,  // 45, 101101:  0, ignore
	0,  // 46, 101110:  0, ignore
	0,  // 47, 101111:  0, ignore
	17, // 48, 110000: 17, sizeCmd
	0,  // 49, 110001:  0, ignore
	0,  // 50, 110010:  0, ignore
	0,  // 51, 110011:  0, ignore
	0,  // 52, 110100:  0, ignore
	0,  // 53, 110101:  0, ignore
	0,  // 54, 110110:  0, ignore
	0,  // 55, 110111:  0, ignore
	0,  // 56, 111000:  0, ignore
	0,  // 57, 111001:  0, ignore
	0,  // 58, 111010:  0, ignore
	0,  // 59, 111011:  0, ignore
	0,  // 60, 111100:  0, ignore (ERASE-WHILE-HELD is handled by other routines)
	0,  // 61, 111101:  0, ignore
	0,  // 62, 111110:  0, ignore
	0,  // 63, 111111:  0, ignore
};

// Lookup-table for all base BPM values, in microseconds-per-tick
const float MCS_TABLE[] PROGMEM = {
	41666.6667, // 60
	40983.6066, // 61
	40322.5806, // 62
	39682.5397, // 63
	39062.5, // 64
	38461.5385, // 65
	37878.7879, // 66
	37313.4328, // 67
	36764.7059, // 68
	36231.8841, // 69
	35714.2857, // 70
	35211.2676, // 71
	34722.2222, // 72
	34246.5753, // 73
	33783.7838, // 74
	33333.3333, // 75
	32894.7368, // 76
	32467.5325, // 77
	32051.2821, // 78
	31645.5696, // 79
	31250.0, // 80
	30864.1975, // 81
	30487.8049, // 82
	30120.4819, // 83
	29761.9048, // 84
	29411.7647, // 85
	29069.7674, // 86
	28735.6322, // 87
	28409.0909, // 88
	28089.8876, // 89
	27777.7778, // 90
	27472.5275, // 91
	27173.913, // 92
	26881.7204, // 93
	26595.7447, // 94
	26315.7895, // 95
	26041.6667, // 96
	25773.1959, // 97
	25510.2041, // 98
	25252.5253, // 99
	25000.0, // 100
	24752.4752, // 101
	24509.8039, // 102
	24271.8447, // 103
	24038.4615, // 104
	23809.5238, // 105
	23584.9057, // 106
	23364.486, // 107
	23148.1481, // 108
	22935.7798, // 109
	22727.2727, // 110
	22522.5225, // 111
	22321.4286, // 112
	22123.8938, // 113
	21929.8246, // 114
	21739.1304, // 115
	21551.7241, // 116
	21367.5214, // 117
	21186.4407, // 118
	21008.4034, // 119
	20833.3333, // 120
	20661.157, // 121
	20491.8033, // 122
	20325.2033, // 123
	20161.2903, // 124
	20000.0, // 125
	19841.2698, // 126
	19685.0394, // 127
	19531.25, // 128
	19379.845, // 129
	19230.7692, // 130
	19083.9695, // 131
	18939.3939, // 132
	18796.9925, // 133
	18656.7164, // 134
	18518.5185, // 135
	18382.3529, // 136
	18248.1752, // 137
	18115.942, // 138
	17985.6115, // 139
	17857.1429, // 140
	17730.4965, // 141
	17605.6338, // 142
	17482.5175, // 143
	17361.1111, // 144
	17241.3793, // 145
	17123.2877, // 146
	17006.8027, // 147
	16891.8919, // 148
	16778.5235, // 149
	16666.6667, // 150
	16556.2914, // 151
	16447.3684, // 152
	16339.8693, // 153
	16233.7662, // 154
	16129.0323, // 155
	16025.641, // 156
	15923.5669, // 157
	15822.7848, // 158
	15723.2704, // 159
	15625.0, // 160
	15527.9503, // 161
	15432.0988, // 162
	15337.4233, // 163
	15243.9024, // 164
	15151.5152, // 165
	15060.241, // 166
	14970.0599, // 167
	14880.9524, // 168
	14792.8994, // 169
	14705.8824, // 170
	14619.883, // 171
	14534.8837, // 172
	14450.8671, // 173
	14367.8161, // 174
	14285.7143, // 175
	14204.5455, // 176
	14124.2938, // 177
	14044.9438, // 178
	13966.4804, // 179
	13888.8889, // 180
	13812.1547, // 181
	13736.2637, // 182
	13661.2022, // 183
	13586.9565, // 184
	13513.5135, // 185
	13440.8602, // 186
	13368.984, // 187
	13297.8723, // 188
	13227.5132, // 189
	13157.8947, // 190
	13089.0052, // 191
	13020.8333, // 192
	12953.3679, // 193
	12886.5979, // 194
	12820.5128, // 195
	12755.102, // 196
	12690.3553, // 197
	12626.2626, // 198
	12562.8141, // 199
	12500.0, // 200
	12437.8109, // 201
	12376.2376, // 202
	12315.2709, // 203
	12254.902, // 204
	12195.122, // 205
	12135.9223, // 206
	12077.2947, // 207
	12019.2308, // 208
	11961.7225, // 209
	11904.7619, // 210
	11848.3412, // 211
	11792.4528, // 212
	11737.0892, // 213
	11682.243, // 214
	11627.907, // 215
	11574.0741, // 216
	11520.7373, // 217
	11467.8899, // 218
	11415.5251, // 219
	11363.6364, // 220
	11312.2172, // 221
	11261.2613, // 222
	11210.7623, // 223
	11160.7143, // 224
	11111.1111, // 225
	11061.9469, // 226
	11013.2159, // 227
	10964.9123, // 228
	10917.0306, // 229
	10869.5652, // 230
	10822.5108, // 231
	10775.8621, // 232
	10729.6137, // 233
	10683.7607, // 234
	10638.2979, // 235
	10593.2203, // 236
	10548.5232, // 237
	10504.2017, // 238
	10460.251, // 239
	10416.6667, // 240
	10373.444, // 241
	10330.5785, // 242
	10288.0658, // 243
	10245.9016, // 244
	10204.0816, // 245
	10162.6016, // 246
	10121.4575, // 247
	10080.6452, // 248
	10040.1606, // 249
	10000.0, // 250
	9960.1594, // 251
	9920.6349, // 252
	9881.4229, // 253
	9842.5197, // 254
	9803.9216 // 255
};
