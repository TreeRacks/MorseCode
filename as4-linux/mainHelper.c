
#include "mainHelper.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

void configurePRUPins(){
    runCommand("config-pin p9_27 pruout");
    runCommand("config-pin p9_28 pruin");
}

void configureI2C(){
    runCommand("config-pin P9_18 i2c");
    runCommand("config-pin P9_17 i2c");
}

void runCommand(char* command)
{
    // Execute the shell command (output into pipe)
    FILE *pipe = popen(command, "r");
    // Ignore output of the command; but consume it
    // so we don't get an error when closing the pipe.
    char buffer[1024];
    while (!feof(pipe) && !ferror(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) == NULL)
            break;
        // printf("--> %s", buffer); // Uncomment for debugging
        }
    // Get the exit code from the pipe; non-zero is an error:
    int exitCode = WEXITSTATUS(pclose(pipe));
    if (exitCode != 0) {
        perror("Unable to execute command:");
        printf(" command: %s\n", command);
        printf(" exit code: %d\n", exitCode);
    }
}

void configureAllPins(){
    runCommand("config-pin p8.15 gpio");
    runCommand("config-pin -q p8.15");
    runCommand("config-pin p8.16 gpio");
    runCommand("config-pin -q p8.16");
    runCommand("config-pin p8.17 gpio");
    runCommand("config-pin -q p8.17");
    runCommand("config-pin p8.18 gpio");
    runCommand("config-pin -q p8.18");
}

void sleepForMs(long long delayInMs){
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *) NULL);
}
