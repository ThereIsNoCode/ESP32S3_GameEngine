#include <stdint.h>

//Input: force magnitude for whichever axis
//Output: acceleration for whichever axis
void Physics_getAcceleration(
    int32_t forceMagnitude, 
    int32_t *accel);

void Physics_AddFriction(uint8_t frictionCoeff, int32_t* velocity, int32_t *force);
void Physics_AddGravity(uint8_t frictionCoeff, int32_t *velocity, int32_t *force);
void Physics_ClampVelocity(int32_t *velocity, int32_t maxSpeed);
void Physics_ApplyForceWithCap(int32_t *velocity, int32_t *force, int32_t maxSpeed);
void Physics_EaseTowardSpeed(int32_t *velocity, int32_t maxSpeed, int32_t lerpFactor /* out of 100 */);