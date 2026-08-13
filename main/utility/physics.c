#include "physics.h"


#define ACCELERATION_GRAVITY 3

//Friction will be defined as powers of two
#define FRICTION_NONE 0
#define FRICTION_HALF 1
#define FRICTION_QUARTER 2

//Input: force magnitude for whichever axis
//Output: acceleration for whichever axis
void Physics_getAcceleration(
    int16_t forceMagnitude, 
    int16_t *accel){

    //F = ma => a = F/m, but no mass at the moment so mass will be treated as one

    (*accel) = forceMagnitude; 
}

void Physics_AddFriction(uint8_t frictionCoeff, int32_t *velocity, int16_t *force){
    //(*force) -= velocity*0.05; //Temperary Until I can find More efficient solution
    *velocity = (*velocity*frictionCoeff)/100; // 243/256 ≈ 0.949   

}

void Physics_AddGravity(uint8_t frictionCoeff, int32_t *velocity, int16_t *force){
    //(*force) -= velocity*0.05; //Temperary Until I can find More efficient solution
    *force += 4; // 243/256 ≈ 0.949
    //ESP_LOGI(DISPLAY_TAG, "VELOCITY: %i", *velocity);

}

#define LERP_SHIFT   8
#define LERP_SCALE   (1 << LERP_SHIFT)      // 256
#define LERP_FACTOR  10                     // 64/256 = 0.25 per frame

void Physics_ClampVelocity(int32_t *velocity, int32_t maxSpeed){
    int32_t target;

    if((*velocity) > maxSpeed) {
        target = maxSpeed;
    }
    else if((*velocity) < -maxSpeed) {
        target = -maxSpeed;
    }
    else {
        return; // already within range, nothing to do
    }

    int32_t diff = target - (*velocity);
    (*velocity) += (diff);

}

void Physics_ApplyForceWithCap(int32_t *velocity, int16_t *force, int32_t maxSpeed){
    // If we're already at/past the cap and force is pushing further past it, zero the force out.
    if ((*velocity) >= maxSpeed && (*force) > 0) *force = 0;
    if ((*velocity) <= -maxSpeed && (*force) < 0) *force = 0;
}

void Physics_EaseTowardSpeed(int32_t *velocity, int32_t maxSpeed, int32_t lerpFactor /* out of 100 */){
    int32_t target;

    if ((*velocity) > maxSpeed) {
        target = maxSpeed;
    }
    else if ((*velocity) < -maxSpeed) {
        target = -maxSpeed;
    }
    else {
        return; // within range, nothing to ease
    }

    int32_t diff = target - (*velocity);
    int32_t step = (diff * lerpFactor) / 100;

    if (step == 0) (*velocity) = target;  // snap to avoid truncation getting stuck
    else (*velocity) += step;
}