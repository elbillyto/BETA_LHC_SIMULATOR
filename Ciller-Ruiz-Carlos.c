//==============================================================================//
//  Filename: main.c								//
//										//
//  Test - Application for @CERN.						//
//==============================================================================//
//																				//
//  Created by Carlos Ciller on Oct 12, 2012.                                   //
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
//  my own work. No one has yet revised them, but I expect they may be close to	//
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
#include <sys/time.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include "math.h"

#include <stdint.h>

/*  LHC SIMULATOR - BASE INFORMATION											*/
//------------------------------------------------------------------------------//
#define NODES_MAX 16
#define LHC_PERIMETER 26659
#define PI 3.14159265358979

#define EV 1.60217646e-19 				/* Relation between eV and Joule. (Energy) */
#define MP 1.67262158e-27 				/* Proton Mass at "0" speed. Specified in IS convention in Kg. */
#define TEV 1e12 						/* TeV. unit adapted to IS. */
#define P_EXPECTED_SPEED 299792455.3 	/* Expected speed of the particle at the latest LHC Phase */

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



/*   Struct Definition   */
/*~~~~~~~~~~~~~~~~~~~~~~~*/

/* At a determinate instant in each node, the relevant value thrown by
 the sensors may be captured and stored in a struct defined as "measure". */

typedef struct _Measure{

	/** We have considered several variables to be monitorized. However they
	 *  may not be accurate (nor exist). This is only a simulation!! */

	/** We will put a lot of effort in getting as much information as possible
	 *  from each simulation. To do so, we will collect, not only the informa-
	 *  tion regarding the behaviour of the particle, but also the information
	 *  of the estimation of the variation. */

	/* NODE_DATA */
	/*~~~~~~~~~~~~~~~~~*/
	float _position;
	int _identifier;

	/* PARTICLE */
	/*~~~~~~~~~~*/
    /** Current speed of the particle. */
	float _particle_Speed;
    /** Set particle current radiation. */
    float _particle_Radiation;

    /* SENSORS */
    /*~~~~~~~~~*/
    /** Magnet current: Actual current at the magnet at the moment of capture */
    float _magnet_Current;
    /** Helium Temperature & Pressure: The coils of the LHC are bathed in
     *  Liquid-Helium to reduce its resistance and become a superconductor material.*/
    float _helium_Temp;
    float _helium_Pressure;
    /** Radio-Frequency Phase: We control the RF Phase at the moment of capture, so
     *  we may adjust it in order to keep on accelerating the particle until almost
     *  the speed of light. */
	float _phase_RF;
} Measure;

typedef struct _LHC_Node{
    /** Get position of the current node. Considering the LHC has 27 km,
     *  and considering the amount of nodes we want to determine, we will
     *  introduce one node every DELTA = 27.0000m/number_of_Nodes  . */
	float _position;

	/** Considering we will have several nodes. We will name every node
	 * 	with a determinate identifier. It will be important once recovering
	 * 	data. */
	int _identifier;

    /** Set the number of possible captures the node may be able to capture*/
    unsigned int _number_Of_Measures;

    /** Set the time difference between a given measure and the next one.*/
	float _cadence;

    /** Array containing the values of the measures. All included
     *  in the previously defined Measure */
    Measure* _measures;

} LHC_Node;


/*  Function definition  */
/*~~~~~~~~~~~~~~~~~~~~~~~*/

/** Header to create Node...*/
void create_Node(int *);

/** Header to destroy Node...*/
void destroy_Node( LHC_Node* );

/** Function to calculate the time difference between two time interval (timeval's) */
int time_Difference(struct timeval *, struct timeval *, struct timeval *);


/*  NODE FUNCTIONS  */
/*~~~~~~~~~~~~~~~~~~*/
/** Function to create the _lhc_Node and to handle both multithreading and simulation */
void create_Node(int *_node_Start) {

	char _name_Node_File[30]="LHC_Sim_ID_Node",_aux[7];
	FILE 	*fp;

	/** Set the global MUTEX on */
	pthread_mutex_lock(&mutex);

	/** Check whether it the turn for the present node to initialize values (sequential disposition
	 *  of NODES.) */
	while(turn!= (*_node_Start-(floor(*_node_Start/10000))*10000)) {
		pthread_mutex_unlock(&mutex);
		pthread_mutex_lock(&mutex);;
	}

	/** Set environment variable */
	int _number_Of_Nodes, i=0,j=0;

	/** Create node */
	LHC_Node* _lhc_Node;

	/** Calculate the number of existing nodes from the '_node_Start' variable.
	 *  Notice each _node_Start has both the information of the total amount of
	 *  nodes and the information regarding to the identifier of the present node:
	 *  XX|XXXX   -  The first two 'X' provides information of the amount of nodes
	 *  and the remaining 'XXXX' provides information of the identifier. */
	_number_Of_Nodes  = floor(*_node_Start/10000);

	/** Allocate memory for each struct LHC-Node*/
	_lhc_Node = ( LHC_Node* ) malloc( sizeof( LHC_Node ) );
	/** Initialize the _lhc_Node parameters */
	_lhc_Node->_identifier=(*_node_Start-_number_Of_Nodes*10000);
	_lhc_Node->_number_Of_Measures=100000;
	_lhc_Node->_position = (26659/_number_Of_Nodes)*_lhc_Node->_identifier;
	_lhc_Node->_cadence = (float)(26659/_number_Of_Nodes)/299792455.3;
	/** Allocate information for the # of measures at each node (this may
	 *  be a lot of information). */
	_lhc_Node->_measures = ( Measure* ) calloc( _lhc_Node->_number_Of_Measures, sizeof( Measure ) );

	printf("Done Creating node and allocating memory - LHC-Node: %d.\n", _lhc_Node->_identifier);

	/** In case this is the last LHC_Node to be created, we set again the global
	 *  variable 'turn' to 0. We can modify the variable without problems thanks
	 *  to the 'mutex' lock we use. */
	if(_lhc_Node->_identifier<_number_Of_Nodes-1) turn++;
	else {
		printf("All the nodes are created.\n");
		turn=0;
	}
	/** Set the global MUTEX off */
	pthread_mutex_unlock(&mutex);

	/** You set the global MUTEX on */
	pthread_mutex_lock(&mutex);

	/** The thread check whether it is his turn or not...  */
	while(turn!= (*_node_Start-(floor(*_node_Start/10000))*10000)) {
		pthread_mutex_unlock(&mutex);
		pthread_mutex_lock(&mutex);
	}

	/** Set time variables to start a new simulation... */
	struct timespec _t_Start,_t_End;
	_t_Start.tv_sec = 0;
	_t_Start.tv_nsec = 0;

	_t_Global = (int)(26659/_number_Of_Nodes)/P_EXPECTED_SPEED;

	/** We calculate the 'expected' times it takes to the particles to reach the NODE again
	 *  (after going all along the LHC_Perimeter - 26659 m).
	 *  However, there are several threads that are running concurrently in the machine, each of
	 *  them trying to capture a measure. To control their correct timing at "sampling", we will
	 *  ask each node to "sleep" for a determinate time so they will let other threads 'lock'
	 *  the process until they capture  */
	_t_End.tv_sec=0;
	_t_End.tv_nsec=26659/P_EXPECTED_SPEED/10;

	/** Once all nodes are created and memory is allocated for all of them, the initial node
	 * (with identifier '0') start the simulation.
	 */
	if(turn==0){
		printf("Starting simulation in 3...\n"); sleep(1);
		printf("2...\n"); sleep(1);
		printf("1..\n"); sleep(1);
	}

	/** In order to achieve synchronization between threads...*/
	while(i<_lhc_Node->_number_Of_Measures) {

		/** Until we don't reach 'run-time' we let the node sleep 1/10 of the total running time */
		while(j<10 && turn!= (*_node_Start-(floor(*_node_Start/10000))*10000)){
			if(j<10) if(nanosleep(&_t_Start,&_t_End)<0) printf("Nanosleep failed!\n");
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex);
			j++;
		}
		j=0;
		/** Perform capture of Measures */
		_lhc_Node->_measures[i]._identifier=_lhc_Node->_identifier;
		_lhc_Node->_measures[i]._position=_lhc_Node->_position;

		/** All measures are going to be comprised between 0 and 1 to make it easier to work with them*/
		_lhc_Node->_measures[i]._particle_Radiation=(float)rand()/RAND_MAX;
		_lhc_Node->_measures[i]._particle_Speed=(float)rand()/RAND_MAX;
		_lhc_Node->_measures[i]._magnet_Current=(float)rand()/RAND_MAX;
		_lhc_Node->_measures[i]._helium_Temp=(float)rand()/RAND_MAX;
		_lhc_Node->_measures[i]._helium_Pressure =(float)rand()/RAND_MAX;
		_lhc_Node->_measures[i]._phase_RF =(float)rand()/RAND_MAX;

		/* Check which is the next node in the sequence */
		if(_lhc_Node->_identifier<=_number_Of_Nodes-1) {
				turn++;
		} else turn=0;

		i++; /** Increment the number of measures */
		pthread_mutex_unlock(&mutex);
		pthread_mutex_lock(&mutex);
	}


	/** File writing*/

	/** Select the proper name for the file...*/
	sprintf(_aux,"%d",*_node_Start);
	strcat(_name_Node_File,_aux);
	strcat(_name_Node_File,".txt");

	fp = fopen(_name_Node_File,"wb"); /** Open for writing */
	if (!fp) printf("Not able to open file %s for writing...\n",_name_Node_File );

	/** Writing all samples collected at each node...*/
	for(i=0;i<_lhc_Node->_number_Of_Measures;i++){
		fprintf(fp,"%d:%d;%f;%f;%f;%f;%f;%f;%f.\n",
				i,
				_lhc_Node->_measures[i]._identifier,
				_lhc_Node->_measures[i]._position,
				_lhc_Node->_measures[i]._particle_Radiation,
				_lhc_Node->_measures[i]._particle_Speed,
				_lhc_Node->_measures[i]._magnet_Current,
				_lhc_Node->_measures[i]._helium_Temp,
				_lhc_Node->_measures[i]._helium_Pressure,
				_lhc_Node->_measures[i]._phase_RF);
	}

	/** Close file */
	fclose(fp);

	/** Destroy all Nodes*/
	if(_lhc_Node->_identifier<=_number_Of_Nodes-1) {
		turn++;
		destroy_Node(_lhc_Node);
	}	else {
			printf("All the nodes have been destroyed.\n");
	}
	pthread_mutex_unlock(&mutex);
}

/** Function to destroy the _lhc_Node and free memory */
void destroy_Node( LHC_Node* _lhc_Node ) {

	/** Checking exist? */
	assert( _lhc_Node );

	int _node_Id = _lhc_Node->_identifier;

	_lhc_Node->_identifier 			= 0;
	_lhc_Node->_number_Of_Measures 	= 0;
	_lhc_Node->_position 			= 0;
	_lhc_Node->_cadence 			= 0.0;

	/** We free the previously allocated memory */
	free( _lhc_Node->_measures);
	free( _lhc_Node );

	printf("Node %d. Destroyed.\n", _node_Id);
}

/** Function to measure the time interval between Start and End of execution */
int time_Difference(struct timeval *_tDiff, struct timeval *_tEnd, struct timeval *_tBegin)
{
    long int diff = (_tEnd->tv_usec + 1000000 * _tEnd->tv_sec) - (_tBegin->tv_usec + 1000000 * _tBegin->tv_sec);
    _tDiff->tv_sec = diff / 1000000;
    _tDiff->tv_usec = diff % 1000000;

    return (diff<0);
}


/*  MAIN  */
/*~~~~~~~~*/
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
