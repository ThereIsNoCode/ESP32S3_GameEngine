#include "entity.h"
#include "../tiles/tiles.h"
#include "../../maps/level_manager.h"
#include "../drivers/esp_family/ESP32-S3.h"
//Used for Handling  position Wrapping once reached end of level
void Entity_WrapX(Entity *entity){
    int32_t levelWidthFP = (lvl_width * TILE_SIZE) << FP_SHIFT;

    if ((*entity).position_x < 0){
        (*entity).position_x += levelWidthFP;
    } else if ((*entity).position_x >= levelWidthFP){
        (*entity).position_x -= levelWidthFP;
    }
}

static inline int Tile_IsSolid(uint8_t tileId){
    return (tileId == 0x01);       // <-- adjust to your solid-tile rule
}


static inline int Wrap_Distance_X(int a, int b)
{
    int mapWidth = lvl_width * TILE_SIZE;
    int dx = a - b;

    if (dx > mapWidth / 2)
        dx -= mapWidth;

    if (dx < -mapWidth / 2)
        dx += mapWidth;

    return dx;
}


// Resolve only horizontal collisions
void Entity_CollideX_Tile(Entity *entity)
{
    const int FP_ONE = 1 << FP_SHIFT;

    int halfSize = entity->size / 2;
    int px = entity->position_x >> FP_SHIFT;
    int py = entity->position_y >> FP_SHIFT;

    int fracX = entity->position_x & (FP_ONE - 1);
    int entityCenterX = px + halfSize;

    // Vertical test stays INTEGER — its sub-pixel tolerance is what keeps
    // the floor you're standing on from acting as a wall.
    int pTop    = py;
    int pBottom = py + entity->size;

    int cx = (px + halfSize) >> 4;
    int cy = (py + halfSize) >> 4;

    int tyStart = cy - 1, tyEnd = cy + 1;
    if (tyStart < 0)         tyStart = 0;
    if (tyEnd >= lvl_height) tyEnd   = lvl_height - 1;

    for (int ty = tyStart; ty <= tyEnd; ty++)
    {
        int tTop    = ty * TILE_SIZE;
        int tBottom = tTop + TILE_SIZE;

        // Integer vertical reject (hoisted out of the inner loop)
        if (pTop >= tBottom || pBottom <= tTop)
            continue;

        for (int txRaw = cx - 1; txRaw <= cx + 1; txRaw++)
        {
            int tx = ((txRaw % lvl_width) + lvl_width) % lvl_width;
            if (!Tile_IsSolid(lvl_data[ty * lvl_width + tx]))
                continue;

            int tLeft  = txRaw * TILE_SIZE;
            int tRight = tLeft + TILE_SIZE;

            int wrappedCenterX = tLeft + TILE_SIZE_HALF
                               + Wrap_Distance_X(entityCenterX, tLeft + TILE_SIZE_HALF);

            // X edges in fixed point (fraction restored) — fixes the flicker
            int pLeftFP  = ((wrappedCenterX - halfSize) << FP_SHIFT) + fracX;
            int pRightFP = pLeftFP + (entity->size << FP_SHIFT);

            int tLeftFP  = tLeft  << FP_SHIFT;
            int tRightFP = tRight << FP_SHIFT;

            if (pLeftFP < tRightFP && pRightFP > tLeftFP)
            {
                if (entity->velocity_x > 0) {
                    entity->position_x = (tLeft - entity->size) << FP_SHIFT;
                    entity->collisionSide |= COLLIDE_RIGHT;
                } else if (entity->velocity_x < 0) {
                    entity->position_x = tRight << FP_SHIFT;
                    entity->collisionSide |= COLLIDE_LEFT;
                }
                entity->velocity_x = 0;
                return;
            }
        }
    }
}



// Resolve only vertical collisions
void Entity_CollideY_Tile(Entity *entity)
{
    int halfSize = entity->size/2;
    int px = entity->position_x >> FP_SHIFT;
    int py = entity->position_y >> FP_SHIFT;


    int pLeft   = px;
    int pRight  = px + entity->size;
    int pTop    = py;
    int pBottom = py + entity->size;


    int cx = ((px + halfSize) >> 4);
    int cy = ((py + halfSize) >> 4);

    int tyStart = cy - 1;
    int tyEnd   = cy + 1;
    if (tyStart < 0)            tyStart = 0;
    if (tyEnd >= lvl_height)    tyEnd   = lvl_height - 1;

    for(int ty = tyStart; ty <= tyEnd; ty++)
    {
        for(int txRaw = cx - 1; txRaw <= cx + 1; txRaw++)
        {
            int tx = ((txRaw % lvl_width) + lvl_width) % lvl_width;


            if(!Tile_IsSolid(lvl_data[ty * lvl_width + tx]))
                continue;


            int tLeft   = txRaw * TILE_SIZE;
            int tTop    = ty * TILE_SIZE;
            int tRight  = tLeft + TILE_SIZE;
            int tBottom = tTop + TILE_SIZE;


            if(pLeft < tRight &&
               pRight > tLeft &&
               pTop < tBottom &&
               pBottom > tTop)
            {

                if(entity->velocity_y > 0)
                {
                    // falling onto floor
                    entity->position_y =
                        (tTop - entity->size) << FP_SHIFT;

                    entity->collisionSide |= COLLIDE_BOTTOM;
                }
                else if(entity->velocity_y < 0)
                {
                    // hitting ceiling
                    entity->position_y =
                        tBottom << FP_SHIFT;

                    entity->collisionSide |= COLLIDE_TOP;
                }


                entity->velocity_y = 0;


                py = entity->position_y >> FP_SHIFT;
                pTop = py;
                pBottom = py + entity->size;
                return;
            }
        }
    }
}
uint8_t Entity_Collide_Entity(Entity *entityMain, Entity *entitySecondary)
{
    int32_t ax = entityMain->position_x, ay = entityMain->position_y;
    int32_t aw = entityMain->size << FP_SHIFT, ah = entityMain->size << FP_SHIFT;
    int32_t bx = entitySecondary->position_x, by = entitySecondary->position_y;
    int32_t bw = entitySecondary->size << FP_SHIFT, bh = entitySecondary->size << FP_SHIFT;

    // penetration depth out each side at the current position (>0 on all four = overlapping)
    int32_t pushR = (bx + bw) - ax;   // main hit on its LEFT   -> COLLIDE_LEFT
    int32_t pushL = (ax + aw) - bx;   // main hit on its RIGHT  -> COLLIDE_RIGHT
    int32_t pushD = (by + bh) - ay;   // main hit on its TOP    -> COLLIDE_TOP
    int32_t pushU = (ay + ah) - by;   // main hit on its BOTTOM -> COLLIDE_BOTTOM

    if (pushL > 0 && pushR > 0 && pushU > 0 && pushD > 0) {
        // Already overlapping -> resolve out the CLOSEST side.
        int32_t ox = (pushL < pushR) ? pushL : pushR;
        int32_t oy = (pushU < pushD) ? pushU : pushD;
        if (ox < oy) return (pushL < pushR) ? COLLIDE_RIGHT  : COLLIDE_LEFT;
        else         return (pushU < pushD) ? COLLIDE_BOTTOM : COLLIDE_TOP;
    }

    // Not overlapping yet -> swept, so a fast mover can't skip clean through.
    const int64_t ONE = (int64_t)1 << FP_SHIFT;
    const int64_t NEG_INF = INT64_MIN / 2, POS_INF = INT64_MAX / 2;

    int32_t vx = entityMain->velocity_x - entitySecondary->velocity_x;
    int32_t vy = entityMain->velocity_y - entitySecondary->velocity_y;

    int32_t xIn, xOut, yIn, yOut;
    if (vx > 0) { xIn = bx - (ax + aw); xOut = (bx + bw) - ax; }
    else        { xIn = (bx + bw) - ax; xOut = bx - (ax + aw); }
    if (vy > 0) { yIn = by - (ay + ah); yOut = (by + bh) - ay; }
    else        { yIn = (by + bh) - ay; yOut = by - (ay + ah); }

    int64_t xEntry, xExit, yEntry, yExit;
    if (vx == 0) {
        if (ax + aw <= bx || ax >= bx + bw) return 0;
        xEntry = NEG_INF; xExit = POS_INF;
    } else {
        xEntry = ((int64_t)xIn  << FP_SHIFT) / vx;
        xExit  = ((int64_t)xOut << FP_SHIFT) / vx;
    }
    if (vy == 0) {
        if (ay + ah <= by || ay >= by + bh) return 0;
        yEntry = NEG_INF; yExit = POS_INF;
    } else {
        yEntry = ((int64_t)yIn  << FP_SHIFT) / vy;
        yExit  = ((int64_t)yOut << FP_SHIFT) / vy;
    }

    int64_t entry = (xEntry > yEntry) ? xEntry : yEntry;
    int64_t exit  = (xExit  < yExit)  ? xExit  : yExit;
    if (entry > exit || entry < 0 || entry > ONE) return 0;

    if (xEntry > yEntry) return (vx > 0) ? COLLIDE_RIGHT  : COLLIDE_LEFT;
    else                 return (vy > 0) ? COLLIDE_BOTTOM : COLLIDE_TOP;
}