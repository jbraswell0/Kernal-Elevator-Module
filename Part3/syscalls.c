#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>


#define SYS_elevator_start  448
#define SYS_issue_request   449
#define SYS_elevator_stop   450

long (*elevator_start)(void);
long (*issue_request)(int, int, int);
long (*elevator_stop)(void);

int main() {
    elevator_start = (long (*)(void)) syscall(sys_elevator_start);
    issue_request = (long (*)(int, int, int)) syscall(sys_issue_request);
    elevator_stop = (long (*)(void)) syscall(sys_elevator_stop);

    printf("Calling elevator_start...\n");
    long start_result = elevator_start();
    printf("Result of elevator_start: %ld\n", start_result);

    printf("Calling issue_request...\n");
    long request_result = issue_request(1, 10, 5); 
    printf("Result of issue_request: %ld\n", request_result);

    printf("Calling elevator_stop...\n");
    long stop_result = elevator_stop();
    printf("Result of elevator_stop: %ld\n", stop_result);

    return 0;
}
