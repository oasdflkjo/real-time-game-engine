#include "../include/renderer.h"
#include "../include/camera.h"
#include "../include/logging.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <stdlib.h>

// GLFW window
static GLFWwindow* window = NULL;

// Window dimensions
static int window_width = 800;
static int window_height = 600;

// Pixels per meter (for SI unit conversion)
#define PIXELS_PER_METER 50.0f

// Error callback for GLFW
static void error_callback(int error, const char* description) {
    LOG_ERROR(LOG_CATEGORY_RENDERER, "GLFW Error %d: %s", error, description);
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
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to initialize GLFW");
        return false;
    }
    
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    window = glfwCreateWindow(window_width, window_height, "Real-Time Game Engine", NULL, NULL);
    if (!window) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to create GLFW window");
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
    
    // Set clear color (black)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Initialized with window size %dx%d (%.2f pixels per meter)", 
           window_width, window_height, PIXELS_PER_METER);
    
    return true;
}

// Shutdown the renderer
void renderer_shutdown(void) {
    if (window) {
        glfwDestroyWindow(window);
        window = NULL;
    }
    
    glfwTerminate();
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Shutdown");
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

// Draw a rectangle at the specified position in screen coordinates
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

// Draw a rectangle at the specified position in world coordinates
static void draw_world_rectangle(const Camera* camera, float x, float y, float width, float height, float r, float g, float b) {
    float screen_x1, screen_y1, screen_x2, screen_y2;
    
    // Convert world coordinates to screen coordinates
    camera_world_to_screen(camera, x - width/2, y - height/2, &screen_x1, &screen_y1);
    camera_world_to_screen(camera, x + width/2, y + height/2, &screen_x2, &screen_y2);
    
    // In OpenGL, Y increases upward, but our screen coordinates have Y increasing downward
    // So we need to flip the Y coordinates
    float temp = screen_y1;
    screen_y1 = window_height - screen_y2;
    screen_y2 = window_height - temp;
    
    // Draw rectangle
    glBegin(GL_QUADS);
    glColor3f(r, g, b);
    glVertex2f(screen_x1, screen_y1);
    glVertex2f(screen_x2, screen_y1);
    glVertex2f(screen_x2, screen_y2);
    glVertex2f(screen_x1, screen_y2);
    glEnd();
}

// Draw a line in screen coordinates
static void draw_line(float x1, float y1, float x2, float y2, float r, float g, float b) {
    glBegin(GL_LINES);
    glColor3f(r, g, b);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

// Draw a line in world coordinates
static void draw_world_line(const Camera* camera, float x1, float y1, float x2, float y2, float r, float g, float b) {
    float screen_x1, screen_y1, screen_x2, screen_y2;
    
    // Convert world coordinates to screen coordinates
    camera_world_to_screen(camera, x1, y1, &screen_x1, &screen_y1);
    camera_world_to_screen(camera, x2, y2, &screen_x2, &screen_y2);
    
    // In OpenGL, Y increases upward, but our screen coordinates have Y increasing downward
    // So we need to flip the Y coordinates
    screen_y1 = window_height - screen_y1;
    screen_y2 = window_height - screen_y2;
    
    // Draw line
    glBegin(GL_LINES);
    glColor3f(r, g, b);
    glVertex2f(screen_x1, screen_y1);
    glVertex2f(screen_x2, screen_y2);
    glEnd();
}

// Draw a grid in world coordinates
static void draw_world_grid(const Camera* camera, float grid_size, float r, float g, float b) {
    // Calculate the visible area in world coordinates
    float world_left, world_top, world_right, world_bottom;
    camera_screen_to_world(camera, 0, 0, &world_left, &world_top);
    camera_screen_to_world(camera, window_width, window_height, &world_right, &world_bottom);
    
    // Swap top and bottom because of coordinate system
    float temp = world_top;
    world_top = world_bottom;
    world_bottom = temp;
    
    // Round to nearest grid line
    float start_x = floor(world_left / grid_size) * grid_size;
    float start_y = floor(world_bottom / grid_size) * grid_size;
    float end_x = ceil(world_right / grid_size) * grid_size;
    float end_y = ceil(world_top / grid_size) * grid_size;
    
    // Draw vertical lines
    for (float x = start_x; x <= end_x; x += grid_size) {
        draw_world_line(camera, x, start_y, x, end_y, r, g, b);
    }
    
    // Draw horizontal lines
    for (float y = start_y; y <= end_y; y += grid_size) {
        draw_world_line(camera, start_x, y, end_x, y, r, g, b);
    }
    
    // Draw coordinate axes with different colors
    draw_world_line(camera, -100.0f, 0.0f, 100.0f, 0.0f, 1.0f, 0.0f, 0.0f); // X-axis (red)
    draw_world_line(camera, 0.0f, -100.0f, 0.0f, 100.0f, 0.0f, 1.0f, 0.0f); // Y-axis (green)
}

// Render the game state
void renderer_draw_game(const GameState* state) {
    if (!state) {
        return;
    }
    
    // Get the current camera
    const Camera* camera = camera_get_current();
    
    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, 0.0, window_height, -1.0, 1.0);  // Note: Y-axis flipped to have origin at bottom-left
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw world grid (1 meter spacing)
    draw_world_grid(camera, 1.0f, 0.3f, 0.3f, 0.3f);  // Lighter gray for better visibility
    
    // Draw ground (green rectangle at y=0)
    float ground_width = 100.0f; // 100 meters wide
    float ground_height = 1.0f;  // 1 meter tall
    draw_world_rectangle(camera, 0.0f, 0.5f, ground_width, ground_height, 0.0f, 0.7f, 0.0f);  // Brighter green
    
    // Draw player (1x2 meter red rectangle)
    // Black outline
    draw_world_rectangle(camera, state->player.position_x, state->player.position_y, 1.1f, 2.1f, 0.3f, 0.3f, 0.3f);
    
    // Red player
    draw_world_rectangle(camera, state->player.position_x, state->player.position_y, 1.0f, 2.0f, 1.0f, 0.2f, 0.2f);  // Brighter red
    
    // Draw player eyes (to show direction)
    draw_world_rectangle(camera, state->player.position_x - 0.2f, state->player.position_y - 0.5f, 0.2f, 0.2f, 1.0f, 1.0f, 1.0f);
    draw_world_rectangle(camera, state->player.position_x + 0.2f, state->player.position_y - 0.5f, 0.2f, 0.2f, 1.0f, 1.0f, 1.0f);
    
    // Draw coordinate axes with different colors
    draw_world_line(camera, -100.0f, 0.0f, 100.0f, 0.0f, 1.0f, 0.0f, 0.0f); // X-axis (red)
    draw_world_line(camera, 0.0f, -100.0f, 0.0f, 100.0f, 0.0f, 1.0f, 0.0f); // Y-axis (green)
    
    // Draw HUD (screen coordinates)
    char position_text[64];
    sprintf(position_text, "Player: (%.2f, %.2f) m", state->player.position_x, state->player.position_y);
    
    // Print player position to console for debugging (less frequently)
    static double last_print_time = 0.0;
    if (state->game_time - last_print_time > 1.0) {
        LOG_INFO(LOG_CATEGORY_RENDERER, "Player: (%.2f, %.2f) m", state->player.position_x, state->player.position_y);
        last_print_time = state->game_time;
    }
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