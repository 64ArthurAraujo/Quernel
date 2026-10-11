#include "../drivers/include/tty.h"
#include "../cpu/include/isr.h"
#include "../cpu/include/timer.h"
#include "../drivers/include/keyboard.h"
#include "../libc/include/memory.h"
#include "../drivers/include/ata.h"
#include "../cpu/include/cpuid.h"
#include "../cpu/include/ports.h"
#include <stdint.h>

void read_ata_drive(int index)
{
    ata_drive_t *drive = get_ata_drive(index);

    for (uint32_t i = 0; i < drive->sectors; i++) 
    {
        uint16_t buffer[256];
        ata_read_28bit_pio(buffer, drive, i);

        hexdump((uint8_t *) buffer, 512); 
    }

    uint16_t buffer[256];
    ata_read_28bit_pio(buffer, drive, 0);

    hexdump((uint8_t *) buffer, 512);
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
    
    asm volatile("hlt");
}
