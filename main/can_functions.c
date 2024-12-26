#include <stdint.h>
#include "mutex_declarations.h"
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/twai.h"
int16_t combinedID(int16_t fn_id, int16_t node_id){
    return (fn_id << 7) + node_id;
}
