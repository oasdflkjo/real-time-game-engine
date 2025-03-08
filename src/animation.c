#include "../include/animation.h"
#include "../include/scheduler.h"
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
    
    printf("[Animation] Initialized\n");
}

// Shutdown the animation system
void animation_shutdown(void) {
    printf("[Animation] Shutdown\n");
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
}

// Get the interpolated game state for rendering
void animation_get_render_state(GameState* result) {
    if (!result) {
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
} 