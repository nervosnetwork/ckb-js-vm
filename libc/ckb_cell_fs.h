#ifndef CKB_C_STDLIB_CKB_CELL_FS_H
#define CKB_C_STDLIB_CKB_CELL_FS_H 1

typedef struct FSBlob {
    uint32_t offset;
    uint32_t length;
} FSBlob;

typedef struct FSEntry {
    FSBlob filename;
    FSBlob content;
} FSEntry;

typedef struct FSCellNode {
    uint32_t count;
    FSEntry *files;
    void *start;
    const char *prefix;
} FSCellNode;

typedef struct FSCell {
    FSCellNode *current;
    struct FSCell *next;
} FSCell;

typedef struct FSFile {
    const char *filename;
    const void *content;
    uint32_t size;
    // indicate how many active users there are, used to avoid excessive opening
    // of the same file.
    // Currently the only valid values are 1 and 0.
    uint8_t rc;
} FSFile;

int ckb_get_file(const char *filename, FSFile **file);
int ckb_load_fs(const char *prefix, void *buf, uint64_t buflen);
void ckb_reset_fs();

#endif
