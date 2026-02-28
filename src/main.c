#include "exec.h"

#include <stdio.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    return titanx_execute(&argv[1], (size_t)(argc - 1));
}
