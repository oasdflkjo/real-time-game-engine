#ifndef RENDERER_H
#define RENDERER_H

#include "game_state.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>

// Initialize the renderer
bool renderer_init(int width, int height);

// Shutdown the renderer
void renderer_shutdown(void);

// Update the renderer (to be called by the scheduler)
void renderer_update(double dt, void* user_data);

// Draw the game state
void renderer_draw_game(const GameState* state);

// Check if the window should close
bool renderer_should_close(void);

// Get the window size
void renderer_get_window_size(int* width, int* height);

// Get the GLFW window handle
GLFWwindow* renderer_get_window(void);

#endif // RENDERER_H 