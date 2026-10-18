#include <quernel/printk.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <stdint.h>
#include <memory.h>

#include "do_mounts.h"

void init_kernel()
{
    isr_setup();
    asm volatile("sti");

    printk("Quernel version 0.0.0.i686\n");

    cpuid_detect();
    ata_detect_drives();
    do_mount_root();

    asm volatile("hlt");
}
