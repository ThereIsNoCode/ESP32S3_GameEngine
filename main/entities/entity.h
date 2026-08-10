#pragma once
#include <stdint.h>
#include "../game_config.h"


typedef struct Entity{
    
    int32_t position_x;
    int32_t position_y;
    int32_t velocity_x;
    int32_t velocity_y;
    int32_t accel_x;
    int32_t accel_y;
    int32_t force_x;
    int32_t force_y;
    uint16_t tint_color; // 0xFFFF //Each nibble represents I,R,G,B (Gamma, red, green, blue), 16 shades each
    uint8_t collisionSide;
    uint8_t size;
    uint8_t state;
    uint8_t id;
} Entity;

typedef struct EntityPacket{
    
    int32_t position_x;
    int32_t position_y;
    uint8_t state;
    uint8_t id;
} EntityPacket;

#define ON_GROUND   (1u << 0)
#define FACE_LEFT   (1u << 1)
#define IS_MOVING   (1u << 2)
#define IS_HOLDING  (1u << 3)

void Entity_CollideY_Tile(Entity *entity);
void Entity_CollideX_Tile(Entity *entity);
uint8_t Entity_Collide_Entity(Entity *entityMain, Entity *entitySecondary);
void Entity_WrapX(Entity *entity);