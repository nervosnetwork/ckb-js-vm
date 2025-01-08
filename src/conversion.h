/*
 * Copyright (c) 2014-2018 The Bitcoin Core developers
 * Distributed under the MIT software license, see the accompanying
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.
 *
 * only used on little endian
 */

#ifndef CONVERSION_H
#define CONVERSION_H

#include <stdint.h>
#include <string.h>

uint32_t static inline le32toh_(uint32_t x) { return x; }

uint32_t static inline htole32_(uint32_t x) { return x; }

uint32_t static inline be32toh_(uint32_t x) { return bswap_32(x); }

uint32_t static inline htobe32_(uint32_t x) { return bswap_32(x); }

uint64_t static inline le64toh_(uint64_t x) { return x; }

uint64_t static inline htole64_(uint64_t x) { return x; }

uint32_t static inline be64toh_(uint64_t x) { return bswap_64(x); }

uint64_t static inline htobe64_(uint64_t x) { return bswap_64(x); }

uint32_t static inline ReadLE32(const unsigned char* ptr) {
    uint32_t x;
    memcpy((char*)&x, ptr, 4);
    return le32toh_(x);
}

void static inline WriteLE32(unsigned char* ptr, uint32_t x) {
    uint32_t v = htole32_(x);
    memcpy(ptr, (char*)&v, 4);
}

uint32_t static inline ReadBE32(const unsigned char* ptr) {
    uint32_t x;
    memcpy((char*)&x, ptr, 4);
    return be32toh_(x);
}

void static inline WriteBE32(unsigned char* ptr, uint32_t x) {
    uint32_t v = htobe32_(x);
    memcpy(ptr, (char*)&v, 4);
}

void static inline WriteLE64(unsigned char* ptr, uint64_t x) {
    uint64_t v = htole64_(x);
    memcpy(ptr, (char*)&v, 8);
}

uint64_t static inline ReadLE64(const unsigned char* ptr) {
    uint64_t x;
    memcpy((char*)&x, ptr, 8);
    return le64toh_(x);
}

void static inline WriteBE64(unsigned char* ptr, uint64_t x) {
    uint64_t v = htobe64_(x);
    memcpy(ptr, (char*)&v, 8);
}

uint64_t static inline ReadBE64(const unsigned char* ptr) {
    uint64_t x;
    memcpy((char*)&x, ptr, 8);
    return be64toh_(x);
}

#endif
