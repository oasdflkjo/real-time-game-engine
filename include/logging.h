#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log levels
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_COUNT
} LogLevel;

// Log categories
typedef enum {
    LOG_CATEGORY_SCHEDULER,
    LOG_CATEGORY_PHYSICS,
    LOG_CATEGORY_RENDERER,
    LOG_CATEGORY_INPUT,
    LOG_CATEGORY_CAMERA,
    LOG_CATEGORY_GAME_STATE,
    LOG_CATEGORY_ANIMATION,
    LOG_CATEGORY_MEMORY,
    LOG_CATEGORY_GENERAL,
    LOG_CATEGORY_GAME,
    LOG_CATEGORY_COUNT
} LogCategory;

// Initialize the logging system
void logging_init(void);

// Shutdown the logging system
void logging_shutdown(void);

// Set the minimum log level for a category
void logging_set_level(LogCategory category, LogLevel level);

// Enable or disable a log category
void logging_enable_category(LogCategory category, bool enable);

// Log a message
void logging_log(LogCategory category, LogLevel level, const char* file, int line, const char* format, ...);

// Process log messages (called by the logging thread)
void logging_process(void);

// Convenience macros for logging
#define LOG_DEBUG(category, ...) logging_log(category, LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(category, ...) logging_log(category, LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARNING(category, ...) logging_log(category, LOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(category, ...) logging_log(category, LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // LOGGING_H 