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
#define BT_SEARCH_TIME 10 / 10 // 10 seconds
#define MIN_CONTACT_TIME 4 * 60 / 100 // 4 minutes in seconds
#define MAX_CONTACT_TIME 20 * 60 / 100 // 20 minutes in seconds
#define REMEMBER_TIME 14 * 24 * 60 * 60 / 100 // 14 days in seconds
#define COVID_TEST_TIME 4 * 60 * 60 / 100 // 4 hours in seconds

// The number of macaddresses created
#define SAMPLES 200
// Max number of recent contacts before they are deleted
#define QUEUESIZE 200

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
    long head, tail;
    int full, empty;
    pthread_mutex_t* mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

struct pthread_args {
    queue* close_cont_q;
    queue* recent_cont_q;
    macaddress* addresses;
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

    if (q->tail == q->buf_size)
        q->tail = 0;
    if (q->tail == q->head)
        q->full = 1;
    q->empty = 0;

    return;
}

void queueDel(queue* q, contact* out)
{
    *out = q->buf[q->head];

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
// Returns the time difference of 2 timevals
double time_difference(struct timeval start, struct timeval end)
{
    double time_diff;
    // Time passed from the moment a recent contact was added in the queue
    time_diff = (double)(end.tv_sec - start.tv_sec) * 1e6;
    time_diff += (double)(end.tv_usec - start.tv_usec);
    time_diff = time_diff / 1e6;
    return time_diff;
}

// Checks if two macaddresses are the same
bool mac_equality(macaddress* a, macaddress* b)
{
    for (int i = 0; i < 6; i++) {
        if (a->mac[i] != b->mac[i])
            return false;
    }
    return true;
}

// Finds if a new contact should be considered as a close contact
bool close_contact_found(contact in_q, contact new_contact)
{
    double time_diff = time_difference(in_q.timestamp, new_contact.timestamp);
    if (mac_equality(&in_q.address, &new_contact.address)) {
        if (time_diff >= MIN_CONTACT_TIME && time_diff < MAX_CONTACT_TIME) {
            return true;
        }
    }
    return false;
}

// Function that prints a contact
// Used for testing
void print_contact(contact* a)
{
    printf("\nMac address is: ");
    for (int i = 0; i < 6; i++) {
        printf("%X ", a->address.mac[i]);
    }
    printf("\nand it's time stamp is: ");
    printf("%ld.%06ld\n", a->timestamp.tv_sec, a->timestamp.tv_usec);
}

// Function that returns the size of a queue buffer
int find_queue_size(queue* q)
{
    // Return size if queue is empty or full
    if (q->empty) {
        return 0;
    }
    if (q->full) {
        return q->buf_size;
    }

    // Size depending on the position of the tail and the head
    if (q->head < q->tail) {
        return (q->tail - q->head);
    } else if (q->head > q->tail) {
        return (q->buf_size - q->head + q->tail);
    }
}

// Converts timeval to double
double timeval2double(struct timeval tv)
{
    double tv_double = 0;
    tv_double = (double)tv.tv_sec * 1e6 + (double)tv.tv_usec;
    tv_double = tv_double / 1e6;
    return tv_double;
}

#endif