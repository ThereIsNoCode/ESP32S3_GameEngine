#pragma once
#include <stdint.h>
#include "entity.h"
#include "../drivers/ILI9341.h"
#include "../tiles/tiles.h"
#include "utility/physics.h"
#include "../networking/packets.h"
#include "../maps/level_manager.h"
#include "player.h"


void Flower_Initialize();
void Flower_Move();
void Flower_Render();