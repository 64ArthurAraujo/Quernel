#include "../drivers/tty.h"
#include "../cpu/isr.h"
#include "../cpu/timer.h"
#include "../drivers/keyboard.h"
#include "../libc/memory.h"
#include "../drivers/ata.h"
#include "../cpu/cpuid.h"
#include "../cpu/ports.h"
#include "../block/gpt.h"
#include "../libc/string.h"
#include <stdint.h>

void list_drives()
{
    ata_drive_t *drives = get_ata_drives();
    static partition_t part;

    for (int drive_i = 0; drive_i < 4; drive_i++)
    {
        if (drives[drive_i].present == 1)
        {
            kprint("ATA: ");
            kprintln((char *)drives[drive_i].model);

            gpt_part_t gp[8];
            int r = gpt_read(&drives[drive_i], gp, 8);

            if (r < 0)
            {
                kprintln("GPT read failed!");
                return;
            }

            if (r > 8)
                r = 8;

            for (int i = 0; i < r; i++)
            {
                part.drive = &drives[drive_i];
                part.start = gp[i].start;
                part.count = gp[i].count;

                char i_str[12];
                char start[12];
                char count[12];

                int_to_ascii(i, i_str);
                u32_to_str(part.start, start);
                u32_to_str(part.count, count);

                kprint("I = ");
                kprintln(i_str);
                kprint("Start = ");
                kprintln(start);
                kprint("Count = ");
                kprintln(count);
            }
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
