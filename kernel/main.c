#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../libc/memory.h"
#include "../cpu/cpuid.h"
#include "../cpu/ports.h"
#include "do_mounts.h"
#include <quernel/printk.h>
#include <drivers/ata.h>
#include <stdint.h>

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
