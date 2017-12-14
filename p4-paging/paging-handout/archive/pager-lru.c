/*
 * File: pager-lru.c
 * Author:       Athit Vue and Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Last Modify Date: 10/28/2017 by Athit Vue
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains an lru pageit
 *      implmentation.
 */

#include <stdio.h> 
#include <stdlib.h>
#include <limits.h>

#include "simulator.h"

void pageit(Pentry q[MAXPROCESSES]) {

	/* This file contains the stub for an LRU pager */
	/* You may need to add/remove/modify any part of this file */

	/* Static vars */
	static int initialized = 0;
	static int tick = 1; // artificial time
	static int timestamps[MAXPROCESSES][MAXPROCPAGES];

	/* Local vars */
	int proctmp;
	int pagetmp;
        int old_page;
        int page;

	/* initialize static vars on first run */
	if (!initialized) {
		for (proctmp = 0; proctmp < MAXPROCESSES; proctmp++) {
			for (pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++) {
				timestamps[proctmp][pagetmp] = 0;
			}
		}
		initialized = 1;
	}

	/* TODO: Implement LRU Paging */

        /* Select the first active process */
        for (proctmp = 0; proctmp < MAXPROCESSES; proctmp++)
        {
          /* Check to see if processor is active. If active, active = 1;
             else, active = 0 (exited). */
          if (q[proctmp].active) // Current process is active!
          {
            // Get the page the program counter needs.
            page = q[proctmp].pc / PAGESIZE;
            // Check to see if page is swapped-out 
            // (if page is not in the pages array). 
            if (!q[proctmp].pages[pagetmp]) // Page is swapped out!
            {
              /* Try to swap in */
              if (!pagein(proctmp, pagetmp)) // Swap in failed!
              {
                int lru_timestamp = INT_MAX;
                int oldest_page = -1;
                /* Look through all npages and swap out LRU page. */ 
                for (old_page = 0; old_page < q[proctmp].npages; old_page++)
                {
                    // Make sure page isn't the one I want.
                    if (old_page != page)
                    {
                      // Make sure old page was paged in (if it's in the 
                      // pages array.
                      if (q[proctmp].pages[old_page])
                      {
                        // Finally, make sure timestamp of oldpage is less than
                        // the last recently use
                        if (timestamps[proctmp][old_page] < lru_timestamp)
                        {
                          // Select LRU (oldest page) to evict
                          oldest_page = old_page;
                          // Record the LRU page timestamp.
                          lru_timestamp = timestamps[proctmp][old_page];
                        }
                      }
                    }
                 }
                 // Pageout the oldest page.
                 pageout(proctmp, oldest_page);
              }   
              else // Swaped in succeded!
              {
                // Record the timestamp of all successful swap-ins. 
                timestamps[proctmp][pagetmp] = tick;
              }
            }
          }
        }
	/* advance time for next pageit iteration */
	tick++;
}

























