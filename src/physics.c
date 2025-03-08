#include "../include/physics.h"
#include "../include/renderer.h"
#include "../include/logging.h"
#include <stdio.h>
#include <math.h>

// Physics constants in SI units
#define GRAVITY 9.81f          // m/s²
#define PLAYER_SPEED 5.0f      // m/s
#define JUMP_VELOCITY 7.0f     // m/s
#define PLAYER_WIDTH 1.0f      // m
#define PLAYER_HEIGHT 2.0f     // m
#define GROUND_Y 0.0f          // m (ground is at y=0)
#define WORLD_WIDTH 100.0f     // m

// Input state
static struct {
    bool move_left;
    bool move_right;
    bool jump;
} input;

// Initialize the physics system
void physics_init(void) {
    // Reset input state
    input.move_left = false;
    input.move_right = false;
    input.jump = false;
    
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Initialized with SI units (gravity = %.2f m/s²)", GRAVITY);
}

// Shutdown the physics system
void physics_shutdown(void) {
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Shutdown");
}

// Apply input to physics
void physics_apply_input(bool move_left, bool move_right, bool jump) {
    // Only one direction can be active at a time
    if (move_left && move_right) {
        // If both are pressed, cancel out
        move_left = false;
        move_right = false;
    }
    
    input.move_left = move_left;
    input.move_right = move_right;
    input.jump = jump;
    
    // Debug output for input
    if (move_left || move_right || jump) {
        LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Input: left=%d, right=%d, jump=%d", 
               move_left, move_right, jump);
    }
}

// Update physics (to be called by the scheduler)
void physics_update(double dt, void* user_data) {
    // Limit the time step to prevent extreme movements
    float safe_dt = (float)dt;
    if (safe_dt > 0.1f) {
        // Warning removed to prevent console spam
        // LOG_WARNING(LOG_CATEGORY_PHYSICS, "Limiting large time step: %.3f -> 0.1 seconds", safe_dt);
        safe_dt = 0.1f;
    }
    
    // Begin writing to the game state
    GameState* state = game_state_begin_write();
    
    // Update player horizontal movement
    if (input.move_left) {
        state->player.velocity_x = -PLAYER_SPEED;
    } else if (input.move_right) {
        state->player.velocity_x = PLAYER_SPEED;
    } else {
        // Apply friction to slow down when no input
        state->player.velocity_x *= 0.9f;
        
        // Stop completely if very slow
        if (fabs(state->player.velocity_x) < 0.1f) {
            state->player.velocity_x = 0.0f;
        }
    }
    
    // Apply jump if grounded
    if (input.jump && state->player.is_grounded) {
        state->player.velocity_y = -JUMP_VELOCITY;
        state->player.is_grounded = false;
        state->player.is_jumping = true;
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Player jumped with velocity %.2f m/s", JUMP_VELOCITY);
    }
    
    // Apply gravity
    state->player.velocity_y += GRAVITY * safe_dt;
    
    // Update player position
    float prev_x = state->player.position_x;
    float prev_y = state->player.position_y;
    
    state->player.position_x += state->player.velocity_x * safe_dt;
    state->player.position_y += state->player.velocity_y * safe_dt;
    
    // Check ground collision
    // Player's feet are at position_y + PLAYER_HEIGHT/2
    float feet_y = state->player.position_y + PLAYER_HEIGHT/2;
    if (feet_y >= GROUND_Y) {
        // Place the player so their feet are exactly on the ground
        state->player.position_y = GROUND_Y - PLAYER_HEIGHT/2;
        state->player.velocity_y = 0.0f;
        state->player.is_grounded = true;
        state->player.is_jumping = false;
    }
    
    // Check world boundaries
    float half_width = PLAYER_WIDTH / 2.0f;
    if (state->player.position_x < -WORLD_WIDTH/2 + half_width) {
        state->player.position_x = -WORLD_WIDTH/2 + half_width;
        state->player.velocity_x = 0.0f;
    } else if (state->player.position_x > WORLD_WIDTH/2 - half_width) {
        state->player.position_x = WORLD_WIDTH/2 - half_width;
        state->player.velocity_x = 0.0f;
    }
    
    // Update game time
    state->game_time += safe_dt;
    
    // Finish writing to the game state
    game_state_end_write();
    
    // Debug output for physics update (only when moving and less frequently)
    static double last_debug_time = 0.0;
    float dx = state->player.position_x - prev_x;
    float dy = state->player.position_y - prev_y;
    
    if ((fabs(dx) > 0.01f || fabs(dy) > 0.01f) && 
        (state->game_time - last_debug_time > 0.5)) {
        LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Updated: pos=(%.2f, %.2f) m, vel=(%.2f, %.2f) m/s, moved=(%.2f, %.2f) m, dt=%.3fs",
               state->player.position_x, state->player.position_y,
               state->player.velocity_x, state->player.velocity_y,
               dx, dy, safe_dt);
        last_debug_time = state->game_time;
    }
} 