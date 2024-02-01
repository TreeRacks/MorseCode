all:
	arm-linux-gnueabihf-gcc -Wall -pthread -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror Keyboard.c LEDMatrix.c main.c mainHelper.c pruLogicAnalyzer.c  -o as4-morsecode
	cp as4-morsecode $(HOME)/cmpt433/public/myApps

