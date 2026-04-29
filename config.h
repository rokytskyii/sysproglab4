#ifndef CONFIG_H
#define CONFIG_H

#define BLOCK_SIZE 128  // Розмір блоку [cite: 35]
#define MAX_FILES 16    // Максимальна кількість дескрипторів у ФС [cite: 118]
#define MAX_BLOCKS 1024 // Загальна кількість блоків носія [cite: 116]
#define MAX_NAME_LEN 32 // Максимальна довжина імені файлу [cite: 138]
#define DIRECT_BLOCKS 5 // Кількість прямих посилань у дескрипторі
#define MAX_FD 8        // Максимальна кількість одночасно відкритих файлів [cite: 154]

#endif