#include "../include/scheduler.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 64

// Scheduler state
static struct {
    Task tasks[MAX_TASKS];
    int task_count;
    bool should_quit;
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_time;
} scheduler;

// Initialize the scheduler
void scheduler_init(void) {
    memset(&scheduler, 0, sizeof(scheduler));
    
    // Initialize high-precision timer
    QueryPerformanceFrequency(&scheduler.frequency);
    QueryPerformanceCounter(&scheduler.start_time);
    
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "Initialized with timer frequency: %lld Hz", 
           scheduler.frequency.QuadPart);
}

// Shutdown the scheduler
void scheduler_shutdown(void) {
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "Shutting down with %d tasks", scheduler.task_count);
    scheduler.task_count = 0;
}

// Get current time in milliseconds
double scheduler_get_time_ms(void) {
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    return (double)(current_time.QuadPart - scheduler.start_time.QuadPart) * 1000.0 / 
           (double)scheduler.frequency.QuadPart;
}

// Sleep until a specific time
void scheduler_sleep_until(double target_time_ms) {
    double current_time_ms = scheduler_get_time_ms();
    double sleep_time_ms = target_time_ms - current_time_ms;
    
    if (sleep_time_ms <= 0) {
        return;  // Already past the target time
    }
    
    // For longer sleeps, use Sleep to save CPU
    if (sleep_time_ms > 2.0) {
        Sleep((DWORD)(sleep_time_ms - 1.0));  // Sleep a bit less to account for Sleep's imprecision
    }
    
    // Busy-wait for the remaining time to ensure precision
    while (scheduler_get_time_ms() < target_time_ms) {
        // Busy wait
    }
}

// Add a task to the scheduler
int scheduler_add_task(TaskFunction function, void* user_data, TaskPriority priority, const char* name) {
    if (scheduler.task_count >= MAX_TASKS) {
        LOG_ERROR(LOG_CATEGORY_SCHEDULER, "Cannot add task, maximum task count reached");
        return -1;
    }
    
    if (priority >= TASK_PRIORITY_COUNT) {
        LOG_ERROR(LOG_CATEGORY_SCHEDULER, "Invalid task priority: %d", priority);
        return -1;
    }
    
    int task_id = scheduler.task_count++;
    Task* task = &scheduler.tasks[task_id];
    
    task->function = function;
    task->user_data = user_data;
    task->priority = priority;
    task->next_execution_time = scheduler_get_time_ms();
    task->last_execution_time = 0;
    task->is_enabled = true;
    task->name = name;
    
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "Added task '%s' with priority %d, interval %.2f ms, budget %.2f ms",
           name, priority, TASK_INTERVAL_MS[priority], TASK_BUDGET_MS[priority]);
    
    return task_id;
}

// Remove a task from the scheduler
void scheduler_remove_task(int task_id) {
    if (task_id < 0 || task_id >= scheduler.task_count) {
        LOG_ERROR(LOG_CATEGORY_SCHEDULER, "Invalid task ID: %d", task_id);
        return;
    }
    
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "Removing task '%s'", scheduler.tasks[task_id].name);
    
    // Move the last task to this position (if it's not already the last)
    if (task_id < scheduler.task_count - 1) {
        scheduler.tasks[task_id] = scheduler.tasks[scheduler.task_count - 1];
    }
    
    scheduler.task_count--;
}

// Enable or disable a task
void scheduler_enable_task(int task_id, bool enable) {
    if (task_id < 0 || task_id >= scheduler.task_count) {
        LOG_ERROR(LOG_CATEGORY_SCHEDULER, "Invalid task ID: %d", task_id);
        return;
    }
    
    scheduler.tasks[task_id].is_enabled = enable;
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "%s task '%s'", 
           enable ? "Enabled" : "Disabled", 
           scheduler.tasks[task_id].name);
}

// Execute a task with time budget enforcement
static void execute_task(Task* task, double current_time_ms) {
    if (!task->is_enabled) {
        return;
    }
    
    double dt = (task->last_execution_time > 0) 
              ? (current_time_ms - task->last_execution_time) 
              : TASK_INTERVAL_MS[task->priority];
    
    double start_time_ms = scheduler_get_time_ms();
    double budget_ms = TASK_BUDGET_MS[task->priority];
    
    // Execute the task
    task->function(dt, task->user_data);
    
    double execution_time_ms = scheduler_get_time_ms() - start_time_ms;
    
    // Check if the task exceeded its time budget
    if (execution_time_ms > budget_ms) {
        // Static variables to track warning frequency per task
        static double warning_cooldowns[MAX_TASKS] = {0};
        static int warning_counts[MAX_TASKS] = {0};
        static double max_overruns[MAX_TASKS] = {0};
        
        // Find task index
        int task_idx = -1;
        for (int i = 0; i < scheduler.task_count; i++) {
            if (&scheduler.tasks[i] == task) {
                task_idx = i;
                break;
            }
        }
        
        if (task_idx >= 0) {
            warning_counts[task_idx]++;
            max_overruns[task_idx] = (execution_time_ms > max_overruns[task_idx]) ? 
                                     execution_time_ms : max_overruns[task_idx];
            
            warning_cooldowns[task_idx] -= dt;
            if (warning_cooldowns[task_idx] <= 0.0) {
                if (warning_counts[task_idx] > 1) {
                    LOG_WARNING(LOG_CATEGORY_SCHEDULER, 
                               "Task '%s' exceeded time budget %d times in the last second (max: %.3f ms, budget: %.3f ms)",
                               task->name, warning_counts[task_idx], max_overruns[task_idx], budget_ms);
                } else {
                    LOG_WARNING(LOG_CATEGORY_SCHEDULER, 
                               "Task '%s' exceeded time budget: %.3f ms (budget: %.3f ms)",
                               task->name, execution_time_ms, budget_ms);
                }
                warning_cooldowns[task_idx] = 1.0; // Reset cooldown to 1 second
                warning_counts[task_idx] = 0;      // Reset counter
                max_overruns[task_idx] = 0;        // Reset max overrun
            }
        }
    }
    
    // Update task timing
    task->last_execution_time = current_time_ms;
    task->next_execution_time = current_time_ms + TASK_INTERVAL_MS[task->priority];
}

// Run the scheduler main loop
void scheduler_run(void) {
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "Starting main loop with %d tasks", scheduler.task_count);
    
    while (!scheduler.should_quit) {
        double current_time_ms = scheduler_get_time_ms();
        bool tasks_executed = false;
        
        // Check each task to see if it's due for execution
        for (int i = 0; i < scheduler.task_count; i++) {
            Task* task = &scheduler.tasks[i];
            
            if (current_time_ms >= task->next_execution_time) {
                execute_task(task, current_time_ms);
                tasks_executed = true;
            }
        }
        
        // If no tasks were executed, sleep for a short time to avoid busy-waiting
        if (!tasks_executed) {
            // Find the next task that needs to be executed
            double next_execution_time = current_time_ms + 1000.0;  // Default to 1 second
            
            for (int i = 0; i < scheduler.task_count; i++) {
                if (scheduler.tasks[i].is_enabled && 
                    scheduler.tasks[i].next_execution_time < next_execution_time) {
                    next_execution_time = scheduler.tasks[i].next_execution_time;
                }
            }
            
            // Sleep until the next task is due
            scheduler_sleep_until(next_execution_time);
        }
    }
    
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "Main loop exited");
}

// Check if the scheduler should quit
bool scheduler_should_quit(void) {
    return scheduler.should_quit;
}

// Request the scheduler to quit
void scheduler_request_quit(void) {
    LOG_INFO(LOG_CATEGORY_SCHEDULER, "Quit requested");
    scheduler.should_quit = true;
} 