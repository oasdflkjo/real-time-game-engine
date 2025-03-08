#include "../include/game_state.h"
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
    
    // Set initial player position
    for (int i = 0; i < 3; i++) {
        state_buffers[i].player.position_x = 100.0f;
        state_buffers[i].player.position_y = 100.0f;
        state_buffers[i].player.is_grounded = true;
    }
    
    printf("[GameState] Initialized with double-buffering\n");
}

// Shutdown the game state system
void game_state_shutdown(void) {
    DeleteCriticalSection(&state_lock);
    printf("[GameState] Shutdown\n");
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
}

// Interpolate between physics states for smooth rendering
void game_state_interpolate(float alpha, GameState* result) {
    if (!result) {
        return;
    }
    
    EnterCriticalSection(&state_lock);
    const GameState* current = &state_buffers[current_buffer];
    const GameState* previous = &state_buffers[previous_buffer];
    LeaveCriticalSection(&state_lock);
    
    // Clamp alpha to [0, 1]
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    
    // Interpolate player position
    result->player.position_x = previous->player.position_x + 
                               (current->player.position_x - previous->player.position_x) * alpha;
    result->player.position_y = previous->player.position_y + 
                               (current->player.position_y - previous->player.position_y) * alpha;
    
    // Copy non-interpolated values
    result->player.velocity_x = current->player.velocity_x;
    result->player.velocity_y = current->player.velocity_y;
    result->player.is_jumping = current->player.is_jumping;
    result->player.is_grounded = current->player.is_grounded;
    
    // Interpolate camera position
    result->camera_x = previous->camera_x + (current->camera_x - previous->camera_x) * alpha;
    result->camera_y = previous->camera_y + (current->camera_y - previous->camera_y) * alpha;
    
    // Use current game time
    result->game_time = current->game_time;
} 