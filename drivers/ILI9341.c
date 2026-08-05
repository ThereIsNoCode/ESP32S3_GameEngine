#include "ILI9341.h"

// ----------------------------------------------------------------
// Command / data writes — matches official driver exactly
// ----------------------------------------------------------------




uint16_t *framebuffer;                      
dma_desc_t dma_descriptors[N_DESC] __attribute__((aligned(4)));

#define BYTES_PER_TRANSFER  30720   // 8 descriptors × 3840 bytes
#define N_TRANSFERS         5       // 5 × 30720 = 153600
#define DESC_PER_TRANSFER   8       // descriptors per SPI transaction
#define BITS_PER_TRANSFER   ((uint32_t)BYTES_PER_TRANSFER * 8 - 1)  // fits in 18 bits ✅
void initialize_frameBuffer() {
    framebuffer = heap_caps_malloc(240 * 320 * 2, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);

    for (int i = 0; i < N_DESC; i++) {
        // EOF on every 8th descriptor (end of each SPI transfer)
        int last_in_chunk = ((i + 1) % DESC_PER_TRANSFER == 0) || (i == N_DESC - 1);

        dma_descriptors[i].dw0  = DMA_DW0_SIZE_FLAG | DMA_DW0_LENGTH_FLAG
                                 | DMA_DW0_OWNER_FLAG
                                 | (last_in_chunk ? DMA_DW0_EOF_FLAG : 0);
        dma_descriptors[i].buf  = (uint32_t)((uint8_t*)framebuffer + i * CHUNK_BYTES);
        dma_descriptors[i].next = (i == N_DESC - 1) ? 0 
                                 : (uint32_t)&dma_descriptors[i + 1];
    }
}

const uint32_t maxBytes = ((240*320*2) * 8) - 1;

//Since SPI_XXX_DLEN_REG (depends on which ESP) got smaller, now I have to do 5 bursts
void DMA_BlastBuffer() {
    SPI2_DMA_CONF_REG &= ~(1<<28);
    ili9341_set_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    for (int t = 0; t < N_TRANSFERS; t++) {
        int base_desc = t * DESC_PER_TRANSFER;

        // Reset GDMA
        GDMA_OUT_CONF0_CH0_REG |=  0b0001;
        GDMA_OUT_CONF0_CH0_REG &= ~0b0001;

        // Load this chunk's first descriptor
        GDMA_OUT_LINK_CH0_REG     = ((uint32_t)&dma_descriptors[base_desc] & 0xFFFFF);
        GDMA_OUT_PERI_SEL_CH0_REG = 0;

        // Start GDMA
        GDMA_OUT_LINK_CH0_REG |= (1u << 21);

        // Enable SPI DMA
        SPI2_DMA_CONF_REG |=  (1<<31)|(1<<30)|(1<<29);
        SPI2_DMA_CONF_REG &= ~((1<<31)|(1<<30)|(1<<29));
        SPI2_DMA_CONF_REG |=  (1<<28);

        SPI2_MS_DLEN_REG = BITS_PER_TRANSFER;
        SPI2_USER_REG    = (1u << 27);

        CS_LOW();
        DC_HIGH();
        SPI2_CMD_REG |= (1u << 23);
        while (SPI2_CMD_REG & (1u << 23)) {}
        SPI2_CMD_REG |= (1u << 24);

        // Wait for GDMA chunk done
        //while (!(GDMA_OUT_INT_RAW_CH0_REG & (1u << 3))) {}
        while (SPI2_CMD_REG & (1u << 24)) {}

        // Clear and stop
        GDMA_OUT_INT_CLR_CH0_REG  = 0x0F;
        GDMA_OUT_LINK_CH0_REG    |=  (1u << 20);
        GDMA_OUT_LINK_CH0_REG    &= ~(1u << 20);
        SPI2_DMA_CONF_REG &= ~(1<<28);
    }

    CS_HIGH();
}

void write_command(uint8_t cmd)
{
    CS_LOW();
    DC_LOW();
    spi_write_byte(cmd);
    CS_HIGH(); 

}

void write_data_byte(uint8_t data)
{
    CS_LOW();
    DC_HIGH();
    spi_write_byte(data);
    CS_HIGH(); 
}

void write_data_word(uint16_t data)
{
    CS_LOW();
    DC_HIGH();
    spi_write_byte((data >> 8) & 0xFF);
    spi_write_byte(data & 0xFF);
    CS_HIGH();
}

// void spi_write_byte(uint8_t data){
//     SPI2_MS_DLEN_REG = 7;
//     SPI2_USER_REG    = (1u << 27);
//     SPI2_W0_REG      = data;
//     SPI2_CMD_REG |= (1u << 23); while (SPI2_CMD_REG & (1u << 23)); // UPDATE
//     SPI2_CMD_REG |= (1u << 24); while (SPI2_CMD_REG & (1u << 24)); // USR (bit 24!)
// }

void spi_write_byte(uint8_t data)
{                       // select (active-low) BEFORE the transfer
    SPI2_USER_REG    = (1u << 27);
    SPI2_MS_DLEN_REG = 7;
    SPI2_W0_REG      = data;
    SPI2_CMD_REG |= (1u << 23); while (SPI2_CMD_REG & (1u << 23));
    SPI2_CMD_REG |= (1u << 24); while (SPI2_CMD_REG & (1u << 24));
}
// ----------------------------------------------------------------
// ILI9341 init — matches official Waveshare LCD_2IN4 sequence
// ----------------------------------------------------------------

void ili9341_reset(void)
{
    RST_HIGH();
    vTaskDelay(pdMS_TO_TICKS(100));
    RST_LOW();
    vTaskDelay(pdMS_TO_TICKS(100));
    RST_HIGH();
    vTaskDelay(pdMS_TO_TICKS(100));
}
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_BGR 0x08
void ili9341_init(void)
{
    ili9341_reset();

    BL_HIGH();
    vTaskDelay(pdMS_TO_TICKS(100));

    write_command(0x11);                     // sleep out
    vTaskDelay(pdMS_TO_TICKS(120));

    write_command(0xCF);
    write_data_byte(0x00);
    write_data_byte(0xC1);
    write_data_byte(0x30);

    write_command(0xED);
    write_data_byte(0x64);
    write_data_byte(0x03);
    write_data_byte(0x12);
    write_data_byte(0x81);

    write_command(0xE8);
    write_data_byte(0x85);
    write_data_byte(0x00);
    write_data_byte(0x79);
 
    write_command(0xCB);
    write_data_byte(0x39);
    write_data_byte(0x2C);
    write_data_byte(0x00);
    write_data_byte(0x34);
    write_data_byte(0x02);

    write_command(0xF7);
    write_data_byte(0x20);

    write_command(0xEA);
    write_data_byte(0x00);
    write_data_byte(0x00);
 
    write_command(0xC0);                     // power control 1
    write_data_byte(0x1D);

    write_command(0xC1);                     // power control 2
    write_data_byte(0x12);

    write_command(0xC5);                     // VCOM control 1
    write_data_byte(0x33);
    write_data_byte(0x3F);
     
    write_command(0xC7);                     // VCOM control 2
    write_data_byte(0x92);
     
    write_command(0x3A);                     // pixel format
    write_data_byte(0x55);                   // RGB565
     
    write_command(0x36);                     // memory access control
    write_data_byte(MADCTL_MX  | 0x08 | 0x20);
     
    write_command(0xB1);                     // frame rate
    write_data_byte(0b00);
    write_data_byte(0b10000);
     
    //Slow Scanline speed
    // write_data_byte(0b11);
    // write_data_byte(0b11111);

    write_command(0xB6);                     // display function control
    write_data_byte(0x0A);
    write_data_byte(0xA2);
     
    write_command(0x44);
    write_data_byte(0x02);
     
    write_command(0xF2);                     // 3G gamma disable
    write_data_byte(0x00);
     
    write_command(0x26);                     // gamma curve
    write_data_byte(0x01);
     
    write_command(0xE0);                     // positive gamma
    write_data_byte(0x0F); write_data_byte(0x22); write_data_byte(0x1C); write_data_byte(0x1B);
    write_data_byte(0x08); write_data_byte(0x0F); write_data_byte(0x48); write_data_byte(0xB8);
    write_data_byte(0x34); write_data_byte(0x05); write_data_byte(0x0C); write_data_byte(0x09);
    write_data_byte(0x0F); write_data_byte(0x07); write_data_byte(0x00);
     
    write_command(0xE1);                     // negative gamma
    write_data_byte(0x00); write_data_byte(0x23); write_data_byte(0x24); write_data_byte(0x07);
    write_data_byte(0x10); write_data_byte(0x07); write_data_byte(0x38); write_data_byte(0x47);
    write_data_byte(0x4B); write_data_byte(0x0A); write_data_byte(0x13); write_data_byte(0x06);
    write_data_byte(0x30); write_data_byte(0x38); write_data_byte(0x0F);
     
    // write_command(0x33);  //Vertical Scrolling Definition TFA + VSA + BFA = 320 or else undefined behaviour
    // write_data_byte(0x00); write_data_byte(0x00); //Parameter 1&2 TFA (Top Fixed Area)
    // write_data_byte(0x01); write_data_byte(0x40); //Parameter 3&4 VSA (Vertical Scrolling Area)
    // write_data_byte(0x00); write_data_byte(0x00); //Parameter 5&6 BFA (Bottom Fixed Area) 



    write_command(0x29);                     // display on
     

    //write_command(0x28);                     // display off
}
void ili9341_fill_screen(uint16_t color)
{
    ili9341_set_window(0, 0, 320, 240);

    CS_LOW();
    DC_HIGH();

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    for (int i = 0; i < 240 * 320; i++) {
        spi_write_byte(hi);
        spi_write_byte(lo);
    }

    CS_HIGH();
}
// ----------------------------------------------------------------
// Drawing — matches official driver window convention (Xend-1, Yend-1)
// ----------------------------------------------------------------

void ili9341_set_window(uint16_t y0, uint16_t x0, uint16_t y1, uint16_t x1)
{

    write_command(0x2A);
    write_data_byte(y0 >> 8);
    write_data_byte(y0 & 0xFF);
    write_data_byte((y1 - 1) >> 8);
    write_data_byte((y1 - 1) & 0xFF);

    write_command(0x2B);
    write_data_byte(x0 >> 8);
    write_data_byte(x0 & 0xFF);
    write_data_byte((x1 - 1) >> 8);
    write_data_byte((x1 - 1) & 0xFF);

    write_command(0x2C);
}



// void ili9341_fill_screen(uint16_t color)
// {
//     ili9341_set_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

//     DC_HIGH();
//     CS_LOW();

//     // Pack 2 pixels per word, fill 8 words = 16 pixels = 32 bytes per transfer
//     uint32_t word = ((uint32_t)(color >> 8)  << 24) |
//                     ((uint32_t)(color & 0xFF) << 16) |
//                     ((uint32_t)(color >> 8)   <<  8) |
//                     ((uint32_t)(color & 0xFF));

//     for (int i = 0; i < 8; i++) {
//         SPI3_W_REG(i) = word;
//     }

//     SPI3_USER_REG  = (1 << 27);
//     SPI3_MOSI_DLEN_REG = (32 * 8 - 1);   // 256 bits - 1
//     SPI3_USER_REG      = (1 << 27);       // SPI_USR_MOSI
//     // 240 * 320 = 76800 pixels * 2 bytes = 153600 bytes
//     // 32 bytes per transfer = 4800 transfers
//     for (uint32_t i = 0; i < 4800; i++) {
//         SPI3_CMD_REG = (1 << 18);
//         while (SPI3_CMD_REG & (1 << 18));
//     }

//     CS_HIGH();
// }

// void ili9341_fill_screen_interlaced(uint16_t color, uint8_t field)
// {
//     // field 0 = even lines, field 1 = odd lines
//     for (uint16_t y = field; y < SCREEN_HEIGHT; y += 2) {
//         ili9341_set_window(0, y, SCREEN_WIDTH, y + 1);
//         DC_HIGH();
//         CS_LOW();

//         // pack color into buffer
//         uint8_t hi = color >> 8;
//         uint8_t lo = color & 0xFF;
//         uint32_t word = ((uint32_t)lo << 24) | ((uint32_t)hi << 16) |
//                         ((uint32_t)lo <<  8) | ((uint32_t)hi);

//         for (int i = 0; i < 8; i++) SPI3_W_REG(i) = word;

//         SPI3_MOSI_DLEN_REG = (60 * 8 - 1);  // 240 pixels * 2 bytes = 480 bytes
//                                               // but max transfer is 64 bytes
//                                               // so split into chunks
//         // send 240 pixels = 480 bytes in 15 x 32-byte transfers
//         SPI3_USER_REG = (1 << 27);
//         SPI3_MOSI_DLEN_REG = (32 * 8 - 1);
//         for (int i = 0; i < 15; i++) {
//             SPI3_CMD_REG = (1 << 18);
//             while (SPI3_CMD_REG & (1 << 18));
//         }
//         CS_HIGH();
//     }
// }

// void ili9341_draw_rect(uint16_t color, uint8_t field, uint16_t x_pos, uint16_t y_pos, uint16_t size)
// {

//     ili9341_set_window(x_pos, y_pos, x_pos + size, y_pos+size);

//     // field 0 = even lines, field 1 = odd lines
//     for (uint16_t y = field; y < SCREEN_HEIGHT; y += 2) {
        
//         DC_HIGH();
//         CS_LOW();

//         // pack color into buffer
//         uint8_t hi = color >> 8;
//         uint8_t lo = color & 0xFF;
//         uint32_t word = ((uint32_t)lo << 24) | ((uint32_t)hi << 16) |
//                         ((uint32_t)lo <<  8) | ((uint32_t)hi);

//         for (int i = 0; i < 8; i++) SPI3_W_REG(i) = word;

//         SPI3_MOSI_DLEN_REG = (60 * 8 - 1);  // 240 pixels * 2 bytes = 480 bytes
//                                               // but max transfer is 64 bytes
//                                               // so split into chunks
//         // send 240 pixels = 480 bytes in 15 x 32-byte transfers
//         SPI2_USER_REG = (1 << 27);
//         SPI3_MOSI_DLEN_REG = (32 * 8 - 1);
//         for (int i = 0; i < 15; i++) {
//             SPI2_CMD_REG = (1 << 18);
//             while (SPI3_CMD_REG & (1 << 18));
//         }
//         CS_HIGH();
//     }
// }

// void ili9341_draw_pixel(uint16_t color)
// {
//     DC_HIGH();                 // data mode
//     CS_LOW();

//     // ILI9341 wants high byte first on the wire.
//     // SPI sends W0 byte 0 first, so put hi in the lowest byte.
//     uint8_t hi = color >> 8;
//     uint8_t lo = color & 0xFF;
//     SPI2_W0_REG = (uint32_t)hi | ((uint32_t)lo << 8);

//     SPI2_USER_REG     = (1 << 27);     // USR_MOSI: enable MOSI phase
//     SPI3_MOSI_DLEN_REG = (16 - 1);     // 1 pixel = 16 bits

//     SPI2_CMD_REG |= (1 << 18);         // SPI_USR: START the transfer
//     while (SPI2_CMD_REG & (1 << 18));  // wait until hardware clears it

//     CS_HIGH();
// }

// inline uint8_t CapColor(const uint16_t color, const uint8_t maxRange){
//     // if(color > maxRange){
//     //     return maxRange;
//     // }
//     return color;
// }

// inline uint16_t tint_gray(const uint8_t shade4, const uint8_t tr, const uint8_t tg, const uint8_t tb, const uint8_t gamma)
// {
//     uint8_t r = CapColor((tr * shade4 / 15)*gamma, 0xF8);   // shade4 is 0..15
//     uint8_t g = CapColor((tg * shade4 / 15)*gamma, 0xF8);
//     uint8_t b = CapColor((tb * shade4 / 15)*gamma, 0xF8);



//     return COLOR_RGB565(r, g, b);
// }

void ShiftScreen(uint16_t offset){
    write_command(0x37);
    write_data_byte((offset>>8) & 0xFF);
    write_data_byte(offset & 0xFF);
}

