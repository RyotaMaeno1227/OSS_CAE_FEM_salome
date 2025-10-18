#include <stdio.h>

int main(int argc, char **argv) {
    printf("FEM4C practice / ch01 hello\n");

    if (argc <= 1) {
        printf("usage: %s <input-file> [extra-args]\n", argv[0]);
        return 0;
    }

    printf("received %d argument(s):\n", argc - 1);
    for (int i = 1; i < argc; ++i) {
        printf("  arg[%d] = %s\n", i, argv[i]);
    }

    return 0;
}
