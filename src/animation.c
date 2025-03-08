#include "../include/animation.h"
#include "../include/scheduler.h"
#include "../include/logging.h"
#include <stdio.h>

// Animation state
static struct {
    double last_physics_time;
    double accumulator;
    GameState render_state;
} animation;

// Initialize the animation system
void animation_init(void) {
    // Reset animation state
    animation.last_physics_time = 0.0;
    animation.accumulator = 0.0;
    
    LOG_INFO(LOG_CATEGORY_ANIMATION, "Initialized");
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
    
    // Reset the accumulator
    animation.accumulator = 0.0;
    
    // Calculate interpolation alpha (0.0 to 1.0)
    float alpha = (float)(animation.accumulator / TASK_INTERVAL_MS[TASK_PRIORITY_PHYSICS_AI]);
    
    // Interpolate between physics states
    game_state_interpolate(alpha, &animation.render_state);
    
    LOG_DEBUG(LOG_CATEGORY_ANIMATION, "Updated with dt=%.3f ms, alpha=%.3f", dt * 1000.0, alpha);
}

// Get the interpolated game state for rendering
void animation_get_render_state(GameState* result) {
    if (!result) {
        LOG_ERROR(LOG_CATEGORY_ANIMATION, "Null result pointer passed to animation_get_render_state");
        return;
    }
    
    // Get the current game state
    const GameState* current_state = game_state_get_read();
    
    // Update the accumulator
    double current_time = current_state->game_time;
    animation.accumulator = current_time - animation.last_physics_time;
    
    // Calculate interpolation alpha (0.0 to 1.0)
    float alpha = (float)(animation.accumulator / TASK_INTERVAL_MS[TASK_PRIORITY_PHYSICS_AI]);
    
    // Interpolate between physics states
    game_state_interpolate(alpha, &animation.render_state);
    
    // Copy the interpolated state to the result
    *result = animation.render_state;
    
    LOG_DEBUG(LOG_CATEGORY_ANIMATION, "Render state prepared with alpha=%.3f", alpha);
} 