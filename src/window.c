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
// Global variables for ncurses windows
WINDOW *display = NULL;
WINDOW *scoreboard = NULL;
// Signal handler function for SIGWINCH
void handleResize(int sig)
{
    endwin(); 
    refresh(); 
    setupNcursesWindows(&display, &scoreboard);  
    refresh(); 
}

void displayObstacles(WINDOW *win, Point *obstacles, double scalex, double scaley) {
    wattron(win, COLOR_PAIR(3)); // Set color to orange
    for (int i = 0; i < NUM_OBSTACLES; ++i) {
        mvwprintw(win, (int)(obstacles[i].y / scaley), (int)(obstacles[i].x / scalex), "#");
    }
    wattroff(win, COLOR_PAIR(3));
}

// Function to display targets with fixed numbers
void displayTargets(WINDOW *win, Point *targets, double scalex, double scaley) {
    wattron(win, COLOR_PAIR(4)); // Set color to green
    for (int i = 0; i < NUM_TARGETS; ++i) {
        mvwprintw(win, (int)(targets[i].y / scaley), (int)(targets[i].x / scalex), "%d", targets[i].number);
    }
    wattroff(win, COLOR_PAIR(4));
}


void logData(FILE *logFile, double *position, int sharedSegSize, int score)
{
    time_t rawtime;
    struct tm *timeinfo;
    char timeStr[9]; // String to store time (HH:MM:SS)

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);

    fprintf(logFile, "[Time: %s] Drone Position: %.2f, %.2f | Score: %d\n",
            timeStr, position[4], position[5], score);
    fflush(logFile);
}

int main(int argc, char *argv[])
{
    // Initializing ncurses
    initscr();
    int key, initial;
    initial = 0;
    int totalScore;
    int lastObstacleHit = 0; 


    // Setting up colors
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_GREEN, COLOR_BLACK);

    // Setting up signal handling for window resize
    signal(SIGWINCH, handleResize);

    // Setting up signal handling for watchdog
    struct sigaction signalAction;

    signalAction.sa_sigaction = handleSignal;
    signalAction.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &signalAction, NULL);
    sigaction(SIGUSR1, &signalAction, NULL);

    // Extracting pipe information from command line arguments
    int pipeWindowKeyboard[2], pipeWatchdogWindow[2], pipeObstaclesWindow[2], pipeTargetsWindow[2];
    sscanf(argv[1], "%d %d|%d %d|%d %d|%d %d", &pipeWindowKeyboard[0], &pipeWindowKeyboard[1], &pipeWatchdogWindow[0], &pipeWatchdogWindow[1], &pipeObstaclesWindow[0], &pipeObstaclesWindow[1], &pipeTargetsWindow[0], &pipeTargetsWindow[1]);
    close(pipeWindowKeyboard[0]);
    close(pipeWatchdogWindow[0]);
    close(pipeObstaclesWindow[1]);
    close(pipeTargetsWindow[1]);

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
     
    // Open the log files
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

        // Reading obstacles from the pipe
        Point obstacles[NUM_OBSTACLES];
        if (read(pipeObstaclesWindow[0], obstacles, sizeof(obstacles)) == -1) {
            perror("read pipeObstaclesWindow");
            exit(EXIT_FAILURE);
        }

        // Reading targets from the pipe
        Point targets[NUM_TARGETS];

        read(pipeTargetsWindow[0], targets, sizeof(targets));

        // Print the score in the scoreboard window
        wattron(scoreboard, COLOR_PAIR(1));
        mvwprintw(scoreboard, 1, 1, "Position of the drone: %.2f,%.2f", position[4], position[5]);
        wattroff(scoreboard, COLOR_PAIR(1));


      // Read the removed target value from shared memory
        int sharedValue; 
        sem_wait(semID);
        memcpy(&sharedValue, shmPointer, sizeof(sharedValue));
        sem_post(semID);

      
       // Check if there's a new obstacle hit
        if (sharedValue == -1 && lastObstacleHit == 0) {
            // New obstacle hit detected
            totalScore -= 2;
            lastObstacleHit = 1; // Update the last obstacle hit state
        } else if (sharedValue != -1) {
            // If the current value is not an obstacle hit, reset the last obstacle hit state
            lastObstacleHit = 0;
        }

        // Handle valid target score updates
        if (1 <= sharedValue && sharedValue <= 10) {
            totalScore += sharedValue;
        }

            // Display the score
             wattron(scoreboard, COLOR_PAIR(1));
             mvwprintw(scoreboard, 2, 1, "Score: %d", totalScore); // Display cumulative score
             wattroff(scoreboard, COLOR_PAIR(1));
        

        // Display obstacles on the window
        displayObstacles(win, obstacles, scalex, scaley);
        
        // Display targets on the window
        displayTargets(win, targets, scalex, scaley);

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
        logData(logFile, position, sharedSegSize, totalScore);
        clear();
        sleep(1);
    }

    // Cleaning up
    close(pipeObstaclesWindow[0]);
    close(pipeTargetsWindow[0]);
    shm_unlink(SHM_PATH);
    sem_close(semID);
    sem_unlink(SEM_PATH);

    endwin();

    // Closing the log file
    fclose(logFile);

    return 0;
}
