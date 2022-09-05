#include "../drivers/include/screen.h"
#include "../cpu/include/isr.h"
#include "../cpu/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../libc/include/memory.h"

void init_kernel()
{
    kprint("Starting Quernel... \n");

    isr_setup();

    asm volatile("sti");
    // init_timer(50);

    clear_screen();

    kprint("Welcome to the Quernel! \nYou are now on the kernel shell \n");
    kprint("Type HELP to list the possible commands \n \n");

    init_keyboard();
}
