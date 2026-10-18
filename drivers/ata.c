#include "../cpu/ports.h"
#include "../libc/memory.h"
#include "../libc/string.h"
#include "ata.h"
#include <quernel/printk.h>
#include <tty.h>

ata_drive_t ata_drives[4];

ata_drive_t *get_ata_drives() { return ata_drives; }

static void ata_delay(uint8_t ctrl)
{
    for (int i = 0; i < 4; i++)
        port_byte_in(ctrl);
}

static int ata_identify(uint8_t channel, uint8_t slave, ata_drive_t *drive)
{
    uint16_t base = channel ? ATA_SECONDARY_BASE : ATA_PRIMARY_BASE;
    uint16_t ctrl = channel ? ATA_SECONDARY_CTRL : ATA_PRIMARY_CTRL;

    if (port_byte_in(base + ATA_REG_STATUS) == 0xFF)
        return 0;

    port_byte_out(base + ATA_REG_DRIVE_HEAD, slave ? 0xB0 : 0xA0);
    ata_delay(ctrl);

    port_byte_out(base + ATA_REG_SECCOUNT, 0);
    port_byte_out(base + ATA_REG_LBA_LO, 0);
    port_byte_out(base + ATA_REG_LBA_MID, 0);
    port_byte_out(base + ATA_REG_LBA_HI, 0);

    port_byte_out(base + ATA_REG_STATUS, ATA_CMD_IDENTIFY);
    ata_delay(ctrl);

    if (port_byte_in(base + ATA_REG_STATUS) == 0)
        return 0;

    uint8_t status;
    while ((status = port_byte_in(base + ATA_REG_STATUS)) & ATA_SR_BSY)
        ;

    if (port_byte_in(base + ATA_REG_LBA_MID) || port_byte_in(base + ATA_REG_LBA_HI))
        return 0;

    while ((status = port_byte_in(base + ATA_REG_STATUS)) & ATA_SR_BSY)
        ;

    if (status & (ATA_SR_ERR | ATA_SR_DF))
    {
        printk("Failed to identify disk!\n");
        return 0;
    }

    uint16_t buffer[256];
    for (int i = 0; i < 256; i++)
        buffer[i] = port_word_in(base);

    for (int i = 0; i < 20; i++)
    {
        drive->model[i * 2] = (buffer[27 + i] >> 8) & 0xFF;
        drive->model[i * 2 + 1] = buffer[27 + i] & 0xFF;
    }

    drive->model[40] = '\0';

    for (int i = 39; i >= 0 && drive->model[i] == ' '; i--)
        drive->model[i] = '\0';

    drive->sectors = buffer[60] | ((uint32_t)buffer[61] >> 16);

    drive->present = 1;
    drive->slave = slave;
    drive->is_master = !slave;
    drive->base = base;
    return 1;
}

void ata_detect_drives()
{
    memset(ata_drives, 0, sizeof(ata_drives));

    int found = 0;

    for (uint8_t ch = 0; ch < 2; ch++)
    {
        for (uint8_t sl = 0; sl < 2; sl++)
        {
            int idx = ch * 2 + sl;

            if (ata_identify(ch, sl, &ata_drives[idx]))
                found++;
        }
    }

    if (!found)
        printk("ATA: Not a single device was detected\n");
}


void read_sector_data(uint16_t base, uint16_t *buffer)
{
    for (int i = 0; i < 256; i++)
        buffer[i] = port_word_in(base);
}

int ata_read_28bit_pio(uint16_t *buffer, ata_drive_t *drive, uint32_t lba_offset)
{
    uint16_t base = drive->base;
    uint16_t ctrl = (base == ATA_PRIMARY_BASE) ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL;

    port_byte_out(base + ATA_REG_DRIVE_HEAD, 0xE0 | (drive->slave << 4) | ((lba_offset >> 24) & 0x0F));
    ata_delay(ctrl);
    port_byte_out(base + ATA_REG_ERROR, 0x00);
    port_byte_out(base + ATA_REG_SECCOUNT, (unsigned char)1);
    port_byte_out(base + ATA_REG_LBA_LO, (uint8_t)(lba_offset));
    port_byte_out(base + ATA_REG_LBA_MID, (uint8_t)(lba_offset >> 8));
    port_byte_out(base + ATA_REG_LBA_HI, (uint8_t)(lba_offset >> 16));
    port_byte_out(base + ATA_REG_STATUS, ATA_CMD_READSECTORS);
    ata_delay(ctrl);

    uint8_t status;
    while ((status = port_byte_in(base + ATA_REG_STATUS)) & ATA_SR_BSY)
        ;

    if (status & (ATA_SR_ERR | ATA_SR_DF))
    {
        printk("28bit pio failed to read sectors!\n");
        return -1;
    }

    if (!(status & ATA_SR_DRQ))
    {
        printk("28bit pio failed to read sectors! No DRQ\n");
        return -1;
    }

    read_sector_data(base, buffer);
    return 0;
}

int read_partition(partition_t *p, uint32_t sector, void *buf)
{
    if (sector >= p->count) return -1;
    return ata_read_28bit_pio(buf, p->drive, p->start + sector);
}