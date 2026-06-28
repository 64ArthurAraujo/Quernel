#include "../drivers/include/tty.h"
#include "../cpu/include/isr.h"
#include "../cpu/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../libc/include/memory.h"
#include "../drivers/include/ata.h"
#include "../cpu/include/cpuid.h"
#include "../cpu/include/ports.h"

void init_kernel()
{
    isr_setup();


    asm volatile("sti");
    
    // unmask only keyboard
    port_byte_out(0x21, 0xFD);

    init_keyboard();

    cpuid_detect();
    ata_detect_drives();

    kprintln("Halting");

}
