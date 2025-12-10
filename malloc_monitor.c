#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include "mem_ioctl.h"

#define PROCFS_NAME "malloc_monitor"
#define STUDENT_ID "2024148005"
#define STUDENT_NAME "JEON Hyunwoo"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JEON Hyunwoo");
MODULE_DESCRIPTION("Memory Allocation Monitor for SP Assignment 2");

// Data structure to store history
struct monitor_entry {
    struct list_head list;
    int pid;
    char type[10]; // "MALLOC", "ACCESS", "KMALLOC"
    unsigned long target_va;
    
    // Page Table Infos
    unsigned long pgd_addr, pgd_val;
    unsigned long pud_addr, pud_val;
    unsigned long pmd_addr, pmd_val;
    unsigned long pte_addr, pte_val;
    unsigned long final_pa;
    
    // Flags to indicate which levels exist
    int has_pgd;
    int has_pud;
    int has_pmd;
    int has_pte;
    int has_pa;
};

static LIST_HEAD(history_list);
static DEFINE_SPINLOCK(history_lock);

static long monitor_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int monitor_show(struct seq_file *m, void *v);
static int monitor_open(struct inode *inode, struct file *file);

static const struct proc_ops monitor_fops = {
    .proc_open = monitor_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
    .proc_ioctl = monitor_ioctl,
};

static void clear_history(void) {
    struct monitor_entry *entry, *tmp;
    spin_lock(&history_lock);
    list_for_each_entry_safe(entry, tmp, &history_list, list) {
        list_del(&entry->list);
        kfree(entry);
    }
    spin_unlock(&history_lock);
}

static int __init monitor_init(void) {
    struct proc_dir_entry *entry;
    entry = proc_create(PROCFS_NAME, 0666, NULL, &monitor_fops);
    if (!entry) {
        return -ENOMEM;
    }
    printk(KERN_INFO "malloc_monitor: module loaded\n");
    return 0;
}

static void __exit monitor_exit(void) {
    remove_proc_entry(PROCFS_NAME, NULL);
    clear_history();
    printk(KERN_INFO "malloc_monitor: module unloaded\n");
}

static int monitor_open(struct inode *inode, struct file *file) {
    return single_open(file, monitor_show, NULL);
}

static int monitor_show(struct seq_file *m, void *v) {
    struct monitor_entry *entry;
    int count = 1;

    seq_printf(m, "ID: %s\n", STUDENT_ID);
    seq_printf(m, "Name: %s\n", STUDENT_NAME);
    seq_printf(m, "============================================================\n");

    spin_lock(&history_lock);
    list_for_each_entry(entry, &history_list, list) {
        seq_printf(m, "[%d] PID: %d | Type: %s\n", count++, entry->pid, entry->type);
        seq_printf(m, "------------------------------------------------------------\n");
        seq_printf(m, "Target VA : 0x%012lx\n", entry->target_va);

        if (strcmp(entry->type, "KMALLOC") == 0) {
            seq_printf(m, "Final PA : 0x%012lx\n", entry->final_pa);
        } else {
            seq_printf(m, "PGD : Addr = 0x%012lx, Val = 0x%016lx\n", entry->pgd_addr, entry->pgd_val);
            seq_printf(m, "PUD : Addr = 0x%012lx, Val = 0x%016lx\n", entry->pud_addr, entry->pud_val);
            seq_printf(m, "PMD : Addr = 0x%012lx, Val = 0x%016lx\n", entry->pmd_addr, entry->pmd_val);
            seq_printf(m, "PTE : Addr = 0x%012lx, Val = 0x%016lx\n", entry->pte_addr, entry->pte_val);
            
            if (entry->has_pa)
                seq_printf(m, "Final PA : 0x%012lx\n", entry->final_pa);
            else
                seq_printf(m, "Final PA : (Not Mapped)\n");
        }
        seq_printf(m, "============================================================\n");
    }
    spin_unlock(&history_lock);

    return 0;
}

static void walk_user_page_table(unsigned long va, struct monitor_entry *entry) {
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct mm_struct *mm = current->mm;

    if (!mm) return;
    
    // We should hold mmap_read_lock, but in ioctl context it's tricky if we might sleep or if we want to be safe.
    // However, usually we should take the lock.
    mmap_read_lock(mm);

    pgd = pgd_offset(mm, va);
    entry->pgd_addr = (unsigned long)pgd;
    entry->pgd_val = pgd_val(*pgd);
    entry->has_pgd = 1;

    if (pgd_none(*pgd) || pgd_bad(*pgd)) goto out;

    p4d = p4d_offset(pgd, va);
    // Assuming 4-level paging, P4D is folded into PGD usually or similar.
    // Use p4d logic if kernel supports 5-level, but assignment said 4-level.
    // On 4-level, p4d_offset just returns pgd.
    
    pud = pud_offset(p4d, va);
    entry->pud_addr = (unsigned long)pud;
    entry->pud_val = pud_val(*pud);
    entry->has_pud = 1;

    if (pud_none(*pud) || pud_bad(*pud)) goto out;

    pmd = pmd_offset(pud, va);
    entry->pmd_addr = (unsigned long)pmd;
    entry->pmd_val = pmd_val(*pmd);
    entry->has_pmd = 1;

    if (pmd_none(*pmd) || pmd_bad(*pmd)) goto out;

    pte = pte_offset_kernel(pmd, va);
    if (!pte) goto out;

    entry->pte_addr = (unsigned long)pte;
    entry->pte_val = pte_val(*pte);
    entry->has_pte = 1;

    if (pte_present(*pte)) {
        entry->final_pa = (pte_pfn(*pte) << PAGE_SHIFT) | (va & ~PAGE_MASK);
        entry->has_pa = 1;
    }
    // pte_unmap(pte); // Not needed for pte_offset_kernel

out:
    mmap_read_unlock(mm);
}

static long monitor_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct monitor_entry *entry;
    void *kptr;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) return -ENOMEM;
    memset(entry, 0, sizeof(*entry));
    entry->pid = current->pid;

    switch (cmd) {
        case CMD_MALLOC:
            strcpy(entry->type, "MALLOC");
            entry->target_va = arg;
            walk_user_page_table(entry->target_va, entry);
            break;

        case CMD_ACCESS:
            strcpy(entry->type, "ACCESS");
            entry->target_va = arg;
            walk_user_page_table(entry->target_va, entry);
            break;

        case CMD_KMALLOC:
            strcpy(entry->type, "KMALLOC");
            kptr = kmalloc(4096, GFP_KERNEL);
            if (kptr) {
                entry->target_va = (unsigned long)kptr;
                entry->final_pa = virt_to_phys(kptr);
                entry->has_pa = 1;
                kfree(kptr);
            }
            break;

        default:
            kfree(entry);
            return -EINVAL;
    }

    spin_lock(&history_lock);
    list_add_tail(&entry->list, &history_list);
    spin_unlock(&history_lock);

    return 0;
}

module_init(monitor_init);
module_exit(monitor_exit);
