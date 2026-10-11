#include "../drivers/include/tty.h"
#include "../cpu/include/isr.h"
#include "../cpu/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../libc/include/memory.h"
#include "../drivers/include/ata.h"
#include "../cpu/include/cpuid.h"
#include "../cpu/include/ports.h"
#include <stdint.h>

void list_drives()
{
    ata_drive_t *drives = get_ata_drives();

    for (int i = 0; i < 4; i++)
    {
        if (drives[i].present == 1)
        {
            kprint("ATA: ");
            kprintln((char *) drives[i].model);
        }
    }
}

void init_kernel()
{
    isr_setup();
    asm volatile("sti");

    // unmask only keyboard
    port_byte_out(0x21, 0xFD);

    init_keyboard();

    cpuid_detect();
    ata_detect_drives();

    list_drives();
    
    asm volatile("hlt");
}
