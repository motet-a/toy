#include "toy.h"

int main(int argc, const char **argv) {
    if (argc <= 1) {
        die("invoke with a file name");
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        die("cannot open the given file");
    }

    size_t max_file_size = 64 * 1000;
    char *source = xmalloc(max_file_size);
    size_t length = fread(source, 1, max_file_size, file);
    source[length] = 0;
    eval_source(source);
    fclose(file);
    free(source);
    return 0;
}
