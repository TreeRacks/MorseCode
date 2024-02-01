#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <stdbool.h>

int readButton(char *button);
void writingToGPIO(float value);
void exportYellowButton();
bool yellowButtonPressed();
void start_startButton();
void stop_stopButton();

#endif
