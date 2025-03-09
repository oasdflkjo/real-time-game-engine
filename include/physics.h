#ifndef PHYSICS_H
#define PHYSICS_H

#include "game_state.h"
#include "input.h"

// Initialize the physics system
void physics_init(void);

// Shutdown the physics system
void physics_shutdown(void);

// Update physics (to be called by the scheduler)
void physics_update(double dt, void* user_data);

// Apply input to physics (legacy function, kept for compatibility)
void physics_apply_input(bool move_left, bool move_right, bool jump);

// Apply analog input to physics
void physics_apply_analog_input(InputState input_state);

#endif // PHYSICS_H 