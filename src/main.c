#include "../include/scheduler.h"
#include "../include/game_state.h"
#include "../include/renderer.h"
#include "../include/physics.h"
#include "../include/input.h"
#include "../include/camera.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// Render task function
void render_task(double dt, void* user_data) {
    static int consecutive_heavy_frames = 0;
    static double last_frame_time = 0;
    
    // Check if the previous frame took too long
    if (last_frame_time > 20.0) { // If last frame took more than 20ms
        consecutive_heavy_frames++;
        
        // Cap the counter to prevent integer overflow
        if (consecutive_heavy_frames > 100) {
            consecutive_heavy_frames = 100;
        }
    } else {
        // Gradually reduce the counter to avoid oscillation
        if (consecutive_heavy_frames > 0) {
            consecutive_heavy_frames--;
        }
    }
    
    // Skip rendering if we've had too many consecutive heavy frames
    // This helps recover from temporary system load spikes
    if (consecutive_heavy_frames > 5) {
        LOG_WARNING(LOG_CATEGORY_RENDERER, "Skipping frame to recover from system load (last frame: %.2f ms)", 
                   last_frame_time);
        
        // Still process input events to keep the game responsive
        renderer_process_input();
        
        // Check if we should quit
        if (renderer_should_close()) {
            scheduler_request_quit();
        }
        
        return;
    }
    
    // Measure frame time
    double start_time = scheduler_get_time_ms();
    
    // Begin frame
    renderer_begin_frame();
    
    // Get the current game state for rendering
    const GameState* game_state = game_state_get_read();
    
    // Draw the game
    renderer_draw_game(game_state);
    
    // End frame
    renderer_end_frame();
    
    // Process input events
    renderer_process_input();
    
    // Check if we should quit
    if (renderer_should_close()) {
        scheduler_request_quit();
    }
    
    // Calculate frame time
    double end_time = scheduler_get_time_ms();
    last_frame_time = end_time - start_time;
    
    // Cap the frame time to a reasonable value to prevent cascading issues
    if (last_frame_time > 1000.0) {
        last_frame_time = 20.0; // Cap at 20ms if something went very wrong
    }
}

// Background task function
void background_task(double dt, void* user_data) {
    // This task runs at a lower frequency (10Hz)
    // It could handle things like:
    // - Asset loading
    // - Garbage collection
    // - Network communication
    // - AI path finding
    
    // Uncomment to log a message every second
    /*
    static int counter = 0;
    static double time_accumulator = 0.0;
    
    time_accumulator += dt;
    
    if (time_accumulator >= 1.0) {
        LOG_DEBUG(LOG_CATEGORY_GENERAL, "Background task executed %d (dt=%.2f ms)", 
                 ++counter, dt * 1000.0);
        time_accumulator = 0.0;
    }
    */
}

int main(int argc, char** argv) {
    // Initialize logging first - must be done before any logging calls
    logging_init();
    
    LOG_INFO(LOG_CATEGORY_GENERAL, "Real-Time Game Engine - Interrupt-Driven Architecture");
    
    // Set process priority to high for better real-time performance
    HANDLE process_handle = GetCurrentProcess();
    SetPriorityClass(process_handle, HIGH_PRIORITY_CLASS);
    
    // Set thread affinity to avoid CPU core switching
    HANDLE thread_handle = GetCurrentThread();
    SetThreadPriority(thread_handle, THREAD_PRIORITY_HIGHEST);
    
    // Get the system's processor count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD processorCount = sysInfo.dwNumberOfProcessors;
    
    // If we have at least 2 cores, reserve one core for the game
    // and leave the others for the OS
    if (processorCount > 1) {
        // Use the second core (index 1) for our game thread
        // This leaves core 0 for Windows system processes
        DWORD_PTR affinityMask = (1 << 1);
        SetThreadAffinityMask(thread_handle, affinityMask);
        LOG_INFO(LOG_CATEGORY_GENERAL, "Set thread affinity to processor 1 (of %d available)", processorCount);
    }
    
    // Disable Windows timer coalescing to improve timer precision
    // This requires Windows 8 or later
    typedef BOOL (WINAPI *SetTimerResolutionFuncType)(ULONG RequestedResolutionInMicroseconds, BOOLEAN Set, PULONG ActualResolutionInMicroseconds);
    
    HMODULE ntdllHandle = LoadLibraryA("ntdll.dll");
    if (ntdllHandle) {
        SetTimerResolutionFuncType NtSetTimerResolution = (SetTimerResolutionFuncType)GetProcAddress(ntdllHandle, "NtSetTimerResolution");
        if (NtSetTimerResolution) {
            ULONG actualResolution;
            // Request 1ms resolution (1000 microseconds)
            if (NtSetTimerResolution(1000, TRUE, &actualResolution)) {
                LOG_INFO(LOG_CATEGORY_GENERAL, "Set Windows timer resolution to %.2f ms", actualResolution / 10000.0);
            }
        }
    }
    
    LOG_INFO(LOG_CATEGORY_GENERAL, "====================================================");
    LOG_INFO(LOG_CATEGORY_GENERAL, "Controls: A = Move Left, D = Move Right, SPACE = Jump, ESC = Quit, F11 = Toggle Fullscreen, F10 = Toggle VSync");
    LOG_INFO(LOG_CATEGORY_GENERAL, "====================================================");
    
    // Set log levels
    logging_set_level(LOG_CATEGORY_SCHEDULER, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_PHYSICS, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_RENDERER, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_INPUT, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_CAMERA, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_GAME_STATE, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_ANIMATION, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_MEMORY, LOG_LEVEL_WARNING);
    logging_set_level(LOG_CATEGORY_GENERAL, LOG_LEVEL_INFO);
    
    // Disable some categories completely if needed
    // logging_enable_category(LOG_CATEGORY_PHYSICS, false);
    // logging_enable_category(LOG_CATEGORY_ANIMATION, false);
    
    // Initialize systems
    scheduler_init();
    game_state_init();
    
    // Initial window dimensions (may be overridden by fullscreen mode)
    int window_width = 1280;
    int window_height = 720;
    
    if (!renderer_init(window_width, window_height)) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to initialize renderer");
        return 1;
    }
    
    // Get the actual window dimensions after initialization
    renderer_get_window_size(&window_width, &window_height);
    
    // Initialize camera with actual window dimensions
    camera_init((float)window_width, (float)window_height);
    
    physics_init();
    input_init();
    
    // Add tasks to the scheduler
    scheduler_add_task(input_update, NULL, TASK_PRIORITY_PHYSICS_AI, "Input");
    scheduler_add_task(physics_update, NULL, TASK_PRIORITY_PHYSICS_AI, "Physics");
    scheduler_add_task(camera_update, NULL, TASK_PRIORITY_ANIMATION, "Camera");
    scheduler_add_task(render_task, NULL, TASK_PRIORITY_RENDER_AUDIO, "Render");
    scheduler_add_task(background_task, NULL, TASK_PRIORITY_BACKGROUND, "Background");
    
    // Run the scheduler
    scheduler_run();
    
    // Shutdown systems in reverse order
    input_shutdown();
    physics_shutdown();
    camera_shutdown();
    renderer_shutdown();
    game_state_shutdown();
    scheduler_shutdown();
    
    // Shutdown logging last
    logging_shutdown();
    
    return 0;
} 