#pragma once
#include <stdint.h>
#include "entity.h"
#include "../drivers/ILI9341.h"
#include "../tiles/tiles.h"
#include "utility/physics.h"
#include "../networking/packets.h"
#include "../maps/level_manager.h"

typedef struct Player{
    uint8_t startJump;
    uint8_t jumpTime;
} Player;

static uint16_t cam_offset = 0;
extern Entity *player_Current;

extern int joyStick_X;
extern int joyStick_Y;
extern Entity playerArr[];
extern Player playerDataArr[];
#define INPUT_RIGHT  (joyStick_X>0)
#define INPUT_LEFT (joyStick_X<0)

uint8_t Player_CheckCollision();

void assign_player(uint8_t id);
void Player_SetPalette(uint16_t player1_tint, uint16_t player2_tint);


void Player_Move();


void Player_Apply_Movement(packet_clientInput_t *input);
void Player_DirectSetPosition(packet_clientPosition_t *input);
void Player_InterpolateRemote(packet_clientPosition_t *input);
// void Player_CollideX();
// void Player_CollideY();
void RenderPlayer();
void RenderOtherPlayer();
//void Player_WrapX();