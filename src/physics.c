#include "../include/physics.h"
#include "../include/renderer.h"
#include "../include/logging.h"
#include "../include/entity.h"
#include "../include/debug_hud.h"
#include "../include/camera.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

// Physics constants in SI units
#define GRAVITY 9.81f          // m/s²
#define WORLD_WIDTH 1000.0f     // m (increased to accommodate 10 platforms)
#define TERMINAL_VELOCITY 20.0f // m/s (maximum falling speed)
#define ENEMY_LOOK_AHEAD 1.0f   // m (enemy look-ahead distance)

// Fixed physics time step (in seconds)
#define FIXED_TIME_STEP 0.00390625f  // 256Hz

// Input state
static struct {
    bool move_left;
    bool move_right;
    bool jump;
} input;

// Analog input state
static InputState analog_input;

// Initialize the physics system
void physics_init(void) {
    // Reset input state
    input.move_left = false;
    input.move_right = false;
    input.jump = false;
    
    // Reset analog input state
    analog_input.jump = false;
    analog_input.move_x = 0.0f;
    analog_input.move_y = 0.0f;
    
    // Initialize the enemy speed in the debug HUD
    debug_hud_set_enemy_speed(20.0f);  // Default enemy speed
    
    // Log the initial game state
    const GameState* state = game_state_get_read();
    
    if (state->player) {
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Initial player position: (%.2f, %.2f)", 
               state->player->position_x, state->player->position_y);
    }
    
    // Log ground positions
    for (int i = 0; i < state->ground_count; i++) {
        const GroundState* ground = &state->grounds[i];
        float ground_top = ground->position_y - ground->height / 2.0f;
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Ground %d: pos=(%.2f, %.2f), top=%.2f", 
               i, ground->position_x, ground->position_y, ground_top);
    }
    
    // Log entity information
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Entity count: %d", state->entity_count);
    
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Initialized with SI units (gravity = %.2f m/s²)", GRAVITY);
}

// Shutdown the physics system
void physics_shutdown(void) {
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Shutdown");
}

// Apply input to physics (legacy function)
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
        LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Legacy Input: left=%d, right=%d, jump=%d", 
               move_left, move_right, jump);
    }
}

// Apply analog input to physics
void physics_apply_analog_input(InputState input_state) {
    // Store the input state
    analog_input = input_state;
    
    // Debug output for significant input changes
    static float last_move_x = 0.0f;
    static bool last_jump = false;
    
    if (fabsf(input_state.move_x - last_move_x) > 0.1f || input_state.jump != last_jump) {
        LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Analog Input: move_x=%.2f, jump=%d", 
               input_state.move_x, input_state.jump);
        last_move_x = input_state.move_x;
        last_jump = input_state.jump;
    }
}

// Check if a point is inside a rectangle
static bool is_point_in_rect(float px, float py, float rx, float ry, float rw, float rh) {
    float half_width = rw / 2.0f;
    float half_height = rh / 2.0f;
    return (px >= rx - half_width && px <= rx + half_width &&
            py >= ry - half_height && py <= ry + half_height);
}

// Check if there's ground beneath a position
static bool is_ground_beneath(const GameState* state, float x, float y, float height) {
    // In our coordinate system, positive y is down
    // The entity position is at the center of the entity
    // So the feet position is at y + height/2
    float feet_y = y + height/2;
    
    // Check a point slightly below the entity's feet
    float check_y = feet_y + 0.2f; // 0.2 meters below the feet
    
    LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Checking for ground beneath (%.2f, %.2f), feet_y=%.2f, check_y=%.2f",
             x, y, feet_y, check_y);
    
    // Check for each ground
    for (int i = 0; i < state->ground_count; i++) {
        const GroundState* ground = &state->grounds[i];
        
        // Calculate ground boundaries
        float ground_left = ground->position_x - ground->width / 2.0f;
        float ground_right = ground->position_x + ground->width / 2.0f;
        float ground_top = ground->position_y - ground->height / 2.0f;
        
        LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Ground %d: bounds=[%.2f, %.2f], top=%.2f",
                i, ground_left, ground_right, ground_top);
        
        // Check if the point is above this ground
        if (x >= ground_left && x <= ground_right && 
            check_y >= ground_top && check_y <= ground_top + 0.3f) {
            LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Found ground beneath (%.2f, %.2f)", x, y);
            return true;
        }
    }
    
    LOG_DEBUG(LOG_CATEGORY_PHYSICS, "No ground found beneath (%.2f, %.2f)", x, y);
    return false;
}

// Debug function to visualize the ground check
static void debug_visualize_ground_check(const GameState* state, float x, float y, float height, bool has_ground) {
    // This function would ideally draw debug visuals, but for now we'll just log
    LOG_INFO(LOG_CATEGORY_PHYSICS, "Ground check at (%.2f, %.2f): %s", 
           x, y, has_ground ? "GROUND FOUND" : "NO GROUND");
}

// Update enemy patrol behavior
static void update_enemy_patrol(Entity* entity, const GameState* state) {
    if (!entity || entity->type != ENTITY_TYPE_ENEMY) {
        return;
    }
    
    // Get the current enemy speed from the debug HUD
    float patrol_speed = debug_hud_get_enemy_speed();
    
    // Log the current speed values
    static float last_logged_speed = 0.0f;
    if (fabs(patrol_speed - last_logged_speed) > 0.1f) {
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Enemy patrol speed updated: %.1f m/s (old velocity: %.1f m/s)", 
                patrol_speed, entity->velocity_x);
        last_logged_speed = patrol_speed;
    }
    
    // Update the enemy's patrol speed
    entity->enemy.patrol_speed = patrol_speed;
    
    // Preserve the direction but update the speed
    float old_velocity = entity->velocity_x;
    if (entity->velocity_x > 0) {
        // Moving right, update speed
        entity->velocity_x = patrol_speed;
    } else if (entity->velocity_x < 0) {
        // Moving left, update speed
        entity->velocity_x = -patrol_speed;
    }
    
    // Log significant velocity changes
    if (fabs(entity->velocity_x - old_velocity) > 0.1f) {
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Enemy velocity changed from %.1f to %.1f m/s", 
                old_velocity, entity->velocity_x);
    }
    
    // Check if we need to change direction due to patrol boundaries
    if (entity->position_x <= entity->enemy.patrol_start_x && entity->velocity_x < 0) {
        // We've reached the left boundary while moving left, so change direction
        entity->velocity_x = patrol_speed;
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Enemy at (%.2f, %.2f) reached left patrol boundary, moving right",
                entity->position_x, entity->position_y);
    } 
    else if (entity->position_x >= entity->enemy.patrol_end_x && entity->velocity_x > 0) {
        // We've reached the right boundary while moving right, so change direction
        entity->velocity_x = -patrol_speed;
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Enemy at (%.2f, %.2f) reached right patrol boundary, moving left",
                entity->position_x, entity->position_y);
    }
    else if (entity->velocity_x == 0) {
        // If the enemy is not moving, start moving right
        entity->velocity_x = patrol_speed;
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Enemy at (%.2f, %.2f) was stationary, now moving right",
                entity->position_x, entity->position_y);
    }
    
    // Check for platform edges
    float look_ahead = ENEMY_LOOK_AHEAD; // Look ahead distance
    float check_x;
    
    if (entity->velocity_x > 0) {
        // Moving right, check ahead to the right
        check_x = entity->position_x + entity->width/2 + look_ahead;
    } else {
        // Moving left, check ahead to the left
        check_x = entity->position_x - entity->width/2 - look_ahead;
    }
    
    // If there's no ground ahead, turn around
    bool has_ground = is_ground_beneath(state, check_x, entity->position_y, entity->height);
    
    // Visualize the ground check
    debug_visualize_ground_check(state, check_x, entity->position_y, entity->height, has_ground);
    
    if (!has_ground) {
        entity->velocity_x = -entity->velocity_x; // Reverse direction
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Enemy at (%.2f, %.2f) reached platform edge, turning around",
                entity->position_x, entity->position_y);
    }
}

// Update a single entity's physics
static void update_entity_physics(Entity* entity, GameState* state, float dt) {
    if (!entity || !entity->is_active) {
        return;
    }
    
    // Store previous position for collision detection
    float prev_x = entity->position_x;
    float prev_y = entity->position_y;
    
    // Apply entity-specific logic
    switch (entity->type) {
        case ENTITY_TYPE_PLAYER:
            // Update player horizontal movement using analog input
            if (fabsf(analog_input.move_x) > 0.01f) {
                // Scale velocity based on analog input (-1.0 to 1.0)
                entity->velocity_x = analog_input.move_x * entity->player.move_speed;
                
                // Update movement flags for animation
                entity->player.is_moving_left = (analog_input.move_x < 0);
                entity->player.is_moving_right = (analog_input.move_x > 0);
                
                // Log significant velocity changes
                static float last_velocity = 0.0f;
                if (fabsf(entity->velocity_x - last_velocity) > 2.0f) {
                    LOG_DEBUG(LOG_CATEGORY_PHYSICS, "Player velocity changed to %.2f m/s (analog: %.2f)", 
                           entity->velocity_x, analog_input.move_x);
                    last_velocity = entity->velocity_x;
                }
            } else {
                // Apply friction to slow down when no input
                entity->velocity_x *= 0.9f;
                entity->player.is_moving_left = false;
                entity->player.is_moving_right = false;
                
                // Stop completely if very slow
                if (fabs(entity->velocity_x) < 0.1f) {
                    entity->velocity_x = 0.0f;
                }
            }
            
            // Apply jump if grounded
            if (analog_input.jump && entity->is_grounded) {
                entity->velocity_y = -entity->player.jump_force;
                entity->is_grounded = false;
                entity->is_jumping = true;
                LOG_INFO(LOG_CATEGORY_PHYSICS, "Player jumped with velocity %.2f m/s", entity->player.jump_force);
            }
            break;
            
        case ENTITY_TYPE_ENEMY:
            break;
            
        default:
            break;
    }
    
    // Apply gravity to non-static entities
    if (!entity->is_static) {
        entity->velocity_y += GRAVITY * dt;
        
        // Apply terminal velocity limit
        if (entity->velocity_y > TERMINAL_VELOCITY) {
            entity->velocity_y = TERMINAL_VELOCITY;
        }
    }
    
    // Update position
    float new_x = entity->position_x + entity->velocity_x * dt;
    float new_y = entity->position_y + entity->velocity_y * dt;
    
    // Reset grounded state
    bool was_grounded = entity->is_grounded;
    entity->is_grounded = false;
    
    // Check ground collision with all ground planes
    float entity_half_width = entity->width / 2.0f;
    float entity_half_height = entity->height / 2.0f;
    
    // Calculate entity's feet position (bottom of entity)
    float entity_feet_y = new_y + entity_half_height;
    
    // Check collision with each ground
    for (int i = 0; i < state->ground_count; i++) {
        GroundState* ground = &state->grounds[i];
        
        // Calculate ground boundaries
        float ground_left = ground->position_x - ground->width / 2.0f;
        float ground_right = ground->position_x + ground->width / 2.0f;
        
        // Calculate the top surface of the ground (important for collision)
        float ground_top = ground->position_y - ground->height / 2.0f;
        
        // Check if entity is horizontally within the ground's bounds
        if (new_x + entity_half_width >= ground_left && 
            new_x - entity_half_width <= ground_right) {
            
            // Calculate entity's feet position in previous frame
            float prev_feet_y = prev_y + entity_half_height;
            
            // Check if entity's feet are at or below the ground's top surface
            // AND the entity was above the ground in the previous frame
            if (entity_feet_y >= ground_top && prev_feet_y <= ground_top) {
                // Place the entity so their feet are exactly on the ground
                new_y = ground_top - entity_half_height;
                entity->velocity_y = 0.0f;
                entity->is_grounded = true;
                entity->is_jumping = false;
                
                if (entity->type == ENTITY_TYPE_PLAYER) {
                    LOG_INFO(LOG_CATEGORY_PHYSICS, "Player landed on ground %d at y=%.2f", i, new_y);
                }
                break;  // Only collide with one ground at a time
            }
        }
    }
    
    // Apply pixel-perfect alignment for horizontal movement to prevent jittering
    // This is especially important when player and enemy move at the same speed
    if (entity->type == ENTITY_TYPE_PLAYER || entity->type == ENTITY_TYPE_ENEMY) {
        // Get the current camera zoom level (pixels per meter)
        const Camera* camera = camera_get_current();
        float zoom = camera->zoom;
        
        // Convert world position to pixel position
        float pixel_x = new_x * zoom;
        
        // Round to nearest pixel
        pixel_x = roundf(pixel_x);
        
        // Convert back to world coordinates
        new_x = pixel_x / zoom;
    }
    
    // Update entity position
    entity->position_x = new_x;
    entity->position_y = new_y;
    
    // Check world boundaries
    if (entity->position_x < -WORLD_WIDTH/2 + entity_half_width) {
        entity->position_x = -WORLD_WIDTH/2 + entity_half_width;
        entity->velocity_x = 0.0f;
    } else if (entity->position_x > WORLD_WIDTH/2 - entity_half_width) {
        entity->position_x = WORLD_WIDTH/2 - entity_half_width;
        entity->velocity_x = 0.0f;
    }
}

// Update physics (to be called by the scheduler at fixed intervals)
void physics_update(double dt, void* user_data) {
    // Get a local copy of the player pointer for debugging
    Entity* player = NULL;
    
    {
        // Begin writing to the game state
        GameState* state = game_state_begin_write();
        
        // Store player pointer for later use
        player = state->player;
        
        // Update all entities
        for (int i = 0; i < state->entity_count; i++) {
            Entity* entity = state->entities[i];
            if (entity && entity->is_active) {
                update_enemy_patrol(entity, state);
                update_entity_physics(entity, state, FIXED_TIME_STEP);
            }
        }
        
        // Finish writing to the game state
        game_state_end_write();
    }
    
    // Check if player has reached the goal (last platform)
    bool goal_reached = game_state_check_win_condition();
    if (goal_reached) {
        // Display a message
        LOG_INFO(LOG_CATEGORY_PHYSICS, "GOAL REACHED! Congratulations! Game will reset shortly...");
    }
    
    // Update the game state (handles delayed reset)
    game_state_update(dt);
    
    // Debug output for physics update (less frequently)
    static double last_debug_time = 0.0;
    static double accumulated_time = 0.0;
    
    accumulated_time += dt;
    if (accumulated_time - last_debug_time > 1.0 && player) {
        LOG_INFO(LOG_CATEGORY_PHYSICS, "Player: pos=(%.2f, %.2f) m, vel=(%.2f, %.2f) m/s, grounded=%d",
               player->position_x, player->position_y,
               player->velocity_x, player->velocity_y,
               player->is_grounded);
        last_debug_time = accumulated_time;
    }
} 