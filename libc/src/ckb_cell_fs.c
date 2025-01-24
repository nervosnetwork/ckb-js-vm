#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "ckb_cell_fs.h"

static FSCell *CELL_FILE_SYSTEM = NULL;

static int get_file(const FSCell *fs, const char *filename, FSFile **f) {
    if (fs == NULL) {
        return -1;
    }
    FSFile *file = malloc(sizeof(FSFile));
    if (file == 0) {
        return -1;
    }
    FSCell *cfs = (FSCell *)fs;
    FSCellNode *node = cfs->current;
    while (node != NULL) {
        if (strncmp(node->prefix + 1, filename, strlen(node->prefix) - 1) != 0) {
            if (cfs->next == NULL) {
                break;
            }
            cfs = cfs->next;
            node = cfs->current;
            continue;
        }
        const char *basename = filename + strlen(node->prefix) - 1;
        if (node->prefix[strlen(node->prefix) - 1] != '/') {
            basename++;
        }
        for (uint32_t i = 0; i < node->count; i++) {
            FSEntry entry = node->files[i];
            if (strcmp(basename, node->start + entry.filename.offset) == 0) {
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

static int load_fs(FSCell **fs, const char *prefix, void *buf, uint64_t buflen) {
    if (fs == NULL || buf == NULL) {
        return -1;
    }

    FSCellNode *node = (FSCellNode *)malloc(sizeof(FSCellNode));
    if (node == NULL) {
        return -1;
    }

    FSCell *newfs = (FSCell *)malloc(sizeof(FSCell));
    if (newfs == NULL) {
        free(node);
        return -1;
    }

    node->prefix = prefix;
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
        char *filename = node->start + entry.filename.offset;
        if (filename[0] == '.' || filename[0] == '/' || filename[0] == '\\' || filename[0] == '~') {
            return -2;
        }
    }

    newfs->next = *fs;
    newfs->current = node;
    *fs = newfs;
    return 0;
}

int ckb_load_fs(const char *prefix, void *buf, uint64_t buflen) {
    int ret = load_fs(&CELL_FILE_SYSTEM, prefix, buf, buflen);
    return ret;
}

void ckb_reset_fs() { CELL_FILE_SYSTEM = NULL; }
