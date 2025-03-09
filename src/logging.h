// Logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

//Define log levels in ascending order of severity
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE  4

#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_ORANGE  "\033[38;5;208m"
#define ANSI_COLOR_RESET "\033[0m"

//Platform-Specific Serial Handling
#if defined(ESP32)
    #define LOG_PRINT(...) Serial.printf(__VA_ARGS__)  //use printf on ESP32
#else
    #define LOG_PRINT(format, ...) do { \
      char logBuffer[128]; \
      snprintf(logBuffer, sizeof(logBuffer), format, ##__VA_ARGS__); \
      Serial.println(logBuffer); \
    } while (0)
#endif

//Check which log level is defined
#if defined(DEBUG)
  #define LOG_LEVEL LOG_LEVEL_DEBUG
#elif defined(INFO)
  #define LOG_LEVEL LOG_LEVEL_INFO
#elif defined(WARN)
  #define LOG_LEVEL LOG_LEVEL_WARN
#elif defined(ERROR)
  #define LOG_LEVEL LOG_LEVEL_ERROR
#else
  #define LOG_LEVEL LOG_LEVEL_NONE  //default to INFO level if nothing is defined
#endif

//logging Macros 
#if LOG_LEVEL <= LOG_LEVEL_DEBUG
    #define debug(format, ...) LOG_PRINT(ANSI_COLOR_ORANGE "DEBUG: %s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #define info(format, ...) LOG_PRINT(ANSI_COLOR_GREEN "INFO:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #define warn(format, ...) LOG_PRINT(ANSI_COLOR_YELLOW "WARN:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #define error(format, ...) LOG_PRINT(ANSI_COLOR_RED "ERROR:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__) 
#elif LOG_LEVEL <= LOG_LEVEL_INFO
    #define debug(...) do {} while (0)
    #define info(format, ...) LOG_PRINT(ANSI_COLOR_GREEN "INFO:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #define warn(format, ...) LOG_PRINT(ANSI_COLOR_YELLOW "WARN:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #define error(format, ...) LOG_PRINT(ANSI_COLOR_RED "ERROR:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#elif LOG_LEVEL <= LOG_LEVEL_WARN
    #define debug(...) do {} while (0)
    #define info(...) do {} while (0)
    #define warn(format, ...) LOG_PRINT(ANSI_COLOR_YELLOW "WARN:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
    #define error(format, ...) LOG_PRINT(ANSI_COLOR_RED "ERROR:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#elif LOG_LEVEL <= LOG_LEVEL_ERROR
    #define debug(...) do {} while (0)
    #define info(...) do {} while (0)
    #define warn(...) do {} while (0)
    #define error(format, ...) LOG_PRINT(ANSI_COLOR_RED "ERROR:%s:%d: " format ANSI_COLOR_RESET "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define debug(...) do {} while (0)
    #define info(...) do {} while (0)
    #define warn(...) do {} while (0)
    #define error(...) do {} while (0)
#endif

#endif // LOGGER_H