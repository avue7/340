/*
 * Name: Athit Vue
 * Program: Multi-Level Feedback
 * Description: this program uses a FCFS queue scheduler for the highest
 *              priority tasks and two round robin queues for the lower
 *              priority tasks. If a task has not been scheduled for 
 *              1000 time cycles the process should then be added to the
 *              higher priority scheduler. Use 0 to represent the highest
 *              priority increasing in value as the priority gets lower.
*/
#include "schedule.h"
#include <stdlib.h>
#include <stdio.h>

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
struct node* root[3]; // Need array size of three. We have 3 queues.
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
  /* Need three roots. Iterate through it and initialize all roots to NULL */
  int i;
  for (i = 0; i < 3; i++)
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
  else 
  {
    return 0;
  }
}

/**
 * Function called every simulated time period to provide a mechanism
 * to age the scheduled processes and trigger feedback if needed.
 */
void age()
{
  // If this function is called then that means that one clock cycle
  // has passed already. Loop through all nodes in all queues and add
  // a 1 to its age. 
  int i;
  for (i = 0; i < 3; i++)
  {
    if (root[i] != NULL)
    {
      // Dont forget to add a one to the first node; the root of each queue.
      root[i]->m_process->age += 1;
      cur = root[i];
      while (cur->m_next != NULL)
      {
        cur = cur->m_next;
        cur->m_process->age += 1;
      }
    }
  }
  // Checking to see if need to trigger feedback
  // Only check queue 1 and 2, queue 0 is highest priority already.
  for (i = 1; i <= 2; i++)
  {
    if (root[i] != NULL)
    {
      cur = root[i];
      while(cur != NULL)
      {
        if (cur->m_process->age >= 1000)
        {
          struct node *temp;
          temp = root[i];
          root[i] = root[i]->m_next;
          temp->m_process->priority = i - 1;
          temp->m_process->age = 0;
          addProcess(temp->m_process);
          free(temp);
        }
        cur = cur->m_next;
      }
    }
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
  // Our root[0] needs to be the FCFS queue and also the highest priority 
  // queue. Root[1] and root[2] queues shall be the lower priority queues.
  // Note: by putting this first and no if else statement after, this insures
  //       that root[0] will always be checked and processed with every 
  //       call to this function. This insures FCFS for this node and also
  //       process until no more in queue before moving forward to the lower
  //       priority queues. 
  if (root[0] != NULL)
  {
    *time = 0; // Need not time (quanta) since first queue is FCFS.
    struct node *temp;
    // Detach root node and assign it to temp node.
    temp = root[0];
    root[0] = root[0]->m_next; // Set the next node as the root node.
    return (temp->m_process);
  }
  
  // Need to iterate to next available queue if root[q_index] becomes NULL.
  while (root[q_index] == NULL)
  {
    if (hasProcess() == 1)
    {
      if (q_index == 0)
      {
        q_index += 1;
      }
      else if (q_index == 1)
      {
        q_index += 1;
      }
      // Special case
      else if (q_index == 2)
      {
        q_index = 1;
      }
    }
    else
    {
      return NULL;
    }
  }

  // Now get next node on root[1] or root[2] queues setting time 
  // appropriately. Need quanta 4 for root[1] and 1 for root[2].
  if (root[q_index] != NULL)
  {
    if (q_index == 1)
    {
      *time = 4;
    }
    if (q_index == 2)
    {
      *time = 1;
    }
    // Check for the root[1] or root[2]'s age. If less than 1000 process
    // it.
    if (root[q_index]->m_process->age < 1000)
    {
      int index = q_index;
      struct node *temp;
      temp = root[index];
      // When we process it take the age == to quanta time off of the
      // node we are processing. 
      if (index == 1)
      {
        temp->m_process->age = -4;
      }
      else
      {
        temp->m_process->age = -1;
      }
      root[index] = root[index]->m_next;
      return (temp->m_process);
    }
    else
    {
      return NULL;
    }
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
  if (root[0] != NULL || root[1] != NULL || root[2] != NULL) 
  {
    return 1;
  }
  else 
  { 
    return 0;
  }
}

