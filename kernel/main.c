#include <quernel/printk.h>
#include <quernel/cpu.h>
#include <drivers/ata.h>
#include <quernel/dev.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>

#include "do_mounts.h"

#include <fs/ext.h>

static root_dev sda = {0};

void init_kernel()
{
    isr_setup();
    asm volatile("sti");

    printk("Quernel version 0.0.0.i686\n");

    cpuid_detect();
    ata_detect_drives();
    do_mount_root(&sda);

    ext2_t *ext2_part = &(sda.ext2_part);

    ext2_inode ino;
    uint32_t num;
    static char data[512];

    if (ext2_namei(ext2_part, "/hello+world.txt", &ino, &num) == 0)
    {
        int64_t n = ext2_read_file(ext2_part, &ino, 0, data, sizeof(data) - 1);

        if (n >= 0)
        {
            data[n] = 0;
            pr_debug("%s\n", data);
        }
    }

    asm volatile("hlt");
}
