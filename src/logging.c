#include "../include/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <windows.h>

// Maximum log message length
#define MAX_LOG_MESSAGE_LENGTH 1024

// Maximum number of log messages in the queue
#define MAX_LOG_QUEUE_SIZE 1024

// Log message structure
typedef struct {
    LogCategory category;
    LogLevel level;
    char file[256];
    int line;
    char message[MAX_LOG_MESSAGE_LENGTH];
    time_t timestamp;
} LogMessage;

// Log queue
static struct {
    LogMessage messages[MAX_LOG_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    CRITICAL_SECTION lock;
    HANDLE thread;
    HANDLE semaphore;
    bool running;
    bool category_enabled[LOG_CATEGORY_COUNT];
    LogLevel category_level[LOG_CATEGORY_COUNT];
} log_queue;

// Log level names
static const char* log_level_names[LOG_LEVEL_COUNT] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR"
};

// Log category names
static const char* log_category_names[LOG_CATEGORY_COUNT] = {
    "SCHEDULER",
    "PHYSICS",
    "RENDERER",
    "INPUT",
    "CAMERA",
    "GAME_STATE",
    "ANIMATION",
    "MEMORY",
    "GENERAL"
};

// Log level colors (ANSI escape codes)
static const char* log_level_colors[LOG_LEVEL_COUNT] = {
    "\033[36m", // Cyan for DEBUG
    "\033[32m", // Green for INFO
    "\033[33m", // Yellow for WARNING
    "\033[31m", // Red for ERROR
};

// Reset color
static const char* reset_color = "\033[0m";

// Logging thread function
DWORD WINAPI logging_thread(LPVOID lpParam) {
    while (log_queue.running) {
        // Wait for a log message
        DWORD result = WaitForSingleObject(log_queue.semaphore, 100);
        
        if (result == WAIT_OBJECT_0) {
            // Process log messages
            logging_process();
        }
    }
    
    // Process any remaining messages
    logging_process();
    
    return 0;
}

// Initialize the logging system
void logging_init(void) {
    // Initialize log queue
    memset(&log_queue, 0, sizeof(log_queue));
    InitializeCriticalSection(&log_queue.lock);
    log_queue.semaphore = CreateSemaphore(NULL, 0, MAX_LOG_QUEUE_SIZE, NULL);
    log_queue.running = true;
    
    // Enable all categories by default with INFO level
    for (int i = 0; i < LOG_CATEGORY_COUNT; i++) {
        log_queue.category_enabled[i] = true;
        log_queue.category_level[i] = LOG_LEVEL_INFO;
    }
    
    // Create logging thread
    log_queue.thread = CreateThread(NULL, 0, logging_thread, NULL, 0, NULL);
    
    // Enable ANSI colors in Windows console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
    SetConsoleMode(hOut, dwMode);
    
    // Log initialization message
    LOG_INFO(LOG_CATEGORY_GENERAL, "Logging system initialized");
}

// Shutdown the logging system
void logging_shutdown(void) {
    // Signal the logging thread to stop
    log_queue.running = false;
    
    // Wait for the logging thread to finish
    WaitForSingleObject(log_queue.thread, INFINITE);
    CloseHandle(log_queue.thread);
    
    // Clean up resources
    CloseHandle(log_queue.semaphore);
    DeleteCriticalSection(&log_queue.lock);
    
    printf("Logging system shutdown\n");
}

// Set the minimum log level for a category
void logging_set_level(LogCategory category, LogLevel level) {
    if (category < 0 || category >= LOG_CATEGORY_COUNT) {
        return;
    }
    
    if (level < 0 || level >= LOG_LEVEL_COUNT) {
        return;
    }
    
    EnterCriticalSection(&log_queue.lock);
    log_queue.category_level[category] = level;
    LeaveCriticalSection(&log_queue.lock);
    
    LOG_INFO(LOG_CATEGORY_GENERAL, "Set log level for %s to %s", 
             log_category_names[category], log_level_names[level]);
}

// Enable or disable a log category
void logging_enable_category(LogCategory category, bool enable) {
    if (category < 0 || category >= LOG_CATEGORY_COUNT) {
        return;
    }
    
    EnterCriticalSection(&log_queue.lock);
    log_queue.category_enabled[category] = enable;
    LeaveCriticalSection(&log_queue.lock);
    
    LOG_INFO(LOG_CATEGORY_GENERAL, "%s logging for %s", 
             enable ? "Enabled" : "Disabled", log_category_names[category]);
}

// Log a message
void logging_log(LogCategory category, LogLevel level, const char* file, int line, const char* format, ...) {
    // Check if this category and level should be logged
    EnterCriticalSection(&log_queue.lock);
    bool should_log = log_queue.category_enabled[category] && level >= log_queue.category_level[category];
    LeaveCriticalSection(&log_queue.lock);
    
    if (!should_log) {
        return;
    }
    
    // Format the message
    va_list args;
    va_start(args, format);
    
    EnterCriticalSection(&log_queue.lock);
    
    // Check if the queue is full
    if (log_queue.count >= MAX_LOG_QUEUE_SIZE) {
        LeaveCriticalSection(&log_queue.lock);
        va_end(args);
        return;
    }
    
    // Add the message to the queue
    LogMessage* message = &log_queue.messages[log_queue.tail];
    message->category = category;
    message->level = level;
    strncpy(message->file, file, sizeof(message->file) - 1);
    message->file[sizeof(message->file) - 1] = '\0';
    message->line = line;
    time(&message->timestamp);
    
    vsnprintf(message->message, MAX_LOG_MESSAGE_LENGTH, format, args);
    
    // Update the queue
    log_queue.tail = (log_queue.tail + 1) % MAX_LOG_QUEUE_SIZE;
    log_queue.count++;
    
    // Signal the logging thread
    ReleaseSemaphore(log_queue.semaphore, 1, NULL);
    
    LeaveCriticalSection(&log_queue.lock);
    
    va_end(args);
}

// Process log messages
void logging_process(void) {
    EnterCriticalSection(&log_queue.lock);
    
    while (log_queue.count > 0) {
        // Get the next message
        LogMessage* message = &log_queue.messages[log_queue.head];
        
        // Format the timestamp
        char timestamp[32];
        struct tm* tm_info = localtime(&message->timestamp);
        strftime(timestamp, sizeof(timestamp), "%H:%M:%S", tm_info);
        
        // Extract the filename from the path
        const char* filename = strrchr(message->file, '/');
        if (!filename) {
            filename = strrchr(message->file, '\\');
        }
        if (filename) {
            filename++; // Skip the slash
        } else {
            filename = message->file;
        }
        
        // Print the message
        printf("%s%s [%s] [%s] [%s:%d] %s%s\n",
               log_level_colors[message->level],
               timestamp,
               log_level_names[message->level],
               log_category_names[message->category],
               filename,
               message->line,
               message->message,
               reset_color);
        
        // Update the queue
        log_queue.head = (log_queue.head + 1) % MAX_LOG_QUEUE_SIZE;
        log_queue.count--;
    }
    
    LeaveCriticalSection(&log_queue.lock);
} 