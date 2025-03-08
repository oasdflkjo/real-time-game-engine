#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

// Task priorities and frequencies
typedef enum {
    TASK_PRIORITY_PHYSICS_AI = 0,    // 60Hz (16.67ms) - Fixed time step
    TASK_PRIORITY_ANIMATION,         // 165Hz (6.06ms)
    TASK_PRIORITY_RENDER_AUDIO,      // 165Hz (6.06ms)
    TASK_PRIORITY_BACKGROUND,        // 10Hz (100ms)
    TASK_PRIORITY_COUNT
} TaskPriority;

// Task time budgets in milliseconds
static const double TASK_BUDGET_MS[TASK_PRIORITY_COUNT] = {
    1.0,    // Physics & AI: 1ms
    1.0,    // Animation: 1ms
    8.0,    // Render & Audio: 8ms (increased from 2ms)
    5.0     // Background: 5ms
};

// Task frequencies in milliseconds
static const double TASK_INTERVAL_MS[TASK_PRIORITY_COUNT] = {
    16.67,  // Physics & AI: 60Hz (16.67ms) - Fixed time step
    6.06,   // Animation: 165Hz (6.06ms)
    6.06,   // Render & Audio: 165Hz (6.06ms)
    100.0   // Background: 10Hz (100ms)
};

// Task function pointer type
typedef void (*TaskFunction)(double dt, void* user_data);

// Task structure
typedef struct {
    TaskFunction function;
    void* user_data;
    TaskPriority priority;
    double next_execution_time;
    double last_execution_time;
    bool is_enabled;
    const char* name;
} Task;

// Scheduler initialization and shutdown
void scheduler_init(void);
void scheduler_shutdown(void);

// Task management
int scheduler_add_task(TaskFunction function, void* user_data, TaskPriority priority, const char* name);
void scheduler_remove_task(int task_id);
void scheduler_enable_task(int task_id, bool enable);

// Scheduler execution
void scheduler_run(void);
bool scheduler_should_quit(void);
void scheduler_request_quit(void);

// Timing utilities
double scheduler_get_time_ms(void);
void scheduler_sleep_until(double target_time_ms);

#endif // SCHEDULER_H 