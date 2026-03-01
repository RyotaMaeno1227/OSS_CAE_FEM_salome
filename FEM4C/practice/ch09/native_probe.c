#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int read_next_data_line(FILE *fp, char *buf, size_t size)
{
    while (fgets(buf, (int)size, fp)) {
        size_t len = strlen(buf);
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = '\0';
        }
        char *p = buf;
        while (*p && isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0' || *p == '#') {
            continue;
        }
        if (p != buf) {
            memmove(buf, p, strlen(p) + 1);
        }
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <native_input>\n", argv[0]);
        return 1;
    }

    /*
     * Example:
     *   ./native_probe examples/t3_cantilever_beam.dat
     * Expected log (example):
     *   Title: T3 Cantilever Beam
     *   Declared nodes: 297 (read 297)
     *   Declared elements: 512 (read 512)
     */

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    char line[512];
    char title[256] = {0};
    int num_nodes = 0;
    int num_elements = 0;

    if (!read_next_data_line(fp, line, sizeof(line))) {
        fprintf(stderr, "no title line found\n");
        fclose(fp);
        return 1;
    }
    strncpy(title, line, sizeof(title) - 1);

    if (!read_next_data_line(fp, line, sizeof(line))) {
        fprintf(stderr, "no size line found\n");
        fclose(fp);
        return 1;
    }
    if (sscanf(line, "%d %d", &num_nodes, &num_elements) != 2) {
        fprintf(stderr, "invalid size line: %s\n", line);
        fclose(fp);
        return 1;
    }

    int node_count = 0;
    for (int i = 0; i < num_nodes; ++i) {
        if (!fgets(line, sizeof(line), fp)) {
            break;
        }
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r') {
            i--;
            continue;
        }
        node_count++;
    }

    int elem_count = 0;
    for (int i = 0; i < num_elements; ++i) {
        if (!fgets(line, sizeof(line), fp)) {
            break;
        }
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r') {
            i--;
            continue;
        }
        elem_count++;
    }

    printf("Title: %s\n", title);
    printf("Declared nodes: %d (read %d)\n", num_nodes, node_count);
    printf("Declared elements: %d (read %d)\n", num_elements, elem_count);

    fclose(fp);
    return 0;
}
