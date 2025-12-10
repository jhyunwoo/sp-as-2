2025 SP Assignment #2
Due: Dec. 15th 12:00pm
Last updated: 2025-11-23
1. Overview
The Linux kernel utilizes a 4-Level Page Table structure to manage the mapping between virtual memory and physical
memory. When a process accesses memory, the CPU (MMU) sequentially references these tables to translate the
Virtual Address (VA) into a Physical Address (PA). Furthermore, to ensure memory efficiency, Linux employs
Demand Paging, a technique that delays the allocation of physical frames until the memory is actually accessed.
In this assignment, you will write a kernel module to transparently observe this entire address translation process.
Whenever a program allocates or uses memory, it will request recording via ioctl to the kernel module. The kernel
module will then track and store the addresses and values of every translation step leading from the virtual address to
the physical address. You will verify the final results through the proc file system, aiming to gain an understanding of
the Linux memory management mechanism. The assignment consists of a report and a programming assignment.
2. Background: IOCTL (Input/Output Control)
Unlike read/write used in the previous assignment, ioctl is a system call used to exchange complex data (such as
structures) between a user program and a kernel module or to execute specific commands. It is an interface used when
complex configuration values need to be passed or specific hardware commands need to be executed, rather than a
simple byte stream. It is widely used in device driver development.
In this assignment, the user program uses ioctl to pass memory addresses to the kernel module. Below is a simple
example of exchanging integer values using ioctl.
1) Shared Header File (test_ioctl.h): Defines the data structure and commands to be used by both the user and
the kernel. This file must be present in both the kernel module and user program directories.
// test_ioctl.h
#ifndef TEST_IOCTL_H
#define TEST_IOCTL_H
#include <linux/ioctl.h>
#define IOCTL_MAGIC ‘a’
struct ioctl_info { int data; }; // Structure to hold data
// _IOWR: Read/Write capable, 'a': Magic number, 1: Sequence number
#define IOCTL_CMD_DOUBLE _IOWR(IOCTL_MAGIC, 1, struct ioctl_info)
#endif
2) User program (user.c)
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "test_ioctl.h" // Shared header
int main() {
// 1. Open the proc file created by the kernel module
int fd = open("/proc/test_proc", O_RDWR);
struct ioctl_info info = { 100 }; // Prepare data
// 2. Send command to kernel (info.data changes after execution)
ioctl(fd, IOCTL_CMD_DOUBLE, &info);
printf("Result: %d\n", info.data); // Output: 200
close(fd);
return 0;
}
3) Kernel Module (mod.c): Register the handler using the .proc_ioctl field of the proc_ops structure.
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "test_ioctl.h" // Shared header
static long my_ioctl(struct file *f, unsigned int cmd, unsigned long arg) {
struct ioctl_info k_info;
if (cmd == IOCTL_CMD_DOUBLE) {
// 1. Copy user data to kernel
if (copy_from_user(&k_info, (void*)arg, sizeof(k_info))) return -EFAULT;
// 2. Process data
k_info.data *= 2;
// 3. Copy result back to user
if (copy_to_user((void*)arg, &k_info, sizeof(k_info))) return -EFAULT;
}
return 0;
}
static const struct proc_ops my_fops = { .proc_ioctl = my_ioctl };
static int __init my_init(void) {
proc_create("test_proc", 0666, NULL, &my_fops);
return 0;
}
static void __exit my_exit(void) { remove_proc_entry("test_proc", NULL); }
module_init(my_init); module_exit(my_exit);
MODULE_LICENSE("GPL");
3. Report Assignment
3.1 Report Format
The report consists of a preliminary research section and a programming results section. The title of the report
should be "SP Assignment 2 Report". Other formatting requirements are the same as the previous assignment.
3.2 Report Contents
• Preliminary Research Section
o Comparative Analysis of Linux Kernel Code: Compare the code of the following two versions:
 Linux Kernel version 2.6.39
 Linux Kernel version 6.14
o Memory Management Analysis: Compare and analyze the actual code related to memory management
techniques in each version.
 Multi-level paging: Analyze the relationship between Page Global Directory (PGD), Page Upper
Directory (PUD), Page Middle Directory (PMD), and Page Table Entry (PTE).
 Explain the relationship between Virtual Address and Physical Address.
 Explain the mechanism flows of related data structures, macros, and functions for each item.
 kmalloc / vmalloc / malloc: Research each allocation method and compare them from the perspective
of physical memory mapping (continuity, allocation timing).
 Investigate how a kernel with high-level paging supports lower-level paging.
o Focus on the commonalities and differences between the two kernel versions.
o Use flowcharts or diagrams to organize the information (3 or more).
o References
• Programming Results Section
o The overall structure and operation process of the created kernel modules.
 Must use diagrams such as flowcharts or block diagrams.
 Focus on the overall flow rather than detailed code explanations.
o Specify the development environment.
 uname -a command output
 Compiler version, memory information, etc.
o Result Analysis
 Capture and attach the output logs of the kernel module.
 Provide an explanation for the output logs for each case.
o Difficulties encountered during the assignment and how you resolved.
4. Programming Assignment
The implementation assignment involves writing one kernel module. The preparation requirements are the same as
the previous assignment.
4.1 malloc_monitor
The malloc_monitor module implemented in this assignment tracks the operation of mmap executed in user space and
kmalloc executed in kernel space, outputting information on the mapped virtual and physical addresses at each point
in time. The goal is not only to show the simple address translation result but also to verify the Linux paging
mechanism visually by recording all intermediate steps of Page Table Walking.
Generally, automatically detecting memory allocation of user processes at arbitrary times requires advanced kernel
techniques like system call hooking or kprobes, but this is out of the scope of this assignment. Therefore, in this
assignment, we assume that the user program explicitly sends a signal (ioctl) to the kernel module at the moment of
memory allocation and access, as shown in the test program.
The kernel module records the memory state whenever it receives this signal. Later, when the kernel module is read,
it prints the recorded memory information. The test program used for grading is also written in a format that sends
signals after memory allocation/access, similar to the example below. To ensure smooth grading, you must use the
IOCTL magic number and commands defined in the provided header file (mem_ioctl.h).
The proc file name created by the kernel module must be "malloc_monitor".
In C/C++, malloc is the standard way to allocate memory. However, since malloc reuses space from a pre-allocated
heap memory pool, it is difficult to observe Demand Paging. Therefore, this assignment assumes that memory
allocation in user code is performed via the mmap function.
// Example of user code
/* 1. User space memory allocation */
char *ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
MAP_PRIVATE | MAP_ANONYMOUS,
-1, 0); // Allocate new memory space
ioctl(fd, CMD_MALLOC, ptr); // Send signal to kernel module
/* 2. User space memory access */
ptr[0] = 'A'; // Memory write
ioctl(fd, CMD_ACCESS, ptr); // Send signal to kernel module
/* 3. Kerel space memory allocation */
ioctl(fd, CMD_KMALLOC, 0); // Send signal to kernel module
• You can check the desired information from the terminal with the following command:
$ cat /proc/malloc_monitor
• This module creates the /proc/malloc_monitor file and records the page table status for 3 memory events.
The kernel module must handle branches based on the command received from the user program as follows:
• Case 1: User Malloc (CMD_MALLOC, Type: MALLOC)
o Goal: Verify the kernel's paging status for the virtual address immediately after memory allocation
(mmap) in user space.
o Requirements:
 Traverse the page table for the received virtual address and check if physical memory
mapping exists.
 Traverse the 4-level page table (PGD -> PUD -> PMD -> PTE) and store the address and
value of the entry at each level.
 If physical memory is not mapped, store information up to the mapped page table level and
fill the subsequent levels with 0.
 For unmapped physical addresses, print "(Not Mapped)".
• Case 2: User Access (CMD_ACCESS, Type: ACCESS)
o Goal: Verify how the paging status changes after an actual data write operation occurs in the
allocated memory in user space.
o Requirements:
 Similar to Case 1, traverse the page table for the received virtual address and check/record
the physical memory mapping.
• Case 3: Kernel Alloc (CMD_KMALLOC, Type: KMALLOC)
o Goal: Check how kernel space memory allocation (kmalloc) differs from user space.
o Requirements:
 Directly allocate memory of an arbitrary size (e.g., 4KB) inside the malloc_monitor module.
 Store the mapped physical address for the allocated virtual address.
 The allocated memory must be properly freed after all operations are done.
 Note: The output format for KMALLOC differs from other cases.
• The output format and an example are as follows.
o First, print the basic information at the top.
 Student ID, Name
o Then, print the recorded history sequentially according to the format below.
o Be careful about the output format, as deductions may occur if it differs.
 The number of '=' characters between the basic information and the process information is
60.
o The values in the example are dummy data.
ID: 2025123456
Name: Hong, Gildong
============================================================
[1] PID: 1234 | Type: MALLOC
------------------------------------------------------------
Target VA : 0x00007f8a1000
PGD : Addr = 0xffff88800a10, Val = 0x000000001b200067
PUD : Addr = 0xffff88800b20, Val = 0x000000001c300067
PMD : Addr = 0xffff88800c30, Val = 0x0000000000000000
PTE : Addr = 0x000000000000, Val = 0x0000000000000000
Final PA : (Not Mapped)
============================================================
[2] PID: 1234 | Type: ACCESS
------------------------------------------------------------
Target VA : 0x00007f8a1000
PGD : Addr = 0xffff88800a10, Val = 0x000000001b200067
PUD : Addr = 0xffff88800b20, Val = 0x000000001c300067
PMD : Addr = 0xffff88800c30, Val = 0x000000001d400067
PTE : Addr = 0xffff88800d40, Val = 0x8000000012a00067
Final PA : 0x0000000012a000
============================================================
[3] PID: 1234 | Type: KMALLOC
------------------------------------------------------------
Target VA : 0xffff88800abc0000
Final PA : 0x00000000abc000
============================================================
4.2 Programming Assignment Compilation Requirements
You must use one Makefile to compile both parts. That is, when the build is initiated with the make command, the
kernel module files malloc_monitor.ko should be created in the same directory where the command was executed.
make clean should delete all files generated by the build. The created kernel modules must be registerable with the
sudo insmod command and removable with the sudo rmmod command.
(example)
os@ubuntu:~$ cd 2025123123
os@ubuntu:~/2025123123$ make
os@ubuntu:~/2025123123$ ls
Makefile malloc_monitor.ko …
os@ubuntu:~/2025123123$ make clean
Makefile …
5. Assignment Submission
You must submit two items: 1) A report file named aer your student ID, converted to PDF (e.g.,
hw2_2025123123.pd) and 2) A source code archive (tar file) named aer your student ID. Regarding the report
file, e filename must be exactly your student ID, and it must be in PDF format. Regarding the source code, you
must create archive file using the commands below. No other compression format or double compression is
allowed and does not include your PDF report inside this tar file.
# Assume you are working in the directory ~/hw2, and that your Makefile is in ~/hw2/.
(vi ~/hw2/Makefile)
# Suppose your student ID is 2025123123. Aer you finish everything, run the following commands to package
your work and submit hw2_2025123123.tar.
# is means that when hw2_2025123123.tar is extracted, there should be exactly one folder named
2025123123. Inside it, the Makefile should be directly visible (no extra subdirectories).
$ cd ~/hw2/..
$ mv hw2 2025123123
$ tar cvf hw2_2025123123.tar 2025123123
6. Precautions
 If the assignment does not compile in the grading environment, the programming part will receive 0 points,
so pay attention to the Linux distribution version, file encoding, kernel version, etc.
 The number of CPUs will be arbitrarily set to 2 or more, so the code must be able to run in various CPU
environments.
 No leniency is given for unreasonable delay—late submissions are not accepted.
 All references (including images and code snippets) must be cited (lack of citation results in point
deductions).
 If you copy or modify another student’s work, both parties get zero points.
 Failure to follow the submission file generation instructions or to match the exact structure (especially
regarding filenames) may lead to point deductions or a zero score.
 Add comments to your program codes (If not, the points will be deducted).
 Submissions that fail to compile receive zero points.
 For assignment-related questions, post a public inquiry on the LearnUs Q&A board.