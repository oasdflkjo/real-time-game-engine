#include "../include/debug_hud.h"
#include "../include/logging.h"
#include "../include/physics.h"
#include "../include/scheduler.h"
#include <stdio.h>
#include <string.h>

// ImGui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

// Debug HUD state
static struct {
    bool visible;
    bool show_demo_window;
    bool show_physics_window;
    bool show_performance_window;
} debug_hud_state;

// Initialize the debug HUD
extern "C" void debug_hud_init(void) {
    // Reset debug HUD state
    debug_hud_state.visible = true;
    debug_hud_state.show_demo_window = false;
    debug_hud_state.show_physics_window = true;
    debug_hud_state.show_performance_window = true;
    
    // Get the GLFW window from the current context
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) {
        LOG_ERROR(LOG_CATEGORY_RENDERER, "No GLFW window context available for ImGui initialization");
        return;
    }
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls
    
    // Setup ImGui style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Debug HUD initialized");
}

// Shutdown the debug HUD
extern "C" void debug_hud_shutdown(void) {
    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    LOG_INFO(LOG_CATEGORY_RENDERER, "Debug HUD shutdown");
}

// Render the physics debug window
static void render_physics_window(const GameState* state) {
    if (!ImGui::Begin("Physics Debug", &debug_hud_state.show_physics_window)) {
        ImGui::End();
        return;
    }
    
    // Player position and velocity
    ImGui::Text("Player Position: (%.2f, %.2f)", state->player.position_x, state->player.position_y);
    ImGui::Text("Player Velocity: (%.2f, %.2f)", state->player.velocity_x, state->player.velocity_y);
    ImGui::Text("Player Grounded: %s", state->player.is_grounded ? "Yes" : "No");
    ImGui::Text("Player Jumping: %s", state->player.is_jumping ? "Yes" : "No");
    
    // Ground planes
    if (ImGui::CollapsingHeader("Ground Planes")) {
        for (int i = 0; i < state->ground_count; i++) {
            const GroundState* ground = &state->grounds[i];
            ImGui::Text("Ground %d: (%.2f, %.2f) [%.2f x %.2f]", 
                       i, ground->position_x, ground->position_y, ground->width, ground->height);
        }
    }
    
    ImGui::End();
}

// Render the performance debug window
static void render_performance_window(void) {
    if (!ImGui::Begin("Performance", &debug_hud_state.show_performance_window)) {
        ImGui::End();
        return;
    }
    
    // FPS counter
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
               1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    
    // Game time
    double game_time = scheduler_get_time_ms() / 1000.0;
    ImGui::Text("Game Time: %.2f seconds", game_time);
    
    ImGui::End();
}

// Render the debug HUD
extern "C" void debug_hud_render(const GameState* state) {
    if (!debug_hud_state.visible) {
        return;
    }
    
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Show ImGui demo window if enabled
    if (debug_hud_state.show_demo_window) {
        ImGui::ShowDemoWindow(&debug_hud_state.show_demo_window);
    }
    
    // Show physics debug window if enabled
    if (debug_hud_state.show_physics_window) {
        render_physics_window(state);
    }
    
    // Show performance debug window if enabled
    if (debug_hud_state.show_performance_window) {
        render_performance_window();
    }
    
    // Main menu bar
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Debug")) {
            ImGui::MenuItem("Demo Window", NULL, &debug_hud_state.show_demo_window);
            ImGui::MenuItem("Physics", NULL, &debug_hud_state.show_physics_window);
            ImGui::MenuItem("Performance", NULL, &debug_hud_state.show_performance_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Toggle the debug HUD visibility
extern "C" void debug_hud_toggle(void) {
    debug_hud_state.visible = !debug_hud_state.visible;
    LOG_INFO(LOG_CATEGORY_RENDERER, "Debug HUD visibility: %s", debug_hud_state.visible ? "On" : "Off");
}

// Check if the debug HUD is visible
extern "C" bool debug_hud_is_visible(void) {
    return debug_hud_state.visible;
} 