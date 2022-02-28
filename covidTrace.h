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
        return NULL;
    }

    contact out;

    int num_of_iterations = close_contacts->num_of_items;
    for (int i = 0; i < num_of_iterations; i++) {
        queueDel(close_contacts, &out);
        fwrite(&out, sizeof(contact), 1, fp);
    }
    fclose(fp);

    return NULL;
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
        pthread_mutex_unlock(&bt_cond);

        // Time passed for covid test
        time_diff = (double)(current_time.tv_sec - covid_time.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - covid_time.tv_usec);
        time_diff = time_diff / 1e6;

        pthread_mutex_lock(&bt_mutex);
        if (time_diff >= COVID_TEST_TIME && covid_condition == 0) {

            covid_condition = 1;
            pthread_cond_signal(&covid_cond);

            // Reset timer
            gettimeofday(&covid_time, NULL);
        }
        pthread_mutex_unlock(&bt_cond);

        // Time passed for the termination of the program
        time_diff = (double)(current_time.tv_sec - termination_time.tv_sec) * 1e6;
        time_diff += (current_time.tv_usec - termination_time.tv_usec);
        time_diff = time_diff / 1e6;

        pthread_mutex_lock(&term_mutex);
        if (time_diff >= TERMINATION_TIME && term_condition == 0) {

            term_condition = 1;
            pthread_cond_signal(&term_condition);
            pthread_exit(NULL);
        }
        pthread_mutex_unlock(&term_mutex);
    }
}

void* bluetooth_search(void* args)
{
}
#endif