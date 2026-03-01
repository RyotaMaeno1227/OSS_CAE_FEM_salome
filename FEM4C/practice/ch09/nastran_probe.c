#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static int starts_with(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void trim_leading(char *s)
{
    char *p = s;
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <nastran_bdf>\n", argv[0]);
        return 1;
    }

    /*
     * Example:
     *   ./nastran_probe NastranBalkFile/3Dtria_example.dat
     * Expected log (example):
     *   GRID:   451
     *   CTRIA3: 800
     *   MAT1:   1
     *   SPC:    11
     *   FORCE:  1
     */

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror("fopen");
        return 1;
    }

    int grid = 0;
    int ctria3 = 0;
    int ctria6 = 0;
    int cquad4 = 0;
    int pshell = 0;
    int mat1 = 0;
    int spc = 0;
    int spc1 = 0;
    int force = 0;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        trim_leading(line);
        if (line[0] == '$' || line[0] == '\0' || line[0] == '\n') {
            continue;
        }
        if (starts_with(line, "GRID*")) {
            grid++;
            continue;
        }
        if (starts_with(line, "GRID")) {
            grid++;
            continue;
        }
        if (starts_with(line, "CTRIA6")) {
            ctria6++;
            continue;
        }
        if (starts_with(line, "CTRIA3")) {
            ctria3++;
            continue;
        }
        if (starts_with(line, "CQUAD4")) {
            cquad4++;
            continue;
        }
        if (starts_with(line, "PSHELL")) {
            pshell++;
            continue;
        }
        if (starts_with(line, "MAT1")) {
            mat1++;
            continue;
        }
        if (starts_with(line, "SPC1")) {
            spc1++;
            continue;
        }
        if (starts_with(line, "SPC")) {
            spc++;
            continue;
        }
        if (starts_with(line, "FORCE")) {
            force++;
            continue;
        }
    }

    fclose(fp);

    printf("GRID:   %d\n", grid);
    printf("CTRIA3: %d\n", ctria3);
    printf("CTRIA6: %d\n", ctria6);
    printf("CQUAD4: %d\n", cquad4);
    printf("PSHELL: %d\n", pshell);
    printf("MAT1:   %d\n", mat1);
    printf("SPC:    %d\n", spc);
    printf("SPC1:   %d\n", spc1);
    printf("FORCE:  %d\n", force);
    return 0;
}
