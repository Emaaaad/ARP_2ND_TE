#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <signal.h>
#include "../include/constant.h"
#include <errno.h>

int main(int argc, char *argv[]) {
    // Pipes
    int pipeKeyboardDrone[2], pipeWindowKeyboard[2], pipeWatchdogKeyboard[2];
    pid_t keyboardPID = getpid();
    sscanf(argv[1], "%d %d|%d %d|%d %d", &pipeWindowKeyboard[0], &pipeWindowKeyboard[1],
           &pipeKeyboardDrone[0], &pipeKeyboardDrone[1],
           &pipeWatchdogKeyboard[0], &pipeWatchdogKeyboard[1]);
    close(pipeWindowKeyboard[1]);
    close(pipeKeyboardDrone[0]);
    close(pipeWatchdogKeyboard[0]);
    write(pipeWatchdogKeyboard[1], &keyboardPID, sizeof(keyboardPID));
    close(pipeWatchdogKeyboard[1]);

    // Signal handeling for watchdog
    struct sigaction signal_action;
    signal_action.sa_sigaction = handleSignal;
    signal_action.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &signal_action, NULL);
    sigaction(SIGUSR1, &signal_action, NULL);

    // Open the log file
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/keyboardLog.txt");
    logFile = fopen(logFilePath, "w"); // "a" mode appends to the file

    if (logFile == NULL) {
        perror("Error opening log file\n");
        exit(EXIT_FAILURE);
    }

    int key;
    int forceDirection[2] = {0, 0};

    while (1) {
        ssize_t keyPress;

        do {
            keyPress = read(pipeWindowKeyboard[0], &key, sizeof(key));
        } while (keyPress == -1 && errno == EINTR);

        if (keyPress < 0) {
            perror("reading error\n");
            fprintf(logFile, "Error reading from pipe\n");
            fclose(logFile);
            close(pipeKeyboardDrone[1]);
            close(pipeWindowKeyboard[0]);
            exit(EXIT_FAILURE);
        }

        // Updateing force-direction based on user input
        switch ((char) key) {
            case 'q': // Enter q to exit
                close(pipeWindowKeyboard[0]);
                close(pipeKeyboardDrone[1]);
                fclose(logFile);
                exit(EXIT_SUCCESS);

            case 's':
                forceDirection[0]--; break;
            case 'r':
                forceDirection[0]++; forceDirection[1]--; break;
            case 'e':
                forceDirection[1]--; break;
            case 'x':
                forceDirection[0]--; forceDirection[1]++; break;
            case 'd':
                forceDirection[0] = 0; forceDirection[1] = 0; break;  // Stop (no movement)
            case 'c':
                forceDirection[1]++; break;
            case 'w':
                forceDirection[0]--; forceDirection[1]--; break;
            case 'f':
                forceDirection[0]++; break;
            case 'v':
                forceDirection[0]++; forceDirection[1]++; break;
        }

        // Sending the updated force-direction to drone.c
        int updateForceDirection = write(pipeKeyboardDrone[1], forceDirection, sizeof(forceDirection));
        if (updateForceDirection < 0) {
            fclose(logFile);
            close(pipeWindowKeyboard[0]); 
            close(pipeKeyboardDrone[1]); //closing unnecessary pipes
            perror("writing error\n");
            exit(EXIT_FAILURE);
        }

        // Writing to the log file
        fprintf(logFile, "Key Press: %c, Force Direction: [%d, %d]\n", (char) key, forceDirection[0], forceDirection[1]);
        fflush(logFile);
    }

    // Closing the log file
    fclose(logFile);

    //Cleaning up
    close(pipeWindowKeyboard[0]);
    close(pipeKeyboardDrone[1]);

    return 0;
}
