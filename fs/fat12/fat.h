#pragma once
#include <stdint.h>
#include "../../drivers/ata.h"
<<<<<<< HEAD
=======
#include "../../kernel/macros.h"
>>>>>>> 57d2674 ((#5) Add macros for compiler attributes)

#define FAT12_MAX_FAT_SECTORS 16

struct __packed fat12_bpb
{
    uint8_t jmp[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
};
_Static_assert(sizeof(struct fat12_bpb) == 62, "bpb size");

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

typedef struct {
    partition_t *part;
    uint32_t sectors_per_cluster;
    uint32_t fat_start;
    uint32_t root_start;
    uint32_t root_sectors;
    uint32_t data_start;
    uint32_t cluster_count;
    uint8_t  fat[FAT12_MAX_FAT_SECTORS * 512] __aligned(2);
} fat12_t; 

struct find_ctx
{
    char name[11];
    struct fat_dirent found;
    int ok;
};

typedef int (*fat12_dir_cb)(const struct fat_dirent *e, void *ctx);

#define ATTR_VOLUME 0x08
#define ATTR_DIR    0x10
#define ATTR_LFN    0x0F

#define FAT12_VALID(c) ((c) >= 2 && (c) <= 0xFEF)

int fat12_mount(fat12_t *fs, partition_t *part);
void fat12_ls(fat12_t *fs, uint16_t dir_cluster);
int fat12_lookup(fat12_t *fs, const char *path, struct fat_dirent *out);
int32_t fat12_read(fat12_t *fs, const struct fat_dirent *e, void *dst, uint32_t max);