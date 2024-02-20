#define _GNU_SOURCE
#include <unistd.h>
#include <fcntl.h>

int main() {
    fork();
    getpid();
    getgid();
    getppid();
    getuid();
    return 0;
}