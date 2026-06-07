#pragma once
#include <stdint.h>

#define ATA_PRIMARY_BASE     0x1F0
#define ATA_SECONDARY_BASE   0x170

#define ATA_PRIMARY_CTRL     0x3F6
#define ATA_SECONDARY_CTRL   0x376

#define ATA_REG_DATA         0x00
#define ATA_REG_ERROR        0x01
#define ATA_REG_SECCOUNT     0x02
#define ATA_REG_LBA_LO       0x03
#define ATA_REG_LBA_MID      0x04
#define ATA_REG_LBA_HI       0x05
#define ATA_REG_DRIVE_HEAD   0x06
#define ATA_REG_STATUS       0x07
#define ATA_REG_COMMAND      0x07

#define ATA_SR_BSY           0x80   /* Drive busy */
#define ATA_SR_DRDY          0x40   /* Drive ready */
#define ATA_SR_ERR           0x01   /* Error */

/* Commands */
#define ATA_CMD_IDENTIFY     0xEC

typedef struct {
    uint8_t  present;
    uint8_t  is_master;
    uint16_t base;
    char     model[41];
    uint32_t sectors;
} ata_drive_t;

static int ata_identify(uint8_t channel, uint8_t slave, ata_drive_t *drive);

void ata_detect_drives();