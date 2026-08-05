#include "player.h"


// const Entity playerArr[] = {
//  {
//     160,  //PosX
//     120,  //PosY
//     0,  //VelX
//     0,  //VelY
//     1,  //AccelX
//     1,  //AccelY
//     0x00FF0000,  //Tint Color
//     0, //COllision Side
//     16, //SIZE
//     0   //ID
// }

// }; 



Entity player = {
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
    0   //ID
};
uint8_t startJump = 0;
uint8_t jumpTIme = 0;


#define PLAYER_MAX_SPEED (4 << 2)

#define PLAYER_MAX_SPEED_WALK     (4 << 2)
#define PLAYER_MAX_SPEED_WALLJUMP (8 << 2)

//They can have their own seperate tint
uint16_t palette_player1[16];
uint16_t palette_player2[16];

void Player_SetPalette(uint16_t player1_tint, uint16_t player2_tint){
    for(int i = 0; i < 16; i += 1){
        palette_player1[i] = tint_gray(i, player1_tint);
        palette_player2[i] = tint_gray(i, player2_tint);
    }
}

int joyStick_X = 0;
int joyStick_Y = 0;

//Fixed-Point Units for everything in player.
void Player_Move(){

    joyStick_X = Read_Joystick_X();
    joyStick_Y = Read_Joystick_Y();

    uint8_t currentCollisionInfo = player.collisionSide;
    player.collisionSide = 0;
    player.force_x = 0;
    player.force_y = 0;
    
    int32_t maxSpeedX = PLAYER_MAX_SPEED_WALK;   // default every frame, overridden below when needed


    //Horizontal Movmeent
    if(joyStick_X<0){
        player.force_x -= 2;
        player.state |= FACE_LEFT;
    }
    else if(joyStick_X>0){
        player.force_x += 2;
        player.state &= ~FACE_LEFT;
    }
    else{
        switch(currentCollisionInfo){
            case 0:
                Physics_AddFriction(99, &(player.velocity_x), &player.force_x);
                break;
            case COLLIDE_BOTTOM:
                Physics_AddFriction(90, &(player.velocity_x), &player.force_x);
                break;
        }
    }

    //Vertical Movement
    
    uint8_t wallSlidingCond = (INPUT_LEFT | INPUT_RIGHT) && (currentCollisionInfo & COLLIDE_LEFT) | (currentCollisionInfo & COLLIDE_RIGHT );



    //Wall-Jump Override Logic:
    if(INPUT_LEFT && currentCollisionInfo & COLLIDE_LEFT && INPUT_JUMP){
        player.velocity_x = 0;
        player.velocity_y = 0;
        player.force_x = 50;
        player.force_y = -40;
        maxSpeedX = PLAYER_MAX_SPEED_WALLJUMP;
    }
    else if(INPUT_RIGHT && currentCollisionInfo & COLLIDE_RIGHT && INPUT_JUMP){
        player.velocity_x = 0;
        player.velocity_y = 0;
        player.force_x = -50;
        player.force_y = -40;
        maxSpeedX = PLAYER_MAX_SPEED_WALLJUMP;
    }else{

        if(INPUT_JUMP && (currentCollisionInfo & COLLIDE_BOTTOM) && startJump == 0) {
            player.force_y = -30;
            startJump = 1;
        }
        else if(startJump == 1 && jumpTIme < 6 && INPUT_JUMP){
            player.force_y = -4;
            jumpTIme += 1;
        } else{
            startJump = 0;
            jumpTIme = 0;
            if(wallSlidingCond && player.velocity_y > 0){
                player.force_y = -3;
            }
        }
        
    }
                                       
    Physics_AddGravity(85, &(player.velocity_y), &player.force_y);
    
    Physics_ApplyForceWithCap(&player.velocity_x, &player.force_x, maxSpeedX);
    
    Physics_getAcceleration(player.force_x, &player.accel_x);
    player.velocity_x += player.accel_x;
    
    //Clamping velocity logic
    Physics_EaseTowardSpeed(&player.velocity_x, maxSpeedX, 10);
    player.position_x += player.velocity_x;
    Entity_WrapX(&player);
    Entity_CollideX_Tile(&player);


    
    Physics_getAcceleration(player.force_y, &player.accel_y);
    player.velocity_y += player.accel_y;
    
    player.position_y += player.velocity_y;
   
    Entity_CollideY_Tile(&player);
}

void RenderPlayer(){
    int convertedScreenPosX = (player.position_x >> 2);
    int convertedScreenPosY = (player.position_y >> 2);
    
    bool flip = (player.state & FACE_LEFT);     // constant for the whole sprite
    
    const uint8_t* playerSprite;
    ESP_LOGI(DISPLAY_TAG, "COLLIDE_RIGHT: %u", player.collisionSide & COLLIDE_RIGHT);
    ESP_LOGI(DISPLAY_TAG, "COLLIDE_RIGHT: %u", player.collisionSide & COLLIDE_LEFT);
    if((INPUT_LEFT | INPUT_RIGHT) && ((player.collisionSide & (COLLIDE_LEFT|COLLIDE_RIGHT))) ){
        playerSprite = tile_player_wall;
    }
    else if(player.velocity_y < 0){
        playerSprite = tile_player_rise;
    }
    else if(player.velocity_y > 0){
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

            uint16_t pixel1 = palette_player1[shade1];
            uint16_t pixel2 = palette_player1[shade2];
            



            uint32_t dstX = 160 + (x << 2);
            
            if(player.state&FACE_LEFT){
                dstX = 160 + (16 - 4) - (x << 2);    // groups reverse
                pixel1 = palette_player1[shade2];
                pixel2 = palette_player1[shade1];
            } else {
                dstX = 160 + (x << 2);
                pixel1 = palette_player1[shade1];
                pixel2 = palette_player1[shade2];
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
