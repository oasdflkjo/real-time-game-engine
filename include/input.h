#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>

// Initialize the input system
void input_init(void);

// Shutdown the input system
void input_shutdown(void);

// Update input state (to be called by the scheduler)
void input_update(double dt, void* user_data);

// Get input state
bool input_is_key_pressed(int key);

#endif // INPUT_H 