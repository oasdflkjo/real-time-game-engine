#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

// Maximum number of ground planes
#define MAX_GROUND_PLANES 10

// Player state
typedef struct {
    float position_x;
    float position_y;
    float velocity_x;
    float velocity_y;
    bool is_jumping;
    bool is_grounded;
} PlayerState;

// Ground state
typedef struct {
    float position_x;  // Center position X
    float position_y;  // Center position Y
    float width;       // Width in meters
    float height;      // Height in meters
} GroundState;

// Game world state - pure game logic, no rendering concepts
typedef struct {
    PlayerState player;
    GroundState grounds[MAX_GROUND_PLANES];
    int ground_count;
    // Add other game entities here as needed
} GameState;

// Initialize the game state system
void game_state_init(void);

// Shutdown the game state system
void game_state_shutdown(void);

// Get a read-only pointer to the current game state
const GameState* game_state_get_read(void);

// Begin writing to the game state (returns a writable pointer)
GameState* game_state_begin_write(void);

// Finish writing to the game state (swaps the buffers)
void game_state_end_write(void);

#endif // GAME_STATE_H 