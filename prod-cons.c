/*
 *	File	: pc.c
 *
 *	Title	: Demo Producer/Consumer.
 *
 *	Short	: A solution to the producer consumer problem using
 *		pthreads.
 *
 *	Long 	:
 *
 *	Author	: Andrae Muys
 *
 *	Date	: 18 September 1997
 *
 *	Revised	:
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define QUEUESIZE 1000
#define LOOP 100

void *producer(void *args);
void *consumer(void *args);

struct workFunction {
    void *(*work)(void *);
    void *arg;
    struct timeval start, end;
    double elapsed;
};

typedef struct {
    struct workFunction buf[QUEUESIZE];
    double *times;
    int index;
    int prod, finished_prod;
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
} queue;

void *work(void *arg) {
    struct workFunction *data = (struct workFunction *)arg;

    struct timeval begin, end;
    begin = data->start;
    end = data->end;

    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    double elapsed;

    if (seconds == 0) {
        elapsed = microseconds;
        printf("Time elapsed: %.0lf microseconds.\n", elapsed);
    } else {
        elapsed = seconds + microseconds * 1e-6;
        printf("Time elapsed: %lf seconds.\n", elapsed);
    }

    data->elapsed = elapsed;

    return (NULL);
}

queue *queueInit(int producers);
void queueDelete(queue *q);
void queueAdd(queue *q, struct workFunction in);
void queueDel(queue *q, struct workFunction *out);

int main(int argc, char *argv[]) {
    queue *fifo;

    int p = atoi(argv[1]);
    int q = atoi(argv[2]);
    int max = 0;
    pthread_t pro[p];
    pthread_t con[q];

    fifo = queueInit(p);
    if (fifo == NULL) {
        fprintf(stderr, "main: Queue Init failed.\n");
        exit(1);
    }
    if (p > q) {
        max = p;
    } else {
        max = q;
    }

    for (int i = 0; i < max; i++) {
        if (i < p) {
            pthread_create(&pro[i], NULL, producer, fifo);
        }
        if (i < q) {
            pthread_create(&con[i], NULL, consumer, fifo);
        }
    }

    for (int i = 0; i < max; i++) {
        if (i < p) {
            pthread_join(pro[i], NULL);
        }
        if (i < q) {
            pthread_join(con[i], NULL);
        }
    }

    double avg = 0;

    for (int i = 0; i < p * LOOP; i++) {
        avg += fifo->times[i];
    }
    avg = avg / (p * LOOP);
    printf("The average wait time is: %lf microseconds.\n", avg);

    queueDelete(fifo);

    return 0;
}

void *producer(void *q) {
    queue *fifo;
    int i;

    fifo = (queue *)q;

    for (i = 0; i < LOOP; i++) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->full) {
            printf("producer: queue FULL.\n");
            pthread_cond_wait(fifo->notFull, fifo->mut);
        }

        struct workFunction input;
        input.work = work;
        input.arg = &input;
        gettimeofday(&input.start, NULL);

        queueAdd(fifo, input);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notEmpty);
    }

    pthread_mutex_lock(fifo->mut);
    fifo->finished_prod++;
    pthread_mutex_unlock(fifo->mut);

    return (NULL);
}

void *consumer(void *q) {
    queue *fifo;
    int i;

    fifo = (queue *)q;

    while (1) {
        pthread_mutex_lock(fifo->mut);
        while (fifo->empty) {
            printf("consumer: queue EMPTY.\n");
            if (fifo->finished_prod == fifo->prod) {
                pthread_cond_signal(fifo->notEmpty);
                pthread_mutex_unlock(fifo->mut);
                return (NULL);
            }
            pthread_cond_wait(fifo->notEmpty, fifo->mut);
        }

        struct workFunction out;

        queueDel(fifo, &out);
        pthread_mutex_unlock(fifo->mut);
        pthread_cond_signal(fifo->notFull);
        printf("consumer: received.\n");
    }
}

/*
    typedef struct {
    struct workFunction buf[QUEUESIZE];
    long head, tail;
    int full, empty;
    pthread_mutex_t *mut;
    pthread_cond_t *notFull, *notEmpty;
    } queue;
*/

queue *queueInit(int producers) {
    queue *q;

    q = (queue *)malloc(sizeof(queue));
    if (q == NULL) return (NULL);

    q->prod = producers;
    q->finished_prod = 0;
    q->times = (double *)malloc(producers * LOOP * sizeof(double));
    if (q->times == NULL) {
        printf("Unable to initialize times array.\n");
        exit(-1);
    }
    q->index = 0;

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
    free(q->times);
    pthread_mutex_destroy(q->mut);
    free(q->mut);
    pthread_cond_destroy(q->notFull);
    free(q->notFull);
    pthread_cond_destroy(q->notEmpty);
    free(q->notEmpty);
    free(q);
}

void queueAdd(queue *q, struct workFunction in) {
    q->buf[q->tail] = in;
    q->tail++;
    if (q->tail == QUEUESIZE) q->tail = 0;
    if (q->tail == q->head) q->full = 1;
    q->empty = 0;

    return;
}

void queueDel(queue *q, struct workFunction *out) {
    *out = q->buf[q->head];
    gettimeofday(&out->end, NULL);
    out->arg = (void *)out;
    out->work(out->arg);
    q->times[q->index] = out->elapsed;
    q->index++;

    q->head++;
    if (q->head == QUEUESIZE) q->head = 0;
    if (q->head == q->tail) q->empty = 1;
    q->full = 0;

    return;
}
