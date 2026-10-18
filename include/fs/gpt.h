#pragma once
#include <drivers/ata.h>

typedef struct
{
    uint32_t start;
    uint32_t count;
    uint8_t type_guid[16];
} gpt_part_t;

int gpt_read(ata_drive_t *drive, gpt_part_t *out, int max);