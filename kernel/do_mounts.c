#include "../drivers/ata.h"
#include "../libc/string.h"
#include "../fs/gpt.h"
#include "../fs/fat.h"
#include "../drivers/tty.h"
#include "../kernel/printk/printk.h"
#include <stdint.h>

void do_mount_root()
{
    ata_drive_t *drives = get_ata_drives();
    static partition_t *part;

    static fat12_t fat12_fs;

    for (int drive_i = 0; drive_i < 4; drive_i++)
    {
        if (drives[drive_i].present == 1)
        {
            printk("ATA: %s\n", (char *)drives[drive_i].model);

            gpt_part_t gp[8];
            int r = gpt_read(&drives[drive_i], gp, 8);

            if (r < 0)
            {
                printk("GPT read failed!");
                return;
            }

            if (r > 8) r = 8;

            for (int i = 0; i < r; i++)
            {
                part->drive = &drives[drive_i];
                part->start = gp[i].start;
                part->count = gp[i].count;
                
                int res = fat12_mount(&fat12_fs, part);   

                if (res == 0) 
                {
                    printk("Doing an LS in the found FAT12 partition\n");
                    fat12_ls(&fat12_fs, 0);

                    break; // we found the first mountable partition, so leave for now
                }
            }
        }
    }
}