#pragma once
#include <drivers/ata.h>
#include <fs/fat12.h>
#include <fs/ext.h>
#include <fs/hfs.h>
#include <stddef.h>

/* The maximum supported number of partitions of a disk */
#define DEV_MAXVOLUMES 8

typedef enum
{
    FAT12_T,
    HFSP_T,
    EXT2_T,
} fs_t;

typedef struct
{
    void *fs;        /* The pointer to this partition's filesystem */
    fs_t fs_type;    /* The type filesystem type of the fs field */
    int mount_order; /* The order that this partition was mounted (incremental 1..2..3..) */
} dev_volume_t;

/**
 * The root device of quernel
 */
typedef struct
{
    ata_drive_t drive;
    dev_volume_t volumes[DEV_MAXVOLUMES];
    int n_mounted_volumes; /* The total number of mounted partitions within this drive */
} root_dev;

#define vol(n) (n > 0 ? n-1 : 0) /* "Translate" a volume number to an array position */