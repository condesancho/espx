#ifndef __COVIDTRACE_H__
#define __COVIDTRACE_H__

#include "utilities.h"

// Used for the testCOVID function
bool first_time = true;

int condition = 0;

pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_cond = PTHREAD_COND_INITIALIZER;

// Returns random macaddress from an array of addressess
macaddress BTnearMe(macaddress* addresses)
{
    int rand_idx = rand() % SAMPLES;

    return addresses[rand_idx];
}

// Function that ruturns true the first time it is called and after with a 60% chance
bool testCOVID()
{
    bool positive_test = false;

    if (first_time) {
        first_time = false;
        positive_test = true;
        return positive_test;
    }

    // Posibility of positive test is 60%
    if (rand() % 10 > 3) {
        positive_test = true;
    }

    return positive_test;
}

void* timer(void* args)
{
    // Stores the starting time
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    struct timeval current_time;
    double time_diff = 0;

    while (1) {

        gettimeofday(&current_time, NULL);
        time_diff = (double)(current_time.tv_sec - start_time.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - start_time.tv_usec);
        // Time passed in seconds
        time_diff = time_diff / 1e6;

        if (time_diff >= 10) {
            pthread_exit(NULL);
        }

        pthread_mutex_lock(&condition_mutex);
        if (time_diff >= 5) {
            condition = 1;
            pthread_cond_signal(&condition_cond);
        }
        pthread_mutex_unlock(&condition_mutex);
    }
}

void* wake_up(void* args)
{
    // pthread_mutex_lock(&condition_mutex);
    while (!condition) {
        pthread_cond_wait(&condition_cond, &condition_mutex);
    }
    // pthread_mutex_lock(&condition_mutex);

    printf("Wake up\n");
}

#endif