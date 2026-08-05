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
uint8_t Entity_Collide_Entity(Entity *entityMain, Entity *entitySecondary){
    int playerPosX = entityMain->position_x >> FP_SHIFT;
    int playerPosY = entityMain->position_y >> FP_SHIFT;
    int entitySecondaryPosX = entitySecondary->position_x >> FP_SHIFT;
    int entitySecondaryPosY = entitySecondary->position_y >> FP_SHIFT;
    int aLeft   = playerPosX;
    int aRight  = aLeft + entityMain->size;
    int aTop    = playerPosY;
    int aBottom = aTop + entityMain->size;

    int bLeft   = entitySecondaryPosX;
    int bRight  = bLeft + entitySecondary->size;
    int bTop    = entitySecondaryPosY;
    int bBottom = bTop + entitySecondary->size;

    // AABB overlap test — bail early if they don't touch
    if (!(aLeft < bRight && aRight > bLeft && aTop < bBottom && aBottom > bTop))
        return 0;

    // How far they overlap on each axis (always positive when overlapping)
    int overlapLeft   = aRight - bLeft;   // push main LEFT  by this much
    int overlapRight  = bRight - aLeft;   // push main RIGHT by this much
    int overlapTop    = aBottom - bTop;   // push main UP    by this much
    int overlapBottom = bBottom - aTop;   // push main DOWN  by this much

    // Smallest overlap on each axis = the direction the collision came from
    int overlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
    int overlapY = (overlapTop  < overlapBottom) ? overlapTop  : overlapBottom;

    if (overlapX < overlapY){
        // Horizontal collision
        if (overlapLeft < overlapRight){
            // secondary is to the RIGHT of main → main was hit on its right side
            return COLLIDE_RIGHT;
        } else {
            return COLLIDE_LEFT;
        }
    } else {
        // Vertical collision
        if (overlapTop < overlapBottom){
            // secondary is BELOW main → main was hit on its bottom side
            return COLLIDE_BOTTOM;
        } else {
            return COLLIDE_TOP;
        }
    }
    return 0;
}