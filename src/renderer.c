#include "../include/renderer.h"
#include "../include/camera.h"
#include "../include/logging.h"
#include "../include/scheduler.h"
#include "../include/debug_hud.h"
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Grid texture
static GLuint grid_texture = 0;
static int grid_texture_size = 512;  // Size of the grid texture (power of 2)
static float grid_texture_scale = 1.0f;  // Scale of the grid in the texture (larger value = smaller grid cells)

// Ground and player textures
static GLuint ground_texture = 0;
static GLuint player_texture = 0;

// Frame rate tracking
static struct {
    double last_time;
    int frames;
    double fps;
    char fps_text[32];
} frame_counter = {0};

// Function declarations
static GLuint generate_grid_texture(int size, float grid_spacing, float line_width, float r, float g, float b, float a);
static void draw_textured_quad(float x, float y, float width, float height, float s1, float t1, float s2, float t2);
static void draw_grid_texture(const Camera* camera);
static void draw_rectangle(float x, float y, float width, float height, float r, float g, float b);
static void draw_world_rectangle(const Camera* camera, float x, float y, float width, float height, float r, float g, float b);
static void draw_line(float x1, float y1, float x2, float y2, float r, float g, float b);
static void draw_world_line(const Camera* camera, float x1, float y1, float x2, float y2, float r, float g, float b);
static void draw_world_grid(const Camera* camera, float grid_size, float r, float g, float b);
static void draw_coordinate_axes(const Camera* camera);
static GLuint generate_ground_texture(int size);
static GLuint generate_player_texture(int size);
static void draw_textured_world_rectangle(const Camera* camera, float x, float y, float width, float height, 
                                         GLuint texture, float s1, float t1, float s2, float t2);

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
    
    // Toggle VSync with V key
    if (key == GLFW_KEY_V && action == GLFW_PRESS) {
        // Cycle through VSync modes: OFF -> ON -> ADAPTIVE -> OFF
        if (vsync_mode == VSYNC_OFF) {
            vsync_mode = VSYNC_ON;
            LOG_INFO(LOG_CATEGORY_RENDERER, "VSync mode: ON");
        } else if (vsync_mode == VSYNC_ON) {
            vsync_mode = VSYNC_ADAPTIVE;
            LOG_INFO(LOG_CATEGORY_RENDERER, "VSync mode: ADAPTIVE");
        } else {
            vsync_mode = VSYNC_OFF;
            LOG_INFO(LOG_CATEGORY_RENDERER, "VSync mode: OFF");
        }
        
        // Apply the new VSync setting
        glfwSwapInterval(vsync_mode);
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
    
    // Create a windowed mode window and its OpenGL context
    if (is_fullscreen) {
        // Get the primary monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        // Create fullscreen window
        window = glfwCreateWindow(mode->width, mode->height, "Real-Time Game Engine", monitor, NULL);
        
        // Update window dimensions
        window_width = mode->width;
        window_height = mode->height;
        
        LOG_INFO(LOG_CATEGORY_RENDERER, "Created fullscreen window (%dx%d)", window_width, window_height);
    } else {
        // Create windowed mode window
        window = glfwCreateWindow(window_width, window_height, "Real-Time Game Engine", NULL, NULL);
        LOG_INFO(LOG_CATEGORY_RENDERER, "Created window (%dx%d)", window_width, window_height);
    }
    
    if (!window) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to create window");
        glfwTerminate();
        return false;
    }
    
    // Make the window's context current
    glfwMakeContextCurrent(window);
    
    // Set VSync mode
    glfwSwapInterval(vsync_mode);
    
    // Set key callback
    glfwSetKeyCallback(window, key_callback);
    
    // Set framebuffer size callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    // Initialize OpenGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Generate grid texture
    grid_texture = generate_grid_texture(grid_texture_size, 0.1f, 0.01f, 0.2f, 0.2f, 0.2f, 1.0f);
    
    // Generate ground texture
    ground_texture = generate_ground_texture(256);
    
    // Generate player texture
    player_texture = generate_player_texture(128);
    
    // Initialize debug HUD
    debug_hud_init();
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Initialized with OpenGL %s", glGetString(GL_VERSION));
    return true;
}

// Shutdown the renderer
void renderer_shutdown(void) {
    // Delete grid texture
    if (grid_texture) {
        glDeleteTextures(1, &grid_texture);
        grid_texture = 0;
    }
    
    if (ground_texture) {
        glDeleteTextures(1, &ground_texture);
        ground_texture = 0;
    }
    
    if (player_texture) {
        glDeleteTextures(1, &player_texture);
        player_texture = 0;
    }
    
    if (window) {
        glfwDestroyWindow(window);
        window = NULL;
    }
    
    glfwTerminate();
    
    // Shutdown debug HUD
    debug_hud_shutdown();
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Shutdown");
}

// Begin a new frame
void renderer_begin_frame(void) {
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
    
    // Poll for events at the beginning of the frame
    glfwPollEvents();
}

// End the current frame and swap buffers
void renderer_end_frame(void) {
    // Ensure all rendering commands are submitted
    glFlush();
    
    // Swap buffers
    glfwSwapBuffers(window);
}

// Draw a rectangle at the specified position in screen coordinates
static void draw_rectangle(float x, float y, float width, float height, float r, float g, float b) {
    glBegin(GL_QUADS);
    glColor3f(r, g, b);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
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
    
    // For horizontal or vertical lines, ensure pixel-perfect alignment
    bool is_horizontal = fabsf(screen_y1 - screen_y2) < 0.01f;
    bool is_vertical = fabsf(screen_x1 - screen_x2) < 0.01f;
    
    if (is_horizontal) {
        // Align horizontal lines to pixel centers
        float y = floorf(screen_y1) + 0.5f;
        screen_y1 = screen_y2 = y;
    } else if (is_vertical) {
        // Align vertical lines to pixel centers
        float x = floorf(screen_x1) + 0.5f;
        screen_x1 = screen_x2 = x;
    }
    
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
}

// Draw coordinate axes
static void draw_coordinate_axes(const Camera* camera) {
    // Get the camera position in screen space
    float center_x, center_y;
    camera_world_to_screen(camera, 0.0f, 0.0f, &center_x, &center_y);
    
    // Convert to integer pixel coordinates
    int pixel_center_x = (int)roundf(center_x);
    int pixel_center_y = (int)roundf(window_height - center_y);
    
    // Set line width to exactly 1.0 for pixel-perfect lines
    glLineWidth(1.0f);
    
    // Draw X-axis (red) - horizontal line through origin
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // Red
    glVertex2f(0, pixel_center_y + 0.5f);
    glVertex2f(window_width, pixel_center_y + 0.5f);
    glEnd();
    
    // Draw Y-axis (green) - vertical line through origin
    glBegin(GL_LINES);
    glColor3f(0.0f, 1.0f, 0.0f); // Green
    glVertex2f(pixel_center_x + 0.5f, 0);
    glVertex2f(pixel_center_x + 0.5f, window_height);
    glEnd();
    
    // Reset line width
    glLineWidth(1.0f);
}

// Generate a grid texture to avoid jittering
static GLuint generate_grid_texture(int size, float grid_spacing, float line_width, float r, float g, float b, float a) {
    LOG_INFO(LOG_CATEGORY_RENDERER, "Generating grid texture: size=%d, spacing=%.1f, width=%.1f", 
           size, grid_spacing, line_width);
    
    // Create a texture
    GLuint texture = 0;
    glGenTextures(1, &texture);
    
    if (texture == 0) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to generate texture");
        return 0;
    }
    
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Create texture data with all pixels initially transparent
    unsigned char* data = (unsigned char*)calloc(size * size * 4, sizeof(unsigned char));
    if (!data) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to allocate memory for grid texture");
        glDeleteTextures(1, &texture);
        return 0;
    }
    
    // Calculate grid parameters
    int grid_pixels = (int)(size / grid_spacing);
    int line_pixels = (int)(line_width * size / 100.0f);  // Line width as percentage of texture size
    if (line_pixels < 1) line_pixels = 1;
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Grid parameters: grid_pixels=%d, line_pixels=%d", 
           grid_pixels, line_pixels);
    
    // Draw grid lines
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            // Calculate index in the texture data
            int index = (y * size + x) * 4;
            
            // Default to transparent
            data[index + 0] = 0;  // R
            data[index + 1] = 0;  // G
            data[index + 2] = 0;  // B
            data[index + 3] = 0;  // A (transparent)
            
            // Check if this pixel is on a grid line
            bool on_grid_x = (x % grid_pixels) < line_pixels;
            bool on_grid_y = (y % grid_pixels) < line_pixels;
            
            // Check if this pixel is on an axis
            bool on_x_axis = abs(y - size/2) < line_pixels * 2;  // Make axes slightly thicker
            bool on_y_axis = abs(x - size/2) < line_pixels * 2;
            
            // Set color based on position
            if (on_x_axis) {
                // X-axis (red)
                data[index + 0] = 255;  // R
                data[index + 1] = 0;    // G
                data[index + 2] = 0;    // B
                data[index + 3] = 255;  // A (opaque)
            } 
            else if (on_y_axis) {
                // Y-axis (green)
                data[index + 0] = 0;    // R
                data[index + 1] = 255;  // G
                data[index + 2] = 0;    // B
                data[index + 3] = 255;  // A (opaque)
            }
            else if (on_grid_x || on_grid_y) {
                // Grid lines (gray)
                data[index + 0] = (unsigned char)(r * 255);
                data[index + 1] = (unsigned char)(g * 255);
                data[index + 2] = (unsigned char)(b * 255);
                data[index + 3] = (unsigned char)(a * 255);
            }
        }
    }
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "OpenGL error when creating texture: %d", error);
        free(data);
        glDeleteTextures(1, &texture);
        return 0;
    }
    
    // Free texture data
    free(data);
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Successfully generated grid texture with ID %u", texture);
    
    return texture;
}

// Draw a textured quad in screen coordinates
static void draw_textured_quad(float x, float y, float width, float height, float s1, float t1, float s2, float t2) {
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_QUADS);
    glTexCoord2f(s1, t1); glVertex2f(x, y);
    glTexCoord2f(s2, t1); glVertex2f(x + width, y);
    glTexCoord2f(s2, t2); glVertex2f(x + width, y + height);
    glTexCoord2f(s1, t2); glVertex2f(x, y + height);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// Draw the grid using the texture
static void draw_grid_texture(const Camera* camera) {
    if (grid_texture == 0) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Cannot draw grid texture: texture not initialized");
        return;
    }
    
    // Save current OpenGL state
    GLboolean blend_enabled = glIsEnabled(GL_BLEND);
    GLint blend_src, blend_dst;
    glGetIntegerv(GL_BLEND_SRC, &blend_src);
    glGetIntegerv(GL_BLEND_DST, &blend_dst);
    
    // Set up blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Bind the grid texture
    glBindTexture(GL_TEXTURE_2D, grid_texture);
    
    // Calculate texture coordinates based on camera position
    float world_left = camera->position_x - camera->width / (2.0f * camera->zoom);
    float world_top = camera->position_y - camera->height / (2.0f * camera->zoom);
    
    // Calculate texture coordinates
    float tex_scale = grid_texture_scale / PIXELS_PER_METER;
    float s1 = world_left * tex_scale;
    float t1 = world_top * tex_scale;
    float s2 = s1 + (camera->width / camera->zoom) * tex_scale;
    float t2 = t1 + (camera->height / camera->zoom) * tex_scale;
    
    LOG_DEBUG(LOG_CATEGORY_RENDERER, "Drawing grid texture: tex_coords=(%.2f,%.2f)-(%.2f,%.2f)", 
             s1, t1, s2, t2);
    
    // Enable texturing
    glEnable(GL_TEXTURE_2D);
    
    // Draw the textured quad covering the entire screen
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // White (no tint)
    
    glBegin(GL_QUADS);
    // Use normal texture coordinates now that camera handles the flipping
    glTexCoord2f(s1, t1); glVertex2f(0, 0);
    glTexCoord2f(s2, t1); glVertex2f(window_width, 0);
    glTexCoord2f(s2, t2); glVertex2f(window_width, window_height);
    glTexCoord2f(s1, t2); glVertex2f(0, window_height);
    glEnd();
    
    // Disable texturing
    glDisable(GL_TEXTURE_2D);
    
    // Restore previous OpenGL state
    if (!blend_enabled) {
        glDisable(GL_BLEND);
    }
    glBlendFunc(blend_src, blend_dst);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "OpenGL error when drawing grid texture: %d", error);
    }
}

// Draw a textured rectangle in world space
static void draw_textured_world_rectangle(const Camera* camera, float x, float y, float width, float height, 
                                         GLuint texture, float s1, float t1, float s2, float t2) {
    if (!camera) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Cannot draw textured world rectangle: camera is NULL");
        return;
    }
    
    // Convert world coordinates to screen coordinates
    float screen_x, screen_y, screen_width, screen_height;
    
    // Convert bottom-left corner
    camera_world_to_screen(camera, x - width/2, y - height/2, &screen_x, &screen_y);
    
    // Convert top-right corner
    float screen_right, screen_top;
    camera_world_to_screen(camera, x + width/2, y + height/2, &screen_right, &screen_top);
    
    // Calculate width and height in screen space
    screen_width = screen_right - screen_x;
    screen_height = screen_top - screen_y;
    
    // Save current OpenGL state
    GLboolean texture_enabled = glIsEnabled(GL_TEXTURE_2D);
    GLboolean blend_enabled = glIsEnabled(GL_BLEND);
    
    // Enable texturing and blending
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Bind the texture
    glBindTexture(GL_TEXTURE_2D, texture);
    
    // Draw the textured quad
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);  // No tint
    
    glBegin(GL_QUADS);
    // Use normal texture coordinates now that camera handles the flipping
    glTexCoord2f(s1, t1); glVertex2f(screen_x, screen_y);
    glTexCoord2f(s2, t1); glVertex2f(screen_x + screen_width, screen_y);
    glTexCoord2f(s2, t2); glVertex2f(screen_x + screen_width, screen_y + screen_height);
    glTexCoord2f(s1, t2); glVertex2f(screen_x, screen_y + screen_height);
    glEnd();
    
    // Restore previous OpenGL state
    if (!texture_enabled) {
        glDisable(GL_TEXTURE_2D);
    }
    if (!blend_enabled) {
        glDisable(GL_BLEND);
    }
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "OpenGL error when drawing textured world rectangle: %d", error);
    }
}

// Generate a ground texture with grass pattern
static GLuint generate_ground_texture(int size) {
    // Create texture data (RGBA format)
    unsigned char* data = (unsigned char*)calloc(size * size * 4, sizeof(unsigned char));
    if (!data) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to allocate memory for ground texture");
        return 0;
    }
    
    // Fill with grass pattern (dark green base with lighter green spots)
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = (y * size + x) * 4;
            
            // Base dark green color
            data[index + 0] = 30;     // R
            data[index + 1] = 100;    // G
            data[index + 2] = 30;     // B
            data[index + 3] = 255;    // A (fully opaque)
            
            // Add some noise/variation
            float noise = (float)rand() / RAND_MAX;
            if (noise > 0.7f) {
                // Lighter green spots
                data[index + 0] = 40;     // R
                data[index + 1] = 140;    // G
                data[index + 2] = 40;     // B
            }
            
            // Add some darker spots
            if (noise < 0.2f) {
                data[index + 0] = 20;     // R
                data[index + 1] = 80;     // G
                data[index + 2] = 20;     // B
            }
        }
    }
    
    // Create OpenGL texture
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    // Free memory
    free(data);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "OpenGL error when creating ground texture: %d", error);
        glDeleteTextures(1, &texture_id);
        return 0;
    }
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Created ground texture with ID %u, size %dx%d", texture_id, size, size);
    return texture_id;
}

// Generate a player texture (simple character)
static GLuint generate_player_texture(int size) {
    // Create texture data (RGBA format)
    unsigned char* data = (unsigned char*)calloc(size * size * 4, sizeof(unsigned char));
    if (!data) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to allocate memory for player texture");
        return 0;
    }
    
    // Fill with red character with white eyes
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = (y * size + x) * 4;
            
            // Normalize coordinates to [0,1] range
            float nx = (float)x / size;
            float ny = (float)y / size;
            
            // Default: transparent
            data[index + 0] = 0;      // R
            data[index + 1] = 0;      // G
            data[index + 2] = 0;      // B
            data[index + 3] = 0;      // A (transparent)
            
            // Body (red rectangle)
            if (nx >= 0.1f && nx <= 0.9f && ny >= 0.1f && ny <= 0.9f) {
                data[index + 0] = 220;    // R
                data[index + 1] = 50;     // G
                data[index + 2] = 50;     // B
                data[index + 3] = 255;    // A (opaque)
                
                // Add some shading
                if (nx < 0.3f) {
                    // Darker on left side
                    data[index + 0] = 180;
                    data[index + 1] = 40;
                    data[index + 2] = 40;
                }
                
                // Eyes (white circles) - moved to lower part of face (0.35f instead of 0.65f)
                float eye_radius = 0.08f;
                float left_eye_x = 0.35f;
                float right_eye_x = 0.65f;
                float eye_y = 0.35f;  // Moved eyes to lower part of face
                
                float dist_left = sqrtf((nx - left_eye_x) * (nx - left_eye_x) + (ny - eye_y) * (ny - eye_y));
                float dist_right = sqrtf((nx - right_eye_x) * (nx - right_eye_x) + (ny - eye_y) * (ny - eye_y));
                
                if (dist_left < eye_radius || dist_right < eye_radius) {
                    data[index + 0] = 255;    // R
                    data[index + 1] = 255;    // G
                    data[index + 2] = 255;    // B
                    data[index + 3] = 255;    // A
                }
            }
        }
    }
    
    // Create OpenGL texture
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    
    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    
    // Free memory
    free(data);
    
    // Check for errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "OpenGL error when creating player texture: %d", error);
        glDeleteTextures(1, &texture_id);
        return 0;
    }
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Created player texture with ID %u, size %dx%d", texture_id, size, size);
    return texture_id;
}

// Update the renderer_draw_game function to include the debug HUD
void renderer_draw_game(const GameState* state) {
    if (!state) {
        return;
    }
    
    // Get the current camera
    const Camera* camera = camera_get_current();
    
    // Clear the screen with black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, 0.0, window_height, -1.0, 1.0);  // Note: Y-axis flipped to have origin at bottom-left
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Enable texture and blend modes for consistent rendering
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Draw world grid using texture (eliminates jitter)
    draw_grid_texture(camera);
    
    // Draw all ground planes
    for (int i = 0; i < state->ground_count; i++) {
        const GroundState* ground = &state->grounds[i];
        float ground_tex_repeat = ground->width / 10.0f;  // Repeat every 10 meters
        draw_textured_world_rectangle(camera, 
                                     ground->position_x, 
                                     ground->position_y, 
                                     ground->width, 
                                     ground->height, 
                                     ground_texture, 0.0f, 0.0f, ground_tex_repeat, 1.0f);
    }
    
    // Draw all entities
    for (int i = 0; i < state->entity_count; i++) {
        const Entity* entity = state->entities[i];
        
        if (!entity || !entity->is_visible) {
            continue;
        }
        
        // Default texture coordinates
        float s1 = 0.0f;
        float t1 = 0.0f;
        float s2 = 1.0f;
        float t2 = 1.0f;
        
        // Choose texture based on entity type
        GLuint texture = 0;
        
        switch (entity->type) {
            case ENTITY_TYPE_PLAYER:
                texture = player_texture;
                break;
                
            case ENTITY_TYPE_ENEMY:
                // Use player texture for enemies too for now
                texture = player_texture;
                break;
                
            default:
                // Default to player texture
                texture = player_texture;
                break;
        }
        
        // Draw the entity
        draw_textured_world_rectangle(camera, 
                                                         entity->position_x, 
                                                         entity->position_y, 
                                                         entity->width, 
                                                         entity->height,
                                                         texture, s1, t1, s2, t2);
    }
    
    // Draw HUD (screen coordinates)
    char position_text[64];
    if (state->player) {
        sprintf(position_text, "Player: (%.2f, %.2f) m", 
               state->player->position_x, state->player->position_y);
    }
    
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
    double current_time = scheduler_get_time_ms() / 1000.0;
    if (current_time - last_print_time > 1.0 && state->player) {
        LOG_INFO(LOG_CATEGORY_RENDERER, "Player: (%.2f, %.2f) m, Entities: %d", 
               state->player->position_x, state->player->position_y, state->entity_count);
        last_print_time = current_time;
    }
    
    // Render debug HUD
    debug_hud_render(state);
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