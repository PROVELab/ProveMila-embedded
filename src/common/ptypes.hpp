#ifndef PTYPES_H
#define PTYPES_H

#define MAX_SIZE_PACKET_DATA 8
// DEFINITIONS, TYPEDEFS
typedef char byte;

template <typename T>
void fillBuf(byte * b, T value){
    union {
        T value;
        byte buffer[sizeof(T)];
    } convert_o_tron; // Yes, this is a kerbal space program reference
    convert_o_tron.value = value;

    for (int i = sizeof(T)-1; i >= 0; i--){
        b[i] = convert_o_tron.buffer[i];
    }
}

template <typename T>
void unFillBuf(byte * b, T * valRef){
    union {
        T value;
        byte buffer[sizeof(T)];
    } convert_o_tron;
    for (int i = 0; i < sizeof(T); i++){
        convert_o_tron.buffer[i] = b[i];
    }
    *valRef = convert_o_tron.value;
}

#endif