#ifndef _CKB_CELL_FS_H
#define _CKB_CELL_FS_H 1

typedef struct FSBlob {
    uint32_t offset;
    uint32_t length;
} FSBlob;

typedef struct FSEntry {
    FSBlob filename;
    FSBlob content;
} FSEntry;

typedef struct CellFileSystemNode {
    uint32_t count;
    FSEntry *files;
    void *start;
} CellFileSystemNode;

typedef struct CellFileSystem {
    CellFileSystemNode *current;
    struct CellFileSystem *next;
} CellFileSystem;

typedef struct FSFile {
    const char *filename;
    const void *content;
    uint32_t size;
    // indicate how many active users there are, used to avoid excessive opening
    // of the same file.
    // Currently the only valid values are 1 and 0.
    uint8_t rc;
} FSFile;

int get_file(const CellFileSystem *fs, const char *filename, FSFile **f);

int ckb_get_file(const char *filename, FSFile **file);

int load_fs(CellFileSystem **fs, void *buf, uint64_t buflen);

int ckb_load_fs(void *buf, uint64_t buflen);

void ckb_reset_fs();

#endif
