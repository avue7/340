/**
 * @file multi-lookup.c
 *
 * @brief 
 *    Implementation of a multi-process application that resolves
 *    domain names to IP addresses. This app will use a Producer-
 *    Consumer type architeture system. The two sub-systems compose
 *    of one requester pool of processes and one resolver pool of processes.
 *    The sub-systems will communicate with each other using a bounded buffer
 *    in shared memeory. MMAP is used to achieve creating the shared memory
 *    segment needed for the processors to communicate. Mutex and conditional
 *    variables are used for synchronization and to eliminate race conditions. 
 * 
 * @reference I am using the file lookup.c by Andy Sayler as a starting point. 
 *            I also adopted some items from this example program on:
 *            http://www.csl.mtu.edu/cs4411.ck/www/NOTES/threads/buf-m-2.c
 *            Accessed date: 12/01/2017.
 *            https://www.linuxquestions.org/questions/programming-9/
 *                  mmap-tutorial-c-c-511265/
 *            Accessed date: 12/06/2017.
 *
 * @author Athit Vue
 * @date 12/01/2017
 */
#include "multi-lookup.h"
#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_RESOLVER_PROCESSES 10 //Macro the max_resolver thread
#define MIN_RESOLVER_PROCESSES 2 // Macro the min_reolver thread
#define MAX_INPUT_FILES 10
#define MIN_REQUESTER_PROCESSES 1
#define MAX_NAME_LENGTH 1025
#define BUFFER_SIZE 10 // Size of elements within the mmap
#define DEBUG 1 // Set to one if you want to debug (main)
#define DEBUG_CONS 1 // To debug the consumers function
#define DEBUG_PRO 0  // To dubug the producers function
#include <assert.h>  // Assert for debugging seg faults

/* Optional, use this mapping data to a file 
 *#define FILEPATH "/tmp/mmapped.bin"
 *#define FILESIZE (BUFFER_SIZE*sizeof(char*))
 */

/*################### GLOBAL VARIABLES ###################################*/
pid_t pro_pid;
pid_t parent_pid, pid;
void* shared_memory;

/* Need a bounded buffer mapped to shared memory */
typedef struct buffer
{
  // Note for future reference: do not use pointers that point 
  // to outside of the mapped segment. Doing so will cause the processors
  // to not be able read or write the data. EG) char* data[].
  char data[BUFFER_SIZE][SBUFSIZE];
  unsigned int count; // Counter for elements currently in the buffer
  unsigned int in;  // Points to next free position in buffer
  unsigned int out; // Points to the first full position
 
  // Flag to let the consumers know that producers are done producing.
  // Needs to be protected and stored in shared memory.
  int producers_done; 

  /* The mutex variable shared between process */
  pthread_mutex_t buffer_mutex;
  pthread_mutex_t output_mutex;
  pthread_mutex_t flag_mutex;
  pthread_mutexattr_t mutexAttr;
  /* The conditional variable shared between processes */
  pthread_cond_t buffer_empty; 
  pthread_cond_t buffer_fill;  
  pthread_condattr_t condAttr;
} buffer_t;  // Type of struct

// Create a pointer to the struct that will be 
// used as the size of the shared memory segment.
// Initialize the variables to 0.
static buffer_t* buffer_ptr = &(struct buffer){
  .count = 0,
  .in = 0,
  .out = 0,
  .producers_done = 0
};
/*########################################################################*/

int main(int argc, char* argv[]){

    /* Local Vars */
    FILE* inputfp = NULL;
    FILE* outputfp = NULL;
    char errorstr[SBUFSIZE];
    int i,j;
    int files = 0;
    
    // Create shared_memory segment for the processes.
    // Exit if cannot create. Cannot continue without this. 
    shared_memory = create_shared_memory(sizeof(buffer_t));
    if (shared_memory == (caddr_t) -1)
    { 
      perror("Error in mmap while allocatiing shared memory\n");
      exit(1);
    }

    // Map our bounded buffer to the shared memory
    buffer_ptr = (buffer_t*) shared_memory;

    // Initialize the shared mutex and conditional variables
    init();
    
    /* Check Arguments */
    if(argc < MINARGS){
        fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
        fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
        return EXIT_FAILURE;
    }
    /* If more than allowable max input files given */
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

    /* Loop Through Input Files */
    for(i=1; i<(argc-1); i++)
    {
        /* Open Input File */
        inputfp = fopen(argv[i], "r");
        if(!inputfp)
        {
          files++;
          sprintf(errorstr, "Error::cannot open input file <%s>", argv[i]);
          perror(errorstr);
          // Check for any valid files. If none found, then cannot gracefully 
          // ignore. Must terminate the program. 
          if (files == (argc-2))
          {
            fprintf(stderr, "Error::no valid input files found:");
            fprintf(stderr, " Terminatiing program!\n");
            return EXIT_FAILURE;
          }     
          continue;
        }
        int n = MIN_RESOLVER_PROCESSES; // Number of childred to fork
        static int child_rank = 0;
        
        // For every file, the parent process will be the producer while 
        // the children forked will be the consumers.
        fork_children(n, outputfp, inputfp, child_rank);
    }

    // Wait for child to end. 
    for (j = 0; j < MIN_RESOLVER_PROCESSES; j++)
    {
      int status;
      pid = wait(&status);
      printf("DEBUG::main(): child [pid: <%d>] exited with status %d.\n",
        pid, status);
    }

    if (DEBUG){ 
      printf("DEBUG::main(): items in bounded buffer after forking"); 
      printf(" is done:\n");
      printf("--- Current [pid: <%d>]\n", getpid());
      for (i = 0; i < BUFFER_SIZE; i++)
      {
        printf("---%i.  <%s> \n", i+1, buffer_ptr->data[i]);
      }
    }

    fclose(outputfp);
    // Destroy, release mutex and condition variables...
    pthread_mutex_destroy(&(buffer_ptr->buffer_mutex));
    pthread_mutex_destroy(&(buffer_ptr->output_mutex));
    pthread_cond_destroy(&(buffer_ptr->buffer_empty));
    pthread_cond_destroy(&(buffer_ptr->buffer_fill));
    // Free resources
    return EXIT_SUCCESS;
}

/**
 * void* create_shared_memory
 * 
 * This function is used to help decompose the main function. This 
 * function will create the shared memory that we will need for 
 * the processors. Without this the processes will not be able to 
 * share variable values and such. Every process maps what the 
 * parent has right at forking. 
 *
 * @param size The size of the shared memory segment.
 * @return void A void pointer to the memory created.
 */
void* create_shared_memory(size_t size)
{
  /*********** KEEP FOR FUTURE REFERENCE ****************************
  // Create a file that maps to the shared memory. This file can be
  // written to by the processors. Lseek will go to the end of file
  // and stretch the file. We need to write a blank space at the end
  // of the file so that we can have the byte size for the file. 

  int fd, result; 
  fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
  if (fd == -1){
    perror("Error opening file for writing\n");
    exit(EXIT_FAILURE);
  }
  result = lseek(fd, FILESIZE-1, SEEK_SET);
  if (result == -1){
    close(fd);
    perror("Error calling lseek() to stretch the file\n");
    exit(EXIT_FAILURE);
  }
  result = write(fd, "", 1);
  if (result != 1){
    close(fd);
    perror("Error writing last byte to the file\n");
    exit(EXIT_FAILURE);
  }
  *******************************************************************/
  // Need to be able to read and write to shared memory
  int protection = PROT_READ | PROT_WRITE;
  // Need process and its children to be able to see and share
  int visibility = MAP_ANONYMOUS | MAP_SHARED;

  return mmap(NULL, size, protection, visibility, /*fd*/0, 0);
}

/**
 * void init()
 *
 * Function to decompose main function use to initialize the mutex and 
 * condtional variables.
 */
void init()
{
  // Set the mutex to be shared between processes
  pthread_mutexattr_setpshared(&(buffer_ptr->mutexAttr), 
    PTHREAD_PROCESS_SHARED);
  pthread_mutex_init(&(buffer_ptr->buffer_mutex), &(buffer_ptr->mutexAttr));
  pthread_mutex_init(&(buffer_ptr->output_mutex), &(buffer_ptr->mutexAttr));
  pthread_mutex_init(&(buffer_ptr->flag_mutex), &(buffer_ptr->mutexAttr));
    
  // Set the conditional variables
  pthread_condattr_setpshared(&(buffer_ptr->condAttr), PTHREAD_PROCESS_SHARED);
  pthread_cond_init(&(buffer_ptr->buffer_empty), &(buffer_ptr->condAttr));
  pthread_cond_init(&(buffer_ptr->buffer_fill), &(buffer_ptr->condAttr));
}

/**
 * void fork
 *
 * Need this recursive function so that other children can access the 
 * shared memory segment. If not recursive then the parent's grandkids
 * will not know about the shared memory segment.
 *
 * @param nprocess The number of children.
 * @param p_outputfp Pointer to the output file.
 * @param p_inputfp Pointer to the input file.
 */
void fork_children(int nprocess, void* p_outputfp, void* p_inputfp,
    int child_count)
{
  int pid;
  child_count++;

  if (nprocess > 0)
  {
    if ((pid = fork()) < 0)
    {
      int errornum = errno;
      fprintf(stderr, "Error::failed to fork(%d: %s)\n", errornum,
        strerror(errornum));
      exit(EXIT_FAILURE);
    }
    if (pid == 0) // Child (CONSUMERS)
    {
      pid = getpid();
      
      if (DEBUG){
        printf("DEBUG::Consumer #%d, [pid: <%i>] created!\n",
          child_count, pid);
        fflush(stdout);
      }
      // Consume as soon as forked 
      consume(child_count, p_outputfp);
      exit(0);
    }   
    else  // Parent (PRODUCER)
    {
      int parent_pid = getpid();

      if (DEBUG){
        printf("DEBUG::Producer #1, [pid: <%i>] created!\n", 
          parent_pid);
        fflush(stdout);
      }
      // Produce as soon as forked
      produce(p_inputfp);
      // Recursive call 
      fork_children((nprocess-1), p_outputfp, p_inputfp, child_count); 
    }
  }
}

/**
 * void consume
 *
 * This function implements the consumers action after forking. 
 * Shared mutex locks and condition variables in shared memory 
 * helps the consumers and producers from having a race condition.
 * The bounded buffer and output file must be protected in this 
 * function.
 *
 * @param cons_rank Rank of the consumers used for debugging. 
 * @param p_outputfp A void pointer to the output file. 
 */
void consume(int cons_rank, void* p_outputfp)
{
  /* Condition flags for consumers */
  int buffer_count;
  int producers_done;

  // Try to obtain the flag mutex lock. Once obtain the set the flag
  // variables to current value. 
  pthread_mutex_lock(&(buffer_ptr->buffer_mutex));
    buffer_count = buffer_ptr->count;
  pthread_mutex_unlock(&(buffer_ptr->buffer_mutex));
  
  pthread_mutex_unlock(&(buffer_ptr->flag_mutex));
    producers_done = buffer_ptr->producers_done;  
  pthread_mutex_unlock(&(buffer_ptr->flag_mutex));

  if (DEBUG_CONS){
    int k;
    printf("DEBUG::consume(): Child [pid: <%i>] can see items:\n", getpid());
    for (k = 0; k < BUFFER_SIZE; k++)
    {
      printf("---%i.  <%s> \n", k+1, buffer_ptr->data[k]);
    }
  }

  // If nothing is in the buffer then check to see if producers are done 
  // producing. If nothing in buffer but producers not done we want the 
  // consumers to enter the while loop and wait for producers to produce.  
  while((buffer_count != 0) || !producers_done) 
  { 
    // Try to obtain the buffer mutex lock
    pthread_mutex_lock(&(buffer_ptr->buffer_mutex));

    char domain_url[INET6_ADDRSTRLEN];
    char* domain_name;
    domain_name = (char*) malloc(sizeof(char)*SBUFSIZE);

    // If the bounded buffer is empty, then wait on condition buffer_fill.
    // Pthread_cond_wait() will atomically (without needing another function)
    // release the mutex lock. 
    while (buffer_ptr->count == 0)
    {
      // When it get's signaled to begin consuming again and producer is done
      // as well as buffer count == 0, then exit...No more to consume. 
      if (buffer_ptr->producers_done && (buffer_ptr->count == 0))
      {
        if (DEBUG_CONS){
          int child_pid = getpid();
          printf("DEBUG::consume(): Consumer#%d child [pid: <%d>] ",
            cons_rank, child_pid);
          printf(" done consuming. Exiting!\n");
        }
        // Release mutex lock and return to exit gracefully
        pthread_mutex_unlock(&(buffer_ptr->buffer_mutex));
        free(domain_name);
        return;
      }

      if (DEBUG_CONS){
          int child_pid = getpid();
          printf("DEBUG::consume(): Consumer#%d, child [pid: <%d>]",
             cons_rank, child_pid); 
          printf(" waiting to consume...\n");
      }
      // Wait on condition while release buffer mutex
      pthread_cond_wait(&(buffer_ptr->buffer_fill), &(buffer_ptr->buffer_mutex));
    }

    if (DEBUG_CONS){ 
      printf("DEBUG::consume(): Consumer#%d,  child [pid: <%d>] is consuming...\n",
         cons_rank, getpid());
      printf("----consuming <%s> \n", buffer_ptr->data[buffer_ptr->out]);
    }

    // If count not 0, then retrieve an item.
    domain_name = buffer_ptr->data[buffer_ptr->out];
    // Advance the pointer to next item. Also make sure it does not go out of 
    // bound by doing a modulus. 
    buffer_ptr->out = (buffer_ptr->out + 1) % BUFFER_SIZE; 
    // Decrement the item count
    buffer_ptr->count--;
    // Signal the producer to resume producing
    pthread_cond_signal(&(buffer_ptr->buffer_empty));
    // Release the mutex lock
    pthread_mutex_unlock(&(buffer_ptr->buffer_mutex));
    // Update the flag
    pthread_mutex_unlock(&(buffer_ptr->flag_mutex));
      producers_done = buffer_ptr->producers_done;  
    pthread_mutex_unlock(&(buffer_ptr->flag_mutex));

    // Now that we have the domain_name lets look it up using the provided
    // dns_lookup
    int status;
    status = dnslookup(domain_name, domain_url, sizeof(domain_url));
    if (status == -1 || status == UTIL_FAILURE)
    {
      fprintf(stderr, " cannot find: \"%s\".\n", domain_name);
      strncpy(domain_url, "", sizeof(domain_url));
    }
    else
    {
      if (DEBUG_CONS){
        printf("Successfully looked up \"%s\".\n", domain_name);
      }
    }
  
    // Once we are done looking it up, we need to write it to the output file.
    // Try to get the lock first.
    pthread_mutex_lock(&(buffer_ptr->output_mutex));
    fprintf ( (FILE*)p_outputfp, "%s,%s\n", domain_name, domain_url);
    pthread_mutex_unlock(&(buffer_ptr->output_mutex));
    // Now let's free up the domain_name 
    //free(domain_name); // NO WORK IF UNCOMMENT!!!
    // domain_name = NULL; 
  }
  // If nothing let to consume and new consumer arrives, tell it to return.
  return;
}

/**
 * void produce
 *
 * This function implements the producers action upon forking. 
 * The bounded buffer must be protected in this function from having
 * a race condition with other processors. Shared mutex locks and 
 * conditional variables help us achieve this in this function. 
 *
 * @param p_inputfp A void pointer to the input file.
 */
void produce(void* p_inputfp)
{
  char* domain_name;
  domain_name = (char*) malloc(sizeof(char)*SBUFSIZE);

  // Scan the file til the parser reaches EOF
  while(fscanf((FILE*)p_inputfp, INPUTFS, domain_name) > 0)
  {   
    // Try to get the mutex lock for the bounded buffer
    pthread_mutex_lock(&(buffer_ptr->buffer_mutex));

    // While bounded buffer is full, we must wait on condition variable 
    // buffer_empty
    while ( buffer_ptr->count == BUFFER_SIZE)
    {
      if (DEBUG_PRO){
        printf("DEBUG::produce(): Producer, [pid: <%d>] waiting to", getpid()); 
        printf("produce...\n");
      }
      pthread_cond_wait(&(buffer_ptr->buffer_empty), 
         &(buffer_ptr->buffer_mutex));
    }

    // Use strcpy to copy data to bounded buffer
    strcpy(buffer_ptr->data[buffer_ptr->in], domain_name);
    // Advance the in pointer..make sure it stays within bound.
    buffer_ptr->in = (buffer_ptr->in + 1) % BUFFER_SIZE;
    // Increment the counter
    buffer_ptr->count++;
    // Signal the  consumer to begin or resume consuming
    pthread_cond_signal(&(buffer_ptr->buffer_fill));

    if (DEBUG_PRO){
      int k;
      printf("DEBUG::produce(): count is <%d>\n", buffer_ptr->count);
      printf("DEBUG::produce(): items in the bounded buffer are:\n");
      for (k = 0; k < BUFFER_SIZE; k++)
      {
        printf("---%i.  <%s> \n", k+1, buffer_ptr->data[k]);
      }
    }

    // Release the bounded buffer lock
    pthread_mutex_unlock(&(buffer_ptr->buffer_mutex));

    // Dynamically allocate new memory for the next domain name item
    domain_name = (char*) malloc(sizeof(char)*SBUFSIZE);    
  }
   
  // Once we reached EOF for each file our producer should be done producing
  if (DEBUG_PRO){
     printf("DEBUG::produce(): Producer, [pid: <%d>] done producing...\n", 
        getpid());
  }
  
  // Update the producers_done flag 
  pthread_mutex_lock(&(buffer_ptr->flag_mutex));
    buffer_ptr->producers_done = 1; 
  pthread_mutex_lock(&(buffer_ptr->flag_mutex));
  
  // Free the resources that domain name points to. 
  free(domain_name);
}







