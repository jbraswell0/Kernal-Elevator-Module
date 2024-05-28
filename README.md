# project2-group-19
---------------------------------------------------------------------
Project 2: Division of Labor (AFTER)
Operating Systems
Team Members:
1. Jared Braswell
2. Elliot Beaver
3. Jacob Proenza-Smith
Part 1: System Call Tracing
- Jared Braswell
Part 2: Timer Kernel Module
- Jared Braswell
- Jacob Proenza-Smith
- Elliot Beaver
Part 3a: Adding System Calls
- Jared Braswell
- Jacob Proenza-Smith
- Elliot Beaver
Part 3b: Kernel Compilation
- Jared Braswell
- Jacob Proenza-Smith
- Elliot Beaver
Part 3c: Threads
- Jared Braswell
- Jacob Proenza-Smith
- Elliot Beaver
Part 3d: Linked List
- Jared Braswell
- Jacob Proenza-Smith
- Elliot Beaver
Part 3e: Mutexes
- Jared Braswell
- Jacob Proenza-Smith
- Elliot Beaver
Part 3f: Scheduling Algorithm
- Jared Braswell
- Jacob Proenza-Smith
- Elliot Beaver
--------------------------------------------------------------------
Files:

Part1:
- empty.c
- empty.trace
- part1.c
- part1.trace

Part2:
- my_timer.c
- Makefile

Part3:
- elevator.c
- syscall_64.tbl
- syscalls.h
- Makefile
- syscalls.c

-------------------------------------------------------------------------------

Running timer:
-Run makefile
-insmod my_timer.ko
-cat /proc/my_timer
-Watch the initial time
-Run a sleep command ex. sleep(3)
-Check elapsed time
-Remove module ex rmmod my_timer

Running the elevator program:
-Navigate to the desired folder ex. Part3.
-Run the 'make' command in the terminal
-insmod elevator.ko
-Navigate to directory that stores the consumer.c and producer.c files
-Run ./consumer --start
-Run ./Producer [desired amount of passengers]
-watch -n 2 cat /proc/elevator
-Once elevator is finished remove kernel module ex. rmmod elevator.ko

---------------------------------------------------------------------

SideNotes: 
Currently the current weight calculation for the elevator does not work correctly. It dispays some weights, but when there are multiple passengers, the total value is not correct. We had a hard time handling the arithmetic with the decimal numbers.
Also, the stop elevator function does not work correctly. Right now when you call stop elevator, it will stop the elevator and delete the current passengers - instead of waiting until all the passengers are off to stop the elevator.
