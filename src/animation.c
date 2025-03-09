#include "../include/animation.h"
#include "../include/scheduler.h"
#include "../include/logging.h"
#include "../include/physics.h"
#include <stdio.h>
#include <string.h>

// Animation state
static AnimationState animation_state;

// Animation definitions
#define PLAYER_IDLE_FRAMES 1
#define PLAYER_RUN_FRAMES 4
#define PLAYER_JUMP_FRAMES 2
#define PLAYER_FALL_FRAMES 1

#define PLAYER_IDLE_FRAME_TIME 0.5f  // 0.5 seconds per frame
#define PLAYER_RUN_FRAME_TIME 0.1f   // 0.1 seconds per frame (10 fps)
#define PLAYER_JUMP_FRAME_TIME 0.2f  // 0.2 seconds per frame
#define PLAYER_FALL_FRAME_TIME 0.2f  // 0.2 seconds per frame

// Initialize the animation system
void animation_init(void) {
    // Initialize player animation
    animation_state.player.current_frame = 0;
    animation_state.player.frame_count = PLAYER_IDLE_FRAMES;
    animation_state.player.frame_time = PLAYER_IDLE_FRAME_TIME;
    animation_state.player.time_accumulator = 0.0f;
    animation_state.player.animation_name = "idle";
    
    // Initialize last update time
    animation_state.last_update_time = scheduler_get_time_ms() / 1000.0;
    
    LOG_INFO(LOG_CATEGORY_ANIMATION, "Initialized animation system");
}

// Shutdown the animation system
void animation_shutdown(void) {
    LOG_INFO(LOG_CATEGORY_ANIMATION, "Shutdown animation system");
}

// Determine which animation to play based on player state
void animation_set_player_animation(const PlayerState* player_state) {
    if (!player_state) {
        return;
    }
    
    const char* new_animation = NULL;
    int new_frame_count = 0;
    float new_frame_time = 0.0f;
    
    // Determine which animation to play based on player state
    if (!player_state->is_grounded) {
        if (player_state->velocity_y < 0) {
            // Player is jumping (moving upward)
            new_animation = "jump";
            new_frame_count = PLAYER_JUMP_FRAMES;
            new_frame_time = PLAYER_JUMP_FRAME_TIME;
        } else {
            // Player is falling
            new_animation = "fall";
            new_frame_count = PLAYER_FALL_FRAMES;
            new_frame_time = PLAYER_FALL_FRAME_TIME;
        }
    } else if (player_state->velocity_x != 0.0f) {
        // Player is running
        new_animation = "run";
        new_frame_count = PLAYER_RUN_FRAMES;
        new_frame_time = PLAYER_RUN_FRAME_TIME;
    } else {
        // Player is idle
        new_animation = "idle";
        new_frame_count = PLAYER_IDLE_FRAMES;
        new_frame_time = PLAYER_IDLE_FRAME_TIME;
    }
    
    // If animation changed, reset frame and accumulator
    if (animation_state.player.animation_name != new_animation) {
        animation_state.player.animation_name = new_animation;
        animation_state.player.frame_count = new_frame_count;
        animation_state.player.frame_time = new_frame_time;
        animation_state.player.current_frame = 0;
        animation_state.player.time_accumulator = 0.0f;
        
        LOG_DEBUG(LOG_CATEGORY_ANIMATION, "Player animation changed to %s (%d frames, %.2f sec/frame)",
                 new_animation, new_frame_count, new_frame_time);
    }
}

// Update animations based on game state (to be called by the scheduler)
void animation_update(double dt, void* user_data) {
    // Get the current game state
    const GameState* game_state = game_state_get_read();
    
    // Update last update time
    animation_state.last_update_time = scheduler_get_time_ms() / 1000.0;
    
    // Determine which animation to play based on player state
    animation_set_player_animation(&game_state->player);
    
    // Update player animation frame
    animation_state.player.time_accumulator += (float)dt;
    if (animation_state.player.time_accumulator >= animation_state.player.frame_time) {
        // Advance to next frame
        animation_state.player.current_frame = 
            (animation_state.player.current_frame + 1) % animation_state.player.frame_count;
        
        // Reset accumulator (keep remainder for smoother animation)
        animation_state.player.time_accumulator -= animation_state.player.frame_time;
        
        LOG_DEBUG(LOG_CATEGORY_ANIMATION, "Player animation frame changed to %d/%d",
                 animation_state.player.current_frame + 1, animation_state.player.frame_count);
    }
}

// Get the current animation state for rendering
const AnimationState* animation_get_state(void) {
    return &animation_state;
} 