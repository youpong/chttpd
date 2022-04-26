#include "util.h"
#include <stdio.h>

int main(int argc, char **argv) {
    ArgsIter *iter = new_ArgsIter(argc, argv);

    printf("PROG_NAME: %s\n", ArgsIter_getProgName(iter));
    while (ArgsIter_hasNext(iter)) {
        printf("args: %s\n", ArgsIter_next(iter));
    }
    delete_ArgsIter(iter);

    return 0;
}
