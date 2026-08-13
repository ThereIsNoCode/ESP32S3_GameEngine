#include "bomb.h"
#include "player.h"
#include "gumby.h"
#include "../tiles/tiles.h"
#include "../../maps/level_manager.h"

#define GUMBY_POOL_SIZE 1
Entity gumbyArr[GUMBY_POOL_SIZE];
Gumby gumbyDataArr[GUMBY_POOL_SIZE];

uint16_t palette_gumby[16];



void Gumby_SetPalette(){
    for(int i = 0; i < 16; i += 1){
        palette_gumby[i] = tint_gray(i, 0x3F00);
    }
}

//Creates Object Pool of bombs
void Gumby_Initialize(){
    for(int i = 0; i < GUMBY_POOL_SIZE; i++){
        gumbyArr[i] = (Entity){
            .position_x    = 16 + 128*i,
            .position_y    = 96,
            .tint_color     = 0x1F00,
            .size          = 16,
            .id            = 0,
            .velocity_x = 5,
            // everything not listed is zero-initialized automatically
        };
        gumbyDataArr[i] = (Gumby){
            .isLeft = 0
        };

    }


    Gumby_SetPalette();
}



void Gumby_Move(){
    Entity player = *player_Current;
    int32_t player_position_x = player_Current->position_x;
    int32_t player_position_y = player_Current->position_y;
    for(int i = 0; i < GUMBY_POOL_SIZE; i++){
        uint8_t currentCollisionInfo = gumbyArr[i].collisionSide;
        gumbyArr[i].collisionSide = 0;

        gumbyArr[i].force_x = 0;
        gumbyArr[i].force_y = 0;


        for(int j = 0; j < 2; j++){
            // ESP_LOGI(DISPLAY_TAG, "PLayer %u, Pos X %d", i, playerArr[i].position_x);
            // ESP_LOGI(DISPLAY_TAG, "PLayer %u, Pos Y %d", i, playerArr[i].position_y);
            uint8_t collisionPlayerInfo = Entity_Collide_Entity(&gumbyArr[i], &playerArr[j]);
            if(collisionPlayerInfo & COLLIDE_TOP){
                playerArr[j].velocity_y = -15;
                playerDataArr[j].startJump = 0;
                playerDataArr[j].jumpTime = 0;
                playerArr[j].collisionSide |= COLLIDE_BOTTOM;
            }
            else if((collisionPlayerInfo & COLLIDE_RIGHT) | (collisionPlayerInfo & COLLIDE_LEFT)){
                playerArr[j].position_x = 32;
                playerArr[j].position_y = 92;
                ESP_LOGI(DISPLAY_TAG, "GUMBY LEFTRIGHT HIT");
            }
        }


        Physics_AddGravity(85, &(gumbyArr[i].velocity_y), &gumbyArr[i].force_y);
        

        if(currentCollisionInfo & COLLIDE_LEFT){
            //ESP_LOGI(DISPLAY_TAG, "BOUNCE To RIGHT");
            gumbyArr[i].velocity_x = 5;
            
        } else if(currentCollisionInfo & COLLIDE_RIGHT){
            //ESP_LOGI(DISPLAY_TAG, "BOUNCE To LEFT");
            gumbyArr[i].velocity_x = -5;

        }


        
        Physics_getAcceleration(gumbyArr[i].force_y, &gumbyArr[i].accel_y);
        gumbyArr[i].velocity_y += gumbyArr[i].accel_y;
        
        gumbyArr[i].position_y += gumbyArr[i].velocity_y;

        Entity_CollideY_Tile(&gumbyArr[i]);



        gumbyArr[i].position_x += gumbyArr[i].velocity_x;
        Entity_WrapX(&gumbyArr[i]);
        Entity_CollideX_Tile(&gumbyArr[i]);



    }
}



void Gumby_Render()
{
    int MAP_PIXEL_WIDTH = lvl_width * TILE_SIZE;   // full map width in RENDER-space pixels
    int MAP_PIXEL_WIDTH_HALF = MAP_PIXEL_WIDTH/2;   // full map width in RENDER-space pixels
    int32_t playerX = player_Current->position_x >> 2;
    for(int i = 0; i < GUMBY_POOL_SIZE; i++){
        // Same camera calculation as DMA_DrawMap, so bomb draws in the correct
        // screen position relative to the scrolling world (not just a fixed X)
        int cameraX = (playerX) - (SCREEN_WIDTH_HALF);

        
        int entityX   = gumbyArr[i].position_x >> 2;

        int dx = entityX - playerX;

        if (dx > MAP_PIXEL_WIDTH_HALF)
            dx -= MAP_PIXEL_WIDTH;

        if (dx < -MAP_PIXEL_WIDTH_HALF)
            dx += MAP_PIXEL_WIDTH;

        int convertedScreenPosX = dx + SCREEN_WIDTH_HALF;
        int convertedScreenPosY = (gumbyArr[i].position_y >> 2);


        for(int y = 0; y < 8; y++){
            int lineTop    = convertedScreenPosY + (y * 2);
            int lineBottom = lineTop + 1;

            uint8_t topValid    = (lineTop    >= 0) && (lineTop    < SCREEN_HEIGHT);
            uint8_t bottomValid = (lineBottom >= 0) && (lineBottom < SCREEN_HEIGHT);

            if(!topValid && !bottomValid){
                continue; // this whole scanline pair is off-screen, skip it
            }

            uint32_t row0 = (uint32_t)lineTop * SCREEN_WIDTH;
            uint32_t row1 = row0 + SCREEN_WIDTH;

            for(int x = 0; x < 4; x++){
                uint8_t pixelByte = tile_gumby[(y << 2) + x];
                uint8_t shade1 = pixelByte >> 4;
                uint8_t shade2 = pixelByte & 0x0F;
                int dstX = convertedScreenPosX + (x << 2);   // signed, not uint32_t

                for(int px = 0; px < 4; px++){
                    int sx = dstX + px;
                    if (sx < 0 || sx >= SCREEN_WIDTH) continue;   // clip — no smear
                    uint8_t shade = (px < 2) ? shade1 : shade2;
                    uint8_t pixel = palette_gumby[shade];
                    if(shade != 0x0){
                        if(topValid)    framebuffer[sx + row0] = pixel;
                        if(bottomValid) framebuffer[sx + row1] = pixel;
                    }
                }
            }
        }
    }
}

