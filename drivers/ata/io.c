#include <quernel/printk.h>
#include <quernel/cpu.h>
#include <drivers/tty.h>
#include <memory.h>
#include <string.h>

#include "ata.h"

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

    uint32_t timeout = 500000;

    uint8_t status;
    while (((status = port_byte_in(base + ATA_REG_STATUS)) & ATA_SR_BSY) && --timeout)
        ;

    if (timeout == 0 || status == 0xFF)
    {
        pr_err("ata fatal: BSY timed out!\n");
        return -1;
    }

    if (status & (ATA_SR_ERR | ATA_SR_DF))
    {
        pr_err("ata fatal: 28bit pio failed to read sectors!\n");
        return -1;
    }

    if (!(status & ATA_SR_DRQ))
    {
        pr_err("ata fatal: 28bit pio failed to read sectors! No DRQ\n");
        return -1;
    }

    read_sector_data(base, buffer);
    return 0;
}

int read_partition(partition_t *p, uint32_t sector, void *buf)
{
    if (sector >= p->count)
        return -1;
    return ata_read_28bit_pio(buf, p->drive, p->start + sector);
}