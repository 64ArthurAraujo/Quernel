#include "./include/gpt.h"
#include "../drivers/include/ata.h"
#include "../drivers/include/tty.h"

int gpt_read(ata_drive_t *drive, gpt_part_t *out, int max)
{
    uint8_t sec[512] __attribute__((aligned(2)));

    if (ata_read_28bit_pio((uint16_t *)sec, drive, 1) != 0)
        return -1;

    const struct gpt_header *h = (const struct gpt_header *)sec;
    static const char magic[8] = {'E', 'F', 'I', ' ', 'P', 'A', 'R', 'T'};

    for (int i = 0; i < 8; i++)
    {
        if (h->signature[i] != magic[i])
        {
            kprintln("gpt_read: Signature did not match magic!");
            return -2;
        }
    }

    uint32_t entry_size = h->entry_size;
    uint32_t num = h->num_entries;
    uint64_t lba64 = h->entries_lba;

    if (entry_size != 128 || lba64 > 0x0FFFFFFF)
    {
        kprintln("gpt_read: Invalid entry_size or lba");
        return -3;
    }

    uint32_t lba = (uint32_t) lba64;
    uint32_t per_sector = 512 / entry_size;
    uint32_t sectors = (num + per_sector - 1) / per_sector;

    int found = 0;

    for (uint32_t s = 0; s < sectors; s++)
    {
        if (ata_read_28bit_pio((uint16_t *) sec, drive, lba + s) != 0)
            return -1;

        for (uint32_t j = 0; j < per_sector; j++)
        {
            if (s * per_sector + j >= num) break;

            const struct gpt_entry *e =  (const struct gpt_entry *)(sec + j * entry_size);

            int used = 0;

            for (int k = 0; k < 16; k++)
            {
                if (e->type_guid[k]) { used = 1; break; }
            }

            if (!used) continue;

            if ((e->first_lba >> 32) || (e->last_lba >> 32)) continue;

            if (found < max) 
            {
                out[found].start = (uint32_t) e->first_lba;
                out[found].count = (uint32_t) (e->last_lba - e->first_lba + 1);

                for (int k = 0; k < 16; k++)
                    out[found].type_guid[k] = e->type_guid[k]; 
            }
            found++;
        }

    }

    return found;
}