#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <stdbool.h>
#include "entity.h"

// Maximum number of ground planes
#define MAX_GROUND_PLANES 10

// Maximum number of entities
#define MAX_ENTITIES 100

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
    Entity* entities[MAX_ENTITIES];
    int entity_count;
    
    // Reference to the player entity (for quick access)
    Entity* player;
    
    GroundState grounds[MAX_GROUND_PLANES];
    int ground_count;
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

// Entity management functions
Entity* game_state_add_entity(Entity* entity);
void game_state_remove_entity(Entity* entity);
void game_state_clear_entities(void);

#endif // GAME_STATE_H 