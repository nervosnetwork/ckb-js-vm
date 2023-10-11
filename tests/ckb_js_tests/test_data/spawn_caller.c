#include "ckb_syscalls.h"

int main() {
    const char *argv[] = {"-f"};
    int8_t spawn_exit_code = -1;
    spawn_args_t spgs = {
        .memory_limit = 8,
        .exit_code = &spawn_exit_code,
        .content = NULL,
        .content_length = NULL,
    };
    int success = ckb_spawn(1, 3, 0, 1, argv, &spgs);
    if (success != 0) {
        return 1;
    }
    if (spawn_exit_code != 0) {
        return 1;
    }
    return 0;
}
