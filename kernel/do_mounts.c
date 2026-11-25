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
#include <memory.h>

#include "do_mounts.h"

void print_mounted_disk(ata_drive_t *drive, root_dev *d)
{
    printk("%s: ", (char *)drive->model);
    for (int i = 0; i < d->n_mounted_volumes; i++)
    {
        dev_volume_t vol = d->volumes[i];
        printk("sda%i (%s) ", i+1, fs_name(vol.fs_type));
    }
    printk("\n");
}

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
    gpt_part_t gp[8];
    int r = gpt_read(drive, gp, 8);

    if (r < 0)
    {
        pr_err("GPT read failed!\n");
        return 0;
    }

    if (r > 8)
        r = 8;

    int vol_i = 0;

    for (int i = 0; i < r; i++)
    {
        part->drive = drive;
        part->start = gp[i].start;
        part->count = gp[i].count;

        dev_volume_t *vol = &(d->volumes[vol_i]);
        // Since we don't yet know what fs is it, we don't know how much space
        // fs will need, so here we alloc the size of the biggest filesystem at the moment
        // which is fat12_t there might be a better way of doing this probably writing to a
        // buffer but at the moment this is enough to get it working.
        vol->fs = kmalloc(sizeof(fat12_t));

        if (fat12_mount((fat12_t *)vol->fs, part) == 0)
        {
            vol->fs_type = FAT12_T;
            vol->mount_order = vol_i;

            vol_i++;
            continue;
        }

        if (hfsplus_mount((hfsplus_t *)vol->fs, part) == 0)
        {
            vol->fs_type = HFSP_T;
            vol->mount_order = vol_i;

            vol_i++;
            continue;
        }

        if (ext2_mount((ext2_t *)vol->fs, part) == 0)
        {
            vol->fs_type = EXT2_T;
            vol->mount_order = vol_i;

            vol_i++;
            continue;
        }
    }

    d->n_mounted_volumes = vol_i;

    print_mounted_disk(drive, d);

    return 1;
}

void do_mount_root(root_dev *r_dev)
{
    ata_drive_t *drives = get_ata_drives();

    partition_t part;

    // We consider that drive 0 is our drive, but im not sure that is always the case
    mount_drive(&drives[0], r_dev, &part);
}