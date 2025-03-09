#ifndef ANIMATION_H
#define ANIMATION_H

#include "game_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Animation state for a player
typedef struct {
    int current_frame;         // Current animation frame
    int frame_count;           // Total frames in current animation
    float frame_time;          // Time per frame
    float time_accumulator;    // Time accumulated for current animation
    const char* animation_name; // Name of current animation (e.g., "idle", "run", "jump")
} PlayerAnimation;

// Animation state for the entire game
typedef struct {
    PlayerAnimation player;
    // Add other entity animations here as needed
    double last_update_time;   // Time of last animation update
} AnimationState;

// Initialize the animation system
void animation_init(void);

// Shutdown the animation system
void animation_shutdown(void);

// Update animations based on game state (to be called by the scheduler)
void animation_update(double dt, void* user_data);

// Get the current animation state for rendering
const AnimationState* animation_get_state(void);

// Determine which animation to play based on player state
void animation_set_player_animation(const PlayerState* player_state);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_H 