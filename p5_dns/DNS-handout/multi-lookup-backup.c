/**
 * @file multi-lookup.c
 * 
 * @brief
 *    Implementation of multi-threaded app that resolves 
 *    domain names to IP addresses.
 *
 * @note I am using the file lookup.c which is a non-threaded 
 *       version as a starting point. 
 * @author Athit Vue
 * @date 11/20/2017
 */

#include "multi-lookup.h" // All prototypes lives here.
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_INPUT_FILES 10
#define MAX_R_THREADS 10      // Max resolver threads
#define MIN_RESOLVER_THREADS 2 // Macro the min_reolver thread
#define DEBUG 0 // For debugging. Set to 1 if you want to debug.
#include <assert.h>

/*################### GLOBAL VARIABLES ###################################*/

int requested_threads=0; //Need a counter for requesters for (queing).
queue request_queue; //FIFO queue for requested threads...holds doman names.

// Flag needed for checking if all requester threads are done
int req_finished;

// Create a global data structure to hold the threads. Use pthread_t as type
pthread_t* request_pool; 
pthread_t* resolver_pool;

// Need to create two different mutex:
// One for the locking the resources tied to the threads in the
// request_queue and one for the output file.
pthread_mutex_t queue_mutex;
pthread_mutex_t output_mutex;


/*########################################################################*/

int main(int argc, char* argv[]){

    /* Local Vars */
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;
    char errorstr[SBUFSIZE];
    int i;
    int num_of_files = argc - 2;

    // Set the flag to 0
    req_finished = 0;
  
    // Initialize all mutex
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_mutex_init(&output_mutex, NULL);
 
    // Allocated space for request and resolver pools
    request_pool = malloc(sizeof(pthread_t)*num_of_files);
    resolver_pool = malloc(sizeof(pthread_t)*MAX_R_THREADS);        
 
    /* Initialize the request queue */
    if(queue_init(&request_queue, 1024) == QUEUE_FAILURE){
       fprintf(stderr, "Error: request_queue did not initialized correctly!!!\n");
       return EXIT_FAILURE;
    }

    /* Check Arguments */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }

    if(argc > MAX_INPUT_FILES + 1){
	fprintf(stderr, "Too many arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    } 

    // NEED TO CREATE THE CONSUMERS FIRST!!!! The consumers must be created to consume
    // as soon as there are things in the queue ready to be consume. The consumer will 
    // consume until all requester threads have joined and there are nothing left in 
    // the queue.
    int status;   
    for (i = 0; i < MAX_R_THREADS; i++)
    {
      status = pthread_create(&(resolver_pool[i]), NULL, process_threads, outputfp);
      if (status != 0)
      {
        fprintf(stderr, "Error: fail to created resolver thread[%d].\n", i);
      }
      else
      {
        if(DEBUG){
          printf("Successfully created resolver thread[%d].\n", i);
        }
      }
    }

    /* Loop Through Input Files */
    for(i = 0; i < num_of_files; i++)
    {
      
      // Open Input File
      inputfp = fopen(argv[i+1], "r");
      if(!inputfp)
      {
        sprintf(errorstr, "Error Opening Input File: %s", argv[i+1]);
        perror(errorstr);
        break;
      }
   
      // Create the requester pool threads
      status = pthread_create(&(request_pool[i]), NULL, insert_to_queue, inputfp);
      // Check status...
      if (status==0)
      {
        requested_threads += 1;
        if (DEBUG){
          printf("Successfully created requester_thread[%d]!\n", i);
        }
      }
      else
      {
        fprintf(stderr, "Error: failed at creating requester_thread[%d]!\n", i);
      }
      // Create the resolver pool threads
     // create_resolver_threads(outputfp);
    }

    // Wait for all threads to finish executiong before proceeding.
    request_threads_join();
    resolver_threads_join();
    
    // Close output file
    fclose(outputfp);
    // Free all resources
    free(request_pool);
    free(resolver_pool);
    queue_cleanup(&request_queue);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&output_mutex);
       
    return EXIT_SUCCESS;
}

/**
 * insert_to_queue
 * 
 * This function implements checking the queue for fullness and inserting 
 * the calling threads data into the queue whenever possible. Mutex locks
 * are used to protect other threads from accessing the critical area, the 
 * queue, while the current thread that has the lock is inserting it's 
 * information or data into the queue. If the queue is full, then the 
 * thread will go to sleep randomly for 0 - 100 microsecs. When it wakes 
 * up, it will attempt to get the lock again and resume until it has 
 * finished inserting all of its data into the queue (when the parser
 * for the file reaches EOF). 
 **/
void* insert_to_queue(void* inputfp)
{
  // Dynamically allocate new domain name
  char* domain_name;
  // Dynamically allocate new domain name
  domain_name = (char*) malloc(sizeof(char)*SBUFSIZE);

  while (fscanf((FILE*)inputfp, INPUTFS, domain_name) > 0)
  {
    // Lock the thread with the function inside pthread.h
    // Try to attain the lock for the queue.
    pthread_mutex_lock(&queue_mutex);

    //Check to see if queue is full
    if (queue_is_full(&request_queue))
    {
      //if(DEBUG){
          printf("Queue is full!!!!\n");
      //}
      // If full need to unlock so consumers can consume.
      pthread_mutex_unlock(&queue_mutex);
      //put current thread to sleep if full for random 0 - 100 microsecs.
      usleep(rand()%100);
      // Try to gain the lock again since we need to push data to the 
      // resolver queue after it wakes back up. 
      pthread_mutex_lock(&queue_mutex);
    }

    // Insert the domain_name at the end of the queue. 
    int status;
    status = queue_push(&request_queue, domain_name);
    if (status==0)
    {
      if (DEBUG)
      {
        printf("Succesfully pushed \"%s\" into the request_queue.\n", 
            domain_name);
      }
    }
    else
    {
      fprintf(stderr, "Error: failed to push \"%s\" int the request_queue.\n",
          domain_name);
    }

    // Done pushing domain_name to queue now release the queue so that the
    // other threads can have it. 
    pthread_mutex_unlock(&queue_mutex);
    // Dynamically allocated new domain_name for next iteration.
    domain_name = (char*) malloc(sizeof(char)*SBUFSIZE);
  }

  // Once our parser is at EOF, close the current file and free all resources
  int status;
  status = fclose((FILE*)inputfp);

  if (status)
  {
    fprintf (stderr, "ERROR: FAILED TO CLOSE FILE AT INSERT TO QUEUE FUNCTION!\n");
  }


  // Free the resources allocated by malloc
  free(domain_name);
  // Call exit on thread to terminate the calling thread (free it)
  pthread_exit(NULL);
  //return NULL;  
}

/**
 * void process_threads
 *
 * The consumer threads (resolver threads) will keep on consuming until the 
 * the queue is empty and req_finished flag is set to 1. When req_finished
 * flag is set to one, we know that the requester threads have completed 
 * their tasks and have exited. For this function, we need to have a mutex
 * lock to protect the shared output file as well as the queue. We only 
 * want one thread accessing each critical area at once or deadlock and race
 * condition may occur. 
 */
void* process_threads(void* p_outputfp)
{
  //Declare local Vars
  char* domain_name;
  char domain_url[INET6_ADDRSTRLEN];
    
  // We need an infinite while loop to keep each resolver thread alive 
  // while the queue is not empty. When the queue becomes empty, we need
  // to check to see if all the requester threads are done. When all of the
  // requester threads are done, then all the resolving threads can exit.
  while(1)
  {  
    // Obtain the lock for the queue
    pthread_mutex_lock(&queue_mutex);
    /*
    * If request_queue is not empty pop off each domain name
    * and use dnslookup to look for it's ip address. 
    */
    if (!queue_is_empty(&request_queue))
    {
      //queue_pop accepts pointer to request_queue and returns void* payload
      domain_name = (char*) queue_pop(&request_queue);
      if (domain_name)
      {
        if (DEBUG){
          printf("Successfully popped \"%s\" out of resolver_queue.\n",
              domain_name);
        }
      }
      else
      {
        fprintf(stderr, "Error: unsuccessful at popping \"%s\" out of", 
            domain_name);
        fprintf(stderr, "resolver_queue.\n");
      }
      // Unlock requester_queue so other threads can access it.
      pthread_mutex_unlock(&queue_mutex);

      // Look for ip address using dns lookup. 
      int status;
      status = dnslookup(domain_name, domain_url, sizeof(domain_url));
      if (status == -1 || status == UTIL_FAILURE)
      {
        fprintf(stderr, "  cannot find: \"%s\".\n", domain_name);
        strncpy(domain_url, "", sizeof(domain_url));
      }
      else
      {
        if(DEBUG){
            printf("Successfully looked up \"%s\".\n", domain_name);
        }
      }

      // Try to get the lock for the output file to write to it.
      pthread_mutex_lock(&output_mutex);
      fprintf ( (FILE*)p_outputfp, "%s,%s\n", domain_name, domain_url);
      pthread_mutex_unlock(&output_mutex);
      // Free domain_name after successful write.
      free (domain_name);
      domain_name = NULL;
    }
    else // Request queue is empty, unlock and check to see if need to exit.
    {
      pthread_mutex_unlock(&queue_mutex);
      if (req_finished)
      {
        pthread_exit(NULL);
      }
    }
  }
}

/**
 * void request_threads_join
 * 
 * This function calls the pthread_join() for the request threads.
 * It will wait for all the request threads to finished excuting 
 * before moving on to the next line of code. 
 */
void request_threads_join()
{
  int request_thread, status;
  for (request_thread = 0; request_thread < requested_threads; request_thread++)
  {
    status = pthread_join(request_pool[request_thread], NULL);
    if (status==0)
    {
      if(DEBUG){
          printf("Request_thread[%d] finished, now waiting...!\n", request_thread);
      }
    }
    else
    {
      fprintf(stderr, "Error: failed called to pthread_join for ");
      fprintf(stderr, "request_thread[%d]!!!\n", request_thread);
    }
  } 
  // Need this to check and see if the resolver thread will need to be exited
  // in the process_thread function.
  req_finished = 1;
}

/**
 * void resolver_threads_join
 * 
 * This function calls the pthread_join() for the resolver threads.
 * It will wait for all the resolver threads to finished excuting 
 * before moving on to the next line of code. 
 */
void resolver_threads_join()
{
  int resolver_thread, status;
  for (resolver_thread = 0; resolver_thread < MAX_INPUT_FILES; resolver_thread++)
  {
    status = pthread_join(resolver_pool[resolver_thread], NULL);
    if (status==0)
    {
      if(DEBUG){
          printf("Resolver_thread[%d] waiting...!\n", resolver_thread);
      }
    }
    else
    {
      fprintf(stderr, "Error: failed called to pthread_join for ");
      fprintf(stderr, "resolver_thread[%d]!!!\n", resolver_thread);
    }
  } 
}
