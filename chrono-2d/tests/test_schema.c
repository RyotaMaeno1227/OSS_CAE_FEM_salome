#include <stdio.h>
#include <string.h>

int main(void) {
    const char *expected =
        "time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot,vn,vt,mu_s,mu_d,stick\n";
    FILE *fp = fopen("artifacts/kkt_descriptor_actions_local.csv", "r");
    if (!fp) {
        fprintf(stderr, "Descriptor CSV not found; run test_coupled_constraint first.\n");
        return 1;
    }
    char header[256];
    if (!fgets(header, sizeof(header), fp)) {
        fclose(fp);
        fprintf(stderr, "Empty descriptor CSV\n");
        return 1;
    }
    fclose(fp);
    if (strcmp(header, expected) != 0) {
        fprintf(stderr, "Schema mismatch in artifacts/kkt_descriptor_actions_local.csv\n");
        return 1;
    }
    printf("Schema test passed.\n");
    return 0;
}
