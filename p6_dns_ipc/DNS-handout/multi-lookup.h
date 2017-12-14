/**
 * Name: AThit Vue
 * 
 * Description: Prototype of multi-lookup.cpp
 * 
 */
#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "util.h" //Need for dnslookup function
#include <sys/ipc.h>  // For semaphores and shared memory
#include <sys/wait.h> // For wait()
#include <sys/shm.h>  // For shared memory
#include <sys/stat.h> // For S_IRUSR and S_IWUSR options
#include <sys/mman.h> 
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// Main function
int main(int argc, char* argv[]);

// Function to create shared memory segment
void* create_shared_memory(size_t size);

// Function use to initialize mutexes and condtional variables
void init();

// Recursive function use to fork a children processes
void fork_children(int nprocess, void* p_outputfp, void* p_inputfp,
    int child_count);

// Function for child process (consumers) to consume
void consume(int cons_rank, void* p_outputfp);

// Function for parent process (producers) to produce
void produce(void* p_inputfp);

#endif

