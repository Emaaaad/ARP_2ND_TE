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
#include <semaphore.h>
#include <math.h> 
#include "../include/constant.h"

// Function to get the current time in seconds
double getCurrentTimeInSeconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// Update the targets' location with a generation timer
void updateObstacles(Point *obstacles_location, double *lastGenerationTime) {
    double currentTime = getCurrentTimeInSeconds();
    int GENERATION_INTERVAL = 10;
    // Generate new targets if enough time has passed
    if ((currentTime - *lastGenerationTime) >= GENERATION_INTERVAL) {
        for (int i = 0; i < NUM_OBSTACLES; ++i) {
            obstacles_location[i].x = rand() % (boardSize-5);
            obstacles_location[i].y = rand() % (boardSize-5);
        }

        // Update the last generation time
        *lastGenerationTime = currentTime;
    }
}

// Logging function for obstacles
void logObstacleData(FILE *logFile, Point *obstacles) {
    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);
    info = localtime(&rawtime);

    for (int i = 0; i < NUM_OBSTACLES; ++i) {
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
        fprintf(logFile, "[%s] Obstacle %d position: (%.2f, %.2f)\n", buffer, i + 1, obstacles[i].x, obstacles[i].y);
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
    int pipeObstaclesWindow[2], pipeWatchdogObstacles[2];
    pid_t obstaclesPID = getpid();
    sscanf(argv[1], "%d %d|%d %d", &pipeObstaclesWindow[0], &pipeObstaclesWindow[1], &pipeWatchdogObstacles[0], &pipeWatchdogObstacles[1]);
    close(pipeObstaclesWindow[0]);
    close(pipeWatchdogObstacles[0]);  // Closing unnecessary pipes
    write(pipeWatchdogObstacles[1], &obstaclesPID, sizeof(obstaclesPID));
    close(pipeWatchdogObstacles[1]);

    // Open the log file for obstacles
    FILE *logObstacleFile;
    char logObstacleFilePath[100];
    snprintf(logObstacleFilePath, sizeof(logObstacleFilePath), "log/obstaclesLog.txt");
    logObstacleFile = fopen(logObstacleFilePath, "w");

    if (logObstacleFile == NULL) {
        perror("Error opening log file for obstacles");
        exit(EXIT_FAILURE);
    }

    double lastGenerationTime = getCurrentTimeInSeconds();



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
        Point obstacles[NUM_OBSTACLES];
        updateObstacles(obstacles, &lastGenerationTime);

        // Logging obstacles positions to the file
        logObstacleData(logObstacleFile, obstacles);

        // Sending targets to window.c via pipe
        write(pipeObstaclesWindow[1], obstacles, sizeof(obstacles));

       // Reading from shared memory
        sem_wait(semID);
        memcpy(position, shmPointer, sharedSegSize);
        sem_post(semID);

        // Check if the drone reaches any of the obstacles
        bool droneReachedObstacle = false;
        for (int i = 0; i < NUM_OBSTACLES; ++i) {
        double distance = sqrt(pow(position[4] - obstacles[i].x, 2) + pow(position[5] - obstacles[i].y, 2));
        if (distance < RADIUS) {
        droneReachedObstacle = true;
        break; 
       }
      }
            
         // Write to shared memory only if an obstacle was reached
        if (droneReachedObstacle) {
            int obstacleHit = -1; // Indicates an obstacle was hit
            sem_wait(semID);
            memcpy(shmPointer, &obstacleHit, sizeof(obstacleHit));
            sem_post(semID);
        }


        sleep(1);
    }

    // Close pipes
    close(pipeObstaclesWindow[1]);

    return 0;
}
