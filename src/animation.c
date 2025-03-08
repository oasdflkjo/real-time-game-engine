#include "../include/animation.h"
#include "../include/scheduler.h"
#include "../include/logging.h"
#include <stdio.h>

// Animation state
static struct {
    double last_physics_time;
    GameState render_state;
} animation;

// Initialize the animation system
void animation_init(void) {
    // Reset animation state
    animation.last_physics_time = 0.0;
    
    LOG_INFO(LOG_CATEGORY_ANIMATION, "Initialized for interrupt-driven fixed time steps");
}

// Shutdown the animation system
void animation_shutdown(void) {
    LOG_INFO(LOG_CATEGORY_ANIMATION, "Shutdown");
}

// Update animations (to be called by the scheduler)
void animation_update(double dt, void* user_data) {
    // Get the current game state
    const GameState* current_state = game_state_get_read();
    
    // Update the last physics time
    animation.last_physics_time = current_state->game_time;
    
    LOG_DEBUG(LOG_CATEGORY_ANIMATION, "Updated with fixed time step");
}

// Get the current game state for rendering (no interpolation needed with fixed time steps)
void animation_get_render_state(GameState* result) {
    if (!result) {
        LOG_ERROR(LOG_CATEGORY_ANIMATION, "Null result pointer passed to animation_get_render_state");
        return;
    }
    
    // Get the current game state directly - no interpolation needed with fixed time steps
    const GameState* current_state = game_state_get_read();
    
    // Copy the current state to the result
    *result = *current_state;
    
    // Ensure camera position exactly matches player position
    result->camera_x = result->player.position_x;
    result->camera_y = result->player.position_y;
    
    LOG_DEBUG(LOG_CATEGORY_ANIMATION, "Render state prepared with fixed time step");
} 