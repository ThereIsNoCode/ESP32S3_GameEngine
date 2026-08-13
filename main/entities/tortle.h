#pragma once
#include <stdint.h>
#include "entity.h"



#define TORTLE_STATUS_SHELL ( 1 << 0 )
#define TORTLE_STATUS_SLIDING ( 1 << 1 )
#define TORTLE_STATUS_IS_GRABBABLE ( 1 << 2 )

typedef struct Tortle{
    uint8_t status; //One bit for shell mode
    uint8_t isGrabbed;
    int8_t ignoreEntityId; //using int8_t since playerId = 0 is host, playerId = 1 is client. -1 can represent no one
} Tortle;

//extern Entity bombArr[BOMB_POOL_SIZE];

void Tortle_Initialize();
void Tortle_Move();
void Tortle_Render();