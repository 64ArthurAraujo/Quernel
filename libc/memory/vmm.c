
/*
 *  libc/memory/vmm.c & vmm.h
 *
 *  Implementations taken from MyraOS (https://github.com/dvir-biton/MyraOS/blob/main/system/memory/src/vmm/vmm.c)
 */
#include <quernel/printk.h>
#include <memory.h>
#include <stdint.h>

#include "pmm.h"
#include "vmm.h"

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

#define KERNEL_SIZE_IN_PAGES (((&_kernel_end - &_kernel_start) + PAGE_SIZE - 1) / PAGE_SIZE)

__attribute__((section(".boot.text"))) static void load_page_directory(uint32_t page_dir)
{
    asm volatile(
        "movl %0, %%cr3"
        :
        : "r"(page_dir)
        : "memory");
}

uint32_t *kernel_page_dir_phys;
page_directory_t *current_page_dir_virt;

void vmm_init(void)
{
    asm volatile("mov %%cr3, %0" : "=r"(kernel_page_dir_phys));
    current_page_dir_virt = (page_directory_t *)RECURSIVE_PAGE_DIRECTORY_BASE;
}

void vmm_map_page(uint32_t vir_addr, uint32_t phys_addr, uint32_t flags)
{
    uint32_t dir_index = (vir_addr >> 22) & 0x3FF;
    uint32_t table_index = (vir_addr >> 12) & 0x3FF;

    if (current_page_dir_virt->entries[dir_index] == 0)
    {
        page_table_t *new_page_table = (page_table_t *) pmm_alloc_page();
        if (new_page_table == NULL)
        {
            pr_err("Failed to create a page table! Out of memory!");
        }

        if (flags & PAGE_USER)
        {
            current_page_dir_virt->entries[dir_index] = MAKE_ENTRY(new_page_table, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
        }
        else 
        {
            current_page_dir_virt->entries[dir_index] = MAKE_ENTRY(new_page_table, PAGE_PRESENT | PAGE_WRITE);
        }

        new_page_table = (page_table_t *)((RECURSIVE_PAGE_TABLE_BASE) + (dir_index * PAGE_SIZE));
        memset(new_page_table, 0, PAGE_SIZE);

        asm volatile("invlpg (%0)" :: "r"(new_page_table));
    }

    page_table_t *page_table = (page_table_t *)(RECURSIVE_PAGE_TABLE_BASE + dir_index * PAGE_SIZE);
    page_table->entries[table_index] = MAKE_ENTRY(phys_addr, flags);

    asm volatile("invlpg (%0)" :: "r" (vir_addr) : "memory");
}

void vmm_unmap_page(uint32_t vir_addr)
{
    uint32_t dir_index = (vir_addr >> 22) & 0x3FF;
    uint32_t table_index = (vir_addr >> 12) & 0x3FF;

    page_table_t *page_table = (page_table_t *)(RECURSIVE_PAGE_TABLE_BASE + dir_index * PAGE_SIZE);
    page_table->entries[table_index] = 0;

    asm volatile("invlpg (%0)" :: "r"(vir_addr) : "memory");
}

uint32_t vmm_virt_to_phys(uint32_t vir_addr)
{
    uint32_t pd_index = (vir_addr) >> 22;
    uint32_t pt_index = ((vir_addr) >> 12) & 0x3FF;

    uint32_t *pt = (uint32_t *)(0xFFC00000 + (pd_index << 12));

    return (pt[pt_index] & ~0xFFF) | ((vir_addr) & 0xFFF);
}