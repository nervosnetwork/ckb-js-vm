#include <stdio.h>
#include "blake2b.h"

/*
 * We are including secp256k1 implementation directly so gcc can strip
 * unused functions. For some unknown reasons, if we link in libsecp256k1.a
 * directly, the final binary will include all functions rather than those used.
 */
#define HAVE_CONFIG_H 1
#define ECMULT_WINDOW_SIZE 6
#include <precomputed_ecmult.c>

#define ERROR_IO -1

int main(int argc, char* argv[]) {
    size_t pre_size = sizeof(secp256k1_pre_g);
    size_t pre128_size = sizeof(secp256k1_pre_g_128);

    FILE* fp_data = fopen("build/secp256k1_data", "wb");
    if (!fp_data) {
        return ERROR_IO;
    }
    fwrite(secp256k1_pre_g, pre_size, 1, fp_data);
    fwrite(secp256k1_pre_g_128, pre128_size, 1, fp_data);
    fclose(fp_data);

    FILE* fp = fopen("build/secp256k1_data_info.h", "w");
    if (!fp) {
        return ERROR_IO;
    }

    fprintf(fp, "#ifndef CKB_SECP256K1_DATA_INFO_H_\n");
    fprintf(fp, "#define CKB_SECP256K1_DATA_INFO_H_\n");
    fprintf(fp, "#define CKB_SECP256K1_DATA_SIZE %ld\n", pre_size + pre128_size);
    fprintf(fp, "#define CKB_SECP256K1_DATA_PRE_SIZE %ld\n", pre_size);
    fprintf(fp, "#define CKB_SECP256K1_DATA_PRE128_SIZE %ld\n", pre128_size);

    fprintf(fp, "static uint8_t ckb_secp256k1_data[] = {\n  ");
    unsigned char* p = (unsigned char*)secp256k1_pre_g;
    for (int i = 0; i < pre_size; i++) {
        fprintf(fp, "0x%02x", p[i]);
        fprintf(fp, ", ");
    }
    fprintf(fp, "\n");
    p = (unsigned char*)secp256k1_pre_g_128;
    for (int i = 0; i < pre128_size; i++) {
        fprintf(fp, "0x%02x", p[i]);
        if (i != (pre128_size - 1)) {
            fprintf(fp, ", ");
        }
    }

    fprintf(fp, "\n};\n");

    fprintf(fp, "#endif\n");
    fclose(fp);

    return 0;
}
