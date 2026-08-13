/* ILI9341 Display Driver for ESP32
   Register-level SPI, no HAL/IDF drivers
   Matches official Waveshare LCD_2IN4 driver behavior
*/
#include <stdio.h>
#include "../tiles/tileset.h"
#include "../drivers/ILI9341.h"
#include "entities/player.h"
#include "entities/gumby.h"
#include "entities/tortle.h"
#include "esp_timer.h"
#include "../maps/level_manager.h"
#include "entities/bomb.h"
#include "networking/network.h"

// ----------------------------------------------------------------
// Entry point
// ----------------------------------------------------------------
// ADD this define — this is what sets MOSI bit length, not USER1

#define pixelCount 76800
#define tileRows 240/16
#define tileColumn 320/16
        


uint32_t playerPosX;
//Result of this functionis 2931us AVG
void InsertTileToFrameBuffer(int tileX, int tileY) //Lets try writing multiple pixels while looping 8x8
{
    uint8_t tileId =
        lvl_data[(tileY * lvl_width) + tileX];

    uint8_t tileTexture[32];
    memcpy(tileTexture, tileSet[tileId], sizeof(tileTexture));
    
    
    for(int y = 0; y < 8; y++){
        uint32_t row0 = ((tileY*16) + (y*2)) * 320;
        uint32_t row1 = row0 + 320;

        for(int x = 0; x < 4; x++){
            uint8_t pixelByte = tileTexture[(y << 2) + x];
            uint8_t shade1 = pixelByte >> 4;
            uint8_t shade2 = pixelByte & 0x0F;

            uint32_t dstX = (tileX<<4) + (x<<2);
            framebuffer[dstX + 0 + row0] = shade1;
            framebuffer[dstX + 1 + row0] = shade1;
            framebuffer[dstX + 0 + row1] = shade1;
            framebuffer[dstX + 1 + row1] = shade1;

            framebuffer[dstX + 2 + row0] = shade2;
            framebuffer[dstX + 3 + row0] = shade2;
            framebuffer[dstX + 2 + row1] = shade2;
            framebuffer[dstX + 3 + row1] = shade2;
        }
    }


}

void InsertTile_Fast(int worldTileX, int tileY, int screenX)
{
    uint8_t tileId = lvl_data[(tileY * lvl_width) + worldTileX];
    const uint8_t *tex = tileSet[tileId];
    Entity player = *player_Current;
    int px = (player.position_x >> 2);   // player world pixel X (top-left)
    int py = (player.position_y >> 2);   // player world pixel Y (top-left)

    int playerCenteredPosX = (player.position_x >> 2)+8;  
    int playerCenteredPosY = (player.position_y >> 2)+8;   
    int playerCenterTileX = playerCenteredPosX >> 4;   // / 16
    int playerCenterTileY = playerCenteredPosY >> 4;

    int playerTileX = px >> 4;   // / 16
    int playerTileY = py >> 4;
    for(int y = 0; y < 8; y++){
        uint32_t row0 = ((tileY*16) + (y*2)) * 320;
        uint32_t row1 = row0 + 320;
        for(int x = 0; x < 4; x++){
            uint8_t pixelByte = tex[(y << 2) + x];
            uint8_t shade1 = pixelByte >> 4;
            uint8_t shade2 = pixelByte & 0x0F;

            uint8_t pixel1 = shade1;
            uint8_t pixel2 = shade2;
            uint32_t dstX = screenX + (x<<2);      // screen-relative now

            // if((playerCenterTileX-1 <= worldTileX && playerCenterTileX+1 >= worldTileX)  && 
            //    (playerCenterTileY-1 <= tileY && playerCenterTileY+1 >= tileY)){
            //     framebuffer[dstX + 0 + row0] = COLOR_RED;
            //     framebuffer[dstX + 1 + row0] = COLOR_RED;
            //     framebuffer[dstX + 0 + row1] = COLOR_RED;
            //     framebuffer[dstX + 1 + row1] = COLOR_RED;
            //     framebuffer[dstX + 2 + row0] = COLOR_RED;
            //     framebuffer[dstX + 3 + row0] = COLOR_RED;
            //     framebuffer[dstX + 2 + row1] = COLOR_RED;
            //     framebuffer[dstX + 3 + row1] = COLOR_RED;
            // }else{
                framebuffer[dstX + 0 + row0] = pixel1;
                framebuffer[dstX + 1 + row0] = pixel1;
                framebuffer[dstX + 0 + row1] = pixel1;
                framebuffer[dstX + 1 + row1] = pixel1;
                framebuffer[dstX + 2 + row0] = pixel2;
                framebuffer[dstX + 3 + row0] = pixel2;
                framebuffer[dstX + 2 + row1] = pixel2;
                framebuffer[dstX + 3 + row1] = pixel2;
            //}


    
        }
    }

    
}

void InsertTile_Clipped(int worldTileX, int tileY, int screenX)
{
    uint8_t tileId = lvl_data[(tileY * lvl_width) + worldTileX];
    const uint8_t *tex = tileSet[tileId];

    for(int y = 0; y < 8; y++){
        uint32_t row0 = ((tileY*16) + (y*2)) * 320;
        uint32_t row1 = row0 + 320;
        for(int x = 0; x < 4; x++){
            uint8_t pixelByte = tex[(y << 2) + x];
            uint8_t shade1 = pixelByte >> 4;
            uint8_t shade2 = pixelByte & 0x0F;
            int base = screenX + (x<<2);
            for(int px = 0; px < 4; px++){
                int col = base + px;
                if ((unsigned)col >= 320u) continue;   // clips <0 and >=320
                uint8_t shade = (px < 2) ? shade1 : shade2;
                uint8_t pixel = shade;
                framebuffer[col + row0] = pixel;
                framebuffer[col + row1] = pixel;
            }
        }
    }
}

void DMA_DrawMap(){
    Entity player = *player_Current;
    // camera left edge = player world X minus half screen (so player is centered)
    int cameraX = (player.position_x >> 2) - 160;

    int firstTileX  = cameraX >> 4;      // arithmetic shift; works for negative cameraX
    int pixelOffset = cameraX & 15;

    for(int y = 0; y < tileRows; y++){
        for(int sx = 0; sx <= 20; sx++){
            int worldTileX = firstTileX + sx;

            worldTileX %= lvl_width;
            if (worldTileX < 0) worldTileX += lvl_width;

            int screenX = (sx << 4) - pixelOffset;

            if (screenX >= 0 && screenX <= 320 - 16)
                InsertTile_Fast(worldTileX, y, screenX);
            else
                InsertTile_Clipped(worldTileX, y, screenX);
        }
    }
}
#include "soc/rtc.h"

const uint8_t isHost = 1;

#define FRAME_TIME_US 41666

static void gameLoop_task(void *pvParameters)
{
    while (1)
    {
        int64_t frame_start = esp_timer_get_time();

        Sample_Joystick();
        Network_Apply_Movement();

        Bomb_Move();
        Gumby_Move();
        Tortle_Move();
        DMA_DrawMap();
        Bomb_Render();
        Gumby_Render();
        Tortle_Render();
        RenderPlayer();
        RenderOtherPlayer();

        DMA_BlastBuffer();

        int64_t frame_end = esp_timer_get_time();
        int64_t frame_time = frame_end - frame_start;

        if (frame_time < FRAME_TIME_US)
        {
            int64_t sleep_us = FRAME_TIME_US - frame_time;
            esp_rom_delay_us((uint32_t)sleep_us);
        }

        int64_t total_frame_time =
            esp_timer_get_time() - frame_start;

        float fps = 1000000.0f / total_frame_time;

        // ESP_LOGI(DISPLAY_TAG,
        //          "Frame: %lld us | FPS: %.2f",
        //          total_frame_time,
        //          fps);
    }
}


void app_main(void)
{
    init_nvs();
    assign_player(0); //as default

    LoadLevel(LVL_ID_TESTLEVEL);
    Bomb_Initialize();
    Gumby_Initialize();
    Tortle_Initialize();
    SPI_Init();
    ili9341_init();
    ili9341_fill_screen(0xF000);
    
    DMA_Init();
    Button_Init();
    
    initialize_frameBuffer();

    //Brown 0x4752
    Player_SetPalette(0x4A00, 0x4752);
    
    ESP_LOGI(DISPLAY_TAG, "COMPLETE INIT");
    ADC_Init();

        if(isHost){
        init_host();  
    }
    else{
        init_client();

    }

    xTaskCreate(gameLoop_task, "game_loop", 4096, NULL, 5, NULL);
    
}