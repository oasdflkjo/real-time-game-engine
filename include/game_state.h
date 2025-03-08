#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>

// Player state
typedef struct {
    float position_x;
    float position_y;
    float velocity_x;
    float velocity_y;
    bool is_jumping;
    bool is_grounded;
} PlayerState;

// Game world state
typedef struct {
    PlayerState player;
    float camera_x;
    float camera_y;
    double game_time;
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

// Interpolate between physics states for smooth rendering
void game_state_interpolate(float alpha, GameState* result);

#endif // GAME_STATE_H 