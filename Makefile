CC = gcc
CFLAGS = -Wall
LIBS = -lrt -pthread -lncurses -lm

# Source files
SERVER_SRC = src/server.c
WINDOW_SRC = src/window.c
KEYBOARD_MANAGER_SRC = src/keyboardManager.c
DRONE_DYNAMICS_SRC = src/droneDynamics.c
WATCHDOG_SRC = src/watchdog.c
MASTER_SRC = src/master.c

# Object files
SERVER_OBJ = bin/server
WINDOW_OBJ = bin/window
KEYBOARD_MANAGER_OBJ = bin/keyboardManager
DRONE_DYNAMICS_OBJ = bin/droneDynamics
WATCHDOG_OBJ = bin/watchdog
MASTER_OBJ = bin/master

# Default target
all: $(SERVER_OBJ) $(WINDOW_OBJ) $(KEYBOARD_MANAGER_OBJ) $(DRONE_DYNAMICS_OBJ) $(WATCHDOG_OBJ) $(MASTER_OBJ)
	./bin/master

$(SERVER_OBJ): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER_OBJ) $(SERVER_SRC) $(LIBS)

$(WINDOW_OBJ): $(WINDOW_SRC)
	$(CC) $(CFLAGS) -o $(WINDOW_OBJ) $(WINDOW_SRC) $(LIBS)

$(KEYBOARD_MANAGER_OBJ): $(KEYBOARD_MANAGER_SRC)
	$(CC) $(CFLAGS) -o $(KEYBOARD_MANAGER_OBJ) $(KEYBOARD_MANAGER_SRC) $(LIBS)

$(DRONE_DYNAMICS_OBJ): $(DRONE_DYNAMICS_SRC)
	$(CC) $(CFLAGS) -o $(DRONE_DYNAMICS_OBJ) $(DRONE_DYNAMICS_SRC) $(LIBS)

$(WATCHDOG_OBJ): $(WATCHDOG_SRC)
	$(CC) $(CFLAGS) -o $(WATCHDOG_OBJ) $(WATCHDOG_SRC) $(LIBS)

$(MASTER_OBJ): $(MASTER_SRC)
	$(CC) $(CFLAGS) -o $(MASTER_OBJ) $(MASTER_SRC) -pthread

clean:
	rm -rf bin/*
	rm -rf log/*

.PHONY: all clean
