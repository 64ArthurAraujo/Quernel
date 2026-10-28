#include <quernel/printk.h>
#include <quernel/config.h>
#include <drivers/tty.h>
#include <drivers/ata.h>
#include <fs/fat12.h>
#include <fs/hfs.h>
#include <fs/ext.h>
#include <fs/gpt.h>
#include <string.h>
#include <stdint.h>

#include "do_mounts.h"

int count_drives(ata_drive_t *d)
{
    int found = 0;
    for (int i = 0; i < 4; i++)
        if (d[i].present == 1)
            found++;
    return found;
}

int mount_drive(ata_drive_t *drive, root_dev *d, partition_t *part)
{
    printk("ATA: %s\n", (char *)drive->model);

    gpt_part_t gp[8];
    int r = gpt_read(drive, gp, 8);

    if (r < 0)
    {
        pr_err("GPT read failed!\n");
        return 0;
    }

    if (r > 8)
        r = 8;

    int is_fat12_mounted = 0;
    int is_hfs_mounted = 0;
    int is_ext2_mounted = 0;

    for (int i = 0; i < r; i++)
    {
        part->drive = drive;
        part->start = gp[i].start;
        part->count = gp[i].count;

        if (!is_fat12_mounted && fat12_mount(&(d->fat12_part), part) == 0)
        {
            pr_debug("Mounted FAT12 file system!\n");
            is_fat12_mounted = 1;
            continue;
        }

        if (!is_hfs_mounted && hfsplus_mount(&(d->hfsp_part), part) == 0)
        {
            pr_debug("Mounted HFS+ file system!\n");
            is_hfs_mounted = 1;
            continue;
        }

        if (!is_ext2_mounted && ext2_mount(&(d->ext2_part), part) == 0)
        {
            pr_debug("Mounted ext2 file system!\n");
            is_ext2_mounted = 1;

            continue;
        }
    }

    return 1;
}

void do_mount_root(root_dev *r_dev)
{
    ata_drive_t *drives = get_ata_drives();

    partition_t part;

    int drive_num = count_drives(drives);

    // We consider that drive 0 is our drive, but im not sure that is always the case
    mount_drive(&drives[0], r_dev, &part);
}