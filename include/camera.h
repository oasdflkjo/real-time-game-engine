#ifndef CAMERA_H
#define CAMERA_H

#include "game_state.h"

// Camera view properties
typedef struct {
    float position_x;
    float position_y;
    float target_x;
    float target_y;
    float zoom;
    float width;
    float height;
} Camera;

// Initialize the camera system
void camera_init(float width, float height);

// Shutdown the camera system
void camera_shutdown(void);

// Update the camera position to follow a target
void camera_update(double dt, void* user_data);

// Update camera dimensions when window is resized
void camera_update_dimensions(float width, float height);

// Get the current camera state
const Camera* camera_get_current(void);

// Convert world coordinates to screen coordinates
void camera_world_to_screen(const Camera* camera, float world_x, float world_y, float* screen_x, float* screen_y);

// Convert screen coordinates to world coordinates
void camera_screen_to_world(const Camera* camera, float screen_x, float screen_y, float* world_x, float* world_y);

#endif // CAMERA_H 