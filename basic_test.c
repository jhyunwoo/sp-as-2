#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include "mem_ioctl.h"

#define PAGE_SIZE 4096

int main() {
    int fd = open("/proc/malloc_monitor", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /proc/malloc_monitor");
        return 1;
    }

    printf("Running Basic Test based on assignment example...\n");

    // Example of user code from assignment
    /* 1. User space memory allocation */
    char *ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1, 0); // Allocate new memory space
    
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }

    if (ioctl(fd, CMD_MALLOC, ptr) < 0) { // Send signal to kernel module
        perror("ioctl CMD_MALLOC failed");
    }

    /* 2. User space memory access */
    ptr[0] = 'A'; // Memory write
    if (ioctl(fd, CMD_ACCESS, ptr) < 0) { // Send signal to kernel module
        perror("ioctl CMD_ACCESS failed");
    }

    /* 3. Kerel space memory allocation */
    if (ioctl(fd, CMD_KMALLOC, 0) < 0) { // Send signal to kernel module
        perror("ioctl CMD_KMALLOC failed");
    }

    // Cleanup (not in snippet but good practice)
    munmap(ptr, PAGE_SIZE);
    close(fd);

    printf("Basic Test Complete. Check /proc/malloc_monitor\n");
    return 0;
}
