#include "../include/input.h"
#include "../include/physics.h"
#include "../include/renderer.h"
#include "../include/logging.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Key definitions
#define KEY_LEFT GLFW_KEY_A
#define KEY_RIGHT GLFW_KEY_D
#define KEY_JUMP GLFW_KEY_SPACE

// PlayStation controller button mappings (may vary by platform)
#define PS_BUTTON_X 1       // X button
#define PS_AXIS_LEFT_X 0    // Left analog stick X axis
#define PS_AXIS_LEFT_Y 1    // Left analog stick Y axis

// Analog stick deadzone (to prevent drift)
#define ANALOG_DEADZONE 0.2f

// Key state tracking
static struct {
    bool left_pressed;
    bool right_pressed;
    bool jump_pressed;
} key_state;

// Controller state tracking
static struct {
    bool connected;
    int controller_id;
    const char* name;
} controller_state;

// Initialize the input system
void input_init(void) {
    // Reset key state
    key_state.left_pressed = false;
    key_state.right_pressed = false;
    key_state.jump_pressed = false;
    
    // Reset controller state
    controller_state.connected = false;
    controller_state.controller_id = -1;
    controller_state.name = NULL;
    
    // Check for connected controllers
    for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_LAST; i++) {
        if (glfwJoystickPresent(i)) {
            controller_state.connected = true;
            controller_state.controller_id = i;
            controller_state.name = glfwGetJoystickName(i);
            
            LOG_INFO(LOG_CATEGORY_INPUT, "Controller connected: %s (ID: %d)", 
                   controller_state.name, controller_state.controller_id);
            
            // Only use the first controller found
            break;
        }
    }
    
    LOG_INFO(LOG_CATEGORY_INPUT, "Initialized with controls: A=left, D=right, SPACE=jump");
    if (controller_state.connected) {
        LOG_INFO(LOG_CATEGORY_INPUT, "Controller support enabled: Left stick=move, X button=jump");
    } else {
        LOG_INFO(LOG_CATEGORY_INPUT, "No controller detected. Connect a controller and restart the game.");
    }
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

// Check controller state
void input_check_controller(bool* left, bool* right, bool* jump) {
    if (!controller_state.connected || controller_state.controller_id < 0) {
        return;  // No controller connected
    }
    
    // Check if controller is still connected
    if (!glfwJoystickPresent(controller_state.controller_id)) {
        LOG_WARNING(LOG_CATEGORY_INPUT, "Controller disconnected: %s", controller_state.name);
        controller_state.connected = false;
        controller_state.controller_id = -1;
        controller_state.name = NULL;
        return;
    }
    
    // Get controller buttons
    int button_count;
    const unsigned char* buttons = glfwGetJoystickButtons(controller_state.controller_id, &button_count);
    
    // Get controller axes
    int axis_count;
    const float* axes = glfwGetJoystickAxes(controller_state.controller_id, &axis_count);
    
    // Check if we have enough buttons and axes
    if (button_count < PS_BUTTON_X + 1 || axis_count < PS_AXIS_LEFT_Y + 1) {
        LOG_WARNING(LOG_CATEGORY_INPUT, "Controller has insufficient buttons or axes");
        return;
    }
    
    // Check X button for jump
    if (buttons[PS_BUTTON_X] == GLFW_PRESS) {
        *jump = true;
    }
    
    // Check left analog stick for movement
    float left_x = axes[PS_AXIS_LEFT_X];
    
    // Apply deadzone
    if (fabsf(left_x) < ANALOG_DEADZONE) {
        left_x = 0.0f;
    }
    
    // Set movement based on analog stick
    if (left_x < -ANALOG_DEADZONE) {
        *left = true;
        *right = false;
    } else if (left_x > ANALOG_DEADZONE) {
        *left = false;
        *right = true;
    }
    
    // Debug output for controller
    static float last_left_x = 0.0f;
    static bool last_jump = false;
    
    if (fabsf(left_x - last_left_x) > 0.1f || last_jump != *jump) {
        LOG_DEBUG(LOG_CATEGORY_INPUT, "Controller: left_x=%.2f, jump=%d", 
               left_x, *jump);
        last_left_x = left_x;
        last_jump = *jump;
    }
}

// Update input state (to be called by the scheduler)
void input_update(double dt, void* user_data) {
    // Get key states from keyboard
    bool left = input_is_key_pressed(KEY_LEFT);
    bool right = input_is_key_pressed(KEY_RIGHT);
    bool jump = input_is_key_pressed(KEY_JUMP);
    
    // Check controller input (will override keyboard if active)
    if (controller_state.connected) {
        input_check_controller(&left, &right, &jump);
    }
    
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
        LOG_DEBUG(LOG_CATEGORY_INPUT, "Input: left=%d, right=%d, jump=%d", 
               left, right, jump);
    }
} 