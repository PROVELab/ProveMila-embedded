#include <Arduino.h>

// Define logging levels
#if defined(DEBUG)
    #define DEBUG_ENABLED
    #define WARN_ENABLED
    #define INFO_ENABLED
    #define ERROR_ENABLED
#elif defined(WARN)
    #define WARN_ENABLED
    #define INFO_ENABLED
    #define ERROR_ENABLED
#elif defined(INFO)
    #define INFO_ENABLED
    #define ERROR_ENABLED
#elif defined(ERROR)
    #define ERROR_ENABLED
#endif

#ifdef DEBUG_ENABLED
    #define DEBUG(string) Serial.printf("\033[92mDEBUG:%s:%d:%s\033[0m\n",  __FILE__, __LINE__, string);
#elif defined(WARN_ENABLED)
    #define WARN(string) Serial.printf("\033[92mWARN:%s:%d:%s\033[0m\n",  __FILE__, __LINE__, string);
#elif defined(INFO_ENABLED)
    #define INFO(string) Serial.printf("\033[92mINFO:%s:%d:%s\033[0m\n",  __FILE__, __LINE__, string);
#elif defined(ERROR_ENABLED)
    #define ERROR(string) Serial.printf("\033[92mERROR:%s:%d:%s\033[0m\n",  __FILE__, __LINE__, string);



int main(){
    DEBUG("This is a debug");
    WARN("This is a warning");
    INFO("This is an info");
    ERROR("This is an error");
}



