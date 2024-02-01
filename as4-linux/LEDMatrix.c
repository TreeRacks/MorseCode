#include "LEDMatrix.h"
#include "pruLogicAnalyzer.h"
#include "Keyboard.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define I2C_DEVICE_ADDRESS 0x70
#define SYS_SETUP_REG 0X21
#define DISPLAY_SETUP_REG 0x81
#define EMPTY 0

static unsigned char logicalFrameArr[numberOfMatrixRows];
static unsigned char physicalFrameArr[numberOfMatrixRows*2];
static char* charRowByRowBits = 0;
static int charCurrentColumns = 0;

// Assume pins already configured for I2C:
// (bbg)$ config-pin P9_18 i2c
// (bbg)$ config-pin P9_17 i2c
int initI2cBus(char* bus, int address)
{
  int i2cFileDesc = open(bus, O_RDWR);
  int result = ioctl(i2cFileDesc, I2C_SLAVE, address);
  if (result < 0) {
    perror("I2C: Unable to set I2C device to slave address.");
    exit(1);
  }
  return i2cFileDesc;
}

void writeI2cReg(int i2cFileDesc, unsigned char regAddr, unsigned char value)
{
  unsigned char buff[2];
  buff[0] = regAddr;
  buff[1] = value;
  int res = write(i2cFileDesc, buff, 2);
  if (res != 2) {
    perror("I2C: Unable to write i2c register.");
    exit(1);
  }
}

void writeMatrixByBytes(unsigned char* physicalFrameValues){
  int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
  int j = 0;
  for(int i = 0; i < 16; i += 2){ // for all 8 rows
      writeI2cReg(i2cFileDesc, i, physicalFrameValues[j]);
      j++;
    }
}

unsigned char readI2cReg(int i2cFileDesc, unsigned char regAddr)
{
  // To read a register, must first write the address
  int res = write(i2cFileDesc, &regAddr, sizeof(regAddr));
  if (res != sizeof(regAddr)) {
    perror("I2C: Unable to write to i2c register.");
    exit(1);
  }
  // Now read the value and return it
  char value = 0;
  res = read(i2cFileDesc, &value, sizeof(value));
  if (res != sizeof(value)) {
    perror("I2C: Unable to read from i2c register");
    exit(1);
  }
  return value;
}

void initializeStartRegisters(){
  int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
  writeI2cReg(i2cFileDesc, SYS_SETUP_REG, 0x00); //write to the system setup register to turn on the matrix.
  writeI2cReg(i2cFileDesc, DISPLAY_SETUP_REG, 0x00); //write to display setup register to turn on LEDs, no flashing.
}

typedef struct {
  char digit; // 0-9 or . or empty space
  char rowBitArr[numberOfMatrixRows]; // represents each row of bits of the char
  char cols; // how wide is this character in terms of columns
} matrixData;

static matrixData matrix [] = { // holds all the bit data for each row for every character that may need to be displayed
  {'_', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 1},
  {'0', {0x20, 0x50, 0x50, 0x50, 0x50, 0x50, 0x20, 0x00}, 4},
  {'X', {0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40}, 1},
  {'2', {0x20, 0x50, 0x40, 0x20, 0x20, 0x10, 0x70, 0x00}, 4},
  {'3', {0x30, 0x40, 0x40, 0x70, 0x40, 0x40, 0x30, 0x00}, 4},
  {'4', {0x40, 0x60, 0x50, 0x50, 0x70, 0x40, 0x40, 0x00}, 4},
  {'5', {0x70, 0x10, 0x10, 0x70, 0x40, 0x50, 0x20, 0x00}, 4},
  {'6', {0x60, 0x10, 0x10, 0x30, 0x50, 0x50, 0x20, 0x00}, 4},
  {'7', {0x70, 0x40, 0x40, 0x40, 0x20, 0x20, 0x20, 0x00}, 4},
  {'8', {0x20, 0x50, 0x50, 0x20, 0x50, 0x50, 0x20, 0x00}, 4},
  {'9', {0x20, 0x50, 0x50, 0x60, 0x40, 0x40, 0x30, 0x00}, 4},
  {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40}, 1},
  {'M', {0x50, 0x70, 0x70, 0x50, 0x50, 0x50, 0x50, 0x00}, 4}
};

static matrixData* searchForHexData(char objectMatrix){ // searches for a char and then returns the address if it is found
  for(int i = 0; matrix[i].digit != EMPTY; i++){
    if (matrix[i].digit == objectMatrix){
      return &matrix[i];
    }
  }
  return NULL;
}

static char shiftLeftOnMatrixBy(int shiftAmountInBytes, char rowValue){ //shiftLeftBy(2,'1')
  if(shiftAmountInBytes >= 0){
    return rowValue >> shiftAmountInBytes;
  }
  else{
    return rowValue << -shiftAmountInBytes;
  }
  return 0;
}

static unsigned char warpFrame(unsigned char logicalFrame){
  unsigned char physicalRows = ((logicalFrame >> 1) | (logicalFrame << 7));
  return physicalRows;
}

static void logicalFrame(){
  for(int i = 0; i < numberOfMatrixRows; i++){
    physicalFrameArr[i] = warpFrame(logicalFrameArr[i]);
  }
  writeMatrixByBytes(physicalFrameArr);
}

static void callMatrixObject(matrixData* currentMatrixData){
  charRowByRowBits = currentMatrixData->rowBitArr; 
  charCurrentColumns = currentMatrixData->cols;
}

static void displayMatrix(char* display){
  //cleaning logicalFrameArr from previous values
  memset(logicalFrameArr,EMPTY, 8);
  char current = ' '; // initialize the current char to be empty
  if(*display != EMPTY){
    current = *display;
  }
  callMatrixObject(searchForHexData(current));
  for(int col = 0; col < numberOfMatrixCols; col+=charCurrentColumns){ // go through all columns
    current = ' '; // initialize the current char to be empty
    if(*display != EMPTY){
      current = *display;
      ++display;      
    }
    callMatrixObject(searchForHexData(current));
    for(int i = 0; i < numberOfMatrixRows; i++){ // for every row
      int shiftAmountInBytes = numberOfMatrixCols - charCurrentColumns - col;
      char rowBits = shiftLeftOnMatrixBy(shiftAmountInBytes,charRowByRowBits[i]); //shift each digit left on the display with a right bitwise shift (>>)
      logicalFrameArr[i] = logicalFrameArr[i] | rowBits;
    }
  }
  logicalFrame();
}

void displayShort(short s){
  char buff[10];
  snprintf(buff, 10, "%hi", s);
  displayMatrix(buff);
}

void displayMode(char* c){
  char buff[10];
  snprintf(buff, 10, "%s", c);
  displayMatrix(buff);
}

void displayInt(int i){
  if(i > 99){
    i = 99;
  }
  else if(i < 0){
    i = 0; 
  }
  else if ((i <= 99) | (i >= 0)){
    char buff[10];
    snprintf(buff, 10, "%d", i);
    displayMatrix(buff); //do some math: eg. from 0-4096 to 0-99
  }
}

void displayDec(double d){ 
  if(d > 9.9){
    d = 9.9;
  }
  else if(d < 0.0){
    d = 0.0; 
  }
  else if ((d <= 9.9) | (d >= 0.0)){
    char buff[10];
    snprintf(buff, 10, "%f", d);
    displayMatrix(buff); //do some math: eg. from 0-4096 to 0-99
  }
}

void displayMorse(char* c){
  char buff[2048];
  snprintf(buff, 2048, "%s", c);
  displayMatrix(buff);
}

