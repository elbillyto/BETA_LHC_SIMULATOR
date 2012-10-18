//==============================================================================//
//  Filename: main.c															//
//																				//
//  Test - Application for @CERN.						//
//==============================================================================//
//																				//
//  Created by Carlos Ciller on Oct 12, 2012.									//
//  Copyright (c) 2012 -. All rights reserved.									//
//  Description : Written in C, Ansi-style.										//
//------------------------------------------------------------------------------//
//																				//
//  The aim of this program is to provide an application partially simulating	//
//  the behaviour of a "Large Hadron Collider".									//
//																				//
//  Since I don't want to exactly monitor how the LHC works,I will improvise and//
//  adapt the "needs" of my program in order to show as many different tools as //
//  required in 'C' language.													//
//																				//
//  Most of the information from within we developed this simulator may be loca-//
//  ted at the following places:												//
//  1) CERN FACTS AND FIGUES:													//
//	http://public.web.cern.ch/public/en/lhc/Facts-en.html						//
//	2) CERN FAQ LHC: The Guide													//
//	http://cdsweb.cern.ch/record/1165534/files/CERN-Brochure-2009-003-Eng.pdf	//
//  3) YOUTUBE: CERN Large Hadron Collider LHC									//
//  http://www.youtube.com/watch?v=_T745HXduHY									//
//																				//
//	All of these calculus where both a combination of extracted information and	//
//  my own work. No one has yet revised them, but I expect they me be close to	//
//  reality.																	//
//																				//
//	VARIABLES:																	//
//  ----------																	//
//	� LHC Circumference: 26.659m												//
//	� # of Magnets: 9300 magnets. Pre-cooled at -193.2�C(80k)					//
//	� Speed at 7 TeV: 299792455.30 m/s (speed of light 299792458.00 m/s)		//
//  � Proton Mass: 1.67262158e-27 (kg) at 0 m/s. 1.24786320961875e-23 at highest//
//  speed. There is a variation of 7460.523196 proton's original mass.			//
//																				//
//	ASSUMPTIONS:																//
//  ------------																//
//  - We will only monitorize a packet of particles (from the 2808 existing		//
//  at a time).																	//
//	- About the amount of info: An average simulation last between 20 & 45 min. //
//  our simulation will be running only 10 seconds, so it will be easier to deal//
//	with the huge amount of data generated at the NODES. At the speed of light, //
//  the particles travel 11103.42427 times per second through the LHC circumfe- //
//  rence. This means each NODE collects 299792455.3(m/s)/26.659(m)= 11245.45014//
//  samples/second.																//
//  - Due to the some unexpecte synchronization problems we may only generate 	//
//  the sequences into files. For further versions of the program we will handle//
//  time sequence syncronization between NODES.									//
//																				//
//  HOW DOES IT WORK?															//
//	-----------------															//
//  The program's execution is easy to explain. The user selects a given 		//
//  number of nodes (between 1 and 16) and the program creates as many threads 	//
//  as nodes were selected.														//
//	For each node, we allocate the space in memory (considering the number 		//
//	of samples to capture) and the space required to store all the specific 	//
//	properties regarding the node (create_Node). Once all nodes are created 	//
//	the system waits 3 seconds before running a simulation which 'should' last 	//
//	_number_Of_Measures-times the time it takes for a set of particles to run 	//
//	all over the LHC biggest circumference (26659m).							//
//	After simulation is finished, each node stores all the information regarding//
//	the false simulation in a txt file for posterior use.						//
// 																				//
//------------------------------------------------------------------------------//

					/***********************************/

//------------------------------------------------------------------------------//
//								PROGRAM START									//
//------------------------------------------------------------------------------//

/* SYSTEMS INCLUDES 															*/
//------------------------------------------------------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* LOCAL INCLUDES 																*/
//------------------------------------------------------------------------------//
#include "../include/lhc_simulator.h"


/*  LHC SIMULATOR - BASE INFORMATION											*/
//------------------------------------------------------------------------------//
#define NODES_MAX 16
#define LHC_PERIMETER 26659

/*  ERROR MESSAGES - Program Execution											*/
//------------------------------------------------------------------------------//
#define FATAL(msg) \
	do{ \
		fprintf(stderr,"%s:%d:[%s]: %s\n", __FILE__, __LINE__, msg, strerror(errno)); \
		exit(-1); \
	} while (0)

/*  GLOBAL VARIABLES (Present during the execution of the whole program)		*/
//------------------------------------------------------------------------------//
pthread_mutex_t mutex;		/** Locker for multithreading critic executions */
int turn=0; 				/** Handles thread sequence */
int _t_Global; 				/** Stores the time delay between nodes once defined */

//int main (int argc, const char * argv[]) {
int main() {

	struct timeval _tvBegin, _tvEnd, _tvDiff;

	/* Clock begin */
	if (gettimeofday(&_tvBegin, NULL)!=0)	{ 	FATAL("Get time of day. Beginning.\n");}
	if (pthread_mutex_init(&mutex,NULL)!=0) { 	FATAL("Error Generating the 'Locking' mutex.\n ");}

	unsigned int 	_numNodes, i=0;
	int				_usual[5]={1, 2, 4, 8, 16};
	int 			j=0, _error=0;
	float			_delta;

//	char* argv[3];
//	argv[0] = 	"./LHC_Simulator";
//	argv[1] = 	"2048";
//	argv[2] = 	"1500";
//	if(atoi(argv[1])>0) _numNodes = atoi(argv[1]);
//	else 				_numNodes =	NODES_MAX;

	printf("*--------------------------------------------------------*\n");
	printf("*--------- LHC - SIMULATOR - BETA VERSION v.1.0  --------*\n");
	printf("*--------------------------------------------------------*\n");
	printf("*..................................AUTHOR: Carlos Ciller.*\n");
	printf("*........................................................*\n \n \n");

	/** The user decides the amount of nodes to set in the LHC Simulator. */
	while(j==0){
		printf("Setting the number of LHC Nodes\n");
		printf("-------------------------------\n"),
		printf("(In order to simplify the process select one of the following:\n ");
		printf("1, 2, 4, 8, 16 --- Selection: ");
		scanf("%d",&_numNodes);
		for(i=0;i<5;i++){
			if (_numNodes == _usual[i] ) {
				printf ("You have entered %d Nodes.\n", _numNodes);
				j=1;
				break;
			}
		}
		if(j==0) {
			printf ("ERROR. Try a number from the ones in the list...\n");
			printf ("***********************************************\n \n");
		}
	}

	/** We initialize the vector of threads and Node identifier */
	pthread_t       thread[_numNodes];
	unsigned int 	_idNode[_numNodes], _aux[_numNodes];

	/** We set an identifier for each LHC_Node, so it is easier to identify
	 *  the measures related to that node. We also include the number of nodes
	 *  so we can pass two variables (added) to each thread.*/
	for(i=0;i<_numNodes;i++){
		_idNode[i]=i+_numNodes*10000;
		_aux[i]=i+_numNodes*10000;
	}

	/** We set the "delta" distance between nodes. The distance between all of
	 *  them is the same.  */
	_delta = (float)LHC_PERIMETER/_numNodes;

	/** We start the multithreading, where each thread corresponds to each node.
	 *  The nodes keep on generating "measures" at a determinate time. Measures
	 *  are generated sequentially between nodes, as the particle passes by. */
	for(i=0;i<_numNodes;i++){
		_error = pthread_create(		&thread[i],
										NULL,
										(void *) create_Node,
										(void *) &_aux[i]);
		if(_error!=0) FATAL("Error creating Thread");
	}

	/** We destroy all threads after finishing */
	for(i=0;i<_numNodes;i++){
		pthread_join(		thread[i],
							NULL);
	}

	/** We destroy the mutex we've using */
	pthread_mutex_destroy(&mutex);
	printf("And not a single thing was done that day! \n");

	/* Clock end */
	if(gettimeofday(&_tvEnd, NULL)!=0) FATAL("Get time of day. End.");

	/* We measure the total elapsed time... */
	time_Difference(&_tvDiff,&_tvEnd, &_tvBegin);

	printf("Elapsed execution time: %ld.%06ld seconds.\n", (long int)_tvDiff.tv_sec, (long int)_tvDiff.tv_usec);

	return 0;
}
