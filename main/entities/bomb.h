#pragma once
#include <stdint.h>
#include "entity.h"

typedef struct Bomb{
    
    uint8_t fuseTimer;
    uint8_t isGrabbed;
    uint8_t ignoreEntityId;
} Bomb;

#define BOMB_POOL_SIZE 5
extern Entity bombArr[BOMB_POOL_SIZE];

void Bomb_Initialize();
void Bomb_Move();
void Bomb_Render();
