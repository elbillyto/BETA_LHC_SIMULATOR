//==============================================================================//
//  Filename: lhc_simulator.c													//
//																				//
//  Test - Application for job TE-EPC-CC-2012-154-LD @CERN.						//
//==============================================================================//
//																				//
//  Created by Carlos Ciller on Oct 13, 2012.									//
//  Copyright (c) 2012 -. All rights reserved.									//
//  Description : Written in C, Ansi-style.										//
//------------------------------------------------------------------------------//

/* Local includes */
#include "../include/lhc_simulator.h"

#define PI 3.14159265358979

#define EV 1.60217646e-19 				/* Relation between eV and Joule. (Energy) */
#define MP 1.67262158e-27 				/* Proton Mass at "0" speed. Specified in IS convention in Kg. */
#define TEV 1e12 						/* TeV. unit adapted to IS. */
#define P_EXPECTED_SPEED 299792455.3 	/* Expected speed of the particle at the latest LHC Phase */


/** We provide information of the already existing global variables */
extern pthread_mutex_t mutex;
extern int turn;
extern int _t_Global;


/*  NODE FUNCTIONS  */
/*~~~~~~~~~~~~~~~~~~*/
/** Function to create the _lhc_Node and to handle both multithreading and simulation */
void create_Node(int *_node_Start) {

	char _name_Node_File[30]="LHC_Sim_ID_Node",_aux[7];
	FILE 	*fp;
	int		fd=0;

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
	_lhc_Node->_number_Of_Measures=1000;
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

	fp = fopen(&_name_Node_File,"wb"); /** Open for writing */
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
