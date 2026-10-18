#include <string.h>
#include "../fs/gpt.h"
#include "../fs/fat12/fat.h"
#include <quernel/printk.h>
#include <drivers/tty.h>
#include <drivers/ata.h>
#include <stdint.h>

void do_mount_root()
{
    ata_drive_t *drives = get_ata_drives();
    static partition_t *part;

    static fat12_t fat12_fs;
    static uint8_t filebuf[4096];

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

            if (r > 8)
                r = 8;

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

                    struct fat_dirent e;

                    if (fat12_lookup(&fat12_fs, "/EFI/BOOT/BOOTX64.EFI", &e) == 0)
                    {
                        int32_t n = fat12_read(&fat12_fs, &e, filebuf, sizeof filebuf - 1);

                        if (n >= 0)
                        {
                            filebuf[n] = '\0';
                            hexdump(filebuf, 412);
                        }
                    }

                    break; // we found the first mountable partition, so leave for now
                }
            }
        }
    }
}