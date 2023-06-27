#include "../drivers/include/screen.h"
#include "../cpu/include/isr.h"
#include "../cpu/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../libc/include/memory.h"

/* Differently named so it doest overlap with other domains */
#define ROOT_DOMAIN "QUERNEL"

void init_kernel()
{
    isr_setup();
    asm volatile("sti");
    init_keyboard();

    klog("Started", ROOT_DOMAIN);
}
