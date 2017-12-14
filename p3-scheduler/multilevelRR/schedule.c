/*
 * Name: Athit Vue
 * Program: Multi-Level Round Robin 
 * Description: this scheduler iterates through different levels
 *              starting with the highest priority (priority 0)
 *              and choses the process next in the queue for that
 *              priority level for a specified quanta or time. The
 *              higher the priority, the more quanta is given. 
*/
#include "schedule.h"
#include <stdlib.h>

/**************************
 ** QUEUE DATA STRUCTURE **
**************************/
/* We need a queue for allowing each thread to access the 
 * resources by a first come first serve bases. Let's create
 * a queue without using the library.
 */
struct node 
{
  PCB* m_process;
  struct node *m_next;
};

/************************************
 ** INITIALIZATION OF QUEUE STRUCT **
************************************/
struct node* root[4];
struct node* cur;  // Pointer to changing current node
int q_index;

/******************************
 ** WORKING FUNCTIONS STARTS **
******************************/

/**
 * Function to initialize any global variables for the scheduler.
 */
void init()
{
  /* Need four roots. Iterate through it and initialize all roots to NULL */
  int i;
  for (i = 0; i < 4; i++)
  {
    root[i] = NULL;
  }
  q_index = 0; // Start the q_index at 0.
  cur = NULL;
}

/** 
 * Function to add a process to the scheduler
 * @Param PCB * - pointer to the PCB for the process/thread to be added to the
 *      scheduler queue
 * @return true/false response for if the addition was successful
 */
int addProcess(PCB *process)
{
  if (process->priority == 0)
  {
    // If root[0] is empty, create a new node that points to it.
    if (root[0] == NULL)
    {
      root[0] = (struct node*) malloc( sizeof(struct node));
      root[0]->m_process = process;
      root[0]->m_next = NULL;
      return 1;      
    }
    // Else create a new node and make it point to the next node. 
    else 
    {
      cur = root[0];
      while (cur->m_next != NULL)
      {
        cur = cur->m_next;
      }
      cur->m_next = malloc( sizeof(struct node));
      cur = cur->m_next;
      cur->m_process = process;
      cur->m_next = NULL;
      return 1;
    }
  }
  else if (process->priority == 1)
  {
    if (root[1] == NULL)
    {
      root[1] = (struct node*) malloc( sizeof(struct node));
      root[1]->m_process = process;
      root[1]->m_next = NULL;
      return 1;
    }
    else 
    {
      cur = root[1];
      while (cur->m_next != NULL)
      {
        cur = cur->m_next;
      }
      cur->m_next = malloc( sizeof(struct node));
      cur = cur->m_next;
      cur->m_process = process;
      cur->m_next = NULL;
      return 1;
    }
  }
  else if (process->priority == 2)
  {
    if (root[2] == NULL)
    {
      root[2] = (struct node*) malloc( sizeof(struct node));
      root[2]->m_process = process;
      root[2]->m_next = NULL;
      return 1;
    }
    else 
    {
      cur = root[2];
      while (cur->m_next != NULL)
      {
        cur = cur->m_next;
      }
      cur->m_next = malloc( sizeof(struct node));
      cur = cur->m_next;
      cur->m_process = process;
      cur->m_next = NULL;
      return 1;
    }
  }
  else if (process->priority == 3)
  {
    if (root[3] == NULL)
    {
      root[3] = (struct node*) malloc( sizeof(struct node));
      root[3]->m_process = process;
      root[3]->m_next = NULL;
      return 1;
    }
    else 
    {
      cur = root[3];
      while (cur->m_next != NULL)
      {
        cur = cur->m_next;
      }
      cur->m_next = malloc( sizeof(struct node));
      cur = cur->m_next;
      cur->m_process = process;
      cur->m_next = NULL;
      return 1;
    }
  }
  else 
  {
    return 0;
  }
}

/**
 * Function to get the next process from the scheduler
 * @Param time - pass by reference variable to store the quanta of time
 * 		the scheduled process should run for
 * @Return returns pointer to process control block that needs to be executed
 * 		returns NULL if there is no process to be scheduled.
 */
PCB* nextProcess(int *time)
{
  // This is a special case for when the queue at each q_index points to 
  // a NULL. This means that we are at the end of the list so there are no
  // more processes to process. Well need to iterate to the next queue that
  // does not contain a null pointer for processing the rest of the queues
  // that actually contain any scheduled processes.
  int index = 0;
  int i;
  for (i = 0; i < 3; i++)
  {
    if (root[q_index] == NULL)
    {
      if (q_index == 3)
      {
        q_index = 0;
      }
      else
      {
        q_index++;
      }
    }
  }
  // For the queues that does not contain a NULL as the root, set it to a 
  // temp node and move the root pointer to the next node. Increment the 
  // q_index to the next queue. Must use modulus for keeping the q_index 
  // from going out of bound of the 0-3 q_index. 
  if (root[q_index] != NULL)
  {
    index = q_index;
    q_index = (q_index+1) % 4;
    *time = 4 - index;
    struct node *temp;
    temp = root[index];
    root[index] = root[index]->m_next;
    return (temp->m_process);
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
  /* The orr(s) insure that the conditions are check from left to right.
     If any of the conditions is true, we return 1 to let the main program
     know that there are more processes to be process. Else, return 0.
  */
  if (root[0] != NULL || root[1] != NULL || root[2] != NULL || root[3] != NULL) 
  {
    return 1;
  }
  else 
  { 
    return 0;
  }
}

