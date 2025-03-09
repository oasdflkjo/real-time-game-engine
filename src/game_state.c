#include "../include/game_state.h"
#include "../include/logging.h"
#include "../include/entity.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

// Double-buffered game state
static GameState state_buffers[3];  // Current, previous, and write buffer
static int current_buffer = 0;
static int previous_buffer = 1;
static int write_buffer = 2;
static CRITICAL_SECTION state_lock;

// Goal reached state
static struct {
    bool goal_reached;
    double reset_timer;
} goal_state = {false, 0.0};

// Initialize the platforms in the game state
static void initialize_platforms(GameState* state) {
    // Clear existing platforms
    state->ground_count = 0;
    
    // Create 10 platforms with increasing difficulty
    // Platform 1 - Starting platform
    state->grounds[0].position_x = -20.0f;  // Center at x=-20
    state->grounds[0].position_y = 0.5f;    // Center at y=0.5 (ground level)
    state->grounds[0].width = 20.0f;        // 20 meters wide
    state->grounds[0].height = 1.0f;        // 1 meter tall
    state->ground_count++;
    
    // Platform 2 - Small gap
    state->grounds[1].position_x = 5.0f;    // Center at x=5
    state->grounds[1].position_y = 0.5f;    // Same height
    state->grounds[1].width = 15.0f;        // 15 meters wide
    state->grounds[1].height = 1.0f;
    state->ground_count++;
    
    // Platform 3 - Higher platform
    state->grounds[2].position_x = 25.0f;   // Center at x=25
    state->grounds[2].position_y = -1.0f;   // Higher platform
    state->grounds[2].width = 15.0f;
    state->grounds[2].height = 1.0f;
    state->ground_count++;
    
    // Platform 4 - Even higher
    state->grounds[3].position_x = 45.0f;
    state->grounds[3].position_y = -2.5f;
    state->grounds[3].width = 15.0f;
    state->grounds[3].height = 1.0f;
    state->ground_count++;
    
    // Platform 5 - Moving down
    state->grounds[4].position_x = 65.0f;
    state->grounds[4].position_y = 0.0f;
    state->grounds[4].width = 15.0f;
    state->grounds[4].height = 1.0f;
    state->ground_count++;
    
    // Platform 6 - Small platform
    state->grounds[5].position_x = 85.0f;
    state->grounds[5].position_y = 0.0f;
    state->grounds[5].width = 10.0f;
    state->grounds[5].height = 1.0f;
    state->ground_count++;
    
    // Platform 7 - Higher again
    state->grounds[6].position_x = 105.0f;
    state->grounds[6].position_y = -3.0f;
    state->grounds[6].width = 10.0f;
    state->grounds[6].height = 1.0f;
    state->ground_count++;
    
    // Platform 8 - Very small platform
    state->grounds[7].position_x = 125.0f;
    state->grounds[7].position_y = -3.0f;
    state->grounds[7].width = 8.0f;
    state->grounds[7].height = 1.0f;
    state->ground_count++;
    
    // Platform 9 - Second to last
    state->grounds[8].position_x = 145.0f;
    state->grounds[8].position_y = -1.0f;
    state->grounds[8].width = 10.0f;
    state->grounds[8].height = 1.0f;
    state->ground_count++;
    
    // Platform 10 - Final platform (goal)
    state->grounds[9].position_x = 165.0f;
    state->grounds[9].position_y = 0.0f;
    state->grounds[9].width = 20.0f;
    state->grounds[9].height = 1.0f;
    state->ground_count++;
    
    // Log ground plane positions for debugging
    for (int i = 0; i < state->ground_count; i++) {
        GroundState* ground = &state->grounds[i];
        float ground_top = ground->position_y - ground->height / 2.0f;
        LOG_INFO(LOG_CATEGORY_GAME_STATE, "Ground %d: pos=(%.2f, %.2f), size=(%.2f, %.2f), top=%.2f",
               i, ground->position_x, ground->position_y, ground->width, ground->height, ground_top);
    }
}

// Initialize the game state system
void game_state_init(void) {
    // Initialize critical section
    InitializeCriticalSection(&state_lock);
    
    // Clear all state buffers
    memset(state_buffers, 0, sizeof(state_buffers));
    
    for (int i = 0; i < 3; i++) {
        // Initialize entity array
        state_buffers[i].entity_count = 0;
        for (int j = 0; j < MAX_ENTITIES; j++) {
            state_buffers[i].entities[j] = NULL;
        }
        
        // Create player entity at the starting position
        Entity* player = entity_create_player(-20.0f, -1.0f);
        game_state_add_entity(player);
        state_buffers[i].player = player;
        
        // Create one enemy entity with a patrol distance
        Entity* enemy = entity_create_enemy(10.0f, -1.0f, 0.0f, 20.0f);
        game_state_add_entity(enemy);
        
        // Initialize platforms
        initialize_platforms(&state_buffers[i]);
    }
    
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Initialized with SI units (meters)");
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Created %d entities", state_buffers[0].entity_count);
}

// Shutdown the game state system
void game_state_shutdown(void) {
    // Free all entities
    game_state_clear_entities();
    
    DeleteCriticalSection(&state_lock);
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Shutdown");
}

// Get a read-only pointer to the current game state
const GameState* game_state_get_read(void) {
    const GameState* state;
    EnterCriticalSection(&state_lock);
    state = &state_buffers[current_buffer];
    LeaveCriticalSection(&state_lock);
    return state;
}

// Begin writing to the game state (returns a writable pointer)
GameState* game_state_begin_write(void) {
    // Copy the current state to the write buffer
    EnterCriticalSection(&state_lock);
    memcpy(&state_buffers[write_buffer], &state_buffers[current_buffer], sizeof(GameState));
    LeaveCriticalSection(&state_lock);
    
    LOG_DEBUG(LOG_CATEGORY_GAME_STATE, "Begin writing to buffer %d", write_buffer);
    return &state_buffers[write_buffer];
}

// Finish writing to the game state (swaps the buffers)
void game_state_end_write(void) {
    EnterCriticalSection(&state_lock);
    
    // Update the previous buffer index
    previous_buffer = current_buffer;
    
    // Swap the current and write buffers
    current_buffer = write_buffer;
    
    // Update the write buffer index
    write_buffer = previous_buffer;
    
    LeaveCriticalSection(&state_lock);
    
    LOG_DEBUG(LOG_CATEGORY_GAME_STATE, "End writing, current=%d, previous=%d, write=%d", 
             current_buffer, previous_buffer, write_buffer);
}

// Add an entity to the game state
Entity* game_state_add_entity(Entity* entity) {
    if (!entity) {
        LOG_ERROR(LOG_CATEGORY_GAME_STATE, "Cannot add NULL entity");
        return NULL;
    }
    
    GameState* state = &state_buffers[current_buffer];
    
    // Check if we have room for more entities
    if (state->entity_count >= MAX_ENTITIES) {
        LOG_ERROR(LOG_CATEGORY_GAME_STATE, "Cannot add entity: maximum entity count reached");
        entity_destroy(entity);
        return NULL;
    }
    
    // Add the entity to the array
    state->entities[state->entity_count] = entity;
    state->entity_count++;
    
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Added entity of type %d at index %d", 
           entity->type, state->entity_count - 1);
    
    return entity;
}

// Remove an entity from the game state
void game_state_remove_entity(Entity* entity) {
    if (!entity) {
        return;
    }
    
    GameState* state = &state_buffers[current_buffer];
    
    // Find the entity in the array
    int index = -1;
    for (int i = 0; i < state->entity_count; i++) {
        if (state->entities[i] == entity) {
            index = i;
            break;
        }
    }
    
    // If the entity was found, remove it
    if (index >= 0) {
        // If this is the player entity, clear the player reference
        if (entity == state->player) {
            state->player = NULL;
        }
        
        // Destroy the entity
        entity_destroy(entity);
        
        // Shift all entities after this one down by one
        for (int i = index; i < state->entity_count - 1; i++) {
            state->entities[i] = state->entities[i + 1];
        }
        
        // Clear the last entity slot
        state->entities[state->entity_count - 1] = NULL;
        state->entity_count--;
        
        LOG_INFO(LOG_CATEGORY_GAME_STATE, "Removed entity at index %d", index);
    }
}

// Clear all entities from the game state
void game_state_clear_entities(void) {
    GameState* state = &state_buffers[current_buffer];
    
    // First, store the player pointer
    Entity* player_ptr = state->player;
    
    // Reset player reference first to prevent accessing it after freeing
    state->player = NULL;
    
    // Create a copy of the entity count since we'll be modifying it
    int entity_count = state->entity_count;
    
    // Reset entity count before destroying entities
    state->entity_count = 0;
    
    // Destroy all entities except the player (we'll handle that last)
    for (int i = 0; i < entity_count; i++) {
        if (state->entities[i] && state->entities[i] != player_ptr) {
            entity_destroy(state->entities[i]);
            state->entities[i] = NULL;
        }
    }
    
    // Now destroy the player entity last
    if (player_ptr) {
        entity_destroy(player_ptr);
    }
    
    // Make sure all entity pointers are NULL
    for (int i = 0; i < MAX_ENTITIES; i++) {
        state->entities[i] = NULL;
    }
    
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Cleared all entities");
}

// Check if player has reached the last platform
bool game_state_check_win_condition(void) {
    const GameState* state = game_state_get_read();
    
    // If we're already in the goal reached state, check if it's time to reset
    if (goal_state.goal_reached) {
        return false;  // Don't trigger another win while waiting to reset
    }
    
    // Make sure we have a player and at least one platform
    if (!state->player || state->ground_count == 0) {
        return false;
    }
    
    // Get the last platform (goal)
    const GroundState* last_platform = &state->grounds[state->ground_count - 1];
    
    // Calculate platform boundaries
    float platform_left = last_platform->position_x - last_platform->width / 2.0f;
    float platform_right = last_platform->position_x + last_platform->width / 2.0f;
    
    // Check if player is on the last platform
    if (state->player->position_x >= platform_left && 
        state->player->position_x <= platform_right && 
        state->player->is_grounded) {
        
        LOG_INFO(LOG_CATEGORY_GAME_STATE, "Player reached the goal platform! Game will reset shortly.");
        
        // Set the goal reached state
        goal_state.goal_reached = true;
        goal_state.reset_timer = 2.0;  // 2 seconds delay before reset
        
        return true;
    }
    
    return false;
}

// Update the goal state timer
void game_state_update(double dt) {
    if (goal_state.goal_reached) {
        goal_state.reset_timer -= dt;
        
        if (goal_state.reset_timer <= 0.0) {
            // Time to reset
            LOG_INFO(LOG_CATEGORY_GAME_STATE, "Reset timer expired, resetting game state");
            
            // Reset the goal state first to prevent any callbacks during reset
            goal_state.goal_reached = false;
            
            // Now it's safe to reset the game state
            game_state_reset();
        }
    }
}

// Reset the game state to the initial state
void game_state_reset(void) {
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Resetting game state");
    
    // Lock the state to prevent concurrent access
    EnterCriticalSection(&state_lock);
    
    // Store the old player and enemy pointers so we can properly free them
    Entity* old_player = state_buffers[current_buffer].player;
    
    // First, null out all entity pointers to prevent accessing freed memory
    state_buffers[current_buffer].player = NULL;
    for (int i = 0; i < state_buffers[current_buffer].entity_count; i++) {
        state_buffers[current_buffer].entities[i] = NULL;
    }
    state_buffers[current_buffer].entity_count = 0;
    
    // Do the same for the other buffers
    state_buffers[previous_buffer].player = NULL;
    state_buffers[previous_buffer].entity_count = 0;
    state_buffers[write_buffer].player = NULL;
    state_buffers[write_buffer].entity_count = 0;
    
    // Now it's safe to free the entities
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (state_buffers[current_buffer].entities[i] && 
            state_buffers[current_buffer].entities[i] != old_player) {
            entity_destroy(state_buffers[current_buffer].entities[i]);
            state_buffers[current_buffer].entities[i] = NULL;
        }
        
        // Clear the other buffers too
        state_buffers[previous_buffer].entities[i] = NULL;
        state_buffers[write_buffer].entities[i] = NULL;
    }
    
    // Free the player last
    if (old_player) {
        entity_destroy(old_player);
    }
    
    // Create player entity at the starting position
    Entity* player = entity_create_player(-20.0f, -1.0f);
    
    // Add the player to all buffers
    state_buffers[current_buffer].entities[0] = player;
    state_buffers[current_buffer].entity_count = 1;
    state_buffers[current_buffer].player = player;
    
    state_buffers[previous_buffer].entities[0] = player;
    state_buffers[previous_buffer].entity_count = 1;
    state_buffers[previous_buffer].player = player;
    
    state_buffers[write_buffer].entities[0] = player;
    state_buffers[write_buffer].entity_count = 1;
    state_buffers[write_buffer].player = player;
    
    // Create one enemy entity with a patrol distance
    Entity* enemy = entity_create_enemy(10.0f, -1.0f, 0.0f, 20.0f);
    
    // Add the enemy to all buffers
    state_buffers[current_buffer].entities[1] = enemy;
    state_buffers[current_buffer].entity_count = 2;
    
    state_buffers[previous_buffer].entities[1] = enemy;
    state_buffers[previous_buffer].entity_count = 2;
    
    state_buffers[write_buffer].entities[1] = enemy;
    state_buffers[write_buffer].entity_count = 2;
    
    // Re-initialize platforms for all buffers
    initialize_platforms(&state_buffers[current_buffer]);
    initialize_platforms(&state_buffers[previous_buffer]);
    initialize_platforms(&state_buffers[write_buffer]);
    
    // Unlock the state
    LeaveCriticalSection(&state_lock);
    
    // Reset goal state
    goal_state.goal_reached = false;
    
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Game state reset complete");
} 