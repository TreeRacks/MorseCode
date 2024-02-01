#include "Keyboard.h"
#include "MorseCode.h"
#include "pruLogicAnalyzer.h"
#include "mainHelper.h"
#include "sharedDataStruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>


//volatile sharedMemStruct_t *sharedMem;
//static int currentIndexInStrArr = 0;
static char *strArr = NULL;
static int buffIndex = 0;
static char *newBuff = NULL; 
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
	        strArr[sharedMem->boolArrLength] = str1[currentIndexInStr1]; 
	        printf("strArr[%d] is %c\n", sharedMem->boolArrLength, strArr[sharedMem->boolArrLength]);
	        
	        
	        currentIndexInStr1++;
	        sharedMem->boolArrLength++;
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
            printf("sharedMem->boolArrLength is %d\n", sharedMem->boolArrLength);
            strArr[sharedMem->boolArrLength] = '0';
            printf("strArr[%d] is %c\n", sharedMem->boolArrLength ,strArr[sharedMem->boolArrLength]);
            sharedMem->boolArrLength += 1;
            
        }
    }
    printf("strArr final array is %s\n", strArr);
  
}

// after that, we will finally convert the string into a big boolean array
// we probably don't want a ton of empty space at the end of our boolean array! 
// maybe have a variable to keep track of how many valid bools we have?
void convertBinaryToBool(char* binaryStr){
    
    for (int j = 0; j < sharedMem->boolArrLength; j++){
        if(strArr[j] == '1'){
		    sharedMem->boolArr[j] = true;
	    }
	    else{
		    sharedMem->boolArr[j] = false;
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
    volatile void *pPruBase = getPruMmapAddr();

    sharedMem = PRU0_MEM_FROM_BASE(pPruBase);
    printf("pPruBase address in morseCode is 0x%x\n", (uint32_t)pPruBase);
    //sharedMem = malloc(2056);
	sharedMem->boolArrLength = 0;
    printf("sharedMem address 0x%x\n", (uint32_t)sharedMem);
    

    strArr = malloc(2048); 
    memset(strArr, 0, 2048);
    
    
    newBuff = getBuff();
    printf("newBuff returned %s\n", newBuff);
    
    while(buffIndex < getNumCh()){

        appendToBinaryString(toBinaryString(MorseCode_getFlashCode(newBuff[buffIndex])));
        printf("\n");
        
        buffIndex++;
    }
    convertBinaryToBool(strArr);
    freePruMmapAddr(pPruBase);

    buffIndex = 0;
}



