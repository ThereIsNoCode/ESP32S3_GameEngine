#pragma once
#include <stdint.h>
#include "entity.h"

typedef struct Bomb{
    
    uint8_t fuseTimer;
    uint8_t isGrabbed;
    uint8_t ignoreEntityId; //Honestly, make this a int8_t since playerId = 0 is host, playerId = 1 is client. -1 can represent no one
} Bomb;

#define BOMB_POOL_SIZE 5
extern Entity bombArr[BOMB_POOL_SIZE];

void Bomb_Initialize();
void Bomb_Move();
void Bomb_Render();
