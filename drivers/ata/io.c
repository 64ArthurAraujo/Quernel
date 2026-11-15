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

static int ata_wait_for_drq(uint16_t base)
{
    uint8_t status;
    uint32_t timeout = 500000;

    while (timeout--)
    {
        status = port_byte_in(base + ATA_REG_STATUS);

        if (status == 0xFF)
        {
            pr_err("ata fatal: status resolved to 0xFF\n");
            return -1;
        }

        if (status & (ATA_SR_ERR | ATA_SR_DF))
        {
            pr_err("ata fatal: status resolved to an error or drive fault!\n");
            return -1;
        }

        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
            return 0;
    }

    pr_err("ata fatal: waiting for DRQ timed out!\n");
    return -1;
}

/**
 * ata_data_xfer - Transfer data by PIO
 *  @buffer: data buffer
 *	@drive: the disk drive
 *	@lba_offset: lba_offset to write to
 *	@rw: read/write
 */
int ata_data_xfer(uint16_t *buffer, ata_drive_t *drive, uint32_t lba_offset, int rw)
{
    // Fail early with rw so we don't send commands uselessly to the disk and we
    // don't have to deal with defaults in the switches later on.
    if (rw < DATA_XFER_R || rw > DATA_XFER_W)
    {
        pr_err("ata: Invalid argument, rw can only be 1 or 2! But it is %i\n", rw);
        return -1;
    }

    uint16_t base = drive->base;
    uint16_t ctrl = (base == ATA_PRIMARY_BASE) ? ATA_PRIMARY_CTRL : ATA_SECONDARY_CTRL;

    port_byte_out(base + ATA_REG_DRIVE_HEAD, 0xE0 | (drive->slave << 4) | ((lba_offset >> 24) & 0x0F));
    ata_delay(ctrl);
    port_byte_out(base + ATA_REG_ERROR, 0x00);
    port_byte_out(base + ATA_REG_SECCOUNT, (unsigned char)1);
    port_byte_out(base + ATA_REG_LBA_LO, (uint8_t)(lba_offset));
    port_byte_out(base + ATA_REG_LBA_MID, (uint8_t)(lba_offset >> 8));
    port_byte_out(base + ATA_REG_LBA_HI, (uint8_t)(lba_offset >> 16));

    switch (rw)
    {
    case DATA_XFER_R:
        port_byte_out(base + ATA_REG_COMMAND, ATA_CMD_READSECTORS);
        break;

    case DATA_XFER_W:
        port_byte_out(base + ATA_REG_COMMAND, ATA_CMD_WRITESECTORS);
        break;
    }

    ata_delay(ctrl);

    if (ata_wait_for_drq(base) < 0)
        return -1;

    switch (rw)
    {
    case DATA_XFER_R:
        read_sector_data(base, buffer);
        return 0;

    case DATA_XFER_W:
        for (int i = 0; i < 256; i++)
            port_word_out(base + ATA_REG_DATA, buffer[i]);

        ata_delay(ctrl);

        uint8_t status;
        uint32_t timeout = 500000;
        while (((status = port_byte_in(base + ATA_REG_STATUS)) & ATA_SR_BSY) && --timeout)
            ;

        if (!(status & (ATA_SR_DRDY | ATA_SR_DSC)))
        {
            pr_err("ata fatal: failed to write sectors\n");
            return -1;
        }

        port_byte_out(base + ATA_REG_COMMAND, ATA_CMD_FLUSHCACHE);

        timeout = 500000;
        while (((status = port_byte_in(base + ATA_REG_STATUS)) & ATA_SR_BSY) && --timeout)
            ;
            
        if (timeout == 0 || (status & (ATA_SR_ERR | ATA_SR_DF)))
        {
            pr_err("ata fatal: failed to write sectors, flush cache failed!\n");
            return -1;
        }

        return 0;
    }
}

int read_partition(partition_t *p, uint32_t sector, void *buf)
{
    if (sector >= p->count)
        return -1;
    return ata_data_xfer(buf, p->drive, p->start + sector, DATA_XFER_R);
}

int write_partition(partition_t *p, uint32_t sector, uint16_t *buf)
{
    if (sector >= p->count)
        return -1;
    return ata_data_xfer(buf, p->drive, p->start + sector, DATA_XFER_W);
}