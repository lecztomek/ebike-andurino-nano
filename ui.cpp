#include "ui.h"
#include <stdio.h>
#include <string.h>

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

static void writeIntAt(char line[17], byte col, int value) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", value);
  writeAt(line, col, buf);
}

static void writeLongAt(char line[17], byte col, long value) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%ld", value);
  writeAt(line, col, buf);
}

// ============================================================
// SCREEN 0 - ekran glowny, male literki
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

  writeAt(line2, 0, "Assist:");

  if (walkingAssistActive) {
    writeAt(line2, 8, "WALKING");
  } else {
    writeAt(line2, 8, assistActive ? "ON" : "OFF");

    if (assistActive) {
      char targetBuf[8];
      snprintf(targetBuf, sizeof(targetBuf), "(%u)", targetAssist);
      writeAt(line2, 11, targetBuf);
    }
  }
}

// ============================================================
// SCREEN 1 - HX, male literki
// ============================================================

static void buildScreen1HxLines(
  int hxLevel,
  long hxTorque,
  bool hxError,
  const char* hxStatusText,
  char line1[17],
  char line2[17]
) {
  clearLine(line1);
  clearLine(line2);

  // linia 1: level + torque
  // przyklad: "Lvl: 45 T:1234"
  writeAt(line1, 0, "Lvl:");
  writeIntAt(line1, 4, hxLevel);
  writeAt(line1, 7, "%");

  writeAt(line1, 9, "T:");
  writeLongAt(line1, 11, hxTorque);

  // linia 2: status
  const char* safeStatus = (hxStatusText && hxStatusText[0]) ? hxStatusText : (hxError ? "ERR" : "OK");
  writeAt(line2, 0, "State:");
  writeAt(line2, 8, safeStatus);
}

// ============================================================
// FONT 2x2 BIG NUMBERS - Tron style
// Oparte na Alpenglow_BigNums2x2
// 8 shared custom glyphs + mapowanie cyfr 0..9 na 4 kafle
// Kolejnosc kafli: UL, LL, UR, LR
// ============================================================

const uint8_t UI_BIG_TRON_GLYPHS[8][8] = {
  { B11111, B11111, B00000, B00000, B00000, B00000, B00000, B00000 }, // 0
  { B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000 }, // 1
  { B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111 }, // 2
  { B11111, B11111, B00011, B00011, B00011, B00011, B11111, B11111 }, // 3
  { B11111, B11111, B11000, B11000, B11000, B11000, B11111, B11111 }, // 4
  { B11111, B11111, B11000, B11000, B11000, B11000, B11000, B11000 }, // 5
  { B00011, B00011, B00011, B00011, B00011, B00011, B11111, B11111 }, // 6
  { B11000, B11000, B11000, B11000, B11000, B11000, B11111, B11111 }  // 7
};

// UWAGA: kolejnosc pod Twoj renderer:
// { UL, LL, UR, LR }
const uint8_t UI_BIG_TRON_DIGITS[11][4] = {
  { 5,   7,   255, 6   }, // 0
  { 0,   2,   1,   7   }, // 1
  { 0,   4,   3,   2   }, // 2
  { 0,   2,   3,   3   }, // 3
  { 1,   0,   1,   5   }, // 4
  { 4,   2,   0,   3   }, // 5
  { 5,   4,   0,   3   }, // 6
  { 0,   32,  3,   1   }, // 7
  { 4,   4,   3,   3   }, // 8
  { 4,   2,   3,   6   }, // 9
  { 32,  32,  32,  32  }  // blank
};

// const uint8_t UI_BIG_TRON_GLYPHS[8][8] = {
//   { B11111, B11111, B00000, B00000, B00000, B00000, B00000, B00000 }, // tron0
//   { B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000 }, // tron1
//   { B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111 }, // tron2
//   { B11111, B11111, B00011, B00011, B00011, B00011, B11111, B11111 }, // tron3
//   { B11111, B11111, B11000, B11000, B11000, B11000, B11111, B11111 }, // tron4
//   { B11111, B11111, B11000, B11000, B11000, B11000, B11000, B11000 }, // tron5
//   { B00011, B00011, B00011, B00011, B00011, B00011, B11111, B11111 }, // tron6
//   { B11111, B11111, B00011, B00011, B00011, B00011, B00011, B00011 }  // tron7
// };

// const uint8_t UI_BIG_TRON_DIGITS[11][4] = {
//   { 5,   255, 7,   6   }, // 0
//   { 0,   2,   1,   255 }, // 1
//   { 0,   255, 3,   2   }, // 2
//   { 0,   2,   3,   255 }, // 3
//   { 1,   0,   255, 1   }, // 4
//   { 4,   2,   0,   255 }, // 5
//   { 5,   4,   0,   255 }, // 6
//   { 0,   32,  255, 1   }, // 7
//   { 255, 4,   3,   255 }, // 8
//   { 255, 2,   3,   6   }, // 9
//   { 32,  32,  32,  32  }  // blank
// };

void buildScreenLines(
  byte currentScreen,
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  int hxLevel,
  long hxTorque,
  bool hxError,
  const char* hxStatusText,
  char line1[17],
  char line2[17]
) {
  switch (currentScreen) {
    case 0:
      buildScreen0Lines(
        rpm,
        targetAssist,
        assistPower,
        walkingAssistActive,
        line1,
        line2
      );
      break;

    case 1:
      buildScreen1HxLines(
        hxLevel,
        hxTorque,
        hxError,
        hxStatusText,
        line1,
        line2
      );
      break;

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