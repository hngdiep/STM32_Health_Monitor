
#include "tft_display.h"
#include "tft_spi2.h"
#include "FreeRTOS.h"
#include "task.h"

// Font 5x7: A-Z
static const uint8_t abc[26][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x41,0x41,0x7F,0x41,0x41}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x20,0x18,0x20,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}  // Z
};

// Font 5x7: 0-9 (sua lai dung chieu)
static const uint8_t num[10][5] = {
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}  // 9
};

// Lay bitmap cua ky tu
static const uint8_t *get_font(char c)
{
    static const uint8_t blank[5] = {0,0,0,0,0};
    static const uint8_t colon[5] = {0,0x36,0x36,0,0};
    static const uint8_t dot[5]   = {0,0x60,0x60,0,0};
    static const uint8_t dash[5]  = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t pct[5]   = {0x63,0x13,0x08,0x64,0x63};
    static const uint8_t ques[5]  = {0x02,0x01,0x51,0x09,0x06};

    if(c >= 'a' && c <= 'z') c -= 32;
    if(c >= 'A' && c <= 'Z') return abc[c-'A'];
    if(c >= '0' && c <= '9') return num[c-'0'];

    switch(c) {
        case ':': return colon;
        case '.': return dot;
        case '-': return dash;
        case '%': return pct;
        case ' ': return blank;
        default:  return ques;
    }
}

// Khoi tao TFT ILI9341
void TFT_Display_Init(void)
{
    TFT_SPI2_Init();
    TFT_Reset();

    TFT_Select();
    TFT_WriteCommand(0x01);
    TFT_Deselect();
    vTaskDelay(pdMS_TO_TICKS(150));

    TFT_Select();
    TFT_WriteCommand(0x3A); TFT_WriteData(0x55);
    TFT_WriteCommand(0x36); TFT_WriteData(0x48);
    TFT_WriteCommand(0x11);
    TFT_Deselect();
    vTaskDelay(pdMS_TO_TICKS(150));

    TFT_Select();
    TFT_WriteCommand(0x29);
    TFT_Deselect();
    vTaskDelay(pdMS_TO_TICKS(50));
}

// Chon vung ghi du lieu
static void window(uint16_t x0,uint16_t y0,
                   uint16_t x1,uint16_t y1)
{
    TFT_WriteCommand(0x2A);
    TFT_WriteData(x0>>8); TFT_WriteData(x0&255);
    TFT_WriteData(x1>>8); TFT_WriteData(x1&255);

    TFT_WriteCommand(0x2B);
    TFT_WriteData(y0>>8); TFT_WriteData(y0&255);
    TFT_WriteData(y1>>8); TFT_WriteData(y1&255);

    TFT_WriteCommand(0x2C);
}

// Ve hinh chu nhat
void TFT_FillRect(uint16_t x,uint16_t y,
                  uint16_t w,uint16_t h,uint16_t color)
{
    if(!w || !h || x>=TFT_WIDTH || y>=TFT_HEIGHT) return;
    if(w>TFT_WIDTH-x) w=TFT_WIDTH-x;
    if(h>TFT_HEIGHT-y) h=TFT_HEIGHT-y;

    uint32_t n=(uint32_t)w*h;
    TFT_Select();
    window(x,y,x+w-1,y+h-1);

    while(n--) {
        TFT_WriteData(color>>8);
        TFT_WriteData(color&255);
    }
    TFT_Deselect();
}

void TFT_FillScreen(uint16_t c)
{
    TFT_FillRect(0,0,TFT_WIDTH,TFT_HEIGHT,c);
}

void TFT_DrawPixel(uint16_t x,uint16_t y,uint16_t c)
{
    TFT_FillRect(x,y,1,1,c);
}

// Ve chu 5x7 phong to 2 lan
void TFT_DrawText(uint16_t x,uint16_t y,
                  const char *text,uint16_t fg,uint16_t bg)
{
    if(!text || y+16>TFT_HEIGHT) return;

    while(*text && x+12<=TFT_WIDTH) {
        const uint8_t *f=get_font(*text++);
        TFT_Select();
        window(x,y,x+11,y+15);

        for(uint8_t yy=0;yy<16;yy++)
            for(uint8_t xx=0;xx<12;xx++) {
                uint8_t col=xx/2, row=yy/2;
                uint16_t c=(col<5 && row<7 &&
                     (f[col] & (1U<<row))) ? fg:bg;
                TFT_WriteData(c>>8);
                TFT_WriteData(c&255);
            }

        TFT_Deselect();
        x+=12;
    }
}
