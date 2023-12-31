#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>  
#include "../include/constant.h"

int serverCounter, windowCounter, keyboardCounter, droneCounter;
pid_t serverPID, windowPID, keyboardPID, dronePID, watchdogPID, pidKB;

// Function to terminate all processes and log the event
void TerminateAll() {
    kill(serverPID, SIGINT);
    kill(windowPID, SIGINT);
    kill(dronePID, SIGINT);
    kill(pidKB, SIGINT);
    kill(keyboardPID, SIGINT);

    // Logging the termination event
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/watchdogLog.txt");
    logFile = fopen(logFilePath, "a");

    if (logFile == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
    fprintf(logFile, "[%s] Watchdog terminated all processes\n", buffer);
    fflush(logFile);
    fclose(logFile);

    printf("Sent signals to all processes\n");
    exit(1);
}

// Signal handler function
void handler_Signal(int signo, siginfo_t *siginfo, void *context) {
    // Logging the received signal
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/watchdogLog.txt");
    logFile = fopen(logFilePath, "a");

    if (logFile == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    time_t rawtime;
    struct tm *info;
    char buffer[80];

    time(&rawtime);
    info = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
    fprintf(logFile, "[%s] Received signal %d from process %d\n", buffer, signo, siginfo->si_pid);
    fflush(logFile);
    fclose(logFile);

    printf("Received signal from %d\n", signo);

    if (signo == SIGINT) {
        TerminateAll();
    }
    if (signo == SIGUSR2) {
        if (siginfo->si_pid == serverPID) {
            printf("Sent signal from server\n");
            serverCounter = 0;
        }
        if (siginfo->si_pid == windowPID) {
            printf("Sent signal from window\n");
            windowCounter = 0;
        }
        if (siginfo->si_pid == keyboardPID) {
            printf("Sent signal from keyboardManager\n");
            keyboardCounter = 0;
        }
        if (siginfo->si_pid == dronePID) {
            printf("Sent signal from droneDynamics\n");
            droneCounter = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    // Pipes
    int pipeWatchdogServer[2], pipeWatchdogWindow[2], pipeWatchdogDrone[2], pipeWatchdogKeyboard[2];
    serverCounter = windowCounter = droneCounter = keyboardCounter = 0;

    // Get PID from all other processes
    sscanf(argv[1], "%d %d|%d %d|%d %d|%d %d|%d", &pipeWatchdogServer[0], &pipeWatchdogServer[1], &pipeWatchdogWindow[0], &pipeWatchdogWindow[1], &pipeWatchdogKeyboard[0], &pipeWatchdogKeyboard[1], &pipeWatchdogDrone[0], &pipeWatchdogDrone[1], &pidKB);
    close(pipeWatchdogServer[1]);
    close(pipeWatchdogDrone[1]);
    close(pipeWatchdogKeyboard[1]);
    close(pipeWatchdogWindow[1]);

    watchdogPID = getpid();
    read(pipeWatchdogDrone[0], &dronePID, sizeof(dronePID));
    read(pipeWatchdogKeyboard[0], &keyboardPID, sizeof(keyboardPID));
    read(pipeWatchdogWindow[0], &windowPID, sizeof(windowPID));
    read(pipeWatchdogServer[0], &serverPID, sizeof(serverPID));

    printf("window: %d\n", windowPID);
    printf("server: %d\n", serverPID);
    printf("droneDynamics: %d\n", dronePID);
    printf("keyboardManager: %d\n", keyboardPID);
    printf("watchdog: %d\n", watchdogPID);

    close(pipeWatchdogServer[0]);
    close(pipeWatchdogDrone[0]);
    close(pipeWatchdogKeyboard[0]);
    close(pipeWatchdogWindow[0]);  // Closing unnecessary pipes

    // Signal handling
    struct sigaction sig_act;
    sig_act.sa_sigaction = handler_Signal;
    sig_act.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sig_act, NULL);
    sigaction(SIGUSR2, &sig_act, NULL);

    // Open the log file
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/watchdogLog.txt");
    logFile = fopen(logFilePath, "a");

    if (logFile == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    while (1) {
        serverCounter++;
        windowCounter++;
        droneCounter++;
        keyboardCounter++;

        // Sending signals to other processes
        if (kill(serverPID, SIGUSR1) == -1) {
            perror("kill server");
            exit(EXIT_FAILURE);
        }
        usleep(50000);

        if (kill(windowPID, SIGUSR1) == -1) {
            perror("kill window");
            exit(EXIT_FAILURE);
        }
        usleep(50000);

        if (kill(keyboardPID, SIGUSR1) == -1) {
            perror("kill keyboardManager");
            exit(EXIT_FAILURE);
        }
        usleep(50000);

        usleep(50000);
        usleep(50000);

        if (kill(dronePID, SIGUSR1) == -1) {
            perror("kill droneDynamics");
            exit(EXIT_FAILURE);
        }
        usleep(50000);

        // Logging the sent signals
        time_t rawtime;
        struct tm *info;
        char buffer[80];

        time(&rawtime);
        info = localtime(&rawtime);

        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
        fprintf(logFile, "[%s] Signals sent to processes: Server(%d), Window(%d), KeyboardManager(%d), DroneDynamics(%d)\n",
                buffer, serverCounter, windowCounter, keyboardCounter, droneCounter);
        fflush(logFile);

        if (serverCounter > counterThresold || windowCounter > counterThresold || droneCounter > counterThresold || keyboardCounter > counterThresold) {
            watchdogPID = getpid();
            TerminateAll();

            // Logging the termination event
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
            fprintf(logFile, "[%s] Watchdog terminated due to process counters exceeding the threshold\n", buffer);
            fflush(logFile);
            exit(1);
        }
    }

    // Closing the log file
    fclose(logFile);

    return 0;
}
