#pragma once
#include <stdint.h>

#define DATA_XFER_R      1 /* Data Transfer READ */
#define DATA_XFER_W      2 /* Data Transfer WRITE */

typedef struct {
    uint8_t  present;
    uint8_t  is_master;
    uint8_t  slave;
    uint16_t base;
    char     model[41];
    uint32_t sectors;
} ata_drive_t;

typedef struct {
    ata_drive_t *drive;
    uint32_t     start;
    uint32_t     count;  
} partition_t;

static int ata_identify(uint8_t channel, uint8_t slave, ata_drive_t *drive);
void ata_detect_drives();
ata_drive_t* get_ata_drives();

int ata_data_xfer(uint16_t *buffer, ata_drive_t *drive, uint32_t lba_offset, int rw);
int read_partition(partition_t *p, uint32_t sector, void *buf);
int write_partition(partition_t *p, uint32_t sector, uint16_t *buf);