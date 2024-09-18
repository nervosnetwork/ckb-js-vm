#include "ckb_syscalls.h"

int main() {
    int err = 0;
    const char *argv[] = {"-f"};
    uint64_t pid = 0;
    uint64_t inherited_fds[1] = {0};
    int8_t exit_code = 0;
    spawn_args_t spgs = {
        .argc = 1,
        .argv = argv,
        .process_id = &pid,
        .inherited_fds = inherited_fds,
    };
    err = ckb_spawn(1, CKB_SOURCE_CELL_DEP, 0, 0, &spgs);
    if (err != 0) {
        return 1;
    }
    err = ckb_wait(pid, &exit_code);
    if (err != 0) {
        return 1;
    }
    return 0;
}
