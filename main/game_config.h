#pragma once
#define TILE_SIZE 16          // player is 16x16
#define TILE_SIZE_HALF 8          // player is 16x16
#define PLAYER_SIZE 16          // player is 16x16
#define FP_SHIFT    2           //FIXED-POINT SHIFT

//COLLISION SIDE FLAGS
#define COLLIDE_NONE   0x00
#define COLLIDE_LEFT   (1u << 0)
#define COLLIDE_RIGHT  (1u << 1)
#define COLLIDE_TOP    (1u << 2)
#define COLLIDE_BOTTOM (1u << 3)