/*
 * showcases.c
 * 
 * This file contains some fun test scenarions to play with the Quernel
 * remember to NOT run these in bare metal because it is very likely
 * to break something important in your system.
 * 
 */
#include <quernel/printk.h>
#include <drivers/ata.h>
#include <quernel/dev.h>
#include <stdint.h>
#include <fs/ext.h>

/**
 * read_hello_world():
 * 
 * Reads the lyrics of the music Hello World by Louie Zong 
 * stored in the rootfs folder that is packed in the iso as 
 * the root filesystem of the quernel formatted in ext2.
 */
void read_hello_world(root_dev sda)
{
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
}

/**
 * read_hello_world_broken():
 * 
 * The scenario is the same of read_hello_world but before 
 * actually reading the text file we overwrite part of the ext2 superblock
 * with zeros causing a error when we try to read the file.
 */
void read_hello_world_broken(root_dev sda)
{
    ext2_t *ext2_part = &(sda.ext2_part);
    
    uint16_t zeros[256] = {0};
    write_partition(&(ext2_part->part), (ext2_part->block_size / 512), zeros);

    read_hello_world(sda);   
}