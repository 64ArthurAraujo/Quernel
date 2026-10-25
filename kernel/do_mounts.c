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

int count_drives(ata_drive_t *d)
{
    int found = 0;
    for (int i = 0; i < 4; i++)
        if (d[i].present == 1)
            found++;
    return found;
}

int read_drive(ata_drive_t drive, fat12_t *fat12, hfsplus_t *hfsp, ext2_t *ext2, partition_t *part)
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
    int is_ext2_mounted = 0;

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

        if (!is_ext2_mounted && ext2_mount(ext2, part) == 0)
        {
            pr_debug("Mounted ext2 file system!\n");
            is_ext2_mounted = 1;

            ext2_inode root, lpf;
            uint32_t ino;

            ext2_iget(ext2, EXT2_ROOT_INO, &root);

            int r = ext2_lookup(ext2, &root, "hello.txt", strlen("hello.txt"), &lpf, &ino);

            if (r == 0)
                pr_info("hello.txt = inode %i, mode 0x%i\n", ino, lpf.i_mode);
            else if (r == -ENOENT)
                pr_info("not found\n");

            continue;
        }
    }

    return 1;
}

void do_mount_root()
{
    ata_drive_t *drives = get_ata_drives();

    partition_t part;
    static fat12_t fat12_fs;
    static hfsplus_t hfs_fs;
    static ext2_t ext2_fs;

    int drive_num = count_drives(drives);

    for (int drive_i = 0; drive_i < drive_num; drive_i++)
    {
        read_drive(drives[drive_i], &fat12_fs, &hfs_fs, &ext2_fs, &part);

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