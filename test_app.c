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

    // 1. User space memory allocation
    printf("Step 1: Malloc (mmap)\n");
    char *ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    printf("Allocated ptr: %p\n", ptr);
    if (ioctl(fd, CMD_MALLOC, (unsigned long)ptr) < 0) {
        perror("ioctl CMD_MALLOC failed");
    }

    // 2. User space memory access
    printf("Step 2: Access (Write)\n");
    ptr[0] = 'A'; // Memory write (page fault triggers allocation)
    if (ioctl(fd, CMD_ACCESS, (unsigned long)ptr) < 0) {
        perror("ioctl CMD_ACCESS failed");
    }

    // 3. Kernel space memory allocation
    printf("Step 3: KMALLOC\n");
    if (ioctl(fd, CMD_KMALLOC, 0) < 0) {
        perror("ioctl CMD_KMALLOC failed");
    }

    // Clean up
    munmap(ptr, PAGE_SIZE);
    close(fd);

    printf("Done. Check /proc/malloc_monitor\n");
    return 0;
}
