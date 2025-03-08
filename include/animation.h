#ifndef ANIMATION_H
#define ANIMATION_H

#include "game_state.h"

// Initialize the animation system
void animation_init(void);

// Shutdown the animation system
void animation_shutdown(void);

// Update animations (to be called by the scheduler)
void animation_update(double dt, void* user_data);

// Get the interpolated game state for rendering
void animation_get_render_state(GameState* result);

#endif // ANIMATION_H 