#pragma once
#include "esp_family/ESP32-S3.h"


// --- Display dimensions ---
#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 320
#define SCREEN_WIDTH_HALF 320/2
// --- RGB565 colors ---

//Old, colors transmit from LSB
//#define COLOR_RGB565(r, g, b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define COLOR_RGB565(r, g, b) ( \
    (((g) & 0x1C) << 11) |  /* green low 3 bits  -> output bits 15..13 */ \
    (((b) & 0xF8) << 5)  |  /* blue  5 bits      -> output bits 12..8  */ \
    ((r) & 0xF8)         |  /* red   5 bits      -> output bits 7..3   */ \
    (((g) >> 5) & 0x07)  )  /* green high 3 bits -> output bits 2..0   */
#define COLOR_BLACK    0x0000
#define COLOR_WHITE    0xFFFF
#define COLOR_RED      0xF800
#define COLOR_GREEN    0x07E0
#define COLOR_BLUE     0x001F
#define COLOR_CYAN     0x07FF
#define COLOR_MAGENTA  0xF81F
#define COLOR_YELLOW   0xFFE0


//https://www.tutorialpedia.org/blog/is-there-a-way-to-limit-an-integer-value-to-a-certain-range-without-branching/#bitwise-masks-and-arithmetic
static inline uint8_t CapColor(uint16_t color, uint8_t maxRange){
    if (color > maxRange) return maxRange;
    return color;
}

static inline uint16_t tint_gray(uint8_t shade4, uint16_t tintColor){
    uint8_t gamma = (tintColor >> 12);
    uint8_t r = CapColor((((tintColor >> 8)&0xF) * shade4 ) * gamma, 0xF8);
    uint8_t g = CapColor((((tintColor >> 4)&0xF) * shade4 ) * gamma, 0xF8);
    uint8_t b = CapColor((((tintColor)&0xF) * shade4 ) * gamma, 0xF8);
    return COLOR_RGB565(r, g, b);
}
void write_command(uint8_t cmd);
void write_data_byte(uint8_t data);
void write_data_word(uint16_t data);
void spi_write_byte(uint8_t data);
void ili9341_reset(void);
void ili9341_init(void);
void ili9341_set_window(uint16_t y0, uint16_t x0, uint16_t y1, uint16_t x1);
void ili9341_fill_screen(uint16_t color);
void ili9341_fill_screen_interlaced(uint16_t color, uint8_t field);
void ili9341_draw_rect(uint16_t color, uint8_t field, uint16_t x_pos, uint16_t y_pos, uint16_t size);
void ili9341_draw_pixel(uint16_t color);
void ShiftScreen(uint16_t offset);

#define CHUNK_BYTES  3840
#define N_DESC       40

void DMA_BlastBuffer();
void initialize_frameBuffer();

extern uint16_t *framebuffer;
extern dma_desc_t dma_descriptors[N_DESC];