#include "../include/scheduler.h"
#include "../include/memory_pool.h"
#include "../include/game_state.h"
#include "../include/renderer.h"
#include "../include/physics.h"
#include "../include/input.h"
#include "../include/animation.h"
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
    
    // For now, just print a message every second
    static int counter = 0;
    static double time_accumulator = 0.0;
    
    time_accumulator += dt;
    
    if (time_accumulator >= 1.0) {
        printf("[Background] Task executed %d (dt=%.2f ms)\n", ++counter, dt * 1000.0);
        time_accumulator = 0.0;
    }
}

int main(int argc, char** argv) {
    printf("Real-Time Game Engine - Interrupt-Driven Architecture\n");
    printf("====================================================\n");
    printf("Controls: A = Move Left, D = Move Right, SPACE = Jump, ESC = Quit\n");
    printf("====================================================\n");
    
    // Initialize systems
    scheduler_init();
    game_state_init();
    
    if (!renderer_init(800, 600)) {
        fprintf(stderr, "Failed to initialize renderer\n");
        return 1;
    }
    
    physics_init();
    input_init();
    animation_init();
    
    // Add tasks to the scheduler
    scheduler_add_task(input_update, NULL, TASK_PRIORITY_PHYSICS_AI, "Input");
    scheduler_add_task(physics_update, NULL, TASK_PRIORITY_PHYSICS_AI, "Physics");
    scheduler_add_task(animation_update, NULL, TASK_PRIORITY_ANIMATION, "Animation");
    scheduler_add_task(render_task, NULL, TASK_PRIORITY_RENDER_AUDIO, "Render");
    scheduler_add_task(background_task, NULL, TASK_PRIORITY_BACKGROUND, "Background");
    
    // Run the scheduler
    scheduler_run();
    
    // Shutdown systems in reverse order
    animation_shutdown();
    input_shutdown();
    physics_shutdown();
    renderer_shutdown();
    game_state_shutdown();
    scheduler_shutdown();
    
    printf("Engine shutdown complete\n");
    
    return 0;
} 