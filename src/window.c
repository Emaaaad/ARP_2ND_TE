#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include "../include/constant.h"

// Function for creating a new window
WINDOW *createBoard(int height, int width, int starty, int startx)
{
    WINDOW *displayWindow;

    displayWindow = newwin(height, width, starty, startx);

    // Left border
    for (int i = 0; i < height * 0.9; ++i)
    {
        mvwaddch(displayWindow, i, 0, '|');
    }

    // Upper border
    for (int i = 0; i < width * 0.9; ++i)
    {
        mvwaddch(displayWindow, 0, i, '=');
    }

    // Right border
    int rightBorderX = (int)(width * 0.9);
    for (int i = 0; i < height * 0.9; ++i)
    {
        mvwaddch(displayWindow, i, rightBorderX, '|');
    }

    // Bottom border
    int bottomBorderY = (int)(height * 0.9);
    for (int i = 0; i < width * 0.9; ++i)
    {
        mvwaddch(displayWindow, bottomBorderY, i, '=');
    }

    wrefresh(displayWindow);

    return displayWindow;
}

// Function for setting up ncurses windows
void setupNcursesWindows(WINDOW **display, WINDOW **scoreboard)
{
    int inPos[4] = {LINES / 200, COLS / 200, (LINES / 200) + LINES * windowHeight, COLS / 200};

    int displayHeight = LINES * windowHeight;
    int displayWidth = COLS * windowWidth;

    int scoreboardHeight = LINES * scoreboardWinHeight;
    int scoreboardWidth = COLS * windowWidth;

    *scoreboard = createBoard(scoreboardHeight, scoreboardWidth, inPos[2], inPos[3]);
    *display = createBoard(displayHeight, displayWidth, inPos[0], inPos[1]);
}

// Function to logging data to a file
void logData(FILE *logFile, double *position, int sharedSegSize)
{
    fprintf(logFile, "Current Position:  %.2f, %.2f\n",
            position[4], position[5]);
    fflush(logFile);
}

int main(int argc, char *argv[])
{
    // Initializing ncurses
    initscr();
    int key, initial;
    initial = 0;

    // Setting up colors
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);

    // Setting up signal handling for watchdog
    struct sigaction signalAction;

    signalAction.sa_sigaction = handleSignal;
    signalAction.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &signalAction, NULL);
    sigaction(SIGUSR1, &signalAction, NULL);

    // Extracting pipe information from command line arguments
    int pipeWindowKeyboard[2], pipeWatchdogWindow[2];
    sscanf(argv[1], "%d %d|%d %d", &pipeWindowKeyboard[0], &pipeWindowKeyboard[1], &pipeWatchdogWindow[0], &pipeWatchdogWindow[1]);
    close(pipeWindowKeyboard[0]);
    close(pipeWatchdogWindow[0]);

    // Sending PID to watchdog
    pid_t windowPID;
    windowPID = getpid();
    if (write(pipeWatchdogWindow[1], &windowPID, sizeof(windowPID)) == -1)
    {
        perror("write pipeWatchdogWindow");
        exit(EXIT_FAILURE);
    }
    printf("%d\n", windowPID);
    close(pipeWatchdogWindow[1]);

    // Shared memory setup
    double position[6] = {boardSize / 2, boardSize / 2, boardSize / 2, boardSize / 2, boardSize / 2, boardSize / 2};
    int sharedSegSize = (sizeof(position));

    sem_t *semID = sem_open(SEM_PATH, 0);
    if (semID == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    int shmfd = shm_open(SHM_PATH, O_RDWR, S_IRWXU | S_IRWXG);
    if (shmfd < 0)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    void *shmPointer = mmap(NULL, sharedSegSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shmPointer == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Open the log file
    FILE *logFile;
    char logFilePath[100];
    snprintf(logFilePath, sizeof(logFilePath), "log/windowLog.txt");
    logFile = fopen(logFilePath, "w");

    if (logFile == NULL)
    {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Refreshing windows
        WINDOW *win, *scoreboard;
        setupNcursesWindows(&win, &scoreboard);
        curs_set(0);
        nodelay(win, TRUE);

        double scalex, scaley;

        scalex = (double)boardSize / ((double)COLS * (windowWidth - 0.1));
        scaley = (double)boardSize / ((double)LINES * (windowHeight - 0.1));

        // Sending the first drone position to drone.c via shared memory
        if (initial == 0)
        {
            sem_wait(semID);
            memcpy(shmPointer, position, sharedSegSize);
            sem_post(semID);

            initial++;
        }

        // Showing the drone and position in the konsole
        wattron(win, COLOR_PAIR(2));
        mvwprintw(win, (int)(position[5] / scaley), (int)(position[4] / scalex), "+");
        wattroff(win, COLOR_PAIR(2));

        wattron(scoreboard, COLOR_PAIR(1));
        mvwprintw(scoreboard, 1, 1, "Position of the drone: %.2f,%.2f", position[4], position[5]);
        wattroff(scoreboard, COLOR_PAIR(1));

        wrefresh(win);
        wrefresh(scoreboard);
        noecho();

        // Sending user input to keyboardManager.c
        key = wgetch(win);
        if (key != ERR)
        {
            int keypress = write(pipeWindowKeyboard[1], &key, sizeof(key));
            if (keypress < 0)
            {
                perror("writing error");
                close(pipeWindowKeyboard[1]);
                exit(EXIT_FAILURE);
            }
            if ((char)key == 'q')
            {
                close(pipeWindowKeyboard[1]);
                fclose(logFile);
                exit(EXIT_SUCCESS);
            }
        }
        usleep(200000);

        // Reading from shared memory
        sem_wait(semID);
        memcpy(position, shmPointer, sharedSegSize);
        sem_post(semID);

        // Writing to the log file
        logData(logFile, position, sharedSegSize);
        clear();
        sleep(1);
    }

    // Cleaning up
    shm_unlink(SHM_PATH);
    sem_close(semID);
    sem_unlink(SEM_PATH);

    endwin();

    // Closing the log file
    fclose(logFile);

    return 0;
}
