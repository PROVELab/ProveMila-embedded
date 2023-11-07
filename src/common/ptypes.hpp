#ifndef PTYPES_H
#define PTYPES_H

// Fills byte buffer b with valRef using unions
template <typename T>
void fillBuf(char * b, T value);

// Fills valRef from b using unions
template <typename T>
void unFillBuf(char * b, T * valRef);

#endif