/**
 * Name: Athit Vue
 *
 * Description: Implementation of multi-threaded app that resolves
 *              domain names to IP addresses. 
 *
 * Please note: I am using the file lookup.c by Andy Sayler as a starting point.
 * Other resources used:
 *     #http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread.h.html
 **/
#include "multi-lookup.h"
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_R_THREADS 10 //Macro the max_resolver thread
#define MIN_REOLVER_THREADS 2 // Macro the min_reolver thread
#define MAX_INPUT_FILES 10


/*################### GLOBAL VARIABLES ###################################*/

int requested_threads=0; //Need a counter for requesters for (queing).
queue request_queue; //FIFO queue for requested threads...holds doman names.

int req_finished;

// Create a globa data structure to hold the threads. Use pthread_t as type
pthread_t request_pool[MAX_R_THREADS];
pthread_t resolver_pool[MAX_R_THREADS];

// This cost me lots of time...Need to create two different mutex locks
// One for the locking the resources tied to the threads in the request_queue
// and one for the process_thread output block.
pthread_mutex_t queue_mutex;
pthread_mutex_t output_mutex;


/*########################################################################*/

int main(int argc, char* argv[]){

    /* Local Vars */
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;
    char errorstr[SBUFSIZE];
    //char firstipstr[INET6_ADDRSTRLEN];
    queue_init(&request_queue, 50);
    int i;         
    req_finished = 0;

    /* Check Arguments */
    if(argc < MINARGS){
	fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
	fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
	return EXIT_FAILURE;
    }

    /* Open Output File */
    outputfp = fopen(argv[(argc-1)], "w");
    if(!outputfp){
	perror("Error Opening Output File");
	return EXIT_FAILURE;
    }

    /* Following the diagram on instruction...must check for response */
    create_resolver_threads(outputfp);

    /* Loop Through Input Files */
    for(i=1; i<(argc-1); i++)
    {
      /* Open Input File */
      inputfp = fopen(argv[i], "r");
      if(!inputfp)
      {
        sprintf(errorstr, "Error Opening Input File: %s", argv[i]);
        perror(errorstr);
        break;
      }
 
      /*
      * Need a request thread pool. Contains set of name files
      * which contains list of domain names. Each name of file is 
      * place into a FIFO queue (already provided). Must put thread
      * to sleep if tries to write to the queue when queue is full;
      * Sleep for random period of time between 0 and 100 microsecs.
      */
        
      // First create threads for each item in file and insert threads
      // name into the request_queue.
      int status;
      status = pthread_create(&(request_pool[i-1]), NULL, insert_to_queue, inputfp);
      // Check status...
      if (status==0)
      {
        requested_threads += 1;
      }
      else
      {
        fprintf(stderr, "Error: failed at creating requester_thread[%d]!\n", i-1);
      } 
    }
        
    // If request_queue is full, thread needs to join
    join_request_pool();

    join_resolver_pool();

    fclose(outputfp);
       
    return EXIT_SUCCESS;
}


/* Create resolver threads for parsing to outfile */
void create_resolver_threads(FILE* outputfp)
{
  int status;
  int i;
  for (i = 0; i < MAX_R_THREADS; i++)
  {
    /* #Reference:
       int pthread_create (pthread_t *thread, const pthread_attr_t *attr,
                           void *(*start_routine)(void*), void *arg);
    */
    status = pthread_create(&(resolver_pool[i]), NULL, process_threads, outputfp);
    if (status != 0)
    {
      fprintf(stderr, "Error: fail to created resolver thread[%d].\n", i);
    }
  }
}

void* process_threads(void* p_outputfp)
{
  //Declare local Vars
  char* domain_name;
  char domain_url[INET6_ADDRSTRLEN];
  // Need while loop..
  while(1)
  { 
    //Lock the resources
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
        //printf("Successfully popped \"%s\" out of resolver_queue.\n", domain_name);
      }
      else
      {
        fprintf(stderr, "Error: unsuccessful at popping \"%s\" out of resolver_queue.\n", domain_name);
      }
      // Unlock req_queue
      pthread_mutex_unlock(&queue_mutex);

      // Look for ip address using dns lookup. 
      int status;
      status = dnslookup(domain_name, domain_url, sizeof(domain_url));
      if (status == -1 || status == UTIL_FAILURE)
      {
        fprintf(stderr, "  cannot find: \"%s\".\n", domain_name);
        //domain_url = "";
        strncpy(domain_url, "", sizeof(domain_url));
      }
      else
      {
        //printf("Successfully looked up \"%s\".\n", domain_name);
      }
      // Unlock the output thread resources
      pthread_mutex_lock(&output_mutex);
      fprintf ( (FILE*)p_outputfp, "%s,%s\n", domain_name, domain_url);
      pthread_mutex_unlock(&output_mutex);
      free (domain_name);
    }
    else
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
 * Use a while loop to iterate thru the files. For each line (thread),
 * we need to set a mutex to lock the thread until finished. If the 
 * FIFO queue is full, we need to tell the thread to go to sleep (wait)
 * for 0-100 microsecs. Once, the thread is done then unlock and go to
 * next domain (thread) and repeat until EOF, makeing sure to 
 * free memory.
 **/
void* insert_to_queue(void* inputfp)
{
  // Dynamically allocate new domain name
  char* domain_name;
  domain_name = (char*) malloc (SBUFSIZE);
  while (fscanf( (FILE*)inputfp, INPUTFS, domain_name) > 0)
  {
    //Lock the thread with the function inside pthread.h
    pthread_mutex_lock(&queue_mutex);
    //Chek and see if queue is full
    if (queue_is_full(&request_queue))
    {
      pthread_mutex_unlock(&queue_mutex);
      //put current thread to sleep if full
      usleep(rand()%100);
      pthread_mutex_lock(&queue_mutex);
    }
    //push the thread to the end of queue
    int status;
    status = queue_push(&request_queue, domain_name);
    if (status==0)
    {
      //printf("Succesfully pushed \"%s\" into the request_queue.\n", domain_name);
    }
    else
    {
      fprintf(stderr, "Error: failed to push \"%s\" int the request_queue.\n", domain_name);
    }
    // Done pushing domain_name to queue now unlock the mutex
    pthread_mutex_unlock(&queue_mutex);
    // dynamically allocated new domain_name for next iteration.
    domain_name = (char*) malloc (SBUFSIZE);
  }
  // Once our parser is at EOF, close the current file and free all resources
  int status;
  status = fclose((FILE*)inputfp);
  if (status)
  {
    fprintf (stderr, "ERROR: FAILED TO CLOSE IN FILE IN INSERT TO QUEUE\n");
  }
  free(domain_name);
  // Call exit on thread to terminate calling thread (free it)
  pthread_exit(NULL);  
}

void join_request_pool()
{
  int request_thread, status;
  for (request_thread = 0; request_thread < requested_threads; request_thread++)
  {
    status = pthread_join(request_pool[request_thread], NULL);
    if (status==0)
    {
      //printf("Successfully joined the request_pool!\n");
    }
    else
    {
      fprintf(stderr, "Error: failed at joining request_pool\n");
    }
  } 
  req_finished = 1; 
}

void join_resolver_pool()
{
  int resolver_thread, status;
  for (resolver_thread = 0; resolver_thread < MAX_INPUT_FILES; resolver_thread++)
  {
    status = pthread_join(resolver_pool[resolver_thread], NULL);
    if (status==0)
    {
      //printf("Successfully joined the resolver_pool!\n");
    }
    else
    {
      fprintf(stderr, "Error: failed at joining resolver_pool!\n");
    }
  } 
}

