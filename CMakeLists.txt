cmake_minimum_required(VERSION 3.10)
project(RealTimeGameEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_INSTALL OFF)
add_subdirectory(external/glfw)

# Add CGLM
option(CGLM_SHARED OFF)
option(CGLM_STATIC ON)
add_subdirectory(external/cglm)

# Find OpenGL
find_package(OpenGL REQUIRED)

# ImGui source files
set(IMGUI_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

# C source files
file(GLOB C_SOURCES "src/*.c")

# C++ source files
set(CXX_SOURCES
    src/debug_hud.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cglm/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
)

# Compiler flags to disable most warnings
if(MSVC)
    # For Visual Studio compiler
    add_compile_options(/W1)  # Set warning level to 1 (lowest)
    # Disable specific warnings but keep target-related ones
    add_compile_options(/wd4244 /wd4305 /wd4996)  # Disable conversion warnings and deprecated function warnings
else()
    # For GCC/Clang compilers
    add_compile_options(-w)  # Disable all warnings
    add_compile_options(-Wmissing-field-initializers)  # Re-enable missed target warnings
endif()

# Create executable
add_executable(${PROJECT_NAME} ${C_SOURCES} ${CXX_SOURCES} ${IMGUI_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    glfw
    cglm
    ${OPENGL_LIBRARIES}
)

# Copy resources to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Windows-specific settings
if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
endif() 