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

#ifdef __cplusplus
}
#endif

#endif // DEBUG_HUD_H 