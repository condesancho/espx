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

#define NUM_OF_SAMPLES 200
#define QUEUESIZE 150
#define TERMINATION_TIME 25920      // seconds
#define BT_SEARCH_TIME 0.1          // seconds
#define MIN_CLOSE_CONTACT_TIME 2.4  // seconds
#define MAX_CLOSE_CONTACT_TIME 12   // seconds
#define REMEMBER_TIME 12096         // seconds
#define COVID_TEST 144              // seconds

/***********   Structs   ***********/
// Struct that holds a 48 bit unsigned integer
struct uint48 {
    uint64_t x : 48;
} __attribute__((packed));

// The struct of the macaddress
typedef struct {
    struct uint48 mac;
    struct timeval timestamp;

} macaddress;

typedef struct {
    macaddress buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

/**
 * Queue functions
 **/
queue *queueInit(void) {
    queue *q;

    q = (queue *)malloc(sizeof(queue));
    if (q == NULL) return (NULL);

    q->empty = 1;
    q->full = 0;
    q->head = 0;
    q->tail = 0;
    q->mut = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(q->mut, NULL);
    q->notFull = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notFull, NULL);
    q->notEmpty = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(q->notEmpty, NULL);

    return (q);
}

void queueDelete(queue *q) {
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);
    free(q);
}

void queueAdd(queue *q, macaddress in) {
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE) q->tail = 0;
    if (q->tail == q->head) q->full = 1;
    q->empty = 0;

    return;
}

void queueDel(queue *q, macaddress *out) {
    *out = q->buf[q->head];

    q->head++;
    if (q->head == QUEUESIZE) q->head = 0;
    if (q->head == q->tail) q->empty = 1;
    q->full = 0;

    return;
}

#endif