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

// Дескриптор файлу (Inode) [cite: 120, 121]
typedef struct
{
    file_type_t type;
    int nlink;                 // Кількість жорстких посилань [cite: 120]
    uint32_t size;             // Розмір у байтах [cite: 120]
    int direct[DIRECT_BLOCKS]; // Прямі посилання на блоки
    int indirect;              // Опосередковане посилання на блок з адресами
} inode_t;

// Запис у директорії (Directory Entry / Hard link) [cite: 127, 128]
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