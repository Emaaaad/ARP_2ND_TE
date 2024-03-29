#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include "../include/constant.h"

int main(int argc, char *argv[]) {
    // Signal handling for watchdog
    struct sigaction sig_act;
    sig_act.sa_sigaction = handleSignal;
    sig_act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sig_act, NULL);
    sigaction(SIGUSR1, &sig_act, NULL);

    // Pipes
    pid_t serverPID;
    serverPID = getpid();
    int pipeWatchdogServer[2];
    sscanf(argv[1], "%d %d", &pipeWatchdogServer[0], &pipeWatchdogServer[1]);

    close(pipeWatchdogServer[0]);
    if (write(pipeWatchdogServer[1], &serverPID, sizeof(serverPID)) == -1) {
        perror("write pipeWatchdogServer");
        exit(EXIT_FAILURE);
    }
    printf("%d\n", serverPID);
    close(pipeWatchdogServer[1]); // Closing unnecessary pipes

    // LOG FILE SETUP
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/ServerLog.txt");
    logFile = fopen(logFilePath, "w"); 

    if (logFile == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    // SHARED MEMORY AND SEMAPHORE SETUP
    double position[6];
    int sharedSegSize = sizeof(position);

    sem_t *semaphoreID = sem_open(SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 1);
    if (semaphoreID == SEM_FAILED) {
        perror("sem_open");
        fclose(logFile);
        exit(EXIT_FAILURE);
    }
    sem_init(semaphoreID, 1, 0);

    int shmFD = shm_open(SHM_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
    if (shmFD < 0) {
        perror("shm_open");
        fclose(logFile);
        sem_close(semaphoreID);
        sem_unlink(SEM_PATH);
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shmFD, sharedSegSize) == -1) {
        perror("ftruncate");
        fclose(logFile);
        sem_close(semaphoreID);
        sem_unlink(SEM_PATH);
        shm_unlink(SHM_PATH);
        exit(EXIT_FAILURE);
    }
    void *shmPointer = mmap(NULL, sharedSegSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmFD, 0);
    if (shmPointer == MAP_FAILED) {
        perror("mmap");
        fclose(logFile);
        sem_close(semaphoreID);
        sem_unlink(SEM_PATH);
        shm_unlink(SHM_PATH);
        exit(EXIT_FAILURE);
    }
    sem_post(semaphoreID);

    while (1) {
    // Get current time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    // COPY POSITION OF THE DRONE FROM SHARED MEMORY
    sem_wait(semaphoreID);
    memcpy(position, shmPointer, sharedSegSize);
    sem_post(semaphoreID);

    // Write to the log file with time
    fprintf(logFile, "[%02d:%02d:%02d] Drone Position: %.2f, %.2f\n",
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
            position[4], position[5]);

    // Read obstacles reached or not
    int obstacleHit;
    sem_wait(semaphoreID);
    memcpy(&obstacleHit, shmPointer, sizeof(obstacleHit));
    sem_post(semaphoreID);

    // Check if an obstacle was hit
    if (obstacleHit == -1) {
        fprintf(logFile, "[%02d:%02d:%02d] Obstacle hit!\n",
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    }

    // Read target with value from shared memory
    int targetHitValue;
    sem_wait(semaphoreID);
    memcpy(&targetHitValue, shmPointer, sizeof(targetHitValue));
    sem_post(semaphoreID);

    // Check if a target with value was hit
    if (targetHitValue >= 1 && targetHitValue <= 10) {
        fprintf(logFile, "[%02d:%02d:%02d] Target hit with value: %d\n",
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, targetHitValue);
    }

    fflush(logFile); // Ensure the data is written to the file immediately
    sleep(1);
}

    // CLEANUP
    shm_unlink(SHM_PATH);
    sem_close(semaphoreID);
    sem_unlink(SEM_PATH);
    munmap(shmPointer, sharedSegSize);

    // Close the log file
    fclose(logFile);

    return 0;
}
