#pragma once
#include <Arduino.h>

struct ScreenDisplayState {
  int lastRPM = 10;
  int lastTargetAssist = 0;
  int lastAssistPower = 0;
  unsigned long lastDisplayUpdateTime = 0;
};

void buildScreenLines(
  byte currentScreen,
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  char line1[17],
  char line2[17]
);

template <typename TLcd>
void renderScreen(
  TLcd& lcd,
  byte currentScreen,
  bool refreshNow,
  unsigned int rpm,
  unsigned int targetAssist,
  unsigned int assistPower,
  bool walkingAssistActive,
  unsigned long currentTime,
  unsigned long displayRefreshInterval,
  ScreenDisplayState& displayState
) {
  if ((refreshNow ||
       rpm != displayState.lastRPM ||
       targetAssist != displayState.lastTargetAssist ||
       assistPower != displayState.lastAssistPower) &&
      (currentTime - displayState.lastDisplayUpdateTime >= displayRefreshInterval)) {

    char line1[17];
    char line2[17];

    buildScreenLines(
      currentScreen,
      rpm,
      targetAssist,
      assistPower,
      walkingAssistActive,
      line1,
      line2
    );

    lcd.setCursor(0, 0);
    lcd.print(line1);

    lcd.setCursor(0, 1);
    lcd.print(line2);

    displayState.lastRPM = rpm;
    displayState.lastTargetAssist = targetAssist;
    displayState.lastAssistPower = assistPower;
    displayState.lastDisplayUpdateTime = currentTime;
  }
}