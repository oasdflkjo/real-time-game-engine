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
        // Initialize player
        state_buffers[i].player.position_x = -15.0f;  // Start on the left platform
        state_buffers[i].player.position_y = -1.0f;   // 1 meter above the ground (since player is 2m tall)
        state_buffers[i].player.is_grounded = true;
        
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