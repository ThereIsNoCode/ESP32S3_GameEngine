#pragma once
#include <stdint.h>
#include "entity.h"
#include "../drivers/ILI9341.h"
#include "../tiles/tiles.h"
#include "utility/physics.h"



typedef struct Player{
    
    int32_t position_x;
    int32_t position_y;
    int32_t velocity_x;
    int32_t velocity_y;
    int32_t accel_x;
    int32_t accel_y;
    uint32_t tint_color; // 0x00FFFFFF each byte represents colour   0x00(R)(G)(B)
    uint8_t collisionSide;
    uint8_t id;
} Player;

static uint16_t cam_offset = 0;
extern Entity player;
extern int joyStick_X;
extern int joyStick_Y;

#define INPUT_RIGHT  (joyStick_X>0)
#define INPUT_LEFT (joyStick_X<0)

uint8_t Player_CheckCollision();

void Player_SetPalette(uint16_t player1_tint, uint16_t player2_tint);
void Player_Move();

// void Player_CollideX();
// void Player_CollideY();
void RenderPlayer();
//void Player_WrapX();