#include <quernel/printk.h>
#include <drivers/tty.h>
#include <drivers/ata.h>
#include <fs/fat12.h>
#include <fs/hfs.h>
#include <fs/gpt.h>
#include <string.h>
#include <stdint.h>

void do_mount_root()
{
    ata_drive_t *drives = get_ata_drives();
    partition_t part;

    static fat12_t fat12_fs;
    static hfsplus_t hfs_fs;

    int is_fat12_mounted = 0;
    int is_hfs_mounted = 0;

    for (int drive_i = 0; drive_i < 4; drive_i++)
    {
        if (drives[drive_i].present != 1)
            continue;

        printk("ATA: %s\n", (char *)drives[drive_i].model);

        gpt_part_t gp[8];
        int r = gpt_read(&drives[drive_i], gp, 8);

        if (r < 0)
        {
            printk("GPT read failed!\n");
            continue;
        }

        if (r > 8)
            r = 8;

        for (int i = 0; i < r; i++)
        {
            part.drive = &drives[drive_i];
            part.start = gp[i].start;
            part.count = gp[i].count;

            if (!is_fat12_mounted && fat12_mount(&fat12_fs, &part) == 0)
            {
                printk("Mounted FAT12 file system!\n");
                is_fat12_mounted = 1;
                continue;
            }

            if (!is_hfs_mounted && hfsplus_mount(&hfs_fs, &part) == 0)
            {
                printk("Mounted HFS+ file system!\n");
                is_hfs_mounted = 1;
                continue;
            }
        }
    }
}