#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "ckb_cell_fs.h"

static CellFileSystem *CELL_FILE_SYSTEM = NULL;

int get_file(const CellFileSystem *fs, const char *filename, FSFile **f) {
    if (fs == NULL) {
        return -1;
    }
    FSFile *file = malloc(sizeof(FSFile));
    if (file == 0) {
        return -1;
    }
    CellFileSystem *cfs = (CellFileSystem *)fs;
    CellFileSystemNode *node = cfs->current;
    while (node != NULL) {
        for (uint32_t i = 0; i < node->count; i++) {
            FSEntry entry = node->files[i];
            if (strcmp(filename, node->start + entry.filename.offset) == 0) {
                // TODO: check the memory addresses are legal
                file->filename = filename;
                file->size = entry.content.length;
                file->content = node->start + entry.content.offset;
                file->rc = 1;
                *f = file;
                return 0;
            }
        }
        if (cfs->next == NULL) {
            break;
        }
        cfs = cfs->next;
        node = cfs->current;
    }
    free(file);
    return -1;
}

int ckb_get_file(const char *filename, FSFile **file) { return get_file(CELL_FILE_SYSTEM, filename, file); }

int load_fs(CellFileSystem **fs, void *buf, uint64_t buflen) {
    if (fs == NULL || buf == NULL) {
        return -1;
    }

    CellFileSystemNode *node = (CellFileSystemNode *)malloc(sizeof(CellFileSystemNode));
    if (node == NULL) {
        return -1;
    }

    CellFileSystem *newfs = (CellFileSystem *)malloc(sizeof(CellFileSystem));
    if (newfs == NULL) {
        free(node);
        return -1;
    }

    node->count = *(uint32_t *)buf;
    if (node->count == 0) {
        node->files = NULL;
        node->start = NULL;
        newfs->next = *fs;
        newfs->current = node;
        *fs = newfs;
        return 0;
    }

    node->files = (FSEntry *)malloc(sizeof(FSEntry) * node->count);
    if (node->files == NULL) {
        free(node);
        free(newfs);
        return -1;
    }
    node->start = buf + sizeof(node->count) + (sizeof(FSEntry) * node->count);

    FSEntry *entries = (FSEntry *)((char *)buf + sizeof(node->count));
    for (uint32_t i = 0; i < node->count; i++) {
        FSEntry entry = entries[i];
        node->files[i] = entry;
    }

    newfs->next = *fs;
    newfs->current = node;
    *fs = newfs;
    return 0;
}

int ckb_load_fs(void *buf, uint64_t buflen) {
    int ret = load_fs(&CELL_FILE_SYSTEM, buf, buflen);
    return ret;
}

void ckb_reset_fs() { CELL_FILE_SYSTEM = NULL; }
