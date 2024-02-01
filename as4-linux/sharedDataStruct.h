#ifndef _SHARED_DATA_STRUCT_H_
#define _SHARED_DATA_STRUCT_H_

#include <stdbool.h>
#include <stdint.h>

// WARNING:
// Fields in the struct must be aligned to match ARM's alignment
//    bool/char, uint8_t:   byte aligned
//    int/long,  uint32_t:  word (4 byte) aligned
//    double,    uint64_t:  dword (8 byte) aligned
// Add padding fields (char _p1) to pad out to alignment.

// My Shared Memory Structure
// ----------------------------------------------------------------

#define NUM_SAMPLES 2048
typedef struct { //0x200
    uint32_t boolArrLength; // length of bool array
    uint32_t currentIndex; // current index of the bool array that the PRU is on
    bool isLEDOn;
    bool isFilledWithSamples; //is the PRU filled with samples?
    bool isButtonPressed;
    uint8_t padding1;
    bool boolArr[NUM_SAMPLES]; //bool array itself
    //char data[NUM_SAMPLES]; //PRU character array (ie. X_XX_).
    
    

} sharedMemStruct_t;

#endif
