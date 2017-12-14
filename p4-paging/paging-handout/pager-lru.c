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
        int lru_page = 0;
        /* Need to set lru_timestamp to a really high integer for initial check of 
           timestamp for old page since we are unsure what the timestamp will be for
           the first successful page-in. */
        int lru_timestamp = 2147483647;

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
	    /* Check to see if processor is active. If active, then active = 1.
               Else, active = 0 -> exited. */
            if (q[proctmp].active) // Current process is active!
            {
                // Get the page the program counter needs.
                pagetmp = q[proctmp].pc / PAGESIZE;
                /* Check to see if page is swapped-out (if page is not in the pages array) */
                if (!q[proctmp].pages[pagetmp]) // Page is swapped-out!
                {
                    /* Try to swap in */
                    if (!pagein(proctmp, pagetmp)) // Swap in failed!
                    {
                        /* Look through all npages and swap out LRU page */ 
                        for (old_page = 0; old_page < q[proctmp].npages; old_page++)
                        {
                            // Make sure page isn't the one I want.
                            if (old_page != pagetmp)
                            {
                                // Make sure old page was paged-in (if it's in the 
                                // pages array).
                                if (q[proctmp].pages[old_page])
                                {
                                    // If current iteration of old_page's timestamp is less 
                                    // than lru_timstamp then we have a new lru_page. 
                                    if (timestamps[proctmp][old_page] < lru_timestamp)
                                    {
                                        // Set the LRU old page to lru_page
                                        lru_page = old_page;
                                        // Set the LRU page timestamp to the new lru time.
                                        lru_timestamp = timestamps[proctmp][old_page];
                                    }
                                }
                            }
                        }
                        // Pageout the LRU page.
                        pageout(proctmp, lru_page);
                    }
                    else // Swapped in succeded!!!
                    {
                        // Record the timestamp of all successful swap-ins. 
                        timestamps[proctmp][pagetmp] = tick;
                        // Debugger:
                        // printf("PAGED IN!!! Tick is :%d\n", tick);
                    }
                }
            }
        }
	/* advance time for next pageit iteration */
	tick++;
        // Debugger:
        // printf("TICK IS: %d\n", tick);
}
