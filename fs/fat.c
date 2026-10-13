#include "../drivers/ata.h"
#include "../drivers/tty.h"
#include "fat.h"

/**
 * Performs checks and returns 1 if valid, 0 on error.
 */
int is_bootsect_valid(struct fat12_bpb *b_sec)
{
    if (b_sec->bytes_per_sector != 512)
    {
        kprintln("Invalid size of bytes per sector, not 512!");
        return 0;
    }

    if (b_sec->sectors_per_cluster == 0 || (b_sec->sectors_per_cluster & (b_sec->sectors_per_cluster - 1)))
    {
        kprint("Invalid number of sectors per cluster!");
        return 0;
    }

    return 1;
}

/**
 * Attempts to mount a fat12 file system from the partition
 * and returns the following status codes:
 * `-1` read error, `-2` no boot signature, `-3` bad or unsupported BPB, `-4` FAT too large for the buffer, `-5` not FAT12.
 */
int fat12_mount(fat12_t *fs, partition_t *part)
{
    uint8_t sec[512] __attribute__((aligned(2)));

    if (read_partition(part, 0, sec) != 0)
    {
        kprintln("fat12_mount: Failed to read partition!");
        return -1;
    }

    if (sec[510] != 0x55 || sec[511] != 0xAA)
        return -2;

    const struct fat12_bpb *b = (const struct fat12_bpb *)sec;

    if (is_bootsect_valid(b) == 0)
    {
        kprintln("fat12_mount: Failed to validate the boot sector!");
        return -3;
    }

    uint32_t total = b->total_sectors_16 ? b->total_sectors_16 : b->total_sectors_32;

    fs->part = part;
    fs->sectors_per_cluster = b->sectors_per_cluster;
    fs->fat_start = b->reserved_sectors;
    fs->root_start = fs->fat_start + b->num_fats * b->sectors_per_fat;
    fs->root_sectors = (b->root_entry_count * 32 + 511) / 512;
    fs->data_start = fs->root_start + fs->root_sectors;

    if (total <= fs->data_start)
    {
        kprintln("fat12_mount: Total of sectors is less or equal to the data_start!");
        return -3;
    }

    fs->cluster_count = (total - fs->data_start) / fs->sectors_per_cluster;

    if (fs->cluster_count >= 4085)
    {
        kprintln("fat12_mount: cluster count exceeds 4085, this partition is likely not a FAT12 partition!");
        return -5;
    }

    for (uint32_t i = 0; i < b->sectors_per_fat; i++)
    {
        // Load the data into fs->fat
        if (read_partition(part, fs->fat_start + i, fs->fat + i * 512) != 0)
        {
            kprintln("fat12_mount: Failed to read filesys data!");
            return -1;
        }
    }

    return 0;
}