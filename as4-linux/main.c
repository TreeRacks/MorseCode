#include "LEDMatrix.h"
#include "Button.h"
#include "MorseCode.h"
#include "displayMatrix.h"
#include "Keyboard.h"
#include "mainHelper.h"
#include "pruLogicAnalyzer.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define I2C_DEVICE_ADDRESS 0x70


int main(){
    configurePRUPins();
    configureI2C();
    initializeStartRegisters(); 
    configureAllPins();
    printf("Starting Morse Code, OH YEAH BABY!!!!!\n");
    //displayMatrix_startDisplay();
    
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    
    checkForEmptyInput();
    
    
    //stop_stopButton();
    //displayMatrix_stopDisplay();
    close(i2cFileDesc);
    printf("\nDone shutdown! Goodbye!\n");
    return 0;
}
