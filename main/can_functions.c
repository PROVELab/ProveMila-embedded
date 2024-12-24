#include <stdint.h>
#include "mutex_declarations.h"

int16_t combinedID(int16_t fn_id, int16_t node_id){
    return (fn_id << 7) + node_id;
}