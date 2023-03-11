#include "../drivers/include/screen.h"
#include "../cpu/include/isr.h"
#include "../cpu/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../libc/include/memory.h"

void init_kernel()
{
    isr_setup();
    asm volatile("sti");
    init_keyboard();

    kprint("Quernel successfully started!\n");
}
