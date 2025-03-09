#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

// Input state structure for analog movement
typedef struct {
    bool jump;           // Jump button state (digital)
    float move_x;        // Horizontal movement (-1.0 to 1.0)
    float move_y;        // Vertical movement (-1.0 to 1.0)
} InputState;

// Initialize the input system
void input_init(void);

// Shutdown the input system
void input_shutdown(void);

// Update input state (to be called by the scheduler)
void input_update(double dt, void* user_data);

// Get input state
bool input_is_key_pressed(int key);

// Get the current input state
InputState input_get_state(void);

// Check controller state (legacy function, kept for compatibility)
void input_check_controller(bool* left, bool* right, bool* jump);

#endif // INPUT_H 