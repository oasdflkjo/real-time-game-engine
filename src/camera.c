#include "../include/camera.h"
#include "../include/game_state.h"
#include "../include/logging.h"
#include <stdio.h>
#include <math.h>

// Camera state
static Camera camera;

// Camera smoothing factor (lower = smoother, higher = more responsive)
#define CAMERA_SMOOTHING 3.0f

// Initialize the camera system
void camera_init(float width, float height) {
    camera.position_x = 0.0f;
    camera.position_y = -1.0f; // Start at player's initial position
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
    
    // Set the target to the player's position
    camera.target_x = state->player.position_x;
    camera.target_y = state->player.position_y;
    
    // Smoothly move the camera towards the target
    float dx = camera.target_x - camera.position_x;
    float dy = camera.target_y - camera.position_y;
    
    // Limit the time step to prevent extreme movements
    float safe_dt = (float)dt;
    if (safe_dt > 0.1f) safe_dt = 0.1f;
    
    // Apply smoothing with a clamped delta time
    camera.position_x += dx * CAMERA_SMOOTHING * safe_dt;
    camera.position_y += dy * CAMERA_SMOOTHING * safe_dt;
    
    // Ensure camera doesn't drift too far from target
    if (fabs(dx) > 10.0f || fabs(dy) > 10.0f) {
        // If too far, snap back to target
        camera.position_x = camera.target_x;
        camera.position_y = camera.target_y;
        LOG_WARNING(LOG_CATEGORY_CAMERA, "Position reset to target due to large distance");
    }
    
    // Debug output (only when camera moves significantly)
    if (fabs(dx) > 0.1f || fabs(dy) > 0.1f) {
        LOG_DEBUG(LOG_CATEGORY_CAMERA, "Position: (%.2f, %.2f), Target: (%.2f, %.2f)",
               camera.position_x, camera.position_y,
               camera.target_x, camera.target_y);
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
    *screen_x = (offset_x * camera->zoom) + (camera->width / 2.0f);
    *screen_y = (offset_y * camera->zoom) + (camera->height / 2.0f);
}

// Convert screen coordinates to world coordinates
void camera_screen_to_world(const Camera* camera, float screen_x, float screen_y, float* world_x, float* world_y) {
    if (!camera || !world_x || !world_y) {
        return;
    }
    
    // Calculate the offset from the screen center
    float offset_x = screen_x - (camera->width / 2.0f);
    float offset_y = screen_y - (camera->height / 2.0f);
    
    // Apply inverse zoom and add camera position
    *world_x = (offset_x / camera->zoom) + camera->position_x;
    *world_y = (offset_y / camera->zoom) + camera->position_y;
} 