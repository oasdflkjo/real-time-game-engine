#include "../include/scheduler.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 64
#define PERF_HISTORY_SIZE 60  // Store 60 seconds of performance data

// Performance monitoring structure
typedef struct {
    double task_execution_times[TASK_PRIORITY_COUNT][PERF_HISTORY_SIZE];
    int current_index;
    double last_report_time;
} PerfMonitor;

// Scheduler state
static struct {
    Task tasks[MAX_TASKS];
    int task_count;
    bool should_quit;
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_time;
    PerfMonitor perf_monitor;
} scheduler;

// Initialize the scheduler
void scheduler_init(void) {
    // Clear all scheduler data
    memset(&scheduler, 0, sizeof(scheduler));
    
    // Initialize high-precision timer
    QueryPerformanceFrequency(&scheduler.frequency);
    QueryPerformanceCounter(&scheduler.start_time);
    
    // Initialize performance monitor
    for (int i = 0; i < TASK_PRIORITY_COUNT; i++) {
        for (int j = 0; j < PERF_HISTORY_SIZE; j++) {
            scheduler.perf_monitor.task_execution_times[i][j] = 0.0;
        }
    }
    scheduler.perf_monitor.current_index = 0;
    scheduler.perf_monitor.last_report_time = 0;
    
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

// Record task execution time for performance monitoring
static void record_task_execution(TaskPriority priority, double execution_time_ms) {
    if (priority < TASK_PRIORITY_COUNT && priority >= 0) {
        scheduler.perf_monitor.task_execution_times[priority][scheduler.perf_monitor.current_index] = execution_time_ms;
    }
}

// Update performance monitor and report statistics periodically
static void update_performance_monitor(double current_time_ms) {
    // Only start monitoring after we've been running for at least 1 second
    if (scheduler.perf_monitor.last_report_time == 0) {
        scheduler.perf_monitor.last_report_time = current_time_ms;
        return;
    }
    
    // Update index every second
    if (current_time_ms - scheduler.perf_monitor.last_report_time >= 1000.0) {
        // Move to next slot in circular buffer
        scheduler.perf_monitor.current_index = (scheduler.perf_monitor.current_index + 1) % PERF_HISTORY_SIZE;
        
        // Clear the new slot
        for (int priority = 0; priority < TASK_PRIORITY_COUNT; priority++) {
            scheduler.perf_monitor.task_execution_times[priority][scheduler.perf_monitor.current_index] = 0.0;
        }
        
        // Report statistics every 10 seconds
        if ((scheduler.perf_monitor.current_index % 10) == 0) {
            // Calculate statistics for each task type
            for (int priority = 0; priority < TASK_PRIORITY_COUNT; priority++) {
                double avg = 0.0;
                double max = 0.0;
                int count = 0;
                
                // Calculate over the last 10 seconds
                for (int i = 0; i < 10; i++) {
                    int idx = (scheduler.perf_monitor.current_index - i + PERF_HISTORY_SIZE) % PERF_HISTORY_SIZE;
                    if (idx >= 0 && idx < PERF_HISTORY_SIZE) {
                        double time = scheduler.perf_monitor.task_execution_times[priority][idx];
                        if (time > 0) {
                            avg += time;
                            if (time > max) max = time;
                            count++;
                        }
                    }
                }
                
                if (count > 0) {
                    avg /= count;
                    const char* priority_names[] = {"Physics", "Animation", "Render", "Background"};
                    if (priority < sizeof(priority_names)/sizeof(priority_names[0])) {
                        LOG_INFO(LOG_CATEGORY_SCHEDULER, "Performance [%s]: avg=%.2f ms, max=%.2f ms (budget=%.2f ms)",
                               priority_names[priority], avg, max, TASK_BUDGET_MS[priority]);
                    }
                }
            }
        }
        
        scheduler.perf_monitor.last_report_time = current_time_ms;
    }
}

// Execute a task with time budget enforcement
static void execute_task(Task* task, double current_time_ms) {
    if (!task || !task->is_enabled || !task->function) {
        return;
    }
    
    double dt = (task->last_execution_time > 0) 
              ? (current_time_ms - task->last_execution_time) 
              : TASK_INTERVAL_MS[task->priority];
    
    // Cap dt to a reasonable value to prevent huge jumps after long pauses
    if (dt > 1000.0) {  // Cap at 1 second
        dt = TASK_INTERVAL_MS[task->priority];
    }
    
    double start_time_ms = scheduler_get_time_ms();
    double budget_ms = TASK_BUDGET_MS[task->priority];
    
    // Execute the task
    task->function(dt, task->user_data);
    
    double execution_time_ms = scheduler_get_time_ms() - start_time_ms;
    
    // Record execution time for performance monitoring
    record_task_execution(task->priority, execution_time_ms);
    
    // Check if the task exceeded its time budget
    if (execution_time_ms > budget_ms) {
        // Static variables to track warning frequency per task
        static double warning_cooldowns[MAX_TASKS] = {0};
        static int warning_counts[MAX_TASKS] = {0};
        static double max_overruns[MAX_TASKS] = {0};
        
        // Find task index
        int task_idx = -1;
        for (int i = 0; i < scheduler.task_count && i < MAX_TASKS; i++) {
            if (&scheduler.tasks[i] == task) {
                task_idx = i;
                break;
            }
        }
        
        if (task_idx >= 0 && task_idx < MAX_TASKS) {
            warning_counts[task_idx]++;
            max_overruns[task_idx] = (execution_time_ms > max_overruns[task_idx]) ? 
                                     execution_time_ms : max_overruns[task_idx];
            
            warning_cooldowns[task_idx] -= dt;
            if (warning_cooldowns[task_idx] <= 0.0) {
                const char* task_name = task->name ? task->name : "Unknown";
                
                if (warning_counts[task_idx] > 1) {
                    LOG_WARNING(LOG_CATEGORY_SCHEDULER, 
                               "Task '%s' exceeded time budget %d times in the last second (max: %.3f ms, budget: %.3f ms)",
                               task_name, warning_counts[task_idx], max_overruns[task_idx], budget_ms);
                } else {
                    LOG_WARNING(LOG_CATEGORY_SCHEDULER, 
                               "Task '%s' exceeded time budget: %.3f ms (budget: %.3f ms)",
                               task_name, execution_time_ms, budget_ms);
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
        
        // Update performance monitor
        update_performance_monitor(current_time_ms);
        
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