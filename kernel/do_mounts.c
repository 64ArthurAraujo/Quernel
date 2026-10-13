#include "../drivers/ata.h"
#include "../libc/string.h"
#include "../fs/gpt.h"
#include "../fs/fat.h"
#include "../drivers/tty.h"
#include <stdint.h>

void kprint_partition(partition_t *part, int i)
{
    char i_str[12];
    char start[12];
    char count[12];

    int_to_ascii(i, i_str);
    u32_to_str(part->start, start);
    u32_to_str(part->count, count);

    kprint("Partition ");
    kprintln(i_str);

    kprint("Start at ");
    kprintln(start);
    kprint("Size of ");
    kprintln(count);
    kprintln("");
}

void do_mount_root()
{
    ata_drive_t *drives = get_ata_drives();
    static partition_t *part;

    static fat12_t fat12_fs;

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

            if (r > 8) r = 8;

            for (int i = 0; i < r; i++)
            {
                part->drive = &drives[drive_i];
                part->start = gp[i].start;
                part->count = gp[i].count;

                kprint_partition(part, i);
                
                
                int res = fat12_mount(&fat12_fs, part);   

                if (res == 0) 
                {
                    kprintln("Mounted fat12 filesys!");
                }
            }
        }
    }
}