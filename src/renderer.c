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

// Fullscreen mode
static bool is_fullscreen = true;

// VSync mode
typedef enum {
    VSYNC_OFF = 0,       // VSync disabled (may cause tearing)
    VSYNC_ON = 1,        // Standard VSync (may limit FPS to refresh rate)
    VSYNC_ADAPTIVE = -1  // Adaptive VSync (FreeSync/G-Sync)
} VSyncMode;

static VSyncMode vsync_mode = VSYNC_ADAPTIVE;

// Pixels per meter (for SI unit conversion)
#define PIXELS_PER_METER 50.0f

// Frame rate tracking
static struct {
    double last_time;
    int frames;
    double fps;
    char fps_text[32];
} frame_counter = {0};

// Error callback for GLFW
static void error_callback(int error, const char* description) {
    LOG_ERROR(LOG_CATEGORY_RENDERER, "GLFW Error %d: %s", error, description);
}

// Window resize callback
static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Update viewport
    glViewport(0, 0, width, height);
    
    // Update cached window dimensions
    window_width = width;
    window_height = height;
    
    // Update camera dimensions
    const Camera* camera = camera_get_current();
    if (camera) {
        camera_update_dimensions((float)width, (float)height);
    }
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Window resized to %dx%d", width, height);
}

// Key callback for GLFW
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    
    // Toggle fullscreen with F11
    if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        is_fullscreen = !is_fullscreen;
        
        if (is_fullscreen) {
            // Get the primary monitor
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            
            // Set window to fullscreen
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            
            // Update window dimensions
            window_width = mode->width;
            window_height = mode->height;
            
            LOG_INFO(LOG_CATEGORY_RENDERER, "Switched to fullscreen mode: %dx%d @ %dHz", 
                   mode->width, mode->height, mode->refreshRate);
        } else {
            // Set window to windowed mode (centered on screen)
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            
            // Use a smaller window size for windowed mode
            int windowed_width = 1280;
            int windowed_height = 720;
            
            // Center the window on the monitor
            int x_pos = (mode->width - windowed_width) / 2;
            int y_pos = (mode->height - windowed_height) / 2;
            
            glfwSetWindowMonitor(window, NULL, x_pos, y_pos, windowed_width, windowed_height, 0);
            
            // Update window dimensions
            window_width = windowed_width;
            window_height = windowed_height;
            
            LOG_INFO(LOG_CATEGORY_RENDERER, "Switched to windowed mode: %dx%d", 
                   windowed_width, windowed_height);
        }
        
        // Update viewport
        glViewport(0, 0, window_width, window_height);
        
        // Re-apply VSync setting after changing display mode
        glfwSwapInterval(vsync_mode);
    }
    
    // Toggle VSync modes with F10
    if (key == GLFW_KEY_F10 && action == GLFW_PRESS) {
        // Cycle through VSync modes
        switch (vsync_mode) {
            case VSYNC_OFF:
                vsync_mode = VSYNC_ON;
                LOG_INFO(LOG_CATEGORY_RENDERER, "VSync: ON (Standard)");
                break;
            case VSYNC_ON:
                vsync_mode = VSYNC_ADAPTIVE;
                LOG_INFO(LOG_CATEGORY_RENDERER, "VSync: ADAPTIVE (FreeSync/G-Sync)");
                break;
            case VSYNC_ADAPTIVE:
                vsync_mode = VSYNC_OFF;
                LOG_INFO(LOG_CATEGORY_RENDERER, "VSync: OFF");
                break;
        }
        
        // Apply the new VSync setting
        glfwSwapInterval(vsync_mode);
    }
}

// Initialize the renderer
bool renderer_init(int width, int height) {
    // Set window dimensions (these will be overridden in fullscreen mode)
    window_width = width;
    window_height = height;
    
    // Set error callback
    glfwSetErrorCallback(error_callback);
    
    // Initialize GLFW
    if (!glfwInit()) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to initialize GLFW");
        return false;
    }
    
    // Get the primary monitor for fullscreen
    GLFWmonitor* monitor = NULL;
    int monitor_x = 0, monitor_y = 0;
    
    if (is_fullscreen) {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        // Use the monitor's native resolution in fullscreen mode
        window_width = mode->width;
        window_height = mode->height;
        
        LOG_INFO(LOG_CATEGORY_RENDERER, "Using fullscreen mode: %dx%d @ %dHz", 
               mode->width, mode->height, mode->refreshRate);
    } else {
        // For windowed mode, center the window on the monitor
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primary);
        
        // Center the window
        monitor_x = (mode->width - window_width) / 2;
        monitor_y = (mode->height - window_height) / 2;
        
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hide window until positioned
    }
    
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    window = glfwCreateWindow(window_width, window_height, "Real-Time Game Engine", monitor, NULL);
    if (!window) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to create GLFW window");
        glfwTerminate();
        return false;
    }
    
    // Position windowed mode window
    if (!is_fullscreen) {
        glfwSetWindowPos(window, monitor_x, monitor_y);
        glfwShowWindow(window);
    }
    
    // Set callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // Make OpenGL context current
    glfwMakeContextCurrent(window);
    
    // Set VSync mode (adaptive for FreeSync/G-Sync)
    glfwSwapInterval(vsync_mode);
    
    const char* vsync_str;
    switch (vsync_mode) {
        case VSYNC_OFF: vsync_str = "OFF"; break;
        case VSYNC_ON: vsync_str = "ON"; break;
        case VSYNC_ADAPTIVE: vsync_str = "ADAPTIVE (FreeSync/G-Sync)"; break;
        default: vsync_str = "UNKNOWN";
    }
    
    // Set viewport
    glViewport(0, 0, window_width, window_height);
    
    // Set clear color (black)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Initialized with %s mode, size %dx%d (%.2f pixels per meter, VSync: %s)", 
           is_fullscreen ? "fullscreen" : "windowed", window_width, window_height, PIXELS_PER_METER, vsync_str);
    
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
    
    // Update frame counter
    double current_time = glfwGetTime();
    frame_counter.frames++;
    
    // Update FPS every second
    if (current_time - frame_counter.last_time >= 1.0) {
        frame_counter.fps = frame_counter.frames / (current_time - frame_counter.last_time);
        frame_counter.frames = 0;
        frame_counter.last_time = current_time;
        
        // Format FPS text
        sprintf(frame_counter.fps_text, "FPS: %.1f", frame_counter.fps);
        
        // Log FPS
        LOG_INFO(LOG_CATEGORY_RENDERER, "Current frame rate: %.1f FPS", frame_counter.fps);
    }
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
    
    // Draw FPS counter in the top-right corner
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, 0.0, window_height, -1.0, 1.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Draw FPS text (simple colored rectangle for now)
    glColor3f(0.0f, 1.0f, 0.0f);
    glRasterPos2f(window_width - 100, window_height - 20);
    
    // Print player position to console for debugging (less frequently)
    static double last_print_time = 0.0;
    if (state->game_time - last_print_time > 1.0) {
        LOG_INFO(LOG_CATEGORY_RENDERER, "Player: (%.2f, %.2f) m, Camera: (%.2f, %.2f) m", 
               state->player.position_x, state->player.position_y,
               camera->position_x, camera->position_y);
        last_print_time = state->game_time;
    }
}

// Check if the window should close
bool renderer_should_close(void) {
    return glfwWindowShouldClose(window);
}

// Get window dimensions
void renderer_get_window_size(int* width, int* height) {
    if (!window) {
        // If window isn't created yet, return the cached values
        if (width) *width = window_width;
        if (height) *height = window_height;
        return;
    }
    
    // Get the actual window size from GLFW
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    // Update cached values
    window_width = w;
    window_height = h;
    
    // Return the values
    if (width) *width = w;
    if (height) *height = h;
}

// Process input events
void renderer_process_input(void) {
    glfwPollEvents();
} 