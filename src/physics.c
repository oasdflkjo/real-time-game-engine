#include "../include/physics.h"
#include "../include/renderer.h"
#include "../include/logging.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

// Physics constants in SI units
#define GRAVITY 9.81f          // m/s²
#define PLAYER_SPEED 20.0f      // m/s
#define JUMP_VELOCITY 10.0f     // m/s
#define PLAYER_WIDTH 1.0f      // m
#define PLAYER_HEIGHT 2.0f     // m
#define GROUND_Y 0.0f          // m (ground is at y=0)
#define WORLD_WIDTH 100.0f     // m

// Fixed physics time step (in seconds)
#define FIXED_TIME_STEP 0.016f  // ~60Hz

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
    
    // Log the initial game state
    const GameState* state = game_state_get_read();
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Initial player position: (%.2f, %.2f)", 
           state->player.position_x, state->player.position_y);
    
    // Log ground positions
    for (int i = 0; i < state->ground_count; i++) {
        const GroundState* ground = &state->grounds[i];
        float ground_top = ground->position_y - ground->height / 2.0f;
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Ground %d: pos=(%.2f, %.2f), top=%.2f", 
               i, ground->position_x, ground->position_y, ground_top);
    }
    
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

// Check if a point is inside a rectangle
static bool is_point_in_rect(float px, float py, float rx, float ry, float rw, float rh) {
    float half_width = rw / 2.0f;
    float half_height = rh / 2.0f;
    return (px >= rx - half_width && px <= rx + half_width &&
            py >= ry - half_height && py <= ry + half_height);
}

// Update physics (to be called by the scheduler at fixed intervals)
void physics_update(double dt, void* user_data) {
    // Begin writing to the game state
    GameState* state = game_state_begin_write();
    
    // Store previous position for logging
    float prev_x = state->player.position_x;
    float prev_y = state->player.position_y;
    
    // Update player horizontal movement with fixed time step
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
    
    // Apply gravity with fixed time step
    state->player.velocity_y += GRAVITY * FIXED_TIME_STEP;
    
    // Update player position with fixed time step
    float new_x = state->player.position_x + state->player.velocity_x * FIXED_TIME_STEP;
    float new_y = state->player.position_y + state->player.velocity_y * FIXED_TIME_STEP;
    
    // Reset grounded state
    bool was_grounded = state->player.is_grounded;
    state->player.is_grounded = false;
    
    // Check ground collision with all ground planes
    float player_half_width = PLAYER_WIDTH / 2.0f;
    float player_half_height = PLAYER_HEIGHT / 2.0f;
    
    // Calculate player's feet position (bottom of player)
    float player_feet_y = new_y + player_half_height;
    
    // Log player position and velocity
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Player: pos=(%.2f, %.2f), vel=(%.2f, %.2f), feet_y=%.2f, grounded=%d", 
           new_x, new_y, state->player.velocity_x, state->player.velocity_y, player_feet_y, was_grounded);
    
    // Check collision with each ground
    for (int i = 0; i < state->ground_count; i++) {
        GroundState* ground = &state->grounds[i];
        
        // Calculate ground boundaries
        float ground_left = ground->position_x - ground->width / 2.0f;
        float ground_right = ground->position_x + ground->width / 2.0f;
        
        // Calculate the top surface of the ground (important for collision)
        float ground_top = ground->position_y - ground->height / 2.0f;
        
        // Log ground position
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Ground %d: pos=(%.2f, %.2f), bounds=[%.2f, %.2f], top=%.2f", 
               i, ground->position_x, ground->position_y, ground_left, ground_right, ground_top);
        
        // Check if player is horizontally within the ground's bounds
        if (new_x + player_half_width >= ground_left && 
            new_x - player_half_width <= ground_right) {
            
            // Calculate player's feet position in previous frame
            float prev_feet_y = prev_y + player_half_height;
            
            LOG_INFO(LOG_CATEGORY_PHYSICS, "Player over ground %d: feet_y=%.2f, prev_feet_y=%.2f, ground_top=%.2f", 
                   i, player_feet_y, prev_feet_y, ground_top);
            
            // Check if player's feet are at or below the ground's top surface
            // AND the player was above the ground in the previous frame
            if (player_feet_y >= ground_top && prev_feet_y <= ground_top) {
                // Place the player so their feet are exactly on the ground
                new_y = ground_top - player_half_height;
                state->player.velocity_y = 0.0f;
                state->player.is_grounded = true;
                state->player.is_jumping = false;
                
                LOG_INFO(LOG_CATEGORY_PHYSICS, "Player landed on ground %d at y=%.2f", i, new_y);
                break;  // Only collide with one ground at a time
            }
        }
    }
    
    // Update player position
    state->player.position_x = new_x;
    state->player.position_y = new_y;
    
    // Check world boundaries
    float half_width = PLAYER_WIDTH / 2.0f;
    if (state->player.position_x < -WORLD_WIDTH/2 + half_width) {
        state->player.position_x = -WORLD_WIDTH/2 + half_width;
        state->player.velocity_x = 0.0f;
    } else if (state->player.position_x > WORLD_WIDTH/2 - half_width) {
        state->player.position_x = WORLD_WIDTH/2 - half_width;
        state->player.velocity_x = 0.0f;
    }
    
    // Finish writing to the game state
    game_state_end_write();
    
    // Debug output for physics update (only when moving and less frequently)
    static double last_debug_time = 0.0;
    static double accumulated_time = 0.0;
    float dx = state->player.position_x - prev_x;
    float dy = state->player.position_y - prev_y;
    
    accumulated_time += dt;
    if ((fabs(dx) > 0.01f || fabs(dy) > 0.01f) && 
        (accumulated_time - last_debug_time > 0.5)) {
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Updated: pos=(%.2f, %.2f) m, vel=(%.2f, %.2f) m/s, grounded=%d",
               state->player.position_x, state->player.position_y,
               state->player.velocity_x, state->player.velocity_y,
               state->player.is_grounded);
        last_debug_time = accumulated_time;
    }
} 