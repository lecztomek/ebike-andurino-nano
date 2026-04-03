#include "ui.h"
#include <stdio.h>

static void clearLine(char line[17]) {
  for (byte i = 0; i < 16; i++) {
    line[i] = ' ';
  }
  line[16] = '\0';
}

static void writeAt(char line[17], byte col, const char* text) {
  while (*text && col < 16) {
    line[col++] = *text++;
  }
}

static void writeUIntAt(char line[17], byte col, unsigned int value) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%u", value);
  writeAt(line, col, buf);
}

// ============================================================
// SCREEN 0 - bez zmian
// ============================================================

static void buildScreen0Lines(
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  char line1[17],
  char line2[17]
) {
  clearLine(line1);
  clearLine(line2);

  bool assistActive = targetAssist > 0;

  writeAt(line1, 0, "Asst:");

  char assistBuf[8];
  snprintf(assistBuf, sizeof(assistBuf), "%u%%", assistPower);
  writeAt(line1, 5, assistBuf);

  writeAt(line1, 9, "RPM:");
  writeUIntAt(line1, 13, rpm);

  writeAt(line2, 0, "Assist: ");

  if (walkingAssistActive) {
    writeAt(line2, 8, "WALKING");
  } else {
    writeAt(line2, 8, assistActive ? "ON " : "OFF");

    if (assistActive) {
      char targetBuf[8];
      snprintf(targetBuf, sizeof(targetBuf), "(%u) ", targetAssist);
      writeAt(line2, 11, targetBuf);
    }
  }
}

// ============================================================
// FONT 2x2 BIG NUMBERS - Tron style
// Oparte na Alpenglow_BigNums2x2
// 8 shared custom glyphs + mapowanie cyfr 0..9 na 4 kafle
// Kolejnosc kafli: UL, LL, UR, LR
// ============================================================

const uint8_t UI_BIG_TRON_GLYPHS[8][8] = {
  { B11111, B11111, B00000, B00000, B00000, B00000, B00000, B00000 }, // tron0
  { B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000 }, // tron1
  { B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111 }, // tron2
  { B11111, B11111, B00011, B00011, B00011, B00011, B11111, B11111 }, // tron3
  { B11111, B11111, B11000, B11000, B11000, B11000, B11111, B11111 }, // tron4
  { B11111, B11111, B11000, B11000, B11000, B11000, B11000, B11000 }, // tron5
  { B00011, B00011, B00011, B00011, B00011, B00011, B11111, B11111 }, // tron6
  { B11111, B11111, B00011, B00011, B00011, B00011, B00011, B00011 }  // tron7
};

const uint8_t UI_BIG_TRON_DIGITS[11][4] = {
  { 5,   255, 7,   6   }, // 0
  { 0,   2,   1,   255 }, // 1
  { 0,   255, 3,   2   }, // 2
  { 0,   2,   3,   255 }, // 3
  { 1,   0,   255, 1   }, // 4
  { 4,   2,   0,   255 }, // 5
  { 5,   4,   0,   255 }, // 6
  { 0,   32,  255, 1   }, // 7
  { 255, 4,   3,   255 }, // 8
  { 255, 2,   3,   6   }, // 9
  { 32,  32,  32,  32  }  // blank
};

void buildScreenLines(
  byte currentScreen,
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  char line1[17],
  char line2[17]
) {
  switch (currentScreen) {
    case 0:
    default:
      buildScreen0Lines(
        rpm,
        targetAssist,
        assistPower,
        walkingAssistActive,
        line1,
        line2
      );
      break;
  }
}