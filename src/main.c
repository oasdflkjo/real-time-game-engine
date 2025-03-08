#include "../include/scheduler.h"
#include "../include/memory_pool.h"
#include "../include/game_state.h"
#include "../include/renderer.h"
#include "../include/physics.h"
#include "../include/input.h"
#include "../include/animation.h"
#include "../include/camera.h"
#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>

// Render task function
void render_task(double dt, void* user_data) {
    // Begin frame
    renderer_begin_frame();
    
    // Get interpolated game state for rendering
    GameState render_state;
    animation_get_render_state(&render_state);
    
    // Draw the game
    renderer_draw_game(&render_state);
    
    // End frame
    renderer_end_frame();
    
    // Process input events
    renderer_process_input();
    
    // Check if we should quit
    if (renderer_should_close()) {
        scheduler_request_quit();
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
    // Initialize logging first
    logging_init();
    
    LOG_INFO(LOG_CATEGORY_GENERAL, "Real-Time Game Engine - Interrupt-Driven Architecture");
    LOG_INFO(LOG_CATEGORY_GENERAL, "====================================================");
    LOG_INFO(LOG_CATEGORY_GENERAL, "Controls: A = Move Left, D = Move Right, SPACE = Jump, ESC = Quit");
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
    
    // Window dimensions
    int window_width = 800;
    int window_height = 600;
    
    if (!renderer_init(window_width, window_height)) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "Failed to initialize renderer");
        return 1;
    }
    
    // Initialize camera with window dimensions
    camera_init((float)window_width, (float)window_height);
    
    physics_init();
    input_init();
    animation_init();
    
    // Add tasks to the scheduler
    scheduler_add_task(input_update, NULL, TASK_PRIORITY_PHYSICS_AI, "Input");
    scheduler_add_task(physics_update, NULL, TASK_PRIORITY_PHYSICS_AI, "Physics");
    scheduler_add_task(camera_update, NULL, TASK_PRIORITY_ANIMATION, "Camera");
    scheduler_add_task(animation_update, NULL, TASK_PRIORITY_ANIMATION, "Animation");
    scheduler_add_task(render_task, NULL, TASK_PRIORITY_RENDER_AUDIO, "Render");
    scheduler_add_task(background_task, NULL, TASK_PRIORITY_BACKGROUND, "Background");
    
    // Run the scheduler
    scheduler_run();
    
    // Shutdown systems in reverse order
    animation_shutdown();
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