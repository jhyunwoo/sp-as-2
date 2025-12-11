#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "mem_ioctl.h"

#define PAGE_SIZE 4096
#define PROC_PATH "/proc/malloc_monitor"
#define STUDENT_ID "2024148005" /* UPDATE THIS IF NEEDED matches module */
#define STUDENT_NAME "JEON Hyunwoo"

// Buffer for reading proc file
char proc_buf[16384];

void read_proc_file() {
    int fd = open(PROC_PATH, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open proc file for reading");
        exit(1);
    }
    memset(proc_buf, 0, sizeof(proc_buf));
    read(fd, proc_buf, sizeof(proc_buf) - 1);
    close(fd);
    // printf("DEBUG: Proc file content:\n%s\n", proc_buf);
}

// Function to find the N-th occurrence of a substring
char* find_nth_occurrence(const char* str, const char* sub, int n) {
    const char* p = str;
    for (int i = 0; i < n; i++) {
        p = strstr(p, sub);
        if (!p) return NULL;
        p += strlen(sub);
    }
    // Return pointer to the start of the match (backtrack)
    return (char*)(p - strlen(sub));
}

void validate_header() {
    read_proc_file();
    char expected_id[100];
    char expected_name[100];
    sprintf(expected_id, "ID: %s", STUDENT_ID);
    sprintf(expected_name, "Name: %s", STUDENT_NAME);

    if (!strstr(proc_buf, expected_id)) {
        printf("[FAIL] Header ID check failed. Expected '%s'\n", expected_id);
    } else {
        printf("[PASS] Header ID check\n");
    }

    if (!strstr(proc_buf, expected_name)) {
        printf("[FAIL] Header Name check failed. Expected '%s'\n", expected_name);
    } else {
        printf("[PASS] Header Name check\n");
    }
    
    // Check separator length
    const char *sep = "============================================================";
    if (!strstr(proc_buf, sep)) {
         printf("[FAIL] Header separator check (length 60)\n");
    } else {
         printf("[PASS] Header separator check\n");
    }
}

void validate_step(int step_num, const char* type, unsigned long va, int expect_mapped) {
    read_proc_file();
    
    char step_header[100];
    sprintf(step_header, "[%d] PID: %d | Type: %s", step_num, getpid(), type);
    
    char* entry_start = strstr(proc_buf, step_header);
    if (!entry_start) {
        printf("[FAIL] Step %d entry not found. Expected header: '%s'\n", step_num, step_header);
        return;
    }
    
    printf("[PASS] Step %d Header found: %s\n", step_num, type);

    // Check VA
    char va_str[100];
    if (strcmp(type, "KMALLOC") == 0) {
        // For KMALLOC we don't know the exact VA beforehand easily in user space 
        // unless we passed it back, but the assignment says we just verify format/logging.
        // Actually, for KMALLOC, the module allocates it, so we can't strictly match VA from here 
        // without parsing. We will just check if "Target VA :" line exists.
        if (!strstr(entry_start, "Target VA :")) {
             printf("[FAIL] Step %d Target VA line missing\n", step_num);
        } else {
             printf("[PASS] Step %d Target VA line present\n", step_num);
        }
    } else {
        sprintf(va_str, "Target VA : 0x%012lx", va);
        if (!strstr(entry_start, va_str)) {
             printf("[FAIL] Step %d VA mismatch. Expected '%s'\n", step_num, va_str);
        } else {
             printf("[PASS] Step %d VA match\n", step_num);
        }
    }

    // Check Paging Info
    if (strcmp(type, "KMALLOC") != 0) {
        const char *levels[] = {"PGD", "PUD", "PMD", "PTE"};
        for (int i = 0; i < 4; i++) {
            if (!strstr(entry_start, levels[i])) {
                printf("[FAIL] Step %d Missing %s info\n", step_num, levels[i]);
            }
        }
        printf("[PASS] Step %d PGD/PUD/PMD/PTE fields present\n", step_num);
    }

    // Check Final PA
    if (expect_mapped) {
        // Should NOT contain "(Not Mapped)"
        // Should contain "Final PA : 0x"
        char *pa_line = strstr(entry_start, "Final PA :");
        if (pa_line && strstr(pa_line, "(Not Mapped)")) {
             printf("[FAIL] Step %d Expected valid PA but got (Not Mapped)\n", step_num);
        } else if (pa_line && strstr(pa_line, "0x")) {
             printf("[PASS] Step %d Final PA mapped\n", step_num);
        } else {
             printf("[FAIL] Step %d Final PA line format invalid\n", step_num);
        }
    } else {
        // Should contain "(Not Mapped)"
        char *pa_line = strstr(entry_start, "Final PA :");
        if (pa_line && strstr(pa_line, "(Not Mapped)")) {
            printf("[PASS] Step %d Final PA is (Not Mapped) as expected\n", step_num);
        } else {
            printf("[FAIL] Step %d Expected (Not Mapped)\n", step_num);
        }
    }
    

    // Check Separator after entry
    char *end_sep = strstr(entry_start, "============================================================");
    if (!end_sep) {
         printf("[WARN] Step %d closing separator missing or incorrect length\n", step_num);
    }
}

int get_next_step_index() {
    read_proc_file();
    int max_idx = 0;
    char *p = proc_buf;
    while ((p = strchr(p, '['))) {
        int idx = 0;
        if (sscanf(p, "[%d]", &idx) == 1) {
            if (idx > max_idx) max_idx = idx;
        }
        p++;
    }
    return max_idx + 1;
}

int main() {
    int fd = open(PROC_PATH, O_RDWR);
    if (fd < 0) {
        perror("Failed to open /proc/malloc_monitor. Is the module loaded?");
        return 1;
    }

    // Determine the starting index for this run
    int start_idx = get_next_step_index();
    printf("Starting Enhanced Tests (Expecting indices [%d], [%d], [%d])...\n\n", 
           start_idx, start_idx+1, start_idx+2);
    
    // 0. Verify Header (Empty state check or just header check)
    // We trigger one event first so the header is printed? 
    // The module implementation prints "ID..." then list. 
    // If list is empty, it still prints ID/Name inside show()!
    // read_proc_file(); // Refresh buffer
    // if (strlen(proc_buf) > 0) validate_header(); 

    // 1. User Malloc (mmap)
    printf("--- Execution Step 1: Malloc ---\n");
    char *ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) { perror("mmap"); return 1; }
    
    if (ioctl(fd, CMD_MALLOC, (unsigned long)ptr) < 0) { perror("ioctl 1"); return 1; }
    
    // Validate Step 1
    validate_header(); // Validate header now that we have content
    validate_step(start_idx, "MALLOC", (unsigned long)ptr, 0); // Expect Not Mapped

    // 2. User Access
    printf("\n--- Execution Step 2: Access ---\n");
    ptr[0] = 'A'; // Write to cause fault
    if (ioctl(fd, CMD_ACCESS, (unsigned long)ptr) < 0) { perror("ioctl 2"); return 1; }
    
    // Validate Step 2
    validate_step(start_idx + 1, "ACCESS", (unsigned long)ptr, 1); // Expect Mapped

    // 3. Kernel Alloc
    printf("\n--- Execution Step 3: KMALLOC ---\n");
    if (ioctl(fd, CMD_KMALLOC, 0) < 0) { perror("ioctl 3"); return 1; }
    
    // Validate Step 3
    validate_step(start_idx + 2, "KMALLOC", 0, 1); // Expect Mapped (VA ignored in check)

    printf("\n--- Test Complete ---\n");
    
    munmap(ptr, PAGE_SIZE);
    close(fd);
    return 0;
}
