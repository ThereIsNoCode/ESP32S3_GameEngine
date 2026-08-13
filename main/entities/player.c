#include "player.h"

Entity playerArr[] = {
    {
        128,  //PosX
        140<<2,  //PosY
        0,  //VelX
        0,  //VelY
        1,  //AccelX
        1,  //AccelY
        0,  //force x
        0, //force y
        0x3F00,  //Tint Color
        0, //COllision Side
        16, //size
        0,
        0,0,0   //ID
    },
    {
        128,  //PosX
        140<<2,  //PosY
        0,  //VelX
        0,  //VelY
        1,  //AccelX
        1,  //AccelY
        0,  //force x
        0, //force y
        0x3F00,  //Tint Color
        0, //COllision Side
        16, //size
        0,
        0,   //ID
        0,0
    }

}; 

Player playerDataArr[] = {
    {
        0,
        0,
    },
    {
        0,
        0,
    }

}; 

Entity *player_Current; 
int joyStick_X;
int joyStick_Y;

#define PLAYER_MAX_SPEED (4 << 2)

#define PLAYER_MAX_SPEED_WALK     (4 << 2)
#define PLAYER_MAX_SPEED_WALLJUMP (8 << 2)

//They can have their own seperate tint
uint16_t palette_player1[16];
uint16_t palette_player2[16];

void assign_player(uint8_t id){
     ESP_LOGI(DISPLAY_TAG, "SETTING PLAYER TO CORRECT ID: %u" , id);
    playerArr[0].id = 0;
     playerArr[1].id = 1;
    player_Current = &playerArr[id];
}

void  Player_SetPalette(uint16_t player1_tint, uint16_t player2_tint){
    // player_Current = &playerArr[0];
    for(int i = 0; i < 16; i += 1){
        palette_player1[i] = tint_gray(i, player1_tint);
        palette_player2[i] = tint_gray(i, player2_tint);
    }
}



//Fixed-Point Units for everything in player.
// void Player_Move(){

//     for(int i = 0; i < 2; i++){
        

//         if(playerArr[i].id == player_Current->id){
//             joyStick_X = Read_Joystick_X();
//             joyStick_Y = Read_Joystick_Y();
//         }
//         else{
//             //get input from user and handle if host
//             //in the case of another client, they won't get any input packets, they will just get snapshot of all player positions, so they don't need to predict where players would be

//         }
//         uint8_t currentCollisionInfo = player_Current->collisionSide;
//         player_Current->collisionSide = 0;
//         player_Current->force_x = 0;
//         player_Current->force_y = 0;
        
//         int32_t maxSpeedX = PLAYER_MAX_SPEED_WALK;   // default every frame, overridden below when needed


//         //Horizontal Movmeent
//         if(joyStick_X<0){
//             player_Current->force_x -= 2;
//             player_Current->state |= FACE_LEFT;
//         }
//         else if(joyStick_X>0){
//             player_Current->force_x += 2;
//             player_Current->state &= ~FACE_LEFT;
//         }
//         else{
//             switch(currentCollisionInfo){
//                 case 0:
//                     Physics_AddFriction(99, &(player_Current->velocity_x), &player_Current->force_x);
//                     break;
//                 case COLLIDE_BOTTOM:
//                     Physics_AddFriction(90, &(player_Current->velocity_x), &player_Current->force_x);
//                     break;
//             }
//         }

//         //Vertical Movement
        
//         uint8_t wallSlidingCond = (INPUT_LEFT | INPUT_RIGHT) && (currentCollisionInfo & COLLIDE_LEFT) | (currentCollisionInfo & COLLIDE_RIGHT );



//         //Wall-Jump Override Logic:
//         if(INPUT_LEFT && currentCollisionInfo & COLLIDE_LEFT && INPUT_JUMP){
//             player_Current->velocity_x = 0;
//             player_Current->velocity_y = 0;
//             player_Current->force_x = 50;
//             player_Current->force_y = -40;
//             maxSpeedX = PLAYER_MAX_SPEED_WALLJUMP;
//         }
//         else if(INPUT_RIGHT && currentCollisionInfo & COLLIDE_RIGHT && INPUT_JUMP){
//             player_Current->velocity_x = 0;
//             player_Current->velocity_y = 0;
//             player_Current->force_x = -50;
//             player_Current->force_y = -40;
//             maxSpeedX = PLAYER_MAX_SPEED_WALLJUMP;
//         }else{

//             if(INPUT_JUMP && (currentCollisionInfo & COLLIDE_BOTTOM) && startJump == 0) {
//                 player_Current->force_y = -30;
//                 startJump = 1;
//             }
//             else if(startJump == 1 && jumpTIme < 6 && INPUT_JUMP){
//                 player_Current->force_y = -4;
//                 jumpTIme += 1;
//             } else{
//                 startJump = 0;
//                 jumpTIme = 0;
//                 if(wallSlidingCond && player_Current->velocity_y > 0){
//                     player_Current->force_y = -3;
//                 }
//             }
            
//         }
                                        
//         Physics_AddGravity(85, &(player_Current->velocity_y), &player_Current->force_y);
        
//         Physics_ApplyForceWithCap(&player_Current->velocity_x, &player_Current->force_x, maxSpeedX);
        
//         Physics_getAcceleration(player_Current->force_x, &player_Current->accel_x);
//         player_Current->velocity_x += player_Current->accel_x;
        
//         //Clamping velocity logic
//         Physics_EaseTowardSpeed(&player_Current->velocity_x, maxSpeedX, 10);
//         player_Current->position_x += player_Current->velocity_x;
//         Entity_WrapX(player_Current);
//         Entity_CollideX_Tile(player_Current);


        
//         Physics_getAcceleration(player_Current->force_y, &player_Current->accel_y);
//         player_Current->velocity_y += player_Current->accel_y;
        
//         player_Current->position_y += player_Current->velocity_y;
    
//         Entity_CollideY_Tile(player_Current);

//     }
// }

void Player_DirectSetPosition(packet_clientPosition_t *input){
    //ESP_LOGI(DISPLAY_TAG, "CURRENT NEW POS: %s", input->position_X);
    playerArr[input->playerId].position_x = input->position_X;
    playerArr[input->playerId].position_y = input->position_Y;
}
void Player_InterpolateRemote(packet_serverSnapshot_t input){
    for (int i = 0; i < 2; i++) {
        //if (i == player_Current->id) continue;   // don't interpolate my own player, I control it directly

        Entity *e = &playerArr[i];
        // Move 1/4 of the remaining distance each frame
        //One thing to consider: Store Network Pos, and Render Pos: where render pos is the interpolated data and network Pos ensures collisions don't occur weirdly 
        e->position_x += (input.players[i].position_x - e->position_x) / 2;
        e->position_y += (input.players[i].position_y - e->position_y) / 2;
    }
}
void Player_Apply_Movement(packet_clientInput_t *input){

    uint8_t isMovingLeft = (input->axis_X < 0);
    uint8_t isMovingRight = (input->axis_X > 0);

    uint8_t currentCollisionInfo = playerArr[input->playerId].collisionSide;
    playerArr[input->playerId].collisionSide = 0;
    playerArr[input->playerId].force_x = 0;
    playerArr[input->playerId].force_y = 0;
    
    int32_t maxSpeedX = PLAYER_MAX_SPEED_WALK;   // default every frame, overridden below when needed


    //Horizontal Movmeent
    if(input->axis_X<0){
        playerArr[input->playerId].force_x -= 2;
        playerArr[input->playerId].state |= FACE_LEFT;
    }
    else if(input->axis_X>0){
        playerArr[input->playerId].force_x += 2;
        playerArr[input->playerId].state &= ~FACE_LEFT;
    }
    else{
        switch(currentCollisionInfo){
            case 0:
                Physics_AddFriction(99, &(playerArr[input->playerId].velocity_x), &playerArr[input->playerId].force_x);
                break;
            case COLLIDE_BOTTOM:
                Physics_AddFriction(90, &(playerArr[input->playerId].velocity_x), &playerArr[input->playerId].force_x);
                break;
        }
    }

    //Vertical Movement
    
    uint8_t wallSlidingCond = (isMovingLeft | isMovingRight) && (currentCollisionInfo & COLLIDE_LEFT) | (currentCollisionInfo & COLLIDE_RIGHT );



    //Wall-Jump Override Logic:
    if(isMovingLeft && currentCollisionInfo & COLLIDE_LEFT && NET_INPUT_PRESSED(input->buttons, NET_BTN_JUMP)){
        playerArr[input->playerId].velocity_x = 0;
        playerArr[input->playerId].velocity_y = 0;
        playerArr[input->playerId].force_x = 50;
        playerArr[input->playerId].force_y = -40;
        ESP_LOGI(DISPLAY_TAG, "WALL JUMPP");
        maxSpeedX = PLAYER_MAX_SPEED_WALLJUMP;
    }
    else if(isMovingRight && currentCollisionInfo & COLLIDE_RIGHT && NET_INPUT_PRESSED(input->buttons, NET_BTN_JUMP)){
        playerArr[input->playerId].velocity_x = 0;
        playerArr[input->playerId].velocity_y = 0;
        playerArr[input->playerId].force_x = -50;
        playerArr[input->playerId].force_y = -40;
        ESP_LOGI(DISPLAY_TAG, "WALL JUMPP");
        maxSpeedX = PLAYER_MAX_SPEED_WALLJUMP;
    }else{

        if(NET_INPUT_PRESSED(input->buttons, NET_BTN_JUMP) && (currentCollisionInfo & COLLIDE_BOTTOM) && playerDataArr[input->playerId].startJump == 0) {
            playerArr[input->playerId].force_y = -30;
            playerDataArr[input->playerId].startJump = 1;
        }
        else if(playerDataArr[input->playerId].startJump == 1 && playerDataArr[input->playerId].jumpTime < 6 && NET_INPUT_PRESSED(input->buttons, NET_BTN_JUMP)){
            playerArr[input->playerId].force_y = -4;
            playerDataArr[input->playerId].jumpTime += 1;
        } else{
            playerDataArr[input->playerId].startJump = 0;
            playerDataArr[input->playerId].jumpTime = 0;
            if(wallSlidingCond && playerArr[input->playerId].velocity_y > 0){
                playerArr[input->playerId].force_y = -3;
            }
        }
        
    }
                                    
    Physics_AddGravity(85, &(playerArr[input->playerId].velocity_y), &playerArr[input->playerId].force_y);
    
    Physics_ApplyForceWithCap(&playerArr[input->playerId].velocity_x, &playerArr[input->playerId].force_x, maxSpeedX);
    
    Physics_getAcceleration(playerArr[input->playerId].force_x, &playerArr[input->playerId].accel_x);
    playerArr[input->playerId].velocity_x += playerArr[input->playerId].accel_x;
    
    //Clamping velocity logic
    Physics_EaseTowardSpeed(&playerArr[input->playerId].velocity_x, maxSpeedX, 10);
    playerArr[input->playerId].position_x += playerArr[input->playerId].velocity_x;
    Entity_WrapX(&playerArr[input->playerId]);
    Entity_CollideX_Tile(&playerArr[input->playerId]);


    
    Physics_getAcceleration(playerArr[input->playerId].force_y, &playerArr[input->playerId].accel_y);
    playerArr[input->playerId].velocity_y += playerArr[input->playerId].accel_y;
    
    playerArr[input->playerId].position_y += playerArr[input->playerId].velocity_y;

    Entity_CollideY_Tile(&playerArr[input->playerId]);

    
}


void RenderOtherPlayer()
{
    // Find the player that is NOT the camera player
    Entity *other = NULL;
    for (int i = 0; i < 2; i++) {
        if (&playerArr[i] != player_Current) {
            other = &playerArr[i];
            break;
        }
    }
    if (other == NULL) return;   // no other player, nothing to draw

    int MAP_PIXEL_WIDTH      = lvl_width * TILE_SIZE;
    int MAP_PIXEL_WIDTH_HALF = MAP_PIXEL_WIDTH / 2;
    int32_t playerX = player_Current->position_x >> 2;   // camera follows current player

    // Camera-relative X, same wraparound math as the bomb
    int otherX = other->position_x >> 2;
    int dx = otherX - playerX;
    if (dx >  MAP_PIXEL_WIDTH_HALF) dx -= MAP_PIXEL_WIDTH;
    if (dx < -MAP_PIXEL_WIDTH_HALF) dx += MAP_PIXEL_WIDTH;

    int convertedScreenPosX = dx + SCREEN_WIDTH_HALF;
    int convertedScreenPosY = other->position_y >> 2;

    // Palette + sprite from the OTHER player's own state (not local input)
    uint16_t *palette = (other->id == 0) ? palette_player1 : palette_player2;

    const uint8_t *playerSprite;
    if(((other->velocity_x<0 )| (other->velocity_x>0)) && ((player_Current->collisionSide & (COLLIDE_LEFT|COLLIDE_RIGHT))) ){
        playerSprite = tile_player_wall;
    }
    else if(other->velocity_y < 0){
        playerSprite = tile_player_rise;
    }
    else if(other->velocity_y > 0){
        playerSprite = tile_player_fall;
    } 
    else{
        playerSprite = tile_player;
    }

    bool flip = (other->state & FACE_LEFT);

    for (int y = 0; y < 8; y++) {
        int lineTop    = convertedScreenPosY + (y * 2);
        int lineBottom = lineTop + 1;

        uint8_t topValid    = (lineTop    >= 0) && (lineTop    < SCREEN_HEIGHT);
        uint8_t bottomValid = (lineBottom >= 0) && (lineBottom < SCREEN_HEIGHT);
        if (!topValid && !bottomValid) continue;

        uint32_t row0 = (uint32_t)lineTop * SCREEN_WIDTH;
        uint32_t row1 = row0 + SCREEN_WIDTH;

        for (int x = 0; x < 4; x++) {
            uint8_t pixelByte = playerSprite[(y << 2) + x];
            uint8_t shade1 = pixelByte >> 4;
            uint8_t shade2 = pixelByte & 0x0F;

            // The player render draws each shade as a 2-pixel-wide group (4 px per byte).
            // Handle horizontal flip by swapping which shade goes left vs right.
            uint8_t leftShade  = flip ? shade2 : shade1;
            uint8_t rightShade = flip ? shade1 : shade2;

            // Flip also reverses the group order across the 16px sprite
            int groupX = flip ? convertedScreenPosX + (16 - 4) - (x << 2)
                              : convertedScreenPosX + (x << 2);

            for (int px = 0; px < 4; px++) {
                int sx = groupX + px;
                if (sx < 0 || sx >= SCREEN_WIDTH) continue;   // clip, no smear

                uint8_t shade = (px < 2) ? leftShade : rightShade;
                if (shade == 0x0) continue;                   // transparent

                uint16_t pixel = palette[shade];
                if (topValid)    framebuffer[sx + row0] = pixel;
                if (bottomValid) framebuffer[sx + row1] = pixel;
            }
        }
    }
}




void RenderPlayer(){

    uint16_t *palette;
    if(player_Current->id == 0){
        //ESP_LOGI(DISPLAY_TAG, "PALLETTEE FOR PLAYER 1");
        palette = palette_player1;
    }else{
        //ESP_LOGI(DISPLAY_TAG, "PALLETTEE FOR PLAYER 2");
        palette = palette_player2;
    }
    int convertedScreenPosX = (player_Current->position_x >> 2);
    int convertedScreenPosY = (player_Current->position_y >> 2);
    
    bool flip = (player_Current->state & FACE_LEFT);     // constant for the whole sprite
    
    const uint8_t* playerSprite;
    //ESP_LOGI(DISPLAY_TAG, "COLLIDE_RIGHT: %u", player_Current->collisionSide & COLLIDE_RIGHT);
    //ESP_LOGI(DISPLAY_TAG, "COLLIDE_RIGHT: %u", player_Current->collisionSide & COLLIDE_LEFT);
    if((INPUT_LEFT | INPUT_RIGHT) && ((player_Current->collisionSide & (COLLIDE_LEFT|COLLIDE_RIGHT))) ){
        playerSprite = tile_player_wall;
    }
    else if(player_Current->velocity_y < 0){
        playerSprite = tile_player_rise;
    }
    else if(player_Current->velocity_y > 0){
        playerSprite = tile_player_fall;
    } 
    else{
        playerSprite = tile_player;
    }

    for(int y = 0; y < 8; y++){
        int lineTop    = convertedScreenPosY + (y * 2);
        int lineBottom = lineTop + 1;

        bool topValid    = (lineTop    >= 0) && (lineTop    < SCREEN_HEIGHT);
        bool bottomValid = (lineBottom >= 0) && (lineBottom < SCREEN_HEIGHT);

        if(!topValid && !bottomValid){
            continue; // this whole scanline pair is off-screen, skip it
        }

        uint32_t row0 = (uint32_t)lineTop * SCREEN_WIDTH;
        uint32_t row1 = row0 + SCREEN_WIDTH;

        for(int x = 0; x < 4; x++){
            uint8_t pixelByte = playerSprite[(y << 2) + x];
            uint8_t shade1 = pixelByte >> 4;
            uint8_t shade2 = pixelByte & 0x0F;

            uint16_t pixel1;
            uint16_t pixel2;
            
            uint32_t dstX = 160 + (x << 2);
            
            if(player_Current->state&FACE_LEFT){
                dstX = 160 + (16 - 4) - (x << 2);    // groups reverse
                pixel1 = palette[shade2];
                pixel2 = palette[shade1];
            } else {
                dstX = 160 + (x << 2);
                pixel1 = palette[shade1];
                pixel2 = palette[shade2];
            }


            if(topValid){
                if(pixel1 != 0x0){
                    framebuffer[dstX + 0 + row0] = pixel1;
                    framebuffer[dstX + 1 + row0] = pixel1;
                }
                if(pixel2 != 0x0){
                    framebuffer[dstX + 2 + row0] = pixel2;
                    framebuffer[dstX + 3 + row0] = pixel2;
                }
            }
            if(bottomValid){
                if(pixel1 != 0x0){
                    framebuffer[dstX + 0 + row1] = pixel1;
                    framebuffer[dstX + 1 + row1] = pixel1;
                }
                if(pixel2 != 0x0){
                    framebuffer[dstX + 2 + row1] = pixel2;
                    framebuffer[dstX + 3 + row1] = pixel2;   
                }
            }
        }
    }
}
