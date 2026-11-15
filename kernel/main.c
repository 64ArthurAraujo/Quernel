#include <quernel/printk.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <quernel/dev.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>

#include "do_mounts.h"
#include "showcases.h"

#include <fs/ext.h>

static root_dev sda = {0};

void init_memory()
{
    pmm_init();
    vmm_init();
    heap_init(HEAP_START_ADDR, HEAP_SIZE);
}

void init_kernel()
{
    printk("Quernel version 0.0.0.i686\n");

    isr_setup();
    asm volatile("sti");

    init_timer(100); // 100 Hz
    init_memory();
    cpuid_detect();
    ata_detect_drives();
    do_mount_root(&sda);

    sleep(250);

    create_and_find_file(sda);

    asm volatile("hlt");
}
