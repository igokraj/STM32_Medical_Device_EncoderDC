#pragma once

#include "main.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_I2C_ADDR (0x3C << 1)   // 7-bit address 0x3C, shifted for HAL (8-bit)

void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_SetCursor(uint8_t x, uint8_t y);   // x in pixels (0-127), y in 8px rows (0-7)
void SSD1306_WriteChar(char c);
void SSD1306_WriteString(const char *str);
void SSD1306_UpdateScreen(void);

// Draws a full-width progress bar on the given 8px row (page 0-7).
// Filled portion is solid, empty portion is an outline (top/bottom pixel only).
void SSD1306_DrawProgressBar(uint8_t page, uint8_t percent);
