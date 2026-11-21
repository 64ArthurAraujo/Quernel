#include <quernel/printk.h>
#include <quernel/config.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <drivers/tty.h>
#include <quernel/dev.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>

#include "do_mounts.h"
#include "showcases.h"
#include "qtest/tests.h"

#include <fs/proc.h>
#include <fs/ext.h>

#define QUERNEL_BANNER "Quernel version 0.0.0.i686\n"

static root_dev sda = {0};

void init_memory()
{
    pmm_init();
    vmm_init();
    heap_init(HEAP_START_ADDR, HEAP_SIZE);
}

void init_kernel()
{
    isr_setup();
    asm volatile("sti");

    init_timer(100); // 100 Hz
    init_memory();
    printk(QUERNEL_BANNER);
    cpuid_detect();
    ata_detect_drives();
    do_mount_root(&sda);

    ext2_t *root_fs = (ext2_t *)sda.volumes[vol(3)].fs;

    proc_write_file(root_fs, "/proc/version", "version", QUERNEL_BANNER);

#if QUER_QTEST
    test_create_find_dir(sda);
    sleep(5000);
    test_create_many_folders(sda);

    pr_info("QTEST: finished all tests!\n");
#endif

    asm volatile("hlt");
}
