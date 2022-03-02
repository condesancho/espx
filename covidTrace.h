#ifndef __COVIDTRACE_H__
#define __COVIDTRACE_H__

#include "utilities.h"

/**
 * Global variables
 **/
// Used for the testCOVID function
bool first_time = true;

pthread_mutex_t bt_mutex, covid_mutex, term_mutex;
pthread_cond_t bt_cond, covid_cond, term_cond;

// Extra variables for conditions
int bt_condition = 0;
int covid_condition = 0;
int term_condition = 0;

/**
 * Functions used by pthread functions
 */
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

void uploadContacts(queue* close_contacts)
{
    // If file doesn't exist create it and write
    FILE* fp = fopen("possible_covid_cases.bin", "ab+");

    if (close_contacts->empty == 1) {
        fclose(fp);
        return;
    }

    contact out;

    int num_of_iterations = close_contacts->num_of_items;
    for (int i = 0; i < num_of_iterations; i++) {
        queueDel(close_contacts, &out);
        fwrite(&out.address, sizeof(macaddress), 1, fp);
    }
    fclose(fp);

    return;
}

/**
 * Pthread functions
 */
void* timer(void* args)
{
    // Stores the starting time
    struct timeval bluetooth_time, covid_time, termination_time;
    gettimeofday(&bluetooth_time, NULL);
    covid_time = bluetooth_time;
    termination_time = bluetooth_time;

    struct timeval current_time;
    double time_diff = 0;

    while (1) {
        // Stores the current time
        gettimeofday(&current_time, NULL);

        // Time passed for bt search
        time_diff = (double)(current_time.tv_sec - bluetooth_time.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - bluetooth_time.tv_usec);
        time_diff = time_diff / 1e6;

        pthread_mutex_lock(&bt_mutex);
        if (time_diff >= BT_SEARCH_TIME && bt_condition == 0) {
            bt_condition = 1;
            pthread_cond_signal(&bt_cond);

            // Reset timer
            gettimeofday(&bluetooth_time, NULL);
        }
        pthread_mutex_unlock(&bt_mutex);

        // Time passed for covid test
        time_diff = (double)(current_time.tv_sec - covid_time.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - covid_time.tv_usec);
        time_diff = time_diff / 1e6;

        pthread_mutex_lock(&covid_mutex);
        if (time_diff >= COVID_TEST_TIME && covid_condition == 0) {
            covid_condition = 1;
            pthread_cond_signal(&covid_cond);

            // Reset timer
            gettimeofday(&covid_time, NULL);
        }
        pthread_mutex_unlock(&covid_mutex);

        // Time passed for the termination of the program
        time_diff = (double)(current_time.tv_sec - termination_time.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - termination_time.tv_usec);
        time_diff = time_diff / 1e6;

        pthread_mutex_lock(&term_mutex);
        if (time_diff >= TERMINATION_TIME && term_condition == 0) {
            term_condition = 1;
            pthread_cond_signal(&bt_cond);
            pthread_cond_signal(&covid_cond);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&term_mutex);
    }
}

// Function that runs a covid test every 4h signalled by the timer function
void* covid_test(void* arg)
{
    struct pthread_args* args = (struct pthread_args*)arg;
    queue* close_cont = args->close_cont_q;
    queue* recent_cont = args->recent_cont_q;

    while (1) {
        pthread_mutex_lock(&covid_mutex);
        while (!covid_condition || !term_condition) {
            pthread_cond_wait(&covid_cond, &covid_mutex);
        }
        covid_condition = 0;
        pthread_mutex_unlock(&covid_mutex);

        if (term_condition)
            pthread_exit(NULL);

        if (testCOVID()) {
            pthread_mutex_lock(&close_cont->mut);
            uploadContacts(close_cont);
            pthread_mutex_unlock(&close_cont->mut);
        }
    }
}

void* bluetooth_search(void* arg)
{
    struct pthread_args* args = (struct pthread_args*)arg;
    queue* close_cont = args->close_cont_q;
    queue* recent_cont = args->recent_cont_q;

    FILE* fp = fopen("timestamps.bin", "ab+");

    while (1) {
        pthread_mutex_lock(&bt_mutex);
        while (!bt_condition || !term_condition) {
            pthread_cond_wait(&bt_cond, &bt_mutex);
        }
        bt_condition = 0;
        pthread_mutex_unlock(&bt_mutex);

        if (term_condition) {
            fclose(fp);
            pthread_exit(NULL);
        }

        contact in;
        // TODO
        in.address = BTnearMe(addresses);
        gettimeofday(&in.timestamp, NULL);

        queueAdd(recent_cont, in);

        fwrite(&in, sizeof(contact), 1, fp);

        // TODO: check duplicate addresses between 4 and 20 min and add them to the close contacts
    }
}

void* delete_contacts(void* arg)
{
    struct pthread_args* args = (struct pthread_args*)arg;
    queue* close_cont = args->close_cont_q;
    queue* recent_cont = args->recent_cont_q;

    struct timeval current_time;
    struct timeval head_timestamp;
    contact out;
    double time_diff = 0;
    while (1) {
        // Stores the current time
        gettimeofday(&current_time, NULL);

        // Exit thread if termination occurs
        if (term_condition)
            pthread_exit(NULL);

        // Find the timestamp of the first contact in the queue
        head_timestamp = recent_cont->buf[recent_cont->head].timestamp;

        // Time passed from the moment a recent contact was added in the queue
        time_diff = (double)(current_time.tv_sec - head_timestamp.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - head_timestamp.tv_usec);
        time_diff = time_diff / 1e6;

        if (time_diff >= MAX_CONTACT_TIME) {
            queueDel(recent_cont, &out);
        }

        // Find the timestamp of the first close contact in the queue
        head_timestamp = close_cont->buf[close_cont->head].timestamp;

        // Time passed from the moment a recent contact was added in the queue
        time_diff = (double)(current_time.tv_sec - head_timestamp.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - head_timestamp.tv_usec);
        time_diff = time_diff / 1e6;

        if (time_diff >= REMEMBER_TIME) {
            queueDel(close_cont, &out);
        }

        usleep(1000);
    }
}

#endif