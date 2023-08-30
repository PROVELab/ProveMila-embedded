#ifndef PTYPES_H
#define PTYPES_H

#define MAX_SIZE_PACKET_DATA 8
// DEFINITIONS, TYPEDEFS
#define MAX_TASK_COUNT 20

template <typename T>
void fillBuf(char * b, T value){
    union {
        T value;
        char buffer[sizeof(T)];
    } convert_o_tron; // Yes, this is a kerbal space program reference
    convert_o_tron.value = value;

    for (int i = sizeof(T)-1; i >= 0; i--){
        b[i] = convert_o_tron.buffer[i];
    }
}

template <typename T>
void unFillBuf(char * b, T * valRef){
    union {
        T value;
        char buffer[sizeof(T)];
    } convert_o_tron;
    for (int i = 0; i < sizeof(T); i++){
        convert_o_tron.buffer[i] = b[i];
    }
    *valRef = convert_o_tron.value;
}

#endif