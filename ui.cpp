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

  // Linia 1 - zgodna z poprzednim układem:
  // "Asst:" na pozycji 0
  // wartość assistPower od pozycji 5
  // "RPM:" od pozycji 9
  // wartość rpm od pozycji 13
  writeAt(line1, 0, "Asst:");

  char assistBuf[8];
  snprintf(assistBuf, sizeof(assistBuf), "%u%%", assistPower);
  writeAt(line1, 5, assistBuf);

  writeAt(line1, 9, "RPM:");
  writeUIntAt(line1, 13, rpm);

  // Linia 2 - zgodna z poprzednim układem:
  // "Assist: " od pozycji 0
  // status od pozycji 8
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