#pragma once
#include <stdint.h>
#include "entity.h"
typedef struct Gumby{
    uint8_t isLeft;
} Gumby;

//extern Entity bombArr[BOMB_POOL_SIZE];

void Gumby_Initialize();
void Gumby_Move();
void Gumby_Render();