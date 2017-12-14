/**
 * @file multi-lookup.h
 *
 * @brief
 *    Prototype for functions use in multi-lookup.c
 * 
 * @author Athit Vue
 * @date 11/20/2017
 */

#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h> // Need for threads
#include "queue.h" // Need to include for queue
#include "util.h"  // Need for dns lookup function

int main(int argc, char* argv[]);

/**
 * Start routine or start up function used by pthread_create
 * inside of create_resolver_threads(). Parses url of the domain name
 * that lives in the request queue to an outfile. 
 */
void* process_threads(void* p_outputfp);

/**
 * Start routine for request threads. Stores domain_name of each req_threads
 * in the queue until full. If full, thread sleeps and wait until not full.
 * Threads are stored in FIFO fashion.
 */
void* insert_to_queue(void* input_file);

////////////////////////////
// HELPER FUNCTIONS BELOW //
////////////////////////////

/* Helper function for threads calling pthread_join() */
void request_threads_join();

/* Helper functio for threads calling pthread_join()  */
void resolver_threads_join();


#endif

