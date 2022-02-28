#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// The time intervals needed, sped up by 100
#define TERMINATION_TIME 30 * 24 * 60 * 60 / 100 // 30 days in seconds
#define BT_SEARCH_TIME 10 / 100 // 10 seconds
#define MIN_CLOSE_CONTACT_TIME 4 * 60 / 100 // 4 minutes in seconds
#define MAX_CLOSE_CONTACT_TIME 20 * 60 / 100 // 20 minutes in seconds
#define REMEMBER_TIME 14 * 24 * 60 * 60 / 100 // 14 days in seconds
#define COVID_TEST_TIME 4 * 60 * 60 / 100 // 4 hours in seconds

// The number of macaddresses created
#define SAMPLES 200
// Max number of recent contacts before they are deleted
#define QUEUESIZE (MAX_CLOSE_CONTACT_TIME / BT_SEARCH_TIME + 1)

/**
 * Structs
 **/
// The struct of the macaddress
typedef struct {
    unsigned char mac[6];
} macaddress;

typedef struct {
    macaddress address;
    struct timeval timestamp;
} contact;

typedef struct {
    contact* buf;
    int buf_size;
    int num_of_items;
    long head, tail;
    int full, empty;
    pthread_mutex_t* mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

struct pthread_args {
    queue* close_cont_q;
    queue* recent_cont_q;
};

/**
 * Queue functions
 **/
queue* queueInit(int buf_size)
{
    queue* q;

    q = (queue*)malloc(sizeof(queue));
    if (q == NULL)
        return (NULL);

    q->buf_size = buf_size;
    q->buf = (contact*)malloc(buf_size * sizeof(contact));
    q->num_of_items = 0;
    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mut, NULL);
    q->notFull = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notFull, NULL);
    q->notEmpty = (pthread_cond_t*)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);

    return (q);
}

void queueDelete(queue* q)
{
    free(q->buf);
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);
    free(q);
}

void queueAdd(queue* q, contact in)
{
    if (q->full == 1) {
        printf("Queue is full\nError\n");
        exit(-1);
    }

    q->buf[q->tail] = in;
    q->tail++;
    q->num_of_items++;

    if (q->tail == q->buf_size)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel(queue* q, contact* out)
{
    if (q->empty == 1) {
        printf("Queue is empty\nError\n");
        exit(-1);
    }
    *out = q->buf[q->head];

    q->num_of_items--;
    q->head++;
    if (q->head == q->buf_size)
        q->head = 0;
    if (q->head == q->tail)
        q->empty = 1;
    q->full = 0;

    return;
}

/**
 * General functions
 **/
// Checks if two macaddresses are the same
bool mac_equality(macaddress* a, macaddress* b)
{
    for (int i = 0; i < 6; i++) {
        if (a->mac[i] != b->mac[i])
            return false;
    }
    return true;
}

// Function that prints a contact
// Used for testing
void print_macaddress(contact* a)
{
    printf("\nMac address is: ");
    for (int i = 0; i < 6; i++) {
        printf("%X ", a->address.mac[i]);
    }
    printf("\nand it's time stamp is: ");
    printf("%ld.%06ld\n", a->timestamp.tv_sec, a->timestamp.tv_usec);
}

#endif