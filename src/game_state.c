#include "../include/game_state.h"
#include "../include/logging.h"
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
    
    // Set initial player position (in meters)
    for (int i = 0; i < 3; i++) {
        state_buffers[i].player.position_x = 0.0f;  // Center of the world
        state_buffers[i].player.position_y = -1.0f; // 1 meter above the ground (since player is 2m tall)
        state_buffers[i].player.is_grounded = true;
    }
    
    LOG_INFO(LOG_CATEGORY_GAME_STATE, "Initialized with SI units (meters)");
}

// Shutdown the game state system
void game_state_shutdown(void) {
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

// Interpolate between physics states for smooth rendering
// NOTE: This function is kept for compatibility but no longer performs interpolation
// with the fixed time step architecture
void game_state_interpolate(float alpha, GameState* result) {
    if (!result) {
        LOG_ERROR(LOG_CATEGORY_GAME_STATE, "Null result pointer passed to game_state_interpolate");
        return;
    }
    
    EnterCriticalSection(&state_lock);
    const GameState* current = &state_buffers[current_buffer];
    LeaveCriticalSection(&state_lock);
    
    // With fixed time steps, we just use the current state directly
    *result = *current;
    
    // Ensure camera position matches player position
    result->camera_x = result->player.position_x;
    result->camera_y = result->player.position_y;
    
    LOG_DEBUG(LOG_CATEGORY_GAME_STATE, "Using current state directly (fixed time step)");
} 