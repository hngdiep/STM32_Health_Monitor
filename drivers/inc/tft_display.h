#ifndef TFT_DISPLAY_H
#define TFT_DISPLAY_H
#include <stdint.h>
#define TFT_WIDTH  240U
#define TFT_HEIGHT 320U
#define TFT_BLACK   0x0000U
#define TFT_WHITE   0xFFFFU
#define TFT_RED     0xF800U
#define TFT_GREEN   0x07E0U
#define TFT_BLUE    0x001FU
#define TFT_YELLOW  0xFFE0U
#define TFT_CYAN    0x07FFU
void TFT_Display_Init(void);
void TFT_DrawPixel(uint16_t x,uint16_t y,uint16_t color);
void TFT_FillScreen(uint16_t color);
void TFT_FillRect(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t color);
void TFT_DrawText(uint16_t x,uint16_t y,const char *utf8,uint16_t fg,uint16_t bg);
#endif
