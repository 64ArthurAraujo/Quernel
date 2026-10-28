#include <quernel/printk.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <quernel/dev.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>

#include "do_mounts.h"

#include <fs/ext.h>

static root_dev sda = {0};

void init_kernel()
{
    isr_setup();
    asm volatile("sti");

    printk("Quernel version 0.0.0.i686\n");

    cpuid_detect();
    ata_detect_drives();
    do_mount_root(&sda);

    asm volatile("hlt");
}
