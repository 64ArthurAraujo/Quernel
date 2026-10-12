#pragma once
#include <stdint.h>
#include "../../drivers/include/ata.h"

typedef struct
{
    uint32_t start;
    uint32_t count;
    uint8_t type_guid[16];
} gpt_part_t;

struct __attribute__((packed)) gpt_header
{
    char signature[8];
    uint32_t revision;
    uint32_t header_size;
    uint32_t header_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[16];
    uint64_t entries_lba;     
    uint32_t num_entries;        
    uint32_t entry_size;        
    uint32_t entries_crc32;
};
_Static_assert(sizeof(struct gpt_header) == 92, "gpt header size");

struct __attribute__((packed)) gpt_entry {
    uint8_t  type_guid[16];      
    uint8_t  part_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;         
    uint64_t attributes;
    uint16_t name[36];          
};
_Static_assert(sizeof(struct gpt_entry) == 128, "gpt entry size");

int gpt_read(ata_drive_t *drive, gpt_part_t *out, int max);