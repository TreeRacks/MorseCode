#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include <stdbool.h>

void getInput();
unsigned int getNumCh();
char* getBuff();
void checkForEmptyInput();
int boolArrayToInt(bool* arr, int count);

#endif
