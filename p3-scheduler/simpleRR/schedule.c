/**
 * Name: Athit Vue
 * Program: Simple Round Robin
 * Description: a simple Round Robin Scheduler with a quanta of 4 time units.
 */
#include "schedule.h"
#include <stdlib.h>

/**************************
 ** QUEUE DATA STRUCTURE **
**************************/
/* Need to create the struct node to hold the each process */
struct node
{
  PCB* m_process;
  struct node* next_node;
};

/*************************************
 ** GLOBAL DECLARATION OF VARIABLES **
*************************************/
int isRoot = 0;    // Initialize isRoot to false
int isEmpty = 1;   // Flag for empty struct; initialized to false
struct node* root_node; // Pointer to the root node (unchanging node).
struct node* cur_node;  // Pointer to the current node (changing node).


/*########################### WORKING FUNCTIONS BELOW #######################*/

/**
 * Function to initialize any global variables for the scheduler.
 */
void init()
{
  // Dynamically allocated memory for root struct
  root_node = (struct node*) malloc(sizeof(struct node));
  root_node->next_node = NULL; // Set the next struct node to NULL;
  cur_node = root_node; // Set current node to root node
}

/**
 * Function to add a process to the scheduler
 * @Param PCB * - pointer to the PCB for the process/thread to be added to the
 *      scheduler queue
 * @return true/false response for if the addition was successful
 */
int addProcess(PCB *process)
{
  // First check to see if struct is empty
  // If empty, then we need to set the process as root node
  if (isEmpty == 1)
  {
    root_node->m_process = process;
    root_node->next_node = NULL;
    isEmpty = 0;
    isRoot = 1;
    return 1;
  } 
  else // Process is not empty
  {
    cur_node = root_node; // set cur to root;
    // Iterate through struct until reach NULL (tail)
    // If NULL is reached then the node before the NULL
    // is set to the current node. 
    while (cur_node->next_node != NULL)
    {
      cur_node = cur_node->next_node;
    }
    // Create a new node to store the current process
    // after the next non-null node.
    cur_node->next_node = malloc(sizeof(struct node));
    // Set the current node to point to the node just allocated
    cur_node = cur_node->next_node; 
    // Store the process in this node. 
    cur_node->m_process = process;
    cur_node->next_node = NULL;
    return 1;
  }
  return 0;
}

/**
 * Function to get the next process from the scheduler
 * @Param time - pass by reference variable to store the quanta of time
 *               the scheduled process should run for
 * @Return returns the Process Control Block of the next process that should be
 *      executed, returns NULL if there are no processes
 */
PCB* nextProcess(int *time)
{
  *time = 4;
  // If root node is only one left in the queue then 
  // set isRoot to false and return that process.
  if (isRoot == 1)
  {
    isRoot = 0;
    return (root_node->m_process);
  }
  struct node* temp_node;
  if (root_node->next_node != NULL)
  {
    temp_node = root_node;
    root_node = root_node->next_node;
    free(temp_node);
    return (root_node->m_process);
  }
  else
  {
    return NULL;
  }
}

/**
 * Function that returns a boolean 1 True/0 False based on if there are any
 * processes still scheduled
 * @Return 1 if there are processes still scheduled 0 if there are no more
 *		scheduled processes
 */
int hasProcess()
{
  if (root_node->next_node == NULL)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
