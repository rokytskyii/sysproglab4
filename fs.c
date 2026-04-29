#include "fs.h"
#include <string.h>
#include <stdio.h>

inode_t inodes[MAX_FILES];
dir_entry_t root_dir[MAX_FILES];
uint8_t disk[MAX_BLOCKS][BLOCK_SIZE]; // Носій у пам'яті [cite: 142]
uint8_t bitmap[MAX_BLOCKS];           // Бітова карта блоків [cite: 115]

void fs_init()
{
    memset(bitmap, 0, MAX_BLOCKS);
    for (int i = 0; i < MAX_FILES; i++)
    {
        inodes[i].type = TYPE_EMPTY;
        inodes[i].nlink = 0;
        inodes[i].size = 0;
        inodes[i].indirect = -1;
        for (int j = 0; j < DIRECT_BLOCKS; j++)
            inodes[i].direct[j] = -1;
        root_dir[i].inode_id = -1;
    }
}

int fs_allocate_block()
{
    for (int i = 0; i < MAX_BLOCKS; i++)
    {
        if (!bitmap[i])
        {
            bitmap[i] = 1;
            memset(disk[i], 0, BLOCK_SIZE);
            return i;
        }
    }
    return -1;
}

void fs_free_block(int idx)
{
    if (idx >= 0 && idx < MAX_BLOCKS)
        bitmap[idx] = 0;
}

// Повертає номер фізичного блоку для логічного зміщення у файлі
int fs_get_phys_block(int inode_id, int log_blk, int allocate)
{
    inode_t *node = &inodes[inode_id];
    if (log_blk < DIRECT_BLOCKS)
    {
        if (node->direct[log_blk] == -1 && allocate)
            node->direct[log_blk] = fs_allocate_block();
        return node->direct[log_blk];
    }
    else
    {
        int ind_idx = log_blk - DIRECT_BLOCKS;
        if (node->indirect == -1)
        {
            if (!allocate)
                return -1;
            node->indirect = fs_allocate_block();
            int *ind_table = (int *)disk[node->indirect];
            for (int i = 0; i < BLOCK_SIZE / sizeof(int); i++)
                ind_table[i] = -1;
        }
        int *ind_table = (int *)disk[node->indirect];
        if (ind_table[ind_idx] == -1 && allocate)
            ind_table[ind_idx] = fs_allocate_block();
        return ind_table[ind_idx];
    }
}