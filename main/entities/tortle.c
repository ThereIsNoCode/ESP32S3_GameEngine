#include "bomb.h"
#include "player.h"
#include "tortle.h"
#include "../tiles/tiles.h"
#include "../../maps/level_manager.h"

#define TORTE_POOL_SIZE 1
Entity tortleArr[TORTE_POOL_SIZE];
Tortle tortleDataArr[TORTE_POOL_SIZE];

uint16_t palette_tortle[16];

#define TORTLE_MAX_SPEED 25
#define TORTLE_SPEED 5

void Tortle_SetPalette(){
    for(int i = 0; i < 16; i += 1){
        palette_tortle[i] = tint_gray(i, 0x10F0);
    }
}

//Creates Object Pool of bombs
void Tortle_Initialize(){
    for(int i = 0; i < TORTE_POOL_SIZE; i++){
        tortleArr[i] = (Entity){
            .position_x    = 128 + 128*i,
            .position_y    = 96,
            .tint_color     = 0x1F00,
            .size          = 16,
            .id            = 0,
            .velocity_x = 5,
            // everything not listed is zero-initialized automatically
        };
        tortleDataArr[i] = (Tortle){
            .status = 0,
            .ignoreEntityId = -1,
        };

    }


    Tortle_SetPalette();
}



void Tortle_Move(){
    Entity player = *player_Current;

    for(int i = 0; i < TORTE_POOL_SIZE; i++){
       
        if(tortleArr[i].state & IS_INACTIVE) { continue; }
       
        uint8_t currentCollisionInfo = tortleArr[i].collisionSide;
        tortleArr[i].collisionSide = 0;

        tortleArr[i].force_x = 0;
        tortleArr[i].force_y = 0;


        int offset = (player_Current->state&FACE_LEFT)*32;



        for(int j = 0; j < 2; j++){
            uint8_t collisionPlayerInfo = Entity_Collide_Entity(&tortleArr[i], &playerArr[j]);


            if((tortleDataArr[i].isGrabbed && INPUT_RUN && tortleDataArr[i].ignoreEntityId == j)){
                tortleArr[i].position_x = playerArr[j].position_x+(32)-offset;
                tortleArr[i].position_y = playerArr[j].position_y-32;
                break;
            } else if(collisionPlayerInfo != 0 && INPUT_RUN && tortleDataArr[i].isGrabbed == false && tortleDataArr[i].status & TORTLE_STATUS_SHELL && !(tortleDataArr[i].status & TORTLE_STATUS_SLIDING)){
                tortleDataArr[i].isGrabbed = true;
                tortleDataArr[i].ignoreEntityId = j;
                break;
            }
            if(tortleDataArr[i].isGrabbed == true  && tortleDataArr[i].ignoreEntityId == j){ //WHen syncing with network, ensure only throws if holder is no longer holding
                
                tortleArr[i].velocity_x = 0;
                tortleArr[i].velocity_y = 0;
                tortleArr[i].force_x += (joystick_X/24);
                tortleArr[i].force_y += (joystick_Y/24);
                if(playerArr[j].state & FACE_LEFT){
                    ESP_LOGI(DISPLAY_TAG, "Hey, thrown tortle LEFT");
                    tortleArr[i].velocity_x = -TORTLE_MAX_SPEED;
                } else{
                    tortleArr[i].velocity_x = TORTLE_MAX_SPEED;
                    ESP_LOGI(DISPLAY_TAG, "Hey, thrown tortle Right");
                }
                tortleDataArr[i].status |= TORTLE_STATUS_SLIDING;
                tortleDataArr[i].isGrabbed = false;
                
            }

            if(tortleDataArr[i].ignoreEntityId == j){
                if(!Entity_Collide_Entity(&tortleArr[i], &playerArr[j])){
                    tortleDataArr[i].ignoreEntityId = -1;
                }
              
            }
            else {
                if(collisionPlayerInfo & COLLIDE_TOP){
                    playerArr[j].velocity_y = -15;
                    playerDataArr[j].startJump = 0;
                    playerDataArr[j].jumpTime = 0;
                    playerArr[j].collisionSide |= COLLIDE_BOTTOM;
                    
                    if(tortleDataArr[i].status & TORTLE_STATUS_SLIDING){
                        tortleDataArr[i].status &= ~TORTLE_STATUS_SLIDING;
                        tortleDataArr[i].status |= TORTLE_STATUS_IS_GRABBABLE;
                    }else if(tortleDataArr[i].status & TORTLE_STATUS_SHELL) {
                        tortleDataArr[i].status |= TORTLE_STATUS_SLIDING;
                        tortleDataArr[i].status &= ~TORTLE_STATUS_IS_GRABBABLE;
                        tortleArr[i].velocity_x = TORTLE_MAX_SPEED; 
                    }
                    else{
                        tortleDataArr[i].status |= TORTLE_STATUS_SHELL;
                        tortleDataArr[i].status |= TORTLE_STATUS_IS_GRABBABLE;
                    }

                
                }
                else if((collisionPlayerInfo & COLLIDE_RIGHT) | (collisionPlayerInfo & COLLIDE_LEFT)){
                    //if sliding or not in shell, kill player
                    //otherwise, its in shell mode not moving... player can push it in direction
                    if((tortleDataArr[i].status & TORTLE_STATUS_SHELL) == 0 || tortleDataArr[i].status & TORTLE_STATUS_SLIDING){
                        playerArr[j].position_x = 32;
                        playerArr[j].position_y = 92;
                    }
                    else{
                        if (collisionPlayerInfo & COLLIDE_RIGHT) {
                            tortleArr[i].velocity_x = -TORTLE_MAX_SPEED; 
                        }
                        else {
                            tortleArr[i].velocity_x = TORTLE_MAX_SPEED; 
                        }
                        tortleDataArr[i].status |= TORTLE_STATUS_SLIDING;
                    }
                }
            }   
        }
        if(tortleDataArr[i].isGrabbed){ continue;} 

        Physics_AddGravity(85, &(tortleArr[i].velocity_y), &tortleArr[i].force_y);
        
        if((tortleDataArr[i].status & TORTLE_STATUS_SHELL) == 0){
            if(currentCollisionInfo & COLLIDE_LEFT){
                //ESP_LOGI(DISPLAY_TAG, "BOUNCE To RIGHT");
                tortleArr[i].velocity_x = TORTLE_SPEED;
            
            } else if(currentCollisionInfo & COLLIDE_RIGHT){
                //ESP_LOGI(DISPLAY_TAG, "BOUNCE To LEFT");
                tortleArr[i].velocity_x = -TORTLE_SPEED;

            }
        } else if ((tortleDataArr[i].status & TORTLE_STATUS_SLIDING) == 0){
            ESP_LOGI(DISPLAY_TAG, "Hey, We resettin velocity :)");
            tortleArr[i].velocity_x = 0;
        } else if ((tortleDataArr[i].status & TORTLE_STATUS_SLIDING)){
            if(currentCollisionInfo & COLLIDE_LEFT){
                //ESP_LOGI(DISPLAY_TAG, "BOUNCE To RIGHT");
                tortleArr[i].velocity_x = TORTLE_MAX_SPEED;
            
            } else if(currentCollisionInfo & COLLIDE_RIGHT){
                //ESP_LOGI(DISPLAY_TAG, "BOUNCE To LEFT");
                tortleArr[i].velocity_x = -TORTLE_MAX_SPEED;

            }
        }


        
        Physics_getAcceleration(tortleArr[i].force_y, &tortleArr[i].accel_y);
        tortleArr[i].velocity_y += tortleArr[i].accel_y;
        
        tortleArr[i].position_y += tortleArr[i].velocity_y;

        Entity_CollideY_Tile(&tortleArr[i]);



        tortleArr[i].position_x += tortleArr[i].velocity_x;
        Entity_WrapX(&tortleArr[i]);
        Entity_CollideX_Tile(&tortleArr[i]);



    }
}



void Tortle_Render()
{


    int MAP_PIXEL_WIDTH = lvl_width * TILE_SIZE;   // full map width in RENDER-space pixels
    int MAP_PIXEL_WIDTH_HALF = MAP_PIXEL_WIDTH/2;   // full map width in RENDER-space pixels
    int32_t playerX = player_Current->position_x >> 2;
    for(int i = 0; i < TORTE_POOL_SIZE; i++){
        if(tortleArr[i].state & IS_INACTIVE) { continue; }


        const uint8_t* tortleSprite;
        if(tortleDataArr[i].status & TORTLE_STATUS_SHELL){
            tortleSprite = tile_tortle_shell;
        }
        else {
            tortleSprite = tile_tortle;
        }

        // Same camera calculation as DMA_DrawMap, so bomb draws in the correct
        // screen position relative to the scrolling world (not just a fixed X)
        int cameraX = (playerX) - (SCREEN_WIDTH_HALF);

        
        int entityX = tortleArr[i].position_x >> 2;

        int dx = entityX - playerX;

        if (dx > MAP_PIXEL_WIDTH_HALF)
            dx -= MAP_PIXEL_WIDTH;

        if (dx < -MAP_PIXEL_WIDTH_HALF)
            dx += MAP_PIXEL_WIDTH;

        int convertedScreenPosX = dx + SCREEN_WIDTH_HALF;
        int convertedScreenPosY = (tortleArr[i].position_y >> 2);


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
                uint8_t pixelByte = tortleSprite[(y << 2) + x];
                uint8_t shade1 = pixelByte >> 4;
                uint8_t shade2 = pixelByte & 0x0F;
                int dstX = convertedScreenPosX + (x << 2);   // signed, not uint32_t

                for(int px = 0; px < 4; px++){
                    int sx = dstX + px;
                    if (sx < 0 || sx >= SCREEN_WIDTH) continue;   // clip — no smear
                    uint8_t shade = (px < 2) ? shade1 : shade2;
                    uint8_t pixel = palette_tortle[shade];
                    if(shade != 0x0){
                        if(topValid)    framebuffer[sx + row0] = pixel;
                        if(bottomValid) framebuffer[sx + row1] = pixel;
                    }
                }
            }
        }
    }
}

