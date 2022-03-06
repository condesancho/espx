#include "covidTrace.h"
#include "createMacaddresses.h"
#include "utilities.h"

int main()
{
    printf("Creating the macaddresses\n");
    // Create file with macaddresses
    createMacaddresses();
    char* filename = "bin/macaddress.bin";
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Error: cannot open file macaddress.bin!\n");
        exit(-1);
    }

    printf("Importing macaddresses\n");
    // Import macaddresses from the file
    macaddress* addresses = (macaddress*)malloc(SAMPLES * sizeof(macaddress));
    for (int i = 0; i < SAMPLES; i++) {
        fread(&addresses[i], sizeof(macaddress), 1, fp);
    }

    queue* recent_contacts_queue = queueInit(QUEUESIZE);
    queue* close_contacts_queue = queueInit(SAMPLES + 1);

    struct pthread_args* arg = (struct pthread_args*)malloc(sizeof(struct pthread_args));
    arg->addresses = addresses;
    arg->close_cont_q = close_contacts_queue;
    arg->recent_cont_q = recent_contacts_queue;

    printf("Creating threads\n");
    pthread_t threads[4];
    int rc1, rc2, rc3, rc4;
    if ((rc1 = pthread_create(&threads[0], NULL, &timer, NULL))) {
        printf("Thread creation failed: %d\n", rc1);
        exit(-1);
    }

    if ((rc2 = pthread_create(&threads[1], NULL, &bluetooth_search, arg))) {
        printf("Thread creation failed: %d\n", rc2);
        exit(-1);
    }

    if ((rc3 = pthread_create(&threads[2], NULL, &covid_test, arg))) {
        printf("Thread creation failed: %d\n", rc3);
        exit(-1);
    }

    if ((rc4 = pthread_create(&threads[3], NULL, &delete_contacts, arg))) {
        printf("Thread creation failed: %d\n", rc4);
        exit(-1);
    }

    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }

    queueDelete(close_contacts_queue);
    queueDelete(recent_contacts_queue);
    free(addresses);
    fclose(fp);

    return 0;
}