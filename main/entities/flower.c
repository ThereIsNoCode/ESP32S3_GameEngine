#include "flower.h"
#define FLOWER_POOL_SIZE 4
Entity flowerArr[FLOWER_POOL_SIZE];



//They can have their own seperate tint
uint16_t palette_flower[16];




void  Flower_SetPalette(){
    for(int i = 0; i < 16; i += 1){
        palette_flower[i] = tint_gray(i, 0x3F00);
    }
}
void Flower_Initialize(){
    for(int i = 0; i < FLOWER_POOL_SIZE; i++){
        flowerArr[i] = (Entity){
            .position_x    = 128 + 128*i,
            .position_y    = 96,
            .tint_color     = 0x1F00,
            .size          = 16,
            .id            = 0,
            // everything not listed is zero-initialized automatically
        };

    }


    Flower_SetPalette();
}

 
void Flower_Move(){
    Entity player = *player_Current;

    for(int i = 0; i < FLOWER_POOL_SIZE; i++){
       
        if(flowerArr[i].state & IS_INACTIVE) { continue; }
       
        uint8_t currentCollisionInfo = flowerArr[i].collisionSide;
        flowerArr[i].collisionSide = 0;

        flowerArr[i].force_x = 0;
        flowerArr[i].force_y = 0;





        for(int j = 0; j < 2; j++){
            uint8_t collisionPlayerInfo = Entity_Collide_Entity(&flowerArr[i], &playerArr[j]);

            if(collisionPlayerInfo != 0) { 
                playerDataArr[j].ability = 1; //Make this an ENUM
                flowerArr[i].state |= IS_INACTIVE;
            }
             
        }

        Physics_AddGravity(85, &(flowerArr[i].velocity_y), &flowerArr[i].force_y);
        
       


        
        Physics_getAcceleration(flowerArr[i].force_y, &flowerArr[i].accel_y);
        flowerArr[i].velocity_y += flowerArr[i].accel_y;
        
        flowerArr[i].position_y += flowerArr[i].velocity_y;

        Entity_CollideY_Tile(&flowerArr[i]);



        //Not needed for abilities
        //flowerArr[i].position_x += tortleArr[i].velocity_x;
        Entity_WrapX(&flowerArr[i]);
        Entity_CollideX_Tile(&flowerArr[i]);



    }
}


void Flower_Render()
{


    int MAP_PIXEL_WIDTH = lvl_width * TILE_SIZE;   // full map width in RENDER-space pixels
    int MAP_PIXEL_WIDTH_HALF = MAP_PIXEL_WIDTH/2;   // full map width in RENDER-space pixels
    int32_t playerX = player_Current->position_x >> 2;
    for(int i = 0; i < FLOWER_POOL_SIZE; i++){
        if(flowerArr[i].state & IS_INACTIVE) { continue; }


        // Same camera calculation as DMA_DrawMap, so bomb draws in the correct
        // screen position relative to the scrolling world (not just a fixed X)
        int cameraX = (playerX) - (SCREEN_WIDTH_HALF);

        
        int entityX = flowerArr[i].position_x >> 2;

        int dx = entityX - playerX;

        if (dx > MAP_PIXEL_WIDTH_HALF)
            dx -= MAP_PIXEL_WIDTH;

        if (dx < -MAP_PIXEL_WIDTH_HALF)
            dx += MAP_PIXEL_WIDTH;

        int convertedScreenPosX = dx + SCREEN_WIDTH_HALF;
        int convertedScreenPosY = (flowerArr[i].position_y >> 2);


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
                uint8_t pixelByte = tile_flower[(y << 2) + x];
                uint8_t shade1 = pixelByte >> 4;
                uint8_t shade2 = pixelByte & 0x0F;
                int dstX = convertedScreenPosX + (x << 2);   // signed, not uint32_t

                for(int px = 0; px < 4; px++){
                    int sx = dstX + px;
                    if (sx < 0 || sx >= SCREEN_WIDTH) continue;   // clip — no smear
                    uint8_t shade = (px < 2) ? shade1 : shade2;
                    uint8_t pixel = palette_flower[shade];
                    if(shade != 0x0){
                        if(topValid)    framebuffer[sx + row0] = pixel;
                        if(bottomValid) framebuffer[sx + row1] = pixel;
                    }
                }
            }
        }
    }
}

