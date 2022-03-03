#ifndef __COVIDTRACE_H__
#define __COVIDTRACE_H__

#include "utilities.h"

/**
 * Global variables
 **/
// Used for the testCOVID function
bool first_time = true;

pthread_mutex_t bt_mutex, covid_mutex, term_mutex;
pthread_cond_t bt_cond, covid_cond;

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

    int num_of_iterations = find_queue_size(close_contacts);
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
/**
 * @brief Function used by a pthread that:
 * 1) Signals another thread to execute the bluetooth search
 * 2) Signals another thread to execute the covid test
 * 3) Signals the termination of all threads
 *
 * @return void*
 */
void* timer()
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
        time_diff = time_difference(current_time, bluetooth_time);

        // pthread_mutex_lock(&bt_mutex);
        if (time_diff >= BT_SEARCH_TIME) {
            printf("Searching\n");
            bt_condition = 1;
            pthread_cond_signal(&bt_cond);

            // Reset timer
            gettimeofday(&bluetooth_time, NULL);
        }
        // pthread_mutex_unlock(&bt_mutex);

        // Time passed for covid test
        time_diff = time_difference(current_time, covid_time);

        // pthread_mutex_lock(&covid_mutex);
        if (time_diff >= COVID_TEST_TIME) {
            printf("Testing\n");
            covid_condition = 1;
            pthread_cond_signal(&covid_cond);

            // Reset timer
            gettimeofday(&covid_time, NULL);
        }
        // pthread_mutex_unlock(&covid_mutex);

        // Time passed for the termination of the program
        time_diff = time_difference(current_time, termination_time);

        // pthread_mutex_lock(&term_mutex);
        if (time_diff >= TERMINATION_TIME) {
            printf("Terminating all threads\n");
            term_condition = 1;
            pthread_cond_signal(&bt_cond);
            pthread_cond_signal(&covid_cond);
            pthread_exit(NULL);
        }
        // pthread_mutex_unlock(&term_mutex);
    }
}

/**
 * @brief Function that runs a covid test every 4h signalled by the timer function
 *
 * @param arg
 * @return void*
 */
void* covid_test(void* arg)
{
    struct pthread_args* args = (struct pthread_args*)arg;
    queue* close_cont = args->close_cont_q;
    queue* recent_cont = args->recent_cont_q;

    while (1) {
        pthread_mutex_lock(&covid_mutex);
        while (!covid_condition && !term_condition) {
            pthread_cond_wait(&covid_cond, &covid_mutex);
        }
        covid_condition = 0;
        pthread_mutex_unlock(&covid_mutex);

        if (term_condition)
            pthread_exit(NULL);

        if (testCOVID()) {
            printf("Test occurred positive uploading contacts\n");
            // pthread_mutex_lock(&close_cont->mut);
            uploadContacts(close_cont);
            // pthread_mutex_unlock(&close_cont->mut);
        }
    }
}

/**
 * @brief Function used by a pthread that is signalled from the timer function
 * and adds a random macaddress to the contact queue
 *
 * @param arg
 * @return void*
 */
void* bluetooth_search(void* arg)
{
    // Convert pthread arguements
    struct pthread_args* args = (struct pthread_args*)arg;
    queue* close_cont = args->close_cont_q;
    queue* recent_cont = args->recent_cont_q;
    macaddress* addresses = args->addresses;

    contact in;

    while (1) {
        pthread_mutex_lock(&bt_mutex);
        while (!bt_condition && !term_condition) {
            pthread_cond_wait(&bt_cond, &bt_mutex);
            printf("yo\n");
        }
        bt_condition = 0;
        pthread_mutex_unlock(&bt_mutex);

        if (term_condition) {
            pthread_exit(NULL);
        }

        // Fill the contact that will go in the queue
        in.address = BTnearMe(addresses);
        gettimeofday(&in.timestamp, NULL);

        queueAdd(recent_cont, in);
    }
}

/**
 * @brief Function used by a pthread that deletes the contacts and close contacts
 * that passed the corresponding time interval
 *
 * @param arg
 * @return void*
 */
void* delete_contacts(void* arg)
{
    // Convert pthread arguements
    struct pthread_args* args = (struct pthread_args*)arg;
    queue* close_cont = args->close_cont_q;
    queue* recent_cont = args->recent_cont_q;

    FILE* fp = fopen("timestamps.bin", "ab+");

    struct timeval current_time;
    struct timeval head_timestamp;
    contact out;
    double time_diff = 0;
    double time_stamp = 0;

    while (1) {
        // Stores the current time
        gettimeofday(&current_time, NULL);

        // Exit thread if termination occurs
        if (term_condition) {
            fclose(fp);
            pthread_exit(NULL);
        }

        // Find the timestamp of the first contact in the queue
        head_timestamp = recent_cont->buf[recent_cont->head].timestamp;

        // Time passed from the moment a recent contact was added in the queue
        time_diff = time_difference(current_time, head_timestamp);

        if (time_diff >= MAX_CONTACT_TIME) {
            queueDel(recent_cont, &out);
            // Save the timestamp of the contact to a file
            time_stamp = timeval2double(out.timestamp);
            fwrite(&time_stamp, sizeof(double), 1, fp);
        }

        // Find the timestamp of the first close contact in the queue
        head_timestamp = close_cont->buf[close_cont->head].timestamp;

        // Time passed from the moment a close contact was added in the queue
        time_diff = time_difference(current_time, head_timestamp);

        if (time_diff >= REMEMBER_TIME) {
            queueDel(close_cont, &out);
        }

        usleep(1000);
    }
}

#endif