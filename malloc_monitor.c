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

// 메모리 할당 및 접근 이력을 저장하기 위한 Linked List 기반 노드 구조체
struct monitor_entry {
    struct list_head list; // Linked List 노드
    int pid; // Process ID
    char type[10]; // 요청 타입
    unsigned long target_val; // 분석 대상 주소
    
    // 4단계 페이지 테이블의 각 단계별 주소 및 값
    unsigned long pgd_add, pgd_val, pud_add, pud_val, pmd_add, pmd_val, pte_add, pte_val, final_pa;
    
    // 각 단계의 페이지 테이블이 존재하는지 여부를 표시하는 변수
    int has_pgd, has_pud, has_pmd, has_pte, has_pa;
};

// 이력 데이터를 저장할 리스트 헤드 초기화
static LIST_HEAD(history_list);

// 다중 프로세스 환경에서 리스트 접근 시 동기화를 위한 스핀락 설정
static DEFINE_SPINLOCK(history_lock);

// 함수 프로토타입 선언
static long monitor_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static int monitor_show(struct seq_file *m, void *v);
static int monitor_open(struct inode *inode, struct file *file);

// Proc 파일 시스템 연산 정의 구조체
static const struct proc_ops monitor_fops = {
    .proc_open = monitor_open,
    .proc_read = seq_read, // seq_file 인터페이스 사용
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
    .proc_ioctl = monitor_ioctl, // 유저 공간으로부터 명령을 받는 핸들러
};

// 저장된 모든 이력을 삭제하고 메모리를 해제
static void clear_history(void) {
    struct monitor_entry *tmp, *entry; // 리스트 조작 전 락을 획득함
    spin_lock(&history_lock);
    list_for_each_entry_safe(entry, tmp, &history_list, list) {
        list_del(&entry->list); // 리스트에서 제거
        kfree(entry); // 할당된 메모리 해제
    }
    spin_unlock(&history_lock); // 락 해제
}

// 모듈 초기화 함수
static int __init init_monitor(void) {
    struct proc_dir_entry *entry;
    // proc 폴더에 파일을 생성함
    entry = proc_create(PROCFS_NAME, 0666, NULL, &monitor_fops);
    if (!entry) {
        return -ENOMEM;
    }
    return 0;
}

// 모듈 종료 함수
static void __exit exit_monitor(void) {
    remove_proc_entry(PROCFS_NAME, NULL);
    clear_history();
}

// open 시스템 콜 핸들러
static int monitor_open(struct inode *inode, struct file *file) {
    return single_open(file, monitor_show, NULL); // monitor_show 함수와 single_open 함수를 연결함
}

// proc/malloc_monitor 를 조회할 때 호출되는 함수
static int monitor_show(struct seq_file *m, void *v) {
    struct monitor_entry *entry;
    int count = 1;

    // 학번, 이름 출력
    seq_printf(m, "ID: %s\n", STUDENT_ID);
    seq_printf(m, "Name: %s\n\n", STUDENT_NAME);
    seq_printf(m, "============================================================\n");

    // 리스트 읽기 시작하여 스핀락 설정
    spin_lock(&history_lock);
    // 저장된 리스트를 모두 순회
    list_for_each_entry(entry, &history_list, list) {
        seq_printf(m, "[%d] PID: %d | Type: %s\n", count++, entry->pid, entry->type);
        seq_printf(m, "------------------------------------------------------------\n");
        seq_printf(m, "Target VA : 0x%lx\n", entry->target_val);

        if (strcmp(entry->type, "KMALLOC") == 0) {
            // KMALLOC의 경우 단순 가상 -> 물리 주소 변환 결과만 출력함
            seq_printf(m, "Final PA : 0x%lx\n", entry->final_pa);
        } else {
            // MALLOC, ACCESS의 경우 4단계 페이지 테이블 정보 모두 출력
            seq_printf(m, "PGD : Addr = 0x%lx, Val = 0x%lx\n", entry->pgd_add, entry->pgd_val);
            seq_printf(m, "PUD : Addr = 0x%lx, Val = 0x%lx\n", entry->pud_add, entry->pud_val);
            seq_printf(m, "PMD : Addr = 0x%lx, Val = 0x%lx\n", entry->pmd_add, entry->pmd_val);
            seq_printf(m, "PTE : Addr = 0x%lx, Val = 0x%lx\n", entry->pte_add, entry->pte_val);
            
            // 실제 물리 메모리 매핑 여부에 따른 결과 출력
            if (entry->has_pa)
                seq_printf(m, "Final PA : 0x%lx\n", entry->final_pa);
            else
                seq_printf(m, "Final PA : (Not Mapped)\n");
        }
        seq_printf(m, "============================================================\n");
    }
    spin_unlock(&history_lock); // 리스트 읽기를 종료하여 스핀락 해제

    return 0;
}

// 사용자 가상 주소에 대한 페이지 테이블 탐색 함수
static void walk_user_page_table(unsigned long va, struct monitor_entry *entry) {
    pgd_t *pgd;
    p4d_t *p4d;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct mm_struct *mm = current->mm; // 현제 프로세스의 메모리 구조체

    if (!mm) {
        return;
    }
    // 페이지 테이블 탐색을 위해 읽기 락 획득
    mmap_read_lock(mm);

    // PGD 탐색
    pgd = pgd_offset(mm, va);
    entry->pgd_add = (unsigned long)pgd;
    entry->pgd_val = pgd_val(*pgd);
    entry->has_pgd = 1;

    // 만약 PGD가 비어있거나 잘못되었다면 중단함
    if (pgd_none(*pgd) || pgd_bad(*pgd)) {
        goto out;
    }

    // P4D 탐색
    p4d = p4d_offset(pgd, va);
 
    // PUD 탐색
    pud = pud_offset(p4d, va);
    entry->pud_add = (unsigned long)pud;
    entry->pud_val = pud_val(*pud);
    entry->has_pud = 1;

    if (pud_none(*pud) || pud_bad(*pud)) {
        goto out;
    }

    // PMD 탐색
    pmd = pmd_offset(pud, va);
    entry->pmd_add = (unsigned long)pmd;
    entry->pmd_val = pmd_val(*pmd);
    entry->has_pmd = 1;

    if (pmd_none(*pmd) || pmd_bad(*pmd)) {
        goto out;
    }

    // PTE 탐색
    pte = pte_offset_kernel(pmd, va); // 커널 공간에서 PTE의 가상 주소를 가져옴
    if (!pte) {
        goto out;
    }

    entry->pte_add = (unsigned long)pte;
    entry->pte_val = pte_val(*pte);
    entry->has_pte = 1;

    // 물리 주소를 계산하기 위해 pte_present를 사용하여 해당 페이지가 실제 물리 메모리에 로드되어 있는지 확인함
    if (pte_present(*pte)) {
        // PFN을 추출하여 시프트 하고 오프셋을 더해 최종 물리 주소를 계산함
        entry->final_pa = (pte_pfn(*pte) << PAGE_SHIFT) | (va & ~PAGE_MASK);
        entry->has_pa = 1; // 매핑됨을 표시
    }

out:
    mmap_read_unlock(mm); // 락 해제
}

// IOCTL 명령어 처리 함수
static long monitor_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    struct monitor_entry *entry;
    void *kptr;

    // 결과를 저장할 구조체 메모리 할당
    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    if (!entry) {
        return -ENOMEM;
    }
    memset(entry, 0, sizeof(*entry));
    entry->pid = current->pid; // 현재 프로세스 ID 저장

    switch (cmd) {
        case CMD_MALLOC:
            strcpy(entry->type, "MALLOC");
            entry->target_val = arg; // 유저가 보낸 가상 주소
            // 유저 공간 페이지 테이블을 탐색함
            walk_user_page_table(entry->target_val, entry);
            break;

        case CMD_ACCESS:
            strcpy(entry->type, "ACCESS");
            entry->target_val = arg; // 유저가 보낸 가상 주소
            // 유저 공간 페이지 테이블을 탐색함
            walk_user_page_table(entry->target_val, entry);
            break;

        case CMD_KMALLOC:
            strcpy(entry->type, "KMALLOC");
            // 커널 공간에서 4KB 할당 테스트
            kptr = kmalloc(4096, GFP_KERNEL);
            if (kptr) {
                entry->target_val = (unsigned long)kptr;
                // 커널 논리 주소를 물리 주소로 즉시 반환
                entry->final_pa = virt_to_phys(kptr);
                entry->has_pa = 1;
                // 주소 확인만 하고 메모리는 바로 해제
                kfree(kptr);
            }
            break;

        default:
            kfree(entry); // 잘못된 명령어일 경우 할당을 취소
            return -EINVAL;
    }

    spin_lock(&history_lock); // 락 획득
    list_add_tail(&entry->list, &history_list); // 결과를 리스트에 추가함
    spin_unlock(&history_lock); // 락 해제

    return 0;
}

module_init(init_monitor);
module_exit(exit_monitor);
