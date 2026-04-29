#ifndef FS_H
#define FS_H

#include "config.h"
#include <stdint.h>

typedef enum
{
    TYPE_EMPTY = 0,
    TYPE_REG = 1,
    TYPE_DIR = 2
} file_type_t;

typedef struct
{
    file_type_t type;
    int nlink;
    uint32_t size;
    int direct[DIRECT_BLOCKS];
    int indirect;
} inode_t;

typedef struct
{
    char name[MAX_NAME_LEN];
    int inode_id;
} dir_entry_t;

extern inode_t inodes[MAX_FILES];
extern dir_entry_t root_dir[MAX_FILES];
extern uint8_t disk[MAX_BLOCKS][BLOCK_SIZE];

void fs_init();
int fs_allocate_block();
void fs_free_block(int idx);
int fs_get_phys_block(int inode_id, int logical_block, int allocate);

#endif