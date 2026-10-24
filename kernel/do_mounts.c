#include <quernel/printk.h>
#include <quernel/config.h>
#include <drivers/tty.h>
#include <drivers/ata.h>
#include <fs/fat12.h>
#include <fs/hfs.h>
#include <fs/gpt.h>
#include <string.h>
#include <stdint.h>

int read_drive(ata_drive_t drive, fat12_t *fat12, hfsplus_t *hfsp, partition_t *part)
{
    printk("ATA: %s\n", (char *)drive.model);

    gpt_part_t gp[8];
    int r = gpt_read(&drive, gp, 8);

    if (r < 0)
    {
        pr_err("GPT read failed!\n");
        return 0;
    }

    if (r > 8)
        r = 8;

    int is_fat12_mounted = 0;
    int is_hfs_mounted = 0;

    for (int i = 0; i < r; i++)
    {
        part->drive = &drive;
        part->start = gp[i].start;
        part->count = gp[i].count;

        if (!is_fat12_mounted && fat12_mount(fat12, part) == 0)
        {
            pr_debug("Mounted FAT12 file system!\n");
            is_fat12_mounted = 1;
            continue;
        }

        if (!is_hfs_mounted && hfsplus_mount(hfsp, part) == 0)
        {
            pr_debug("Mounted HFS+ file system!\n");
            is_hfs_mounted = 1;
            continue;
        }
    }

    return 1;
}

int total_drives(ata_drive_t *drives)
{
    int found = 0;

    for (int drive_i = 0; drive_i < 4; drive_i++)
    {
        if (drives[drive_i].present == 1)
            found += 1;
    }

    return found;
}

void do_mount_root()
{
    ata_drive_t *drives = get_ata_drives();

    partition_t part;
    static fat12_t fat12_fs;
    static hfsplus_t hfs_fs;

    int drive_num = total_drives(drives);

    for (int drive_i = 0; drive_i < drive_num; drive_i++)
    {
        read_drive(drives[drive_i], &fat12_fs, &hfs_fs, &part);

#if !ALLOW_MULTIDISK
        if (drive_num > 1)
        {
            // this considers that 0 is always the root disk where the kernel is stored but im not completely sure thats true
            pr_info("Multiple disks where detected but support them is disabled, only root will be considered!\n");
            break;
        }
#endif
    }
}