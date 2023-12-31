#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include "../include/constant.h"

// Function for computing new position using Euler's Method
double computePosition(double force, double x1, double x2) {
    double newPosition = x1 + (force * T) - ((M * (x1 - x2)) / (M + K * T));
    return newPosition;
}

// Function to update the drone's position based on force direction
double updatePosition(double *position, int *forceDirection) {
    double newPositionX = computePosition(forceDirection[0], position[4], position[2]);
    double newPositionY = computePosition(forceDirection[1], position[5], position[3]);

    // Boundary conditions
    newPositionX = fmax(0, fmin(newPositionX, boardSize));
    newPositionY = fmax(0, fmin(newPositionY, boardSize));

    // Updating position array
    memmove(position, position + 2, 4 * sizeof(double));
    position[4] = newPositionX;
    position[5] = newPositionY;

    return *position;
}

// Logging function
void logData(FILE *logFile, double *position) {
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
    fprintf(logFile, "[%s] Previous position: (%.2f, %.2f) | Updated Position: (%.2f, %.2f)\n", buffer, position[2], position[3], position[4], position[5]);
    fflush(logFile);
}

int main(int argc, char *argv[]) {
    // Signal handling for watchdog
    struct sigaction signal_action;
    signal_action.sa_sigaction = handleSignal;
    signal_action.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &signal_action, NULL);
    sigaction(SIGUSR1, &signal_action, NULL);

    // Pipes
    int pipeKeyboardDrone[2], pipeWatchdogDrone[2];
    pid_t dronePID = getpid();
    sscanf(argv[1], "%d %d|%d %d", &pipeKeyboardDrone[0], &pipeKeyboardDrone[1], &pipeWatchdogDrone[0], &pipeWatchdogDrone[1]);
    close(pipeKeyboardDrone[1]);
    close(pipeWatchdogDrone[0]);  // Closing unnecessary pipes
    write(pipeWatchdogDrone[1], &dronePID, sizeof(dronePID));
    close(pipeWatchdogDrone[1]);

    // Make the read non-blocking so the drone can move without user input
    int flags = fcntl(pipeKeyboardDrone[0], F_GETFL);
    fcntl(pipeKeyboardDrone[0], F_SETFL, flags | O_NONBLOCK);

    int forceDirection[2]; // force direction of x and y coordinates
    double position[6];
    int initial = 0;

    // Shared memory setup
    int sharedSegSize = (1 * sizeof(position));
    sem_t *semaphoreID = sem_open(SEM_PATH, 0);
    if (semaphoreID == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    int shmFD = shm_open(SHM_PATH, O_RDWR, S_IRWXU | S_IRWXG);
    if (shmFD < 0) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void *shmPointer = mmap(NULL, sharedSegSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFD, 0);
    if (shmPointer == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Open the log file
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/droneDynamicsLog.txt");
    logFile = fopen(logFilePath, "w");

    if (logFile == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Receive command force from keyboard_manager
        ssize_t readCommand = read(pipeKeyboardDrone[0], forceDirection, sizeof(forceDirection));

        // Wait until the user's initial input
        if (initial == 0) {
            sem_wait(semaphoreID);
            memcpy(position, shmPointer, sharedSegSize); // Get the initial position of the drone from window.c
            sem_post(semaphoreID);

            if (readCommand < 0) {
                if (errno != EAGAIN) {
                    perror("reading error");
                    exit(EXIT_FAILURE);
                }
            } else if (readCommand > 0) { // User's initial input
                updatePosition(position, forceDirection);
                initial++;
            }
        } else { // For next inputs
            updatePosition(position, forceDirection);
        }

        // Sending updated drone position to window via shared memory
        sem_wait(semaphoreID);
        memcpy(shmPointer, position, sharedSegSize);
        sem_post(semaphoreID);

        // Write to the log file
        logData(logFile, position);
        usleep(300000);
    }

    // Cleaning up
    close(pipeKeyboardDrone[0]);
    shm_unlink(SHM_PATH);
    sem_close(semaphoreID);
    sem_unlink(SEM_PATH);

    // Closing the log file
    fclose(logFile);

    return 0;
}
