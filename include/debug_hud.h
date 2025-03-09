#ifndef DEBUG_HUD_H
#define DEBUG_HUD_H

#include "game_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the debug HUD
void debug_hud_init(void);

// Shutdown the debug HUD
void debug_hud_shutdown(void);

// Render the debug HUD
void debug_hud_render(const GameState* state);

// Toggle the debug HUD visibility
void debug_hud_toggle(void);

// Check if the debug HUD is visible
bool debug_hud_is_visible(void);

// Get the enemy speed from the debug HUD
float debug_hud_get_enemy_speed(void);

// Set the initial enemy speed in the debug HUD
void debug_hud_set_enemy_speed(float speed);

// Show a goal reached message
void debug_hud_show_goal_message(void);

#ifdef __cplusplus
}
#endif

#endif // DEBUG_HUD_H 