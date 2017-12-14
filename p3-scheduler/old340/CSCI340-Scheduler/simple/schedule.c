/*
 * Name: Athit Vue
 * Program: Simple Scheduler
 * Description: a simple First Come First Serve (FCFS) scheduler.
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
int isRoot;        // check for root node
int isEmpty;       // flag for empty struct
struct node* root; // Pointer to root, first node unchange
struct node* cur;  // Pointer to changing current node

/******************************
 ** WORKING FUNCTIONS STARTS **
******************************/

/**
 * Function to initialize any global variables for the scheduler.
 */
void init()
{
  root = (struct node*) malloc(sizeof(struct node));
  isEmpty = 1;
  root->m_next = NULL;
  cur = root;  
}

/** 
 * Function to add a process to the scheduler
 * @Param PCB * - pointer to the PCB for the process/thread to be added to the
 *      scheduler queue
 * @return true/false response for if the addition was successful
 */
int addProcess(PCB *process)
{
  // If struct is empty, then the first node is root
  if (isEmpty == 1)
  {
    root->m_process = process;
    root->m_next = NULL;
    isEmpty = 0;
    isRoot = 1;
    return 1;
  } 
  // Else it is not root
  else 
  {
    cur = root;
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
  return 0;
}

/**
 * Function to get the next process from the scheduler
 *
 * @Return returns the Process Control Block of the next process that should be
 *      executed, returns NULL if there are no processes
 */
PCB* nextProcess()
{
  if (isRoot == 1)
  {
    isRoot = 0;
    return (root->m_process);
  }
  struct node* temp = root;
  if (root->m_next != NULL)
  {
    temp = root;
    root = root->m_next;
    free(temp);
    return (root->m_process);
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
  if (root->m_next == NULL)
  {
    return 0;
  }
  else 
  { 
    return 1;
  }
}
