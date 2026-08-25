// **** DISPLAY LOGIC - WHAT TO SHOW ON THE OLED, USING THE PROJECT'S STATE ****
#include "display.h"
#include "ssd1306.h"
#include "state.h"
#include <stdio.h>
#include <string.h>

#define DISPLAY_REFRESH_MS   200    // how often the screen actually redraws
#define MESSAGE_DURATION_MS  2000   // how long a Display_ShowMessage() stays visible

static char messageText[22] = "";
static uint32_t messageExpireTick = 0;

void Display_Init(void)
{
  SSD1306_Init();
}

void Display_ShowMessage(const char *msg)
{
  strncpy(messageText, msg, sizeof(messageText) - 1);
  messageText[sizeof(messageText) - 1] = '\0';
  messageExpireTick = HAL_GetTick() + MESSAGE_DURATION_MS;
}

static const char *StatusName(SystemStatus_t s)
{
  switch (s) {
    case IDLE:    return "IDLE";
    case RUNNING: return "RUNNING";
    case E_STOP:  return "E-STOP";
    default:      return "?";
  }
}

void Display_Update(void)
{
  static uint32_t lastUpdateTick = 0;
  uint32_t now = HAL_GetTick();

  if (now - lastUpdateTick < DISPLAY_REFRESH_MS) {
    return;
  }
  lastUpdateTick = now;

  char line[22];
  SSD1306_Clear();

  // Row 0: system state
  SSD1306_SetCursor(0, 0);
  snprintf(line, sizeof(line), "STATE: %s", StatusName(systemStatus));
  SSD1306_WriteString(line);

  // Row 1: speed - target vs measured
  SSD1306_SetCursor(0, 1);
  snprintf(line, sizeof(line), "RPM   %d/%d", (int)rpm, (int)targetRPM);
  SSD1306_WriteString(line);

  // Row 2: target time, or remaining time while running - shown as MM:SS
  SSD1306_SetCursor(0, 2);
  if (systemStatus == RUNNING) {
    int32_t elapsedSec = (HAL_GetTick() - runStartTick) / 1000;
    int32_t remainingSec = (int32_t)targetTimeSec - elapsedSec;
    if (remainingSec < 0) {
      remainingSec = 0;
    }
    snprintf(line, sizeof(line), "TIME  %02d:%02d", (int)(remainingSec / 60), (int)(remainingSec % 60));
  } else {
    snprintf(line, sizeof(line), "TIME  %02d:%02d", (int)(targetTimeSec / 60), (int)(targetTimeSec % 60));
  }
  SSD1306_WriteString(line);

  // Row 3: which parameter +/- currently changes (only meaningful in IDLE)
  SSD1306_SetCursor(0, 3);
  if (systemStatus == IDLE) {
    snprintf(line, sizeof(line), "EDIT: %s", (editMode == EDIT_SPEED) ? "SPEED" : "TIME");
  } else {
    snprintf(line, sizeof(line), "EDIT: -");
  }
  SSD1306_WriteString(line);

  // Row 4: lid status
  SSD1306_SetCursor(0, 4);
  snprintf(line, sizeof(line), "LID: %s", lid_open ? "OPEN" : "CLOSED");
  SSD1306_WriteString(line);

  // Row 5: lock (servo) status
  SSD1306_SetCursor(0, 5);
  snprintf(line, sizeof(line), "LOCK: %s", servoLocked ? "LOCKED" : "UNLOCKED");
  SSD1306_WriteString(line);

  // Row 6: work progress bar (only meaningful while RUNNING)
  if (systemStatus == RUNNING && targetTimeSec > 0) {
    int32_t elapsedSec = (HAL_GetTick() - runStartTick) / 1000;
    int percent = (elapsedSec * 100) / targetTimeSec;
    if (percent > 100) {
      percent = 100;
    }
    SSD1306_DrawProgressBar(6, (uint8_t)percent);
  }

  // Row 7: temporary message
  if (HAL_GetTick() < messageExpireTick) {
    SSD1306_SetCursor(0, 7);
    SSD1306_WriteString(messageText);
  }

  SSD1306_UpdateScreen();
}
