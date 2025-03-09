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
        
        // Create player entity
        Entity* player = entity_create_player(-15.0f, -1.0f);
        game_state_add_entity(player);
        state_buffers[i].player = player;
        
        // Create one enemy entity with a long patrol distance
        Entity* enemy = entity_create_enemy(10.0f, -1.0f, -10.0f, 30.0f);
        game_state_add_entity(enemy);
        
        // Initialize ground planes
        state_buffers[i].ground_count = 3;
        
        // Left platform - Note: For a 1m tall ground, the top surface is at y=-0.5
        state_buffers[i].grounds[0].position_x = -20.0f;  // Center at x=-20
        state_buffers[i].grounds[0].position_y = 0.5f;    // Center at y=0.5 (ground level)
        state_buffers[i].grounds[0].width = 20.0f;        // 20 meters wide
        state_buffers[i].grounds[0].height = 1.0f;        // 1 meter tall
        
        // Gap (no ground here)
        
        // Right platform
        state_buffers[i].grounds[1].position_x = 20.0f;   // Center at x=20
        state_buffers[i].grounds[1].position_y = 0.5f;    // Center at y=0.5 (ground level)
        state_buffers[i].grounds[1].width = 20.0f;        // 20 meters wide
        state_buffers[i].grounds[1].height = 1.0f;        // 1 meter tall
        
        // Far platform (higher)
        state_buffers[i].grounds[2].position_x = 50.0f;   // Center at x=50
        state_buffers[i].grounds[2].position_y = -2.0f;   // Center at y=-2 (higher platform)
        state_buffers[i].grounds[2].width = 20.0f;        // 20 meters wide
        state_buffers[i].grounds[2].height = 1.0f;        // 1 meter tall
    }
    
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Initialized with SI units (meters)");
    
    // Log ground plane positions for debugging
    for (int i = 0; i < state_buffers[0].ground_count; i++) {
        GroundState* ground = &state_buffers[0].grounds[i];
        float ground_top = ground->position_y - ground->height / 2.0f;
        LOG_INFO(LOG_CATEGORY_GAME_STATE, "Ground %d: pos=(%.2f, %.2f), size=(%.2f, %.2f), top=%.2f",
               i, ground->position_x, ground->position_y, ground->width, ground->height, ground_top);
    }
    
    // Log entity information
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
    
    // Destroy all entities
    for (int i = 0; i < state->entity_count; i++) {
        if (state->entities[i]) {
            entity_destroy(state->entities[i]);
            state->entities[i] = NULL;
        }
    }
    
    // Reset entity count and player reference
    state->entity_count = 0;
    state->player = NULL;
    
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Cleared all entities");
} 