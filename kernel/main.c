#include "../drivers/tty.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"
#include "../libc/memory.h"
#include "../cpu/cpuid.h"
#include "../cpu/ports.h"
#include "../drivers/ata.h"
#include "do_mounts.h"
#include "printk/printk.h"
#include <stdint.h>

void init_kernel()
{
    isr_setup();
    asm volatile("sti");

    // unmask only keyboard
    port_byte_out(0x21, 0xFD);

    init_keyboard();

    printk("Quernel version 0.0.0.i686\n");

    cpuid_detect();
    ata_detect_drives();

    do_mount_root();

    asm volatile("hlt");
}
