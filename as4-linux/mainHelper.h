#ifndef mainHelper_H_
#define mainHelper_H_
#include <stdbool.h>

void runCommand(char* command);
void configureI2C();
void configureAllPins();
void sleepForMs(long long delayInMs);
void configurePRUPins();

#endif

