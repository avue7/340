/*
 *Author: Akshay Joshi
 *
 *
 * File: pager-lru.c
 * Author:       Andy Sayler
 *               http://www.andysayler.com
 * Adopted From: Dr. Alva Couch
 *               http://www.cs.tufts.edu/~couch/
 *
 * Project: CSCI 3753 Programming Assignment 4
 * Create Date: Unknown
 * Modify Date: 2012/04/03
 * Description:
 * 	This file contains an pager predict pageit
 *      implmentation.
 */

#include <stdio.h>
#include <stdlib.h>

#include "simulator.h"

void pageit(Pentry q[MAXPROCESSES]) {

	/* Static vars */
	static int initialized = 0;//pc
	static int tick = 1; // artificial time//
	static int timestamps[MAXPROCESSES][MAXPROCPAGES];
	/* Local vars */
	int proctmp;//proc
	int pagetmp=0;//old page
	int page;
	int pc=initialized;

	/* initialize static vars on first run */
	if (!initialized)
	{
		for (proctmp = 0; proctmp < MAXPROCESSES; proctmp++)
		 {
			for (pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++)
			{
				timestamps[proctmp][pagetmp]=0;
			}
		}
		initialized=1;
	}
	/*
	Two for loops with MAXPROCESSES and MAXPROCPAGES
	*/
		for (proctmp = 0; proctmp < MAXPROCESSES; proctmp++)
		{
			if(q[proctmp].active)
			{
				pc=q[proctmp].pc;
				page=pc/PAGESIZE;
				if ((!q[proctmp].pages[page]) && (!pagein(proctmp, page)))
				{
						for (pagetmp = 0; pagetmp < MAXPROCPAGES; pagetmp++)
						{
							timestamps[proctmp][pagetmp] = tick;
							if(timestamps[proctmp][pagetmp]!=1){}
							if(proctmp!=pagetmp)
							{
								if(pagetmp!=page)
								{
									if((pageout(proctmp,pagetmp)))
									{
										//pagein(a,b);//pageout minimum time
										continue;//used page
									}//pagein the required one
								}
							}
						}
					}
				}
			}
	/* advance time for next pageit iteration */
	tick++;
}
