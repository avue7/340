/**
 * Name: AThit Vue
 * 
 * Description: Prototype of multi-lookup.cpp
 * 
 */
#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H
#include "queue.h" //Need to include for queue
#include <pthread.h> // Need for threads

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "util.h" //Need for dnslookup function

int main(int argc, char* argv[]);

/* Creates resolver threads */
void create_resolver_threads(FILE* outputfp);

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
void* insert_to_queue(void* inputfp);


/* Helper function for threads to join request thread pool. */
void join_request_pool();

/* Helper functio for threads to join the resolver thread pool */
void join_resolver_pool();


#endif