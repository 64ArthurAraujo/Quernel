#pragma once
#include <quernel/macros.h>

#define FAT12_MAX_FAT_SECTORS 16

struct __packed fat_dirent
{
    char name[11];
    uint8_t attr;
    uint8_t nt_reserved;
    uint8_t create_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t access_date;
    uint16_t cluster_hi; 
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_lo; 
    uint32_t size;    
};
_Static_assert(sizeof(struct fat_dirent) == 32, "dirent size");

typedef struct
{
    partition_t *part;
    uint32_t sectors_per_cluster;
    uint32_t fat_start;
    uint32_t root_start;
    uint32_t root_sectors;
    uint32_t data_start;
    uint32_t cluster_count;
    uint8_t fat[FAT12_MAX_FAT_SECTORS * 512] __aligned(2);
} fat12_t;

int fat12_mount(fat12_t *fs, partition_t *part);
void fat12_ls(fat12_t *fs, uint16_t dir_cluster);
int fat12_lookup(fat12_t *fs, const char *path, struct fat_dirent *out);
int32_t fat12_read(fat12_t *fs, const struct fat_dirent *e, void *dst, uint32_t max);