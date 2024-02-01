#include "Keyboard.h"
#include "MorseCode.h"
#include "pruLogicAnalyzer.h"

#include "mainHelper.h"
#include "LEDMatrix.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


// If you print something to the terminal but don’t want to print a linefeed yet (say the “>” for
// the prompt, or the “_” and “X” characters while the Morse code is flashed out) you will
// need to flush the file buffer after each printf():
// fflush(stdout);
#define I2C_DEVICE_ADDRESS 0x70

static char *buff;
static size_t numCh;

int boolArrayToInt(bool* arr, int count)
{
    int ret = 0;
    int tmp;
    for (int i = 0; i < count; i++) {
        tmp = arr[i];
        ret |= tmp << (count - i - 1);
    }
    return ret;
}

void getInput(){
    
    buff = NULL;
    
    size_t sizeAllocated = 0;
    printf(">");
    fflush(stdout);
    numCh = getline(&buff, &sizeAllocated, stdin);
    // Now use buff[] and it’s numCh characters.
    // You can ignore sizeAllocated.
    //printf("getline returned %d\n", numCh);
       // ..<your code here>..

    trimWhiteSpace(buff);
    printf("Flashing out %d characters: '%s'\n", strlen(buff), buff); 
    
    numCh = strlen(buff);
    
    generateBoolArr();

    logicAnalyzer();

}

void checkForEmptyInput(){
    while(1){
        getInput();            
            
        if(buff[0] == '\n'){
            break;
        }
       
        free(buff);

    }
}

char* getBuff(){
    return buff;
}
unsigned int getNumCh(){
    return numCh;
}
