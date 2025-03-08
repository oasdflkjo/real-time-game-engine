#include "../include/renderer.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <stdlib.h>

// GLFW window
static GLFWwindow* window = NULL;

// Window dimensions
static int window_width = 800;
static int window_height = 600;

// Error callback for GLFW
static void error_callback(int error, const char* description) {
    fprintf(stderr, "[GLFW] Error %d: %s\n", error, description);
}

// Key callback for GLFW
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

// Initialize the renderer
bool renderer_init(int width, int height) {
    // Set window dimensions
    window_width = width;
    window_height = height;
    
    // Set error callback
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "[Renderer] Failed to initialize GLFW\n");
        return false;
    }
    
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    window = glfwCreateWindow(window_width, window_height, "Real-Time Game Engine", NULL, NULL);
    if (!window) {
        fprintf(stderr, "[Renderer] Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }
    
    // Set key callback
    glfwSetKeyCallback(window, key_callback);
    
    // Make OpenGL context current
    glfwMakeContextCurrent(window);
    
    // Enable vsync
    glfwSwapInterval(1);
    
    // Set viewport
    glViewport(0, 0, window_width, window_height);
    
    // Set clear color (sky blue)
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
    
    printf("[Renderer] Initialized with window size %dx%d\n", window_width, window_height);
    
    return true;
}

// Shutdown the renderer
void renderer_shutdown(void) {
    if (window) {
        glfwDestroyWindow(window);
        window = NULL;
    }
    
    glfwTerminate();
    
    printf("[Renderer] Shutdown\n");
}

// Begin a new frame
void renderer_begin_frame(void) {
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

// End the current frame and swap buffers
void renderer_end_frame(void) {
    // Swap buffers
    glfwSwapBuffers(window);
}

// Draw a rectangle at the specified position
static void draw_rectangle(float x, float y, float width, float height, float r, float g, float b) {
    // Calculate coordinates
    float x1 = x;
    float y1 = y;
    float x2 = x + width;
    float y2 = y + height;
    
    // Draw rectangle
    glBegin(GL_QUADS);
    glColor3f(r, g, b);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

// Draw a line
static void draw_line(float x1, float y1, float x2, float y2, float r, float g, float b) {
    glBegin(GL_LINES);
    glColor3f(r, g, b);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

// Draw a grid
static void draw_grid(float grid_size, float r, float g, float b) {
    // Draw vertical lines
    for (float x = 0; x <= window_width; x += grid_size) {
        draw_line(x, 0, x, window_height, r, g, b);
    }
    
    // Draw horizontal lines
    for (float y = 0; y <= window_height; y += grid_size) {
        draw_line(0, y, window_width, y, r, g, b);
    }
}

// Render the game state
void renderer_draw_game(const GameState* state) {
    if (!state) {
        return;
    }
    
    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, window_height, 0.0, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw grid
    draw_grid(50.0f, 0.8f, 0.8f, 0.8f);
    
    // Draw ground
    draw_rectangle(
        0.0f,
        window_height - 50.0f,
        window_width,
        50.0f,
        0.0f, 0.5f, 0.0f
    );
    
    // Draw player with outline
    // Black outline
    draw_rectangle(
        state->player.position_x - 22.0f,
        state->player.position_y - 42.0f,
        44.0f,
        44.0f,
        0.0f, 0.0f, 0.0f
    );
    
    // Red player
    draw_rectangle(
        state->player.position_x - 20.0f,
        state->player.position_y - 40.0f,
        40.0f,
        40.0f,
        1.0f, 0.0f, 0.0f
    );
    
    // Draw player eyes (to show direction)
    draw_rectangle(
        state->player.position_x - 10.0f,
        state->player.position_y - 30.0f,
        5.0f,
        5.0f,
        1.0f, 1.0f, 1.0f
    );
    
    draw_rectangle(
        state->player.position_x + 5.0f,
        state->player.position_y - 30.0f,
        5.0f,
        5.0f,
        1.0f, 1.0f, 1.0f
    );
    
    // Print player position to console for debugging
    printf("Player position: X=%.1f, Y=%.1f\n", state->player.position_x, state->player.position_y);
}

// Check if the window should close
bool renderer_should_close(void) {
    return glfwWindowShouldClose(window);
}

// Get window dimensions
void renderer_get_window_size(int* width, int* height) {
    if (width) *width = window_width;
    if (height) *height = window_height;
}

// Process input events
void renderer_process_input(void) {
    glfwPollEvents();
} 