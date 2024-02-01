#include "displayMatrix.h" 
#include "mainHelper.h"
#include "LEDMatrix.h"
#include "pruLogicAnalyzer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>

static pthread_t threadMatrix;
static bool stopDisplay = false;

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


static void* displayMatrix(void* arg){
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, 0x70);
    while(!stopDisplay){
        bool* next8 = getNext8();
        int rowCode = boolArrayToInt(next8, 8); 
        writeI2cReg(i2cFileDesc, 0x00, rowCode);
        writeI2cReg(i2cFileDesc, 0x02, rowCode);
        writeI2cReg(i2cFileDesc, 0x04, rowCode);
        writeI2cReg(i2cFileDesc, 0x06, rowCode);
        sleepForMs(300);
        
        

        
    }
    return NULL;
}


void clearDisplay(){
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    for(int i = 0; i < 16; i+=2){
        writeI2cReg(i2cFileDesc, i, 0x00);
    }
}

void displayMatrix_startDisplay(){
    pthread_create(&threadMatrix, NULL, displayMatrix, NULL);
}

void displayMatrix_stopDisplay(){
    stopDisplay = true;
    pthread_join(threadMatrix, NULL);
}


