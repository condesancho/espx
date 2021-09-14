#include "utilities.h"

int main() {
    macaddress a;

    printf("The size of a.mac and a.timestamp is: %ld and %ld\n", sizeof(a.mac),
           sizeof(a.timestamp));

    return 0;
}