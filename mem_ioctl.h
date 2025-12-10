#ifndef MEM_IOCTL_H
#define MEM_IOCTL_H

#include <linux/ioctl.h>

#define MEM_MAGIC 'm'

#define CMD_MALLOC  _IOW(MEM_MAGIC, 1, unsigned long)
#define CMD_ACCESS  _IOW(MEM_MAGIC, 2, unsigned long)
#define CMD_KMALLOC _IO(MEM_MAGIC, 3)

#endif
