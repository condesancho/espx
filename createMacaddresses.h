#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utilities.h"

struct uint48 {
    uint64_t x : 48;
} __attribute__((packed));

void createMacaddresses() {
    srand(time(NULL));

    FILE *f;

    f = fopen("macaddress.bin", "w");

    if (f == NULL) {
        printf("Error: cannot open file macaddress.bin!\n");
        exit(-1);
    }

    for (int i = 0; i < NUM_OF_SAMPLES; i++) {
        uint64_t address = pow(2, 17) * (uint64_t)rand();

        struct uint48 mac;
        mac.x = address;

        fwrite(&mac, sizeof(struct uint48), 1, f);
    }

    fclose(f);

    return;
}