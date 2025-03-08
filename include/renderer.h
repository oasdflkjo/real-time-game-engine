#ifndef RENDERER_H
#define RENDERER_H

#include "game_state.h"

// Initialize the renderer
bool renderer_init(int window_width, int window_height);

// Shutdown the renderer
void renderer_shutdown(void);

// Begin a new frame
void renderer_begin_frame(void);

// End the current frame and swap buffers
void renderer_end_frame(void);

// Render the game state
void renderer_draw_game(const GameState* state);

// Check if the window should close
bool renderer_should_close(void);

// Get window dimensions
void renderer_get_window_size(int* width, int* height);

// Process input events
void renderer_process_input(void);

#endif // RENDERER_H 