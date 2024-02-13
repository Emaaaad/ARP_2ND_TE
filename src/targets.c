
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/mman.h>
#include <math.h>
#include <semaphore.h>
#include "../include/constant.h"



// Update the target's location
void updateTargets(Point *targets_location) {
    static bool initialized = false;

    if (!initialized) {
        // Use the process ID as the seed for the random number generator
        srand((unsigned int)getpid());

        // Generate new targets only once
        for (int i = 0; i < NUM_TARGETS; ++i) {
            targets_location[i].x = rand() % (boardSize-10);
            targets_location[i].y = rand() % (boardSize-10);
            targets_location[i].number = rand() % 10 + 1;
        }
        initialized = true;
    }
}

// Logging function
void logData(FILE *logFile, Point *targets) {
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);
    info = localtime(&rawtime);

    for (int i = 0; i < NUM_TARGETS; ++i) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
        fprintf(logFile, "[%s] Target %d position: (%.2f, %.2f) | Generated Number: %d\n", buffer, i + 1, targets[i].x, targets[i].y, targets[i].number);
    }

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
    int pipeTargetsWindow[2], pipeWatchdogTargets[2];
    pid_t obstaclePID = getpid();
    sscanf(argv[1], "%d %d|%d %d", &pipeTargetsWindow[0], &pipeTargetsWindow[1], &pipeWatchdogTargets[0], &pipeWatchdogTargets[1]);
    close(pipeTargetsWindow[0]);
    close(pipeWatchdogTargets[0]);  // Closing unnecessary pipes
    write(pipeWatchdogTargets[1], &obstaclePID, sizeof(obstaclePID));
    close(pipeWatchdogTargets[1]);

    // Open the log file
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/targetsLog.txt");
    logFile = fopen(logFilePath, "w");

    if (logFile == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    // Shared memory setup
    double position[6] = {boardSize / 2, boardSize / 2, boardSize / 2, boardSize / 2, boardSize / 2, boardSize / 2};
    int sharedSegSize = sizeof(position);

    sem_t *semID = sem_open(SEM_PATH, 0);
    if (semID == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    int shmfd = shm_open(SHM_PATH, O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    void *shmPointer = mmap(NULL, sharedSegSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shmPointer == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    while (1) {
        Point targets[NUM_TARGETS];
        updateTargets(targets);

        // Logging targets positions and generated numbers to the file
        logData(logFile, targets);

        // Sending targets to window.c via pipe
        write(pipeTargetsWindow[1], targets, sizeof(targets));

        // Reading from shared memory
        sem_wait(semID);
        memcpy(position, shmPointer, sharedSegSize);
        sem_post(semID);

        // Check if the drone reaches any of the targets
        bool droneReachedTarget = false;
        int targetReachedIndex = -1;
        for (int i = 0; i < NUM_TARGETS; ++i) {
            double distance = sqrt(pow(position[4] - targets[i].x, 2) + pow(position[5] - targets[i].y, 2));
            if (distance < RADIUS) {
                droneReachedTarget = true;
                targetReachedIndex = i;
                break;
            }
        }

        // If the drone reached any target, update targets
        if (droneReachedTarget) {
            // Store the value of the removed target
            int removedTarget,removedTargetValue;
            removedTarget = targets[targetReachedIndex].number;
            
            if (1 <= removedTarget && removedTarget<= 10 )
             {
                removedTargetValue = removedTarget ;
             }

            // Update targets (remove the target reached by the drone)
            for (int i = targetReachedIndex; i < NUM_TARGETS - 1; ++i) {
                targets[i] = targets[i + 1];
            }


            // Generate a new target for the last position
            targets[NUM_TARGETS - 1].x = rand() % (boardSize-10);
            targets[NUM_TARGETS - 1].y = rand() % (boardSize-10);
            targets[NUM_TARGETS - 1].number = rand() % 10 + 1;

            // Logging updated targets positions and generated numbers to the file
            logData(logFile, targets);

            // Sending updated targets to window.c via pipe
            write(pipeTargetsWindow[1], targets, sizeof(targets));

            // Write the removed target value to shared memory
            sem_wait(semID);
            memcpy(shmPointer, &removedTargetValue, sizeof(removedTargetValue));
            sem_post(semID);
        }

        sleep(1);
    }

    // Close pipes
    close(pipeTargetsWindow[1]);

    return 0;
}
