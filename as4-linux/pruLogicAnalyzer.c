#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "Keyboard.h"
#include "LEDMatrix.h"
#include "mainHelper.h"
#include "displayMatrix.h"
#include "sharedDataStruct.h"

// General PRU Memomry Sharing Routine
// ----------------------------------------------------------------
#define PRU_ADDR      0x4A300000   // Start of PRU memory Page 184 am335x TRM
#define PRU_LEN       0x80000      // Length of PRU memory
#define PRU0_DRAM     0x00000      // Offset to DRAM
#define PRU1_DRAM     0x02000
#define PRU_SHAREDMEM 0x10000      // Offset to shared memory
#define PRU_MEM_RESERVED 0x200     // Amount used by stack and heap

// Convert base address to each memory section
#define PRU0_MEM_FROM_BASE(base) ( (base) + PRU0_DRAM + PRU_MEM_RESERVED)
#define PRU1_MEM_FROM_BASE(base) ( (base) + PRU1_DRAM + PRU_MEM_RESERVED)
#define PRUSHARED_MEM_FROM_BASE(base) ( (base) + PRU_SHAREDMEM)


static char *strArr = NULL;
static int buffIndex = 0;
static char *newBuff = NULL;
volatile sharedMemStruct_t *pSharedPru0;
volatile void *pPruBase;
char* morseOutput = NULL;
int i2cFileDesc;
bool* next8 = NULL;


// Return the address of the PRU's base memory
volatile void* getPruMmapAddr(void)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("ERROR: could not open /dev/mem");
        exit(EXIT_FAILURE);
    }

    // Points to start of PRU memory.
    volatile void* pPruBase = mmap(0, PRU_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PRU_ADDR);
    if (pPruBase == MAP_FAILED) {
        perror("ERROR: could not map memory");
        exit(EXIT_FAILURE);
    }
    close(fd);

    return pPruBase;
}
void freePruMmapAddr(volatile void* pPruBase)
{
    if (munmap((void*) pPruBase, PRU_LEN)) {
        perror("PRU munmap failed");
        exit(EXIT_FAILURE);
    }
}

// Morse Code Encodings (from http://en.wikipedia.org/wiki/Morse_code)
//   Encoding created by Brian Fraser. Released under GPL.
//
// Encoding description:
// - msb to be output first, followed by 2nd msb... (left to right)
// - each bit gets one "dot" time.
// - "dashes" are encoded here as being 3 times as long as "dots". Therefore
//   a single dash will be the bits: 111.
// - ignore trailing 0's (once last 1 output, rest of 0's ignored).
// - Space between dashes and dots is one dot time, so is therefore encoded
//   as a 0 bit between two 1 bits.
//
// Example:
//   R = dot   dash   dot       -- Morse code
//     =  1  0 111  0  1        -- 1=LED on, 0=LED off
//     =  1011 101              -- Written together in groups of 4 bits.
//     =  1011 1010 0000 0000   -- Pad with 0's on right to make 16 bits long.
//     =  B    A    0    0      -- Convert to hex digits
//     = 0xBA00                 -- Full hex value (see value in table below)
//
// Between characters, must have 3-dot times (total) of off (0's) (not encoded here)
// Between words, must have 7-dot times (total) of off (0's) (not encoded here).
//


static unsigned short morsecode_codes[] = {
		0xB800,	// A 1011 1
		0xEA80,	// B 1110 1010 1
		0xEBA0,	// C 1110 1011 101
		0xEA00,	// D 1110 101
		0x8000,	// E 1
		0xAE80,	// F 1010 1110 1
		0xEE80,	// G 1110 1110 1
		0xAA00,	// H 1010 101
		0xA000,	// I 101
		0xBBB8,	// J 1011 1011 1011 1
		0xEB80,	// K 1110 1011 1
		0xBA80,	// L 1011 1010 1
		0xEE00,	// M 1110 111
		0xE800,	// N 1110 1
		0xEEE0,	// O 1110 1110 111
		0xBBA0,	// P 1011 1011 101
		0xEEB8,	// Q 1110 1110 1011 1
		0xBA00,	// R 1011 101
		0xA800,	// S 1010 1
		0xE000,	// T 111
		0xAE00,	// U 1010 111
		0xAB80,	// V 1010 1011 1
		0xBB80,	// W 1011 1011 1
		0xEAE0,	// X 1110 1010 111
		0xEBB8,	// Y 1110 1011 1011 1
		0xEEA0	// Z 1110 1110 101
};

// Return the flash code based on the input character.
// Returns 0 if not a-z or A-Z.
unsigned short MorseCode_getFlashCode(char ch)
{
	unsigned short flashCode = 0;

	// Convert to lower case:
	// Lower case has an extra bit (0x20) set. Remove that bit.
	ch &= ~(0x20);

	// If valid letter, look it up in the array:
	if (ch >= 'A' && ch <= 'Z') {
		flashCode = morsecode_codes[ch - 'A'];
	}
	return flashCode;
}

//  function to convert a provided decimal to a string/char array of binary values
//  taken and modified from Stack Overflow user MikeCAT 
//  at https://stackoverflow.com/questions/68069279/converting-int-to-binary-string-in-c

char* toBinaryString(unsigned short n) {
  int num_bits = sizeof(unsigned short) * 8;
  char *string = malloc(num_bits + 1);
  if (!string) {
    return NULL;
  }
  for (int i = num_bits - 1; i >= 0; i--) {
    string[i] = (n & 1) + '0';
    n >>= 1;
  }
  string[num_bits] = '\0';
  return string;
}

int lengthOfStr(char char1){ // pass in a single character from buff[].
    // have a ton of if statements for every character
    // eg. if char1 is r, return 7.
    switch(char1){
        case ' ':
            return 7;
        case 'a':
        case 'A':
            return 5;
        case 'b':
        case 'B':
            return 9;
        case 'c':
        case 'C':
            return 11;
        case 'd':
        case 'D':
            return 7;
        case 'e':
        case 'E':
            return 1;
        case 'f':
        case 'F':
            return 9;
        case 'g':
        case 'G':
            return 9;
        case 'h':
        case 'H':
            return 7;
        case 'i':
        case 'I':
            return 3;
        case 'j':
        case 'J':
            return 13;
        case 'k':
        case 'K':
            return 9;
        case 'l':
        case 'L':
            return 9;
        case 'm':
        case 'M':
            return 7;
        case 'n':
        case 'N':
            return 5;
        case 'o':
        case 'O':
            return 11;
        case 'p':
        case 'P':
            return 11;
        case 'q':
        case 'Q':
            return 13;
        case 'r':
        case 'R':
            return 7;
        case 's':
        case 'S':
            return 5;
        case 't':
        case 'T':
            return 3;
        case 'u':
        case 'U':
            return 7;
        case 'v':
        case 'V':
            return 9;
        case 'w':
        case 'W':
            return 9;
        case 'x':
        case 'X':
            return 11;
        case 'y':
        case 'Y':
            return 13;
        case 'z':
        case 'Z':
            return 11;
        default:            
            return 0;
    }
}

// the next step is to combine/concatenate multiple char arrays together to make one big string
void appendToBinaryString(char* str1){ // we call this function for every character 
    
    int currentIndexInStr1 = 0;
    //while(strcmp(str1, "1000000000000000") != 0){ // get rid of this while loop
        //printf("you are here\n");
	    for(int i = 0; i < lengthOfStr(newBuff[buffIndex]); i++){
	        strArr[pSharedPru0->boolArrLength] = str1[currentIndexInStr1]; 
	        //printf("strArr[%d] is %c\n", pSharedPru0->boolArrLength, strArr[pSharedPru0->boolArrLength]);
	        
	        
	        currentIndexInStr1++;
	        pSharedPru0->boolArrLength++;
	    }	    
    //}
    if(newBuff[buffIndex] == ' '){
        printf("whitespace\n");
        // for(int i = 0; i < 7; i++){
            
        //     printf("currentIndexInStrArr is %d\n", currentIndexInStrArr);
        //     strArr[currentIndexInStrArr] = '0';
        //     printf("strArr[%d] is %c\n", currentIndexInStrArr ,strArr[currentIndexInStrArr]);
        //currentIndexInStrArr += 1; 
            
        // }
    } else if(MorseCode_getFlashCode(newBuff[buffIndex] + 1) != 0){ // this condition may need changing: only run this code if 
                                                                      // the buff[currentIndexInBuff++] is a letter
        for(int i = 0; i < 3; i++){
            //printf("pSharedPru0->boolArrLength is %d\n", pSharedPru0->boolArrLength);
            if(pSharedPru0->boolArrLength != strlen(strArr)){
                strArr[pSharedPru0->boolArrLength] = '0';
                //printf("strArr[%d] is %c\n", pSharedPru0->boolArrLength ,strArr[pSharedPru0->boolArrLength]);
                pSharedPru0->boolArrLength += 1;
            }
            
            
        }
    }
    //printf("strArr final array is %s\n", strArr);
  
}

// after that, we will finally convert the string into a big boolean array
// we probably don't want a ton of empty space at the end of our boolean array! 
// maybe have a variable to keep track of how many valid bools we have?
void convertBinaryToBool(char* binaryStr){
    
    for (int j = 0; j < pSharedPru0->boolArrLength; j++){
        if(strArr[j] == '1'){
		    pSharedPru0->boolArr[j] = true;
	    }
	    else{
		    pSharedPru0->boolArr[j] = false;
	    }
    }
    // for (int j = 0; j < sharedMem->boolArrLength; j++){
    //     printf("%d", sharedMem->boolArr[j] );
    // }
}

/**
 * Remove trailing white space characters from string
 */
void trimWhiteSpace(char * str)
{
    int i = strlen(str) - 1;

    /* Set default index */
    //index = buffIndex;

    /* Find last index of non-white space character */
    while(i > 0)
    {
        if(str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n')
        {
            i--;
        }
        else break;
    }

    /* Mark next character to last non-white space character as NULL */
    str[i + 1] = '\0';
}

void generateBoolArr(){
    // volatile void *pPruBase = getPruMmapAddr();

    // sharedMem = PRU0_MEM_FROM_BASE(pPruBase);
    // printf("pPruBase address in morseCode is 0x%x\n", (uint32_t)pPruBase);
    // //sharedMem = malloc(2056);
    pPruBase = getPruMmapAddr();
    pSharedPru0 = PRU0_MEM_FROM_BASE(pPruBase);
	pSharedPru0->boolArrLength = 0;
    // printf("sharedMem address 0x%x\n", (uint32_t)sharedMem);
    
    strArr = malloc(2056); 
    
    memset(strArr, 0, 2056);
    
    
    newBuff = getBuff();
    //printf("newBuff returned %s\n", newBuff);
    
    while(buffIndex < getNumCh()){

        appendToBinaryString(toBinaryString(MorseCode_getFlashCode(newBuff[buffIndex])));
        printf("\n");
        
        buffIndex++;
    }
    convertBinaryToBool(strArr);

    buffIndex = 0;
}



int logicAnalyzer(void)
{
    int i2cFileDesc = initI2cBus(I2CDRV_LINUX_BUS1, I2C_DEVICE_ADDRESS);
    morseOutput = malloc(2056);
    next8 = malloc(8);
    memset(morseOutput, 0, 2056);
    
    pSharedPru0->currentIndex = 0;


    // Get access to shared memory for my uses
    

    //printf("pPruBase is at 0x%x\n", (uint32_t)pPruBase);
    // Print out the mem contents:
    //printf("From the PRU, memory hold:\n");
    //printf("Address 0x%x\n", (uint32_t)pSharedPru0);

    // while (true) {
        //printf("Waiting for data from PRU...\n");
        while (!pSharedPru0->isFilledWithSamples) {
            //Wait
            //printf("you are here\n");
            sleep(1);
        }
        // Data's available, so print it.
        for (int i = 0; i < pSharedPru0->boolArrLength; i++) {
            
            if(pSharedPru0->boolArr[i] == true){
                
                morseOutput[i] = 'X';
            }
            else{
                morseOutput[i] = '_';
            }

            
        }
        for (int i = 0; i < pSharedPru0->boolArrLength; i++) {
            printf("%c", morseOutput[i]);
        }
        printf("\n");
        for(int i = 0; i < pSharedPru0->boolArrLength; i++){
            // wipe the matrix
            memset(next8, 0, 8);


            printf("%c", morseOutput[pSharedPru0->currentIndex]);
             //set the bit values of the next 8 dot times on the LED Matrix
             for(int i = 1; i <= 7; i++){
                if(pSharedPru0->currentIndex+i < pSharedPru0->boolArrLength){
                    next8[i] = pSharedPru0->boolArr[pSharedPru0->currentIndex+i];
                }
                else{
                    next8[i] = false;
                }
             }
                // next8[0] = pSharedPru0->boolArr[pSharedPru0->currentIndex];
                // next8[1] = pSharedPru0->boolArr[pSharedPru0->currentIndex+1];
                // next8[2] = pSharedPru0->boolArr[pSharedPru0->currentIndex+2];
                // next8[3] = pSharedPru0->boolArr[pSharedPru0->currentIndex+3];
                // next8[4] = pSharedPru0->boolArr[pSharedPru0->currentIndex+4];
                // next8[5] = pSharedPru0->boolArr[pSharedPru0->currentIndex+5];
                // next8[6] = pSharedPru0->boolArr[pSharedPru0->currentIndex+6];
                // next8[7] = pSharedPru0->boolArr[pSharedPru0->currentIndex+7];
            


            // bool* next8 = getNext8();
            int rowCode = boolArrayToInt(next8, 8); 
            
            writeI2cReg(i2cFileDesc, 0x00, rowCode);
            writeI2cReg(i2cFileDesc, 0x02, rowCode);
            writeI2cReg(i2cFileDesc, 0x04, rowCode);
            writeI2cReg(i2cFileDesc, 0x06, rowCode);
            
            

            if(pSharedPru0->boolArr[i] == true){
                pSharedPru0->isLEDOn = true; 
            }else{
                pSharedPru0->isLEDOn = false;
            }
            
            fflush(stdout);

            
            if(pSharedPru0->isButtonPressed){
                sleepForMs(1000);
            }
            else{ 
                sleepForMs(300);
            }
            
            pSharedPru0->currentIndex++;
        }
        // wipe the matrix
            writeI2cReg(i2cFileDesc, 0x00, 0);
            writeI2cReg(i2cFileDesc, 0x02, 0);
            writeI2cReg(i2cFileDesc, 0x04, 0);
            writeI2cReg(i2cFileDesc, 0x06, 0);
            writeI2cReg(i2cFileDesc, 0x08, 0);
            writeI2cReg(i2cFileDesc, 0x0A, 0);
            writeI2cReg(i2cFileDesc, 0x0C, 0);
            writeI2cReg(i2cFileDesc, 0x0E, 0);   
        
        printf("\n");
        

        // Signal we are done with the data
        pSharedPru0->isFilledWithSamples = false;
    // }

    // Cleanup
    freePruMmapAddr(pPruBase);
    free(next8);
    free(morseOutput);
    return 0;
}



void* getNext8(){   
    
    //for(int i = 0; i < pSharedPru0->boolArrLength; i++){
    
    //}
    return next8;
}

