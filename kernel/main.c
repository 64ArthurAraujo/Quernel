#include "../drivers/include/screen.h"
#include "../cpu/include/isr.h"
#include "../cpu/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../libc/include/memory.h"
#include "../drivers/include/ata.h"
#include "../cpu/include/cpuid.h"

void init_kernel()
{
    isr_setup();
    asm volatile("sti");
    init_keyboard();

    cpuid_detect();
    ata_detect_drives();
}
