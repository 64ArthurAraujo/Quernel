#pragma once
#include <quernel/macros.h>

// ext2

/*
 * Constants relative to the data blocks
 */
#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)

#define EXT2_ROOT_INO 2

#define EXT2_MAX_GROUPS 64

/*
 * Structure of an inode on the disk
 */
typedef struct
{
    uint16_t i_mode;        /* File mode */
    uint16_t i_uid;         /* Low 16 bits of Owner Uid */
    uint32_t i_size;        /* Size in bytes */
    uint32_t i_atime;       /* Access time */
    uint32_t i_ctime;       /* Creation time */
    uint32_t i_mtime;       /* Modification time */
    uint32_t i_dtime;       /* Deletion Time */
    uint16_t i_gid;         /* Low 16 bits of Group Id */
    uint16_t i_links_count; /* Links count */
    uint32_t i_blocks;      /* Blocks count */
    uint32_t i_flags;       /* File flags */
    union
    {
        struct
        {
            uint32_t l_i_reserved1;
        } linux1;
        struct
        {
            uint32_t h_i_translator;
        } hurd1;
        struct
        {
            uint32_t m_i_reserved1;
        } masix1;
    } osd1;                          /* OS dependent 1 */
    uint32_t i_block[EXT2_N_BLOCKS]; /* Pointers to blocks */
    uint32_t i_generation;           /* File version (for NFS) */
    uint32_t i_file_acl;             /* File ACL */
    uint32_t i_dir_acl;              /* Directory ACL */
    uint32_t i_faddr;                /* Fragment address */
    union
    {
        struct
        {
            uint8_t l_i_frag;  /* Fragment number */
            uint8_t l_i_fsize; /* Fragment size */
            uint16_t i_pad1;
            uint16_t l_i_uid_high; /* these 2 fields    */
            uint16_t l_i_gid_high; /* were reserved2[0] */
            uint32_t l_i_reserved2;
        } linux2;
        struct
        {
            uint8_t h_i_frag;  /* Fragment number */
            uint8_t h_i_fsize; /* Fragment size */
            uint16_t h_i_mode_high;
            uint16_t h_i_uid_high;
            uint16_t h_i_gid_high;
            uint32_t h_i_author;
        } hurd2;
        struct
        {
            uint8_t m_i_frag;  /* Fragment number */
            uint8_t m_i_fsize; /* Fragment size */
            uint16_t m_pad1;
            uint32_t m_i_reserved2[2];
        } masix2;
    } osd2; /* OS dependent 2 */
} __packed ext2_inode;

/**
 * Structure of the ext2 super block
 */
typedef struct
{
    uint32_t inodes_count;        /* Total number of inodes in file system */
    uint32_t blocks_count;        /* Total number of blocks in file system */
    uint32_t r_blocks_count;      /* Number of blocks reserved for superuser */
    uint32_t free_blocks_count;   /* Total number of unallocated blocks  */
    uint32_t free_inodes_count;   /* Total number of unallocated inodes */
    uint32_t first_data_block;    /* Block number of the block containing the superblock (also the starting block number, NOT always zero.) */
    uint32_t log_block_size;      /* log2 (block size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the block size)  */
    uint32_t log_frag_size;       /* log2 (fragment size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the fragment size) */
    uint32_t blocks_per_group;    /* Number of blocks in each block group */
    uint32_t frags_per_group;     /* Number of fragments in each block group */
    uint32_t inodes_per_group;    /* Number of inodes in each block group */
    uint32_t mount_time;          /* Last mount time (in POSIX time) */
    uint32_t write_time;          /* Last written time (in POSIX time) */
    uint16_t mount_count;         /* Mount count */
    uint16_t max_mount_count;     /* Maximal mount count */
    uint16_t signature;           /* Magic signature */
    uint16_t state;               /* File system state */
    uint16_t errors;              /* Behaviour when detecting errors */
    uint16_t minor_rev_level;     /* minor revision level */
    uint32_t last_check;          /* time of last check */
    uint32_t check_interval;      /* max. time between checks */
    uint32_t creator_os;          /* OS */
    uint32_t rev_level;           /* Revision level */
    uint16_t def_resuid;          /* Default uid for reserved blocks */
    uint16_t def_resgid;          /* Default gid for reserved blocks */
    
    // These are ext2 extended super block fields
    uint32_t s_first_ino;         /* First non-reserved inode */
    uint16_t s_inode_size;        /* size of inode structure */
    uint16_t s_block_group_nr;    /* block group # of this superblock */
    uint32_t s_feature_compat;    /* compatible feature set */
    uint32_t s_feature_incompat;  /* incompatible feature set */
    uint32_t s_feature_ro_compat; /* readonly-compatible feature set */
    uint8_t s_uuid[16];           /* 128-bit uuid for volume */
    char s_volume_name[16];       /* volume name */
    char s_last_mounted[64];      /* directory where last mounted */
} ext2_super_block;

/*
 * Structure of a blocks group descriptor
 */
typedef struct
{
    uint32_t block_bitmap;      /* Blocks bitmap block */
    uint32_t inode_bitmap;      /* Inodes bitmap block */
    uint32_t inode_table;       /* Inodes table block */
    uint16_t free_blocks_count; /* Free blocks count */
    uint16_t free_inodes_count; /* Free inodes count */
    uint16_t used_dirs_count;   /* Directories count */
    uint16_t pad;
    uint32_t reserved[3];
} ext2_group_desc;

typedef struct
{
    partition_t part;
    ext2_super_block sb;
    ext2_group_desc bgd[EXT2_MAX_GROUPS];
    uint32_t block_size;
    uint32_t inode_size;
    uint32_t group_count;
} ext2_t;

typedef struct
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[];      
} __packed ext2_dir_entry;

int ext2_mount(ext2_t *fs, partition_t *part);
int ext2_lookup(ext2_t *fs, const ext2_inode *dir, const char *name, uint32_t len, ext2_inode *out, uint32_t *out_ino);
int ext2_iget(ext2_t *fs, uint32_t ino, ext2_inode *out);