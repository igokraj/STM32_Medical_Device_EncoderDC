#pragma once

void Display_Init(void);
void Display_Update(void);

// Shows a short message on the bottom line for ~2 seconds (e.g. "CLOSE THE LID")
void Display_ShowMessage(const char *msg);
