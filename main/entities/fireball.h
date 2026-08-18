#pragma once
#include <stdint.h>
#include "entity.h"
#include "../drivers/ILI9341.h"
#include "../tiles/tiles.h"
#include "utility/physics.h"
#include "../networking/packets.h"
#include "../maps/level_manager.h"
#include "player.h"

#define FIREBALL_STATUS_IS_BOUNCEY (1 << 0)
#define FIREBALL_STATUS_DIR_LEFT (1 << 1)

typedef struct Fireball{
    uint8_t status; //One bit for whether bouncing or bullet projectile
    int8_t ignoreEntityId; //using int8_t since playerId = 0 is host, playerId = 1 is client. -1 can represent no one
    uint8_t activeTime; // Time fireball stays active until it fades
} Fireball;

void Fireball_Initialize();
void Fireball_Move();
void Fireball_Render();