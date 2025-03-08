#include "../include/input.h"
#include "../include/physics.h"
#include "../include/renderer.h"
#include "../include/logging.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

// Key definitions
#define KEY_LEFT GLFW_KEY_A
#define KEY_RIGHT GLFW_KEY_D
#define KEY_JUMP GLFW_KEY_SPACE

// Key state tracking
static struct {
    bool left_pressed;
    bool right_pressed;
    bool jump_pressed;
} key_state;

// Initialize the input system
void input_init(void) {
    // Reset key state
    key_state.left_pressed = false;
    key_state.right_pressed = false;
    key_state.jump_pressed = false;
    
    LOG_INFO(LOG_CATEGORY_INPUT, "Initialized with controls: A=left, D=right, SPACE=jump");
}

// Shutdown the input system
void input_shutdown(void) {
    LOG_INFO(LOG_CATEGORY_INPUT, "Shutdown");
}

// Check if a key is pressed
bool input_is_key_pressed(int key) {
    // Get the GLFW window from the renderer
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) {
        LOG_ERROR(LOG_CATEGORY_INPUT, "No GLFW window context available");
        return false;
    }
    
    return glfwGetKey(window, key) == GLFW_PRESS;
}

// Update input state (to be called by the scheduler)
void input_update(double dt, void* user_data) {
    // Get key states
    bool left = input_is_key_pressed(KEY_LEFT);
    bool right = input_is_key_pressed(KEY_RIGHT);
    bool jump = input_is_key_pressed(KEY_JUMP);
    
    // Check for changes in key state
    bool left_changed = (left != key_state.left_pressed);
    bool right_changed = (right != key_state.right_pressed);
    bool jump_changed = (jump != key_state.jump_pressed);
    
    // Update key state
    key_state.left_pressed = left;
    key_state.right_pressed = right;
    key_state.jump_pressed = jump;
    
    // Apply input to physics
    physics_apply_input(left, right, jump);
    
    // Debug output for input polling (only when keys change)
    if (left_changed || right_changed || jump_changed) {
        LOG_DEBUG(LOG_CATEGORY_INPUT, "Polled: left=%d, right=%d, jump=%d", 
               left, right, jump);
    }
} 