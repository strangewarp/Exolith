
// Build all unique scales
void populateScales() {
  word scltop = 0;
  for (word i = 0; i < 352; i++) {
    scales[i] = 0;
  }
  for (word permute = 1; permute <= 4095; permute++) {
    bool scalefound = false;
    for (word s = 0; s < scltop; s++) {
      word rotscl = permute;
      for (byte i = 0; i < 12; i++) {
        rotscl = rotateScale(rotscl, 1);
        if (rotscl == scales[s]) {
          scalefound = true;
          break;
        }
      }
      if (scalefound) { break; }
    }
    if (scalefound) { continue; }
    scales[scltop] = permute;
    for (byte i = 0; i < 12; i++) {
      scales[scltop] = max(scales[scltop], rotateScale(scales[scltop], 1));
    }
    scltop++;
  }
}

// Populate k-speces values, based on the contents of scales
void populateSpecies() {
  
}

// Rotate a scale by a given amount
word rotateScale(word scl, short amt) {
  for (word i = 0; i != abs(amt); i++) {
    scl = (amt < 0) ? (((scl & 2048) ? 1 : 0) | (scl << 1)) : (((scl & 1) ? 2048 : 0) | (scl >> 1));
  }
  return scl;
}


