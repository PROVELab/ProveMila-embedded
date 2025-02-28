// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// Define log levels in ascending order of severity
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE  4

#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_ORANGE  "\033[38;5;208m"

#define INFO(string) do { \
    Serial.print(ANSI_COLOR_GREEN "INFO:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(string); \
} while(0)


#define DEBUG(string) do { \
    Serial.print(ANSI_COLOR_ORANGE "DEBUG:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(string); \
} while(0)

#define WARN(string) do { \
    Serial.print(ANSI_COLOR_YELLOW "WARN:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(string); \
} while(0)

#define ERROR(string) do { \
    Serial.print(ANSI_COLOR_RED "ERROR:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(string); \
} while(0)



// Check which log level is defined
#if defined(DEBUG)
  #define LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(INFO)
  #define LOG_LEVEL LOG_LEVEL_INFO
#elif defined(WARN)
  #define LOG_LEVEL LOG_LEVEL_WARN
#elif defined(ERROR)
  #define LOG_LEVEL LOG_LEVEL_ERROR
#else
  #define LOG_LEVEL LOG_LEVEL_INFO  // Default to INFO level if nothing is defined
#endif

// Debug level - only enabled if log level is DEBUG
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
  #define debug(message) do { \
    Serial.print("DEBUG:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(message); \
  } while(0)
#else
  #define debug(message) do {} while(0)
#endif

// Info level - enabled if log level is DEBUG or INFO
#if LOG_LEVEL <= LOG_LEVEL_INFO
  #define info(message) do { \
    Serial.print("INFO:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(message); \
  } while(0)
#else
  #define info(message) do {} while(0)
#endif

// Warning level - enabled if log level is DEBUG, INFO, or WARN
#if LOG_LEVEL <= LOG_LEVEL_WARN
  #define warn(message) do { \
    Serial.print("WARN:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(message); \
  } while(0)
#else
  #define warn(message) do {} while(0)
#endif

// Error level - always enabled unless NONE is specified
#if LOG_LEVEL <= LOG_LEVEL_ERROR
  #define error(message) do { \
    Serial.print("ERROR:"); \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(": "); \
    Serial.println(message); \
  } while(0)
#else
  #define error(message) do {} while(0)
#endif

// Initialize logger
inline void initLogger(long baudRate = 115200) {
  Serial.begin(baudRate);
  while (!Serial && !Serial.available()) {
    delay(10);  // Wait for serial connection
  }
}

#endif // LOGGER_H