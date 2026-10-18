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