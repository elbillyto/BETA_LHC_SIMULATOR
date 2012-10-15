//==============================================================================//
//  Filename: lhc_simulator.h													//
//																				//
//  Test - Application for job TE-EPC-CC-2012-154-LD @CERN.						//
//==============================================================================//
//																				//
//  Created by Carlos Ciller on Oct 13, 2012.									//
//  Copyright (c) 2012 -. All rights reserved.									//
//  Description : Written in C, Ansi-style.										//
//------------------------------------------------------------------------------//

#ifndef LHC_SIMULATOR_H_
#define LHC_SIMULATOR_H_

/* System includes */
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

/** Function to initialize a determinate node. */
void Node(int *);

/** Header to create Node...*/
void create_Node(int *);

/** Header to destroy Node...*/
void destroy_Node( LHC_Node* );

/** Function to calculate the time difference between two time interval (timeval's) */
int time_Difference(struct timeval *, struct timeval *, struct timeval *);

#endif /* LHC_SIMULATOR_H_ */
