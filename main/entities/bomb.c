#include "bomb.h"
#include "player.h"
#include "../tiles/tiles.h"
#include "../../maps/level_manager.h"

#define BOMB_POOL_SIZE 5
Entity bombArr[BOMB_POOL_SIZE];
Bomb bombDataArr[BOMB_POOL_SIZE];

uint16_t palette_bombNeutral[16];
uint16_t palette_bombFlash[16];



void Bomb_SetPalette(){
    for(int i = 0; i < 16; i += 1){
        palette_bombNeutral[i] = tint_gray(i, 0x3F00);
        palette_bombFlash[i] = tint_gray(i, 0xFF00);
    }
}

//Creates Object Pool of bombs
void Bomb_Initialize(){
    for(int i = 0; i < BOMB_POOL_SIZE; i++){
        bombArr[i] = (Entity){
            .position_x    = 16 + 128*i,
            .position_y    = 96,
            .tint_color     = 0xFF00,
            .size          = 14,
            .id            = 0,
            // everything not listed is zero-initialized automatically
        };

    }
    Bomb_SetPalette();
}



void Bomb_Move(){
    Entity player = *player_Current;
    int32_t player_position_x = player_Current->position_x;
    int32_t player_position_y = player_Current->position_y;
    for(int i = 0; i < BOMB_POOL_SIZE; i++){
        uint8_t currentCollisionInfo = bombArr[i].collisionSide;
        bombArr[i].collisionSide = 0;

        bombArr[i].force_x = 0;
        bombArr[i].force_y = 0;

        uint8_t collisionPlayerInfo = Entity_Collide_Entity(&bombArr[i], player_Current);

        int offset = (player_Current->state&FACE_LEFT)*32;

        if(bombDataArr[i].isGrabbed && INPUT_RUN){
            bombArr[i].position_x = player_position_x+(32)-offset;
            bombArr[i].position_y = player_position_y-32;
            bombDataArr[i].ignoreEntityId = 1; //SHould be ID of Entity, which all ID's start at 1 since 0 means nothing to ignore
            continue;
        } else if(collisionPlayerInfo != 0 && INPUT_RUN){
            bombDataArr[i].ignoreEntityId = 1;
            bombDataArr[i].isGrabbed = true;
            continue;
        }

        Physics_AddGravity(85, &(bombArr[i].velocity_y), &bombArr[i].force_y);
        


        for(int j = 0; j < BOMB_POOL_SIZE; j++){
            // if(j == i){
            //     continue;
            // }

            collisionPlayerInfo |= Entity_Collide_Entity(&bombArr[i], &bombArr[j]);

        }

        if(currentCollisionInfo & COLLIDE_LEFT){
            //ESP_LOGI(DISPLAY_TAG, "BOUNCE To RIGHT");
            bombArr[i].velocity_x = 0;
            
            bombArr[i].force_x += 10;
            bombArr[i].force_y += -10;
        } else if(currentCollisionInfo & COLLIDE_RIGHT){
            //ESP_LOGI(DISPLAY_TAG, "BOUNCE To LEFT");
            bombArr[i].velocity_x = 0;
            bombArr[i].force_x += -10;
            bombArr[i].force_y += -10;
        }
        
        
        if(bombDataArr[i].isGrabbed == true){
            bombArr[i].velocity_x = 0;
            bombArr[i].velocity_y = 0;
            bombArr[i].force_x += (joystick_X/24);
            bombArr[i].force_y += (joystick_Y/24);
            bombDataArr[i].isGrabbed = false;
            
        }
        if(bombDataArr[i].ignoreEntityId == 1){
            if(!Entity_Collide_Entity(&bombArr[i], player_Current)){
                bombDataArr[i].ignoreEntityId = 0;
            }

            
        } 
        else{
            if(collisionPlayerInfo & COLLIDE_LEFT){
                bombArr[i].velocity_x = 0;
                bombArr[i].force_x += 40;
                bombArr[i].force_y += -10;

            } else if (collisionPlayerInfo & COLLIDE_RIGHT){
                bombArr[i].velocity_x = 0;
                bombArr[i].force_x += -40;
                bombArr[i].force_y += -10;
            }
            else{
            switch(currentCollisionInfo){
                case 0:
                    Physics_AddFriction(95, &(bombArr[i].velocity_x), &bombArr[i].force_x);
                    break;
                case COLLIDE_BOTTOM:
                    Physics_AddFriction(90, &(bombArr[i].velocity_x), &bombArr[i].force_x);
                    break;
                
            }
        }
        }


        // bombDataArr[i].isGrabbed = false;
        

        // if(bombPushTimer > 0 && bombPushTimer < 5){
        //     bomb.force_x = pushDirForce;
        //     //bomb.force_y = -4;
        //     bombPushTimer += 1;
        // }
        // else{
        //     bombPushTimer = 0;
        // }



        
        Physics_getAcceleration(bombArr[i].force_y, &bombArr[i].accel_y);
        bombArr[i].velocity_y += bombArr[i].accel_y;
        
        bombArr[i].position_y += bombArr[i].velocity_y;

        Entity_CollideY_Tile(&bombArr[i]);

        Physics_getAcceleration(bombArr[i].force_x, &bombArr[i].accel_x);
        bombArr[i].velocity_x += bombArr[i].accel_x;

        //Physics_EaseTowardSpeed(&bomb.velocity_x, 0, 10);

        bombArr[i].position_x += bombArr[i].velocity_x;
        Entity_WrapX(&bombArr[i]);
        Entity_CollideX_Tile(&bombArr[i]);


        //Entity_CollideY_Entity(&bomb, &player);
    }
}



void Bomb_Render()
{
    int MAP_PIXEL_WIDTH = lvl_width * TILE_SIZE;   // full map width in RENDER-space pixels
    int MAP_PIXEL_WIDTH_HALF = MAP_PIXEL_WIDTH/2;   // full map width in RENDER-space pixels
    int32_t playerX = player_Current->position_x >> 2;
    for(int i = 0; i < BOMB_POOL_SIZE; i++){
        // Same camera calculation as DMA_DrawMap, so bomb draws in the correct
        // screen position relative to the scrolling world (not just a fixed X)
        int cameraX = (playerX) - (SCREEN_WIDTH_HALF);

        
        int bombX   = bombArr[i].position_x >> 2;

        int dx = bombX - playerX;

        if (dx > MAP_PIXEL_WIDTH_HALF)
            dx -= MAP_PIXEL_WIDTH;

        if (dx < -MAP_PIXEL_WIDTH_HALF)
            dx += MAP_PIXEL_WIDTH;

        int convertedScreenPosX = dx + SCREEN_WIDTH_HALF;
        int convertedScreenPosY = (bombArr[i].position_y >> 2);


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
                uint8_t pixelByte = tile_bomb[(y << 2) + x];
                uint8_t shade1 = pixelByte >> 4;
                uint8_t shade2 = pixelByte & 0x0F;
                int dstX = convertedScreenPosX + (x << 2);   // signed, not uint32_t

                for(int px = 0; px < 4; px++){
                    int sx = dstX + px;
                    if (sx < 0 || sx >= SCREEN_WIDTH) continue;   // clip — no smear
                    uint8_t shade = (px < 2) ? shade1 : shade2;
                    uint8_t pixel = palette_bombNeutral[shade];
                    if(shade != 0x0){
                        if(topValid)    framebuffer[sx + row0] = pixel;
                        if(bottomValid) framebuffer[sx + row1] = pixel;
                    }
                }
            }
        }
    }
}

