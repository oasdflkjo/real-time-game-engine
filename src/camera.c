#include "../include/camera.h"
#include "../include/game_state.h"
#include "../include/logging.h"
#include <stdio.h>
#include <math.h>

// Camera state
static Camera camera;

// Initialize the camera system
void camera_init(float width, float height) {
    camera.position_x = 0.0f;
    camera.position_y = -1.0f; // Fixed Y position (ground level is at y=0, player height is 2.0)
    camera.target_x = 0.0f;
    camera.target_y = -1.0f;
    camera.zoom = 50.0f;  // 50 pixels per meter
    camera.width = width;
    camera.height = height;
    
    LOG_INFO(LOG_CATEGORY_CAMERA, "Initialized with dimensions %.1fx%.1f, zoom=%.1f", 
           width, height, camera.zoom);
}

// Shutdown the camera system
void camera_shutdown(void) {
    LOG_INFO(LOG_CATEGORY_CAMERA, "Shutdown");
}

// Update the camera position to follow a target
void camera_update(double dt, void* user_data) {
    // Get the current game state
    const GameState* state = game_state_get_read();
    
    // Calculate the target position (player position)
    float target_x = state->player.position_x;
    
    // For Y position, we'll use a fixed value to keep the ground level stable
    // This prevents the camera from following the player during jumps
    float target_y = -1.0f;  // Fixed Y position (same as initial camera position)
    
    // Calculate the pixel-aligned camera position
    // This ensures the camera position is always aligned to pixel boundaries
    // which prevents jittering when rendering the grid and axes
    float pixel_x = target_x * camera.zoom;
    
    // Round to nearest pixel
    pixel_x = roundf(pixel_x);
    
    // Convert back to world coordinates
    camera.position_x = pixel_x / camera.zoom;
    
    // Set Y position directly (no pixel alignment needed since it's fixed)
    camera.position_y = target_y;
    
    // Also update target position (for consistency)
    camera.target_x = target_x;
    camera.target_y = target_y;
    
    // Debug output (only when camera moves significantly)
    static float last_x = 0.0f;
    float dx = camera.position_x - last_x;
    
    if (fabs(dx) > 0.1f) {
        LOG_DEBUG(LOG_CATEGORY_CAMERA, "Position: (%.2f, %.2f), Pixel-aligned: (%.2f, %.2f)",
               target_x, target_y, camera.position_x, camera.position_y);
        last_x = camera.position_x;
    }
}

// Get the current camera state
const Camera* camera_get_current(void) {
    return &camera;
}

// Convert world coordinates to screen coordinates
void camera_world_to_screen(const Camera* camera, float world_x, float world_y, float* screen_x, float* screen_y) {
    if (!camera || !screen_x || !screen_y) {
        return;
    }
    
    // Calculate the offset from the camera position
    float offset_x = world_x - camera->position_x;
    float offset_y = world_y - camera->position_y;
    
    // Apply zoom and center on screen
    // Flip the y-axis so that positive y in world space goes up on screen
    *screen_x = (offset_x * camera->zoom) + (camera->width / 2.0f);
    *screen_y = (camera->height / 2.0f) - (offset_y * camera->zoom);  // Flipped y-axis
}

// Convert screen coordinates to world coordinates
void camera_screen_to_world(const Camera* camera, float screen_x, float screen_y, float* world_x, float* world_y) {
    if (!camera || !world_x || !world_y) {
        return;
    }
    
    // Calculate the offset from the screen center
    float offset_x = screen_x - (camera->width / 2.0f);
    // Flip the y-axis to match our world-to-screen conversion
    float offset_y = (camera->height / 2.0f) - screen_y;
    
    // Apply inverse zoom and add camera position
    *world_x = (offset_x / camera->zoom) + camera->position_x;
    *world_y = (offset_y / camera->zoom) + camera->position_y;
}

// Update camera dimensions when window is resized
void camera_update_dimensions(float width, float height) {
    camera.width = width;
    camera.height = height;
    
    LOG_INFO(LOG_CATEGORY_CAMERA, "Updated dimensions to %.1fx%.1f", width, height);
} 