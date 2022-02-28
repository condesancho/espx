#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utilities.h"

void createMacaddresses()
{
    srand(time(NULL));

    FILE* f;
    // Create macaddress.bin file to store the random generated mac addresses
    f = fopen("macaddress.bin", "w");

    if (f == NULL) {
        printf("Error: cannot open file macaddress.bin!\n");
        exit(-1);
    }

    // Mac address is a 6 byte array of unsigned chars
    unsigned char mac[6];
    for (int i = 0; i < SAMPLES; i++) {
        for (int j = 0; j < 6; j++) {
            mac[j] = rand() % 256;
        }

        fwrite(&mac, 6 * sizeof(unsigned char), 1, f);
    }

    fclose(f);

    return;
}