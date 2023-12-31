#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h> 
#include "../include/constant.h"

// Function to execute a program with specified arguments and handle errors
void summon(char **programArgs, int fd1, int fd2, int displayKonsole) {
    if (displayKonsole) {
        execvp(programArgs[0], programArgs);
    } else {
        dup2(fd1, STDOUT_FILENO);
        dup2(fd2, STDERR_FILENO);
        execvp(programArgs[0], programArgs);
    }

    perror("Execution failed");
    exit(EXIT_FAILURE);
}

int main() {
    // Pipe descriptors for communication between processes
    int pipeWindowKeyboard[2];
    int pipeKeyboardDrone[2];

    // Check if pipes are created successfully
    if (pipe(pipeWindowKeyboard) == -1 || pipe(pipeKeyboardDrone) == -1) {
        perror("pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Additional pipes for communication with the watchdog process
    int pipeWatchdogServer[2];
    int pipeWatchdogWindow[2];
    int pipeWatchdogKeyboard[2];
    int pipeWatchdogDrone[2];

    // Check if additional pipes are created successfully
    if (pipe(pipeWatchdogDrone) == -1 || pipe(pipeWatchdogKeyboard) == -1 || 
        pipe(pipeWatchdogServer) == -1 || pipe(pipeWatchdogWindow) == -1) {
        perror("pipe creation failed");
        exit(EXIT_FAILURE);
    }

    // Array to store PIDs of all processes
    pid_t allPID[numberOfProcesses];
    char *nameOfProcess[numberOfProcesses] = {"Server", "Window", "KeyboardManager", "DroneDynamics", "Watchdog"};

    // Loop to fork and launch each process
    for (int i = 0; i < numberOfProcesses; i++) {
        pid_t pid = fork();
        allPID[i] = pid;
        char args[maxMsgLength];

        if (pid == 0) { // Child process
            switch (i) {
                case 0:
                    // Server process
                    sprintf(args, "%d %d", pipeWatchdogServer[0], pipeWatchdogServer[1]);
                    char *argsServer[] = {"./bin/server", args, NULL};
                    summon(argsServer, 0, 0, 0);
                    break;
                case 1:
                    // Window process
                    sprintf(args, "%d %d|%d %d", pipeWindowKeyboard[0], pipeWindowKeyboard[1], 
                            pipeWatchdogWindow[0], pipeWatchdogWindow[1]);
                    char *argsWindow[] = {"/usr/bin/konsole", "-e", "./bin/window", args, NULL};
                    summon(argsWindow, 0, 0, 1);
                    break;
                case 2:
                    // KeyboardManager process
                    sprintf(args, "%d %d|%d %d|%d %d", pipeWindowKeyboard[0], pipeWindowKeyboard[1], 
                            pipeKeyboardDrone[0], pipeKeyboardDrone[1], 
                            pipeWatchdogKeyboard[0], pipeWatchdogKeyboard[1]);
                    char *argsKeyboard[] = {"./bin/keyboardManager", args, NULL};
                    summon(argsKeyboard, 0, 0, 0);
                    break;
                case 3:
                    // DroneDynamics process
                    sprintf(args, "%d %d|%d %d", pipeKeyboardDrone[0], pipeKeyboardDrone[1], 
                            pipeWatchdogDrone[0], pipeWatchdogDrone[1]);
                    char *argsDrone[] = {"./bin/droneDynamics", args, NULL};
                    summon(argsDrone, 0, 0, 0);
                    break;
                case 4:
                    // Watchdog process
                    sprintf(args, "%d %d|%d %d|%d %d|%d %d|%d", pipeWatchdogServer[0], pipeWatchdogServer[1], 
                            pipeWatchdogWindow[0], pipeWatchdogWindow[1], 
                            pipeWatchdogKeyboard[0], pipeWatchdogKeyboard[1], 
                            pipeWatchdogDrone[0], pipeWatchdogDrone[1], allPID[3]);
                    char *argsWatchdog[] = {"/usr/bin/konsole", "-e", "./bin/watchdog", args, NULL};
                    summon(argsWatchdog, 0, 0, 1);
                    break;
            }
        } else if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            printf("Launched %s, PID: %d\n", nameOfProcess[i], pid);
        }
    }

    // Wait for any child process to terminate
    int status;
    pid_t terminatedPid = wait(&status);
    if (terminatedPid == -1) {
        perror("waitpid failed");
        exit(EXIT_FAILURE);
    }

    // Terminate all other processes if one process exits
    for (int i = 0; i < numberOfProcesses; i++) {
        if (allPID[i] != terminatedPid) {
            // Kill process and check for errors
            if (kill(allPID[i], SIGTERM) == -1) {
                perror("kill failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    return EXIT_SUCCESS;
}
