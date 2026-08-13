#pragma once
#include <stdint.h>
#include "entity.h"



#define TORTLE_STATUS_SHELL ( 1 << 0 )
#define TORTLE_STATUS_SLIDING ( 1 << 1 )
#define TORTLE_STATUS_IS_GRABBABLE ( 1 << 2 )

typedef struct Tortle{
    uint8_t status; //One bit for shell mode

} Tortle;

//extern Entity bombArr[BOMB_POOL_SIZE];

void Tortle_Initialize();
void Tortle_Move();
void Tortle_Render();