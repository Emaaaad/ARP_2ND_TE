CC = gcc
CFLAGS = -Wall
LIBS = -lrt -pthread -lncurses -lm

# Source files
SERVER_SRC = src/server.c
WINDOW_SRC = src/window.c
KEYBOARD_MANAGER_SRC = src/keyboardManager.c
DRONE_DYNAMICS_SRC = src/droneDynamics.c
WATCHDOG_SRC = src/watchdog.c
TARGETS_SRC = src/targets.c
OBSTACLES_SRC = src/obstacles.c
MASTER_SRC = src/master.c

# Object files
SERVER_OBJ = bin/server
WINDOW_OBJ = bin/window
KEYBOARD_MANAGER_OBJ = bin/keyboardManager
DRONE_DYNAMICS_OBJ = bin/droneDynamics
WATCHDOG_OBJ = bin/watchdog
TARGETS_OBJ = bin/targets
OBSTACLES_OBJ = bin/obstacles
MASTER_OBJ = bin/master

# Directories
BIN_DIR = bin
LOG_DIR = log

# Default target
all: create_directories $(SERVER_OBJ) $(WINDOW_OBJ) $(KEYBOARD_MANAGER_OBJ) $(DRONE_DYNAMICS_OBJ) $(WATCHDOG_OBJ) $(TARGETS_OBJ) $(OBSTACLES_OBJ) $(MASTER_OBJ)
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

$(TARGETS_OBJ): $(TARGETS_SRC)
	$(CC) $(CFLAGS) -o $(TARGETS_OBJ) $(TARGETS_SRC) $(LIBS)

$(OBSTACLES_OBJ): $(OBSTACLES_SRC)
	$(CC) $(CFLAGS) -o $(OBSTACLES_OBJ) $(OBSTACLES_SRC) $(LIBS)

$(MASTER_OBJ): $(MASTER_SRC)
	$(CC) $(CFLAGS) -o $(MASTER_OBJ) $(MASTER_SRC) -pthread

create_directories:
	mkdir -p $(BIN_DIR)
	mkdir -p $(LOG_DIR)

clean:
	@echo "Cleaning up..."
	@echo "Removing binaries in $(BIN_DIR)..."
	rm -rf $(BIN_DIR)
	@echo "Removing logs in $(LOG_DIR)..."
	rm -rf $(LOG_DIR)
	@echo "Cleanup complete."

.PHONY: all clean create_directories
