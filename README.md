# Multi-Process Drone System

## Contributors
- [Seyed Emad Razavi](https://github.com/Emaaaad)
- [Tanvir Rahman Sajal](https://github.com/tanvirrsajal)

## Overview
This project implements a multi-process drone navigation system in C. The system is managed by a master process and includes components such as keyboard input, drone dynamics, window display, watchdog, server, obstacles, and targets. Users can control the drone's movement using predefined keys, and the system displays the drone's movement on the screen.

## Features
- Multi-process architecture
- User-controlled drone movement
- Real-time display of drone position
- Watchdog for process monitoring
- Shared memory and inter-process communication

## Components
1. **keyboardManager:** Accepts user input to control the drone's direction and force.
2. **droneDynamics:** Computes the drone's position based on user input and obstacle avoidance logic.
3. **Window:** Displays the drone's movement on the screen.
4. **watchdog:** Monitors all processes to ensure correct operation.
5. **server:** Handles shared memory access and communication with the watchdog.
6. **obstacles:** Generates obstacles that the drone must navigate around.
7. **targets:** Defines targets for the drone to reach.

The processes communicate using shared memory and semaphores, pipes, and the system is designed to run in a Linux environment. The user can control the drone by pressing defined keys on the keyboard, and the drone moves accordingly on the screen. Two Konsoles are displayed; one shows the drone movement and the other for the watchdog, displaying the sent and received signals to ensure everything is working properly.

This is the first part of the complete project. Future enhancements will include targets and obstacles, which the user will need to navigate, grabbing targets and avoiding obstacles.

## Tools Required
- GCC compiler
- A Linux environment (tested only on Linux)
- Ncurses library
- Konsole terminal

## How to Run

### Compile
To compile the code, navigate to the project directory in the terminal and type `make`. This will compile the source files and generate the executable.

### Removing Old Bin Files
Before recompiling the code, it's recommended to remove old binary files. To do this, run the command:

```bash
make clean 
```
The user can command the drone using the keyboard with the following directions:

- `w`: Move up-left
- `e`: Move up
- `r`: Move up-right
- `s`: Move left
- `d`: Stop
- `f`: Move right
- `x`: Move down-left
- `c`: Move down
- `v`: Move down-right

Note: Pressing the same key increases the speed of the drone.

## Components System and Architecture
![System Architecture](https://github.com/Emaaaad/ARP_1ST_TE/blob/main/ARP1%20(1).png)

### Master Process (`master.c`)

The `master.c` module serves as the central command unit of our multi-process drone system, orchestrating the various components crucial to the system's functionality. It performs several key roles:

- **Process Creation and Management:** Utilizing fundamental fork mechanisms, it initiates and oversees the child processes essential for the system: the drone, server, keyboard manager, and watchdog. This creation process includes assigning and tracking their Process Identifiers (PIDs) for effective management.

- **Inter-Process Communication:** The master process is adept at facilitating communication between these child processes using pipes. This setup ensures a streamlined flow of information, allowing each process to function in concert with others.

- **Lifecycle Control:** A significant aspect of its functionality is to monitor the lifecycle of these child processes. It efficiently manages their initiation, operational state, and termination.

- **Graceful Termination:** In response to a user interrupt signal (such as `SIGINT`), the master process takes charge to orderly conclude the system's operation. It does this by terminating all child processes in a controlled manner, thereby ensuring a clean and stable end to the simulation.

The `master.c`'s robust process management and inter-process communication establish it as the backbone of the drone system, ensuring a cohesive and synchronized operation across all components.

### Window Process (`window.c`)

The `window.c` module is integral to the user interface of our multi-process drone system, primarily handling the system's visual output. Its responsibilities and workflow are as follows:

- **Initialization:** It starts by initializing the ncurses library, which is pivotal for creating the system's text-based user interface. This step includes configuring color schemes and installing necessary signal handlers for `SIGINT` and `SIGUSR1`, ensuring responsive and controlled behavior under different system states.

- **Communication Setup:** The process establishes a communication channel with the `keyboardManager` by reading from specified pipes. Additionally, it registers itself with the system's monitoring framework by sending its Process Identifier (PID) to the watchdog. This action integrates the `window` process into the overall process supervision.

- **Core Loop Operations:**
    1. **Reading Drone Position:** Continuously fetches the drone's current coordinates from shared memory, ensuring that the visual representation is synchronized with the drone's actual location.
    2. **Handling User Input:** Captures and forwards user inputs to the `keyboardManager`. These inputs are crucial as they dictate the drone's movements.
    3. **Display Update:** Utilizes the ncurses library to dynamically update the drone's position on the screen, providing real-time visual feedback to the user.

- **Termination:** Upon receiving a `SIGINT` signal, the `window` process gracefully exits, closing the ncurses interface and ensuring a smooth and orderly shutdown of the visual component of the system.

This module not only presents the real-time position of the drone but also plays a crucial role in facilitating user interaction, making it a cornerstone of the system's user experience.


### Keyboard Manager Process (`keyboardManager.c`)

The `keyboardManager.c` module is a critical component for user interaction in our multi-process drone system. It has several key functions:

- **Establishing Communication:** The module initiates its operation by setting up a communication link with the `window` process. This connection is vital for the real-time reception of user inputs, which are the primary drivers of the drone's movement.

- **Input Interpretation:** Each user input, specifying direction and force, is read continuously from the `window` process. The `keyboardManager` processes these inputs, translating them into precise commands that dictate the drone's trajectory and speed.

- **Command Transmission:** These navigational directives are then relayed to the `droneDynamics.c` module via a dedicated pipe. This design ensures uninterrupted and fluid command flow from the user to the drone's control system.

- **Operational Loop and Termination:** The process maintains an ongoing loop of reading inputs and updating drone motion parameters. This loop persists until the `keyboardManager` receives a `SIGINT` signal, at which point it gracefully ceases operations. This controlled termination not only halts command transmission but also ensures an orderly conclusion of the user input handling function within the system.

The `keyboardManager.c`'s adept handling of user inputs and seamless command relay underscores its pivotal role in facilitating an engaging and responsive user experience in the drone simulation.


### Drone Dynamics Process (`droneDynamics.c`)

The `droneDynamics.c` component is crucial for the operational aspect of the drone within our multi-process system. Its initialization and ongoing functions include:

- **Initialization and Configuration:** Upon startup, `droneDynamics.c` secures its Process Identifier (PID) and sets up shared memory segments and semaphores. This configuration is key to establishing a synchronized communication channel with both the `server` and `window` processes.

- **Acquiring Initial Coordinates:** The process begins its operational cycle by fetching the drone's initial position from the `window.c` interface, laying the foundation for its navigational computations.

- **Responsive Command Processing:** In a continuous loop, `droneDynamics.c` listens attentively for directional commands from the `keyboardManager`. These inputs are integral to determining the drone's subsequent movements.

- **Position Recalculation and Update:** After assimilating the commands, the process recalculates the drone's position. The newly computed coordinates are then promptly written back to the shared memory. This allows the `window` process to access and display the drone's current location, providing real-time feedback in the simulation environment.

- **Graceful Termination:** The lifecycle of `droneDynamics.c` is designed to be responsive and adaptable. It remains in its operational loop until it receives a `SIGINT` signal. Upon this signal, the process terminates gracefully, ensuring an orderly and clean cessation of its activities within the broader context of the multi-process system.

Through these functionalities, the `droneDynamics.c` process plays a pivotal role in the dynamic simulation of the drone's movements, directly impacting the system's interactivity and user engagement.

### Server Process (`server.c`)

The `server.c` module is a key component in maintaining the integrity and smooth operation of our multi-process drone system. Its initiation and operational procedures are outlined as follows:

- **Initiation and Monitoring Integration:** At the start, `server.c` announces its presence to the watchdog by sending its Process Identifier (PID). This step is crucial for integrating the server process into the system's overall health monitoring, ensuring any issues are promptly detected for maintaining system reliability.

- **Active Data Retrieval:** A primary function of this module is to actively access the drone's positional data stored in shared memory. This task is vital for various system operations that depend on the drone's current location.

- **Concurrent Access Management:** To handle access to shared memory effectively, especially considering the simultaneous read-write operations by different processes, `server.c` employs semaphores. These semaphores are instrumental in orchestrating orderly access to the shared memory, preventing data conflicts and ensuring data integrity.

- **Synchronized Operations:** The server process not only retrieves data but also plays a pivotal role in maintaining a synchronized state within the system. It ensures that the drone's positional data is consistently current and accurately reflects the ongoing read-write dynamics between the server's read operations and the drone's write operations.

This meticulous approach adopted by the `server.c` process underscores its significance in the system, particularly in terms of data synchronization and operational harmony between various components of the drone system.


### Watchdog Process (`watchdog.c`)

The `watchdog.c` module functions as a critical monitoring component in our multi-process drone system, ensuring the stability and responsiveness of all processes. Its key operations include:

- **Initialization and Signal Handling:** Initially, `watchdog.c` acquires the Process Identifiers (PIDs) of all other processes in the system. This enables it to monitor and manage these processes effectively. Additionally, it sets up handlers for signals like `SIGINT` and `SIGUSR2`, preparing it to respond appropriately to various system states and requests.

- **Process Monitoring:** One of the primary functions of the watchdog is to continuously send `SIGUSR1` signals to all processes. This serves as a regular check to ensure that each process is running and responsive. Alongside this, the watchdog maintains counters for each process, tracking their responsiveness over time.

- **Responsiveness Check and System Termination:** If any process fails to respond within a predetermined threshold, it is a signal to the watchdog that something may be wrong. In such cases, the watchdog takes decisive action by sending a `SIGINT` signal to terminate all processes, effectively bringing the system to a controlled stop. This is a crucial mechanism to prevent system malfunctions or unresponsive states.

- **Self-Termination:** The watchdog itself is also designed to terminate gracefully upon receiving a `SIGINT` signal. This allows for a clean shutdown of the monitoring component when the system is being intentionally stopped.

Through these functionalities, `watchdog.c` upholds the operational integrity of the drone system, playing a vital role in maintaining the overall health and reliability of the process ecosystem.


## Conclusion
We have developed a sophisticated multi-process drone simulation system that leverages shared memory for efficient inter-process communication and features a text-based user interface. This system exemplifies the collaborative operation of multiple processes: the keyboardManager captures and processes user commands to dictate the drone's trajectory; the droneDynamics module dynamically computes the drone's flight position; the window process renders the drone's real-time location within the simulation environment; the watchdog oversees process health, ensuring system integrity; and the server process facilitates access to shared memory, acting as a conduit between the drone's operational logic and the supervisory watchdog. This program not only demonstrates the practical application of concurrent processing and synchronization mechanisms, such as semaphores, but also establishes a robust framework upon which more complex functionalities, such as obstacle avoidance and target tracking, can be integrated in future iterations of the project.
