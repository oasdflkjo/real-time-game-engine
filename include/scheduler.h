#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

// Task priorities and frequencies
typedef enum {
    TASK_PRIORITY_PHYSICS_AI = 0,    // 100Hz (10ms)
    TASK_PRIORITY_ANIMATION,         // 160Hz (6.25ms)
    TASK_PRIORITY_RENDER_AUDIO,      // 160Hz (6.25ms)
    TASK_PRIORITY_BACKGROUND,        // 10Hz (100ms)
    TASK_PRIORITY_COUNT
} TaskPriority;

// Task time budgets in milliseconds
static const double TASK_BUDGET_MS[TASK_PRIORITY_COUNT] = {
    1.0,    // Physics & AI: 1ms
    1.0,    // Animation: 1ms
    2.0,    // Render & Audio: 2ms
    5.0     // Background: 5ms
};

// Task frequencies in milliseconds
static const double TASK_INTERVAL_MS[TASK_PRIORITY_COUNT] = {
    10.0,   // Physics & AI: 100Hz (10ms)
    6.25,   // Animation: 160Hz (6.25ms)
    6.25,   // Render & Audio: 160Hz (6.25ms)
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