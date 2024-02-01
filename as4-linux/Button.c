#include "Button.h"
#include "mainHelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>

#define yellowButtonPath "/sys/class/gpio/gpio27/value"

static pthread_t threadButton;
static pthread_mutex_t buttonMutex = PTHREAD_MUTEX_INITIALIZER;
static bool stopButton = false;

static void* Button(void* arg){
    while(!stopButton){
        if(yellowButtonPressed()){
            while(yellowButtonPressed()){};

            sleepForMs(100);
        } 
        sleepForMs(10); 
    }
    return NULL;
}


void start_startButton(){
    pthread_create(&threadButton, NULL, Button, NULL);
}

void stop_stopButton(){
    stopButton = true;
    pthread_join(threadButton, NULL);
}

int readButton(char *button)
{
    FILE *pFile = fopen(button, "r");
    if (pFile == NULL) {
        printf("ERROR: Unable to open file (%s) for read\n", button);
        exit(-1);
    }
    // Read string (line)
    const int MAX_LENGTH = 1024;
    char buff[MAX_LENGTH];
    fgets(buff, MAX_LENGTH, pFile);
    // Close
    fclose(pFile);
    // printf("Read: '%s'\n", buff);
    return(atoi(buff));
}

void writingToGPIO(float value){
    FILE *pFile = fopen("/sys/class/gpio/export", "w");
    if (pFile == NULL) {
        printf("ERROR: Unable to open export file.\n");
        exit(1);
    }
    fprintf(pFile, "%f", value);
    fclose(pFile);
}

void exportYellowButton(){
    writingToGPIO(27);
}

bool yellowButtonPressed(){
    return (readButton(yellowButtonPath) == 1);
}


