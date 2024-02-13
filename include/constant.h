#ifndef CONSTANT_H
#define CONSTANT_H

#include <stdbool.h> // Include the stdbool.h header
#include <bits/sigaction.h>
#define _POSIX_C_SOURCE 200809L
#include <signal.h>


#define maxMsgLength 400

#define SEM_PATH "/sem_path"
#define SHM_PATH "/shm_path"
#define SHM_SIZE sizeof(struct Position)
#define NUM_OBSTACLES 5
#define NUM_TARGETS 5

#define M 1.0
#define K 1.0
#define T 0.5

#define RADIUS 2.0


#define boardSize 100
#define numberOfProcesses 7


#define counterThresold 6

#define windowWidth 1.00
#define scoreboardWinHeight 0.20
#define windowHeight 0.80

typedef struct {
    double x;
    double y;
    int number;
    bool reached; // Flag to indicate if the target has been reached
} Point;


typedef struct {
    double row;
    double col;
    char symbol;
    short color_pair;
    int active;
} Character;


void handleSignal(int signo, siginfo_t *siginfo, void *context) {
    if (signo == SIGINT) {
        exit(1);
    }
    if (signo == SIGUSR1) {
        pid_t watchdogPID = siginfo->si_pid;
        kill(watchdogPID, SIGUSR2);
    }
}

#endif