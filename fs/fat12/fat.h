#pragma once
#include <drivers/ata.h>
#include <quernel/macros.h>
#include <stdint.h>
#include <fs/fat12.h>

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

/* super.c */
int fat12_mount(fat12_t *fs, partition_t *part);

/* dir.c */
int fat12_walk_dir(fat12_t *fs, uint16_t dir_cluster, fat12_dir_cb cb, void *ctx);
void fat12_ls(fat12_t *fs, uint16_t dir_cluster);
uint32_t cluster_to_sector(const fat12_t *fs, uint16_t c);
uint16_t fat12_next(const fat12_t *fs, uint16_t c);

/* namei.c */
int fat12_lookup(fat12_t *fs, const char *path, struct fat_dirent *out);
void fat12_format_name(const struct fat_dirent *e, char *out);
int fat12_make_83(const char *s, char out[11]);

/* file.c */
int32_t fat12_read(fat12_t *fs, const struct fat_dirent *e, void *dst, uint32_t max);