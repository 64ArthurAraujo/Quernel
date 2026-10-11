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
    
    ata_drive_t *drive = get_ata_drive(0);
    ata_read_28bit_pio(drive, 0);
    
    asm volatile("hlt");
}
