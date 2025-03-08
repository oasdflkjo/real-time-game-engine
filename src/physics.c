#include "../include/physics.h"
#include "../include/renderer.h"
#include <stdio.h>
#include <math.h>

// Physics constants
#define GRAVITY 980.0f
#define PLAYER_SPEED 300.0f
#define JUMP_VELOCITY 500.0f

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
    
    printf("[Physics] Initialized\n");
}

// Shutdown the physics system
void physics_shutdown(void) {
    printf("[Physics] Shutdown\n");
}

// Apply input to physics
void physics_apply_input(bool move_left, bool move_right, bool jump) {
    input.move_left = move_left;
    input.move_right = move_right;
    input.jump = jump;
    
    // Debug output for input
    if (move_left || move_right || jump) {
        printf("[Physics] Input: left=%d, right=%d, jump=%d\n", 
               move_left, move_right, jump);
    }
}

// Update physics (to be called by the scheduler)
void physics_update(double dt, void* user_data) {
    // Begin writing to the game state
    GameState* state = game_state_begin_write();
    
    // Get window dimensions
    int window_width, window_height;
    renderer_get_window_size(&window_width, &window_height);
    
    // Update player horizontal movement
    if (input.move_left) {
        state->player.velocity_x = -PLAYER_SPEED;
    } else if (input.move_right) {
        state->player.velocity_x = PLAYER_SPEED;
    } else {
        // Apply friction to slow down when no input
        state->player.velocity_x *= 0.9f;
        
        // Stop completely if very slow
        if (fabs(state->player.velocity_x) < 5.0f) {
            state->player.velocity_x = 0.0f;
        }
    }
    
    // Apply jump if grounded
    if (input.jump && state->player.is_grounded) {
        state->player.velocity_y = -JUMP_VELOCITY;
        state->player.is_grounded = false;
        state->player.is_jumping = true;
        printf("[Physics] Player jumped\n");
    }
    
    // Apply gravity
    state->player.velocity_y += GRAVITY * dt;
    
    // Update player position
    state->player.position_x += state->player.velocity_x * dt;
    state->player.position_y += state->player.velocity_y * dt;
    
    // Check ground collision
    float ground_y = window_height - 50.0f;
    if (state->player.position_y >= ground_y - 40.0f) {
        state->player.position_y = ground_y - 40.0f;
        state->player.velocity_y = 0.0f;
        state->player.is_grounded = true;
        state->player.is_jumping = false;
    }
    
    // Check wall collisions
    if (state->player.position_x < 20.0f) {
        state->player.position_x = 20.0f;
        state->player.velocity_x = 0.0f;
    } else if (state->player.position_x > window_width - 20.0f) {
        state->player.position_x = window_width - 20.0f;
        state->player.velocity_x = 0.0f;
    }
    
    // Update game time
    state->game_time += dt;
    
    // Finish writing to the game state
    game_state_end_write();
    
    // Debug output for physics update (only when moving)
    if (fabs(state->player.velocity_x) > 0.1f || fabs(state->player.velocity_y) > 0.1f) {
        printf("[Physics] Updated: pos=(%.1f, %.1f), vel=(%.1f, %.1f), grounded=%d\n",
               state->player.position_x, state->player.position_y,
               state->player.velocity_x, state->player.velocity_y,
               state->player.is_grounded);
    }
} 