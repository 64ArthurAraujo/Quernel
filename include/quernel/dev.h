#pragma once
#include <drivers/ata.h>
#include <fs/fat12.h>
#include <fs/ext.h>
#include <fs/hfs.h>

/**
 * The root device of quernel 
 */
typedef struct 
{
    ata_drive_t drive;
    fat12_t fat12_part;
    hfsplus_t hfsp_part; /* GRUB + kernel.elf */
    ext2_t ext2_part; /* This is the root of the kernel, where programs will be stored and virtual files and devices will be mounted. */
} root_dev;
