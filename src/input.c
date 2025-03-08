#include "../include/input.h"
#include "../include/physics.h"
#include "../include/renderer.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

// Key definitions
#define KEY_LEFT GLFW_KEY_A
#define KEY_RIGHT GLFW_KEY_D
#define KEY_JUMP GLFW_KEY_SPACE

// Initialize the input system
void input_init(void) {
    printf("[Input] Initialized with controls: A=left, D=right, SPACE=jump\n");
}

// Shutdown the input system
void input_shutdown(void) {
    printf("[Input] Shutdown\n");
}

// Check if a key is pressed
bool input_is_key_pressed(int key) {
    // Get the GLFW window from the renderer
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) {
        return false;
    }
    
    return glfwGetKey(window, key) == GLFW_PRESS;
}

// Update input state (to be called by the scheduler)
void input_update(double dt, void* user_data) {
    // Get key states
    bool move_left = input_is_key_pressed(KEY_LEFT);
    bool move_right = input_is_key_pressed(KEY_RIGHT);
    bool jump = input_is_key_pressed(KEY_JUMP);
    
    // Apply input to physics
    physics_apply_input(move_left, move_right, jump);
    
    // Debug output for input polling
    static bool last_left = false;
    static bool last_right = false;
    static bool last_jump = false;
    
    if (move_left != last_left || move_right != last_right || jump != last_jump) {
        printf("[Input] Polled: left=%d, right=%d, jump=%d\n", 
               move_left, move_right, jump);
        
        last_left = move_left;
        last_right = move_right;
        last_jump = jump;
    }
} 